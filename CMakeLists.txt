cmake_minimum_required(VERSION 3.20)
project(NES_Emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- GoogleTest (imported targets) ----
find_package(GTest CONFIG REQUIRED)

# ---- SFML2 ----
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# ---- macOS SDK convenience (optional) ----
if(APPLE)
    execute_process(COMMAND xcrun --show-sdk-path
            OUTPUT_VARIABLE SDK_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(SDK_PATH)
        add_compile_options(-isysroot "${SDK_PATH}")
        add_link_options(-isysroot "${SDK_PATH}")
    endif()
endif()

# ---- Library: all sources under src/, EXCEPT main.cpp and ui/ ----
file(GLOB_RECURSE NES_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(FILTER NES_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
list(FILTER NES_SOURCES EXCLUDE REGEX ".*/ui/.*\\.cpp$")

add_library(nes_lib ${NES_SOURCES})
target_include_directories(nes_lib
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ---- UI Library ----
add_library(nes_ui STATIC
    ${PROJECT_SOURCE_DIR}/src/ui/ui.cpp
)
target_include_directories(nes_ui
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(nes_ui PUBLIC sfml-graphics sfml-window sfml-system)

# ---- Executable with main.cpp; link SDL, and UI ----
add_executable(nes_emu ${PROJECT_SOURCE_DIR}/src/main.cpp
        include/nes/ppu.hpp
        include/nes/component.hpp
        include/nes/timing.hpp
        include/nes/apu.hpp
        src/nes/ROM.cpp
        src/nes/apu.cpp
)
target_link_libraries(nes_emu PRIVATE nes_lib nes_ui)

# ---- Tests ----
option(BUILD_TESTING "Build tests" ON)
if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)  # your tests CMake should link nes_lib and GTest::gtest_main
endif()