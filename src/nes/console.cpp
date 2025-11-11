#include "nes/console.hpp"
#include <cstdio>

console::console() { init(); }
console::~console() = default;

bool console::load_rom(char* filepath) {
    if (!rom.load_from_file(filepath)) {
        std::fprintf(stderr, "[console] Failed to load ROM: %s\n", filepath);
        return false;
    }
    std::fprintf(stderr, "[console] Loaded: %s | mapper=%u | PRG banks=%d | CHR banks=%d | PRG=%zu bytes\n",
                 filepath, rom.mapper(), rom.prg_banks(), rom.chr_banks(), rom.prg_size_bytes());

    if (rom.mapper() != 0) {
        std::fprintf(stderr, "[console] WARNING: Only Mapper 0 (NROM) supported right now.\n");
    }

    mem.map_prg(rom.prg_data(), rom.prg_size_bytes());
    cpu.attach_memory(&mem);
    cpu.reset();

    uint16_t reset = static_cast<uint16_t>(mem.read(0xFFFC)) |
                     (static_cast<uint16_t>(mem.read(0xFFFD)) << 8);
    std::fprintf(stderr, "[console] ResetVector=$%04X  FirstOpcode=$%02X\n", reset, mem.read(reset));
    return true;
}

void console::run_rom() {
    for (int i = 0; i < 200000; ++i) {
        cpu.step_to(master_cycle_count + 3); // advance ~1 CPU boundary (3 PPU)
        master_cycle_count += 3;
        if ((i % 5000) == 0) {
            std::fprintf(stderr, "RUN i=%d  PC=%04X A=%02X X=%02X Y=%02X P=%02X\n", i, cpu.pc(), cpu.a());
        }
    }
}

void console::step(cycle_t stepcount) {
    cpu.step_to(master_cycle_count + stepcount);
    master_cycle_count += stepcount;
}

void console::init() {
    master_cycle_count = 0;
}
