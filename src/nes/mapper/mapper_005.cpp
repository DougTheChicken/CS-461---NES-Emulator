#include "nes/mapper/mapper_005.hpp"

namespace nes {

    // constructor: pass bank counts up to base mapper class
    Mapper_005::Mapper_005(uint16_t prgBanks, uint16_t chrBanks)
        // member initialiser using base mapper class
        : Mapper(prgBanks, chrBanks) {
            // set banks
            for (int i = 0; i < 5; i++) {
                nPRGBank[i] = (prgBanks * 2) - 1;
            }
        }

        // cpu mapper read
        bool Mapper_005::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
            // irq status reg
            if (addr == 0x5204) {
                data = (bIRQPending ? 0x80 : 0x00) | (bInFrame ? 0x40 : 0x00);
                bIRQPending = false;
                return false;
            }

            // check range - hardware multipier regs
            if (addr == 0x5205 || addr == 0x5206) {
                // do math
                uint16_t product = (uint16_t)nMultA * (uint16_t)nMultB;

                // check address
                if (addr == 0x5205) {
                    // get lower 8 bits
                    data = product & 0xFF;
                } else {
                    // get upper 8 bits
                    data = product >> 8;
                }

                return false;
            }

            // check range - exram access
            if (addr >= 0x5C00 && addr <= 0x5FFF && nExRAMMode >= 2) {
                // access internal exram
                data = pExRAM[addr - 0x5C00];

                return false;
            }

            // check range - prg mapping logic
            if (addr >= 0x6000 && addr <= 0xFFFF) {
                uint8_t window = 0;
                uint32_t offset = 0;

                // check range - window 0 ($6000-$7FFF)
                if (addr <= 0x7FFF) {
                    offset = addr - 0x6000;
                } else {
                    switch (nPRGMode) {
                        case 0:     // 32 kb
                            window = 4; // uses reg $5117
                            mapped_addr = ((nPRGBank[window] & ~0x03) * 0x2000) + (addr - 0x8000);
                            return true;
                        case 1:     // 16 kb
                            if (addr <= 0xBFFF) {
                                window = 2;
                            } else {
                                window = 4;
                            }
                            offset = addr & 0x3FFF;
                            mapped_addr = ((nPRGBank[window] & ~0x01) * 0x2000) + offset;
                            return true;
                        case 2:     // 16 kb + 8 kb
                            if (addr <= 0xBFFF) {
                                window = 2;
                                offset = addr & 0x3FFF;
                            } else if (addr <= 0xDFFF) {
                                window = 3;
                                offset = addr & 0x1FFF;
                            } else {
                                window = 4;
                                offset = addr & 0x1FFF;
                            }
                            break;
                        case 3:     // 8 kb
                            if (addr <= 0x9FFF) {
                                window = 1;
                            } else if (addr <= 0xBFFF) {
                                window = 2;
                            } else if (addr <= 0xDFFF) {
                                window = 3;
                            } else {
                                window = 4;
                            }
                            offset = addr & 0x1FFF;
                            break;
                    }
                }
                mapped_addr = (nPRGBank[window] * 0x2000) + offset;

                return true;
            }
            
