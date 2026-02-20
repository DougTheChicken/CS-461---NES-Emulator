#include "nes/mem.hpp"
#include <cstring>
#include <cstdio>
#include "nes/apu.hpp"
#include "nes/ppu.hpp"

namespace nes {

Memory::Memory(PPU& ppu_, APU& apu_) : ppu(ppu_), apu(apu_) {
    reset();
}

void Memory::reset() {
    std::memset(ram, 0, sizeof(ram));
    //std::memset(ppu_regs, 0, sizeof(ppu_regs));
    //std::memset(apu_regs, 0, sizeof(apu_regs));

    controller1 = controller2 = 0;
	controller1_shift = controller2_shift = 0;
	strobe = false;
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
        return ppu.cpu_read_register(r);
    }

    // $4000-$4017: APU and I/O
    if (addr < 0x4018) {
        if (addr == 0x4016) {
            // If strobe is high, continuously read the 'A' button state
            if (strobe) return controller1 & 1;

            // Otherwise, read bit 0 and shift right
            uint8_t ret = controller1_shift & 1;
            controller1_shift >>= 1;
            return ret;
        }
        if (addr == 0x4017) {
            if (strobe) return controller2 & 1;

            uint8_t ret = controller2_shift & 1;
            controller2_shift >>= 1;
            return ret;
        }
        if (addr == 0x4014) return oam_dma;
        if (addr == 0x4015) return apu.read_status();
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
        //uint16_t r = 0x2000 + (addr & 0x7);
        // ppu_regs[r & 0x7] = value;
        // compute the register directly and write the value
        ppu.cpu_write_register(0x2000 | (addr & 0x7), value);
        return;
    }

    // $4000-$4017: APU and I/O
    if (addr < 0x4018) {
        if (addr == 0x4014) { oam_dma = value; return; }

        // Writing to 0x4016 controls the strobe latch for BOTH controllers
        if (addr == 0x4016) {
            strobe = (value & 1);
            if (strobe) {
                // While strobe is high, copy physical state into shift registers
                controller1_shift = controller1;
                controller2_shift = controller2;
            }
            return;
        }

        // Note: Writing to 0x4017 does NOT affect the controllers on the NES. 
        // It is strictly for the APU frame counter.
        if (addr == 0x4017) {
            apu_regs[addr - 0x4000] = value;
            return;
        }

        apu_regs[addr - 0x4000] = value;
        return;
    }

    // ROM space ignored for now
}

} // namespace nes
