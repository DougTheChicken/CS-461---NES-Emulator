#include "nes/cpu.hpp"
#include <cstdio>
#include <cstdlib>   // for std::abort
#include "nes/mem.hpp"

namespace nes {

    static cycle_t next_cpu_tick_ppu = 0;

    CPU::CPU() { init_state(); }

    inline uint8_t CPU::fetch8() { return mem->read(PC++); }

    inline uint16_t CPU::fetch16() {
        uint8_t lo = fetch8();
        uint8_t hi = fetch8();
        return (uint16_t)lo | ((uint16_t)hi << 8);
    }

    inline void CPU::push(uint8_t v) { mem->write((uint16_t)(0x0100 | S), v); S--; }
    inline uint8_t CPU::pop() { S++; return mem->read((uint16_t)(0x0100 | S)); }

    void CPU::init_state() {
        A = X = Y = 0;
        S = 0xFD;
        P = 0x24;

        uint8_t lo = mem ? mem->read(0xFFFC) : 0x00;
        uint8_t hi = mem ? mem->read(0xFFFD) : 0x80;
        PC = (uint16_t)lo | ((uint16_t)hi << 8);

        cycles_until_cpu_boundary = 0;
        next_cpu_tick_ppu = 0;

        std::fprintf(stderr, "[cpu] Reset: PC=$%04X (vec=$%02X%02X)\n",
                     (unsigned)PC, (unsigned)hi, (unsigned)lo);
    }

    void CPU::reset() { init_state(); }

    void CPU::step_to(cycle_t ppu_target) {
        while (next_cpu_tick_ppu < ppu_target) { step(); next_cpu_tick_ppu += CPU_TO_PPU; }
    }

