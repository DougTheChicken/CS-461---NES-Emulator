#pragma once
#include <cstdint>

// From https://www.nesdev.org/wiki/PPU_frame_timing
// Implements NTSC timing. No PAL implementation.
using cycle_t = int64_t;                     // global unit: 1 = 1 PPU cycle
constexpr int  CPU_TO_PPU = 3;               // 1 CPU cycle = 3 PPU cycles
constexpr long long MASTER_HZ = 21477272LL;  // Master clock
constexpr double PPU_HZ = MASTER_HZ / 4.0;   // ~5,369,318 Hz
constexpr double CPU_HZ = MASTER_HZ / 12.0;  // ~1,789,772 Hz
constexpr int PPU_DOTS_PER_SCANLINE = 341;
constexpr int PPU_SCANLINES_PER_FRAME = 262;
// each PPU frame is 341*262=89342 PPU clocks long
constexpr cycle_t PPU_CYCLES_PER_FRAME =
    static_cast<cycle_t>(PPU_DOTS_PER_SCANLINE) * PPU_SCANLINES_PER_FRAME;


inline cycle_t ms_to_ppu_cycles(double ms) {
    return static_cast<cycle_t>(PPU_HZ * (ms / 1000.0));
}
