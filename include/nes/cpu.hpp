#pragma once
#include <cstdint>
#include <string>
#include "nes/timing.hpp"

class Memory;

namespace nes {
    class CPU {
    public:
        // constructor runs initialization to known state
        CPU();

        void attach_memory(Memory* m) { mem = m; }

        // Reset should also re-init to known state
        void reset();

        void step_to(cycle_t ppu_target);
        void step();

        uint16_t pc() const { return PC; }
        uint8_t  a()  const { return A;  }
        uint8_t  x()  const { return X;  }
        uint8_t  y()  const { return Y;  }
        uint8_t  p()  const { return P;  }

        std::string lookupInstruction(int);

    private:
        // single source of truth for initial/reset state
        void init_state();

        // Registers
        uint16_t PC = 0x0000;   // PC is loaded from reset vector in init_state()
        uint8_t  A = 0, X = 0, Y = 0;
        uint8_t  S = 0xFD;      // stack pointer
        uint8_t  P = 0x24;      // NV-BDIZC
        cycle_t  cycles_until_cpu_boundary = 0;

        Memory*  mem = nullptr;

        // Flags
        static constexpr uint8_t C_FLAG = 0x01;
        static constexpr uint8_t Z_FLAG = 0x02;
        static constexpr uint8_t I_FLAG = 0x04;
        static constexpr uint8_t D_FLAG = 0x08;
        static constexpr uint8_t B_FLAG = 0x10;
        static constexpr uint8_t U_FLAG = 0x20;
        static constexpr uint8_t V_FLAG = 0x40;
        static constexpr uint8_t N_FLAG = 0x80;

        inline void setZN(uint8_t v) {
            if (v == 0) P |= Z_FLAG; else P &= ~Z_FLAG;
            if (v & 0x80) P |= N_FLAG; else P &= ~N_FLAG;
        }
        inline void setC(bool on) { if (on) P |= C_FLAG; else P &= ~C_FLAG; }

        inline uint8_t  fetch8();
        inline uint16_t fetch16();

        // Stack helpers (stack page $0100)
        inline void push(uint8_t v);
        inline uint8_t pop();
    };
}
