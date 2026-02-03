#include "nes/ROM.hpp"
#include <fstream>
#include <array>
#include <algorithm>

namespace nes {
    static constexpr uint8_t NES_MAGIC[4] = { 'N','E','S', 0x1A };

    bool ROM::load_from_file(const char* path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;

        std::array<uint8_t,16> header{};
        f.read(reinterpret_cast<char*>(header.data()), 16);
        if (!f) return false;

        if (!std::equal(header.begin(), header.begin() + 4, NES_MAGIC)) return false;

        this->PRG_ROM_size = static_cast<std::size_t>(header[4]); // 16KB banks
        this->CHR_ROM_size = static_cast<std::size_t>(header[5]); // 8KB banks

        uint8_t flags6  = header[6];
        uint8_t flags7  = header[7];
        (void)header[8];
        (void)header[9];
        (void)header[10];

        this->nametable_arrangement          = (flags6 & 0x01) != 0;
        this->has_battery_backed_RAM         = (flags6 & 0x02) != 0;
        this->has_trainer                    = (flags6 & 0x04) != 0;
        this->has_alternate_nametable_layout = (flags6 & 0x08) != 0;
        this->mapper_id                      = (flags6 >> 4) | (flags7 & 0xF0);

        if (has_trainer) {
            this->Trainer_data.resize(512);
            f.read(reinterpret_cast<char*>(this->Trainer_data.data()), 512);
            if (!f) return false;
        }

        this->PRG_data.resize(this->PRG_ROM_size * 16384);
        f.read(reinterpret_cast<char*>(this->PRG_data.data()), this->PRG_data.size());
        if (!f) return false;

        this->CHR_data.resize(this->CHR_ROM_size * 8192);
        f.read(reinterpret_cast<char*>(this->CHR_data.data()), this->CHR_data.size());
        if (!f) return false;

        this->parsed = true;
        return true;
    }

    void ROM::reset() {
        this->parsed = false;
        this->PRG_ROM_size = 0;
        this->CHR_ROM_size = 0;

        // booleans: use false, not NULL
        this->nametable_arrangement = false;
        this->has_battery_backed_RAM = false;
        this->has_trainer = false;
        this->has_alternate_nametable_layout = false;

        this->mapper_id = 0;
        this->Trainer_data.clear();
        this->PRG_data.clear();
        this->CHR_data.clear();
    }
}
