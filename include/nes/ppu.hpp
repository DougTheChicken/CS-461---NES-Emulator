#pragma once
#include <cstdint>
namespace nes {
    class PPU {
    public:
        void reset() {}
        void step() {}

        // $2000 values
        uint8_t base_nametable_select = 0; // nametable x and y
        bool vram_address_increment_flag = false; //  (0: add 1, going across; 1: add 32, going down)
        bool sprite_pattern_table_address_flag = false; // $0000 when clear, $1000 when set
        bool background_pattern_table_address_flag = false; // $0000 when clear, $1000 when set
        bool sprite_size_flag = false;  // 8x16 when set
        bool ppu_master_slave_flag = false; //(0: read backdrop from EXT pins; 1: output color on EXT pins)
        bool vblank_nmi_flag = false; // (0: off, 1: on)

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
        uint8_t oam_address = 0;  // address for OAM read/write

        // $2004 values  OAMDATA - Sprite RAM data  ($2004 read/write)
        uint8_t oam_data = 0; // data port for OAM buffer read/write

        // $2005 values PPUSCROLL - X and Y scroll ($2005 write)
        // from https://www.nesdev.org/wiki/PPU_registers#PPUSCROLL_-_X_and_Y_scroll_($2005_write)
        // This register is used to change the scroll position, telling the PPU which pixel of the nametable
        // selected through PPUCTRL should be at the top left corner of the rendered screen. PPUSCROLL takes
        // two writes: the first is the X scroll and the second is the Y scroll. Whether this is the first or
        // second write is tracked internally by the w register, which is shared with PPUADDR. Typically, this
        // register is written to during vertical blanking to make the next frame start rendering from the
        // desired location, but it can also be modified during rendering in order to split the screen.
        // Changes made to the vertical scroll during rendering will only take effect on the next frame.
        // Together with the nametable bits in PPUCTRL, the scroll can be thought of as 9 bits per component,
        // and PPUCTRL must be updated along with PPUSCROLL to fully specify the scroll position.

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
        uint8_t data_buffer = 0; // Internal read buffer for buffered VRAM reads. pallette reads are unbuffered and
        // still update the buffere with the mirrored nametable byte.

        // $4014 values OAMDMA - Sprite DMA ($4014 write)
        uint8_t oam_dma_page = 0; // High byte of CPU address for OAM DMA ($XX00–$XXFF)

        // PPU internal memory
        uint8_t nametable_ram[2048] = {}; // CIRAM $2000-$2FFF (mirroring handled by mapper/bus)
        uint8_t palette_ram[32] = {}; // $3F00-$3F1F (with mirrors)
        uint8_t oam[256] = {}; // Primary OAM (64 sprites * 4 bytes)

        // from https://www.nesdev.org/wiki/PPU_rendering?utm_source=chatgpt.com#Line-by-line_timing
        // line-by-line timing
        bool odd_frame = false; // For odd frames, the cycle at the end of the scanline is skipped
        uint16_t scanline = -1; // -1 prerender, 0-239 render, 240 post-render; 241-260 vblank
        uint16_t cycle = 0; // 0 - 340 (https://www.nesdev.org/w/images/default/thumb/4/4f/Ppu.svg/2560px-Ppu.svg.png)

    private:

    };
}
