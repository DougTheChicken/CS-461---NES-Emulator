// console is a top-level implementation of the entire NES
// this is where ROMs are load, system-wide reset happens
#pragma once

#include "component.hpp"

class console
{
    console();
    ~console();

    void start();
    void stop();
    void reset();
    void load_rom();
    void run_rom();
    void step(cycle_t stepcount);

    void test();
    void init();
};
