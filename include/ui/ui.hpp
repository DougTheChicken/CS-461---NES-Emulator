#pragma once

#include <cstdint>
#include <SFML/Graphics.hpp>
#include <nfd.h>
#include <memory>

class console;

namespace ui {
    // nes controller bitmask
    enum NESButton : uint8_t {
        A      = 0x01,
        B      = 0x02,
        SELECT = 0x04,
        START  = 0x08,
        UP     = 0x10,
        DOWN   = 0x20,
        LEFT   = 0x40,
        RIGHT  = 0x80
    };

    // Initializes the UI subsystem. Returns true on success, false on failure
    bool init();

    // Initializes audio playback. Should be called after loading a ROM
    bool init_audio(console& emu);

    // handle mapping logic for controler input
    void process_input(sf::Event& event, console& emu);

    // Steps the UI, processing events and rendering. Returns false if the user requested to exit
    bool step(console& emu, bool& is_running);

    // Shuts down the UI subsystem, cleaning up resources
    void shutdown();
}