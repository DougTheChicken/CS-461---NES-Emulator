#pragma once
#include <cstdint>

namespace nes
{
    class PPU;

    class Scanline
    {
    public:
        explicit Scanline(PPU& ppu) : ppu(ppu)
        {
        }
        // TODO: define Scanline public and private methods

        // from https://www.nesdev.org/wiki/PPU_rendering?utm_source=chatgpt.com#Line-by-line_timing
        // line-by-line timing
        bool odd_frame = false; // For odd frames, the cycle at the end of the scanline is skipped
        int16_t scanline = -1; // -1 prerender, 0-239 render, 240 post-render; 241-260 vblank
        uint16_t cycle = 0; // 0 - 340 (https://www.nesdev.org/w/images/default/thumb/4/4f/Ppu.svg/2560px-Ppu.svg.png)

    private:
        PPU& ppu;
    }; // end class Scanline

    class BackgroundPipeline
    {
    public:
        explicit BackgroundPipeline(PPU& ppu) : ppu(ppu)
        {
        }
        // TODO: define Background public and private methods

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
        // TODO: define SpritePipeline public and private methods

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
        uint8_t spr_x[8]{}, spr_attr[8]{};
        uint8_t spr_oam_index[8]{};
        int sprite_count = 0;

        // secondary OAM working set for the current scanline
        uint8_t secondary_oam[32] = {}; // 8 sprites * 4 bytes
        int secondary_count = 0;
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
        void oamdma_execute(uint8_t* page_data); // CPU/bus operation so PPU can get data

        // PPU cannot directly access memory outside of its own space
        // so we get handed callback methods by the mapper or cartridge
        // that the PPU can then use to access  pattern tables $0000-$1FFF
        void set_chr_read_callback(uint8_t (*callback)(uint16_t));
        void set_chr_write_callback(void (*callback)(uint16_t, uint8_t));



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

    private:
        // internal PPU bus routing with no CPU side effects
        uint8_t ppu_bus_read(uint16_t address);
        void ppu_bus_write(uint16_t address, uint8_t value);

        // on oamdma_execute, internal PPU write into oam[(oam_address + i) & 0xFF] and
        // increment oam_address
        void oamdma_copy_256(const uint8_t* page_data);

        // $2007 helper methods
        uint8_t cpu_read_ppudata();
        void cpu_write_ppudata(uint8_t value); // cpu just provides 8-bit data value in $2007

        // tricky, so we only implement v increment logic once
        void increment_v();

        // Memory address mirroring helpers
        // https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
        uint16_t mirror_nametable_address(uint16_t address);

        // returns canonical address in $3F00-$3F1F after folding in address
        // and applying the sprint to background mirror quirk
        // from https://www.nesdev.org/wiki/PPU_palettes
        uint16_t mirror_palette_address(uint16_t address);

        // fields for CHR ROM/RAM callbacks (pattern tables $0000-$1FFF)
        uint8_t (*chr_read_callback)(uint16_t) = nullptr;
        void (*chr_write_callback)(uint16_t, uint8_t) = nullptr;

        // want these to access PPU state without shipping a DTO around since we're all friends here.
        friend class BackgroundPipeline;
        friend class Scanline;
        friend class SpritePipeline;
    };
}
