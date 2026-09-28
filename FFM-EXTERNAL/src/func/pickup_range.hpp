#pragma once
#include "../other/addr.hpp"
#include "../other/memory.hpp"

namespace func {
    static constexpr uint64_t PICKUP_RANGE_BASE = 0xF1D5F70;
    static constexpr uint64_t PICKUP_RANGE_OFFSETS[] = { 0x30, 0x4B8, 0x1EB4 };
    static constexpr int PR_OFFSET_COUNT = sizeof(PICKUP_RANGE_OFFSETS) / sizeof(PICKUP_RANGE_OFFSETS[0]);

    inline float get_pickup_range() {
        if (proc::lib == 0) return 0.0f;
        uint64_t addr = find_addr(PICKUP_RANGE_BASE, PICKUP_RANGE_OFFSETS, PR_OFFSET_COUNT);
        if (addr == 0) return 0.0f;
        return rpm<float>(addr);
    }

    inline void set_pickup_range(float value) {
        uint64_t addr = find_addr(PICKUP_RANGE_BASE, PICKUP_RANGE_OFFSETS, PR_OFFSET_COUNT);
        if (addr == 0) return;
        wpm<float>(addr, value);
    }
}
