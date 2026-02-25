#include "nes/cpu.hpp"
#include <cstdio>
#include <cstdlib>
#include "nes/mem.hpp"

namespace nes {

// NOTE: Ideally this should be a CPU member (not file-static), but leaving it here
// to minimize changes outside this file.
static cycle_t next_cpu_tick_ppu = 0;

CPU::CPU() {
    init_state();
}

uint8_t CPU::fetch8() {
    return mem->read(PC++);
}

uint16_t CPU::fetch16() {
    uint8_t lo = fetch8();
    uint8_t hi = fetch8();
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

void CPU::push(uint8_t v) {
    mem->write((uint16_t)(0x0100 | S), v);
    S--;
}

uint8_t CPU::pop() {
    S++;
    return mem->read((uint16_t)(0x0100 | S));
}

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

    void CPU::nmi() {
    // Push PC high byte, then low byte
    push((uint8_t)(PC >> 8));
    push((uint8_t)(PC & 0xFF));
    // Push status with B clear, U set
    push((P & ~B_FLAG) | U_FLAG);
    // Set interrupt disable flag
    P |= I_FLAG;
    // Load PC from NMI vector $FFFA-$FFFB
    uint8_t lo = mem->read(0xFFFA);
    uint8_t hi = mem->read(0xFFFB);
    PC = (uint16_t)lo | ((uint16_t)hi << 8);
}

    void CPU::irq() {
    // IRQ is ignored if interrupt disable flag is set
    if (P & I_FLAG) return;
    // Push PC high byte, then low byte
    push((uint8_t)(PC >> 8));
    push((uint8_t)(PC & 0xFF));
    // Push status with B clear, U set
    push((P & ~B_FLAG) | U_FLAG);
    // Set interrupt disable flag
    P |= I_FLAG;
    // Load PC from IRQ vector $FFFE-$FFFF
    uint8_t lo = mem->read(0xFFFE);
    uint8_t hi = mem->read(0xFFFF);
    PC = (uint16_t)lo | ((uint16_t)hi << 8);
}

void CPU::reset() {
    init_state();
}

void CPU::step_to(cycle_t ppu_target) {
    while (next_cpu_tick_ppu < ppu_target) {
        int cpu_cycles = step();
        next_cpu_tick_ppu += static_cast<cycle_t>(cpu_cycles) * CPU_TO_PPU;
    }
}

int CPU::step() {
    if (!mem) return 0;

    int cycles_before = cycles_until_cpu_boundary;

    uint16_t pc_before = PC;
    uint8_t opcode = fetch8();

    static int trace = 0;
    if ((trace++ % 128) == 0) {
        std::fprintf(stderr,
                     "TRACE PC=%04X OPC=%02X A=%02X X=%02X Y=%02X P=%02X S=%02X\n",
                     (unsigned)pc_before, (unsigned)opcode,
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
            A = mem->read((uint16_t)(base + Y));
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
            uint8_t v = (uint8_t)(mem->read(zp) + 1);
            mem->write(zp, v);
            setZN(v);
            cycles_until_cpu_boundary += 5;
            break;
        }

        // ---- Compare ----
        case 0xC9: { // CMP #
            uint8_t v = fetch8();
            uint16_t r = (uint16_t)A - (uint16_t)v;
            if (A >= v) P |= C_FLAG; else P &= ~C_FLAG;
            setZN((uint8_t)r);
            cycles_until_cpu_boundary += 2;
            break;
        }
        case 0xC5: { // CMP zp
            uint8_t zp = fetch8();
            uint8_t v = mem->read(zp);
            uint16_t r = (uint16_t)A - (uint16_t)v;
            if (A >= v) P |= C_FLAG; else P &= ~C_FLAG;
            setZN((uint8_t)r);
            cycles_until_cpu_boundary += 3;
            break;
        }
        case 0xE0: { // CPX #
            uint8_t v = fetch8();
            uint16_t r = (uint16_t)X - (uint16_t)v;
            if (X >= v) P |= C_FLAG; else P &= ~C_FLAG;
            setZN((uint8_t)r);
            cycles_until_cpu_boundary += 2;
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
        case 0x20: { // JSR abs
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
    // cycles consumed by THIS instruction
    return cycles_until_cpu_boundary - cycles_before;
}

    std::string CPU::lookupInstruction(int opcode) {
    switch (opcode & 0xFF) {
        case 0x01: return "ORA"; // ORA (indirect,X)
        default:   return "???";
    }
}

} // namespace nes