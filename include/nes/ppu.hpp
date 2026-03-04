#pragma once
#include <cstdint>
#include <functional>

namespace nes
{
    class PPU;

    class Scanline
    {
    public:

        void reset();
        void tick(bool rendering_enabled);
        bool odd_frame() const;
        int16_t scanline() const;
        uint16_t cycle() const;
        uint32_t frame_count() const;
        bool is_vblank_start() const;
        bool is_vblank_end() const;
        bool is_prerender_scanline() const;
        bool is_postrender_scanline() const;
        bool is_visible_scanline() const;
        bool is_render_scanline() const;
        bool is_visible_cycle() const;
        bool is_prefetch_cycle() const;
        bool is_end_of_scanline() const;
        bool is_start_of_scanline() const;
        bool is_frame_start() const;
        bool is_bg_fetch_cycle() const;
        bool is_sprite_fetch_cycle() const;
        bool is_sprite_clear_cycle() const;
        bool is_sprite_evaluation_cycle() const;

    private:

        // from https://www.nesdev.org/wiki/PPU_rendering#Line-by-line_timing
        // For odd frames, the cycle at the end of the scanline is skipped
        bool odd_frame_ = false;

        // -1 prerender, 0-239 render, 240 post-render; 241-260 vblank
        int16_t scanline_ = -1;

        // 0 - 340 (https://www.nesdev.org/w/images/default/thumb/4/4f/Ppu.svg/2560px-Ppu.svg.png)
        uint16_t cycle_ = 0;

        // frame counter for fun
        uint32_t frame_count_ = 0;


    }; // end class Scanline

    class BackgroundPipeline
    {
    public:
        explicit BackgroundPipeline(PPU& ppu) : ppu(ppu)
        {
            reset();
        }
            // brings all registers and latches to 0
            void reset();

            // advance internal shift registers by 1 bit
            // should be called every ppu cycle when rendering is enabled
            void tick();

            // transfers the next latched bytes into the shift registers
            // should occur every 8 cycles
            void reload();

            // logic for the 8-cycle fetch phase
            // should populate the 'next_' latches using the current vram address
            void fetch_nametable();     // 1-2: fetch nametable
            void fetch_attribute();     // 3-4: fetch attribute
            void fetch_pattern_low();   // 5-6: fetch low pattern
            void fetch_pattern_high();  // 7-8: fetch high pattern

            // calculate the 4-bit palette index for the current pixel
            // should use the fine_x scroll (0-7) from the ppu internal 'x' register
            uint8_t get_pixel() const;

    private:
        PPU& ppu;

        // background shift registers
        uint16_t bg_pattern_low = 0, bg_pattern_high = 0;
        uint16_t bg_attr_low = 0, bg_attr_high = 0;

        // latches for the next 8 pixels, persistent across 8-cycle fetch batch
        uint8_t next_tile_id = 0;
        uint8_t next_attr = 0;
        uint8_t next_pattern_low = 0;
        uint8_t next_pattern_high = 0;
    }; // end class BackgroundPipeline

    class SpritePipeline
    {
    public:
        explicit SpritePipeline(PPU& ppu) : ppu(ppu)
        {
        }

        void reset();
        void clear(int cycle);
        void tick();
        bool is_sprite_zero(int index);
        uint8_t get_pixel(int index);
        bool intersects(int scanline, uint8_t y);
        void evaluate(int scanline, int cycle);
        void fetch(int scanline, int cycle);
        void begin_scanline(int scanline);
        bool overflow() const;
        void clear_unused_shifters();
        static uint8_t maybe_flipped_h(uint8_t attributes, uint8_t byte);
        static bool flip_vertical(uint8_t attributes);
        uint16_t compute_pattern_address(int scanline, uint8_t y, uint8_t tile, uint8_t attr);
        int secondary_count = 0; // # of sprites in secondary oam (eval in progress)
        int render_count = 0;   // # of sprites locked in for current scanline's display

        // render working set: snapshotted from eval at cycle 257 each scanline
        uint8_t spr_x_render[8]{}, spr_attr_render[8]{};
        uint8_t secondary_src_index[8];

       static constexpr uint8_t reverse_table[256] = {
            0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
            0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
            0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
            0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
            0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
            0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
            0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
            0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
            0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
            0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
            0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
            0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
            0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
            0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
            0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
            0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
            0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
            0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
            0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
            0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
            0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
            0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
            0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
            0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
            0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
            0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
            0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
            0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
            0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
            0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
            0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
            0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
        };

    private:
        PPU& ppu;

