#include "data.hpp"
#include "Offsets.h"
#include "../other/utils.hpp"
#include "../other/memory.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <cstdio>
#include <cmath>
#include <android/log.h>
#define DLOG(...) __android_log_print(ANDROID_LOG_INFO, "ZqwData", __VA_ARGS__)

namespace data {

    std::vector<PlayerData>  players;
    std::mutex               players_mutex;
    std::atomic<uint64_t>    localPlayerAddr{0};
    std::atomic<bool>        inMatch{false};
    matrix                   viewMatrix{};

    static std::atomic<bool> g_run{false};
    static std::thread       g_thread;

    static inline bool is_valid_ptr(uint64_t p) {
        return (p >= 0x10000 && p < 0x00007FFFFFFFFFFFULL);
    }

    static inline bool is_valid_matrix(const matrix& m) {
        if (std::isnan(m.m11) || std::isnan(m.m22) || std::isnan(m.m33) || std::isnan(m.m44)) return false;
        if (std::isinf(m.m11) || std::isinf(m.m22) || std::isinf(m.m33) || std::isinf(m.m44)) return false;
        float col1 = fabsf(m.m11) + fabsf(m.m21) + fabsf(m.m31);
        float col2 = fabsf(m.m12) + fabsf(m.m22) + fabsf(m.m32);
        float col4 = fabsf(m.m14) + fabsf(m.m24) + fabsf(m.m34) + fabsf(m.m44);
        return (col1 > 0.05f && col2 > 0.05f && col4 > 0.05f);
    }

    // Lê HP num único chunk do pool (2 rpm total).
    static bool readHP(uint64_t player, int& outHP) {
        outHP = 0;
        auto pool  = rpm<uint64_t>(player + Offsets::Player_Data);
        if (!is_valid_ptr(pool)) return false;
        auto datas = rpm<uint64_t>(pool + 0x10);
        if (!is_valid_ptr(datas)) return false;
        auto elem  = rpm<uint64_t>(datas + 0x20);        // varID 0 = HP
        if (!is_valid_ptr(elem)) return false;
        outHP = rpm<int>(elem + 0x18);
        return true;
    }

    // Read Il2CppString at player+0x490 as UTF-8 into buf.
    // Filters glyphs outside our font atlas (Latin/Cyrillic ranges) so
    // decorative names like "【★ISAGI★】" collapse to "ISAGI" instead of "?".
    static int readPlayerName(uint64_t player, char* buf, size_t buflen) {
        buf[0] = 0;
        auto sp = rpm<uint64_t>(player + Offsets::Player_Name);
        if (!sp || sp < 0x10000) return 0;
        int len = rpm<int>(sp + 0x10);
        if (len <= 0 || len > 64) return 0;
        uint16_t utf16[65]; if (len > 64) len = 64;
        if (!mem_read(sp + 0x14, utf16, len * 2)) return 0;
        int n = 0;
        for (int i = 0; i < len && n < (int)buflen - 1; i++) {
            uint16_t c = utf16[i];
            // Drop anything the font atlas cannot render (CJK, kana, hangul,
            // emoji, arrows, box-drawing, etc.) — otherwise ImGui shows '?'.
            // Strict ASCII only: font atlas here has no Latin-1 Extended glyphs,
            // so anything else would render as '?'. Drops accents (José→Jos) —
            // trade-off for guaranteed clean names.
            if (c < 0x20 || c > 0x7E) continue;
            if (c < 0x80) buf[n++] = (char)c;
            else if (c < 0x800) {
                if (n + 2 >= (int)buflen) break;
                buf[n++] = 0xC0 | (c >> 6);
                buf[n++] = 0x80 | (c & 0x3F);
            } else {
                if (n + 3 >= (int)buflen) break;
                buf[n++] = 0xE0 | (c >> 12);
                buf[n++] = 0x80 | ((c >> 6) & 0x3F);
                buf[n++] = 0x80 | (c & 0x3F);
            }
        }
        // Trim leading/trailing spaces & dashes that decorative names leave behind
        while (n > 0 && (buf[n-1] == ' ' || buf[n-1] == '-' || buf[n-1] == '_')) buf[--n] = 0;
        int lead = 0;
        while (buf[lead] == ' ' || buf[lead] == '-' || buf[lead] == '_') lead++;
        if (lead > 0) { for (int k = 0; k <= n - lead; k++) buf[k] = buf[k+lead]; n -= lead; }
        buf[n] = 0;
        return n;
    }

