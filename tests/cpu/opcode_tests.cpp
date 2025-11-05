//
// Created by Chris Blum on 11/4/25.
//
#include "nes/cpu.hpp"
#include <gtest/gtest.h>

namespace {

	// Tests the ORA opcode
	TEST(OpcodeTest, Ora) {
	std::string ora = "ORA";
	nes::CPU cpu;
	EXPECT_EQ(ora, cpu.lookupInstruction(1));
	// EXPECT_EQ(ora, cpu.lookupInstruction(2));
}

}