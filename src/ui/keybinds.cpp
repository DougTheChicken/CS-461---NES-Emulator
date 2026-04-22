#include "ui/keybinds.hpp"
#include <fstream>
#include <cstdio>

namespace {
    struct KeyName {
        const char* name;
        sf::Keyboard::Key key;
    };

    static const KeyName k_key_names[] = {
        {"A", sf::Keyboard::A}, {"B", sf::Keyboard::B}, {"C", sf::Keyboard::C},
        {"D", sf::Keyboard::D}, {"E", sf::Keyboard::E}, {"F", sf::Keyboard::F},
        {"G", sf::Keyboard::G}, {"H", sf::Keyboard::H}, {"I", sf::Keyboard::I},
        {"J", sf::Keyboard::J}, {"K", sf::Keyboard::K}, {"L", sf::Keyboard::L},
        {"M", sf::Keyboard::M}, {"N", sf::Keyboard::N}, {"O", sf::Keyboard::O},
        {"P", sf::Keyboard::P}, {"Q", sf::Keyboard::Q}, {"R", sf::Keyboard::R},
        {"S", sf::Keyboard::S}, {"T", sf::Keyboard::T}, {"U", sf::Keyboard::U},
        {"V", sf::Keyboard::V}, {"W", sf::Keyboard::W}, {"X", sf::Keyboard::X},
        {"Y", sf::Keyboard::Y}, {"Z", sf::Keyboard::Z},
        {"Num0", sf::Keyboard::Num0}, {"Num1", sf::Keyboard::Num1},
        {"Num2", sf::Keyboard::Num2}, {"Num3", sf::Keyboard::Num3},
        {"Num4", sf::Keyboard::Num4}, {"Num5", sf::Keyboard::Num5},
        {"Num6", sf::Keyboard::Num6}, {"Num7", sf::Keyboard::Num7},
        {"Num8", sf::Keyboard::Num8}, {"Num9", sf::Keyboard::Num9},
        {"Numpad0", sf::Keyboard::Numpad0}, {"Numpad1", sf::Keyboard::Numpad1},
        {"Numpad2", sf::Keyboard::Numpad2}, {"Numpad3", sf::Keyboard::Numpad3},
        {"Numpad4", sf::Keyboard::Numpad4}, {"Numpad5", sf::Keyboard::Numpad5},
        {"Numpad6", sf::Keyboard::Numpad6}, {"Numpad7", sf::Keyboard::Numpad7},
        {"Numpad8", sf::Keyboard::Numpad8}, {"Numpad9", sf::Keyboard::Numpad9},
        {"F1", sf::Keyboard::F1}, {"F2", sf::Keyboard::F2},
        {"F3", sf::Keyboard::F3}, {"F4", sf::Keyboard::F4},
        {"F5", sf::Keyboard::F5}, {"F6", sf::Keyboard::F6},
        {"F7", sf::Keyboard::F7}, {"F8", sf::Keyboard::F8},
        {"F9", sf::Keyboard::F9}, {"F10", sf::Keyboard::F10},
        {"F11", sf::Keyboard::F11}, {"F12", sf::Keyboard::F12},
        {"F13", sf::Keyboard::F13}, {"F14", sf::Keyboard::F14},
        {"F15", sf::Keyboard::F15},
        {"Up", sf::Keyboard::Up}, {"Down", sf::Keyboard::Down},
        {"Left", sf::Keyboard::Left}, {"Right", sf::Keyboard::Right},
        {"Home", sf::Keyboard::Home}, {"End", sf::Keyboard::End},
        {"PageUp", sf::Keyboard::PageUp}, {"PageDown", sf::Keyboard::PageDown},
        {"Insert", sf::Keyboard::Insert}, {"Delete", sf::Keyboard::Delete},
        {"LShift", sf::Keyboard::LShift}, {"RShift", sf::Keyboard::RShift},
        {"LControl", sf::Keyboard::LControl}, {"RControl", sf::Keyboard::RControl},
        {"LAlt", sf::Keyboard::LAlt}, {"RAlt", sf::Keyboard::RAlt},
        {"Space", sf::Keyboard::Space}, {"Return", sf::Keyboard::Return},
        {"BackSpace", sf::Keyboard::BackSpace}, {"Tab", sf::Keyboard::Tab},
        {"LBracket", sf::Keyboard::LBracket}, {"RBracket", sf::Keyboard::RBracket},
        {"SemiColon", sf::Keyboard::SemiColon}, {"Comma", sf::Keyboard::Comma},
        {"Period", sf::Keyboard::Period}, {"Quote", sf::Keyboard::Quote},
        {"Slash", sf::Keyboard::Slash}, {"BackSlash", sf::Keyboard::BackSlash},
        {"Tilde", sf::Keyboard::Tilde}, {"Equal", sf::Keyboard::Equal},
        {"Dash", sf::Keyboard::Dash},
        {"Escape", sf::Keyboard::Escape}, {"Pause", sf::Keyboard::Pause},
    };

