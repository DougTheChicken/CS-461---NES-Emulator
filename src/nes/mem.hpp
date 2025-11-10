#pragma once
#include <cstdint>

class Memory {
    public:
        Memory();

        uint8_t read(uint16_t address);

        void write(uint16_t address, uint8_t data);

        void dump(uint16_t start, uint16_t end);

    private:
        uint8_t ram[2048];

};