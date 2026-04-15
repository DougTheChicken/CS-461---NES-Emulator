#include "nes/mapper/mapper_003.hpp"

namespace nes {

    // constructor: pass bank counts up to base mapper class
    Mapper_003::Mapper_003(uint16_t prgBanks, uint16_t chrBanks)
        // member initialiser using base mapper class
        : Mapper(prgBanks, chrBanks) {
            nSelectedCHRBank = 0;
        }

        // cpu mapper read
        // mapper 3 (cnrom) handles prg-rom range $8000-$FFFF
        bool Mapper_003::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
            // check range
            if (addr >= 0x8000 && addr <= 0xFFFF) {
                // mirroring logic:
                // if 1 bank (16 kb), mask with 0x3FFF to repeat data at $C000
                // if 2 banks (32 kb), mask with 0x7FFF for linear mapping
                mapped_addr = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
                return true;
            }

            // if outside range, don't allow cpu to read
            return false;
        }

        // cpu mapper write
        // mapper 3 (cnrom) does have bank-switching registers
        bool Mapper_003::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
            // check range
            if (addr >= 0x8000 && addr <= 0xFFFF) {
                // cnrom supports up to 4 banks
                nSelectedCHRBank = data & 0x03;
            }

            // if outside range, don't allow cpu to write
            return false;
        }

        // ppu mapper read
        // mapper 3 (cnrom) handles graphics range $0000-$1FFF
        bool Mapper_003::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                // map the 8 kb chr bank
                mapped_addr = (nSelectedCHRBank * 0x2000) + addr;
                return true;
            }

            // if outside range, don't allow ppu to read
            return false;
        }

        // ppu mapper write
        // mapper 3 (cnrom) uses chr-rom, so these writes are generally not allowed
        bool Mapper_003::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
            return false;
        }
} // namespace nes