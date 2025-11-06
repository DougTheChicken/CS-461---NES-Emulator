#pragma once
#include <cstdint>
#include <string>
namespace nes { class CPU { public: void reset(); void step(); std::string lookupInstruction(int); }; }
