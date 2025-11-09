#include "nes/cpu.hpp"
#include <string>

#include "nes/timing.hpp"

namespace nes {
	cycle_t next_cpu_tick_ppu = 0; // when the next CPU cycle boundary occurs (in PPU cycles)
    void CPU::reset() {}

	void CPU::step_to(cycle_t count)
    {
    	while (next_cpu_tick_ppu < count)
    	{
    		step();
    	}
    	next_cpu_tick_ppu += CPU_TO_PPU;
    }

	void CPU::step() {}

	std::string CPU::lookupInstruction(int opcode) {
		switch (opcode) { case 0x01: return "ORA"; default: return "???"; }
	}
}
