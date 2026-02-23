#include "nes/ROM.hpp"
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

static std::string find_nestest_rom() {
    namespace fs = std::filesystem;

    // - repo root:          ./tests/test_roms/...
    // - build dir:          ../tests/test_roms/...
    // - build/tests dir:    ../../tests/test_roms/...
    const std::vector<std::string> candidates = {
        "./tests/test_roms/nestest/nestest.nes",
        "../tests/test_roms/nestest/nestest.nes",
        "../../tests/test_roms/nestest/nestest.nes",
    };

    for (const auto& p : candidates) {
        if (fs::exists(p)) return p;
    }
    return "";
}

TEST(LOAD, load_from_file_success) {
    const std::string path = find_nestest_rom();
    ASSERT_FALSE(path.empty()) << "Could not locate nestest ROM from current working directory.";

    nes::ROM rom;
    bool result = rom.load_from_file(path.c_str());
    EXPECT_EQ(result, true);
}

TEST(LOAD, prg_size_bytes) {
    const std::string path = find_nestest_rom();
    ASSERT_FALSE(path.empty()) << "Could not locate nestest ROM from current working directory.";

    nes::ROM rom;
    rom.load_from_file(path.c_str());
    EXPECT_EQ(rom.prg_size_bytes(), 16384u);
}

TEST(LOAD, chr_size_bytes) {
    const std::string path = find_nestest_rom();
    ASSERT_FALSE(path.empty()) << "Could not locate nestest ROM from current working directory.";

    nes::ROM rom;
    rom.load_from_file(path.c_str());
    EXPECT_EQ(rom.chr_size_bytes(), 8192u);
}

TEST(LOAD, alternate_nametable_layout) {
    const std::string path = find_nestest_rom();
    ASSERT_FALSE(path.empty()) << "Could not locate nestest ROM from current working directory.";

    nes::ROM rom;
    rom.load_from_file(path.c_str());
    EXPECT_EQ(rom.alternate_nametable_layout(), false);
}

TEST(LOAD, mapper) {
    const std::string path = find_nestest_rom();
    ASSERT_FALSE(path.empty()) << "Could not locate nestest ROM from current working directory.";

    nes::ROM rom;
    rom.load_from_file(path.c_str());
    EXPECT_EQ(rom.mapper(), 0u);
}

TEST(LOAD, is_loaded) {
    const std::string path = find_nestest_rom();
    ASSERT_FALSE(path.empty()) << "Could not locate nestest ROM from current working directory.";

    nes::ROM rom;
    rom.load_from_file(path.c_str());
    EXPECT_EQ(rom.is_loaded(), true);
}

TEST(RESET, reset_clears_loaded) {
    nes::ROM rom;
    rom.reset();
    EXPECT_EQ(rom.is_loaded(), false);
}

TEST(RESET, reset_clears_PNG) {
    nes::ROM rom;
    rom.reset();
    EXPECT_EQ(rom.prg_size_bytes(), 0u);
}

TEST(RESET, reset_clears_CHR) {
    nes::ROM rom;
    rom.reset();
    EXPECT_EQ(rom.chr_size_bytes(), 0u);
}

TEST(RESET, reset_clears_mapper) {
    nes::ROM rom;
    rom.reset();
    EXPECT_EQ(rom.mapper(), 0u);
}

TEST(RESET, reset_clears_nametable_layout) {
    nes::ROM rom;
    rom.reset();
    EXPECT_EQ(rom.alternate_nametable_layout(), false);
}

TEST(RESET, reset_clears_data) {
    nes::ROM rom;
    rom.reset();
    EXPECT_EQ(rom.prg_data(), nullptr);
}