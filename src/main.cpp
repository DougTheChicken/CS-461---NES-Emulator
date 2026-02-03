#include "nes/console.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <romfile>\n", argv[0]);
        return 1;
    }

    console c;

    if (!c.load_rom(argv[1])) {
        std::fprintf(stderr, "Failed to load ROM\n");
        return 1;
    }

    c.run_rom();
    return 0;
}
