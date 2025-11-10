// console is a top-level implementation of the entire NES
// this is where ROMs are load, system-wide reset happens
#pragma once

#include "cpu.hpp"
#include "timing.hpp"
#include "apu.hpp"
#include "ppu.hpp"
#include "ROM.hpp"

class console
{
    public:
        console();
        ~console();

        void start();
        void stop();
        void reset();
        bool load_rom(char* filepath);
        void run_rom();
        void step(cycle_t stepcount);

        void test();
        void init();
    
    private:
        nes::ROM rom;
        unsigned long long master_cycle_count;
        nes::CPU cpu;
        nes::PPU ppu;
        nes::APU apu;
};
