#include "nes/ROM.hpp"
#include <gtest/gtest.h>
#include <string>
#include "support/test_utils.hpp"

namespace {

class RomFixture : public ::testing::Test {
protected:
    void SetUp() override {
        rom_path = test_support::find_nestest_rom();
        ASSERT_FALSE(rom_path.empty()) << "Could not locate nestest ROM from current working directory.";
    }

    std::string rom_path;
};

TEST_F(RomFixture, LoadFromFileSuccess) {
    nes::ROM rom;
    EXPECT_TRUE(rom.load_from_file(rom_path.c_str()));
}

TEST_F(RomFixture, ReportsRomMetadata) {
    nes::ROM rom;
    ASSERT_TRUE(rom.load_from_file(rom_path.c_str()));

    EXPECT_EQ(rom.prg_size_bytes(), 16384u);
    EXPECT_EQ(rom.chr_size_bytes(), 8192u);
    EXPECT_FALSE(rom.alternate_nametable_layout());
    EXPECT_EQ(rom.mapper(), 0u);
    EXPECT_TRUE(rom.is_loaded());
}

TEST(ROMResetTest, ClearsLoadedStateAndData) {
    nes::ROM rom;
    rom.reset();

    EXPECT_FALSE(rom.is_loaded());
    EXPECT_EQ(rom.prg_size_bytes(), 0u);
    EXPECT_EQ(rom.chr_size_bytes(), 0u);
    EXPECT_EQ(rom.mapper(), 0u);
    EXPECT_FALSE(rom.alternate_nametable_layout());
    EXPECT_EQ(rom.prg_data(), nullptr);
}

TEST(ROMLoadTest, MissingFileFailsCleanly) {
    nes::ROM rom;
    EXPECT_FALSE(rom.load_from_file("/this/path/should/not/exist.nes"));
}

} // namespace