BUILD_DIR := build
.PHONY: all build run test clean

all: build

build:
	cmake -S . -B $(BUILD_DIR) -DBUILD_TESTING=ON
	cmake --build $(BUILD_DIR) -j

run: build
	@if [ -x "$(BUILD_DIR)/nes_emu" ]; then \
	  "$(BUILD_DIR)/nes_emu"; \
	elif [ -x "$(BUILD_DIR)/src/nes_emu" ]; then \
	  "$(BUILD_DIR)/src/nes_emu"; \
	elif [ -x "$(BUILD_DIR)/Release/nes_emu.exe" ]; then \
	  "$(BUILD_DIR)/Release/nes_emu.exe"; \
	else \
	  echo "nes_emu binary not found."; exit 1; \
	fi

# Run the FULL test suite registered with CTest (includes gtest_discover_tests).
test: build
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf $(BUILD_DIR)