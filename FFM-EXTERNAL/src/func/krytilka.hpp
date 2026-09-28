#pragma once
#include "../other/addr.hpp"
#include "../other/memory.hpp"

namespace func {
    static constexpr uint64_t KRYTILKA_BASE = 0xF1D5F70;
    static constexpr uint64_t KRYTILKA_OFFSETS[] = { 0x30, 0x4B8, 0x510, 0x1F4 };
    static constexpr int K_OFFSET_COUNT = sizeof(KRYTILKA_OFFSETS) / sizeof(KRYTILKA_OFFSETS[0]);

    inline float get_krytilka() {
        if (proc::lib == 0) return 0.0f;
        

        uint64_t addr = find_addr(KRYTILKA_BASE, KRYTILKA_OFFSETS, K_OFFSET_COUNT);
        if (addr == 0) return 0.0f;
        
        return rpm<float>(addr);
    }

    inline void set_krytilka(float value) {
        uint64_t addr = find_addr(KRYTILKA_BASE, KRYTILKA_OFFSETS, K_OFFSET_COUNT);
        if (addr == 0) return;
        
        wpm<float>(addr, value);
    }
}
