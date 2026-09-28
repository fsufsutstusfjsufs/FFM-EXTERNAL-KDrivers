#pragma once
#include "../other/addr.hpp"
#include "../other/memory.hpp"

namespace func {
    static constexpr uint64_t MAX_STEP_HEIGHT_BASE = 0xF1D5F70;
    static constexpr uint64_t MAX_STEP_HEIGHT_OFFSETS[] = { 0x30, 0x4B8, 0x518, 0x218 };
    static constexpr int MSH_OFFSET_COUNT = sizeof(MAX_STEP_HEIGHT_OFFSETS) / sizeof(MAX_STEP_HEIGHT_OFFSETS[0]);

    inline float get_max_step_height() {
        if (proc::lib == 0) return 0.0f;
        uint64_t addr = find_addr(MAX_STEP_HEIGHT_BASE, MAX_STEP_HEIGHT_OFFSETS, MSH_OFFSET_COUNT);
        if (addr == 0) return 0.0f;
        return rpm<float>(addr);
    }

    inline void set_max_step_height(float value) {
        uint64_t addr = find_addr(MAX_STEP_HEIGHT_BASE, MAX_STEP_HEIGHT_OFFSETS, MSH_OFFSET_COUNT);
        if (addr == 0) return;
        wpm<float>(addr, value);
    }
}
