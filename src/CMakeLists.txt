# List sources explicitly (recommended), or use GLOB for speed while prototyping.
file(GLOB_RECURSE NES_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(nes_emu ${NES_SOURCES}
        main.cpp)

target_include_directories(nes_emu
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
)

target_compile_options(nes_emu PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_libraries(nes_emu PRIVATE ${SDL2_LINK_LIBRARIES})
