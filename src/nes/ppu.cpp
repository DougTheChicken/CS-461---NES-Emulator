#include "nes/ppu.hpp"

namespace nes {


    // ============================================================================
    // PPU
    // ============================================================================

    PPU::PPU() : bg(*this), sprites(*this), timing(*this) { reset(); }
    void PPU::reset() {}

    // https://www.nesdev.org/wiki/PPU_registers#PPUDATA_-_VRAM_data_($2007_read/write)
    uint8_t PPU::cpu_read_ppudata()
    {
        uint16_t addr = v & 0x3FFF;
        uint8_t value = 0;

        if (addr < 0x3F00)
        {
            // Normal buffered VRAM read
            value = data_buffer;
            data_buffer = ppu_bus_read(addr);
        }
        else
        {
            // Palette read: return palette immediately (no priming needed)
            value = ppu_bus_read(addr) & 0x3F;

            // Under-the-overlay read fills the buffer "as normal"
            data_buffer = ppu_bus_read(addr & 0x2FFF);
        }

        // tricky, so we only implement v increment logic once
        increment_v();

        return value;
    }

    void PPU::cpu_write_ppudata(uint8_t value)
    {

    }

    uint8_t PPU::cpu_read_register(uint16_t address)
    {
        return 1;
    }

    void PPU::cpu_write_register(uint16_t address, uint8_t value)
    {

    }

    void PPU::cpu_write_oamdma(uint8_t page)
    {

    }

    uint8_t PPU::ppu_bus_read(uint16_t address)
    {

    }

    void PPU::increment_v()
    {
        v += vram_address_increment_flag ? 32 : 1;
    }

    const uint32_t* PPU::framebuffer_output() const { return framebuffer; };

}
