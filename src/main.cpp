#include "nes/console.hpp"
#include "nes/timing.hpp"
#include "ui/ui.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    // Initialize UI
    if (!ui::init()) {
        std::fprintf(stderr, "Failed to initialize UI\n");
        return 1;
    }

    // Create emulator instance
    console emu;
    bool is_running = false;

    // If ROM provided via CLI, load it immediately
    if (argc >= 2) {
        if (emu.load_rom(argv[1])) {
            std::fprintf(stderr, "Loaded ROM from CLI: %s\n", argv[1]);
            ui::init_audio(emu);
            is_running = true;
        } else {
            std::fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
        }
    }

    // Main loop
    std::fprintf(stderr, "Entering UI loop (press ESC to exit, SPACE to pause/run)\n");

    // Use a fractional accumulator so we step exactly the right number of CPU cycles per frame on average.
    // this fixes a systematic timing bias that would otherwise cause the generated audio sample rate to be slightly off
    // (e.g. 44100.8 Hz instead of 44100 Hz)
    // this fixes one of the most annoying bugs I have seen where the audio is just *slightly* wrong
    // I knew I wasn't crazy...
    double cycle_accumulator = 0.0;
    constexpr double CYCLES_PER_FRAME = CPU_HZ / 60.0; // ~29829.54...

    while (ui::step(emu, is_running)) {
        if (is_running) {
            // Step the emulator by the appropriate number of CPU cycles for one frame
            // (using a fractional accumulator to avoid systematic timing bias)
            cycle_accumulator += CYCLES_PER_FRAME;
            auto cycles = static_cast<cycle_t>(cycle_accumulator);
            cycle_accumulator -= static_cast<double>(cycles);
            emu.step(cycles * CPU_TO_PPU);
        }
    }

    // Cleanup
    ui::shutdown();
    std::fprintf(stderr, "Exiting\n");
    
    return 0;
}
