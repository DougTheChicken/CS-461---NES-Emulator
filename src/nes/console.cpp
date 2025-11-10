// console is a top-level implementation of the entire NES
// this is where ROMs are load, system-wide reset happens


#include "nes/console.hpp"
#include "nes/cpu.hpp"

using namespace nes;

console::console(){
    this->init();
}

console::~console(){
    delete &this->rom;
}

void console::start(){

}

void console::stop(){

}

void console::reset(){
    // reset all components, clear data and varables, and reinitialize
    this->master_cycle_count = 0;
    this->cpu.reset();
    this->ppu.reset();
    this->apu.reset();
    delete &this->rom;
    this->init();
}

bool console::load_rom(char* filepath){
    return this->rom.load_from_file(filepath);
}

void console::run_rom(){

}

void console::step(cycle_t stepcount){
    // step all components by stepcount cycles
    this->cpu.step_to(this->master_cycle_count + stepcount);
    // ppu.step_to(master_cycle_count + stepcount);
    // apu.step_to(master_cycle_count + stepcount);
    this->master_cycle_count += stepcount;
}

void console::test(){

}

void console::init(){
    this->master_cycle_count = 0;
    this->cpu = nes::CPU();
    this->ppu = nes::PPU();
    this->apu = nes::APU();
    this->rom = nes::ROM();
}