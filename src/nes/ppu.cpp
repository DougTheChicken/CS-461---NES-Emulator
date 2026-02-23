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

        // TODO: background scanlines and cycles

        // vblank flag: set at scanline 241 cycle 1, cleared at pre-render cycle1
        if (timing.is_vblank_start())
        {
            vblank_flag = true;
            // post an interrupt to the cpu if vblank nmi is set
            if (vblank_nmi_flag) { nmi_pending = true; }
        }

        // vblank is over. pencils down. i hope you finished all your work.
        if (timing.is_vblank_end())
        {
            // clean up for render time
            vblank_flag = false;
            nmi_pending = false;
            sprite_zero_hit_flag = false;
            sprite_overflow_flag = false;
        }

        // scanlines 0-239
        if (timing.is_visible_scanline())
        {
            // https://www.nesdev.org/wiki/PPU_sprite_evaluation
            // Each scanline, the PPU reads the spritelist (that is, Object Attribute Memory) to see which to draw
            if (timing.is_start_of_scanline())
            {
                sprites.begin_scanline(timing.scanline());
            }
            // 64 bytes of secondary OAM cleared
            else if (timing.is_sprite_clear_cycle())
            {
                // First, it clears the list of sprites to draw in secondary oam, 1 byte at time
                sprites.clear(timing.cycle());
            }
            else if (timing.is_sprite_evaluation_cycle())
            {
                // Second, it reads through OAM, checking which sprites will be on this scanline.
                // It chooses the first eight it finds that do.
                sprites.evaluate(timing.scanline(), timing.cycle());
            }
            else if (timing.is_sprite_fetch_cycle())
            {
                sprites.fetch(timing.scanline(), timing.cycle());
            }
        }

        // tick once per PPU cycle
        timing.tick(rendering_enabled());
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
        // 14-bits of address space in palette addresses
        address &= 0x3FFF;

        // mirrors $3F20-$3FFF down to $3F00-$3F1F, $3F10 becomes 0x10, etc
        uint8_t i = address & 0X001F;
        if (i == 0x10) i = 0x00;
        if (i == 0x14) i = 0x04;
        if (i == 0x18) i = 0x08;
        if (i == 0x1C) i = 0x0C;

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
    bool Scanline::is_sprite_clear_cycle() const { return cycle_ >=1 && cycle_ <= 64; };
    bool Scanline::is_sprite_evaluation_cycle() const { return cycle_ >= 65 && cycle_ <= 256; };
    bool Scanline::is_sprite_fetch_cycle() const { return cycle_ >= 257 && cycle_ <= 320; };

    // ============================================================================
    // SpritePipeline
    // ============================================================================

    // clear 1 byte of secondary oam
    void SpritePipeline::clear(int cycle)
    {
        // clearing secondary oam with 0xFF means they never match the
        // scanline by accident
        secondary_oam[cycle - 1] = 0xFF;
    }

    // Does this sprite intersect this scanline?
    bool SpritePipeline::intersects(int scanline, uint8_t top)
    {
        int sprite_height = ppu.sprite_size_flag ? 16 : 8;
        return (scanline >= top) && (scanline < top + sprite_height);
    }

    // there are 64 sprites, each is 4-bytes: x, tile, attr, y
    // we need to walk each one and see if it is interesected by this scanline
    // and then copy the hits into our secondary oam, tra
    void SpritePipeline::evaluate(int scanline, int cycle)
    {
        // if we already scanned all sprites, we are done
        if (oam_scan_index >= 64) return;

        int base = oam_scan_index * 4;

        // extract fields
        uint8_t y = ppu.oam[base];
        uint8_t tile = ppu.oam[base + 1];
        uint8_t attr = ppu.oam[base + 2];
        uint8_t x = ppu.oam[base + 3];

        if (intersects(scanline, y))
        {
            // got a hit but we only handle up to 8 sprites per scanline
            if (secondary_count < 8)
            {
                // secondary oam is 256 bytes of 64 sprites x 4 bytes each,
                // get our starting offeset from sprite #
                int dest = secondary_count * 4;

                // store our hit sprite in secondary oam so we can draw it later
                secondary_oam[dest] = y;
                secondary_oam[dest + 1] = tile;
                secondary_oam[dest + 2] = attr;
                secondary_oam[dest + 3] = x;

                // store state for fetch and draw
                spr_x[secondary_count] = x; // x counter to avoid oam until needed
                spr_attr[secondary_count] = attr; // palette, front/back, reversed

                // sprite 0 hit detection, take low 8 bits of sprite index (0..7). we check for sprite 0 during draw.
                spr_oam_index[secondary_count] = static_cast<uint8_t>(oam_scan_index);

                secondary_count++;
            }
            else
            {
                // more than 8 sprites, so declare an overflow
                overflow_ = true;
            }
        }
        oam_scan_index++;
    }

    // Cycles 257-320: Sprite fetches (8 sprites total, 8 cycles per sprite)
    void SpritePipeline::fetch(int scanline, int cycle)
    {
        // once per scanline
        if (cycle == 257) { clear_unused_shifters(); }

        // 8 sprites
        uint8_t sprite_index = (cycle - 257) / 8;

        // 8 cycles per sprite
        uint8_t phase = (cycle - 257) % 8;

        // can't go over the max
        if (sprite_index >= secondary_count) { return; }

        // extract sprite fields
        int base = sprite_index * 4;
        uint8_t y = secondary_oam[base];
        uint8_t tile = secondary_oam[base + 1];
        uint8_t attr = secondary_oam[base + 2];
        uint8_t x = secondary_oam[base + 3];

        // from https://www.nesdev.org/wiki/PPU_rendering#Cycles_257-320
        // The tile data for the sprites on the next scanline are fetched here.
        // Again, each memory access takes 2 PPU cycles to complete,
        // and 4 are performed for each of the 8 sprites:
        // 0 Garbage nametable byte but we store state for drawing and get our pattern table address
        // 2 Garbage nametable byte
        // 4 Pattern table tile low
        // 6 Pattern table tile high (8 bytes above pattern table tile low address)
        if (phase == 0)
        {
            // store state for draw
            spr_x[secondary_count] = x; // x counter to avoid oam until needed
            spr_attr[secondary_count] = attr; // palette, front/back, reversed

            // latch pattern address for this sprite
            spr_pattern_addr[sprite_index] = compute_pattern_address(scanline, y, tile, attr);
        }
        else if (phase == 4)
        {
            spr_shift_low[sprite_index] =
                maybe_flipped_h(attr, ppu.ppu_bus_read(spr_pattern_addr[sprite_index]));
        }
        else if (phase == 6)
        {
            spr_shift_high[sprite_index] =
                maybe_flipped_h(attr, ppu.ppu_bus_read(spr_pattern_addr[sprite_index] + 8));
        }
    }

    // Returns the pattern address for the low byte of the sprite pattern row, so add 8 bytes to result to get the high
    uint16_t SpritePipeline::compute_pattern_address(
        int scanline,
        uint8_t y,
        uint8_t tile,
        uint8_t attr
    )
    {
        const int sprite_height = ppu.sprite_size_flag ? 16 : 8;

        // fine_y = scanline offset within the sprite
        // 0..7 for 8x8 or 0..15 for 8x16
        int fine_y = scanline - y;

        // Vertical flip? (attr bit 7)
        if (attr & 0x80)
            fine_y = (sprite_height - 1) - fine_y;  // takes a fine_y of 0 1 2 3 4 ... and turns it into ... 4 3 2 1 0

        if (!ppu.sprite_size_flag) // 8x16 when set
        {
            // from https://www.nesdev.org/wiki/PPU_programmer_reference#PPUCTRL_-_Miscellaneous_settings_($2000_write)
            // in 8x8, sprite_pattern_table_address_flag (0: $0000; 1: $1000; ignored in 8x16 mode)
            uint16_t table_base = ppu.sprite_pattern_table_address_flag ? 0x1000 : 0x0000;

            // 4kb base + (each tile is 16 bytes) + row into the sprite = btye address into pattern table for the row
            return table_base + (tile * 16) + fine_y;
        }

        // from https://www.nesdev.org/wiki/PPU_programmer_reference#Byte_1_-_Tile/index
        // "For 8x16 sprites (bit 5 of PPUCTRL set), the PPU ignores the pattern table selection and
        // selects a pattern table from bit 0 of this number.
        // Bits 7-0: Tile number of top of sprite (0 to 254; bottom half gets the next tile)"
        uint16_t table_base = (tile & 0x01) ? 0x1000 : 0x0000;

        // valid top indexes are even, so first force an even index from the tile to make next step easy
        uint8_t tile_index = tile & 0xFE;

        // for our given line, with 16 lines of y, we stay here on 0..7 and move up 1 on 8-15
        uint8_t tile_for_row = (fine_y < 8) ? tile_index : (tile_index + 1);

        // 4kb base + (each tile is 16 bytes) + (our row is the low 3 bits of fine y from either high/low sprite we had)
        return table_base + ((tile_for_row) * 16) + (fine_y & 0x07);
    }

    // horizontally flip the byte if attr says so
    uint8_t SpritePipeline::maybe_flipped_h(const uint8_t attributes, const uint8_t byte)
    {
        // attr holds "flip horizontal" and if so, we use a lookup table to reverse the bits
        if ((attributes & 0x40) != 0) { return reverse_table[byte]; }
        return byte;
    }

    // returns true if attr
    bool SpritePipeline::flip_vertical(const uint8_t attributes) { return (attributes & 0x80) != 0; }

    void SpritePipeline::clear_unused_shifters()
    {
        for (int i = secondary_count; i < 8; i++)
        {
            spr_shift_low[i] = 0;
            spr_shift_high[i] = 0;
        }
    }

    // reset everything and take it from the top
    void SpritePipeline::begin_scanline(int scanline)
    {
        // # of sprites in secondary oam
        secondary_count = 0;

        // primary oam selector
        oam_scan_index = 0;

        // whether we exceed 8 sprites on the current scanline
        overflow_ = false;
    }

    bool SpritePipeline::overflow() const { return overflow_; }


}

