#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <string>

namespace ui {
    // one controller's worth of key bindings
    struct PlayerKeys {
        sf::Keyboard::Key up, down, left, right;
        sf::Keyboard::Key a, b, select, start;
    };

    // emulator-level hotkeys (not sent to the NES)
    struct EmuKeys {
        sf::Keyboard::Key pause, step_frame, reset, fullscreen, load_rom, quit;
    };

    struct KeyBinds {
        PlayerKeys p1, p2; // p1 = left side of keyboard, p2 = right side
        EmuKeys emu;

        // returns the built-in default layout
        static KeyBinds defaults();
        // loads from an INI file; creates the file with defaults if it doesn't exist
        static KeyBinds load_or_create(const std::string& path);
    };

    // converts a key name string (e.g. "LShift") to the matching SFML key code
    sf::Keyboard::Key key_from_string(const std::string& name);
    // converts an SFML key code back to its name string for writing to the config file
    std::string key_to_string(sf::Keyboard::Key key);
}
