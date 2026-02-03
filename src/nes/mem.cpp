#include "nes/mem.hpp"
#include <cstring>
#include <cstdio>

namespace nes {

Memory::Memory() {
    reset();
}

void Memory::reset() {
    std::memset(ram, 0, sizeof(ram));
    std::memset(ppu_regs, 0, sizeof(ppu_regs));
    std::memset(apu_regs, 0, sizeof(apu_regs));
    controller1 = controller2 = 0;
    oam_dma = 0;
    // prg pointer and size are left as-is; console will re-map on ROM load
}

void Memory::map_prg(const uint8_t* prg_data, std::size_t prg_size) {
    prg = prg_data;
    prg_size_bytes = prg_size;
}

uint8_t Memory::read(uint16_t addr) {
    // $0000-$1FFF: internal RAM, mirrored every 2KB
    if (addr < 0x2000) {
        return ram[addr & 0x07FF];
    }

    // $2000-$3FFF: PPU registers, mirrored every 8 bytes
    if (addr < 0x4000) {
        uint16_t r = 0x2000 + (addr & 0x7);
        // Minimal: return PPUSTATUS bit7 set so tight loops don't hang in early bringup
        if (r == 0x2002) {
            return 0x80;
        }
        return ppu_regs[r & 0x7];
    }

    // $4000-$4017: APU and I/O
    if (addr < 0x4018) {
        if (addr == 0x4016) return controller1;
        if (addr == 0x4017) return controller2;
        if (addr == 0x4014) return oam_dma;
        return apu_regs[addr - 0x4000];
    }

    // $8000-$FFFF: PRG ROM
    if (addr >= 0x8000 && prg && prg_size_bytes) {
        uint32_t offset = addr - 0x8000;
        // NROM-128: 16KB mirrored; NROM-256: 32KB direct
        if (prg_size_bytes == 0x4000) {
            offset &= 0x3FFF;
        } else {
            offset &= (uint32_t)(prg_size_bytes - 1);
        }
        return prg[offset];
    }

    return 0x00;
}

void Memory::write(uint16_t addr, uint8_t value) {
    // $0000-$1FFF: internal RAM
    if (addr < 0x2000) {
        ram[addr & 0x07FF] = value;
        return;
    }

    // $2000-$3FFF: PPU registers
    if (addr < 0x4000) {
        uint16_t r = 0x2000 + (addr & 0x7);
        ppu_regs[r & 0x7] = value;
        return;
    }

    // $4000-$4017: APU and I/O
    if (addr < 0x4018) {
        if (addr == 0x4014) { oam_dma = value; return; }
        if (addr == 0x4016) { controller1 = value; return; }
        if (addr == 0x4017) { controller2 = value; return; }
        apu_regs[addr - 0x4000] = value;
        return;
    }

    // ROM space ignored for now
}

} // namespace nes
