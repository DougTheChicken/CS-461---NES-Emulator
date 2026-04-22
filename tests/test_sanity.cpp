#include "nes/console.hpp"

#include <gtest/gtest.h>

#include <string>

#include "support/test_utils.hpp"

TEST(IntegrationSmokeTest, ConsoleAdvancesCpuCycles) {
    console con;

    const std::string rom_path = test_support::find_nestest_rom();
    ASSERT_FALSE(rom_path.empty()) << "Could not locate nestest.nes";
    ASSERT_TRUE(con.load_rom(rom_path.c_str()));

    const unsigned long long before = con.get_cpu_cycles();
    const int used = con.step_instruction();
    const unsigned long long after = con.get_cpu_cycles();

    EXPECT_GT(after, before);
    EXPECT_GT(used, 0);
}