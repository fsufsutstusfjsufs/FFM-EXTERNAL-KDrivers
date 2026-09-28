// Neo FF — ESP, Silent Aim & ImGui Menu
#include "Android_draw/draw.h"
#include "ui/styles/styles.h"
#include "ui/menu.hpp"
#include "ui/volume_menu_handler.h"     // 🟢 CHANGE: Include .h file instead of .cpp
#include "other/memory.hpp"
#include "game/data.hpp"
#include "func/esp.hpp"
#include "func/silent.hpp"
#include "ui/cfg.hpp"
#include <cstdio>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "Neo FF", __VA_ARGS__)

static void launch_target() {
    system("monkey -p com.dts.freefiremax -c android.intent.category.LAUNCHER 1");
}

int main() {
    LOG("main start");
    screen_config();
    LOG("screen: %dx%d orient=%u", displayInfo.width, displayInfo.height, displayInfo.orientation);

    int max_size = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    int min_size = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        LOG("initGUI FAILED"); return -1;
    }
    LOG("initGUI OK (%dx%d)", native_window_screen_x, native_window_screen_y);

    // 🟢 ADD THIS LINE: Initialize Volume+ menu listener
    volume_listener_init();
    LOG("Volume++ listener initialized");

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    launch_target();
    game::init();
    data::thread_start();
    silent::thread_start();

    int frame = 0;
    while (true) {
        auto frame_start = std::chrono::steady_clock::now();

        drawBegin();
        bool run = game::valid();
        if ((frame++ % 240) == 0) {
            /*
            uint32_t magic = rpm<uint32_t>(proc::lib);
            LOG("frame %d run=%d players=%zu", frame, (int)run, data::players.size());
            printf("[Neo FF] frame %d | run=%d (PID=%d, lib=0x%lx, magic=0x%08x [%s]) | players=%zu\n",
                   frame, (int)run, proc::pid, (unsigned long)proc::lib, magic,
                   (magic == 0x464C457F ? "ELF OK" : "WRONG BASE"), data::players.size());
            fflush(stdout);
            */
        }
        ui::menu::render();
        drawEnd();

        // Thermal Frame Pacing (60 / 90 / 120 / Max)
        int target_fps = cfg::settings::get_target_fps();
        if (target_fps > 0) {
            int64_t target_us = 1000000 / target_fps;
            auto frame_end = std::chrono::steady_clock::now();
            int64_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start).count();
            if (elapsed_us < target_us) {
                usleep(static_cast<useconds_t>(target_us - elapsed_us));
            }
        } else {
            usleep(1000); // Uncapped mode yielding
        }
    }
    return 0;
}
