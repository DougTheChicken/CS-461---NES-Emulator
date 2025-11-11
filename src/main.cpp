#include <SDL.h>
#include <cstdio>
#include "nes/console.hpp"

int main(int argc, char** argv) {
    // Headless ROM run (prove CPU works with real ROM), then exit or continue to SDL.
    if (argc >= 2) {
        console c;
        if (c.load_rom(argv[1])) {
            c.run_rom();
        } else {
            std::fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
        }
    } else {
        std::fprintf(stderr, "Tip: pass a ROM path (NROM) to run headless: %s path/to/rom.nes\n", argv[0]);
    }

    // (Optional) keep your SDL gradient demo here if you want a window after headless run.
    return 0;
}
