#pragma once
#include "mapper.hpp"

namespace nes {

    class Mapper_002 : public Mapper {

        public:
            // constructor
            Mapper_002(uint16_t prgBanks, uint16_t charBanks);
            // deconstructor
            // override acting as safety net if mapper deconstructor is no longer virtual for some reason
            ~Mapper_002() override = default;

            // interface overrides
            // cpu mapper read and write
            bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
            bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;
            // ppu mapper read and write
            bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
            bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;

        private:
            uint8_t nSelectedPRGBankLow = 0x00;
            uint8_t nSelectedPRGBankHigh = 0x00;

    };

} // namespace nes