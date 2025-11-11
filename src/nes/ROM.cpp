#include "nes/ROM.hpp"
#include <fstream>
#include <array>
#include <algorithm>

namespace nes {
    static constexpr uint8_t NES_MAGIC[4] = { 'N','E','S', 0x1A };

    bool ROM::load_from_file(const char* path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;

        std::array<uint8_t,16> hdr{};
        f.read(reinterpret_cast<char*>(hdr.data()), 16);
        if (!f || !std::equal(hdr.begin(), hdr.begin()+4, NES_MAGIC)) return false;

        PRG_ROM_size = hdr[4];            // 16KB banks
        CHR_ROM_size = hdr[5];            // 8KB banks
        uint8_t flags6 = hdr[6];
        uint8_t flags7 = hdr[7];
        mapper_id = (flags6 >> 4) | (flags7 & 0xF0);

        bool has_trainer = (flags6 & 0x04) != 0;

        if (has_trainer) f.seekg(512, std::ios::cur);
        if (!f) return false;

        const std::size_t prg_bytes = static_cast<std::size_t>(PRG_ROM_size) * 16384u;
        const std::size_t chr_bytes = static_cast<std::size_t>(CHR_ROM_size) * 8192u;

        ROM_data.assign(prg_bytes + chr_bytes, 0);
        if (prg_bytes) {
            f.read(reinterpret_cast<char*>(ROM_data.data()), prg_bytes);
            if (!f) return false;
        }
        if (chr_bytes) {
            f.read(reinterpret_cast<char*>(ROM_data.data() + prg_bytes), chr_bytes);
            if (!f) return false;
        }
        return true;
    }

    std::size_t ROM::prg_size_bytes() const {
        return static_cast<std::size_t>(PRG_ROM_size) * 16384u;
    }

    const uint8_t* ROM::prg_data() const {
        return ROM_data.empty() ? nullptr : ROM_data.data();
    }
}