    // Read weapon type (int): Player→WeaponOnHand→Data→Type
    static int readWeaponType(uint64_t player) {
        auto w = rpm<uint64_t>(player + Offsets::Player_WeaponOnHand);
        if (!is_valid_ptr(w)) return -1;
        auto wd = rpm<uint64_t>(w + Offsets::Weapon_Data);
        if (!is_valid_ptr(wd)) return -1;
        return rpm<int>(wd + Offsets::WeaponData_Type);
    }

    static uint64_t resolve_gamefacade_dynamic(uint64_t lib_base) {
        if (!lib_base) return 0;
        // GameFacade.CurrentMatch() RVA = 0x75ebd88, SetCurrentGame() RVA = 0x75e9240
        const uint64_t rvas[] = { 0x75ebd88, 0x75e9240 };
        for (uint64_t rva : rvas) {
            uint32_t insns[8] = {0};
            if (!mem_read(lib_base + rva, insns, sizeof(insns))) continue;

            for (int i = 0; i < 6; i++) {
                uint32_t insn1 = insns[i];
                // ADRP: (insn & 0x9F000000) == 0x90000000
                if ((insn1 & 0x9F000000) == 0x90000000) {
                    int reg_adrp = insn1 & 0x1F;
                    int64_t immlo = (insn1 >> 29) & 0x3;
                    int64_t immhi = (insn1 >> 5) & 0x7FFFF;
                    int64_t imm = (immhi << 2) | immlo;
                    if (imm & (1 << 20)) imm |= ~((int64_t)0x1FFFFF); // sign extend
                    uint64_t pc = lib_base + rva + (i * 4);
                    uint64_t page = (pc & ~0xFFFULL) + (imm << 12);

                    for (int j = i + 1; j < i + 4 && j < 8; j++) {
                        uint32_t insn2 = insns[j];
                        // LDR (64-bit unsigned imm): (insn & 0xFFC00000) == 0xF9400000
                        if ((insn2 & 0xFFC00000) == 0xF9400000) {
                            int rn = (insn2 >> 5) & 0x1F;
                            if (rn == reg_adrp) {
                                uint64_t imm12 = ((insn2 >> 10) & 0xFFF) << 3;
                                uint64_t target_addr = page + imm12;
                                if (target_addr >= lib_base) {
                                    uint64_t offset = target_addr - lib_base;
                                    if (offset > 0x100000 && offset < 0x20000000) {
                                        return offset;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return 0;
    }

    void update() {
        static bool s_base_logged = false;
        static bool s_base_not_logged = false;
        static bool s_chain_logged = false;

        if (proc::pid <= 0 || proc::lib == 0) {
            if (!s_base_not_logged) {
                printf("[!] Base address not found\n");
                fflush(stdout);
                s_base_not_logged = true;
                s_base_logged = false;
            }
            if (s_chain_logged) {
                s_chain_logged = false;
            }
            return;
        }

        if (!s_base_logged) {
            printf("[+] Valid base address found: 0x%lx\n", (unsigned long)proc::lib);
            fflush(stdout);
            s_base_logged = true;
            s_base_not_logged = false;
        }

        std::vector<PlayerData> tmp;
        tmp.reserve(64);

        // Resolve GameFacade offset (header + dynamic ARM64 check)
        uint64_t gf_offset = Offsets::GameFacade;
        static uint64_t s_dyn_gf = 0;
        static bool s_dyn_checked = false;
        if (!s_dyn_checked && proc::lib != 0) {
            s_dyn_checked = true;
            s_dyn_gf = resolve_gamefacade_dynamic(proc::lib);
        }

        // Base chain: GameFacade -> StaticFields -> BaseGame (matching hook.h)
        uint64_t gf_ptr = proc::lib + gf_offset;
        uint64_t GfCell = rpm<uint64_t>(gf_ptr);
        if (!is_valid_ptr(GfCell) && s_dyn_gf != 0 && s_dyn_gf != gf_offset) {
            gf_offset = s_dyn_gf;
            gf_ptr = proc::lib + gf_offset;
            GfCell = rpm<uint64_t>(gf_ptr);
        }

        if (!is_valid_ptr(GfCell)) {
            if (s_chain_logged) s_chain_logged = false;
            return;
        }

        uint64_t BaseGame = 0;
        uint64_t sf1 = 0, curGame1 = 0, curMatch1 = 0;
        uint64_t classPtr = 0, sf2 = 0;

        // Path A: GfCell is GameFacade Il2CppClass directly (matching hook.h: P2 = Read(GameFacade_c + 0xB8); BaseGame = Read(P2))
        sf1 = rpm<uint64_t>(GfCell + Offsets::Staticfield);
        if (is_valid_ptr(sf1)) {
            curGame1 = rpm<uint64_t>(sf1 + 0x0);
            curMatch1 = rpm<uint64_t>(sf1 + 0x8);
            if (is_valid_ptr(curGame1)) BaseGame = curGame1;
            else if (is_valid_ptr(curMatch1)) BaseGame = curMatch1;
        }

        // Path B: GfCell is a pointer to GameFacade Il2CppClass (pointer indirection fallback)
        if (!is_valid_ptr(BaseGame)) {
            classPtr = rpm<uint64_t>(GfCell);
            if (is_valid_ptr(classPtr)) {
                sf2 = rpm<uint64_t>(classPtr + Offsets::Staticfield);
                if (is_valid_ptr(sf2)) {
                    uint64_t cg2 = rpm<uint64_t>(sf2 + 0x0);
                    uint64_t cm2 = rpm<uint64_t>(sf2 + 0x8);
                    if (is_valid_ptr(cg2)) { BaseGame = cg2; sf1 = sf2; curGame1 = cg2; }
                    else if (is_valid_ptr(cm2)) { BaseGame = cm2; sf1 = sf2; curMatch1 = cm2; }
                }
            }
        }

        if (!is_valid_ptr(BaseGame)) {
            if (s_chain_logged) s_chain_logged = false;
            return;
        }

        // CurrentMatch & localPlayer (hook.h: m_Match = BaseGame + 0x90; localPlayer = m_Match + 0xD8)
        auto m_Match = rpm<uint64_t>(BaseGame + Offsets::CurrentMatch);
        uint64_t localPlayer = 0;
        if (is_valid_ptr(m_Match)) {
            localPlayer = rpm<uint64_t>(m_Match + Offsets::localPlayer);
        }

        // Active match determination (hook.h line 726: MatchIsRunning == 1)
        int matchState = is_valid_ptr(m_Match) ? rpm<int>(m_Match + Offsets::MatchIsRunning) : -1;
        bool matchActive = (matchState == 1) || is_valid_ptr(localPlayer);
        inMatch.store(matchActive);

        if (!matchActive) {
            if (s_chain_logged) s_chain_logged = false;
            localPlayerAddr.store(0);
            std::lock_guard<std::mutex> lk(players_mutex);
            players.clear();
            return;
        }

        // Camera & ViewMatrix resolution
        // Candidates based on dump.h & offset.h:
        // 1. FollowCamera + 0x30 (AFMGCAELKBD - Camera in CameraControllerBase)
        // 2. BaseGame + 0xD8 (CameraBase: m_CameraControllerManager) -> 0x20 (Camera EKIPKGNKILM)
        // 3. FollowCamera + 0x20 (m_Manager) -> 0x20 (Camera EKIPKGNKILM)
        // 4. BaseGame + 0xD8 -> 0x28 (Camera LBHAFMEPGCO)
        // 5. FollowCamera + 0x20 directly (legacy fallback)
        uint64_t Camera = 0, FollowCamera = 0;
        uint64_t IntPtrCam = 0;
        const char* camSourceName = "NONE";
        bool matrix_valid = false;

        auto try_camera = [&](uint64_t candidate, const char* name) -> bool {
            if (!is_valid_ptr(candidate)) return false;
            uint64_t intPtr = rpm<uint64_t>(candidate + Offsets::IntPtrCam); // 0x10
            if (!is_valid_ptr(intPtr)) return false;
            matrix vm = rpm<matrix>(intPtr + Offsets::Matrix); // 0x100
            if (is_valid_matrix(vm)) {
                Camera = candidate;
                IntPtrCam = intPtr;
                viewMatrix = vm;
                camSourceName = name;
                matrix_valid = true;
                return true;
            }
            return false;
        };

        if (is_valid_ptr(localPlayer)) {
            FollowCamera = rpm<uint64_t>(localPlayer + Offsets::FollowCamera);
        }

        // Candidate 1: FollowCamera + 0x30
        if (is_valid_ptr(FollowCamera)) {
            uint64_t c30 = rpm<uint64_t>(FollowCamera + 0x30);
            if (try_camera(c30, "FollowCam+0x30 (AFMGCAELKBD)")) goto cam_resolved;
        }

        // Candidate 2: BaseGame + 0xD8 -> +0x20
        if (is_valid_ptr(BaseGame)) {
            uint64_t camMgr = rpm<uint64_t>(BaseGame + Offsets::CameraBase); // 0xD8
            if (is_valid_ptr(camMgr)) {
                uint64_t c20 = rpm<uint64_t>(camMgr + Offsets::Camera); // 0x20
                if (try_camera(c20, "BaseGame+0xD8->+0x20 (EKIPKGNKILM)")) goto cam_resolved;
                uint64_t c28 = rpm<uint64_t>(camMgr + 0x28);
                if (try_camera(c28, "BaseGame+0xD8->+0x28 (LBHAFMEPGCO)")) goto cam_resolved;
            }
        }

        // Candidate 3: FollowCamera + 0x20 -> +0x20
        if (is_valid_ptr(FollowCamera)) {
            uint64_t mgr = rpm<uint64_t>(FollowCamera + Offsets::Camera); // 0x20
            if (is_valid_ptr(mgr)) {
                uint64_t c20 = rpm<uint64_t>(mgr + 0x20);
                if (try_camera(c20, "FollowCam->m_Manager->+0x20")) goto cam_resolved;
            }
            // Candidate 4: FollowCamera + 0x20 directly
            if (try_camera(mgr, "FollowCam+0x20 (legacy)")) goto cam_resolved;
        }

    cam_resolved:
        localPlayerAddr.store(localPlayer ? localPlayer : 0);

        if (!matrix_valid) {
            if (s_chain_logged) s_chain_logged = false;
            return;
        }

        // Dictionary & entitylist (hook.h: dictionary = BaseGame + 0xC0; entitylist = dictionary + 0x18)
        uint64_t dictionary = rpm<uint64_t>(BaseGame + Offsets::Dictionary);
        if (!is_valid_ptr(dictionary) && is_valid_ptr(m_Match)) {
            dictionary = rpm<uint64_t>(m_Match + Offsets::Dictionary);
        }
        if (!is_valid_ptr(dictionary)) {
            if (s_chain_logged) s_chain_logged = false;
            return;
        }

        uint64_t entitylist = rpm<uint64_t>(dictionary + Offsets::entryAddr); // 0x18
        if (!is_valid_ptr(entitylist)) {
            entitylist = rpm<uint64_t>(dictionary + Offsets::enemyList); // 0x10 fallback
        }
        if (!is_valid_ptr(entitylist)) {
            entitylist = dictionary;
        }

        if (!s_chain_logged) {
            printf("[+] Pointer chain worked successfully!\n");
            fflush(stdout);
            s_chain_logged = true;
        }

        Vector3 localPos = Vector3::Zero();
        if (is_valid_ptr(localPlayer)) localPos = GetNodePosition(GetPlayerHeadTF(localPlayer));

        Vector3 sample_headWorld = Vector3::Zero();
        Vector3 sample_headScreen = Vector3::Zero();
        Vector3 sample_toeWorld = Vector3::Zero();
        Vector3 sample_toeScreen = Vector3::Zero();
        int sample_enemy_idx = -1;

        int _cnt_enter = 0, _cnt_dead = 0, _cnt_avatar = 0, _cnt_invis = 0, _cnt_team = 0, _cnt_headzero = 0, _cnt_dist = 0, _cnt_screen = 0, _cnt_pushed = 0;
        // Loop through entity pointer array (hook.h line 742: entitylist + i * 8)
        for (int i = 0; i < 500; i++) {
            uint64_t enemy = rpm<uint64_t>(entitylist + i * sizeof(uint64_t));
            if (!is_valid_ptr(enemy) || enemy == localPlayer) continue;
            _cnt_enter++;

            // 1. AvatarManager validation (hook.h line 746: AvatarManager = Read(enemy + 0x778))
            auto avatarMgr = rpm<uint64_t>(enemy + Offsets::AvatarManager); // 0x778
            if (!is_valid_ptr(avatarMgr)) continue;
            _cnt_avatar++;

            // 2. UmaAvatarSimple (hook.h line 748: UmaAvatarSimple = Read(avatarMgr + 0x138))
            auto umaSimple = rpm<uint64_t>(avatarMgr + Offsets::Avatar); // 0x138
            if (!is_valid_ptr(umaSimple)) continue;

            // 3. IsVisible check (hook.h line 750: if (IsVisible == 1))
            uint8_t isVis = rpm<uint8_t>(umaSimple + Offsets::Avatar_IsVisible); // 0x101
            if (isVis != 1) { _cnt_invis++; continue; }

            // 4. UmaData & Team check (hook.h line 752, 767)
            auto umaData = rpm<uint64_t>(umaSimple + Offsets::Avatar_Data); // 0x28
            if (!is_valid_ptr(umaData)) { _cnt_invis++; continue; }
            if (rpm<uint8_t>(umaData + Offsets::Avatar_Data_IsTeam) != 0) { _cnt_team++; continue; }

            // 5. Dead check (hook.h line 768: if (Player_isdead != 0) continue)
            if (rpm<uint8_t>(enemy + Offsets::Player_IsDead) != 0) { _cnt_dead++; continue; } // 0x7C

            // 5.5 PhyXdata gate (hook.h line 754-755: if (phyxData != 0) { ... })
            // In old ff, the ENTIRE ESP drawing block is nested inside this gate.
            // If phyxData is null the entity is completely skipped.
            auto phyxData = rpm<uint64_t>(enemy + Offsets::PhyXdata); // 0x1D60
            if (!phyxData) { _cnt_dead++; continue; }

            // Knocked check (hook.h lines 756-758)
            bool isKnocked = false;
            {
                auto maybeDead = rpm<uint64_t>(phyxData + Offsets::maybeDead); // 0x20
                if (maybeDead && rpm<int>(maybeDead + Offsets::isKnocked) == 8) // 0x10
                    isKnocked = true;
            }

            // 6. Camera check (hook.h line 772-773: if (CameraPosition == Vector3::Zero()) continue)
            auto camTF = rpm<uint64_t>(enemy + Offsets::MainCameraTransform); // 0x3F0
            if (is_valid_ptr(camTF)) {
                Vector3 camPos = GetPosition(camTF);
                if (camPos == Vector3::Zero()) { _cnt_dead++; continue; }
            } else {
                _cnt_dead++;
                continue;
            }

            // 7. HP check: if HP <= 0, enemy is confirmed dead
            int hp = 0;
            bool hasHP = readHP(enemy, hp);
            if (hasHP && hp <= 0) { _cnt_dead++; continue; }
            if (!hasHP || hp <= 0) hp = 100;

            bool isBot = rpm<bool>(enemy + Offsets::IsBot);

            // 8. Bone position validation (hook.h line 770-773, 781)
            auto headTF = GetPlayerHeadTF(enemy); // 0x6A8
            if (!is_valid_ptr(headTF)) { _cnt_headzero++; continue; }
            auto headWorld = GetNodePosition(headTF);
            if (headWorld == Vector3::Zero()) { _cnt_headzero++; continue; }

            auto spineTF = GetPlayerSpineTF(enemy); // 0x6B8
            Vector3 spineWorld = Vector3::Zero();
            if (is_valid_ptr(spineTF)) {
                spineWorld = GetNodePosition(spineTF);
            }
            if (spineWorld == Vector3::Zero()) {
                spineWorld = headWorld;
            }
            Vector3 neckWorld = Vector3::Lerp(headWorld, spineWorld, 0.35f);

            auto feetTF = GetPlayerPeTF(enemy); // 0x6D0
            if (!is_valid_ptr(feetTF)) { _cnt_headzero++; continue; }
            auto feetWorld = GetNodePosition(feetTF);
            if (feetWorld == Vector3::Zero()) { _cnt_headzero++; continue; }

            float dist = (localPos != Vector3::Zero()) ? calculate_distance(localPos, headWorld) : 10.f;
            if (dist > cfg::esp::max_range) { _cnt_dist++; continue; }

            auto headScreen = WorldToScreenPoint(viewMatrix, headWorld);
            if (headScreen.x < 0) { _cnt_screen++; continue; }

            auto feetScreen = WorldToScreenPoint(viewMatrix, feetWorld);
            if (feetScreen.x < 0) { _cnt_screen++; continue; }

            if (sample_enemy_idx < 0) {
                sample_enemy_idx = i;
                sample_headWorld = headWorld;
                sample_headScreen = headScreen;
                sample_toeWorld = feetWorld;
                sample_toeScreen = feetScreen;
            }

            _cnt_pushed++;
            PlayerData pd;
            readPlayerName(enemy, pd.name, sizeof(pd.name));
            pd.weaponType = readWeaponType(enemy);
            if (!pd.name[0] && isBot) {
                snprintf(pd.name, sizeof(pd.name), "BOT");
            }
            pd.addr       = enemy;
            pd.Head       = headScreen;
            pd.Toe        = feetScreen;
            pd.headWorld  = headWorld;
            pd.spineWorld = spineWorld;
            pd.neckWorld  = neckWorld;
            pd.isKnocked  = isKnocked;
            pd.isVisible  = (isVis == 1);
            pd.isBot      = isBot;
            pd.distance   = dist;
            pd.curHP      = hp;
            pd.maxHP      = 200;
            tmp.push_back(pd);
        }

        /*
        if (do_print) {
            DBG_LOG("=== [OFFSET VALIDATION REPORT] ===");
            DBG_LOG("[+] PID=%d | lib=0x%lx", proc::pid, (unsigned long)proc::lib);
            DBG_LOG("[*] GfCell=0x%lx | BaseGame=0x%lx | m_Match=0x%lx | localPlayer=0x%lx",
                    (unsigned long)GfCell, (unsigned long)BaseGame, (unsigned long)m_Match, (unsigned long)localPlayer);
            DBG_LOG("[*] CamSource: %s | Camera=0x%lx | IntPtrCam=0x%lx",
                    camSourceName, (unsigned long)Camera, (unsigned long)IntPtrCam);
            DBG_LOG("[*] ViewMatrix: [m11=%.3f, m12=%.3f, m14=%.3f] [m21=%.3f, m22=%.3f, m24=%.3f] [m41=%.3f, m42=%.3f, m44=%.3f]",
                    viewMatrix.m11, viewMatrix.m12, viewMatrix.m14,
                    viewMatrix.m21, viewMatrix.m22, viewMatrix.m24,
                    viewMatrix.m41, viewMatrix.m42, viewMatrix.m44);
            DBG_LOG("[*] LocalPos=(%.1f, %.1f, %.1f) | Screen=(%.0fx%.0f, center=%.0f,%.0f)",
                    localPos.x, localPos.y, localPos.z, g_sw, g_sh, g_sw * 0.5f, g_sh * 0.5f);
            if (sample_enemy_idx >= 0) {
                DBG_LOG("[*] Sample Enemy[%d]: headW=(%.1f, %.1f, %.1f) -> headScr=(%.1f, %.1f) | toeW=(%.1f, %.1f, %.1f) -> toeScr=(%.1f, %.1f)",
                        sample_enemy_idx,
                        sample_headWorld.x, sample_headWorld.y, sample_headWorld.z,
                        sample_headScreen.x, sample_headScreen.y,
                        sample_toeWorld.x, sample_toeWorld.y, sample_toeWorld.z,
                        sample_toeScreen.x, sample_toeScreen.y);
            }
            DBG_LOG("[*] Dictionary=0x%lx | entitylist=0x%lx", (unsigned long)dictionary, (unsigned long)entitylist);
            DBG_LOG("[*] Loop: enter=%d avatarMgr=%d invis_skip=%d team_skip=%d dead_skip=%d headzero=%d dist_skip=%d screen_fail=%d PUSHED=%d",
                    _cnt_enter, _cnt_avatar, _cnt_invis, _cnt_team, _cnt_dead, _cnt_headzero, _cnt_dist, _cnt_screen, _cnt_pushed);
            DBG_LOG("==================================");
        }
        */
        {
            std::lock_guard<std::mutex> lk(players_mutex);
            players = std::move(tmp);
        }
    }

    // Thread stealth: intervalo em torno de 15ms com jitter simples, evita cadência fixa.
    static void loop() {
        uint32_t seed = 0x9E3779B1u;
        while (g_run.load(std::memory_order_relaxed)) {
            update();
            seed = seed * 1664525u + 1013904223u;
            int jitter = (seed >> 24) & 7;                 // 0..7 ms
            std::this_thread::sleep_for(std::chrono::milliseconds(12 + jitter));
        }
    }

    void thread_start() {
        if (g_run.exchange(true)) return;
        g_thread = std::thread(loop);
    }

    void thread_stop() {
        if (!g_run.exchange(false)) return;
        if (g_thread.joinable()) g_thread.join();
        std::lock_guard<std::mutex> lk(players_mutex);
        players.clear();
    }
}
