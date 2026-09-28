#pragma once
#include "../other/addr.hpp"
#include "../other/memory.hpp"

namespace func {
    static constexpr uint64_t PLAYER_SIZE_Z_BASE = 0xF1D5F70;
    static constexpr uint64_t PLAYER_SIZE_Z_OFFSETS[] = { 0x30, 0x4B8, 0x510, 0x204 };
    static constexpr int PSZ_OFFSET_COUNT = sizeof(PLAYER_SIZE_Z_OFFSETS) / sizeof(PLAYER_SIZE_Z_OFFSETS[0]);

    inline float get_player_size_z() {
        if (proc::lib == 0) return 0.0f;
        uint64_t addr = find_addr(PLAYER_SIZE_Z_BASE, PLAYER_SIZE_Z_OFFSETS, PSZ_OFFSET_COUNT);
        if (addr == 0) return 0.0f;
        return rpm<float>(addr);
    }

    inline void set_player_size_z(float value) {
        uint64_t addr = find_addr(PLAYER_SIZE_Z_BASE, PLAYER_SIZE_Z_OFFSETS, PSZ_OFFSET_COUNT);
        if (addr == 0) return;
        
        wpm<float>(addr, value);
    }
}
