#pragma once
#include "../other/mem.hpp"
namespace func {
    static constexpr uint64_t IPAD_OFFSET = 0x36C9BD0;

    inline float get_ipad() {
        if (proc::lib == 0) return 1.0f;
        int fd = get_mem_fd();
        float val = 1.0f;
        if (fd >= 0) pread(fd, &val, sizeof(float), proc::lib + IPAD_OFFSET);
        return val;
    }

    inline void set_ipad(float value) {
        int fd = get_mem_fd();
        if (fd < 0) return;
        pwrite(fd, &value, sizeof(float), proc::lib + IPAD_OFFSET);
    }
}
