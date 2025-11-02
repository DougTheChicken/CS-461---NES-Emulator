BUILD_DIR := build
.PHONY: all build run test clean
all: build
build:
	cmake -S . -B $(BUILD_DIR) -DBUILD_TESTING=ON
	cmake --build $(BUILD_DIR) -j
run: build
	@if [ -x "$(BUILD_DIR)/src/nes_emu" ]; then \
	  "$(BUILD_DIR)/src/nes_emu"; \
	elif [ -x "$(BUILD_DIR)/nes_emu" ]; then \
	  "$(BUILD_DIR)/nes_emu"; \
	elif [ -x "$(BUILD_DIR)/Release/nes_emu.exe" ]; then \
	  "$(BUILD_DIR)/Release/nes_emu.exe"; \
	else \
	  echo "nes_emu binary not found."; exit 1; \
	fi
test: build
	@if [ -x "$(BUILD_DIR)/tests/tests" ]; then \
	  "$(BUILD_DIR)/tests/tests"; \
	elif [ -x "$(BUILD_DIR)/Release/tests.exe" ]; then \
	  "$(BUILD_DIR)/Release/tests.exe"; \
	else \
	  echo "tests binary not found. Try: cmake -S . -B build -DBUILD_TESTING=ON"; exit 1; \
	fi
clean:
	rm -rf $(BUILD_DIR)
