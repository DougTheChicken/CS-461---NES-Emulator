//
// Created by Chris Blum on 11/4/25.
//
#include "nes/cpu.hpp"
#include <string>

namespace nes {
    void CPU::reset() {}
    void CPU::step() {}
	std::string CPU::lookupInstruction(int opcode) {
		switch (opcode) { case 0x01: return "ORA"; default: return "???"; }
	}
}
