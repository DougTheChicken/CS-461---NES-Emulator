#include "ui/ui.hpp"
#include "ui/audio.hpp"
#include "ui/keybinds.hpp"
#include "nes/console.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>

namespace {
    sf::RenderWindow* g_window = nullptr;
    ui::KeyBinds g_keybinds = ui::KeyBinds::defaults(); // loaded from keybinds.ini on init()
    sf::Texture g_texture;
    sf::Sprite g_sprite;
    std::unique_ptr<ui::AudioStream> g_audioStream = nullptr;
    bool g_fullscreen = false;
    
    constexpr unsigned int SCALE = 3;  // Scale up the 256x240 resolution since it's so small
    constexpr unsigned int FB_WIDTH = 256;
    constexpr unsigned int FB_HEIGHT = 240;

    // Update sprite scale and position based on current window size
    static void update_view() {
        if (!::g_window) return;
        sf::Vector2u winSize = ::g_window->getSize();

        // Compute scale to fit framebuffer into window while preserving aspect ratio
        float scaleX = static_cast<float>(winSize.x) / static_cast<float>(FB_WIDTH);
        float scaleY = static_cast<float>(winSize.y) / static_cast<float>(FB_HEIGHT);

        float scale = ::g_fullscreen ? std::min(scaleX, scaleY) : static_cast<float>(SCALE);

        ::g_sprite.setScale(scale, scale);

        // Center the sprite in the window
        float offsetX = (static_cast<float>(winSize.x) - FB_WIDTH * scale) / 2.0f;
        float offsetY = (static_cast<float>(winSize.y) - FB_HEIGHT * scale) / 2.0f;
        ::g_sprite.setPosition(offsetX, offsetY);

        // Ensure the view covers the whole window
        ::g_window->setView(sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(winSize.x), static_cast<float>(winSize.y))));
    }

    static uint8_t button_for_key(const ui::PlayerKeys& keys, sf::Keyboard::Key key) {
        if (key == keys.up) return ui::UP;
        if (key == keys.down) return ui::DOWN;
        if (key == keys.left) return ui::LEFT;
        if (key == keys.right) return ui::RIGHT;
        if (key == keys.a) return ui::A;
        if (key == keys.b) return ui::B;
        if (key == keys.select) return ui::SELECT;
        if (key == keys.start) return ui::START;
        return 0;
    }

    static void update_controller_state(console& emu, int player, uint8_t mask, bool pressed) {
        uint8_t state;
        if (mask == 0) return;

        if (player == 1) {
            state = emu.get_mem().get_controller1();
            state = pressed ? static_cast<uint8_t>(state | mask)
                            : static_cast<uint8_t>(state & static_cast<uint8_t>(~mask));
            emu.get_mem().set_controller1(state);
        } else {
            state = emu.get_mem().get_controller2();
            state = pressed ? static_cast<uint8_t>(state | mask)
                            : static_cast<uint8_t>(state & static_cast<uint8_t>(~mask));
            emu.get_mem().set_controller2(state);
        }
    }
}

namespace ui {

    std::string openFileDialog()
    {
        nfdchar_t* outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog(nullptr, nullptr, &outPath);

        if (result == NFD_OKAY)
        {
            std::string path = outPath;
            free(outPath);
            return path;
        }
        else if (result == NFD_CANCEL)
        {
            printf("User pressed cancel.\n");
        }
        else
        {
            printf("Error: %s\n", NFD_GetError());
        }

        return "";
    }

    bool init() {
        // load keybinds from file; creates keybinds.ini with defaults if missing
        ::g_keybinds = ui::KeyBinds::load_or_create("keybinds.ini");
        try {
            ::g_window = new sf::RenderWindow(
                sf::VideoMode(FB_WIDTH * SCALE, FB_HEIGHT * SCALE),
                "NES Emulator"
            );
            ::g_fullscreen = false;
            
            if (!::g_window->isOpen()) {
                delete ::g_window;
                ::g_window = nullptr;
                return false;
            }
            
            // Create texture for framebuffer
            if (!::g_texture.create(FB_WIDTH, FB_HEIGHT)) {
                ::g_window->close();
                delete ::g_window;
                ::g_window = nullptr;
                return false;
            }
            
            // Setup sprite
            ::g_sprite.setTexture(::g_texture);
            update_view();
            
            ::g_window->setFramerateLimit(60);
            
            return true;
        } catch (...) {
            return false;
        }
    }

