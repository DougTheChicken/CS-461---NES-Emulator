#include "nes/ppu.hpp"

namespace nes {


    // ============================================================================
    // PPU
    // ============================================================================

    PPU::PPU() : bg(*this), sprites(*this), timing(*this) { reset(); }
    void PPU::reset() {}

    uint8_t PPU::cpu_read_ppudata()
    {
        return 1;
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

    const uint32_t* PPU::framebuffer_output() const { return framebuffer; };

}
