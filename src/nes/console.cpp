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

    if (rom.mapper() != 0) {
        std::fprintf(stderr, "[console] WARNING: Only Mapper 0 (NROM) supported right now.\n");
    }

    mem.map_prg(rom.prg_data(), rom.prg_size_bytes());
    cpu.attach_memory(&mem);
    ppu.vertical_mirroring = rom.alternate_nametable_layout();

    // TODO: change 0x1FFF bank translation when we implement mapper support
    ppu.set_chr_read_callback([this](uint16_t addr) -> uint8_t { return rom.chr_data()[addr & 0x1FFF]; });
    ppu.set_chr_write_callback([this](uint16_t addr, uint8_t value) {
        // TODO: when Mapper with chr_write(uint16_t addr, uint8_t value) uncomment mapper->chr_write(addr, value);
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
    mem.reset();
    ppu.reset();
    apu.reset();
    std::memset(framebuffer_data, 0, sizeof(framebuffer_data)); // Clear framebuffer
    std::fprintf(stderr, "[console] Reset complete\n");
}

const uint32_t* console::framebuffer() const {
    return framebuffer_data;
}

void console::update_framebuffer() {
    // Placeholder: generate a simple test pattern
    // This will be replaced when PPU is fully implemented
    for (int y = 0; y < 240; ++y) {
        for (int x = 0; x < 256; ++x) {
            // Create a simple pattern based on CPU state
            uint8_t r = static_cast<uint8_t>((x + cpu.a()) & 0xFF);
            uint8_t g = static_cast<uint8_t>((y + cpu.x()) & 0xFF);
            uint8_t b = static_cast<uint8_t>(cpu.y());
            framebuffer_data[y * 256 + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
}
