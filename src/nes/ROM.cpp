#include "nes/ROM.hpp"

#include <cstdio>
#include <cstdlib>

using namespace nes;

nes::ROM::ROM(){
    this->ROM_data = nullptr;
    this->ROM_size = 0;
}

nes::ROM::~ROM(){
    if (this->ROM_data != nullptr){
        delete[] this->ROM_data;
    }
}

bool nes::ROM::load_from_file(const char* filepath){
    // open file
    FILE* file = std::fopen(filepath, "rb");
    if (!file) {
        return false;
    }

    // read header
    char header[16];
    std::fread(header, sizeof(char), 16, file);

    // parse header
    this->PRG_ROM_size = header[4];
    this->CHR_ROM_size = header[5];
    this->flags6 = header[6];
    this->flags7 = header[7];
    this->flags8 = header[8];
    this->flags9 = header[9];
    this->flags10 = header[10];

    // calculate ROM size
    this->ROM_size = (this->PRG_ROM_size * 16384) + (this->CHR_ROM_size * 8192);

    // allocate memory for ROM data
    this->ROM_data = new char[this->ROM_size];

    // read ROM data
    std::fread(this->ROM_data, sizeof(char), this->ROM_size, file);

    // close file
    std::fclose(file);

    return true;
}

const char* nes::ROM::get_data() const{
    return this->ROM_data;
}

size_t nes::ROM::get_size() const{
    return this->ROM_size;
}
