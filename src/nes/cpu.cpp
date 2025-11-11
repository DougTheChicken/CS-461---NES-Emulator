#include "nes/cpu.hpp"
#include <cstdio>
#include "nes/mem.hpp"

namespace nes {
    static cycle_t next_cpu_tick_ppu = 0;

    inline uint8_t  CPU::fetch8()  { return mem->read(PC++); }
    inline uint16_t CPU::fetch16() { uint8_t lo=fetch8(), hi=fetch8(); return (uint16_t)lo | ((uint16_t)hi<<8); }

    inline void CPU::push(uint8_t v){ mem->write((uint16_t)(0x0100 | S), v); S--; }
    inline uint8_t CPU::pop(){ S++; return mem->read((uint16_t)(0x0100 | S)); }

    void CPU::reset() {
        A = X = Y = 0; S = 0xFD; P = 0x24;
        uint8_t lo = mem ? mem->read(0xFFFC) : 0x00;
        uint8_t hi = mem ? mem->read(0xFFFD) : 0x80;
        PC = (uint16_t)lo | ((uint16_t)hi << 8);
        cycles_until_cpu_boundary = 0;
        std::fprintf(stderr, "[cpu] Reset: PC=$%04X (vec=$%02X%02X)\n", PC, hi, lo);
    }

    void CPU::step_to(cycle_t ppu_target) {
        while (next_cpu_tick_ppu < ppu_target) { step(); next_cpu_tick_ppu += CPU_TO_PPU; }
    }

