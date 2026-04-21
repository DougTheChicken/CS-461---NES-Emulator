#include "ui/audio.hpp"
#include "nes/console.hpp"

#include <gtest/gtest.h>

namespace {

TEST(AudioStreamTest, InitializesWithDefaultSampleRate) {
    console emu;
    ui::AudioStream stream(emu);

    EXPECT_TRUE(stream.initialize());
}

TEST(AudioStreamTest, InitializesWithCustomSampleRate) {
    console emu;
    ui::AudioStream stream(emu);

    EXPECT_TRUE(stream.initialize(22050));
}

} // namespace
