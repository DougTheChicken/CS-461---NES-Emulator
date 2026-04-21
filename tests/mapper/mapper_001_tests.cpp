#include "nes/mapper/mapper_001.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Mapper001Test, MapsPrgRamAndFixedUpperBank) {
    nes::Mapper_001 mapper(2, 1, false);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_TRUE(mapper.cpuMapWrite(0x6000, mapped_addr, 0x5A));
    EXPECT_TRUE(mapper.cpuMapRead(0x6000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0xFFFFFFFFu);
    EXPECT_EQ(data, 0x5Au);

    EXPECT_TRUE(mapper.cpuMapRead(0xC000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0x4000u);
}

TEST(Mapper001Test, AllowsChrRamWritesWhenConfigured) {
    nes::Mapper_001 mapper(2, 1, true);
    uint32_t mapped_addr = 0;

    EXPECT_TRUE(mapper.ppuMapWrite(0x0000, mapped_addr));
    EXPECT_EQ(mapped_addr, 0x0000u);
    EXPECT_TRUE(mapper.ppuMapWrite(0x1234, mapped_addr));
    EXPECT_EQ(mapped_addr, 0x0234u);
}

} // namespace
