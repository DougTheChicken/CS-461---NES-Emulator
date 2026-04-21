#include "nes/mapper/mapper_007.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Mapper007Test, SelectsBankAndMirrorMode) {
    nes::Mapper_007 mapper(8, 1);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_FALSE(mapper.cpuMapWrite(0x8000, mapped_addr, 0x13));
    EXPECT_TRUE(mapper.cpuMapRead(0x8000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0x18000u);
    EXPECT_EQ(mapper.mirrorMode(), 1u);

    EXPECT_TRUE(mapper.ppuMapRead(0x0123, mapped_addr));
    EXPECT_EQ(mapped_addr, 0x0123u);
}

} // namespace
