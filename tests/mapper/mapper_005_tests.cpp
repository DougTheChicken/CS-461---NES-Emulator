#include "nes/mapper/mapper_005.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Mapper005Test, ExRamAndMultiplierWork) {
    nes::Mapper_005 mapper(8, 8);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_FALSE(mapper.cpuMapWrite(0x5C10, mapped_addr, 0x77));
    EXPECT_FALSE(mapper.cpuMapWrite(0x5104, mapped_addr, 0x02));
    EXPECT_FALSE(mapper.cpuMapRead(0x5C10, mapped_addr, data));
    EXPECT_EQ(data, 0x77u);

    EXPECT_FALSE(mapper.cpuMapWrite(0x5205, mapped_addr, 12));
    EXPECT_FALSE(mapper.cpuMapWrite(0x5206, mapped_addr, 34));
    EXPECT_FALSE(mapper.cpuMapRead(0x5205, mapped_addr, data));
    EXPECT_EQ(data, 0x98u);
    EXPECT_FALSE(mapper.cpuMapRead(0x5206, mapped_addr, data));
    EXPECT_EQ(data, 0x01u);

    EXPECT_FALSE(mapper.cpuMapWrite(0x5105, mapped_addr, 0xAA));
    EXPECT_EQ(mapper.mirrorMode(), 0xAAu);
}

} // namespace
