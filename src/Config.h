#pragma once

#include <cstdint>
#include <string>

enum class SchedulerType { FCFS, RR };

// Settings read from config.txt by the "initialize" command.
// Defaults are the spec's sample values. Ranges up to 2^32 need 64-bit fields.
struct Config {
    uint32_t numCpu = 4;                         // num-cpu            [1, 128]
    SchedulerType scheduler = SchedulerType::RR; // scheduler          "fcfs" or "rr"
    uint64_t quantumCycles = 5;                  // quantum-cycles     [1, 2^32], rr only
    uint64_t batchProcessFreq = 1;               // batch-process-freq [1, 2^32]
    uint64_t minIns = 1000;                      // min-ins            [1, 2^32]
    uint64_t maxIns = 2000;                      // max-ins            [1, 2^32]
    uint64_t delayPerExec = 0;                   // delay-per-exec     [0, 2^32]
};

// Loads the space-separated "key value" file at path into config.
// Returns false and sets error when the file is missing or a value is invalid.
bool loadConfig(const std::string& path, Config& config, std::string& error);
