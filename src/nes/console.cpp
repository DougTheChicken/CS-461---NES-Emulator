#include "nes/console.hpp"
#include <cstdio>
#include <cstring>
#include <cstddef>

console::console()
    : rom(),
      ppu(),
      apu(),
      mem(ppu, apu),
      cpu()
{
    std::memset(framebuffer_data, 0, sizeof(framebuffer_data));
    init();
}
console::~console() = default;

bool console::load_rom(const char* filepath) {
    if (!rom.load_from_file(filepath)) {
        std::fprintf(stderr, "[console] Failed to load ROM: %s\n", filepath);
        return false;
    }

    // attach mapper to memory
    // allows cpu to use mapper for prg-rom addresses ($8000-$FFFF)
    mem.set_mapper(rom.get_mapper());
    mem.map_prg(rom.prg_data(), rom.prg_size_bytes());

    cpu.attach_memory(&mem);
    ppu.vertical_mirroring = rom.alternate_nametable_layout();

    const std::size_t prg_banks = rom.prg_size_bytes() / 16384;
    const std::size_t chr_banks = rom.chr_size_bytes() / 8192;

    std::fprintf(stderr,
        "[console] Loaded: %s | mapper=%u | PRG banks=%zu | CHR banks=%zu | PRG=%zu bytes\n",
        filepath,
        (unsigned)rom.mapper(),
        prg_banks,
        chr_banks,
        rom.prg_size_bytes()
    );

    // uses mapper to translate ppu adresses into chr-rom / ram indices
    ppu.set_chr_read_callback([this](uint16_t addr) -> uint8_t {
        uint32_t mapped_addr = 0;
        // check if active mapper from rom can handle proposed address
        if (rom.get_mapper()->ppuMapRead(addr, mapped_addr)) {
            return rom.chr_data()[mapped_addr];
        }
        return 0;
    });

    // allows writing to chr-ram if mapper permits
    ppu.set_chr_write_callback([this](uint16_t addr, uint8_t value) {
        uint32_t mapped_addr = 0;
        // check if active mapper from rom can handle proposed address
        if (rom.get_mapper()->ppuMapWrite(addr, mapped_addr)) {
            // NOTE: rom.chr_data() returns const uint8_t*, hence the cast
            const_cast<uint8_t*>(rom.chr_data())[mapped_addr] = value;
        }
    });


    cpu.reset();

    uint16_t reset = static_cast<uint16_t>(mem.read(0xFFFC)) |
                     (static_cast<uint16_t>(mem.read(0xFFFD)) << 8);



    std::fprintf(stderr, "[console] ResetVector=$%04X  FirstOpcode=$%02X\n",
                 (unsigned)reset, (unsigned)mem.read(reset));
    return true;
}

bool console::rom_loaded() const {
    return rom.is_loaded();
}

void console::run_rom() {
    for (int i = 0; i < 200000; ++i) {
        step_instruction();

        if ((i % 5000) == 0) {
            std::fprintf(stderr,
                "RUN i=%d  PC=%04X A=%02X X=%02X Y=%02X P=%02X  cpu_cycles=%llu\n",
                i,
                (unsigned)cpu.pc(),
                (unsigned)cpu.a(),
                (unsigned)cpu.x(),
                (unsigned)cpu.y(),
                (unsigned)cpu.p(),
                get_cpu_cycles()
            );
        }
    }
}

void console::step(cycle_t stepcount) {

    // we need to step apu & ppu along with cpu
    cycle_t target = master_cycle_count + stepcount;

    while (master_cycle_count < target) {
        // step PPU
        for (int i = 0; i < CPU_TO_PPU; i++) { ppu.step(); }
        apu.step(); // step one APU to one CPU here. APU handles its own timing

        // check NMI and IRQ
        if (ppu.nmi_pending) {
            ppu.nmi_pending = false;
            cpu.nmi();
        }
        if (apu.frame_irq_flag || apu.dmc.irq_pending) { cpu.irq(); }

        // step CPU
        cpu.step_to(master_cycle_count + 1);
        master_cycle_count++;
    }
    update_framebuffer();
}

unsigned long long console::get_cpu_cycles() const {
    return master_cycle_count / CPU_TO_PPU;
}

void console::step_cpu_cycles(unsigned long long cpu_cycles) {
    step(static_cast<cycle_t>(cpu_cycles * CPU_TO_PPU));
}

int console::step_instruction() {
    int used_cpu_cycles = cpu.step();
    if (used_cpu_cycles < 0) used_cpu_cycles = 0;

    master_cycle_count +=
        static_cast<unsigned long long>(used_cpu_cycles) * CPU_TO_PPU;

    update_framebuffer();
    return used_cpu_cycles;
}

void console::init() {
    master_cycle_count = 0;

    cpu.attach_memory(&mem);
    apu.dmc.attach_memory(&mem);
    cpu.reset();
}

void console::reset_all() {
    master_cycle_count = 0;
    cpu.reset();
    rom.reset();
    mem.set_mapper(nullptr);
    mem.reset();
    ppu.reset();
    apu.reset();
    std::memset(framebuffer_data, 0, sizeof(framebuffer_data)); // Clear framebuffer
    std::fprintf(stderr, "[console] Reset complete\n");
}

const uint32_t* console::framebuffer() const {
    return ppu.framebuffer_output();
}

void console::update_framebuffer() {

}
