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
        open_bus = ram[addr & 0x07FF];
        return open_bus;
    }

    // $2000-$3FFF: PPU registers, mirrored every 8 bytes
    if (addr < 0x4000) {
        uint16_t r = 0x2000 + (addr & 0x7);
        open_bus = ppu.cpu_read_register(r);
        return open_bus;
    }

    // $4000-$4017: APU and I/O
    if (addr < 0x4018) {
        if (addr == 0x4016) {
            // If strobe is high, continuously read the 'A' button state
            if (strobe) { open_bus = controller1 & 1; return open_bus; }

            // Otherwise, read bit 0 and shift right
            uint8_t ret = controller1_shift & 1;
            controller1_shift >>= 1;
            open_bus = ret;
            return open_bus;
        }
        if (addr == 0x4017) {
            if (strobe) { open_bus = controller2 & 1; return open_bus; }

            uint8_t ret = controller2_shift & 1;
            controller2_shift >>= 1;
            open_bus = ret;
            return open_bus;
        }
        if (addr == 0x4014) return open_bus;
        if (addr == 0x4015) { open_bus = apu.read_status(); return open_bus; }
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
        open_bus = prg[offset];
        return open_bus;
    }

    return open_bus;
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
        if (addr == 0x4014) {
            oam_dma = value;
            // we need to copy the page to the PPU
            uint8_t page[256];
            uint16_t base = static_cast<uint16_t>(value) << 8;
            for (uint16_t i = 0; i < 256; i++) page[i] = read(base + i);
            ppu.oam_dma_execute(page);
            return;
        }

        // Writing to 0x4016 controls the strobe latch for BOTH controllers
        if (addr == 0x4016) {
            bool prev = strobe;
            strobe = (value & 1) != 0;

            // Latch controller state when strobe is high OR on falling edge (1 -> 0)
            if (strobe || (prev && !strobe)) {
                controller1_shift = controller1;
                controller2_shift = controller2;
            }
            return;
        }

        // Everything else to the APU!
        apu.write_register(addr, value);
    }

    // ROM space ignored for now
}

} // namespace nes
