#pragma once
#include <cstdint>

namespace nes {

    // base mapper class for all other mappers to follow
    class Mapper {

        public:
            // constructor
            Mapper(uint16_t prgBanks, uint16_t chrBanks)
                // member initialiser
                : prgBanks(prgBanks), chrBanks(chrBanks) {}
            // destructor
            virtual ~Mapper() = default;

            // interface rules
            // these are pure virtual functions to prevent the creation of "generic" mappers
            // input (addr): 16-bit address the cpu or ppu are trying to access
            // output (mapped_addr): mapper calculated index of (potentially) large rom array
            //        NOTE: rom arrays can be much larger than 64 kb, hence use of uint32_t
            // return (bool): true if address is handled, false otherwise
            // cpu mapper read and write
            virtual bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) = 0;
            virtual bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) = 0;
            // ppu mapper read and write
            virtual bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) = 0;
            virtual bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) = 0;
			// PPU Mirroring (not all mappers have to implement this, so not pure virtual)
            virtual uint8_t mirrorMode() { return 0xFF; }

        // this is protected rather than private so other mapper handling classes
        // (i.e. nrom, mmc1, mmc3, etc.) can still see these variables and modify them
        protected:
            // prgBanks: number of 16 kb prg-rom units (where game code lives)
            // uint16_t instead of uint8_t for mapper 5 handling
            uint16_t prgBanks = 0;
            // chrBanks: number of 8 kb chr-rom units (where graphics / sprites live)
            // uint16_t instead of uint8_t for mapper 5 handling
            uint16_t chrBanks = 0;

    };

} // namespace nes