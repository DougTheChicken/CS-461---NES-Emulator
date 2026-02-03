#pragma once
#include <cstdint>

namespace nes {
    using cycle_t = std::uint64_t;
    static constexpr cycle_t CPU_TO_PPU = 3; // 1 CPU cycle = 3 PPU cycles
}

// Backwards-compat alias (some older files may use cycle_t without namespace)
using cycle_t = nes::cycle_t;
