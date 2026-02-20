#pragma once
#include <cstdint>
#include "nes/timing.hpp"   // cycle_t is defined in the global namespace here
#include "nes/cpu.hpp"
#include "nes/ROM.hpp"
#include "nes/mem.hpp"
#include "nes/ppu.hpp"
#include "nes/apu.hpp"

class console {
public:
    console();
    ~console();

    bool load_rom(char* filepath);

    void run_rom();                 // headless smoke test
    void step(cycle_t stepcount);   // use ::cycle_t (global)
    void init();

    void reset_all();
    
    // UI/Framebuffer interface
    const uint32_t* framebuffer() const { return framebuffer_data; }

private:
    nes::ROM  rom;
    nes::PPU ppu;
    nes::APU apu;
    nes::Memory    mem;
    unsigned long long master_cycle_count = 0;
    nes::CPU  cpu;
    
    // Framebuffer for rendering (256x240 ARGB)
    uint32_t framebuffer_data[256 * 240];
    void update_framebuffer();
};
