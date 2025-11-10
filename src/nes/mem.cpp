#include "mem.hpp"
#include <cstdio>
#include <cstring>

Memory::Memory(){
    std::memset(ram, 0, sizeof(ram));
}

uint8_t Memory::read(uint16_t addr){
    if (addr < 0x2000){
        return ram[addr % 0x0800];
    }
    return 0x00;
}

void Memory::write(uint16_t addr, uint8_t data){
    if (addr < 0x0200){
        ram[addr % 0x0800] = data;
        return;
    }
}

void Memory::dump(uint16_t start, uint16_t end){
    for (uint16_t addr = start; addr <= end; addr++){
        printf("%04X: %02X\n", addr, read(addr));
    }
}
