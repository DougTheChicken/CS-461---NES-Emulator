# CS-461 NES Emulator
A cross-platform NES emulator built as part of the OSU CS-461/462 Capstone project.


## Current Status
The project is under active development. Core system scaffolding is in place, with CPU, memory, and ROM loading under implementation. Rendering and input are handled through SFML.

## Repository Structure
```text
src/nes/        Core emulator components (CPU, PPU, memory)
tests/          Test harness and validation ROMs
.github/        CI workflows
build/          Out-of-source build artifacts
```

## High-Level Architecture
- **CPU** – 6502 instruction execution and registers
- **Memory** – NES memory map and mirroring
- **PPU** – Graphics pipeline and framebuffer
- **ROM Loader** – iNES parsing and PRG/CHR mapping
- **Input** – NES controller state via SFML events


## Tech
- **Language:** C++20
- **Build:** CMake (+ Makefile shim)
- **Window/Input/Rendering:** SFML
- **CI:** GitHub Actions (Linux/macOS/Windows)
- **Tests:** basic test harness for build validation (functional tests in progress)

## Quick Start

### macOS
```bash
brew install cmake googletest sfml@2
make build
make run
make test
```

### Ubuntu/Debian
```bash
sudo apt-get update && sudo apt-get install -y cmake g++ libgtest-dev libgmock-dev libsfml-dev
sudo cmake -S /usr/src/googletest -B /tmp/build-gtest
sudo cmake --build /tmp/build-gtest --config Release
sudo cmake --install /tmp/build-gtest
make build
make run
make test
```

### Windows
PowerShell:
```pwsh
cd C:\
git clone https://github.com/microsoft/vcpkg.git
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install sfml:x64-windows gtest:x64-windows
C:\vcpkg\vcpkg integrate install
Switch to local project directory
choco install -y cmake
cmake -S . -B build `
  -G "Visual Studio 17 2022" -A x64 `
  -DBUILD_TESTING=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
./build/src/Release/nes_emu.exe
./build/tests/Release/tests.exe
```

## Running ROMs
ROM loading is under active development. At this stage, the emulator validates the toolchain and core system scaffolding but does not yet fully execute commercial NES ROMs.

## Controls

Default keyboard layout keeps the original single-player controls on player 1 and adds a second keyboard layout for player 2.

Player 1 uses the original controls:
- Movement: arrow keys
- `A` button: `Z`
- `B` button: `X`
- `Select`: `Right Shift`
- `Start`: `Return`

Player 2 uses the alternate controls:
- Movement: `W`, `A`, `S`, `D`
- `A` button: `F`
- `B` button: `G`
- `Select`: `Left Shift`
- `Start`: `Tab`

Emulator hotkeys:
- Pause/resume: `Space`
- Step one frame: `F2`
- Reset: `R`
- Toggle fullscreen: `F11`
- Load ROM: `O`
- Quit: `Escape`

## Changing Key Bindings

On first launch, the emulator creates `keybinds.ini` in the working directory. Edit that file and restart the emulator to apply changes.

The file has three sections:
- `[player1]` for controller 1
- `[player2]` for controller 2
- `[emulator]` for emulator hotkeys

Example:

```ini
[player1]
up     = Up
down   = Down
left   = Left
right  = Right
a      = Z
b      = X
select = RShift
start  = Return
```

Use SFML key names such as `A`, `Space`, `Return`, `LShift`, `Comma`, `Period`, `Up`, or `F11`.

Each action must have a unique key. If the same key is assigned to more than one action, the later conflicting entry is reset to its default binding when the file is loaded.


## Contributing
- Feature branches: `feature/<short-desc>`
- Pull requests required (1 reviewer minimum)
- CI must pass before merging
- Keep PRs small and link related issues


## Upcoming Work
- 6502 opcode table + `CPU.step()`
- NES palette + PPU framebuffer
- Basic User Interface
