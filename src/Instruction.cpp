#include "Instruction.h"

#include <algorithm>
#include <utility>

namespace {

const char* const kVariables[] = {"x", "y", "z"};

uint64_t randomBetween(std::minstd_rand& rng, uint64_t low, uint64_t high) {
    return std::uniform_int_distribution<uint64_t>(low, high)(rng);
}

std::string randomVariable(std::minstd_rand& rng) {
    return kVariables[randomBetween(rng, 0, 2)];
}

uint16_t randomValue(std::minstd_rand& rng) {
    return static_cast<uint16_t>(randomBetween(rng, 0, UINT16_MAX));
}

Operand randomOperand(std::minstd_rand& rng) {
    Operand operand;
    operand.isVariable = randomBetween(rng, 0, 1) == 0;
    if (operand.isVariable) {
        operand.variable = randomVariable(rng);
    } else {
        operand.value = randomValue(rng);
    }
    return operand;
}

} // namespace

uint64_t executedCount(const Instruction& statement) {
    if (statement.type != InstructionType::FOR) {
        return 1;
    }
    uint64_t bodyCount = 0;
    for (const Instruction& child : statement.body) {
        bodyCount += executedCount(child);
    }
    return bodyCount * statement.repeats;
}

Instruction randomStatement(std::minstd_rand& rng, const std::string& processName, int depth, uint64_t maxCount) {
    bool canLoop = depth < kMaxForNesting && maxCount >= 2;
    Instruction statement;
    statement.type = static_cast<InstructionType>(randomBetween(rng, 0, canLoop ? 5 : 4));

    switch (statement.type) {
    case InstructionType::PRINT:
        statement.message = "Hello world from " + processName + "!";
        break;
    case InstructionType::DECLARE:
        statement.variable = randomVariable(rng);
        statement.first.value = randomValue(rng);
        break;
    case InstructionType::ADD:
    case InstructionType::SUBTRACT:
        statement.variable = randomVariable(rng);
        statement.first = randomOperand(rng);
        statement.second = randomOperand(rng);
        break;
    case InstructionType::SLEEP:
        statement.sleepTicks = static_cast<uint8_t>(randomBetween(rng, 1, 10));
        break;
    case InstructionType::FOR: {
        // The body runs `repeats` times, so it may use at most maxCount / repeats instructions.
        statement.repeats = static_cast<uint16_t>(randomBetween(rng, 2, std::min<uint64_t>(5, maxCount)));
        uint64_t bodyBudget = maxCount / statement.repeats;
        uint64_t length = randomBetween(rng, 1, std::min<uint64_t>(5, bodyBudget));
        uint64_t used = 0;
        for (uint64_t i = 0; i < length; ++i) {
            // Leave at least one instruction for each statement still to come.
            uint64_t statementsLeft = length - i - 1;
            Instruction child = randomStatement(rng, processName, depth + 1, bodyBudget - used - statementsLeft);
            used += executedCount(child);
            statement.body.push_back(std::move(child));
        }
        break;
    }
    }
    return statement;
}
