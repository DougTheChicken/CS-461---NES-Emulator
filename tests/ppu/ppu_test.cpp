#include "nes/apu.hpp"
#include "nes/ppu.hpp"

#include <gtest/gtest.h>

namespace {

TEST(PPUTest, ResetStateIsClean) {
    nes::PPU ppu;
    ppu.reset();

    EXPECT_EQ(ppu.oam_address, 0);
    EXPECT_EQ(ppu.base_nametable_select, 0);
    EXPECT_FALSE(ppu.vram_address_increment_flag);
    EXPECT_FALSE(ppu.sprite_pattern_table_address_flag);
    EXPECT_FALSE(ppu.background_pattern_table_address_flag);
    EXPECT_FALSE(ppu.sprite_size_flag);
    EXPECT_FALSE(ppu.ppu_master_slave_flag);
    EXPECT_FALSE(ppu.vblank_nmi_flag);
    EXPECT_FALSE(ppu.greyscale_flag);
    EXPECT_FALSE(ppu.leftmost_background_flag);
    EXPECT_FALSE(ppu.leftmost_sprite_flag);
    EXPECT_FALSE(ppu.background_rendering_flag);
    EXPECT_FALSE(ppu.sprite_rendering_flag);
    EXPECT_FALSE(ppu.emphasize_red);
    EXPECT_FALSE(ppu.emphasize_green);
    EXPECT_FALSE(ppu.emphasize_blue);
    EXPECT_FALSE(ppu.sprite_overflow_flag);
    EXPECT_FALSE(ppu.sprite_zero_hit_flag);
    EXPECT_TRUE(ppu.vblank_flag);
    EXPECT_EQ(ppu.v, 0);
    EXPECT_EQ(ppu.t, 0);
    EXPECT_EQ(ppu.x, 0);
    EXPECT_FALSE(ppu.w);
    EXPECT_EQ(ppu.data_buffer, 0);
    EXPECT_EQ(ppu.oam_dma_page, 0);
    EXPECT_FALSE(ppu.nmi_pending);
    EXPECT_FALSE(ppu.nmi_prev);
    EXPECT_FALSE(ppu.frame_complete);
    EXPECT_FALSE(ppu.vertical_mirroring);
    EXPECT_EQ(ppu.open_bus, 0);
    EXPECT_FALSE(ppu.timing.odd_frame());
    EXPECT_EQ(ppu.timing.scanline(), -1);
    EXPECT_EQ(ppu.timing.cycle(), 0);
    EXPECT_FALSE(ppu.sprites.overflow());
}

TEST(PPUTest, HorizontalFlipHelperReversesBits) {
    EXPECT_EQ(nes::SpritePipeline::maybe_flipped_h(0x00, 0b10110001), 0b10110001);
    EXPECT_EQ(nes::SpritePipeline::maybe_flipped_h(0x40, 0b10110001), static_cast<uint8_t>(0b10001101));
    EXPECT_EQ(nes::SpritePipeline::maybe_flipped_h(0x40, 0x00), 0x00);
    EXPECT_EQ(nes::SpritePipeline::maybe_flipped_h(0x40, 0xFF), 0xFF);
}

TEST(PPUTest, VBlankFlagAppearsDuringStepAndClearsOnStatusRead) {
    nes::PPU ppu;

    bool found_vblank = false;
    for (int i = 0; i < 100000 && !found_vblank; ++i) {
        ppu.step();
        found_vblank = ppu.vblank_flag;
    }

    EXPECT_TRUE(found_vblank);
    EXPECT_TRUE(ppu.vblank_flag);

    ppu.cpu_read_register(0x2002);
    EXPECT_FALSE(ppu.vblank_flag);
}

// Blargg palette_ram test coverage:
// 1) Tests passed
// 2) Palette read shouldn't be buffered like other VRAM
// 3) Palette write/read doesn't work
// 4) Palette should be mirrored within $3f00-$3fff
// 5) Write to $10 should be mirrored at $00
// 6) Write to $00 should be mirrored at $10

static void set_ppuaddr(nes::PPU& ppu, uint16_t addr) {
    ppu.cpu_read_register(0x2002); // clear write latch (w = false)
    ppu.cpu_write_register(0x2006, (addr >> 8) & 0x3F);
    ppu.cpu_write_register(0x2006, addr & 0xFF);
}

TEST(PPUPaletteRamTest, BasicWriteRead) {
    nes::PPU ppu;
    set_ppuaddr(ppu, 0x3F01);
    ppu.cpu_write_register(0x2007, 0x15);
    set_ppuaddr(ppu, 0x3F01);
    EXPECT_EQ(ppu.cpu_read_register(0x2007), 0x15);
}

TEST(PPUPaletteRamTest, ReadIsImmediateNotBuffered) {
    nes::PPU ppu;
    set_ppuaddr(ppu, 0x3F05);
    ppu.cpu_write_register(0x2007, 0x27);
    // First read from palette address must return the palette value immediately,
    // not the stale buffer from a prior non-palette read.
    set_ppuaddr(ppu, 0x3F05);
    EXPECT_EQ(ppu.cpu_read_register(0x2007), 0x27);
}

TEST(PPUPaletteRamTest, MirroredIn3F00To3FFF) {
    nes::PPU ppu;
    // Write to $3F01 and verify it is readable at $3F21 (and $3F41, etc.)
    set_ppuaddr(ppu, 0x3F01);
    ppu.cpu_write_register(0x2007, 0x25);
    set_ppuaddr(ppu, 0x3F21);
    EXPECT_EQ(ppu.cpu_read_register(0x2007), 0x25);
}

TEST(PPUPaletteRamTest, WriteToX10MirroredAtX00) {
    // $3F10 (sprite bg colour) is a mirror of $3F00 (background colour 0)
    nes::PPU ppu;
    set_ppuaddr(ppu, 0x3F10);
    ppu.cpu_write_register(0x2007, 0x15);
    set_ppuaddr(ppu, 0x3F00);
    EXPECT_EQ(ppu.cpu_read_register(0x2007), 0x15);
}

TEST(PPUPaletteRamTest, WriteToX00MirroredAtX10) {
    // $3F00 (background colour 0) is a mirror of $3F10 (sprite bg colour)
    nes::PPU ppu;
    set_ppuaddr(ppu, 0x3F00);
    ppu.cpu_write_register(0x2007, 0x20);
    set_ppuaddr(ppu, 0x3F10);
    EXPECT_EQ(ppu.cpu_read_register(0x2007), 0x20);
}

TEST(PPUPaletteRamTest, AllFourTransparentColoursMirror) {
    // $3F14/$3F18/$3F1C also mirror $3F04/$3F08/$3F0C
    nes::PPU ppu;

    struct { uint16_t sprite_addr; uint16_t bg_addr; uint8_t val; } cases[] = {
        { 0x3F14, 0x3F04, 0x11 },
        { 0x3F18, 0x3F08, 0x22 },
        { 0x3F1C, 0x3F0C, 0x33 },
    };

    for (auto& c : cases) {
        set_ppuaddr(ppu, c.sprite_addr);
        ppu.cpu_write_register(0x2007, c.val);
        set_ppuaddr(ppu, c.bg_addr);
        EXPECT_EQ(ppu.cpu_read_register(0x2007), c.val);

        set_ppuaddr(ppu, c.bg_addr);
        ppu.cpu_write_register(0x2007, c.val ^ 0x0F);
        set_ppuaddr(ppu, c.sprite_addr);
        EXPECT_EQ(ppu.cpu_read_register(0x2007), c.val ^ 0x0F);
    }
}

} // namespace
