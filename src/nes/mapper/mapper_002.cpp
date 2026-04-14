#include "nes/mapper/mapper_002.hpp"

namespace nes {

    // constructor: pass bank counts up to base mapper class
    Mapper_002::Mapper_002(uint16_t prgBanks, uint16_t chrBanks)
        // member initialiser using base mapper class
        : Mapper(prgBanks, chrBanks) {
            nSelectedPRGBankLow = 0;
            nSelectedPRGBankHigh = prgBanks - 1;
        }

        // cpu mapper read
        // mapper 2 (uxrom) handles prg-rom range $8000-$FFFF
        bool Mapper_002::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
            // check range
            if (addr >= 0x8000 && addr <= 0xBFFF) {
                // switchable bank
                mapped_addr = (nSelectedPRGBankLow * 0x4000) + (addr & 0x3FFF);
                return true;
            }

            // check range
            if (addr >= 0xC000 && addr <= 0xFFFF) {
                // fixed bank
                mapped_addr = (nSelectedPRGBankHigh * 0x4000) + (addr & 0x3FFF);
                return true;
            }

            // if outside range, don't allow cpu to read
            return false;
        }

        // cpu mapper write
        //mapper 2 (uxrom) uses cpu writes to the rom range to trigger bank switching
        bool Mapper_002::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
            // check range
            if (addr >= 0x8000 && addr <= 0xFFFF) {
                // writing to this range will trigger a bank switch
                // 'data' determines the bank
                // uxrom usually only uses the lower 4 bits
                nSelectedPRGBankLow = data & 0x0F;
            }

            // always return false since there is no writing done
            return false;
        }

        // ppu mapper read
        // mapper 2 (uxrom) handles graphics range $0000-$1FFF
        bool Mapper_002::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                // mapper 2 (uxrom) uses 1:1 mapping for the ppu
                mapped_addr = addr;
                return true;
            }

            // if outside range, don't allow ppu to read
            return false;
        }

        // ppu mapper write
        // mapper 2 (uxrom) handles graphics range $0000-$1FFF
        bool Mapper_002::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                // check if chr-ram is being used
                if (chrBanks == 0) {
                    // mapper 2 (uxrom) uses 1:1 mapping for the ppu
                    mapped_addr = addr;
                    return true;
                } else {
                    // treat as chr-rom and ignore writes
                    return false;
                }
            }

            // if outside range, don't allow ppu to write
            return false;
        }
} // namespace nes