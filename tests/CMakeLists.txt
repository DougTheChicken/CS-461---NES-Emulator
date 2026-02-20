
find_package(GTest CONFIG REQUIRED)


add_executable(opcode_tests
        cpu/opcode_tests.cpp
)
target_link_libraries(opcode_tests PRIVATE nes_lib GTest::gtest_main)

add_executable(ROM_tests
        ROM/ROM_tests.cpp
)
target_link_libraries(ROM_tests PRIVATE nes_lib GTest::gtest_main)

add_executable(mem_tests
        mem/mem_test.cpp
)
target_link_libraries(mem_tests PRIVATE nes_lib)
add_test(NAME mem_tests COMMAND mem_tests)

add_executable(tests
        test_sanity.cpp
)
target_link_libraries(tests PRIVATE nes_lib)
add_test(NAME sanity_tests COMMAND tests)

include(GoogleTest)
gtest_discover_tests(opcode_tests)
gtest_discover_tests(ROM_tests)