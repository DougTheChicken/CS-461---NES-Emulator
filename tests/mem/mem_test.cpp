#include <iostream>
#include <cassert>
#include "../../include/nes/mem.hpp"

// A helper to print "PASS" in green
void log_pass(const char* test_name) {
    std::cout << "\033[1;32m[PASS] " << test_name << "\033[0m" << std::endl;
}

void test_ram_functionality() {
    Memory mem;

    // 1. Basic Read/Write
    mem.write(0x0000, 0x42);
    assert(mem.read(0x0000) == 0x42);

    // 2. RAM Mirroring
    // Writing to $0000 should be visible at $0800, $1000, $1800
    mem.write(0x0000, 0xAB);
    assert(mem.read(0x0800) == 0xAB); // Mirror 1
    assert(mem.read(0x1000) == 0xAB); // Mirror 2
    assert(mem.read(0x1800) == 0xAB); // Mirror 3

    // 3. Reverse Mirroring
    // Writing to a mirror ($1FFF) should affect base ($07FF)
    mem.write(0x1FFF, 0xCD);
    assert(mem.read(0x07FF) == 0xCD);

    log_pass("RAM Read/Write & Mirroring");
}

void test_ppu_registers() {
    Memory mem;

    // 1. Test Initial PPU Status ($2002)
    // Your constructor sets ppu_status = 0x80 (VBlank)
    uint8_t status = mem.read(0x2002);
    assert((status & 0x80) == 0x80);

    // 2. Test PPU Register Mirroring
    // $2000 mirrored every 8 bytes ($2008, $2010, etc.)
    // Since PPUCTRL ($2000) is usually write-only and returns 0 or open bus,
    // we can only test that writing to mirrors doesn't crash.
    // However, we CAN test that reading $2002 works at $200A (Mirror)
    assert((mem.read(0x200A) & 0x80) == 0x80);

    log_pass("PPU Register Access & Mirroring");
}

void test_bounds_safety() {
    Memory mem;

    // Test Open Bus / Unmapped regions
    // $4020 is start of Cartridge space, but no cart loaded yet.
    // Should return 0x00 (or open bus behavior) and NOT crash.
    assert(mem.read(0x4020) == 0x00);

    log_pass("Bounds & Safety");
}

int main() {
    std::cout << "Running NES Memory Tests..." << std::endl;

    test_ram_functionality();
    test_ppu_registers();
    test_bounds_safety();

    std::cout << "\nAll tests passed successfully!!" << std::endl;
    return 0;
}