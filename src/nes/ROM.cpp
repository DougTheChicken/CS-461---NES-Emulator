#include "nes/ROM.hpp"
#include <fstream>
#include <array>
#include <algorithm>

namespace nes {
    static constexpr uint8_t NES_MAGIC[4] = { 'N','E','S', 0x1A };

    bool ROM::load_from_file(const char* path) {
        // Open file
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;

        // Read header
        std::array<uint8_t,16> header{};
        f.read(reinterpret_cast<char*>(header.data()), 16);
        if (!f) return false;
        // Look for NES magic header
        if (!f || !std::equal(header.begin(), header.begin()+4, NES_MAGIC)) return false;

        // Parse header
        this->PRG_ROM_size = static_cast<std::size_t>(header[4]);            // 16KB banks
        this->CHR_ROM_size = static_cast<std::size_t>(header[5]);            // 8KB banks
        uint8_t flags6 = header[6];
        uint8_t flags7 = header[7];     // mostly unused for now
        uint8_t flags8 = header[8];     // unused for now
        uint8_t flags9 = header[9];     // unused for now
        uint8_t flags10 = header[10];   // unused for now

        // Parse flags6
        this->nametable_arrangement             = (flags6 & 0x01) != 0;
        this->has_battery_backed_RAM            = (flags6 & 0x02) != 0;
        this->has_trainer                       = (flags6 & 0x04) != 0;
        this->has_alternate_nametable_layout    = (flags6 & 0x08) != 0;
        this->mapper_id                         = (flags6 >> 4) | (flags7 & 0xF0);

        // Read Trainer if present
        if (has_trainer) {
            this->Trainer_data.resize(512);
            f.read(reinterpret_cast<char*>(this->Trainer_data.data()), 512);
            if (!f) return false;
        }

        // Read PRG data
        this->PRG_data.resize(this->PRG_ROM_size * 16384);
        f.read(reinterpret_cast<char*>(this->PRG_data.data()), this->PRG_data.size());
        if (!f) return false;

        // Read CHR data
        this->CHR_data.resize(this->CHR_ROM_size * 8192);
        f.read(reinterpret_cast<char*>(this->CHR_data.data()), this->CHR_data.size());
        if (!f) return false;

        // Not Implemented:
        // - Read PlayChoice INST-ROM
        // - Read PlayChoice PROM

        this->parsed = true;
        return true;
    }

    void ROM::reset() {
        this->parsed = false;
        this->PRG_ROM_size = 0;
        this->CHR_ROM_size = 0;
        this->nametable_arrangement = NULL;
        this->has_battery_backed_RAM = NULL;
        this->has_trainer = NULL;
        this->has_alternate_nametable_layout = NULL;
        this->mapper_id = 0;
        this->Trainer_data.clear();
        this->PRG_data.clear();
        this->CHR_data.clear();
    }
}
