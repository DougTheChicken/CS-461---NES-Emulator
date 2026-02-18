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
        // mask off upper two bits: only 14 bits are used for addressing
        // PPU addresses $0000 – $3FFF. Failing to mask ruins mirrors of space.
        uint16_t addr = v & 0x3FFF;
        uint8_t value = 0;

        // from https://www.nesdev.org/wiki/PPU_memory_map
        // buffered reads are in the following ranges
        // $0000–$1FFF  CHR-ROM or CHR-RAM (pattern tables)
        // $2000–$2FFF  Nametables
        // $3000–$3EFF  Mirror of $2000–$2EFF The PPU does not render from this address range, so negligible utility.
        if (addr < 0x3F00)
        {
            // Normal buffered VRAM read
            // Note: VRAM reads are delayed by 1 read so
            // we are going to return what was read last time PPU VRAM was read,
            value = data_buffer;
            //  and we do a read to get ready to provide it the next time we do a read
            data_buffer = ppu_bus_read(addr);
        }
        else
        {
            // Palette read: return palette immediately, no delayed reads
            // $3F00-$3F1F 	Palette RAM indexes
            // $3F20-$3FFF 	Mirrors of $3F00-$3F1
            value = ppu_bus_read(addr) & 0x3F;

            // from https://www.nesdev.org/wiki/PPU_programmer_reference#Reading_palette_RAM
            // Under-the-overlay read fills the buffer "as normal"
            data_buffer = ppu_bus_read(addr & 0x2FFF);
        }

        // tricky, so we only implement v increment logic once and call it when needed
        // cpu reads trigger an increment of v because $2007 is a streaming data port -- the cpu is
        // gonna keep reading bytes until they are gone. it will either increment 1 or 32, depending on PPUCTRL but
        // that's not really our problem. we just get ready for the next read by incrementing after we read.
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
        v &= 0x7FFF; // let's add some safety to keep v in bounds
    }

    const uint32_t* PPU::framebuffer_output() const { return framebuffer; };

}