        // from: https://setsideb.com/behind-the-code-about-the-nes-sprite-capabilities
        // The gist: while each scanline is being prepared for display, the NES’ PPU looks
        // through the entries for the machine’s 64 hardware sprites in order, finds the first
        // eight that will display on the current line, and copies their attribute data to a
        // small area of internal RAM. There is only space there for eight sprites, so, the
        // NES cannot display more than eight sprites in a single scan line. Any later sprites
        // in the primary attribute data won’t have room to be copied, and so the PPU won’t be
        // able to display them.
        // sprite shift registers
        uint8_t spr_shift_low[8]{}, spr_shift_high[8]{};
        uint8_t spr_oam_index_render[8]{}; // render working set
        uint16_t spr_pattern_addr[8]{}; // pattern address for this scanline’s sprites

        // secondary OAM working set for the current scanline
        uint8_t secondary_oam[32] = {}; // 8 sprites * 4 bytes

        // need a way to index the OAM sprites, incremented each evaluation step, 0-63
        // "which primary OAM sprite is being tested against this scanline"
        int oam_scan_index = 0;
        bool overflow_ = false;


    }; // end class SpritePipeline

    class PPU
    {
    public:
        // All register fields from https://www.nesdev.org/wiki/PPU_registers
        // Useful knowledge https://austinmorlan.com/posts/nes_rendering_overview/
        // Sprite-specific https://setsideb.com/behind-the-code-about-the-nes-sprite-capabilities
        PPU();
        BackgroundPipeline bg;
        SpritePipeline sprites;
        Scanline timing;

        void reset();
        void step();

        // from https://www.nesdev.org/wiki/PPU_registers#Summary
        // CPU interface for $2000 - $2007
        uint8_t cpu_read_register(uint16_t address); // CPU reads $2000-$2007
        void cpu_write_register(uint16_t address, uint8_t value); // CPU writes $2000-$2007

        // PPU addressable space $0000 - $3FFF
        // TODO: determine if these wrappers are necessary. default to private bus read/write for now.
        // uint8_t ppu_read(uint16_t address);
        // void ppu_write(uint16_t address, uint8_t value);

        // CPU interface for $4014
        void oam_dma_execute(const uint8_t* page_data); // CPU/bus operation so PPU can get data

        // PPU cannot directly access memory outside of its own space
        // so we get handed callback methods by the mapper or cartridge
        // that the PPU can then use to access  pattern tables $0000-$1FFF
        // we want to pass a lambda but we can't convert to function pointers
        // so we make these take std::function which can take a lambda
        using ChrReadFn = std::function<uint8_t(uint16_t)>; // read takes (address)
        using ChrWriteFn = std::function<void(uint16_t, uint8_t)>; // write takes (address, value)
        void set_chr_read_callback(ChrReadFn callback) { chr_read_callback = std::move(callback); }
        void set_chr_write_callback(ChrWriteFn callback) {chr_write_callback = std::move(callback);}


        // helper to tell Scanline, BackgroundPipeline and SpritePipeline when one of the rendering flags is set
        bool rendering_enabled() const;

        // $2000 values
        uint8_t base_nametable_select = 0; // nametable x and y
        bool vram_address_increment_flag = false; //  (0: add 1, going across; 1: add 32, going down)
        bool sprite_pattern_table_address_flag = false; // $0000 when clear, $1000 when set
        bool background_pattern_table_address_flag = false; // $0000 when clear, $1000 when set
        bool sprite_size_flag = false; // 8x16 when set
        bool ppu_master_slave_flag = false; //(0: read backdrop from EXT pins; 1: output color on EXT pins)
        bool vblank_nmi_flag = false; // (0: off, 1: on) NMI enable on vblank

        // $2001 values
        bool greyscale_flag = false; // (0: normal color, 1: greyscale)
        bool leftmost_background_flag = false; // 1: Show background in leftmost 8 pixels of screen, 0: Hide
        bool leftmost_sprite_flag = false; // 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
        bool background_rendering_flag = false; // 1: Enable background rendering
        bool sprite_rendering_flag = false; // 1: Enable sprite rendering

        // Color emphasis causes a color tint effect that works by darkening the other two color components,
        // making the selected component comparatively brighter and thus emphasized.
        bool emphasize_red = false;
        bool emphasize_green = false;
        bool emphasize_blue = false;

        // $2002 values PPUSTATUS - Rendering events
        bool sprite_overflow_flag = false; // Set when more than 8 sprites appear on a scanline
        bool sprite_zero_hit_flag = false; // Set when sprite 0 overlaps a non-transparent background pixel
        bool vblank_flag = false; // Set at start of VBlank, cleared on read of $2002. Unreliabe, use NMI instead

