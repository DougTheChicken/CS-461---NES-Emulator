#include "nes/mapper/mapper_004.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Mapper004Test, UpdatesPrgBanksAndIrqState) {
    nes::Mapper_004 mapper(4, 2);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_TRUE(mapper.cpuMapRead(0xC000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0xC000u);

    EXPECT_FALSE(mapper.cpuMapWrite(0x8000, mapped_addr, 0x06));
    EXPECT_FALSE(mapper.cpuMapWrite(0x8001, mapped_addr, 0x03));
    EXPECT_TRUE(mapper.cpuMapRead(0x8000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0x6000u);

    EXPECT_FALSE(mapper.cpuMapWrite(0xA000, mapped_addr, 0x01));
    EXPECT_EQ(mapper.mirrorMode(), 1u);

    EXPECT_FALSE(mapper.cpuMapWrite(0xC000, mapped_addr, 0x01));
    EXPECT_FALSE(mapper.cpuMapWrite(0xE001, mapped_addr, 0x00));
    mapper.scanline();
    EXPECT_FALSE(mapper.irqActive());
    mapper.scanline();
    EXPECT_TRUE(mapper.irqActive());
    mapper.irqClear();
    EXPECT_FALSE(mapper.irqActive());
}

} // namespace
