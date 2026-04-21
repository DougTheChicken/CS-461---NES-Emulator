#include "nes/console.hpp"

#include <gtest/gtest.h>

#include <string>

#include "support/test_utils.hpp"

namespace {

TEST(ConsoleTest, LoadsNestestRomAndStepsInstructions) {
    console con;

    const std::string rom_path = test_support::find_nestest_rom();
    ASSERT_FALSE(rom_path.empty()) << "Could not locate nestest.nes";
    ASSERT_TRUE(con.load_rom(rom_path.c_str()));
    EXPECT_TRUE(con.rom_loaded());

    const auto before = con.get_cpu_cycles();
    const int used_cycles = con.step_instruction();
    EXPECT_GT(used_cycles, 0);
    EXPECT_GT(con.get_cpu_cycles(), before);
}

TEST(ConsoleTest, ResetAllClearsLoadedRomAndCycleCounter) {
    console con;

    const std::string rom_path = test_support::find_nestest_rom();
    ASSERT_FALSE(rom_path.empty()) << "Could not locate nestest.nes";
    ASSERT_TRUE(con.load_rom(rom_path.c_str()));

    con.reset_all();

    EXPECT_FALSE(con.rom_loaded());
    EXPECT_EQ(con.get_cpu_cycles(), 0u);
    EXPECT_EQ(con.get_mem().get_controller1(), 0u);
    EXPECT_EQ(con.get_mem().get_controller2(), 0u);
}

} // namespace