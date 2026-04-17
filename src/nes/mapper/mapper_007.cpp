#include "nes/mapper/mapper_007.hpp"

namespace nes {

    // constructor: pass bank counts up to base mapper class
    Mapper_007::Mapper_007(uint16_t prgBanks, uint16_t chrBanks)
        // member initialiser using base mapper class
        : Mapper(prgBanks, chrBanks) {}

        // cpu mapper read
        bool Mapper_007::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
            // check range
            if (addr >= 0x8000 && addr <= 0xFFFF) {
                mapped_addr = (nSelectedBank * 0x8000) + (addr & 0x7FFF);
                return true;
            }

            // if outside range, don't allow cpu to read
            return false;
        }

        // cpu mapper write
        bool Mapper_007::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
            // check range
            if (addr >= 0x8000 && addr <= 0xFFFF) {
                nSelectedBank = data & 0x07;
                if (data & 0x10) {
                    nMirrorMode = 1;
                } else {
                    nMirrorMode = 0;
                }
            }

            // if outside range, don't allow cpu to write
            return false;
        }

        // ppu mapper read
        bool Mapper_007::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                mapped_addr = addr;
                return true;
            }

            // if outside range, don't allow ppu to read
            return false;
        }

        // ppu mapper write
        bool Mapper_007::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                mapped_addr = addr;
                return true;
            }

            // if outside range, don't allow ppu to write
            return false;
        }

        // mapper to cpu interrupt interface
        uint8_t Mapper_007::mirrorMode() {
            return nMirrorMode;
        }

} // namespace nes