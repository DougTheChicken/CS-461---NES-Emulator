#include "nes/mapper/mapper_000.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Mapper000Test, MirrorsPrgRomAcrossSingleBank) {
    nes::Mapper_000 mapper(1, 1);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_TRUE(mapper.cpuMapRead(0x8000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0x0000u);

    EXPECT_TRUE(mapper.cpuMapRead(0xC000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0x0000u);
}

TEST(Mapper000Test, SupportsChrRamWritesWhenEnabled) {
    nes::Mapper_000 mapper(1, 1, true);
    uint32_t mapped_addr = 0;

    EXPECT_TRUE(mapper.ppuMapWrite(0x0010, mapped_addr));
    EXPECT_EQ(mapped_addr, 0x0010u);
    EXPECT_TRUE(mapper.ppuMapWrite(0x1234, mapped_addr));
    EXPECT_EQ(mapped_addr, 0x1234u);

    EXPECT_FALSE(mapper.cpuMapWrite(0x8000, mapped_addr, 0xAB));
}

} // namespace
