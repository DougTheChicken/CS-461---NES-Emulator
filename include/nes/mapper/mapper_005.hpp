#pragma once
#include "mapper.hpp"

namespace nes {

    class Mapper_005 : public Mapper {

        public:
            // constructor
            Mapper_005(uint16_t prgBanks, uint16_t charBanks);
            // deconstructor
            // override acting as safety net if mapper deconstructor is no longer virtual for some reason
            ~Mapper_005() override = default;

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
            // prg banking ($5100, $5113-$5117)
            uint8_t nPRGMode = 3;
            uint8_t nPRGBank[5]; // $6000, $8000, $A000, $C000, $E000

            // chr banking ($5101, $5120-$5130)
            uint8_t nCHRMode = 0;
            uint8_t nCHRUpperBits = 0; // $5130
            uint8_t nLastCHRSet = 0;   // tracks if a or b regs were written last
            uint32_t pCHRBank[12]; // a regs ($5120-7) & b regs ($5128-B)

            // irq counter ($5203, $5204)
            bool bIRQEnable = false;
            bool bIRQPending = false;
            bool bInFrame = false;
            uint8_t nIRQTarget = 0;
            uint8_t nScanlineCounter = 0;

            // hardware multiplier ($5205, $5206)
            uint8_t nMultA = 0xFF;
            uint8_t nMultB = 0xFF;

            // exram ($5104, $5C00-$5FFF)
            uint8_t nExRAMMode = 0;
            uint8_t pExRAM[1024];

            // fill mode tile/colour ($5105 - $5107)
            uint8_t nMirrorMode = 0;
            uint8_t nFillTile = 0;
            uint8_t nFillAttribute = 0;

            // prg-ram protect regs ($5102, $5103)
            uint8_t nProtect1 = 0x00;
            uint8_t nProtect2 = 0x00;

    };

} // namespace nes