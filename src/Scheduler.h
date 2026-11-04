#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Config.h"
#include "Process.h"

// One CPU core. It is idle when process is null.
struct CpuCore {
    std::shared_ptr<Process> process;
    uint64_t quantumUsed = 0; // rr: ticks used in the current time slice
    uint64_t delayLeft = 0;   // delay-per-exec busy-wait ticks remaining
};

// Data shown by screen -ls and report-util.
struct UtilizationSnapshot {
    int coresUsed = 0;
    int coresAvailable = 0;
    std::vector<std::shared_ptr<Process>> running;
    std::vector<std::shared_ptr<Process>> finished;
};

// Real-time CPU scheduler. A background thread advances the CPU tick counter
// and runs the cores until the console exits.
class Scheduler {
public:
    explicit Scheduler(const Config& config);
    ~Scheduler();

    void start();
    void stop();

    // screen -s: creates a process with random instructions and queues it.
    std::shared_ptr<Process> createProcess(const std::string& name);

    // screen -r: finds a process by name, or returns nullptr.
    std::shared_ptr<Process> findProcess(const std::string& name) const;

    // scheduler-start / scheduler-stop: generate p01, p02, ... every batch-process-freq ticks.
    void startBatchGeneration();
    void stopBatchGeneration();

    UtilizationSnapshot snapshot() const;

private:
    // One pass is one CPU tick: generate due processes, dispatch (fcfs or rr), run each core.
    void tickLoop();

    Config config;
    std::vector<CpuCore> cores;
    std::vector<std::shared_ptr<Process>> processes; // every process, in creation order
    std::deque<std::shared_ptr<Process>> readyQueue;
    uint64_t cpuTicks = 0;
    int nextProcessId = 1;
    int nextBatchNumber = 1;

    std::atomic<bool> running{false};
    std::atomic<bool> generatingBatch{false};
    std::thread tickThread;
    mutable std::mutex mutex;
};
