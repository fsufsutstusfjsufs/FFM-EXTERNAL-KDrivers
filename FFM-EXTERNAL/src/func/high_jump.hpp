#pragma once
#include "../other/addr.hpp"
#include "../other/memory.hpp"

namespace func {
    static constexpr uint64_t HIGH_JUMP_BASE = 0xF1D5F70;
    static constexpr uint64_t HIGH_JUMP_OFFSETS[] = { 0x30, 0x4B8, 0x518, 0x21C };
    static constexpr int HJ_OFFSET_COUNT = sizeof(HIGH_JUMP_OFFSETS) / sizeof(HIGH_JUMP_OFFSETS[0]);

    inline float get_high_jump() {
        if (proc::lib == 0) return 0.0f;
        uint64_t addr = find_addr(HIGH_JUMP_BASE, HIGH_JUMP_OFFSETS, HJ_OFFSET_COUNT);
        if (addr == 0) return 0.0f;
        return rpm<float>(addr);
    }

    inline void set_high_jump(float value) {
        uint64_t addr = find_addr(HIGH_JUMP_BASE, HIGH_JUMP_OFFSETS, HJ_OFFSET_COUNT);
        if (addr == 0) return;
        wpm<float>(addr, value);
    }
}
