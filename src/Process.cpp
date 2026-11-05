#include "Process.h"

#include <algorithm>
#include <random>
#include <utility>

namespace {

uint16_t clampToUint16(int64_t value) {
    return static_cast<uint16_t>(std::clamp<int64_t>(value, 0, UINT16_MAX));
}

} // namespace

Process::Process(int id, std::string name, uint64_t totalInstructions, uint32_t seed)
    : id(id), name(std::move(name)), createdAt(std::time(nullptr)), totalInstructions(totalInstructions),
      rng(seed) {
    variables["x"] = 0; // every process starts with x = 0 in its symbol table
}

StepResult Process::step(int core) {
    const Instruction& instruction = nextInstruction();
    execute(instruction, core);
    ++executed;
    if (executed >= totalInstructions) {
        // A finished process releases its variables and loop state.
        variables.clear();
        loops.clear();
        statement = Instruction();
        return StepResult::FINISHED;
    }
    return instruction.type == InstructionType::SLEEP ? StepResult::SLEPT : StepResult::RAN;
}

void Process::execute(const Instruction& instruction, int core) {
    switch (instruction.type) {
    case InstructionType::PRINT: {
        std::string text = instruction.message;
        if (!instruction.variable.empty()) {
            text += std::to_string(valueOf(instruction.variable));
        }
        logs.push_back({std::time(nullptr), core, std::move(text)});
        break;
    }
    case InstructionType::DECLARE:
        valueOf(instruction.variable) = instruction.first.value;
        break;
    case InstructionType::ADD: {
        uint16_t sum = clampToUint16(int64_t{read(instruction.first)} + read(instruction.second));
        valueOf(instruction.variable) = sum;
        break;
    }
    case InstructionType::SUBTRACT: {
        uint16_t difference = clampToUint16(int64_t{read(instruction.first)} - read(instruction.second));
        valueOf(instruction.variable) = difference;
        break;
    }
    case InstructionType::SLEEP:
        sleepTicks = instruction.sleepTicks;
        break;
    case InstructionType::FOR:
        break; // step() enters loops itself
    }
}

void Process::startLoop(Instruction loop) {
    loops.clear();
    statement = std::move(loop);
    loops.push_back({&statement, 0, statement.repeats});
}

int Process::getId() const {
    return id;
}

const std::string& Process::getName() const {
    return name;
}

std::time_t Process::getCreatedAt() const {
    return createdAt;
}

uint64_t Process::getExecuted() const {
    return executed;
}

uint64_t Process::getTotalInstructions() const {
    return totalInstructions;
}

bool Process::isFinished() const {
    return executed >= totalInstructions;
}

const std::vector<LogEntry>& Process::getLogs() const {
    return logs;
}

const std::unordered_map<std::string, uint16_t>& Process::getVariables() const {
    return variables;
}

uint8_t Process::getSleepTicks() const {
    return sleepTicks;
}

const Instruction& Process::nextInstruction() {
    while (true) {
        if (loops.empty()) {
            // Instructions alternate PRINT("Value from: " + x), ADD(x, x, [1-10]), PRINT, ADD, ...
            // ponytail: replaces randomStatement for the quiz; switch back to
            // randomStatement(rng, name, 0, totalInstructions - executed) for the random mix.
            Instruction next;
            if (executed % 2 == 0) {
                next.type = InstructionType::PRINT;
                next.message = "Value from: ";
                next.variable = "x";
            } else {
                next.type = InstructionType::ADD;
                next.variable = "x";
                next.first.isVariable = true;
                next.first.variable = "x";
                next.second.value = static_cast<uint16_t>(std::uniform_int_distribution<int>(1, 10)(rng));
            }
            statement = std::move(next);
            return statement;
        }

        LoopFrame& frame = loops.back();
        if (frame.next == frame.loop->body.size()) {
            // End of the body: run it again or leave the loop.
            if (--frame.repeatsLeft > 0) {
                frame.next = 0;
            } else {
                loops.pop_back();
            }
            continue;
        }

        const Instruction& instruction = frame.loop->body[frame.next++];
        if (instruction.type != InstructionType::FOR) {
            return instruction;
        }
        loops.push_back({&instruction, 0, instruction.repeats});
    }
}

uint16_t& Process::valueOf(const std::string& variable) {
    // Variables used before being declared start at 0.
    return variables.try_emplace(variable, 0).first->second;
}

uint16_t Process::read(const Operand& operand) {
    return operand.isVariable ? valueOf(operand.variable) : operand.value;
}
