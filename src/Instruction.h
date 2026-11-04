#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

enum class InstructionType { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR };

// An instruction operand: a variable name or a uint16 literal.
struct Operand {
    bool isVariable = false;
    std::string variable;
    uint16_t value = 0;
};

// One process instruction. The fields used depend on the type:
//   PRINT    logs message, followed by the value of variable if one is given
//   DECLARE  variable = first.value
//   ADD      variable = first + second
//   SUBTRACT variable = first - second
//   SLEEP    gives up the CPU for sleepTicks ticks
//   FOR      runs body `repeats` times
struct Instruction {
    InstructionType type = InstructionType::PRINT;
    std::string message;
    std::string variable;
    Operand first;
    Operand second;
    uint8_t sleepTicks = 0;
    uint16_t repeats = 0;
    std::vector<Instruction> body;
};

// FOR loops nest at most this deep.
constexpr int kMaxForNesting = 3;

// Number of instructions a statement executes: 1, or for a FOR, its body's count times repeats.
uint64_t executedCount(const Instruction& statement);

// Generates a random statement that executes between 1 and maxCount instructions. depth is the
// number of FOR loops around it; a FOR is only generated below kMaxForNesting.
Instruction randomStatement(std::minstd_rand& rng, const std::string& processName, int depth, uint64_t maxCount);
