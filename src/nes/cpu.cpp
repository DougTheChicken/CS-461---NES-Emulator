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

// ---- addressing mode helpers -----
uint16_t CPU::addr_abs() { // absolute
    return fetch16();
}

uint16_t CPU::addr_zp() { // zero page
    return fetch8();
}

uint16_t CPU::addr_zpx() { // zero page, x
    // zero page indexing needs to wrap within $0000-$00FF
    return (addr_zp() + X) & 0xFF;
}

uint16_t CPU::addr_zpy() { // zero page, y
    // zero page indexing needs to wrap within $0000-$00FF
    return (addr_zp() + Y) & 0xFF;
}

uint16_t CPU::addr_absx(bool check_page_cross = false) { // absolute indexed, x
    uint16_t base = addr_abs();
    uint16_t addr = base + X;

    // check high byte for changes (handle page crossing)
    if (check_page_cross && ((base & 0XFF00) != (addr & 0xFF00)))
        cycles_until_cpu_boundary += 1;

    return addr;
}

uint16_t CPU::addr_absy(bool check_page_cross = false) { // absolute indexed, y
    uint16_t base = addr_abs();
    uint16_t addr = base + Y;

    // check high byte for changes (handle page crossing)
    if (check_page_cross && ((base & 0XFF00) != (addr & 0xFF00)))
        cycles_until_cpu_boundary += 1;

    return addr;
}

uint16_t CPU::addr_indx() { // indirect, x
    // take byte, add x, and add 0xFF to keep ptr within zero page (prevents wrap bug)
    uint8_t ptr = (addr_zp() + X) & 0xFF;

    // read 2 bytes from zero page where ptr is looking
    uint8_t low = mem->read(ptr);
    uint8_t high = mem->read((ptr + 1) & 0xFF);

    // return address
    return (high << 8) | low;
}

