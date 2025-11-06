#include <SDL.h>
#include <cstdio>
#include <string>

constexpr int NES_WIDTH = 256;
constexpr int NES_HEIGHT = 240;
constexpr int SCALE = 3;
static void sdlFail(const std::string& step) {
    std::fprintf(stderr, "SDL error at %s: %s\n", step.c_str(), SDL_GetError());
    std::fflush(stderr);
    std::exit(1);
}
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) sdlFail("SDL_Init");
    SDL_Window* window = SDL_CreateWindow("CS 461 NES Emulator - Enjoy the nostalgia!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        NES_WIDTH * SCALE, NES_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (!window) sdlFail("SDL_CreateWindow");
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) sdlFail("SDL_CreateRenderer");
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, NES_WIDTH, NES_HEIGHT);
    if (!texture) sdlFail("SDL_CreateTexture");
    bool running = true; uint32_t frame = 0;
    while (running) {
        SDL_Event e; while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }
        void* pixels = nullptr; int pitch = 0;
        if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) sdlFail("SDL_LockTexture");
        for (int y = 0; y < NES_HEIGHT; ++y) {
            auto* px = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pixels) + y * pitch);
            for (int x = 0; x < NES_WIDTH; ++x) {
                uint8_t r = static_cast<uint8_t>((x + frame) % 256);
                uint8_t g = static_cast<uint8_t>((y * 2) % 256);
                uint8_t b = static_cast<uint8_t>(((x ^ y) + frame) % 256);
                uint32_t argb = (0xFFu << 24) | (r << 16) | (g << 8) | b;
                px[x] = argb;
            }
        }
        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer); SDL_RenderCopy(renderer, texture, nullptr, nullptr); SDL_RenderPresent(renderer);
        SDL_Delay(16); frame++;
    }
    SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
