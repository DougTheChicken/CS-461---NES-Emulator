#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

class Memory {
public:
    Memory();

    void map_prg(const uint8_t* prg_data, size_t prg_size); // 16KB or 32KB

    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

private:
    // $0000–$07FF internal RAM (mirrored to $1FFF)
    uint8_t ram[2048];

    // PRG ROM (mapped at $8000–$FFFF)
    const uint8_t* prg = nullptr;
    size_t prg_len = 0; // 0x4000 or 0x8000

    // super-minimal PPU stub state
    uint8_t ppu_status = 0x80; // bit7 (VBlank) set so init loops don’t hang
};
