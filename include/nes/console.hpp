#pragma once

#include <cstdint>
#include <cstddef>

#include "nes/ROM.hpp"
#include "nes/cpu.hpp"
#include "nes/mem.hpp"
#include "nes/ppu.hpp"
#include "nes/apu.hpp"

class console {
public:
    console();
    ~console();

    bool load_rom(const char* filepath);
    bool rom_loaded() const;

    void run_rom();
    void step(cycle_t stepcount);
    void init();
    void reset_all();

    const uint32_t* framebuffer() const;

    unsigned long long get_cpu_cycles() const;
    void step_cpu_cycles(unsigned long long cpu_cycles);
    int step_instruction();

private:
    void update_framebuffer();

    nes::ROM rom;
    nes::PPU ppu;
    nes::APU apu;
    nes::Memory mem;   // must be AFTER ppu/apu (constructed using them)
    nes::CPU cpu;

    unsigned long long master_cycle_count = 0;
    uint32_t framebuffer_data[256 * 240];
};