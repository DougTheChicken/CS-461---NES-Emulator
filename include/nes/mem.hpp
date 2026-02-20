#pragma once
#include <cstdint>
#include <cstddef>

namespace nes {

class Memory {
public:
    Memory();

    // CPU-visible bus reads/writes
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);

    // Map PRG ROM into $8000-$FFFF. For NROM, either 16KB mirrored or 32KB.
    void map_prg(const uint8_t* prg_data, std::size_t prg_size);

    // Update physical button state.
    // If strobe is active, it continuously loads into the shift register.
    void set_controller1(uint8_t v) {
        controller1 = v;
        if (strobe) controller1_shift = v;
    }
    void set_controller2(uint8_t v) {
        controller2 = v;
        if (strobe) controller2_shift = v;
    }

    // Reset memory-mapped state back to a known state (does not modify ROM bytes)
    void reset();

private:
    // Internal RAM ($0000-$07FF), mirrored to $1FFF
    uint8_t ram[0x800]{};

    // PRG ROM bytes (owned by ROM)
    const uint8_t* prg = nullptr;
    std::size_t prg_size_bytes = 0;

    // Controller registers (placeholders)
    uint8_t controller1 = 0;
    uint8_t controller2 = 0;

    // Internal shift registers (read by the CPU bit-by-bit)
    uint8_t controller1_shift = 0;
    uint8_t controller2_shift = 0;

	// Strobe state for controllers (bit 0 of $4016)
	bool strobe = false;

    // PPU/APU registers (placeholders)
    uint8_t ppu_regs[8]{};
    uint8_t apu_regs[0x18]{};

    // DMA register placeholder ($4014)
    uint8_t oam_dma = 0;
};

} // namespace nes
