#include "nes/ROM.hpp"
#include <gtest/gtest.h>

const char* TEST_ROM_PATH = "./tests/test_roms/nestest/nestest.nes";

TEST(LOAD, load_from_file_success) {
    nes::ROM rom;
    bool result = rom.load_from_file(TEST_ROM_PATH);
    EXPECT_EQ(result, true);
}

TEST(LOAD, prg_size_bytes) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    EXPECT_EQ(rom.prg_size_bytes(), 16384);
}

TEST(LOAD, chr_size_bytes) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    EXPECT_EQ(rom.chr_size_bytes(), 8192);
}

TEST(LOAD, alternate_nametable_layout) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    EXPECT_EQ(rom.alternate_nametable_layout(), false);
}

TEST(LOAD, mapper) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    EXPECT_EQ(rom.mapper(), 0);
}

TEST(LOAD, is_loaded) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    EXPECT_EQ(rom.is_loaded(), true);
}

TEST(RESET, reset_clears_loaded) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    rom.reset();
    EXPECT_EQ(rom.is_loaded(), false);
}

TEST(RESET, reset_clears_PNG) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    rom.reset();
    EXPECT_EQ(rom.prg_size_bytes(), 0);
}

TEST(RESET, reset_clears_CHR) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    rom.reset();
    EXPECT_EQ(rom.chr_size_bytes(), 0);
}

TEST(RESET, reset_clears_mapper) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    rom.reset();
    EXPECT_EQ(rom.mapper(), 0);
}

TEST(RESET, reset_clears_nametable_layout) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    rom.reset();
    EXPECT_EQ(rom.alternate_nametable_layout(), false);
}

TEST(RESET, reset_clears_data) {
    nes::ROM rom;
    rom.load_from_file(TEST_ROM_PATH);
    rom.reset();
    EXPECT_EQ(rom.prg_size_bytes(), 0);
    EXPECT_EQ(rom.chr_size_bytes(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}