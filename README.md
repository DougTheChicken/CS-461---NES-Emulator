# CS-461---NES-Emulator
Capstone - Build an NES Emulator and Run NES ROMs

Proves our toolchain, repo health, and collaboration for Progress Report #1.

## Tech
- **Language:** C++20
- **Build:** CMake (+ Makefile shim)
- **Window/Input/Rendering:** SDL2
- **CI:** GitHub Actions (Linux/macOS/Windows)
- **Tests:** simple placeholder test binary

## Quick Start

### macOS
```bash
brew install sdl2 cmake
make build
make run
make test
```

### Ubuntu/Debian
```bash
sudo apt-get update && sudo apt-get install -y libsdl2-dev cmake g++
make build
make run
make test
```

### Windows
PowerShell:
```pwsh
choco install -y cmake sdl2
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config
./build/src/nes_emu.exe
./build/tests/tests.exe
```

## Workflow
- Feature branches: `feature/<short-desc>`
- PRs required; 1 reviewer; CI must pass
- Keep PRs small; link Issues

## Next
- 6502 opcode table + `CPU.step()`
- NES palette + PPU framebuffer
- iNES ROM loader
- Controller input mapping