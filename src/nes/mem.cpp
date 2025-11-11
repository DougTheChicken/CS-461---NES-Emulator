#include "nes/mem.hpp"
#include <cstdio>

Memory::Memory() {
    std::memset(ram, 0, sizeof(ram));
    ppu_status = 0x80; // pretend we're in VBlank so $2002 returns bit7=1
}

void Memory::map_prg(const uint8_t* prg_data, size_t prg_size) {
    prg = prg_data;
    prg_len = prg_size; // 16384 or 32768
}

uint8_t Memory::read(uint16_t addr) {
    // $0000–$1FFF: 2KB RAM mirrored every 0x800
    if (addr < 0x2000) return ram[addr & 0x07FF];

    // $2000–$3FFF: PPU regs mirrored every 8 bytes
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        uint16_t reg = 0x2000 | (addr & 0x0007);
        switch (reg) {
            case 0x2002: {
                // PPUSTATUS read; return VBlank set so BPL (branch on N=0) falls through.
                uint8_t val = static_cast<uint8_t>(ppu_status | 0x80);
                // debug once in a while so we know it's used
                static int dbg = 0;
                if ((dbg++ % 64) == 0) {
                    std::fprintf(stderr, "[mem] READ $2002 -> %02X\n", val);
                }
                return val;
            }
            default:
                return 0x00; // other PPU regs unimplemented
        }
    }

    // $8000–$FFFF: PRG ROM (mirror 16KB if needed)
    if (addr >= 0x8000 && prg && prg_len) {
        size_t off = static_cast<size_t>(addr - 0x8000);
        if (prg_len == 0x4000) off &= 0x3FFF; // mirror 16KB into $C000–$FFFF
        return prg[off];
    }

    // everything else (APU/IO) unimplemented
    return 0x00;
}

void Memory::write(uint16_t addr, uint8_t data) {
    if (addr < 0x2000) {
        ram[addr & 0x07FF] = data;
        return;
    }

    // $2000–$3FFF: PPU regs mirrored; ignore writes for now
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        return;
    }

    // Ignore all other writes (APU/IO/mappers) for v0.0.2
}
