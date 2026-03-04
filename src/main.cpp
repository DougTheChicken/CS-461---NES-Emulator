#include "nes/console.hpp"
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
    
    while (ui::step(emu, is_running)) {
        // If emulator is running, step forward
        // NES runs at ~1.79 MHz, so we step by a small amount per frame
        // At 60 FPS, that's ~29830 cycles per frame
        if (is_running) {
            emu.step(29830);
        }
    }

    // Cleanup
    ui::shutdown();
    std::fprintf(stderr, "Exiting\n");
    
    return 0;
}
