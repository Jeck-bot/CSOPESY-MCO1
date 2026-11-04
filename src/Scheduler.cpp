#include "Scheduler.h"

Scheduler::Scheduler(const Config& config) : config(config), cores(config.numCpu) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    // TODO: launch tickThread running tickLoop().
}

void Scheduler::stop() {
    // TODO: end tickLoop() and join tickThread.
}

std::shared_ptr<Process> Scheduler::createProcess(const std::string& name) {
    // TODO: build the process with generateInstructions(), store it, and push it to readyQueue.
    return nullptr;
}

std::shared_ptr<Process> Scheduler::findProcess(const std::string& name) const {
    // TODO
    return nullptr;
}

void Scheduler::startBatchGeneration() {
    generatingBatch = true;
}

void Scheduler::stopBatchGeneration() {
    generatingBatch = false;
}

UtilizationSnapshot Scheduler::snapshot() const {
    // TODO: count busy cores and split processes into running and finished.
    return {};
}

void Scheduler::tickLoop() {
    // TODO: while running: every batchProcessFreq ticks create a process if generatingBatch,
    // wake sleeping processes, fill idle cores from readyQueue (fcfs, or rr with quantumCycles),
    // honour delayPerExec, execute one instruction per busy core, then advance cpuTicks.
}