uint16_t CPU::addr_indy(bool check_page_cross = false) { // indirect, y
    // take byte, add x, and add 0xFF to keep ptr within zero page (prevents wrap bug)
    uint8_t ptr = addr_zp();

    // read 2 bytes from zero page where ptr is looking
    uint8_t low = mem->read(ptr);
    uint8_t high = mem->read((ptr + 1) & 0xFF);

    // create base address and add y for final address
    uint16_t base = (high << 8) | low;
    uint16_t addr = base + Y;

    // check high byte for changes (handle page crossing)
    if (check_page_cross && ((base & 0XFF00) != (addr & 0xFF00)))
        cycles_until_cpu_boundary += 1;

    return addr;
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
    
    // add 7 cycles to ensure cpu and ppu stay in sync
    cycles_until_cpu_boundary += 7;
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

    if (mem->get_ppu().nmi_pending) {
        // acknowledge the interrupt
        mem->get_ppu().nmi_pending = false;
        // execute the nmi sequence
        nmi();
    }

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
        case 0x30: { // bmi rel
            // fetch offset
            int8_t off = (int8_t)fetch8();
            // base timing
            cycles_until_cpu_boundary += 2;

            // check status and n_flag
            if (P & N_FLAG) {
                // execute branch (1 cycle)
                uint16_t old_pc = PC;
                PC += off;
                cycles_until_cpu_boundary += 1;
                
                // handle page crossing
                if ((old_pc & 0xFF00) != (PC & 0xFF00)) // old and new pc are on different pages
                    cycles_until_cpu_boundary += 1; // takes additional cycle if true
            }

            break;
        }
        case 0x50: { // bvc rel
            // fetch offset
            int8_t off = (int8_t)fetch8();
            // base timing
            cycles_until_cpu_boundary += 2;

            // check status and v_flag (if overflow flag is clear)
            if (!(P & V_FLAG)) {
                // execute branch (1 cycle)
                uint16_t old_pc = PC;
                PC += off;
                cycles_until_cpu_boundary += 1;
                
                // handle page crossing
                if ((old_pc & 0xFF00) != (PC & 0xFF00)) // old and new pc are on different pages
                    cycles_until_cpu_boundary += 1; // takes additional cycle if true
            }

            break;
        }
        case 0x70: { // bvs rel
            // fetch offset
            int8_t off = (int8_t)fetch8();
            // base timing
            cycles_until_cpu_boundary += 2;

            // check status and v_flag
            if ((P & V_FLAG)) {
                // execute branch (1 cycle)
                uint16_t old_pc = PC;
                PC += off;
                cycles_until_cpu_boundary += 1;
                
                // handle page crossing
                if ((old_pc & 0xFF00) != (PC & 0xFF00)) // old and new pc are on different pages
                    cycles_until_cpu_boundary += 1; // takes additional cycle if true
            }

            break;
        }
        case 0x90: { // bcc rel
            // fetch offset
            int8_t off = (int8_t)fetch8();
            // base timing
            cycles_until_cpu_boundary += 2;

            // check status and c_flag (if carry flag is 0)
            if (!(P & C_FLAG)) {
                // execute branch (1 cycle)
                uint16_t old_pc = PC;
                PC += off;
                cycles_until_cpu_boundary += 1;
                
                // handle page crossing
                if ((old_pc & 0xFF00) != (PC & 0xFF00)) // old and new pc are on different pages
                    cycles_until_cpu_boundary += 1; // takes additional cycle if true
            }

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

        case 0x4A: { // LSR A
            // Handle the Carry Flag (C_FLAG)
            if (A & 0x01) {
                P |= C_FLAG;
            }
            else {
                P &= ~C_FLAG;
            }

            // Shift the accumulator right by 1
            A >>= 1;

            // Handle Zero (Z_FLAG) and Negative (N_FLAG)
            setZN(A);

            cycles_until_cpu_boundary += 2;
            break;
        }

        case 0x26: { // ROL zp
            // Fetch the zero-page address and read the current value from memory
            uint8_t zp_addr = fetch8();
            uint8_t value = mem->read(zp_addr);

            // Grab the bit that is about to fall off the left edge (to become the new carry)
            bool new_carry = (value & 0x80) != 0;

            // Grab the current carry flag (to rotate into Bit 0)
            uint8_t old_carry = (P & C_FLAG) ? 1 : 0;

            // Perform the rotation
            value = (value << 1) | old_carry;

            // Write the rotated value back to the zero page
            mem->write(zp_addr, value);

            // Update the CPU Status Flags
            if (new_carry) {
                P |= C_FLAG;
            }
            else {
                P &= ~C_FLAG;
            }
            setZN(value);

            cycles_until_cpu_boundary += 5;
            break;
        }

        case 0x45: { // EOR zp
            // Fetch the zero-page address and read the value
            uint8_t zp_addr = fetch8();
            uint8_t value = mem->read(zp_addr);

            // Perform the bitwise XOR with the Accumulator
            A ^= value;

            // Update the Zero and Negative flags based on the new Accumulator value
            setZN(A);

            cycles_until_cpu_boundary += 3;
            break;
        }

        case 0x25: { // AND zp
            // Fetch the zero-page address and read the value
            uint8_t zp_addr = fetch8();
            uint8_t value = mem->read(zp_addr);

            // Perform the bitwise AND with the Accumulator
            A &= value;

            // Update the Zero and Negative flags based on the new Accumulator value
            setZN(A);

            cycles_until_cpu_boundary += 3;
            break;
        }

        case 0x86: { // STX zp
            // Fetch the zero-page address
            uint8_t zp_addr = fetch8();

            // Write the value of the X register to that memory address
            mem->write(zp_addr, X);

            cycles_until_cpu_boundary += 3;
            break;
        }

        case 0x40: { // RTI - Return from Interrupt
            // Pop the Processor Status (P) register from the stack
            P = (pop() & ~B_FLAG) | U_FLAG;

            // Pop the Program Counter (PC) from the stack
            uint16_t pc_low = pop();
            uint16_t pc_high = pop();

            // Reconstruct the 16-bit Program Counter
            PC = (pc_high << 8) | pc_low;

            cycles_until_cpu_boundary += 6;
            break;
        }

        case 0x69: { // ADC imm
            // Fetch the immediate value directly from the instruction
            uint8_t value = fetch8();

            // Perform the 16-bit addition
            uint8_t carry_in = (P & C_FLAG) ? 1 : 0;
            uint16_t sum = A + value + carry_in;

            // Set the Overflow Flag (V_FLAG)
            if (~(A ^ value) & (A ^ sum) & 0x80) {
                P |= V_FLAG;
            }
            else {
                P &= ~V_FLAG;
            }

            // Set the Carry Flag
            if (sum > 0xFF) {
                P |= C_FLAG;
            }
            else {
                P &= ~C_FLAG;
            }

            // Store the 8-bit result in the Accumulator
            A = sum & 0xFF;

            // Update Zero and Negative flags
            setZN(A);

            cycles_until_cpu_boundary += 2;
            break;
        }

        case 0xB0: { // BCS rel
            // Fetch the offset and cast it to a signed 8-bit integer
            int8_t offset = static_cast<int8_t>(fetch8());

            // Base cycles
            cycles_until_cpu_boundary += 2;

            // Check if we should branch
            if (P & C_FLAG) {
                uint16_t old_pc = PC;

                // Add the signed offset to the Program Counter
                PC += offset;

                // Add 1 cycle because the branch succeeded
                cycles_until_cpu_boundary += 1;

                // Add 1 more cycle if the branch crossed a page boundary
                if ((old_pc & 0xFF00) != (PC & 0xFF00)) {
                    cycles_until_cpu_boundary += 1;
                }
            }
            break;
        }

        // ---- Shift ----

        case 0x0A: { // asl accumulator
            // handle carry flag
            if (A & 0x80)
                P |= C_FLAG;
            else
                P &= ~C_FLAG;

            // shift accumulator left 1
            A <<= 1;

            // update flags
            setZN(A);

            // adjust timing
            cycles_until_cpu_boundary += 2;
            break;
        }

        case 0x06: { // asl zero page
            // fetch address
            uint8_t addr = fetch8();
            // read value
            uint8_t val = mem->read(addr);

            // handle carry flag
            if (val & 0x80)
                P |= C_FLAG;
            else
                P &= ~C_FLAG;

            // shift bits left 1
            val <<= 1;

            // write back to memory
            mem->write(addr, val);
            // update flags
            setZN(val);

            // adjust timing
            cycles_until_cpu_boundary += 5;

            break;
        }

        // lsr accumulator already handled

        case 0x46: { // lsr zero page
            // fetch address
            uint8_t addr = fetch8();
            // read value
            uint8_t val = mem->read(addr);

            // handle carry flag
            if (val & 0x01)
                P |= C_FLAG;
            else
                P &= ~C_FLAG;

            // shift bits right 1
            val >>= 1;

            // write back to memory
            mem->write(addr, val);
            // update flags
            setZN(val);

            // adjust timing
            cycles_until_cpu_boundary += 5;

            break;
        }

        case 0x2A: { // rol accumulator
            // capture bit
            uint8_t old_c = (P & C_FLAG) ? 1 : 0;

            // handle carry flag
            if (A & 0x80)
                P |= C_FLAG;
            else
                P &= ~C_FLAG;

            // rotation for accumulator
            A = (A << 1) | old_c;

            // update flags
            setZN(A);

            // adjust timing
            cycles_until_cpu_boundary += 2;
            break;
        }

        // rol zero page already handled

        case 0x6A: { // ror accumulator
            // capture bit
            uint8_t old_c = (P & C_FLAG) ? 0x80 : 0;

            // handle carry flag
            if (A & 0x01)
                P |= C_FLAG;
            else
                P &= ~C_FLAG;

            // rotation for accumulator
            A = (A >> 1) | old_c;

            // update flags
            setZN(A);

            // adjust timing
            cycles_until_cpu_boundary += 2;
            break;
        }

        case 0x66: { // ror zero page
            // fetch address
            uint8_t addr = fetch8();
            // read value
            uint8_t val = mem->read(addr);
            // capture bit
            uint8_t old_c = (P & C_FLAG) ? 0x80 : 0;

            // handle carry flag
            if (val & 0x01)
                P |= C_FLAG;
            else
                P &= ~C_FLAG;

            // rotation of bits
            val = (val >> 1) | old_c;

            // write back to memory
            mem->write(addr, val);
            // update flags
            setZN(val);

            // adjust timing
            cycles_until_cpu_boundary += 5;

            break;
        }

        // ---- Interrupts ----
        case 0x00: { // brk - force interrupt
            // increment pc by 1 to prevent rti from executing brk in infinite loop
            PC++;

            // push pc high byte (8-15)
            push((PC >> 8) & 0xFF);
            // push pc low byte (0-7)
            push(PC & 0xFF);
            // push status reg
            push(P | B_FLAG | U_FLAG);

            // set i_flag to prevent nesting interrupts
            P |= I_FLAG;

            // load new pc from $FFFE (irq / brk vector)
            PC = mem->read(0xFFFE);

            // adjust timing
            cycles_until_cpu_boundary += 7;

            break;
        }
        
        // rti already implemented

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