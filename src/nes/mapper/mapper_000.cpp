#include "nes/mapper/mapper_000.hpp"

namespace nes {

    // constructor: pass bank counts up to base mapper class
    Mapper_000::Mapper_000(uint16_t prgBanks, uint16_t chrBanks, bool is_chr_ram)
        // member initialiser using base mapper class
        : Mapper(prgBanks, chrBanks), is_chr_ram(is_chr_ram) {}

        // cpu mapper read
        // mapper 0 (nrom) handles prg-rom range $8000-$FFFF
        bool Mapper_000::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) {
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
        // mapper 0 (nrom) doesn't have bank-switching registers (it's a simple mapper)
        bool Mapper_000::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
            // CPU writes in PRG-ROM space are ignored.
            return false;
        }

        // ppu mapper read
        // mapper 0 (nrom) handles graphics range $0000-$1FFF
        bool Mapper_000::ppuMapRead(uint16_t addr, uint32_t &mapped_addr) {
            // check range
            if (addr >= 0x0000 && addr <= 0x1FFF) {
                // mapper 0 (nrom) uses 1:1 mapping
                mapped_addr = addr;
                return true;
            }

            // if outside range, don't allow ppu to read
            return false;
        }

        // ppu mapper write
        // mapper 0 (nrom) uses rom for chr unless the cartridge provides chr-ram
        bool Mapper_000::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) {
            if (addr >= 0x0000 && addr <= 0x1FFF && is_chr_ram) {
                mapped_addr = addr;
                return true;
            }

            return false;
        }
} // namespace nes