#include "nes/mapper/mapper_003.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Mapper003Test, SelectsChrBankOnCpuWrite) {
    nes::Mapper_003 mapper(2, 4);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_FALSE(mapper.cpuMapWrite(0x8000, mapped_addr, 0x03));
    EXPECT_TRUE(mapper.ppuMapRead(0x0001, mapped_addr));
    EXPECT_EQ(mapped_addr, 0x6001u);
}

} // namespace