    void CPU::step() {
        if (!mem) return;

        uint8_t opcode = fetch8();

        static int trace = 0;
        if ((trace++ % 128) == 0) {
            std::fprintf(stderr, "TRACE PC=%04X OPC=%02X A=%02X X=%02X Y=%02X P=%02X S=%02X\n",
                         (unsigned)(PC-1), opcode, A, X, Y, P, S);
        }

        switch (opcode) {
            // ---- Status ops ----
            case 0x18: /* CLC */ P &= ~C_FLAG;               cycles_until_cpu_boundary += 2; break;
            case 0x38: /* SEC */ P |=  C_FLAG;               cycles_until_cpu_boundary += 2; break;
            case 0x58: /* CLI */ P &= ~I_FLAG;               cycles_until_cpu_boundary += 2; break;
            case 0x78: /* SEI */ P |=  I_FLAG;               cycles_until_cpu_boundary += 2; break;
            case 0xB8: /* CLV */ P &= ~V_FLAG;               cycles_until_cpu_boundary += 2; break;
            case 0xD8: /* CLD */ P &= ~D_FLAG;               cycles_until_cpu_boundary += 2; break;

            // ---- Loads ----
            case 0xA9: { uint8_t v = fetch8(); A = v; setZN(A); cycles_until_cpu_boundary += 2; break; }                  // LDA #imm
            case 0xAD: { uint16_t a = fetch16(); uint8_t v = mem->read(a); A = v; setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA abs
            case 0xA5: { uint8_t zp = fetch8(); uint8_t v = mem->read(zp); A = v; setZN(A); cycles_until_cpu_boundary += 3; break; } // LDA zp
            case 0xB5: { uint8_t zp = fetch8(); uint8_t v = mem->read((uint8_t)(zp + X)); A = v; setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA zp,X
            case 0xBD: { uint16_t a = fetch16(); uint8_t v = mem->read((uint16_t)(a + X)); A = v; setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA abs,X  (ignore +1 page-cross)
            case 0xB9: { uint16_t a = fetch16(); uint8_t v = mem->read((uint16_t)(a + Y)); A = v; setZN(A); cycles_until_cpu_boundary += 4; break; } // LDA abs,Y
            case 0xA2: { uint8_t v = fetch8(); X = v; setZN(X); cycles_until_cpu_boundary += 2; break; }                  // LDX #imm
            case 0xA0: { uint8_t v = fetch8(); Y = v; setZN(Y); cycles_until_cpu_boundary += 2; break; }                  // LDY #imm

            // ---- Stores ----
            case 0x8D: { uint16_t a = fetch16(); mem->write(a, A); cycles_until_cpu_boundary += 4; break; }               // STA abs
            case 0x8E: { uint16_t a = fetch16(); mem->write(a, X); cycles_until_cpu_boundary += 4; break; }               // STX abs
            case 0x8C: { uint16_t a = fetch16(); mem->write(a, Y); cycles_until_cpu_boundary += 4; break; }               // STY abs
            case 0x85: { uint8_t zp = fetch8(); mem->write(zp, A); cycles_until_cpu_boundary += 3; break; }               // STA zp
            case 0x95: { uint8_t zp = fetch8(); mem->write((uint8_t)(zp + X), A); cycles_until_cpu_boundary += 4; break; } // STA zp,X
            case 0x9D: { uint16_t a = fetch16(); mem->write((uint16_t)(a + X), A); cycles_until_cpu_boundary += 5; break; } // STA abs,X
            case 0x99: { uint16_t a = fetch16(); mem->write((uint16_t)(a + Y), A); cycles_until_cpu_boundary += 5; break; } // STA abs,Y

            // ---- Transfers ----
            case 0xAA: /* TAX */ X = A; setZN(X); cycles_until_cpu_boundary += 2; break;
            case 0x8A: /* TXA */ A = X; setZN(A); cycles_until_cpu_boundary += 2; break;
            case 0xA8: /* TAY */ Y = A; setZN(Y); cycles_until_cpu_boundary += 2; break;
            case 0x98: /* TYA */ A = Y; setZN(A); cycles_until_cpu_boundary += 2; break;
            case 0xBA: /* TSX */ X = S; setZN(X); cycles_until_cpu_boundary += 2; break;
            case 0x9A: /* TXS */ S = X;           cycles_until_cpu_boundary += 2; break;

            // ---- Inc/Dec ----
            case 0xE8: /* INX */ X = (uint8_t)(X + 1); setZN(X); cycles_until_cpu_boundary += 2; break;
            case 0xC8: /* INY */ Y = (uint8_t)(Y + 1); setZN(Y); cycles_until_cpu_boundary += 2; break;
            case 0xCA: /* DEX */ X = (uint8_t)(X - 1); setZN(X); cycles_until_cpu_boundary += 2; break;
            case 0x88: /* DEY */ Y = (uint8_t)(Y - 1); setZN(Y); cycles_until_cpu_boundary += 2; break;

            // ---- Stack ----
            case 0x48: /* PHA */ push(A);                                            cycles_until_cpu_boundary += 3; break;
            case 0x68: /* PLA */ A = pop(); setZN(A);                                 cycles_until_cpu_boundary += 4; break;
            case 0x08: /* PHP */ push((uint8_t)(P | B_FLAG | U_FLAG));                cycles_until_cpu_boundary += 3; break;
            case 0x28: /* PLP */ { P = (uint8_t)((pop() & ~B_FLAG) | U_FLAG);         cycles_until_cpu_boundary += 4; break; }

            // ---- Logic (immediate) ----
            case 0x29: { uint8_t v = fetch8(); A &= v; setZN(A); cycles_until_cpu_boundary += 2; break; } // AND #
            case 0x09: { uint8_t v = fetch8(); A |= v; setZN(A); cycles_until_cpu_boundary += 2; break; } // ORA #
            case 0x49: { uint8_t v = fetch8(); A ^= v; setZN(A); cycles_until_cpu_boundary += 2; break; } // EOR #

            // ---- BIT tests ----
            case 0x2C: { // BIT abs
                uint16_t a = fetch16(); uint8_t v = mem->read(a);
                ((A & v) == 0) ? (P |= Z_FLAG) : (P &= ~Z_FLAG);
                (v & 0x80) ? (P |= N_FLAG) : (P &= ~N_FLAG);
                (v & 0x40) ? (P |= V_FLAG) : (P &= ~V_FLAG);
                cycles_until_cpu_boundary += 4; break;
            }
            case 0x24: { // BIT zp
                uint8_t zp = fetch8(); uint8_t v = mem->read(zp);
                ((A & v) == 0) ? (P |= Z_FLAG) : (P &= ~Z_FLAG);
                (v & 0x80) ? (P |= N_FLAG) : (P &= ~N_FLAG);
                (v & 0x40) ? (P |= V_FLAG) : (P &= ~V_FLAG);
                cycles_until_cpu_boundary += 3; break;
            }

            // ---- Compares (immediate) ----
            case 0xC9: { /* CMP # */
                uint8_t v = fetch8();
                uint8_t r = (uint8_t)(A - v);
                if (A >= v) P |= C_FLAG; else P &= ~C_FLAG;
                setZN(r);
                cycles_until_cpu_boundary += 2; break;
            }
            case 0xE0: { /* CPX # */
                uint8_t v = fetch8();
                uint8_t r = (uint8_t)(X - v);
                if (X >= v) P |= C_FLAG; else P &= ~C_FLAG;
                setZN(r);
                cycles_until_cpu_boundary += 2; break;
            }
            case 0xC0: { /* CPY # */
                uint8_t v = fetch8();
                uint8_t r = (uint8_t)(Y - v);
                if (Y >= v) P |= C_FLAG; else P &= ~C_FLAG;
                setZN(r);
                cycles_until_cpu_boundary += 2; break;
            }

            // ---- ADC/SBC (immediate) ----
            case 0x69: { /* ADC # */
                uint8_t v = fetch8();
                uint16_t sum = (uint16_t)A + v + ((P & C_FLAG) ? 1 : 0);
                (sum & 0x100) ? (P |= C_FLAG) : (P &= ~C_FLAG);
                uint8_t res = (uint8_t)sum;
                if (((~(A ^ v)) & (A ^ res) & 0x80)) P |= V_FLAG; else P &= ~V_FLAG;
                A = res; setZN(A);
                cycles_until_cpu_boundary += 2; break;
            }
            case 0xE9: { /* SBC # */
                uint8_t v = fetch8();
                uint16_t diff = (uint16_t)A - v - ((P & C_FLAG) ? 0 : 1);
                (diff & 0x100) ? (P &= ~C_FLAG) : (P |= C_FLAG); // borrow clears C
                uint8_t res = (uint8_t)diff;
                if (((A ^ v) & (A ^ res) & 0x80)) P |= V_FLAG; else P &= ~V_FLAG;
                A = res; setZN(A);
                cycles_until_cpu_boundary += 2; break;
            }

            // ---- Shifts ----
            case 0x4A: { /* LSR A */ uint8_t old=A; (old & 1) ? (P |= C_FLAG) : (P &= ~C_FLAG);
                         A = (uint8_t)(old>>1); setZN(A); cycles_until_cpu_boundary += 2; break; }
            case 0x46: { /* LSR zp */ uint8_t zp=fetch8(); uint8_t v=mem->read(zp);
                         (v & 1) ? (P |= C_FLAG) : (P &= ~C_FLAG);
                         v>>=1; mem->write(zp,v); setZN(v); cycles_until_cpu_boundary += 5; break; }
            case 0x4E: { /* LSR abs */ uint16_t a=fetch16(); uint8_t v=mem->read(a);
                         (v & 1) ? (P |= C_FLAG) : (P &= ~C_FLAG);
                         v>>=1; mem->write(a,v); setZN(v); cycles_until_cpu_boundary += 6; break; }

            // ---- Branches ----
            case 0x10: { int8_t off=(int8_t)fetch8(); bool N=(P&N_FLAG)!=0; if(!N){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BPL
            case 0x30: { int8_t off=(int8_t)fetch8(); bool N=(P&N_FLAG)!=0; if( N){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BMI
            case 0x90: { int8_t off=(int8_t)fetch8(); bool C=(P&C_FLAG)!=0; if(!C){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BCC
            case 0xB0: { int8_t off=(int8_t)fetch8(); bool C=(P&C_FLAG)!=0; if( C){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BCS
            case 0xD0: { int8_t off=(int8_t)fetch8(); bool Z=(P&Z_FLAG)!=0; if(!Z){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BNE
            case 0xF0: { int8_t off=(int8_t)fetch8(); bool Z=(P&Z_FLAG)!=0; if( Z){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BEQ
            case 0x50: { int8_t off=(int8_t)fetch8(); bool V=(P&V_FLAG)!=0; if(!V){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BVC
            case 0x70: { int8_t off=(int8_t)fetch8(); bool V=(P&V_FLAG)!=0; if( V){ PC=(uint16_t)(PC+off); cycles_until_cpu_boundary+=1; } cycles_until_cpu_boundary+=2; break; } // BVS

            // ---- Jumps / subroutines ----
            case 0x4C: { uint16_t a = fetch16(); PC = a;                cycles_until_cpu_boundary += 3; break; } // JMP abs
            case 0x6C: { // JMP (ind) with 6502 page-wrap bug
                uint16_t ptr = fetch16();
                uint16_t lo_addr = ptr;
                uint16_t hi_addr = (uint16_t)((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
                uint8_t lo = mem->read(lo_addr), hi = mem->read(hi_addr);
                PC = (uint16_t)lo | ((uint16_t)hi << 8);
                cycles_until_cpu_boundary += 5; break;
            }
            case 0x20: { uint16_t addr = fetch16(); uint16_t ret=(uint16_t)(PC-1);
                         push((uint8_t)((ret>>8)&0xFF)); push((uint8_t)(ret&0xFF));
                         PC = addr;                                     cycles_until_cpu_boundary += 6; break; } // JSR abs
            case 0x60: { uint8_t lo = pop(); uint8_t hi = pop();
                         uint16_t ret = (uint16_t)lo | ((uint16_t)hi<<8);
                         PC = (uint16_t)(ret + 1);                       cycles_until_cpu_boundary += 6; break; } // RTS

                // ---- Extra loads (X/Y, zp/abs) ----
            case 0xA6: { /* LDX zp */ uint8_t zp=fetch8(); X=mem->read(zp); setZN(X); cycles_until_cpu_boundary+=3; break; }
            case 0xB6: { /* LDX zp,Y */ uint8_t zp=fetch8(); X=mem->read((uint8_t)(zp+Y)); setZN(X); cycles_until_cpu_boundary+=4; break; }
            case 0xAE: { /* LDX abs */ uint16_t a=fetch16(); X=mem->read(a); setZN(X); cycles_until_cpu_boundary+=4; break; }
            case 0xBE: { /* LDX abs,Y */ uint16_t a=fetch16(); X=mem->read((uint16_t)(a+Y)); setZN(X); cycles_until_cpu_boundary+=4; break; }

            case 0xA4: { /* LDY zp */ uint8_t zp=fetch8(); Y=mem->read(zp); setZN(Y); cycles_until_cpu_boundary+=3; break; }
            case 0xB4: { /* LDY zp,X */ uint8_t zp=fetch8(); Y=mem->read((uint8_t)(zp+X)); setZN(Y); cycles_until_cpu_boundary+=4; break; }
            case 0xAC: { /* LDY abs */ uint16_t a=fetch16(); Y=mem->read(a); setZN(Y); cycles_until_cpu_boundary+=4; break; }
            case 0xBC: { /* LDY abs,X */ uint16_t a=fetch16(); Y=mem->read((uint16_t)(a+X)); setZN(Y); cycles_until_cpu_boundary+=4; break; }

                // ---- Extra stores (zero page) ----
            case 0x86: { /* STX zp */ uint8_t zp=fetch8(); mem->write(zp,X); cycles_until_cpu_boundary+=3; break; }
            case 0x84: { /* STY zp */ uint8_t zp=fetch8(); mem->write(zp,Y); cycles_until_cpu_boundary+=3; break; }

                // ---- INC/DEC memory (used by a lot of init code) ----
            case 0xE6: { /* INC zp */ uint8_t zp=fetch8(); uint8_t v=mem->read(zp); v=(uint8_t)(v+1); mem->write(zp,v); setZN(v); cycles_until_cpu_boundary+=5; break; }
            case 0xC6: { /* DEC zp */ uint8_t zp=fetch8(); uint8_t v=mem->read(zp); v=(uint8_t)(v-1); mem->write(zp,v); setZN(v); cycles_until_cpu_boundary+=5; break; }
            case 0xEE: { /* INC abs */ uint16_t a=fetch16(); uint8_t v=mem->read(a); v=(uint8_t)(v+1); mem->write(a,v); setZN(v); cycles_until_cpu_boundary+=6; break; }
            case 0xCE: { /* DEC abs */ uint16_t a=fetch16(); uint8_t v=mem->read(a); v=(uint8_t)(v-1); mem->write(a,v); setZN(v); cycles_until_cpu_boundary+=6; break; }


            // ---- NOP ----
            case 0xEA: cycles_until_cpu_boundary += 2; break;

            // ---- Default: treat unknown as NOP ----
            default:   cycles_until_cpu_boundary += 2; break;
        }
    }

    std::string CPU::lookupInstruction(int opcode) {
        switch (opcode) { case 0x01: return "ORA"; default: return "???"; }
    }
}
