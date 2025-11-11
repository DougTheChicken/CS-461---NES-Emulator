#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

namespace nes {
    class ROM {
    public:
        bool load_from_file(const char* path);

        std::size_t     prg_size_bytes() const;
        const uint8_t*  prg_data() const;

        uint8_t mapper() const { return mapper_id; }
        int     prg_banks() const { return PRG_ROM_size; }
        int     chr_banks() const { return CHR_ROM_size; }

    private:
        int PRG_ROM_size = 0;   // 16KB units
        int CHR_ROM_size = 0;   // 8KB units
        uint8_t mapper_id = 0;

        // PRG then CHR (trainer skipped if present)
        std::vector<uint8_t> ROM_data;
    };
}