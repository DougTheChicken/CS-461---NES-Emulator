#include "nes/ppu.hpp"

namespace nes {

    // ============================================================================
    // PPU
    // ============================================================================

    PPU::PPU() : bg(*this), sprites(*this), timing(*this) { reset(); }

    void PPU::reset()
    {
        oam_address = 0;
    }

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

    // from https://www.nesdev.org/wiki/PPU_programmer_reference#PPUDATA_-_VRAM_data_($2007_read/write)
    // write the value at the v address and increment v
    void PPU::cpu_write_ppudata(uint8_t value)
    {
        // masked to PPUDATA region for safety
        uint16_t addr = v & 0x3FFF;
        ppu_bus_write(addr, value);
        increment_v();
    }

    // from https://www.nesdev.org/wiki/PPU_registers#Summary
    // there are only 3 legitimate readable registers on the PPU,
    // $2002 PPUSTATUS
    // $2004 OAMDATA
    // $2007 PPUDATA
    // all other reads should return open bus (last value placed on the CPU data bus)
    uint8_t PPU::cpu_read_register(uint16_t address)
    {
        // default to last value placed on data bus
        uint8_t value;

        // address mirroring: CPU can read $2002 via $200A, $3FFA, ...
        // mask address &= 0x2007.
        switch (address & 0x2007)
        {
            // from https://www.nesdev.org/wiki/PPU_registers#PPUSTATUS_-_Rendering_events_($2002_read)
            // 7  bit  0
            // ---- ----
            // VSOx xxxx
            // |||| ||||
            // |||+-++++- (PPU open bus or 2C05 PPU identifier)
            // ||+------- Sprite overflow flag
            // |+-------- Sprite 0 hit flag
            // +--------- Vblank flag, cleared on read.
            case 0x2002:
                {
                    // PPUSTATUS contains 3 flags so we assemble it from open bus + our flags
                    value = open_bus & 0x1F;
                    if (vblank_flag) value |= 0x80;
                    if (sprite_zero_hit_flag) value |= 0x40;
                    if (sprite_overflow_flag) value |= 0x20;

                    // "Vblank flag, cleared on read."
                    vblank_flag = false;

                    // "Reading this register has the side effect of clearing the PPU's internal w register."
                    w = false;
                    break;
                }
            // from https://www.nesdev.org/wiki/PPU_registers#OAMDATA_-_Sprite_RAM_data_($2004_read/write)
            // reads do not increment OAMADDR after reading
            case 0x2004:
                {
                    value = oam[oam_address];
                    break;
                }
            case 0x2007:
                {
                    value = cpu_read_ppudata();
                    break;
                }
        default:
                {
                    value = open_bus;
                    break;
                }
        }
        open_bus = value;
        return value;
    }

    // from https://www.nesdev.org/wiki/PPU_registers#Summary
    void PPU::cpu_write_register(uint16_t address, uint8_t value)
    {
        // always place last value written on data bus aka "latch open bus"
        open_bus = value;

        // address mirroring: CPU can write $2002 via $200A, $3FFA, ...
        // mask address &= 0x2007.
        switch (address & 0x2007)
        {
        case 0x2000:
            {
                break;
            }
        case 0x2001:
            {
                break;
            }
        case 0x2003:
            {
                break;
            }
            // from https://www.nesdev.org/wiki/PPU_registers#OAMDATA_-_Sprite_RAM_data_($2004_read/write)
        case 0x2004:
            {
                oam[oam_address] =value;
                break;
            }
        case 0x2005:
            {
                break;
            }
        case 0x2006:
            {
                break;
            }
        case 0x2007:
            {
                break;
            }
        default:
            {
                break;
            }
        }
    }

    // from https://www.nesdev.org/wiki/PPU_OAM#DMA
    // we get a page from Memory and put 256 bytes in oam in 1 shot
    void PPU::oam_dma_execute(const uint8_t* page_data) {
        for (int i = 0; i < 256; i++)
        {
            oam[oam_address] = page_data[i];
            oam_address = static_cast<uint8_t>(oam_address + 1); // heavyweight, but guarantee that wrapping happens
        }
    }

