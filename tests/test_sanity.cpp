#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "nes/console.hpp"

static std::string find_nestest_rom() {
    namespace fs = std::filesystem;

    const std::vector<std::string> candidates = {
        "./tests/test_roms/nestest/nestest.nes",
        "../tests/test_roms/nestest/nestest.nes",
        "../../tests/test_roms/nestest/nestest.nes",
    };

    for (const auto& p : candidates) {
        if (fs::exists(p)) return p;
    }
    return "";
}

int main() {
    std::printf("[sanity] starting...\n");

    console con;

    const std::string rom_path = find_nestest_rom();
    if (rom_path.empty()) {
        std::fprintf(stderr, "[sanity] FAIL: could not locate nestest.nes\n");
        return 1;
    }

    // console::load_rom takes char*, but it doesn't modify the string.
    if (!con.load_rom(const_cast<char*>(rom_path.c_str()))) {
        std::fprintf(stderr, "[sanity] FAIL: load_rom failed: %s\n", rom_path.c_str());
        return 1;
    }

    unsigned long long c0 = con.get_cpu_cycles();
    int used = con.step_instruction();
    unsigned long long c1 = con.get_cpu_cycles();

    if (c1 <= c0) {
        std::fprintf(stderr,
                     "[sanity] FAIL: cpu cycles did not advance (%llu -> %llu), used=%d\n",
                     c0, c1, used);
        return 1;
    }

    std::printf("[sanity] PASS: cpu_cycles %llu -> %llu (used=%d)\n", c0, c1, used);
    return 0;
}