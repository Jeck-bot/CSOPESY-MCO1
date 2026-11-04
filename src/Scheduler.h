#pragma once

#include <atomic>
#include <cstdint>
#include <ctime>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Config.h"
#include "Process.h"

// One row of screen -ls: a process on a core, or a finished process (core is -1).
struct ProcessRow {
    std::string name;
    std::time_t createdAt = 0;
    int core = -1;
    uint64_t executed = 0;
    uint64_t total = 0;
};

// What screen -ls and report-util show.
struct Utilization {
    uint32_t coresUsed = 0;
    uint32_t coreCount = 0;
    std::vector<ProcessRow> running;  // processes on cores, in core order
    std::vector<ProcessRow> finished; // in the order they finished
};

// What process-smi shows.
struct ProcessView {
    std::string name;
    int id = 0;
    std::vector<LogEntry> logs;
    uint64_t executed = 0;
    uint64_t total = 0;
    bool finished = false;
};

// Real-time CPU scheduler. A background thread runs one CPU tick every 10 ms on all cores.
class Scheduler {
public:
    Scheduler(const Config& config, uint64_t seed);
    ~Scheduler();
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // Starts and stops the background thread that calls tick().
    void start();
    void stop();

    // One CPU cycle: run every busy core, generate a batch process when due, wake sleepers whose
    // sleep is over, and give idle cores the next ready processes.
    void tick();

    // screen -s: creates a process with a random length in [min-ins, max-ins] and queues it.
    // Returns false if the name is taken.
    bool createProcess(const std::string& name);

    // The process with this name, or nullopt if there is none.
    std::optional<ProcessView> view(const std::string& name) const;

    // scheduler-start / scheduler-stop. Return false if generation was already on / off.
    bool startBatchGeneration();
    bool stopBatchGeneration();

    Utilization utilization() const;

private:
    struct CpuCore {
        std::shared_ptr<Process> process; // null when idle
        uint64_t quantumUsed = 0;         // ticks since this process was dispatched
    };

    void addProcess(const std::string& name);
    std::string nextBatchName();

    Config config;
    std::mt19937_64 rng;
    std::vector<CpuCore> cores;
    std::deque<std::shared_ptr<Process>> readyQueue;
    std::multimap<uint64_t, std::shared_ptr<Process>> sleeping; // keyed by the tick they wake
    std::vector<std::shared_ptr<Process>> finished;              // in the order they finished
    std::unordered_map<std::string, std::shared_ptr<Process>> processes;
    uint64_t cpuTicks = 0;
    uint64_t ticksSinceBatch = 0;
    int nextProcessId = 1;
    int nextBatchNumber = 1;
    bool generatingBatch = false;

    std::atomic<bool> running{false};
    std::thread tickThread;
    mutable std::mutex mutex;
};