    // given a 14-bit address and a value, read it from the correct memory.
    // the address determines where we read from
    uint8_t PPU::ppu_bus_read(uint16_t address)
    {
        // clamp to 14-bitspace
        address &= 0x3FFF;

        if (address < 0x2000)
        {
            // TODO: is this sufficient for now for cart/CHR reads?
            return chr_ram[address];
        }
        // $2000 - $3EFF nametable read
        if (address < 0x3F00)
        {
            // get index from mirrored address and get the value in the spot
            return nametable_ram[mirror_nametable_address(address)];
        }
        // $3F00 - $3FFF palelette read
        // get index from mirrored address and get the value in the spot
        return palette_ram[mirror_palette_address(address)];
    }

    // given a 14-bit address and a value, write it to the correct memory.
    // teh address determines where we write to
    void PPU::ppu_bus_write(uint16_t address, uint8_t value)
    {
        // clamp to 14-bitspace
        address &= 0x3FFF;

        if (address < 0x2000)
        {
            // TODO: is this sufficient for now for cart/CHR writes?
            chr_ram[address] = value;
        }
        // $2000 - $3EFF nametable write
        else if (address < 0x3F00)
        {
            // get index from mirrored address and stick the value in the spot
            nametable_ram[mirror_nametable_address(address)] = value;
        }
        // $3F00 - $3FFF palelette write
        else if (address < 0x4000)
        {
            // get index from mirrored address and stick the value in the spot
            palette_ram[mirror_palette_address(address)] = value;
        }
    }

    // Memory address mirroring helper
    // https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
    // 4 logical nametables indexed each as a 1kb nametable with an offset
    // $2000–$23FF (NT0)
    // $2400–$27FF (NT1)
    // $2800–$2BFF (NT2)
    // $2C00–$2FFF (NT3)
    uint16_t PPU::mirror_nametable_address(uint16_t address)
    {
        // Fold $3000-$3EFF down into $2000-$2EFF since it is a complete mirror
        address &= 0x2FFF;

        // get our offset into the 4kb space
        uint16_t offset = address - 0x2000;

        // find which nametable (0..3) it belongs to
        uint16_t table  = offset >> 10;

        // get our index into our nametable 0..0x3FF
        // by masking to keep lower 10-bits to get 0..1023
        uint16_t index  = offset & 0x03FF;

        // real physical memory page the address uses: either 0 or 1 that
        // the logical table maps to after mirroring
        uint16_t physical;

        // vertical mirror maps
        // NT0 and NT1 to physical 0
        // NT2 and NT3 to physical 1
        if (vertical_mirroring)
        {
            // use upper bit of logical nametable index
            // 00, 01 become 0 and 10, 11 become 1
            physical = table >> 1;
        }

        // horizontal mirror maps
        // NT0 nad NT2 to physical 0
        // NT1 and NT3 to physical 1
        else
        {
            // user lower bit of logical nametable index
            // 00 -> 0 and 10 -> 0
            // 01 -> 1 and 11 -> 1
            physical = table & 1;
        }

        // physical selects which 1 KB nametable page (0 or 1)
        // index selects the byte within that page (0–1023)
        // final address = page_base + in-page offset which is a
        // final index into 2kb of nametable VRAM 0x000-0x7FF
        return (physical * 0x0400) + index;

    }

    // Memory address mirroring helper
    // Addresses  $3F10, $3F14, $3F18, and $3F1C are
    // mirrors of $3F00, $3F04, $3F08, and $3F0C respectively.
    // yields an index from 0..31 for accessing the palette table
    uint16_t PPU::mirror_palette_address(uint16_t address)
    {
        // $3F10 becomes 0x10, etc
        address &= 0x3FFF;

        uint8_t i = 0;
        if (address == 0x10) i = 0x00;
        if (address == 0x14) i = 0x04;
        if (address == 0x18) i = 0x08;
        if (address == 0x1C) i = 0x0C;

        return i;
    }

    void PPU::increment_v()
    {
        v += vram_address_increment_flag ? 32 : 1;
        v &= 0x7FFF; // let's add some safety to keep v in bounds
    }

    const uint32_t* PPU::framebuffer_output() const { return framebuffer; };

}
