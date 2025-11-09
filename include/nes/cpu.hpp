#pragma once
#include <string>
#include "timing.hpp"
namespace nes { class CPU { public: void reset(); void step_to(cycle_t count); void step(); std::string lookupInstruction(int); }; }
