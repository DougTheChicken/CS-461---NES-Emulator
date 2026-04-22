#include "nes/apu.hpp"
#include "nes/mem.hpp"
#include "nes/ppu.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>

#include "nes/mapper/mapper_000.hpp"

namespace {

class BusFixture : public ::testing::Test {
protected:
    BusFixture()
        : ppu(), apu(), mem(ppu, apu) {}

    nes::PPU ppu;
    nes::APU apu;
    nes::Memory mem;
};

TEST_F(BusFixture, RamMirrorsAcrossInternalSpace) {
    mem.write(0x0000, 0x42);
    EXPECT_EQ(mem.read(0x0000), 0x42);

    mem.write(0x0000, 0xAB);
    EXPECT_EQ(mem.read(0x0800), 0xAB);
    EXPECT_EQ(mem.read(0x1000), 0xAB);
    EXPECT_EQ(mem.read(0x1800), 0xAB);

    mem.write(0x1FFF, 0xCD);
    EXPECT_EQ(mem.read(0x07FF), 0xCD);
}

TEST_F(BusFixture, PpuRegisterMirroringReadsStatus) {
    EXPECT_EQ(mem.read(0x2002) & 0x80, 0x80);

    nes::PPU mirrored_ppu;
    nes::APU mirrored_apu;
    nes::Memory mirrored_mem(mirrored_ppu, mirrored_apu);
    EXPECT_EQ(mirrored_mem.read(0x200A) & 0x80, 0x80);
}

TEST_F(BusFixture, ControllerStrobeAndSerialReadWork) {
    mem.set_controller1(0x09);
    mem.set_controller2(0x09);

    mem.write(0x4016, 1);
    mem.write(0x4016, 0);

    EXPECT_EQ(mem.read(0x4016), 1);
    EXPECT_EQ(mem.read(0x4016), 0);
    EXPECT_EQ(mem.read(0x4016), 0);
    EXPECT_EQ(mem.read(0x4016), 1);
    EXPECT_EQ(mem.read(0x4016), 0);
    EXPECT_EQ(mem.read(0x4016), 0);
    EXPECT_EQ(mem.read(0x4016), 0);
    EXPECT_EQ(mem.read(0x4016), 0);

    EXPECT_EQ(mem.read(0x4017), 1);
    EXPECT_EQ(mem.read(0x4017), 0);
    EXPECT_EQ(mem.read(0x4017), 0);
    EXPECT_EQ(mem.read(0x4017), 1);
    EXPECT_EQ(mem.read(0x4017), 0);
    EXPECT_EQ(mem.read(0x4017), 0);
    EXPECT_EQ(mem.read(0x4017), 0);
    EXPECT_EQ(mem.read(0x4017), 0);
}

TEST_F(BusFixture, CartridgePrgMappingReadsThroughMapper) {
    std::array<uint8_t, 16384> prg{};
    prg[0x0000] = 0x11;
    prg[0x3FFC] = 0x34;
    prg[0x3FFD] = 0x12;

    mem.set_mapper(std::make_shared<nes::Mapper_000>(1, 1));
    mem.map_prg(prg.data(), prg.size());

    EXPECT_EQ(mem.read(0x8000), 0x11);
    EXPECT_EQ(mem.read(0xFFFC), 0x34);
    EXPECT_EQ(mem.read(0xFFFD), 0x12);
}

} // namespace
