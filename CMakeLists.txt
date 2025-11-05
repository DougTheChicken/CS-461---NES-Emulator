cmake_minimum_required(VERSION 3.20)
project(NES_Emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- GoogleTest (imported targets) ----
find_package(GTest CONFIG REQUIRED)

# ---- SDL2: prefer CONFIG, fallback to pkg-config ----
set(SDL2_MODE "")
find_package(SDL2 CONFIG QUIET)
if (SDL2_FOUND)
    set(SDL2_MODE "CONFIG")  # provides SDL2::SDL2, SDL2::SDL2main (on Windows)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    set(SDL2_MODE "PKG")
    # Create a compatible imported target so the rest of the file is uniform.
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})
    if (WIN32 AND SDL2_LINK_LIBRARIES)
        # If pkg-config exposes SDL2main too
        add_library(SDL2::SDL2main INTERFACE IMPORTED)
        target_link_libraries(SDL2::SDL2main INTERFACE SDL2main)
    endif()
endif()
message(STATUS "SDL2 mode: ${SDL2_MODE}")

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

# ---- Library: all sources under src/, EXCEPT main.cpp ----
file(GLOB_RECURSE NES_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(FILTER NES_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(nes_lib ${NES_SOURCES})
target_include_directories(nes_lib
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ---- Executable with main.cpp; link SDL only here ----
add_executable(nes_emu ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(nes_emu PRIVATE nes_lib SDL2::SDL2)

if (WIN32)
    # Many SDL2 configs want SDL2main on Windows
    if (TARGET SDL2::SDL2main)
        target_link_libraries(nes_emu PRIVATE SDL2::SDL2main)
    endif()
endif()

# ---- Tests ----
option(BUILD_TESTING "Build tests" ON)
if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)  # your tests CMake should link nes_lib and GTest::gtest_main
endif()