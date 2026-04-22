#include "nes/timing.hpp"
#include "nes/ppu.hpp"

#include <gtest/gtest.h>

namespace {

TEST(TimingTest, ConstantsMatchNtscTiming) {
    EXPECT_EQ(CPU_TO_PPU, 3);
    EXPECT_EQ(PPU_CYCLES_PER_FRAME, 89342);
}

TEST(ScanlineTest, StartsAtPrerenderLine) {
    nes::Scanline scanline;

    EXPECT_TRUE(scanline.is_prerender_scanline());
    EXPECT_TRUE(scanline.is_frame_start());
    EXPECT_EQ(scanline.scanline(), -1);
    EXPECT_EQ(scanline.cycle(), 0u);
}

TEST(ScanlineTest, ReachesVBlankStartAndNextFrame) {
    nes::Scanline scanline;

    bool saw_vblank = false;
    for (int i = 0; i < 100000 && !saw_vblank; ++i) {
        scanline.tick(true);
        saw_vblank = scanline.is_vblank_start();
    }

    EXPECT_TRUE(saw_vblank);
    EXPECT_TRUE(scanline.is_vblank_start());

    bool saw_new_frame = false;
    for (int i = 0; i < 100000 && !saw_new_frame; ++i) {
        scanline.tick(true);
        saw_new_frame = scanline.frame_count() > 0;
    }

    EXPECT_TRUE(saw_new_frame);
    EXPECT_GE(scanline.frame_count(), 1u);
}

} // namespace