    struct KeyField {
        const char* label;
        sf::Keyboard::Key* key;
        sf::Keyboard::Key default_key;
    };

// strips leading and trailing whitespace from a string
    static std::string trim(const std::string& s) {
        std::string::size_type a = s.find_first_not_of(" \t\r\n");
        std::string::size_type b;
        if (a == std::string::npos) return "";
        b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    static KeyField key_fields(ui::KeyBinds& kb, const ui::KeyBinds& defaults, int index) {
        switch (index) {
            case 0: return {"player1.up", &kb.p1.up, defaults.p1.up};
            case 1: return {"player1.down", &kb.p1.down, defaults.p1.down};
            case 2: return {"player1.left", &kb.p1.left, defaults.p1.left};
            case 3: return {"player1.right", &kb.p1.right, defaults.p1.right};
            case 4: return {"player1.a", &kb.p1.a, defaults.p1.a};
            case 5: return {"player1.b", &kb.p1.b, defaults.p1.b};
            case 6: return {"player1.select", &kb.p1.select, defaults.p1.select};
            case 7: return {"player1.start", &kb.p1.start, defaults.p1.start};
            case 8: return {"player2.up", &kb.p2.up, defaults.p2.up};
            case 9: return {"player2.down", &kb.p2.down, defaults.p2.down};
            case 10: return {"player2.left", &kb.p2.left, defaults.p2.left};
            case 11: return {"player2.right", &kb.p2.right, defaults.p2.right};
            case 12: return {"player2.a", &kb.p2.a, defaults.p2.a};
            case 13: return {"player2.b", &kb.p2.b, defaults.p2.b};
            case 14: return {"player2.select", &kb.p2.select, defaults.p2.select};
            case 15: return {"player2.start", &kb.p2.start, defaults.p2.start};
            case 16: return {"emulator.pause", &kb.emu.pause, defaults.emu.pause};
            case 17: return {"emulator.step_frame", &kb.emu.step_frame, defaults.emu.step_frame};
            case 18: return {"emulator.reset", &kb.emu.reset, defaults.emu.reset};
            case 19: return {"emulator.fullscreen", &kb.emu.fullscreen, defaults.emu.fullscreen};
            case 20: return {"emulator.load_rom", &kb.emu.load_rom, defaults.emu.load_rom};
            default: return {"emulator.quit", &kb.emu.quit, defaults.emu.quit};
        }
    }

    static void validate_bindings(ui::KeyBinds& kb) {
        const ui::KeyBinds defaults = ui::KeyBinds::defaults();
        const int field_count = 22;
        int i;
        int j;

        for (i = 0; i < field_count; ++i) {
            KeyField a = key_fields(kb, defaults, i);
            if (*a.key == sf::Keyboard::Unknown) {
                *a.key = a.default_key;
            }
        }

        for (i = 0; i < field_count; ++i) {
            KeyField a = key_fields(kb, defaults, i);
            for (j = i + 1; j < field_count; ++j) {
                KeyField b = key_fields(kb, defaults, j);
                if (*a.key != *b.key) continue;

                std::fprintf(stderr,
                             "[UI] Duplicate key binding for %s and %s (%s). Resetting %s to %s.\n",
                             a.label,
                             b.label,
                             ui::key_to_string(*a.key).c_str(),
                             b.label,
                             ui::key_to_string(b.default_key).c_str());
                *b.key = b.default_key;
            }
        }
    }
}

namespace ui {
    sf::Keyboard::Key key_from_string(const std::string& name) {
        size_t i;
        for (i = 0; i < sizeof(k_key_names) / sizeof(k_key_names[0]); ++i) {
            if (name == k_key_names[i].name) {
                return k_key_names[i].key;
            }
        }
        return sf::Keyboard::Unknown;
    }

