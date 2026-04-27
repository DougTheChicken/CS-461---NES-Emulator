#include "nes/cpu.hpp"
#include "nes/mem.hpp"
#include "nes/ppu.hpp"
#include "nes/apu.hpp"
#include "nes/ROM.hpp"
#include "nes/mapper/mapper_000.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>

namespace {

class CpuFixture : public ::testing::Test {
protected:
    CpuFixture()
        : ppu(), apu(), mem(ppu, apu), cpu() {
        cpu.attach_memory(&mem);
    }

    void load_program(const std::array<uint8_t, 16384>& prg) {
        cartridge.load_test_data(
            std::vector<uint8_t>(prg.begin(), prg.end()),
            std::make_shared<nes::Mapper_000>(1, 1)
        );
        mem.insert_cartridge(&cartridge);
        cpu.reset();
    }

    nes::PPU ppu;
    nes::APU apu;
    nes::Memory mem;
    nes::CPU cpu;
    nes::ROM cartridge;
};

TEST_F(CpuFixture, LookupInstructionReportsKnownMnemonic) {
    EXPECT_EQ(cpu.lookupInstruction(0x01), "ORA");
    EXPECT_EQ(cpu.lookupInstruction(0x02), "???");
}

TEST_F(CpuFixture, ExecutesSimpleProgramAndStoresRegisters) {
    std::array<uint8_t, 16384> prg{};
    prg[0x0000] = 0xA9; // LDA #$42
    prg[0x0001] = 0x42;
    prg[0x0002] = 0xAA; // TAX
    prg[0x0003] = 0xE8; // INX
    prg[0x3FFC] = 0x00;
    prg[0x3FFD] = 0x80;

    load_program(prg);

    EXPECT_EQ(cpu.pc(), 0x8000);
    EXPECT_GT(cpu.step(), 0);
    EXPECT_EQ(cpu.a(), 0x42);
    EXPECT_TRUE((cpu.p() & 0x02) == 0);

    EXPECT_GT(cpu.step(), 0);
    EXPECT_EQ(cpu.x(), 0x42);

    EXPECT_GT(cpu.step(), 0);
    EXPECT_EQ(cpu.x(), 0x43);
}

TEST_F(CpuFixture, StoresThroughMemoryBus) {
    std::array<uint8_t, 16384> prg{};
    prg[0x0000] = 0xA9; // LDA #$7E
    prg[0x0001] = 0x7E;
    prg[0x0002] = 0x8D; // STA $0002
    prg[0x0003] = 0x02;
    prg[0x0004] = 0x00;
    prg[0x0005] = 0x00; // BRK-like stop byte, never executed in this test
    prg[0x3FFC] = 0x00;
    prg[0x3FFD] = 0x80;

    load_program(prg);

    EXPECT_GT(cpu.step(), 0);
    EXPECT_GT(cpu.step(), 0);
    EXPECT_EQ(mem.read(0x0002), 0x7E);
}

} // namespace