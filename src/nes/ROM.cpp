#include "nes/ROM.hpp"
#include <fstream>
#include <array>
#include <algorithm>

namespace nes {
    static constexpr uint8_t NES_MAGIC[4] = { 'N','E','S', 0x1A };
    static constexpr std::size_t HEADER_SIZE        = 16;
    static constexpr std::size_t TRAINER_SIZE       = 512;
    static constexpr std::size_t PRG_BANK_SIZE      = 16 * 1024;
    static constexpr std::size_t CHR_BANK_SIZE      = 8 * 1024;

    // Parses the ROM file based on the iNES file format specification: https://www.nesdev.org/wiki/INES
    bool ROM::load_from_file(const char* path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;

        // Helper function to read an exact number of bytes from the file stream, returning false if the read fails
        // Used to simplify error handling when reading the ROM file (cleaner happy path code)
        // TODO: validate file length before reading (defensive against truncated ROMs)
        auto read_exact = [&](void* dst, std::size_t size) {
            f.read(static_cast<char*>(dst), size);
            return static_cast<bool>(f);
        };

        // Read header
        std::array<uint8_t, HEADER_SIZE> header{};
        if (!read_exact(header.data(), header.size())) return false;
        
        // Validate iNES magic header
        if (!std::equal(header.begin(), header.begin() + 4, NES_MAGIC)) return false;
    
        // ROM sizes (in banks)
        const std::size_t prg_bank_count = header[4];
        const std::size_t chr_bank_count = header[5];

        PRG_ROM_size = prg_bank_count;
        CHR_ROM_size = chr_bank_count; // TODO: handle CHR RAM case (0 banks means 8KB of CHR RAM, not 0 bytes of CHR ROM)

        const uint8_t flags6 = header[6];
        const uint8_t flags7 = header[7];
        // Not Implemented:
        // const uint8_t flags8 = header[8];
        // const uint8_t flags9 = header[9];
        // const uint8_t flags10 = header[10];

        // Flags 6 bit layout: https://www.nesdev.org/wiki/INES#Flags_6
        nametable_arrangement_          = (flags6 & 0x01) != 0;
        has_battery_backed_RAM         = (flags6 & 0x02) != 0;
        has_trainer                    = (flags6 & 0x04) != 0;
        has_alternate_nametable_layout = (flags6 & 0x08) != 0;
        // Mapper ID is split across flags 6 and 7, bit layout: https://www.nesdev.org/wiki/INES#Flags_7
        mapper_id = (flags6 >> 4) | (flags7 & 0xF0);

        // Optional trainer
        if (has_trainer) {
            Trainer_data.resize(TRAINER_SIZE);
            if (!read_exact(Trainer_data.data(), Trainer_data.size())) {
                return false;
            }
        }

        // PRG ROM
        PRG_data.resize(prg_bank_count * PRG_BANK_SIZE);
        if (!read_exact(PRG_data.data(), PRG_data.size())) {
            return false;
        }

        // CHR ROM
        CHR_data.resize(chr_bank_count * CHR_BANK_SIZE);
        if (!read_exact(CHR_data.data(), CHR_data.size())) {
            return false;
        }

        // Not Implemented:
        // - Read PlayChoice INST-ROM
        // - Read PlayChoice PROM

        parsed = true;
        return true;
    }

    void ROM::reset() {
        this->parsed = false;
        this->PRG_ROM_size = 0;
        this->CHR_ROM_size = 0;

        // booleans: use false, not NULL
        this->nametable_arrangement_ = false;
        this->has_battery_backed_RAM = false;
        this->has_trainer = false;
        this->has_alternate_nametable_layout = false;

        this->mapper_id = 0;
        this->Trainer_data.clear();
        this->PRG_data.clear();
        this->CHR_data.clear();
    }
}
