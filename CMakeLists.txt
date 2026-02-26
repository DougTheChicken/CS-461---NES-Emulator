cmake_minimum_required(VERSION 3.20)
project(NES_Emulator LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- GoogleTest (imported targets) ----
find_package(GTest CONFIG REQUIRED)

if(APPLE)
    # Prefer Homebrew keg-only sfml@2 if present (Apple Silicon or Intel)
    foreach(_sfml_prefix
            "/opt/homebrew/opt/sfml@2"
            "/usr/local/opt/sfml@2")
        if(EXISTS "${_sfml_prefix}/lib/cmake/SFML/SFMLConfig.cmake")
            list(PREPEND CMAKE_PREFIX_PATH "${_sfml_prefix}")
            break()
        endif()
    endforeach()

    # ---- macOS SDK convenience (optional) ----
    execute_process(COMMAND xcrun --show-sdk-path
            OUTPUT_VARIABLE SDK_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(SDK_PATH)
        add_compile_options(-isysroot "${SDK_PATH}")
        add_link_options(-isysroot "${SDK_PATH}")
    endif()
endif()


# ---- SFML2 ----
# Ask for SFML 2.5+ (allows Homebrew sfml@2 2.6.x), prefer CONFIG mode
find_package(SFML 2.5 CONFIG QUIET COMPONENTS graphics window system)
if(NOT SFML_FOUND)
    find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
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
include(FetchContent)

# Fetch nativefiledialog (NFD) so <nfd.h> and the library are available
FetchContent_Declare(
    nfd
    GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(nfd)

# Make NFD sources available to build: include headers and compile zenity backend
set(NFD_SRC_DIR ${nfd_SOURCE_DIR}/src)
set(NFD_SOURCES
        ${NFD_SRC_DIR}/nfd_common.c
        ${NFD_SRC_DIR}/nfd_zenity.c
)

target_include_directories(nes_ui PUBLIC ${NFD_SRC_DIR}/include)
target_sources(nes_ui PRIVATE ${NFD_SOURCES})

# Silence warning only for NFD sources (GNU/Clang only)
set_source_files_properties(${NFD_SOURCES} PROPERTIES
        COMPILE_FLAGS "$<$<C_COMPILER_ID:GNU,Clang>:-Wno-format-truncation>"
)

# Link UI target with SFML, nes_lib and gtest
target_link_libraries(nes_ui PUBLIC sfml-graphics sfml-window sfml-system nes_lib GTest::gtest_main GTest::gtest)
if(TGUI_FOUND)
    target_link_libraries(nes_ui PUBLIC TGUI::TGUI)
endif()

# ---- Executable with main.cpp; link SDL, and UI ----
add_executable(nes_emu ${PROJECT_SOURCE_DIR}/src/main.cpp
        include/nes/ppu.hpp
        include/nes/component.hpp
        include/nes/timing.hpp
        include/nes/apu.hpp
        src/nes/ROM.cpp
        src/nes/apu.cpp
        src/nes/ppu.cpp
)
target_link_libraries(nes_emu PRIVATE nes_lib nes_ui)

# ---- Tests ----
option(BUILD_TESTING "Build tests" ON)
if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)  # your tests CMake should link nes_lib and GTest::gtest_main
endif()