    std::string key_to_string(sf::Keyboard::Key key) {
        size_t i;
        for (i = 0; i < sizeof(k_key_names) / sizeof(k_key_names[0]); ++i) {
            if (key == k_key_names[i].key) {
                return k_key_names[i].name;
            }
        }
        return "Unknown";
    }

    KeyBinds KeyBinds::defaults() {
        KeyBinds kb;
        kb.p1.up = sf::Keyboard::Up;
        kb.p1.down = sf::Keyboard::Down;
        kb.p1.left = sf::Keyboard::Left;
        kb.p1.right = sf::Keyboard::Right;
        kb.p1.a = sf::Keyboard::Z;
        kb.p1.b = sf::Keyboard::X;
        kb.p1.select = sf::Keyboard::RShift;
        kb.p1.start = sf::Keyboard::Return;

        kb.p2.up = sf::Keyboard::W;
        kb.p2.down = sf::Keyboard::S;
        kb.p2.left = sf::Keyboard::A;
        kb.p2.right = sf::Keyboard::D;
        kb.p2.a = sf::Keyboard::F;
        kb.p2.b = sf::Keyboard::G;
        kb.p2.select = sf::Keyboard::LShift;
        kb.p2.start = sf::Keyboard::Tab;

        kb.emu.pause = sf::Keyboard::Space;
        kb.emu.step_frame = sf::Keyboard::F2;
        kb.emu.reset = sf::Keyboard::R;
        kb.emu.fullscreen = sf::Keyboard::F11;
        kb.emu.load_rom = sf::Keyboard::O;
        kb.emu.quit = sf::Keyboard::Escape;
        return kb;
    }

// writes a fully-commented default keybinds.ini so the user has something to edit
    static void write_defaults(const std::string& path, const KeyBinds& kb) {
        std::ofstream f(path.c_str());
        if (!f.is_open()) return;

        f << "# NES Emulator key bindings\n"
          << "# Edit this file to remap controls, then restart the emulator.\n"
          << "# Duplicate keys are not allowed; conflicting entries fall back to defaults.\n"
          << "#\n"
          << "# Supported key names:\n"
          << "#   Letters : A B C ... Z\n"
          << "#   Numbers : Num0 Num1 ... Num9\n"
          << "#   Numpad  : Numpad0 Numpad1 ... Numpad9\n"
          << "#   Function: F1 F2 ... F15\n"
          << "#   Arrows  : Up Down Left Right\n"
          << "#   Special : Space Return BackSpace Tab Escape Pause\n"
          << "#             LShift RShift LControl RControl LAlt RAlt\n"
          << "#             LBracket RBracket SemiColon Comma Period\n"
          << "#             Quote Slash BackSlash Tilde Equal Dash\n"
          << "#             Home End PageUp PageDown Insert Delete\n"
          << "\n"
          << "[player1]\n"
          << "up     = " << key_to_string(kb.p1.up) << "\n"
          << "down   = " << key_to_string(kb.p1.down) << "\n"
          << "left   = " << key_to_string(kb.p1.left) << "\n"
          << "right  = " << key_to_string(kb.p1.right) << "\n"
          << "a      = " << key_to_string(kb.p1.a) << "\n"
          << "b      = " << key_to_string(kb.p1.b) << "\n"
          << "select = " << key_to_string(kb.p1.select) << "\n"
          << "start  = " << key_to_string(kb.p1.start) << "\n"
          << "\n"
          << "[player2]\n"
          << "up     = " << key_to_string(kb.p2.up) << "\n"
          << "down   = " << key_to_string(kb.p2.down) << "\n"
          << "left   = " << key_to_string(kb.p2.left) << "\n"
          << "right  = " << key_to_string(kb.p2.right) << "\n"
          << "a      = " << key_to_string(kb.p2.a) << "\n"
          << "b      = " << key_to_string(kb.p2.b) << "\n"
          << "select = " << key_to_string(kb.p2.select) << "\n"
          << "start  = " << key_to_string(kb.p2.start) << "\n"
          << "\n"
          << "[emulator]\n"
          << "pause      = " << key_to_string(kb.emu.pause) << "\n"
          << "step_frame = " << key_to_string(kb.emu.step_frame) << "\n"
          << "reset      = " << key_to_string(kb.emu.reset) << "\n"
          << "fullscreen = " << key_to_string(kb.emu.fullscreen) << "\n"
          << "load_rom   = " << key_to_string(kb.emu.load_rom) << "\n"
          << "quit       = " << key_to_string(kb.emu.quit) << "\n";
    }

