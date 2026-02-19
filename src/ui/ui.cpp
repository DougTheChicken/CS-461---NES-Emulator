#include "ui/ui.hpp"
#include "nes/console.hpp"
#include <SFML/Graphics.hpp>

namespace {
    sf::RenderWindow* g_window = nullptr;
    sf::Texture g_texture;
    sf::Sprite g_sprite;
    
    constexpr unsigned int SCALE = 3;  // Scale up the 256x240 resolution since it's so small
    constexpr unsigned int FB_WIDTH = 256;
    constexpr unsigned int FB_HEIGHT = 240;
}

namespace ui {
    bool init() {
        try {
            ::g_window = new sf::RenderWindow(
                sf::VideoMode(FB_WIDTH * SCALE, FB_HEIGHT * SCALE),
                "NES Emulator"
            );
            
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
            ::g_sprite.setScale(SCALE, SCALE);
            
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
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                is_running = false;
                ::g_window->close();
                return false;
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
        // TODO: Clean up any additional resources if needed
        if (::g_window) {
            ::g_window->close();
            delete ::g_window;
            ::g_window = nullptr;
        }
    }
}
