#include "nes/mem.hpp"
#include <cstring>
#include <cstdio>
#include "nes/apu.hpp"
#include "nes/ppu.hpp"
#include "nes/ROM.hpp"

namespace nes {

    Memory::Memory(PPU& ppu_, APU& apu_) : ppu(ppu_), apu(apu_) {
        reset();
    }

    void Memory::reset() {
        std::memset(ram, 0, sizeof(ram));

        controller1 = controller2 = 0;
        controller1_shift = controller2_shift = 0;
        strobe = false;
        oam_dma = 0;
        cartridge = nullptr; // Clear the inserted cartridge on reset
    }

    uint8_t Memory::read(uint16_t addr) {
        // 1. Ask the cartridge first! (Covers $8000-$FFFF PRG ROM and $6000-$7FFF Save RAM)
        if (cartridge && cartridge->cpuRead(addr, open_bus)) {
            return open_bus;
        }

        // 2. Internal RAM ($0000-$1FFF, mirrored every 2KB)
        if (addr < 0x2000) {
            open_bus = ram[addr & 0x07FF];
            return open_bus;
        }

        // 3. PPU registers ($2000-$3FFF), mirrored every 8 bytes
        if (addr < 0x4000) {
            uint16_t r = 0x2000 + (addr & 0x7);
            open_bus = ppu.cpu_read_register(r);
            return open_bus;
        }

        // 4. APU and I/O ($4000-$4017)
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

        return open_bus;
    }

    void Memory::write(uint16_t addr, uint8_t value) {
        open_bus = value;

        // 1. Ask the cartridge first! (Covers mapper register writes and Save RAM)
        if (cartridge && cartridge->cpuWrite(addr, value)) {
            return;
        }

        // 2. Internal RAM ($0000-$1FFF, mirrored every 2KB)
        if (addr < 0x2000) {
            ram[addr & 0x07FF] = value;
            return;
        }

        // 3. PPU registers ($2000-$3FFF, mirrored every 8 bytes)
        if (addr < 0x4000) {
            ppu.cpu_write_register(0x2000 | (addr & 0x7), value);
            return;
        }

        // 4. APU and I/O ($4000-$4017)
        if (addr < 0x4018) {
            if (addr == 0x4014) {
                oam_dma = value;
                // we need to copy the page to the PPU
                uint8_t page[256];
                uint16_t base = static_cast<uint16_t>(value) << 8;
                for (uint16_t i = 0; i < 256; i++) page[i] = read(base + i);

                ppu.oam_dma_execute(page);
                ppu.oam_dma_pending_stall = 513 * CPU_TO_PPU;
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
    }

} // namespace nes