// ============================================================================
// BackgroundPipeline
// ============================================================================

    // brings all registers and latches to 0
    void BackgroundPipeline::reset() 
    {
        bg_pattern_low = 0;
        bg_pattern_high = 0;
        bg_attr_low = 0;
        bg_attr_high = 0;
        next_pattern_high = 0;
        next_pattern_low = 0;
        next_tile_id = 0;
        next_attr = 0;
    }

    // advance internal shift registers by 1 bit
    // should be called every ppu cycle when rendering is enabled
    void BackgroundPipeline::tick() 
    {
        bg_pattern_low <<= 1;
        bg_pattern_high <<= 1;
        bg_attr_low <<= 1;
        bg_attr_high <<= 1;
    }

    // transfers the next latched bytes into the shift registers
    // should occur every 8 cycles
    void BackgroundPipeline::reload() 
    {
        // load pattern data into the lower 8 bits
        bg_pattern_low  = (bg_pattern_low & 0xFF00)  | next_pattern_low;
        bg_pattern_high = (bg_pattern_high & 0xFF00) | next_pattern_high;

        // handle attribute bit 0
        // logic: clear lower 8 bits and fill with 1s
        bg_attr_low &= 0xFF00;
        if (next_attr & 0x01) {
            bg_attr_low |= 0x00FF;
        }

        // handle attribute bit 1
        // logic: clear lower 8 bits and fill with 1s
        bg_attr_high &= 0xFF00;
        if (next_attr & 0x02) {
            bg_attr_high |= 0x00FF;
        }
    }

    // logic for the 8-cycle fetch phase
    // should populate the 'next_' latches using the current vram address
    void BackgroundPipeline::fetch_nametable()      // 1-2: fetch nametable
    {
        uint16_t base = 0x2000;
        
        // determine which nametable we are currently in
        uint16_t nametable_offset = (ppu.v & 0x0C00); 
        
        // determine which row & column
        uint16_t cell_offset = (ppu.v & 0x03FF); 

        // determine which exact tile needs to be drawn
        uint16_t final_address = base + nametable_offset + cell_offset;

        // save the tile index for the next stages
        next_tile_id = ppu.ppu_bus_read(final_address);
    }
    void BackgroundPipeline::fetch_attribute()      // 3-4: fetch attribute
    {
        // attribute address calculation
        uint16_t address = 0x23C0 | (ppu.v & 0x0C00) | ((ppu.v >> 4) & 0x38) | ((ppu.v >> 2) & 0x07);
        uint8_t raw_byte = ppu.ppu_bus_read(address);

        // shift the byte to get the correct 2 bits for the current pixel area
        // if in the bottom half of a 32x32 attribute block, shift 4 bits
        if (ppu.v & 0x0040) raw_byte >>= 4; 
        // if in the right half of a 32x32 attribute block, shift 2 bits
        if (ppu.v & 0x0002) raw_byte >>= 2;

        next_attr = raw_byte & 0x03; // mask everything but the needed 2 bits
    }
    void BackgroundPipeline::fetch_pattern_low()    // 5-6: fetch low pattern
    {
        uint16_t fine_y = (ppu.v >> 12) & 0x07;
        uint16_t table_base = ppu.background_pattern_table_address_flag ? 0x1000 : 0x0000;
        uint16_t address = table_base + (next_tile_id * 16) + fine_y;
        next_pattern_low = ppu.ppu_bus_read(address);
    }
    void BackgroundPipeline::fetch_pattern_high()    // 7-8: fetch high pattern
    {
        uint16_t fine_y = (ppu.v >> 12) & 0x07;
        uint16_t table_base = ppu.background_pattern_table_address_flag ? 0x1000 : 0x0000;
        // high plane is 8 bytes after the low plane
        uint16_t address = table_base + (next_tile_id * 16) + fine_y + 8;
        next_pattern_high = ppu.ppu_bus_read(address);
    }

    // calculate the 4-bit palette index for the current pixel
    // should use the fine_x scroll (0-7) from the ppu internal 'x' register
    uint8_t BackgroundPipeline::get_pixel() const 
    {
        // determine how many pixels offset from the start we currently are
        // note: this comes from the ppu fine_x scroll
        uint8_t scroll_offset = ppu.x;

        // mask to look at exactly one bit in shift registers
        uint16_t pixel_selector = 0x8000 >> scroll_offset;

        // check each of the data layers for a 1 in this spot
        uint8_t shape_bit_0 = 0;
        if (bg_pattern_low & pixel_selector) {
            shape_bit_0 = 1;        // value if bit 0 is set
        }

        uint8_t shape_bit_1 = 0;
        if (bg_pattern_high & pixel_selector) {
            shape_bit_1 = 2;        // value if bit 1 is set
        }

        // Attribute Layer (The "Color Palette" for the tile)
        uint8_t palette_bit_0 = 0;
        if (bg_attr_low & pixel_selector) {
            palette_bit_0 = 4;      // value if bit 2 is set
        }

        uint8_t palette_bit_1 = 0;
        if (bg_attr_high & pixel_selector) {
            palette_bit_1 = 8;      // value if bit 3 is set
        }

        // add all bits together to determine the final colour index (0-15)
        uint8_t pixel_index = shape_bit_0 + shape_bit_1 + palette_bit_0 + palette_bit_1;
        return pixel_index;
    }
}