            // if outside any range, don't allow cpu to read
            return false;
        }

        // cpu mapper write
        bool Mapper_005::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
            // check range - mmc5 conrol; registers ($5000 - $5206)
            if (addr >= 0x5000 && addr <= 0x5206) {
                switch (addr) {
                    case 0x5100:
                        nPRGMode = data & 0x03;
                        break;
                    case 0x5101:
                        nCHRMode = data & 0x03;
                        break;
                    case 0x5102:
                        nProtect1 = data & 0x03;
                        break;
                    case 0x5103:
                        nProtect2 = data & 0x03;
                        break;
                    case 0x5104:
                        nExRAMMode = data & 0x03;
                        break;

                    // fill mode
                    case 0x5105:
                        nMirrorMode = data;
                        break;
                    case 0x5106:
                        nFillTile = data;
                        break;
                    case 0x5107:
                        nFillAttribute = data & 0x03;
                        break;

                    // prg bank select
                    case 0x5113:
                        nPRGBank[0] = data;
                        break;
                    case 0x5114:
                        nPRGBank[1] = data;
                        break;
                    case 0x5115:
                        nPRGBank[2] = data;
                        break;
                    case 0x5116:
                        nPRGBank[3] = data;
                        break;
                    case 0x5117:
                        nPRGBank[4] = data | 0x80;
                        break;
                    
                    // chr register
                    case 0x5120:
                    case 0x5121:
                    case 0x5122:
                    case 0x5123:
                    case 0x5124:
                    case 0x5125:
                    case 0x5126:
                    case 0x5127:
                        pCHRBank[addr - 0x5120] = data | (nCHRUpperBits << 8);
                        nLastCHRSet = 0;
                        break;
                    case 0x5128:
                    case 0x5129:
                    case 0x512A:
                    case 0x512B:
                        pCHRBank[addr - 0x5120] = data | (nCHRUpperBits << 8);
                        nLastCHRSet = 1;
                        break;
                    case 0x5130:
                        nCHRUpperBits = data & 0x03;
                        break;

                    // irq control
                    case 0x5203:
                        nIRQTarget = data;
                        break;
                    case 0x5204:
                        bIRQEnable = (data & 0x80);
                        break;

                    // hardware multiplier
                    case 0x5205:
                        nMultA = data;
                        break;
                    case 0x5206:
                        nMultB = data;
                        break;
                }

                // reg writes don't go to rom
                return false;                
            }

            // check range - exram ($5C00 - $5FFF)
            if (addr >= 0x5C00 && addr <= 0x5FFF && nExRAMMode <= 2) {
                pExRAM[addr - 0x5C00] = data;
                return false;
            }

            // check range - prg-ram/rom ($6000 - $FFFF)
            if (addr >= 0x6000 && addr <= 0xFFFF) {
                // check if ram is free/unlocked
                if (nProtect1 == 0x02 && nProtect2 == 0x01) {
                    uint8_t window = 0;
                    // find the current window index
                    // maybe make this a helper???
                    if (addr >= 0x8000 && addr <= 0x9FFF) {
                        window = 1;
                    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
                        window = 2;
                    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
                        window = 3;
                    } else {
                        window = 4;
                    }

                    // check if in ram or rom - 0: ram & 1: rom
                    if ((nPRGBank[window] & 0x80) == 0) {
                        mapped_addr = (nPRGBank[window] * 0x2000) + (addr & 0x1FFF);
                        return true;
                    }
                }

                // ignoring since it's rom or locked/busy
                return false;
            }

            // if outside any range, don't allow cpu to write
            return false;
        }

        // ppu mapper read
        bool Mapper_005::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
            // check range - chr ($0000 - $1FFF)
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                uint8_t window = 0;

                // determine the window based on nchrmode
                switch (nCHRMode) {
                    case 0:     // 8 kb
                        window = 7;
                        break;
                    case 1:     // 4 kb
                        if (addr <= 0x0FFF) {
                            window = 3;
                        } else {
                            window = 7;
                        }
                        break;
                    case 2:     // 2 kb
                        window = (addr >> 11) * 2 + 1;
                        break;
                    case 3:     // 1 kb
                        window = (addr >> 10);
                        break;
                }

                // set bank set
                uint32_t bank = pCHRBank[window + (nLastCHRSet * 8)];

                // calculate final address
                mapped_addr = (bank * 0x0400) + (addr & 0x03FF);
                return true;
            }

            // check range - nametable ($2000 - $3FFF)
            if (addr >= 0x2000 && addr <= 0x3FFF) {
                // determine which nametable
                int nt_window = (addr >> 10) & 0x03;

                // see where window is mapped
                uint8_t mode = (nMirrorMode >> (nt_window * 2)) & 0x03;

                // check mode
                if (mode == 2) {         // exram
                    mapped_addr = 0xFF000000 | pExRAM[addr & 0x03FF];
                    return true;
                } else if (mode == 3) {         // fill mode
                    // check attribute range ($3C0 - $3FF)
                    if ((addr & 0x03FF) >= 0x03C0) {
                        mapped_addr = 0xFF000000 | nFillAttribute;
                    } else {
                        mapped_addr = 0xFF000000 | nFillTile;
                    }
                    return true;
                }

                return false;
            }

            // if outside any range, don't allow ppu to read
            return false;
        }

        // ppu mapper write
        bool Mapper_005::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
            // check range - nametable ($2000 - $3FFF)
            if (addr >= 0x2000 && addr <= 0x3FFF) {
                int nt_window = (addr >> 10) & 0x03;
                uint8_t mode = (nMirrorMode >> (nt_window * 2)) & 0x03;

                if (mode == 2) {    // exram
                    mapped_addr = 0xFF000000 | (addr & 0x03FF);
                    return true;
                }
            }
            // if outside any range, don't allow ppu to write
            return false;
        }

        // cpu interrupt interface
        bool Mapper_005::irqActive() {
            return bIRQPending && bIRQEnable;
        }
        
        void Mapper_005::irqClear() {
            bIRQPending = false;
        }

        void Mapper_005::scanline() {
            if (bInFrame) {
                nScanlineCounter++;
                if (nScanlineCounter == nIRQTarget) {
                    bIRQPending = true;
                }
            }
        }

        uint8_t Mapper_005::mirrorMode() {
            return nMirrorMode;
        }
} // namespace nes