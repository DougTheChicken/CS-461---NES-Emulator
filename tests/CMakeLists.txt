
find_package(GTest CONFIG REQUIRED)


add_executable(opcode_tests
        cpu/opcode_tests.cpp
)

target_link_libraries(opcode_tests
        PRIVATE
        nes_lib
        GTest::gtest_main
)

add_executable(mem_tests "mem/mem_test.cpp")

target_link_libraries(mem_tests
        PRIVATE
        nes_lib
)

add_executable(ROM_tests "ROM/ROM_tests.cpp")

target_link_libraries(ROM_tests
        PRIVATE
        nes_lib
        GTest::gtest_main
)

# wherever you define nes_lib (or your main target)
target_include_directories(nes_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(opcode_tests)

add_executable(tests test_sanity.cpp)
