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
    
    mem.insert_cartridge(&rom);
    ppu.insert_cartridge(&rom);

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

void console::step(cycle_t stepcount)
{
    const cycle_t target = master_cycle_count + stepcount;

    while (master_cycle_count < target)
    {
        // one PPU cycle always happens
        ppu.step();
        master_cycle_count++;

        // stalls expressed in PPU cycles
        bool stalled = false;

        if (apu.dmc.pending_stall_cycles > 0) {
            apu.dmc.pending_stall_cycles--;
            stalled = true;
        }
        if (ppu.oam_dma_pending_stall > 0) {
            ppu.oam_dma_pending_stall--;
            stalled = true;
        }

        if (stalled) {
            cpu.add_stall(1);

            // APU runs on CPU boundaries
            if ((master_cycle_count % CPU_TO_PPU) == 0) {
                apu.step();
            }
            continue;
        }

        // 3) CPU/APU on CPU boundaries
        if ((master_cycle_count % CPU_TO_PPU) == 0)
        {
            apu.step();

            // catch CPU up to this boundary
            cpu.step_to(master_cycle_count, ppu, apu);

            // latch interrupts for next instruction boundary
            if (ppu.nmi_pending) {
                ppu.nmi_pending = false;
                cpu.nmi();
            }
            if (apu.frame_irq_flag || apu.dmc.irq_pending) {
                cpu.irq();
            }
        }
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
    return ppu.framebuffer_output();
}

void console::update_framebuffer() {

}