    void CPU::step() {
        if (!mem) return;

        uint8_t opcode = fetch8();

        static int trace = 0;
        if ((trace++ % 128) == 0) {
            std::fprintf(stderr,
                "TRACE PC=%04X OPC=%02X A=%02X X=%02X Y=%02X P=%02X S=%02X\n",
                (unsigned)(PC - 1), (unsigned)opcode,
                (unsigned)A, (unsigned)X, (unsigned)Y, (unsigned)P, (unsigned)S);
        }

        switch (opcode) {

            // ---- Status ops ----
            case 0x18: P &= ~C_FLAG; cycles_until_cpu_boundary += 2; break; // CLC
            case 0x38: P |=  C_FLAG; cycles_until_cpu_boundary += 2; break; // SEC
            case 0x58: P &= ~I_FLAG; cycles_until_cpu_boundary += 2; break; // CLI
            case 0x78: P |=  I_FLAG; cycles_until_cpu_boundary += 2; break; // SEI
            case 0xB8: P &= ~V_FLAG; cycles_until_cpu_boundary += 2; break; // CLV
            case 0xD8: P &= ~D_FLAG; cycles_until_cpu_boundary += 2; break; // CLD

            // ---- Loads ----
            case 0xA9: { A = fetch8(); setZN(A); cycles_until_cpu_boundary += 2; break; } // LDA #
            case 0xAD: { uint16_t a = fetch16(); A = mem->read(a); setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA abs
            case 0xA5: { uint8_t zp = fetch8(); A = mem->read(zp); setZN(A); cycles_until_cpu_boundary += 3; break; } // LDA zp
            case 0xB5: { uint8_t zp = fetch8(); A = mem->read((uint8_t)(zp + X)); setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA zp,X
            case 0xBD: { uint16_t a = fetch16(); A = mem->read((uint16_t)(a + X)); setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA abs,X
            case 0xB9: { uint16_t a = fetch16(); A = mem->read((uint16_t)(a + Y)); setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA abs,Y

            case 0xB1: { // LDA (zp),Y
                uint8_t zp = fetch8();
                uint8_t lo = mem->read(zp);
                uint8_t hi = mem->read((uint8_t)(zp + 1));
                uint16_t base = (uint16_t)lo | ((uint16_t)hi << 8);
                uint16_t addr = (uint16_t)(base + Y);
                A = mem->read(addr);
                setZN(A);
                cycles_until_cpu_boundary += 5;
                break;
            }

            case 0xA2: { X = fetch8(); setZN(X); cycles_until_cpu_boundary += 2; break; } // LDX #
            case 0xA0: { Y = fetch8(); setZN(Y); cycles_until_cpu_boundary += 2; break; } // LDY #

            // ---- Stores ----
            case 0x8D: { uint16_t a = fetch16(); mem->write(a, A); cycles_until_cpu_boundary += 4; break; } // STA abs
            case 0x85: { uint8_t zp = fetch8(); mem->write(zp, A); cycles_until_cpu_boundary += 3; break; } // STA zp
            case 0x8E: { uint16_t a = fetch16(); mem->write(a, X); cycles_until_cpu_boundary += 4; break; } // STX abs
            case 0x8C: { uint16_t a = fetch16(); mem->write(a, Y); cycles_until_cpu_boundary += 4; break; } // STY abs

            // ---- Transfers ----
            case 0xAA: X = A; setZN(X); cycles_until_cpu_boundary += 2; break; // TAX
            case 0x8A: A = X; setZN(A); cycles_until_cpu_boundary += 2; break; // TXA
            case 0xA8: Y = A; setZN(Y); cycles_until_cpu_boundary += 2; break; // TAY
            case 0x98: A = Y; setZN(A); cycles_until_cpu_boundary += 2; break; // TYA
            case 0x9A: S = X; cycles_until_cpu_boundary += 2; break;           // TXS
            case 0xBA: X = S; setZN(X); cycles_until_cpu_boundary += 2; break; // TSX

            // ---- INC/DEC ----
            case 0xE8: X++; setZN(X); cycles_until_cpu_boundary += 2; break; // INX
            case 0xC8: Y++; setZN(Y); cycles_until_cpu_boundary += 2; break; // INY
            case 0xCA: X--; setZN(X); cycles_until_cpu_boundary += 2; break; // DEX
            case 0x88: Y--; setZN(Y); cycles_until_cpu_boundary += 2; break; // DEY
            case 0xE6: { // INC zp
                uint8_t zp = fetch8();
                uint8_t v = mem->read(zp);
                v = (uint8_t)(v + 1);
                mem->write(zp, v);
                setZN(v);
                cycles_until_cpu_boundary += 5;
                break;
            }

            // ---- Compares ----
            case 0xE0: { // CPX #imm
                uint8_t v = fetch8();
                uint8_t r = (uint8_t)(X - v);
                if (X >= v) P |= C_FLAG; else P &= ~C_FLAG;
                setZN(r);
                cycles_until_cpu_boundary += 2;
                break;
            }
            case 0xC9: { // CMP #imm
                uint8_t v = fetch8();
                uint8_t r = (uint8_t)(A - v);
                if (A >= v) P |= C_FLAG; else P &= ~C_FLAG;
                setZN(r);
                cycles_until_cpu_boundary += 2;
                break;
            }
            case 0xC5: { // CMP zp
                uint8_t zp = fetch8();
                uint8_t v = mem->read(zp);
                uint8_t r = (uint8_t)(A - v);
                if (A >= v) P |= C_FLAG; else P &= ~C_FLAG;
                setZN(r);
                cycles_until_cpu_boundary += 3;
                break;
            }

            // ---- Branches ----
            case 0x10: { // BPL rel
                int8_t off = (int8_t)fetch8();
                bool N = (P & N_FLAG) != 0;
                if (!N) { PC = (uint16_t)(PC + off); cycles_until_cpu_boundary += 1; }
                cycles_until_cpu_boundary += 2;
                break;
            }
            case 0xD0: { // BNE rel
                int8_t off = (int8_t)fetch8();
                bool Z = (P & Z_FLAG) != 0;
                if (!Z) { PC = (uint16_t)(PC + off); cycles_until_cpu_boundary += 1; }
                cycles_until_cpu_boundary += 2;
                break;
            }
            case 0xF0: { // BEQ rel
                int8_t off = (int8_t)fetch8();
                bool Z = (P & Z_FLAG) != 0;
                if (Z) { PC = (uint16_t)(PC + off); cycles_until_cpu_boundary += 1; }
                cycles_until_cpu_boundary += 2;
                break;
            }

            // ---- Stack ----
            case 0x48: push(A); cycles_until_cpu_boundary += 3; break; // PHA
            case 0x68: A = pop(); setZN(A); cycles_until_cpu_boundary += 4; break; // PLA
            case 0x08: push((uint8_t)(P | B_FLAG | U_FLAG)); cycles_until_cpu_boundary += 3; break; // PHP
            case 0x28: P = (uint8_t)((pop() & ~B_FLAG) | U_FLAG); cycles_until_cpu_boundary += 4; break; // PLP

            // ---- Jumps ----
            case 0x4C: PC = fetch16(); cycles_until_cpu_boundary += 3; break; // JMP abs
            case 0x20: { // JSR
                uint16_t addr = fetch16();
                uint16_t ret = (uint16_t)(PC - 1);
                push((uint8_t)((ret >> 8) & 0xFF));
                push((uint8_t)(ret & 0xFF));
                PC = addr;
                cycles_until_cpu_boundary += 6;
                break;
            }
            case 0x60: { // RTS
                uint8_t lo = pop();
                uint8_t hi = pop();
                PC = (uint16_t)(((uint16_t)lo | ((uint16_t)hi << 8)) + 1);
                cycles_until_cpu_boundary += 6;
                break;
            }

            // ---- NOP ----
            case 0xEA: cycles_until_cpu_boundary += 2; break;

            default:
                std::fprintf(stderr,
                    "[cpu] UNIMPL opcode=%02X at PC=%04X  A=%02X X=%02X Y=%02X P=%02X S=%02X\n",
                    (unsigned)opcode,
                    (unsigned)(PC - 1),
                    (unsigned)A,
                    (unsigned)X,
                    (unsigned)Y,
                    (unsigned)P,
                    (unsigned)S);
                std::abort();
        }
    }

    std::string CPU::lookupInstruction(int opcode) {
        (void)opcode;
        return "???";
    }
}