        // From https://www.nesdev.org/wiki/PPU_memory_map#OAM
        // The PPU internally contains 256 bytes of memory known as Object Attribute Memory which determines how
        // sprites are rendered. The CPU can manipulate this memory through memory mapped registers at
        // OAMADDR ($2003), OAMDATA ($2004), and OAMDMA ($4014).
        // OAM can be viewed as an array with 64 entries.
        // Each entry has 4 bytes:
        // the sprite Y coordinate,
        // the sprite tile number,
        // the sprite attribute, and
        // the sprite X coordinate.
        //  Address Low Nibble  Description
        //  $0, $4, $8, $C 	    Sprite Y coordinate
        //  $1, $5, $9, $D 	    Sprite tile #
        //  $2, $6, $A, $E 	    Sprite attribute
        //  $3, $7, $B, $F 	    Sprite X coordinate

        // $2003 values OAMADDR - Sprite RAM address ($2003 write)
        uint8_t oam_address = 0; // address for OAM read/write

        // $2004 values  OAMDATA - Sprite RAM data  ($2004 read/write)
        // implemented via oam[oam_address] calls. comment left for completeness.


        // $2005 values PPUSCROLL - X and Y scroll ($2005 write)
        // from https://www.nesdev.org/wiki/PPU_registers#PPUSCROLL_-_X_and_Y_scroll_($2005_write)

        // ********
        // The PPU scroll registers share internal state with the PPU address registers. Because of this,
        // PPUSCROLL and the nametable bits in PPUCTRL should be written **__AFTER__*** any writes to PPUADDR.
        // ********

        // Internal registers (scroll/address latches)
        uint16_t v = 0; // Current VRAM address (15 bits). During rendering, used for the scroll position.

        // internal register t: Temporary VRAM address (15 bits)
        uint16_t t = 0;

        // fine-x position of the current scroll, used during rendering alongside v.
        uint8_t x = 0;

        // toggles on each write to either PPUSCROLL or PPUADDR, indicating whether this is the first or second
        // write. Clears on reads of PPUSTATUS. Sometimes called the 'write latch' or 'write toggle'.
        bool w = false;

        // $2006 values PPUADDR - VRAM address ($2006 write)
        // $2007 values PPUDATA - VRAM data ($2007 read/write)
        // Internal read buffer for buffered VRAM reads. pallette reads are unbuffered and
        // still update the buffere with the mirrored nametable byte.
        uint8_t data_buffer = 0;

        // $4014 values OAMDMA - Sprite DMA ($4014 write)
        uint8_t oam_dma_page = 0; // High byte of CPU address for OAM DMA ($XX00–$XXFF)

        // PPU internal memory
        uint8_t nametable_ram[2048] = {}; // CIRAM $2000-$2FFF (mirroring handled by mapper/bus)
        uint8_t palette_ram[32] = {}; // $3F00-$3F1F (with mirrors)
        uint8_t oam[256] = {}; // Primary OAM (64 sprites * 4 bytes)

        // https://www.nesdev.org/wiki/CHR_ROM_vs_CHR_RAM
        // Cartridge RAM $0000 - $1FFF
        uint8_t chr_ram[0x2000] = {};

        // Non-Maskable Interrupt management
        bool nmi_pending = false; // signal to CPU: take NMI
        bool nmi_prev = false; // edge detect previous (vblank_flag && vblank_nmi_flag)

        // framebuffer for output
        uint32_t framebuffer[256*240]{};
        bool frame_complete = false;
        const uint32_t* framebuffer_output() const;

        // are we doing vertical or horizontal mirroring
        bool vertical_mirroring = false;

        // last value on CPU-PPU line
        uint8_t open_bus = 0;

    private:
        // internal PPU bus routing with no CPU side effects
        uint8_t ppu_bus_read(uint16_t address);
        void ppu_bus_write(uint16_t address, uint8_t value);

        // $2007 helper methods
        uint8_t cpu_read_ppudata();
        void cpu_write_ppudata(uint8_t value); // cpu just provides 8-bit data value in $2007

        // tricky, so we only implement v increment logic once
        void increment_v();

        void increment_scroll_x();
        void increment_scroll_y();
        void transfer_address_x();
        void transfer_address_y();

        // Memory address mirroring helpers
        // https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
        uint16_t mirror_nametable_address(uint16_t address);

        // returns canonical address in $3F00-$3F1F after folding in address
        // and applying the sprint to background mirror quirk
        // from https://www.nesdev.org/wiki/PPU_palettes
        uint16_t mirror_palette_address(uint16_t address);

        // fields for CHR ROM/RAM callbacks (pattern tables $0000-$1FFF)
        ChrReadFn chr_read_callback;
        ChrWriteFn chr_write_callback;

        // want these to access PPU state without shipping a DTO around since we're all friends here.
        friend class BackgroundPipeline;
        friend class Scanline;
        friend class SpritePipeline;
    };
}
