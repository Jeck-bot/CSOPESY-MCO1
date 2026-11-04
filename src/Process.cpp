#include "Process.h"

#include <utility>

#include "Utils.h"

Process::Process(int id, std::string name, std::vector<Instruction> instructions)
    : id(id), name(std::move(name)), createdAt(currentTimestamp()), instructions(std::move(instructions)) {
    // TODO: set totalLines from the instruction list, counting FOR bodies.
}

void Process::executeNextInstruction(int core) {
    // TODO: run the instruction at currentLine. PRINT appends to logs; DECLARE, ADD and
    // SUBTRACT update variables clamped to [0, 65535]; SLEEP sets sleepTicksLeft;
    // FOR runs its body. Mark the process FINISHED after the last instruction.
}

bool Process::isFinished() const {
    return state == ProcessState::FINISHED;
}
