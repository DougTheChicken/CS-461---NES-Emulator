#pragma once
#include "mapper.hpp"

namespace nes {

    class Mapper_007 : public Mapper {

        public:
            // constructor
            Mapper_007(uint16_t prgBanks, uint16_t charBanks);
            // deconstructor
            // override acting as safety net if mapper deconstructor is no longer virtual for some reason
            ~Mapper_007() override = default;

            // interface overrides
            // cpu mapper read and write
            bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
            bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;
            // ppu mapper read and write
            bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
            bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
            
            // mapper to cpu interrupt interface
            uint8_t mirrorMode() override;      // refractor/cleanup may move this

        private:
            uint8_t nSelectedBank = 0;
            uint8_t nMirrorMode = 0;            // refractor/cleanup may move this

    };

} // namespace nes