#include "Scheduler.h"

#include <chrono>
#include <utility>

namespace {

constexpr std::chrono::milliseconds kTickPeriod(10);

ProcessRow rowFor(const Process& process, int core) {
    return {process.getName(), process.getCreatedAt(), core, process.getExecuted(), process.getTotalInstructions()};
}

} // namespace

Scheduler::Scheduler(const Config& config, uint64_t seed) : config(config), rng(seed), cores(config.numCpu) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (running.exchange(true)) {
        return;
    }
    tickThread = std::thread([this] {
        auto nextTick = std::chrono::steady_clock::now();
        while (running) {
            tick();
            nextTick += kTickPeriod;
            auto now = std::chrono::steady_clock::now();
            if (nextTick < now) {
                nextTick = now; // fell behind: carry on from now instead of rushing to catch up
            }
            std::this_thread::sleep_until(nextTick);
        }
    });
}

void Scheduler::stop() {
    running = false;
    if (tickThread.joinable()) {
        tickThread.join();
    }
}

void Scheduler::tick() {
    std::lock_guard<std::mutex> lock(mutex);

    for (size_t i = 0; i < cores.size(); ++i) {
        CpuCore& core = cores[i];
        if (!core.process) {
            continue;
        }
        Process& process = *core.process;

        if (process.delayLeft > 0) {
            --process.delayLeft; // busy-waiting: the process keeps the core
        } else {
            StepResult result = process.step(static_cast<int>(i));
            process.delayLeft = config.delayPerExec;
            if (result == StepResult::FINISHED) {
                finished.push_back(std::move(core.process));
                continue;
            }
            if (result == StepResult::SLEPT) {
                // Off the CPU for sleepTicks ticks, then back to the ready queue.
                sleeping.emplace(cpuTicks + process.getSleepTicks() + 1, std::move(core.process));
                continue;
            }
        }

        ++core.quantumUsed;
        if (config.scheduler == SchedulerType::RR && core.quantumUsed >= config.quantumCycles && !readyQueue.empty()) {
            readyQueue.push_back(std::move(core.process));
        }
    }

    if (generatingBatch && ++ticksSinceBatch >= config.batchProcessFreq * kTicksPerBatchCycle) {
        ticksSinceBatch = 0;
        addProcess(nextBatchName());
    }

    // Wake processes that may run again from the next tick.
    while (!sleeping.empty() && sleeping.begin()->first <= cpuTicks + 1) {
        readyQueue.push_back(std::move(sleeping.begin()->second));
        sleeping.erase(sleeping.begin());
    }

    for (CpuCore& core : cores) {
        if (!core.process && !readyQueue.empty()) {
            core.process = std::move(readyQueue.front());
            readyQueue.pop_front();
            core.quantumUsed = 0;
        }
    }

    ++cpuTicks;
}

bool Scheduler::createProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    if (processes.count(name) > 0) {
        return false;
    }
    addProcess(name);
    return true;
}

std::optional<ProcessView> Scheduler::view(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto found = processes.find(name);
    if (found == processes.end()) {
        return std::nullopt;
    }
    const Process& process = *found->second;
    return ProcessView{process.getName(), process.getId(), process.getLogs(), process.getExecuted(),
                       process.getTotalInstructions(), process.isFinished()};
}

bool Scheduler::startBatchGeneration() {
    std::lock_guard<std::mutex> lock(mutex);
    if (generatingBatch) {
        return false;
    }
    generatingBatch = true;
    ticksSinceBatch = 0;
    return true;
}

bool Scheduler::stopBatchGeneration() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!generatingBatch) {
        return false;
    }
    generatingBatch = false;
    return true;
}

Utilization Scheduler::utilization() const {
    std::lock_guard<std::mutex> lock(mutex);
    Utilization usage;
    usage.coreCount = static_cast<uint32_t>(cores.size());
    for (size_t i = 0; i < cores.size(); ++i) {
        if (cores[i].process) {
            ++usage.coresUsed;
            usage.running.push_back(rowFor(*cores[i].process, static_cast<int>(i)));
        }
    }
    for (const std::shared_ptr<Process>& process : finished) {
        usage.finished.push_back(rowFor(*process, -1));
    }
    return usage;
}

void Scheduler::addProcess(const std::string& name) {
    uint64_t length = std::uniform_int_distribution<uint64_t>(config.minIns, config.maxIns)(rng);
    uint32_t seed = static_cast<uint32_t>(rng());
    auto process = std::make_shared<Process>(nextProcessId++, name, length, seed);
    processes.emplace(name, process);
    readyQueue.push_back(std::move(process));
}

std::string Scheduler::nextBatchName() {
    // p01, p02, ..., p99, p100, ...; skips names already used by "screen -s".
    while (true) {
        std::string number = std::to_string(nextBatchNumber++);
        std::string name = "p" + std::string(number.size() < 2 ? "0" : "") + number;
        if (processes.count(name) == 0) {
            return name;
        }
    }
}
