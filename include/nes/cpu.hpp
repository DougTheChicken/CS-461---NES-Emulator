#pragma once
#include <cstdint>
#include <string>
#include "nes/timing.hpp"
#include "nes/ppu.hpp"

namespace nes {

class Memory;

class CPU {
public:
    CPU();

    void attach_memory(Memory* m) { mem = m; }
    void reset();

    // Run CPU until PPU timeline reaches ppu_target (ppu cycles)
    void step_to(cycle_t ppu_target);

    // Execute a single instruction
    int step();

    // handle non-maskable interrupts for PPU
    void nmi();

    // handle interrupts for APU
    void irq();

    // Debug getters
    uint16_t pc() const { return PC; }
    uint8_t  a()  const { return A; }
    uint8_t  x()  const { return X; }
    uint8_t  y()  const { return Y; }
    uint8_t  p()  const { return P; }
    uint8_t  s()  const { return S; }

    std::string lookupInstruction(int opcode);

private:
    // Registers
    uint8_t  A = 0, X = 0, Y = 0;
    uint8_t  P = 0x24;
    uint8_t  S = 0xFD;
    uint16_t PC = 0x8000;

    Memory* mem = nullptr;

    // Helpers
    uint8_t fetch8();
    uint16_t fetch16();
    void push(uint8_t v);
    uint8_t pop();

    // addressing mode helpers
    uint16_t addr_abs();
    uint16_t addr_zp();
    uint16_t addr_zpx();
    uint16_t addr_zpy();
    uint16_t addr_absx(bool check_page_cross = false);
    uint16_t addr_absy(bool check_page_cross = false);
    uint16_t addr_indx();
    uint16_t addr_indy(bool check_page_cross = false);

    // arithmetic helpers
    void do_adc(uint8_t value);

    void init_state();
    void setZN(uint8_t v) {
        if (v == 0) P |= Z_FLAG; else P &= ~Z_FLAG;
        if (v & 0x80) P |= N_FLAG; else P &= ~N_FLAG;
    }

    // Flags
    static constexpr uint8_t C_FLAG = 0x01;
    static constexpr uint8_t Z_FLAG = 0x02;
    static constexpr uint8_t I_FLAG = 0x04;
    static constexpr uint8_t D_FLAG = 0x08;
    static constexpr uint8_t B_FLAG = 0x10;
    static constexpr uint8_t U_FLAG = 0x20;
    static constexpr uint8_t V_FLAG = 0x40;
    static constexpr uint8_t N_FLAG = 0x80;

    int cycles_until_cpu_boundary = 0;
};

} // namespace nes
