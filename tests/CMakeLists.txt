
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(opcode_tests
        cpu/opcode_tests.cpp
)
target_link_libraries(opcode_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(opcode_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(cpu_tests
        cpu/cpu_tests.cpp
)
target_link_libraries(cpu_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(cpu_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(ROM_tests
        ROM/ROM_tests.cpp
)
target_link_libraries(ROM_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(ROM_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(console_tests
        console/console_tests.cpp
)
target_link_libraries(console_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(console_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mapper_000_tests
        mapper/mapper_000_tests.cpp
)
target_link_libraries(mapper_000_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mapper_000_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mapper_001_tests
        mapper/mapper_001_tests.cpp
)
target_link_libraries(mapper_001_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mapper_001_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mapper_002_tests
        mapper/mapper_002_tests.cpp
)
target_link_libraries(mapper_002_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mapper_002_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mapper_003_tests
        mapper/mapper_003_tests.cpp
)
target_link_libraries(mapper_003_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mapper_003_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mapper_004_tests
        mapper/mapper_004_tests.cpp
)
target_link_libraries(mapper_004_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mapper_004_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mapper_005_tests
        mapper/mapper_005_tests.cpp
)
target_link_libraries(mapper_005_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mapper_005_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mapper_007_tests
        mapper/mapper_007_tests.cpp
)
target_link_libraries(mapper_007_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mapper_007_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(mem_tests
        mem/mem_test.cpp
)
target_link_libraries(mem_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(mem_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)


add_executable(ppu_tests
        ppu/ppu_test.cpp
)
target_link_libraries(ppu_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(ppu_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(apu_tests
        apu/apu_test.cpp
)
target_link_libraries(apu_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(apu_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(timing_tests
        timing/timing_tests.cpp
)
target_link_libraries(timing_tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(timing_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(ui_tests
        ui/ui_tests.cpp
)
target_link_libraries(ui_tests PRIVATE nes_lib nes_ui GTest::gtest_main)
target_include_directories(ui_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(audio_tests
        ui/audio_tests.cpp
)
target_link_libraries(audio_tests PRIVATE nes_lib nes_ui GTest::gtest_main)
target_include_directories(audio_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

add_executable(tests
        test_sanity.cpp
)
target_link_libraries(tests PRIVATE nes_lib GTest::gtest_main)
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)

gtest_discover_tests(opcode_tests)
gtest_discover_tests(cpu_tests)
gtest_discover_tests(ROM_tests)
gtest_discover_tests(console_tests)
gtest_discover_tests(mapper_000_tests)
gtest_discover_tests(mapper_001_tests)
gtest_discover_tests(mapper_002_tests)
gtest_discover_tests(mapper_003_tests)
gtest_discover_tests(mapper_004_tests)
gtest_discover_tests(mapper_005_tests)
gtest_discover_tests(mapper_007_tests)
gtest_discover_tests(mem_tests)
gtest_discover_tests(ppu_tests)
gtest_discover_tests(apu_tests)
gtest_discover_tests(timing_tests)
gtest_discover_tests(ui_tests)
gtest_discover_tests(audio_tests)
gtest_discover_tests(tests)