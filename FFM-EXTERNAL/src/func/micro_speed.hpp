#pragma once
#include "../other/addr.hpp"
#include "../other/memory.hpp"

namespace func {
    static constexpr uint64_t MICRO_SPEED_BASE = 0xF1D5F70;
    static constexpr uint64_t MICRO_SPEED_OFFSETS[] = { 0x30, 0x4B8, 0x1168 };
    static constexpr int MS_OFFSET_COUNT = sizeof(MICRO_SPEED_OFFSETS) / sizeof(MICRO_SPEED_OFFSETS[0]);

    inline float get_micro_speed() {
        if (proc::lib == 0) return 0.0f;
        uint64_t addr = find_addr(MICRO_SPEED_BASE, MICRO_SPEED_OFFSETS, MS_OFFSET_COUNT);
        if (addr == 0) return 0.0f;
        return rpm<float>(addr);
    }

    inline void set_micro_speed(float value) {
        uint64_t addr = find_addr(MICRO_SPEED_BASE, MICRO_SPEED_OFFSETS, MS_OFFSET_COUNT);
        if (addr == 0) return;
        
        wpm<float>(addr, value);
    }
}
