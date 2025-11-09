#pragma once

// abstract base class for every NES component
// includes stub implementations of iterrupt handling for interrupt handling (both non-maskable interrupts NMI
// and interrupt requests IRQ)
// in components that actually handle interrups (e.g. PPU, APU, CPU)
#include <cstdint>
#include "timing.hpp"

class console;

class component
{
public:
    // Mandatory component pure virtual functions to be implemented by all components
    virtual void start(console *sys) = 0;
    virtual void reset() = 0;
    virtual void step_to(cycle_t count) = 0;

    // Optional interrupt interface:
    virtual void nmi() {}             // default = no-op
    virtual void clear_nmi() {}
    virtual void irq(uint8_t source) {}
    virtual void clear_irq(uint8_t source) {};
};