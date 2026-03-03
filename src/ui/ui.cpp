#include "ui/ui.hpp"
#include "nes/console.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>

namespace {
    sf::RenderWindow* g_window = nullptr;
    sf::Texture g_texture;
    sf::Sprite g_sprite;
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
    
    bool step(console& emu, bool& is_running) {
        if (!::g_window || !::g_window->isOpen()) {
            return false;
        }
        
        // Process events
        sf::Event event;
        while (::g_window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                is_running = false;
                ::g_window->close();
                return false;
            }
            // Handle keyboard input for controlling the emulator
            // Space - toggle pause/resume
            // S - step one frame
            // R - reset emulator
            // F - toggle fullscreen
            // L - load ROM
            auto tppe = event.type;
            if (event.type == sf::Event::KeyPressed){
                switch (event.key.code)
                {
                    case sf::Keyboard::Escape:
                        is_running = false;
                        ::g_window->close();
                        return false;
                        break;
                    case sf::Keyboard::Space:
                        if (emu.rom_loaded()) {
                            if (!is_running) {
                                std::fprintf(stderr, "Resuming emulation\n");
                            } else {
                                std::fprintf(stderr, "Pausing emulation\n");
                            }
                            is_running = !is_running;
                        }
                        break;
                    case sf::Keyboard::S:
                        if (emu.rom_loaded()) {
                            std::fprintf(stderr, "Stepping one frame\n");
                            emu.step(1);
                        }
                        break;
                    case sf::Keyboard::R:
                        if (emu.rom_loaded()) {
                            std::fprintf(stderr, "Resetting emulator\n");
                            emu.reset_all();
                            is_running = false;
                        }
                        break;
                    case sf::Keyboard::F:
                        if (emu.rom_loaded()) {
                            std::fprintf(stderr, "Toggling fullscreen\n");
                            if (::g_fullscreen) {
                                ::g_window->create(sf::VideoMode(FB_WIDTH * SCALE, FB_HEIGHT * SCALE), "NES Emulator", sf::Style::Default);
                            } else {
                                ::g_window->create(sf::VideoMode::getDesktopMode(), "NES Emulator", sf::Style::Fullscreen);
                            }
                            // Reapply settings for the new window
                            ::g_window->setFramerateLimit(60);
                            ::g_fullscreen = !::g_fullscreen;
                            update_view();
                        }
                        break;
                    case sf::Keyboard::L:{
                        if (emu.rom_loaded()) {
                            // unload rom
                            std::fprintf(stderr, "Unloading current ROM\n");
                            emu.reset_all();
                            is_running = false;
                        }
                        // add file select menu
                        std::string filePath = openFileDialog();
                        if (!filePath.empty()) {
                            std::fprintf(stderr, "Loading ROM: %s\n", filePath.c_str());
                            emu.load_rom(filePath.c_str());
                            is_running = true;
                        }
                    }
                        break;
                    default:
                        break;
                }
            }
        }

        uint8_t buttons = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) buttons |= 0x80;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  buttons |= 0x40;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  buttons |= 0x20;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    buttons |= 0x10;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Return)) buttons |= 0x08; // Start
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) buttons |= 0x04; // Select
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))     buttons |= 0x02;  // B
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))     buttons |= 0x01;  // A
        emu.set_controller1(buttons);

        // TODO: map some buttons to controller2

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
        // TODO: Clean up any additional resources if needed
        if (::g_window) {
            ::g_window->close();
            delete ::g_window;
            ::g_window = nullptr;
        }
    }
}
