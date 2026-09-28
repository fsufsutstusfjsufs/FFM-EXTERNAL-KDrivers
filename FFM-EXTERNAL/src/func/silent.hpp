#pragma once

#include <cmath>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <thread>
#include <sched.h>
#include <unistd.h>

#include "../other/memory.hpp"
#include "../other/vector3.h"
#include "../game/Offsets.h"
#include "../game/math.hpp"
#include "../game/data.hpp"
#include "../ui/cfg.hpp"

// Silent Aim Architecture — matched to Scarecrow hook.h
// Two-Thread Producer / Consumer design:
// 1. Writer Thread (SilentAimThread):
//    Dedicated high-speed thread. When firing (g_hasData == true), runs in a zero-delay
//    tight loop, continuously writing g_targetDir to g_hitObj + Offsets::raycast.
//    This prevents the game engine from overwriting the raycast vector before the bullet spawns.
// 2. Producer Thread:
//    Reads localPlayer and selects closest target in FOV (from data::players cached headWorld).
//    Checks isFiring. If firing, computes target direction (targetPos - ammoBase) and sets
//    g_hitObj, g_targetDir, g_hasData. If not firing, clears silent aim data immediately.

namespace silent {

    inline std::atomic<bool> g_run{false};
    inline std::thread       g_writer_thread;
    inline std::thread       g_producer_thread;

    // Shared state between Producer and Writer (matching hook.h)
    inline std::mutex        g_silent_lock;
    inline uint64_t          g_hitObj = 0;
    inline Vector3           g_targetDir = {0.f, 0.f, 0.f};
    inline std::atomic<bool> g_hasData{false};

    // Public target for UI overlay
    // Public target for UI overlay
    struct Target {
        uint64_t addr;
        Vector3  targetScreen;
        float    distance;
        float    fovDelta;
    };
    inline Target      g_target{ 0, {}, 0.f, 0.f };
    inline std::mutex  g_target_mutex;

    inline void clear_silent_data() {
        if (g_hasData.load(std::memory_order_relaxed) || g_hitObj != 0) {
            std::lock_guard<std::mutex> lock(g_silent_lock);
            g_hasData.store(false, std::memory_order_relaxed);
            g_hitObj = 0;
        }
    }

    // High-speed writer thread — directly equivalent to SilentAimThread() in hook.h
    static void writer_loop() {
        while (g_run.load(std::memory_order_relaxed)) {
            if (!g_hasData.load(std::memory_order_relaxed) || g_hitObj == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Zero-sleep continuous memory flooding while shooting
            {
                std::lock_guard<std::mutex> lock(g_silent_lock);
                if (g_hasData.load(std::memory_order_relaxed) && g_hitObj != 0) {
                    wpm<Vector3>(g_hitObj + Offsets::raycast, g_targetDir);
                }
            }
            sched_yield();
        }
    }

    // Fast target selector using cached PlayerData (no redundant RPM calls)
    static inline uint64_t select_target(Vector3& outTargetWorld, Vector3& outTargetScreen,
                                         float& outDist, float& outFov) {
        float fov  = cfg::aim::visible_fov;
        float maxd = cfg::aim::visible_max;
        float cx = g_sw * 0.5f;
        float cy = g_sh * 0.5f;
        float bestFov = fov;
        uint64_t bestAddr = 0;

        std::lock_guard<std::mutex> lk(data::players_mutex);
        for (const auto& p : data::players) {
            if (!p.addr || p.curHP <= 0) continue;
            if (!p.isVisible) continue;
            if (p.isKnocked && !cfg::aim::target_knocked) continue;
            if (p.distance > maxd) continue;

            Vector3 targetWorld = p.getTargetWorld(cfg::aim::target_position);
            if (targetWorld == Vector3::Zero()) continue;

            Vector3 targetScreen = WorldToScreenPoint(data::viewMatrix, targetWorld);
            if (targetScreen.x < 1 || targetScreen.y < 1) continue;

            float dx = targetScreen.x - cx;
            float dy = targetScreen.y - cy;
            float d  = sqrtf(dx * dx + dy * dy);
            if (d > fov) continue;

            if (d < bestFov) {
                bestFov = d;
                bestAddr = p.addr;
                outTargetScreen = targetScreen;
                outTargetWorld  = targetWorld;
                outDist = p.distance;
                outFov = d;
            }
        }
        return bestAddr;
    }

    // Producer loop: updates target and computes aim direction
    static void producer_loop() {
        while (g_run.load(std::memory_order_relaxed)) {
            if (!cfg::aim::visible_enabled || !data::inMatch.load()) {
                clear_silent_data();
                usleep(8000);
                continue;
            }

            uint64_t localPlayer = data::localPlayerAddr.load();
            if (!localPlayer) {
                clear_silent_data();
                usleep(6000);
                continue;
            }

            Vector3 targetWorld = Vector3::Zero(), targetScreen = Vector3::Zero();
            float dist = 0.f, fov = 0.f;
            uint64_t enemy = select_target(targetWorld, targetScreen, dist, fov);
            {
                std::lock_guard<std::mutex> lk(g_target_mutex);
                g_target = { enemy, targetScreen, dist, fov };
            }

            uint64_t hitObjAddress = rpm<uint64_t>(localPlayer + Offsets::aimInfo);
            int isFiring = rpm<int>(localPlayer + Offsets::isFiring);

            if (enemy && isFiring > 0 && hitObjAddress != 0 && !(targetWorld == Vector3::Zero())) {
                Vector3 ammoBase = rpm<Vector3>(hitObjAddress + Offsets::startPos);
                Vector3 dir;
                dir.x = targetWorld.x - ammoBase.x;
                dir.y = (targetWorld.y + cfg::aim::head_bias) - ammoBase.y;
                dir.z = targetWorld.z - ammoBase.z;

                {
                    std::lock_guard<std::mutex> lock(g_silent_lock);
                    g_hitObj = hitObjAddress;
                    g_targetDir = dir;
                    g_hasData.store(true, std::memory_order_relaxed);
                }
                // Very brief yield during active firing so writer thread has full throughput
                usleep(1000);
            } else {
                clear_silent_data();
                usleep(3000);
            }
        }
    }

    inline void thread_start() {
        if (g_run.exchange(true)) return;
        g_writer_thread   = std::thread(writer_loop);
        g_producer_thread = std::thread(producer_loop);
    }

    inline void thread_stop() {
        if (!g_run.exchange(false)) return;
        clear_silent_data();
        if (g_writer_thread.joinable())   g_writer_thread.join();
        if (g_producer_thread.joinable()) g_producer_thread.join();
    }

    inline void draw_overlay() {
        if (!cfg::aim::visible_enabled) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 center(g_sw * 0.5f, g_sh * 0.5f);

        if (cfg::aim::fov_circle) {
            ImU32 c = ImGui::ColorConvertFloat4ToU32(cfg::aim::fov_col);
            dl->AddCircle(center, cfg::aim::visible_fov, c, 64, 1.5f);
        }

        Target t;
        { std::lock_guard<std::mutex> lk(g_target_mutex); t = g_target; }
        if (t.addr && t.targetScreen.x > 1) {
            ImVec2 pos(t.targetScreen.x, t.targetScreen.y);
            dl->AddCircle(pos, 12.f, IM_COL32(255, 60, 60, 220), 24, 2.f);
            dl->AddCircleFilled(pos, 3.f, IM_COL32(255, 255, 255, 255));
        }
    }
}
