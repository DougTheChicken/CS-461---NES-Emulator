#pragma once
#include <cstdint>
#include "mapper.hpp"
#include <vector>

namespace nes {

    class Mapper_001 : public Mapper {
    public:
        Mapper_001(uint16_t prgBanks, uint16_t chrBanks, bool is_chr_ram);
        ~Mapper_001() override = default;

        bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr, uint8_t &data) override;
        bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) override;

        bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) override;
        bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) override;

        uint8_t mirrorMode() override {
            return control_register & 0x03;
        }

    private:
        bool is_chr_ram = false;

        std::vector<uint8_t> prg_ram;

        // MMC1 Internal Registers
        uint8_t shift_register = 0x10; // Initialized to binary 10000
        uint8_t control_register = 0x1C;
        uint8_t chr_bank_0 = 0x00;
        uint8_t chr_bank_1 = 0x00;
        uint8_t prg_bank = 0x00;

        // Calculated Bank Offsets
        uint8_t prg_bank_16_lo = 0;
        uint8_t prg_bank_16_hi = 0;
        uint8_t chr_bank_4_lo = 0;
        uint8_t chr_bank_4_hi = 0;

        void update_offsets();
    };

} // namespace nes