    KeyBinds KeyBinds::load_or_create(const std::string& path) {
        KeyBinds kb = defaults();
        std::ifstream file(path.c_str());
        std::string line;
        std::string section;

        if (!file.is_open()) {
            write_defaults(path, kb);
            std::fprintf(stderr, "[UI] Created default keybinds: %s\n", path.c_str());
            return kb;
        }

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;

            if (line[0] == '[') {
                std::string::size_type end = line.find(']');
                if (end != std::string::npos) {
                    section = trim(line.substr(1, end - 1));
                }
                continue;
            }

            std::string::size_type eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string field = trim(line.substr(0, eq));
            sf::Keyboard::Key k = key_from_string(trim(line.substr(eq + 1)));
            if (k == sf::Keyboard::Unknown) {
                std::fprintf(stderr,
                             "[UI] Ignoring unknown key name for %s.%s\n",
                             section.c_str(),
                             field.c_str());
                continue;
            }

            if (section == "player1") {
                if      (field == "up")     kb.p1.up = k;
                else if (field == "down")   kb.p1.down = k;
                else if (field == "left")   kb.p1.left = k;
                else if (field == "right")  kb.p1.right = k;
                else if (field == "a")      kb.p1.a = k;
                else if (field == "b")      kb.p1.b = k;
                else if (field == "select") kb.p1.select = k;
                else if (field == "start")  kb.p1.start = k;
            } else if (section == "player2") {
                if      (field == "up")     kb.p2.up = k;
                else if (field == "down")   kb.p2.down = k;
                else if (field == "left")   kb.p2.left = k;
                else if (field == "right")  kb.p2.right = k;
                else if (field == "a")      kb.p2.a = k;
                else if (field == "b")      kb.p2.b = k;
                else if (field == "select") kb.p2.select = k;
                else if (field == "start")  kb.p2.start = k;
            } else if (section == "emulator") {
                if      (field == "pause")      kb.emu.pause = k;
                else if (field == "step_frame") kb.emu.step_frame = k;
                else if (field == "reset")      kb.emu.reset = k;
                else if (field == "fullscreen") kb.emu.fullscreen = k;
                else if (field == "load_rom")   kb.emu.load_rom = k;
                else if (field == "quit")       kb.emu.quit = k;
            }
        }

        validate_bindings(kb);
        std::fprintf(stderr, "[UI] Loaded keybinds: %s\n", path.c_str());
        return kb;
    }
}
