#include "nes/ppu.hpp"

namespace nes {

    // ============================================================================
    // PPU
    // ============================================================================

    PPU::PPU() : bg(*this), sprites(*this), timing() { reset(); }

    void PPU::reset()
    {
        oam_address = 0;
        // TODO: more to do here
    }

    void PPU::step() {
        // TODO: More to do here
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

        // address mirroring: CPU can write $2002 via $2008, $2010, ...
        // mask address &= 0x2007.
        switch (address & 0x2007)
        {
        case 0x2000: // PPUCTRL https://www.nesdev.org/wiki/PPU_registers#PPUCTRL_-_Miscellaneous_settings_($2000_write)
            {
                base_nametable_select                   = (value & 0x03);
                vram_address_increment_flag             = (value & 0x04) != 0;
                sprite_pattern_table_address_flag       = (value & 0x08) != 0;
                background_pattern_table_address_flag   = (value & 0x10) != 0;
                sprite_size_flag                        = (value & 0x20) != 0;
                ppu_master_slave_flag                   = (value & 0x40) != 0;
                vblank_nmi_flag                         = (value & 0x80) != 0;

                // temp vram t bits 11-10 are for nametable select and come from PPUCTRL 1-0
                t = (t & 0xF3FF) | ((value & 0x03) << 10);

                break;
            }
        case 0x2001: // PPUMASK https://www.nesdev.org/wiki/PPU_registers#PPUMASK_-_Rendering_settings_($2001_write)
            {
                greyscale_flag              = (value & 0x01) != 0;
                leftmost_background_flag    = (value & 0x02) != 0;
                leftmost_sprite_flag        = (value & 0x04) != 0;
                background_rendering_flag   = (value & 0x08) != 0;
                sprite_rendering_flag       = (value & 0x10) != 0;
                emphasize_red               = (value & 0x20) != 0;
                emphasize_green             = (value & 0x40) != 0;
                emphasize_blue              = (value & 0x80) != 0;

                break;
            }
        case 0x2003: // OAMADDR https://www.nesdev.org/wiki/PPU_registers#OAMADDR_-_Sprite_RAM_address_($2003_write)
            {
                oam_address = value;
                break;
            }
            // from https://www.nesdev.org/wiki/PPU_registers#OAMDATA_-_Sprite_RAM_data_($2004_read/write)
        case 0x2004:
            {
                // reads do not increment OAMADDR after reading but writes DO
                oam[oam_address++] =value;
                break;
            }
        case 0x2005: // PPUSCROLL https://www.nesdev.org/wiki/PPU_registers#PPUSCROLL
            {
                // change the scroll position, telling the PPU which pixel of the nametable selected through
                // PPUCTRL should be at the top left corner of the rendered screen.
                // PPUSCROLL takes two writes:
                // the first is the X scroll and the second is the Y scroll.
                // Whether this is the first or second write is tracked internally by the w register,
                if (!w)
                {
                    // first write is horizontal x
                    // fine X   = value bits 2–0  → x
                    x = value & 0x07;

                    // coarse X = value bits 7–3  → t bits 4–0
                    t = (t & 0xFFE0) | (value >> 3);
                }
                else
                {
                    // second write is vertical y
                    // fine Y   = value bits 2–0  → t bits 14–12
                    t = (t & 0x8FFF) | ((value & 0x07) << 12); // fine Y

                    // coarse Y = value bits 7–3  → t bits 9–5
                    t = (t & 0xFC1F) | ((value & 0xF8) << 2);
                }

                // flip first write bit
                w = !w;
                break;
            }
        case 0x2006: // PPUADDR https://www.nesdev.org/wiki/PPU_registers#PPUADDR
            {
                // Whether this is the first or second write is tracked by the PPU's internal w register
                if (!w)
                {
                    // we carefully write temporary VRAM t while preserving x and scroll by masking
                    // by writing top 6 bits
                    t = (t & 0x00FF) | ((value & 0x3F) << 8);

                    // "However, bit 14 of the internal t register that holds the data written to PPUADDR is forced
                    // to 0 when writing the PPUADDR high byte. This detail doesn't matter when using PPUADDR to set
                    // a VRAM address, but is an important limitation when using it to control mid-screen scrolling"
                    // so force bit 14 to 0 but don't touch any other bits
                    t &= ~0x4000;
                }
                else
                {
                    // write just the low byte
                    t = (t & 0xFF00) | value;

                    // copy to current VRAM  address
                    v = t;
                }

                // flip first write bit
                w = !w;
                break;
            }
        case 0x2007: // PPUDATA
            {
                ppu_bus_write(v, value);
                increment_v();
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

    bool PPU::rendering_enabled() const { return sprite_rendering_flag || background_rendering_flag; }

    // ============================================================================
    // Scanline
    // ============================================================================

    void Scanline::reset() {
        odd_frame_ = false;
        scanline_ = -1;
        cycle_ = 0;
        frame_count_ = 0;
    }

    // https://www.nesdev.org/wiki/PPU_frame_timing#Even/Odd_Frames
    void Scanline::tick(const bool rendering_enabled) {
        // on prerender scanline cycle jump from 339,261 to 0,0
        if (scanline_ == -1 && cycle_ == 339 && odd_frame_ && rendering_enabled) {
            cycle_ = 0;
            scanline_ = 0;
            // odd_frame_ = false; TODO: delete this?
            return;
        }

        // PPU advances in PPU clock cycles (often called dots) through each scanline
        cycle_++;
        // each scanline lasts for 341 PPU clock cycles
        if (cycle_ > 340) {
            // after the last cycle of a scanline, the next scanline begins at cycle 0
            cycle_ = 0;
            // PPU renders 262 scanlines per frame; scanline counter advances each scanline.
            scanline_++;

            // Visible scanlines are 0-239; vblank scanlines are 241-260; pre-render is -1 (aka 261)
            if (scanline_ > 260) {
                // next is the pre-render scanline
                scanline_= -1;
                // PPU has an even/odd flag toggled every frame, regardless of rendering enabled/disabled.
                odd_frame_= !odd_frame_;
                frame_count_++;
            }
        }
    }

    bool Scanline::odd_frame() const { return odd_frame_; };
    int16_t Scanline::scanline() const { return scanline_; };
    uint16_t Scanline::cycle() const { return cycle_; };
    uint32_t Scanline::frame_count() const { return frame_count_; };

    // https://www.nesdev.org/wiki/PPU_rendering#Vertical_blanking_lines_(241-260)
    bool Scanline::is_vblank_start() const { return scanline_ == 241 && cycle_ == 1; };
    bool Scanline::is_vblank_end() const { return is_prerender_scanline() && cycle_ == 1; };

    // https://www.nesdev.org/wiki/PPU_rendering#Pre-render_scanline_(-1_or_261)
    bool Scanline::is_prerender_scanline() const { return scanline_ == -1; };

    // https://www.nesdev.org/wiki/PPU_rendering#Visible_scanlines_(0-239)
    bool Scanline::is_visible_scanline() const { return scanline_ >= 0 && scanline_ <= 239; };
    bool Scanline::is_render_scanline() const { return is_prerender_scanline() || is_visible_scanline(); };
    bool Scanline::is_postrender_scanline() const  { return scanline_ == 240 ;}

    // https://www.nesdev.org/wiki/PPU_rendering#Cycle_0
    bool Scanline::is_start_of_scanline() const { return cycle_ == 0; };
    bool Scanline::is_frame_start() const { return is_prerender_scanline() && is_start_of_scanline(); };

    // https://www.nesdev.org/wiki/PPU_rendering#Cycles_1-256
    bool Scanline::is_visible_cycle() const { return cycle_ >=1 && cycle_ <= 256; };

    // https://www.nesdev.org/wiki/PPU_rendering#Cycles_321-336
    bool Scanline::is_prefetch_cycle()  const { return cycle_ >= 321 && cycle_ <= 336; };
    bool Scanline::is_end_of_scanline() const  { return cycle_ == 340; }

    // sprite and bg fetch helpers to avoid collisions
    bool Scanline::is_bg_fetch_cycle() const { return is_visible_cycle() || is_prefetch_cycle(); };

    // https://www.nesdev.org/wiki/PPU_sprite_evaluation#Details
    // https://www.nesdev.org/w/images/default/thumb/4/4f/Ppu.svg/2560px-Ppu.svg.png
    bool Scanline::is_sprite_fetch_cycle() const { return cycle_ >= 257 && cycle_ <= 320; };
}
