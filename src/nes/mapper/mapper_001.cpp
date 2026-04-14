#include "nes/mapper/mapper_001.hpp"

namespace nes {

    Mapper_001::Mapper_001(uint16_t prgBanks, uint16_t chrBanks, bool is_chr_ram)
        : Mapper(prgBanks, chrBanks), is_chr_ram(is_chr_ram) {
        // Initial power-on state
        prg_ram.resize(8192, 0x00);
        prg_bank_16_lo = 0;
        prg_bank_16_hi = prgBanks - 1; // High bank is fixed to the last bank on boot
        control_register = 0x1C;
    }

    void Mapper_001::update_offsets() {
        // PRG ROM Mapping Mode (Control Register Bits 2 & 3)
        uint8_t prg_mode = (control_register >> 2) & 0x03;

        uint8_t safe_prg_bank = prg_bank % prgBanks;

        if (prg_mode == 0 || prg_mode == 1) {
            prg_bank_16_lo = (safe_prg_bank & 0xFE);
            prg_bank_16_hi = (safe_prg_bank & 0xFE) + 1;
        }
        else if (prg_mode == 2) {
            prg_bank_16_lo = 0;
            prg_bank_16_hi = safe_prg_bank;
        }
        else if (prg_mode == 3) {
            prg_bank_16_lo = safe_prg_bank;
            prg_bank_16_hi = prgBanks - 1;
        }

        // CHR ROM Mapping Mode (Control Register Bit 4)
        uint8_t chr_mode = (control_register >> 4) & 0x01;

        uint8_t num_4kb_banks = (chrBanks == 0) ? 2 : (chrBanks * 2);

        uint8_t safe_chr_bank_0 = chr_bank_0 % num_4kb_banks;
        uint8_t safe_chr_bank_1 = chr_bank_1 % num_4kb_banks;

        if (chr_mode == 0) {
            chr_bank_4_lo = (safe_chr_bank_0 & 0xFE);
            chr_bank_4_hi = (safe_chr_bank_0 & 0xFE) + 1;
        }
        else {
            chr_bank_4_lo = safe_chr_bank_0;
            chr_bank_4_hi = safe_chr_bank_1;
        }
    }

    bool Mapper_001::cpuMapRead(uint16_t addr, uint32_t& mapped_addr, uint8_t &data) {
        if (addr >= 0x6000 && addr <= 0x7FFF) {
            data = prg_ram[addr & 0x1FFF];
            mapped_addr = 0xFFFFFFFF;
            return true;
        }
        if (addr >= 0x8000 && addr <= 0xBFFF) {
            mapped_addr = (prg_bank_16_lo * 0x4000) + (addr & 0x3FFF);
            return true;
        }
        if (addr >= 0xC000 && addr <= 0xFFFF) {
            mapped_addr = (prg_bank_16_hi * 0x4000) + (addr & 0x3FFF);
            return true;
        }
        return false;
    }

    bool Mapper_001::cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
        if (addr >= 0x6000 && addr <= 0x7FFF) {
            prg_ram[addr & 0x1FFF] = data; // Write directly to our new RAM array
            return true; // Tells the CPU we successfully handled the write
        }
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            if (data & 0x80) {
                // Reset Shift Register
                shift_register = 0x10;
                control_register |= 0x0C; // Lock PRG ROM to mode 3
                update_offsets();
            }
            else {
                // Shift in the lowest bit
                bool complete = shift_register & 0x01;
                shift_register >>= 1;
                shift_register |= ((data & 0x01) << 4);

                if (complete) {
                    // 5th write completed, extract target register from address
                    uint8_t target_reg = (addr >> 13) & 0x03;

                    if (target_reg == 0) {
                        control_register = shift_register & 0x1F;
                    }
                    else if (target_reg == 1) {
                        chr_bank_0 = shift_register & 0x1F;
                    }
                    else if (target_reg == 2) {
                        chr_bank_1 = shift_register & 0x1F;
                    }
                    else if (target_reg == 3) {
                        prg_bank = shift_register & 0x0F;
                    }

                    update_offsets();
                    shift_register = 0x10; // Reset after complete write
                }
            }
            return false; // Tells CPU not to write to physical ROM array
        }
        return false;
    }

    bool Mapper_001::ppuMapRead(uint16_t addr, uint32_t& mapped_addr) {
        if (addr >= 0x0000 && addr <= 0x0FFF) {
            mapped_addr = (chr_bank_4_lo * 0x1000) + (addr & 0x0FFF);
            return true;
        }
        if (addr >= 0x1000 && addr <= 0x1FFF) {
            mapped_addr = (chr_bank_4_hi * 0x1000) + (addr & 0x0FFF);
            return true;
        }
        return false;
    }

    bool Mapper_001::ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) {
        if (addr >= 0x0000 && addr <= 0x1FFF && is_chr_ram) {
            if (addr <= 0x0FFF) {
                mapped_addr = (chr_bank_4_lo * 0x1000) + (addr & 0x0FFF);
            }
            else {
                mapped_addr = (chr_bank_4_hi * 0x1000) + (addr & 0x0FFF);
            }
            return true;
        }
        return false;
    }

} // namespace nes