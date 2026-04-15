#include "nes/mapper/mapper_004.hpp"

namespace nes {

    // constructor: pass bank counts up to base mapper class
    Mapper_004::Mapper_004(uint16_t prgBanks, uint16_t chrBanks)
        // member initialiser using base mapper class
        : Mapper(prgBanks, chrBanks) {
            // initialise registers
            for (int i = 0; i < 8; i++)
                pRegister[i] = 0;

            // setup for fixed prg banks (last two)
            pPRGBank[2] = (prgBanks *  2 - 2) * 0x2000;
            pPRGBank[3] = (prgBanks *  2 - 1) * 0x2000;
        }

        // cpu mapper read
        // mapper 4 (mmc3) handles prg-rom range $8000-$FFFF
        bool Mapper_004::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
            // check range
            if (addr >= 0x8000 && addr <= 0xFFFF) {
                // divide range into 4 slots of 8 kb (0x2000)
                int slot = (addr - 0x8000) / 0x2000;
                mapped_addr = pPRGBank[slot] + (addr & 0x1FFF);
                return true;
            }

            // if outside range, don't allow cpu to read
            return false;
        }

        // cpu mapper write
        // mapper 4 (mmc3) handles complex bank switching
        bool Mapper_004::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
            // check range
            if (addr >= 0x8000 && addr <= 0x9FFF) {
                // check address parity
                if (!(addr & 0x0001)) {
                    // if even address: bank select
                    nTargetRegister = data & 0x07;
                    bPRGBankMode = data & 0x40;
                    bCHRInversion = data & 0x80;
                } else { 
                    // if odd address: bank data
                    pRegister[nTargetRegister] = data;
                }

                // update chr banks
                // determine offset
                int offset = bCHRInversion ? 4 : 0;

                // 2 kb banks (r0 & r1)
                pCHRBank[(0 + offset) % 8] = (pRegister[0] & 0xFE) * 0x0400;
                pCHRBank[(1 + offset) % 8] = (pRegister[0] | 0x01) * 0x0400;
                pCHRBank[(2 + offset) % 8] = (pRegister[1] & 0xFE) * 0x0400;
                pCHRBank[(3 + offset) % 8] = (pRegister[1] | 0x01) * 0x0400;

                // 1 kb banks (r2 - r5)
                pCHRBank[(4 + offset) % 8] = pRegister[2] * 0x0400;
                pCHRBank[(5 + offset) % 8] = pRegister[3] * 0x0400;
                pCHRBank[(6 + offset) % 8] = pRegister[4] * 0x0400;
                pCHRBank[(7 + offset) % 8] = pRegister[5] * 0x0400;

                // update prg banks
                if (bPRGBankMode) {
                    pPRGBank[0] = (prgBanks * 2 - 2) * 0x2000;
                    pPRGBank[2] = pRegister[6] * 0x2000;
                } else {
                    pPRGBank[0] = pRegister[6] * 0x2000;
                    pPRGBank[2] = (prgBanks * 2 - 2) * 0x2000;
                }
                pPRGBank[1] = pRegister[7] * 0x2000;
                
                return false;
            }

            // check range
            if (addr >= 0xA000 && addr <= 0xBFFF) {
                // check address parity
                if (!(addr & 0x0001)) {
                    // mirroring ($A000)
                    // 0: vertical | 1: horizontal
                    nMirrorMode = (data & 0x01);
                }

                return false;
            }

            // check range
            if (addr >= 0xC000 && addr <= 0xDFFF) {
                // check address parity
                if (!(addr & 0x0001)) {
                    // irq latch ($C000)
                    nIRQLatch = data;
                } else {
                    // irq reload ($C001)
                    bIRQReload = true;
                }

                return false;
            }

            // check range
            if (addr >= 0xE000 && addr <= 0xFFFF) {
                // check address parity
                if (!(addr & 0x0001)) {
                    // irq disable ($E000)
                    bIRQEnable = false;
                    bIRQActive = false;
                } else {
                    // irq enable ($E001)
                    bIRQEnable = true;
                }

                return false;
            }

            // if outside range, don't allow cpu to write
            return false;
        }

        // ppu mapper read
        // mapper 4 (mmc3) handles graphics range $0000-$1FFF
        bool Mapper_004::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                // divide range into 8 slots of 1 kb (0x0400)
                int slot = addr / 0x0400;
                mapped_addr = pCHRBank[slot] + (addr & 0x03FF);
                return true;
            }

            // if outside range, don't allow ppu to read
            return false;
        }

        // ppu mapper write
        // mapper 4 (mmc3) uses chr-ram to write if no chr-rom is present
        bool Mapper_004::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                // chr-ram check
                if (chrBanks == 0) {
                    mapped_addr = addr;
                    return true;
				}
            }

            // if outside range, don't allow ppu to write
            return false;
        }

        // checks if mapper is requesting an interrupt
        bool Mapper_004::irqActive() {
            return bIRQActive;
        }

        // acknowledges and clears mapper's interrupt request
        void Mapper_004::irqClear() {
            bIRQActive = false;
        }

        // advances the irq counter
        void Mapper_004::scanline() {
            // checks if counter is done or reload is set
            if (nIRQCounter == 0 || bIRQReload) {
                nIRQCounter = nIRQLatch;
                bIRQReload = false;
            } else {
                nIRQCounter--;
            }

            // checks if counter is done and irq is enabled
            if (nIRQCounter == 0 && bIRQEnable) {
                // trigger an irq to the cpu
                bIRQActive = true;
            }
        }

        // checks the current hardware mirroring mode
        uint8_t Mapper_004::mirrorMode() {
            return nMirrorMode;
        }
} // namespace nes