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

    // Storing state for $2000-$2007
    uint8_t ppu_ctrl = 0x00; // $2000
    uint8_t ppu_mask = 0x00; // $2001
    uint8_t ppu_status = 0x00; // $2002
    uint8_t oam_addr = 0x00; // $2003
    uint8_t ppu_scroll = 0x00; // $2005
    uint8_t ppu_addr = 0x00; // $2006
};
