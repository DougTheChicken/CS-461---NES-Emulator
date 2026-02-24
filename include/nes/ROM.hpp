#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

// TODO: Make the iNES ROM spec a struct and parse it in a more structured way.

namespace nes {
    class ROM {
    public:
        bool load_from_file(const char* path);
        void reset();

        std::size_t prg_size_bytes() const { return PRG_ROM_size * 16384u; }
        const uint8_t* prg_data() const { return PRG_data.data(); }
        std::size_t chr_size_bytes() const { return CHR_ROM_size * 8192u; }
        const uint8_t* chr_data() const { return CHR_data.data(); }
        const bool alternate_nametable_layout() const { return has_alternate_nametable_layout; }
        const bool nametable_arrangement() const { return nametable_arrangement_; }
        uint8_t mapper() const { return mapper_id; }
        bool is_loaded() const { return parsed; }

    private:
        bool parsed = false;

        // Header
        int PRG_ROM_size = 0;   // 16KB units
        int CHR_ROM_size = 0;   // 8KB units

        // Flags 6
        bool nametable_arrangement_; // false: vertical, true: horizontal
        bool has_battery_backed_RAM;
        bool has_trainer;
        bool has_alternate_nametable_layout;
        uint8_t mapper_id = 0;

        // Flags 7-10 can be added later as needed as almost no games use them

        // Data
        std::vector<uint8_t> Trainer_data; // 512 bytes if present, parsed but not used for now
        std::vector<uint8_t> PRG_data;
        std::vector<uint8_t> CHR_data;
    };
}