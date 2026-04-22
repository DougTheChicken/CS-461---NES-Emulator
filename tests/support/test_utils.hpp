#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace test_support {

inline std::string find_nestest_rom() {
    namespace fs = std::filesystem;

    const std::vector<std::string> candidates = {
        "./tests/test_roms/nestest/nestest.nes",
        "../tests/test_roms/nestest/nestest.nes",
        "../../tests/test_roms/nestest/nestest.nes",
    };

    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return "";
}

} // namespace test_support