    bool init_audio(console& emu) {
        try {
            if (::g_audioStream) {
                ::g_audioStream->stop();
                ::g_audioStream.reset();
            }

            ::g_audioStream = std::make_unique<AudioStream>(emu);
            if (!::g_audioStream->initialize(44100)) {
                std::fprintf(stderr, "[UI] Failed to initialize audio stream\n");
                ::g_audioStream.reset();
                return false;
            }

            ::g_audioStream->play();
            std::fprintf(stderr, "[UI] Audio initialized and playback started\n");
            return true;
        } catch (...) {
            std::fprintf(stderr, "[UI] Exception during audio initialization\n");
            return false;
        }
    }

    void process_input(sf::Event& event, console& emu) {
        if (event.type != sf::Event::KeyPressed && event.type != sf::Event::KeyReleased)
            return;

        bool pressed = (event.type == sf::Event::KeyPressed);
        sf::Keyboard::Key k = event.key.code;
        uint8_t p1_mask = button_for_key(::g_keybinds.p1, k);
        uint8_t p2_mask = button_for_key(::g_keybinds.p2, k);

        update_controller_state(emu, 1, p1_mask, pressed);
        update_controller_state(emu, 2, p2_mask, pressed);
    }
    
    bool step(console& emu, bool& is_running) {
        if (!::g_window || !::g_window->isOpen()) {
            return false;
        }
        
        // Process events
        sf::Event event;
        while (::g_window->pollEvent(event)) {
            // call controller
            process_input(event, emu);

            if (event.type == sf::Event::Closed) {
                is_running = false;
                ::g_window->close();
                return false;
            }
            if (event.type == sf::Event::KeyPressed) {
                const ui::EmuKeys& ek = ::g_keybinds.emu; // emulator hotkeys loaded from keybinds.ini
                sf::Keyboard::Key k = event.key.code;

                if (k == ek.quit) {
                    is_running = false;
                    ::g_window->close();
                    return false;
                } else if (k == ek.pause && emu.rom_loaded()) {
                    is_running = !is_running;
                    std::fprintf(stderr, is_running ? "Resuming emulation\n" : "Pausing emulation\n");
                } else if (k == ek.step_frame && emu.rom_loaded()) {
                    std::fprintf(stderr, "Stepping one frame\n");
                    emu.step(1);
                } else if (k == ek.reset && emu.rom_loaded()) {
                    std::fprintf(stderr, "Resetting emulator\n");
                    emu.reset_all();
                    is_running = false;
                } else if (k == ek.fullscreen && emu.rom_loaded()) {
                    std::fprintf(stderr, "Toggling fullscreen\n");
                    if (::g_fullscreen)
                        ::g_window->create(sf::VideoMode(FB_WIDTH * SCALE, FB_HEIGHT * SCALE), "NES Emulator", sf::Style::Default);
                    else
                        ::g_window->create(sf::VideoMode::getDesktopMode(), "NES Emulator", sf::Style::Fullscreen);
                    ::g_window->setFramerateLimit(60);
                    ::g_fullscreen = !::g_fullscreen;
                    update_view();
                } else if (k == ek.load_rom) {
                    if (emu.rom_loaded()) {
                        std::fprintf(stderr, "Unloading current ROM\n");
                        emu.reset_all();
                        is_running = false;
                    }
                    std::string filePath = openFileDialog();
                    if (!filePath.empty()) {
                        std::fprintf(stderr, "Loading ROM: %s\n", filePath.c_str());
                        emu.load_rom(filePath.c_str());
                        init_audio(emu);
                        is_running = true;
                    }
                }
            }
        }
        
        // Update texture from framebuffer
        const uint32_t* framebuffer = emu.framebuffer();
        ::g_texture.update((const uint8_t*)framebuffer);
        
        // Clear and render
        ::g_window->clear(sf::Color::Black);
        ::g_window->draw(::g_sprite);
        ::g_window->display();
        
        return true;
    }
    
    void shutdown() {
        // Stop and cleanup audio
        if (::g_audioStream) {
            ::g_audioStream->stop();
            ::g_audioStream.reset();
        }

        // TODO: Clean up any additional resources if needed
        if (::g_window) {
            ::g_window->close();
            delete ::g_window;
            ::g_window = nullptr;
        }
    }
}
