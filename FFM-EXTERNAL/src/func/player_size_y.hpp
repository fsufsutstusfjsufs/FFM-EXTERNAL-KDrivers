#pragma once
#include "../other/addr.hpp"
#include "../other/memory.hpp"

namespace func {
    static constexpr uint64_t PLAYER_SIZE_Y_BASE = 0xF1D5F70;
    static constexpr uint64_t PLAYER_SIZE_Y_OFFSETS[] = { 0x30, 0x4B8, 0x510, 0x200 };
    static constexpr int PSY_OFFSET_COUNT = sizeof(PLAYER_SIZE_Y_OFFSETS) / sizeof(PLAYER_SIZE_Y_OFFSETS[0]);

    inline float get_player_size_y() {
        if (proc::lib == 0) return 0.0f;
        uint64_t addr = find_addr(PLAYER_SIZE_Y_BASE, PLAYER_SIZE_Y_OFFSETS, PSY_OFFSET_COUNT);
        if (addr == 0) return 0.0f;
        return rpm<float>(addr);
    }

    inline void set_player_size_y(float value) {
        uint64_t addr = find_addr(PLAYER_SIZE_Y_BASE, PLAYER_SIZE_Y_OFFSETS, PSY_OFFSET_COUNT);
        if (addr == 0) return;
        
        wpm<float>(addr, value);
    }
}
