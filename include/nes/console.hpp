#pragma once

#include <cstdint>
#include "nes/timing.hpp"   // cycle_t is defined in the global namespace here
#include "nes/cpu.hpp"
#include "nes/ROM.hpp"
#include "nes/mem.hpp"

class console {
public:
    console();
    ~console();

    bool load_rom(char* filepath);
    void run_rom();                 // headless smoke test
    void step(cycle_t stepcount);   // use ::cycle_t (global)
    void init();

private:
    nes::ROM  rom;
    Memory    mem;
    unsigned long long master_cycle_count = 0;
    nes::CPU  cpu;
};
