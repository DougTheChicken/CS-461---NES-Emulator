#pragma once
#include "mapper.hpp"

namespace nes {

    class Mapper_004 : public Mapper {

        public:
            // constructor
            Mapper_004(uint16_t prgBanks, uint16_t charBanks);
            // deconstructor
            // override acting as safety net if mapper deconstructor is no longer virtual for some reason
            ~Mapper_004() override = default;

            // interface overrides
            // cpu mapper read and write
            bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
            bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;
            // ppu mapper read and write
            bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
            bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;

            // mapper to cpu interrupt interface
            bool irqActive();
            void irqClear();
            void scanline(); // called by the ppu every scanline
            uint8_t mirrorMode() override;

        private:
            // mmc3 uses a target register system
            uint8_t nTargetRegister = 0x00;

            // control for inversion
            bool bPRGBankMode = false;
            bool bCHRInversion = false;

            // internal registers for bank indices
            uint32_t pRegister[8];
            uint32_t pPRGBank[4];
            uint32_t pCHRBank[8];

            // mirroring
            // 0: vertical | 1: horizontal
            uint8_t nMirrorMode = 0;

            // irq counter logic
            bool bIRQActive = false;
            bool bIRQEnable = false;
            bool bIRQReload = false;
            uint16_t nIRQCounter = 0x0000;
            uint16_t nIRQLatch = 0x0000;

    };

} // namespace nes