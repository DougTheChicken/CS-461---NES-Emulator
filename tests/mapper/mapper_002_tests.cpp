#include "nes/mapper/mapper_002.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Mapper002Test, SwitchesLowPrgBankOnCpuWrite) {
    nes::Mapper_002 mapper(4, 1);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_TRUE(mapper.cpuMapRead(0x8000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0x0000u);

    EXPECT_FALSE(mapper.cpuMapWrite(0x8000, mapped_addr, 0x02));
    EXPECT_TRUE(mapper.cpuMapRead(0x8000, mapped_addr, data));
    EXPECT_EQ(mapped_addr, 0x8000u);
}

} // namespace
