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

} // namespace
