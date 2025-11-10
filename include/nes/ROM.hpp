#pragma once
#include <cstddef>

namespace nes {
    class ROM {
        public:
            ROM();
            ~ROM();

            bool load_from_file(const char* filepath);
            const char* get_data() const;
            size_t get_size() const;

        private:
            // Header fields (will likely need accessors at some point) (might need to be broken down further for mappers)
            char PRG_ROM_size; // Size of PRG ROM in 16KB units
            char CHR_ROM_size; // Size of CHR ROM in 8KB units
            char flags6;      // Mapper, mirroring, battery, trainer
            char flags7;      // Mapper, VS/Playchoice, NES 2.0
            char flags8;      // PRG-RAM size (rarely used extension)
            char flags9;      // TV system (rarely used extension)
            char flags10;     // TV system, PRG-RAM presence (rarely used extension)

            // Data fields
            char* ROM_data;
            size_t ROM_size;
            char* filepath;
    };
}