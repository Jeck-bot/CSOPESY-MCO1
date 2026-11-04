#pragma once

#include <cstdint>
#include <ctime>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "Instruction.h"

// What happened when a process ran one instruction.
enum class StepResult { RAN, SLEPT, FINISHED };

// One PRINT output. process-smi shows it as: (time) Core:N "text"
struct LogEntry {
    std::time_t time = 0;
    int core = 0;
    std::string text;
};

// A process created by "screen -s" or by scheduler-start. Its instructions are generated from its
// seed as it runs, and exactly totalInstructions of them execute.
class Process {
public:
    Process(int id, std::string name, uint64_t totalInstructions, uint32_t seed);
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // Runs the next instruction on core. Returns SLEPT after a SLEEP (see getSleepTicks()) and
    // FINISHED after the last instruction.
    StepResult step(int core);

    // Executes one instruction other than FOR.
    void execute(const Instruction& instruction, int core);

    // Makes loop the current statement; step() runs its body until the loop ends.
    void startLoop(Instruction loop);

    int getId() const;
    const std::string& getName() const;
    std::time_t getCreatedAt() const;
    uint64_t getExecuted() const;           // "Current instruction line"
    uint64_t getTotalInstructions() const;  // "Lines of code"
    bool isFinished() const;
    const std::vector<LogEntry>& getLogs() const;
    const std::unordered_map<std::string, uint16_t>& getVariables() const;
    uint8_t getSleepTicks() const;          // ticks requested by the last SLEEP

    // Busy-wait ticks left before the next instruction (delay-per-exec). Managed by the scheduler.
    uint64_t delayLeft = 0;

private:
    // Position inside an active FOR loop.
    struct LoopFrame {
        const Instruction* loop;
        size_t next;           // index of the next body instruction
        uint16_t repeatsLeft;  // repetitions left, including the current one
    };

    const Instruction& nextInstruction();
    uint16_t& valueOf(const std::string& variable);
    uint16_t read(const Operand& operand);

    int id;
    std::string name;
    std::time_t createdAt;
    uint64_t totalInstructions;
    uint64_t executed = 0;
    std::minstd_rand rng;
    Instruction statement;                                 // the top-level statement being run
    std::vector<LoopFrame> loops;                          // active FOR loops, innermost last
    std::unordered_map<std::string, uint16_t> variables;  // kept until the process finishes
    std::vector<LogEntry> logs;
    uint8_t sleepTicks = 0;
};
