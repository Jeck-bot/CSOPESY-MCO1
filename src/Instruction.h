#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Config.h"

enum class InstructionType { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR };

// One process instruction. args holds the operands as generated:
//   PRINT    {msg} or {msg, var}             logs msg, optionally followed by var's value
//   DECLARE  {var, value}                    declares a uint16 variable
//   ADD      {var1, var2/value, var3/value}  var1 = var2/value + var3/value
//   SUBTRACT {var1, var2/value, var3/value}  var1 = var2/value - var3/value
//   SLEEP    {X}                             sleeps X (uint8) CPU ticks
// FOR repeats body `repeats` times; FOR loops nest at most 3 deep.
struct Instruction {
    InstructionType type = InstructionType::PRINT;
    std::vector<std::string> args;
    std::vector<Instruction> body;
    uint16_t repeats = 0;
};

constexpr int kMaxForNesting = 3;

// Creates a random instruction list for a process, sized within [config.minIns, config.maxIns].
// PRINT messages default to "Hello world from <processName>!".
std::vector<Instruction> generateInstructions(const std::string& processName, const Config& config);
