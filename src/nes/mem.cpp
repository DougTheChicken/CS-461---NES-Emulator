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
            case 0x2004: {
                return 0x00;
            }
            case 0x2007: {
                return 0x00;
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
        switch (addr & 0x0007) {
        case 0: // $2000 PPUCTRL
            ppu_ctrl = data;
            // TODO: Trigger NMI logic if bit 7 changes?
            break;
        case 1: // $2001 PPUMASK
            ppu_mask = data;
            break;
        case 2: // $2002 PPUSTATUS
            // READ ONLY: Writing to status usually does nothing
            break;
        case 3: // $2003 OAMADDR
            oam_addr = data;
            break;
        case 4: // $2004 OAMDATA
            // TODO: Write to OAM memory at oam_addr
            break;
        case 5: // $2005 PPUSCROLL
            ppu_scroll = data;
            // Note: Real hardware requires a 'write toggle' here (x/y split)
            break;
        case 6: // $2006 PPUADDR
            ppu_addr = data;
            // Note: Real hardware requires a 'write toggle' here (high/low byte)
            break;
        case 7: // $2007 PPUDATA
            // TODO: Write data to VRAM at ppu_addr, then increment ppu_addr
            break;
        }
        return;
    }

    // Ignore all other writes (APU/IO/mappers) for v0.0.2
}
