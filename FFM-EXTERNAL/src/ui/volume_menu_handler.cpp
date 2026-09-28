/**
 * Volume++ Menu Toggle Handler
 * 
 * Ye file integrate karna hai apke project mein
 * Include: #include "volume_menu_handler.cpp"
 * 
 * Functionality:
 * - Volume++ button se menu toggle
 * - Bottom Bar toggle bhi kaam karega
 * - Dono methods compatible hain
 */

#include "../src/ui/bar.hpp"
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <vector>
#include <thread>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstdio>
#include <string>  // 🟢 ADD THIS LINE

// ============ EXTERNAL C-LINKAGE FOR IMGUI INTEGRATION ============
extern "C" {
    void toggle_menu_extern() {
        ui::bar::toggle();
    }
}

// ============ ANDROID KEY INPUT EVENT HANDLER ============
// Use this in imgui_impl_android.cpp ImGui_ImplAndroid_HandleInputEvent()
bool handle_android_input_event(int32_t key_code, int32_t action) {
    // key_code 24 = AKEYCODE_VOLUME_UP
    // action 0 = AKEY_EVENT_ACTION_DOWN
    if (key_code == 24 && action == 0) {
        toggle_menu_extern();
        return true;
    }
    return false;
}

// ============ NATIVE INPUT DEVICE LISTENER (Advanced) ============
namespace volume_hook {

    static void input_device_thread(int fd, std::string device_path) {
        struct input_event events[16];
        
        while (fd >= 0) {
            ssize_t bytes_read = read(fd, events, sizeof(events));
            if (bytes_read < (ssize_t)sizeof(struct input_event)) {
                if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    usleep(10000);
                    continue;
                }
                break;
            }

            size_t event_count = bytes_read / sizeof(struct input_event);
            for (size_t i = 0; i < event_count; i++) {
                // EV_KEY event with KEY_VOLUMEUP pressed (value 1)
                if (events[i].type == EV_KEY && 
                    events[i].code == KEY_VOLUMEUP && 
                    events[i].value == 1) {
                    ui::bar::toggle();
                }
            }
        }

        if (fd >= 0) {
            close(fd);
        }
    }
}

// ============ INITIALIZE VOLUME LISTENER ============
// Call this function once from main() or initialization code
void volume_listener_init() {
    const char *dir_path = "/dev/input/";
    DIR *dir = opendir(dir_path);
    
    if (!dir) {
        printf("[!] Volume listener: Unable to open /dev/input (Root/Permission required)\n");
        return;
    }

    std::vector<int> active_fds;
    struct dirent *entry;

    // Scan all event nodes
    while ((entry = readdir(dir)) != nullptr) {
        if (strstr(entry->d_name, "event")) {
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "%s%s", dir_path, entry->d_name);

            int fd = open(full_path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            // Check if device supports EV_KEY events
            unsigned char evbit[EV_MAX / 8 + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0 || 
                !(evbit[EV_KEY / 8] & (1 << (EV_KEY % 8)))) {
                close(fd);
                continue;
            }

            // Check if device supports KEY_VOLUMEUP
            unsigned char keybit[KEY_MAX / 8 + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) {
                if (keybit[KEY_VOLUMEUP / 8] & (1 << (KEY_VOLUMEUP % 8))) {
                    active_fds.push_back(fd);
                    // Spawn thread for each volume input node
                    std::thread(volume_hook::input_device_thread, fd, 
                              std::string(full_path)).detach();
                } else {
                    close(fd);
                }
            } else {
                close(fd);
            }
        }
    }
    closedir(dir);

    if (active_fds.empty()) {
        printf("[!] Volume listener: No volume nodes found (May need root)\n");
        return;
    }

    printf("[+] Volume++ listener initialized successfully\n");
}
