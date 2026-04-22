#include "nes/mapper/mapper_000.hpp"
#include "nes/mapper/mapper_001.hpp"
#include "nes/mapper/mapper_002.hpp"
#include "nes/mapper/mapper_003.hpp"
#include "nes/mapper/mapper_004.hpp"
#include "nes/mapper/mapper_005.hpp"
#include "nes/mapper/mapper_007.hpp"

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

TEST(Mapper003Test, SelectsChrBankOnCpuWrite) {
    nes::Mapper_003 mapper(2, 4);
    uint32_t mapped_addr = 0;
    uint8_t data = 0;

    EXPECT_FALSE(mapper.cpuMapWrite(0x8000, mapped_addr, 0x03));
    EXPECT_TRUE(mapper.ppuMapRead(0x0001, mapped_addr));
    EXPECT_EQ(mapped_addr, 0x6001u);
}

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