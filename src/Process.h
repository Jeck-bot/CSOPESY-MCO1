#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Instruction.h"

enum class ProcessState { READY, RUNNING, SLEEPING, FINISHED };

// One PRINT output. process-smi shows it as: (timestamp) Core:N "message"
struct LogEntry {
    std::string timestamp;
    int coreId = 0;
    std::string message;
};

// A process created by "screen -s" or by scheduler-start.
struct Process {
    Process(int id, std::string name, std::vector<Instruction> instructions);

    // Executes the next instruction on the given core. The scheduler calls this once per CPU tick.
    void executeNextInstruction(int core);

    bool isFinished() const;

    int id;
    std::string name;
    std::string createdAt;                               // shown by screen -ls
    std::vector<Instruction> instructions;
    uint64_t currentLine = 0;                            // "Current instruction line"
    uint64_t totalLines = 0;                             // "Lines of code"
    std::unordered_map<std::string, uint16_t> variables; // kept until the process finishes
    std::vector<LogEntry> logs;
    ProcessState state = ProcessState::READY;
    int coreId = -1;                                     // -1 while not on a core
    uint64_t sleepTicksLeft = 0;
};
