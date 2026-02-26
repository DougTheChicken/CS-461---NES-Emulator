#include <iostream>
#include <cassert>
#include "../../include/nes/mem.hpp"
#include "../../include/nes/ppu.hpp"
#include "../../include/nes/apu.hpp"

// A helper to print "PASS" in green
void log_pass(const char *test_name) {
    std::cout << "\033[1;32m[PASS] " << test_name << "\033[0m" << std::endl;
}

void test_ppu_reset() {
    nes::PPU ppu;
    nes::APU apu;
    nes::Memory mem(ppu, apu);
    ppu.reset();
    assert(ppu.oam_address == 0);
    assert(ppu.base_nametable_select == 0);
    assert(ppu.vram_address_increment_flag == false);
    assert(ppu.sprite_pattern_table_address_flag == false);
    assert(ppu.background_pattern_table_address_flag == false);
    assert(ppu.sprite_size_flag == false);
    assert(ppu.ppu_master_slave_flag == false);
    assert(ppu.vblank_nmi_flag == false);
    assert(ppu.greyscale_flag == false);
    assert(ppu.leftmost_background_flag == false);
    assert(ppu.leftmost_sprite_flag == false);
    assert(ppu.background_rendering_flag == false);
    assert(ppu.sprite_rendering_flag == false);
    assert(ppu.emphasize_red == false);
    assert(ppu.emphasize_green == false);
    assert(ppu.emphasize_blue == false);
    assert(ppu.sprite_overflow_flag == false);
    assert(ppu.sprite_zero_hit_flag == false);
    assert(ppu.vblank_flag == false);
    assert(ppu.v == 0);
    assert(ppu.t == 0);
    assert(ppu.x == 0);
    assert(ppu.w == false);
    assert(ppu.data_buffer == 0);
    assert(ppu.oam_dma_page == 0);
    assert(ppu.nmi_pending == false);
    assert(ppu.nmi_prev == false);
    assert(ppu.frame_complete == false);
    assert(ppu.vertical_mirroring == false);
    assert(ppu.open_bus == 0);
    assert(ppu.timing.odd_frame() == false);
    assert(ppu.timing.scanline() == -1);
    assert(ppu.timing.cycle() == 0);
    assert(ppu.sprites.overflow() == false);
    // TODO: consider adding accessors to improve testability to sprite and background pipelines

    log_pass("PPU Reset");
}

static void test_sprite_hflip() {
    std::printf("\n[SpritePipeline horizontal flip]\n");

    assert(nes::SpritePipeline::maybe_flipped_h(0x00, 0b10110001) == 0b10110001);
    log_pass("no-flip attribute returns byte unchanged");

    assert(nes::SpritePipeline::maybe_flipped_h(0x40, 0b10110001) == (uint8_t)0b10001101);
    log_pass("flip attribute reverses all 8 bits");

    assert(nes::SpritePipeline::maybe_flipped_h(0x40, 0x00) == 0x00);
    log_pass("flipping 0x00 is stable");

    assert(nes::SpritePipeline::maybe_flipped_h(0x40, 0xFF) == 0xFF);
    log_pass("flipping 0xFF is stable");
}

static void test_vblank_flag_via_step() {
    std::printf("\n[VBlank flag via step()]\n");
    {
        nes::PPU ppu;
        bool found = false;
        for (int i = 0; i < 100000 && !found; i++) {
            ppu.step();
            if (ppu.vblank_flag) found = true;
        }

        assert(found == true);
        assert(ppu.vblank_flag == true);

        log_pass("step() sets vblank_flag at scanline 241 cycle 1");
    }
    {
        nes::PPU ppu;
        for (int i = 0; i < 100000 && !ppu.vblank_flag; i++) ppu.step();
        assert(ppu.vblank_flag == true);

        ppu.cpu_read_register(0x2002);
        assert(ppu.vblank_flag == false);

        log_pass("reading PPUSTATUS clears vblank_flag set by step()");
    }
}

int main() {
    std::cout << "Running NES PPU Tests..." << std::endl;

    test_ppu_reset();
    test_sprite_hflip();
    test_vblank_flag_via_step();

    std::cout << "\nAll tests passed successfully!!" << std::endl;
    return 0;
}
