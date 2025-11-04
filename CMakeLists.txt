cmake_minimum_required(VERSION 3.20)

project(NES_Emulator LANGUAGES CXX)

# ---- Cross-platform SDL2 discovery ----
set(SDL2_MODE "")

# Try CONFIG mode first (works with vcpkg and Homebrew's SDL2)
find_package(SDL2 CONFIG QUIET)

if (SDL2_FOUND)
    set(SDL2_MODE "CONFIG")
else()
    # Fallback to pkg-config (apt/brew)
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules(SDL2 REQUIRED sdl2)
        set(SDL2_MODE "PKG")
    else()
        message(FATAL_ERROR "SDL2 not found via CONFIG or pkg-config.")
    endif()
endif()

message(STATUS "SDL2 mode: ${SDL2_MODE}")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS: ensure SDK headers are visible to the compiler and CLion
if(APPLE)
    execute_process(COMMAND xcrun --show-sdk-path
            OUTPUT_VARIABLE SDK_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(SDK_PATH)
        set(CMAKE_OSX_SYSROOT "${SDK_PATH}")
        add_compile_options(-isysroot "${SDK_PATH}")
        add_link_options(-isysroot "${SDK_PATH}")
    endif()
endif()

if(WIN32)
    include_directories(${SDL2_INCLUDE_DIR})
    link_directories(${SDL2_LIB_DIR})

    set(SDL2_LIBS SDL2 SDL2main)

else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

add_subdirectory(src)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
