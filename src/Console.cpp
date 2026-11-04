#include "Console.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "Utils.h"

namespace {

const char* const kPrompt = "root:\\> ";
const char* const kDivider = "----------------------------------------";

void clearScreen() {
    std::system("cls");
}

// The text after the first `count` words of line, trimmed.
std::string textAfterWords(const std::string& line, size_t count) {
    std::istringstream stream(line);
    std::string word;
    for (size_t i = 0; i < count && stream >> word; ++i) {
    }
    std::string rest;
    std::getline(stream, rest);
    return trim(rest);
}

// "100%" when the share is whole, otherwise two decimals, e.g. "66.67%".
std::string formatPercent(uint32_t used, uint32_t total) {
    if (used * 100 % total == 0) {
        return std::to_string(used * 100 / total) + "%";
    }
    std::ostringstream text;
    text << std::fixed << std::setprecision(2) << used * 100.0 / total << "%";
    return text.str();
}

} // namespace

void Console::run() {
    printBanner();

    std::string line;
    while (true) {
        std::cout << kPrompt;
        if (!std::getline(std::cin, line) || !handleCommand(line)) {
            break;
        }
    }
}

void Console::printBanner() const {
    std::cout << R"ART(  ____   ____     ___    ____    _____   ____   __   __
 / ___| / ___|   / _ \  |  _ \  | ____| / ___|  \ \ / /
| |     \___ \  | | | | | |_) | |  _|   \___ \   \ V /
| |___   ___) | | |_| | |  __/  | |___   ___) |   | |
 \____| |____/   \___/  |_|     |_____| |____/    |_|
)ART";
    std::cout << "-------------------------------------------------------\n"
              << "Welcome to CSOPESY Emulator!\n"
              << "\n"
              << "Developers:\n"
              << "Campo, Benette Enzo\n"
              << "Garcia, Andrea Gayle\n"
              << "Morgan, Jack Owen\n"
              << "Tenorio, Jeroen Ralph\n"
              << "\n"
              << "Last updated: 11-04-2026\n"
              << "-------------------------------------------------------\n"
              << "\n";
}

bool Console::handleCommand(const std::string& line) {
    std::vector<std::string> words = splitWords(line);
    if (words.empty()) {
        return true;
    }
    std::string command = toLower(words[0]);
    if (command == "exit") {
        return false;
    }

    // Until "initialize" has run, it is the only other command recognized.
    if (!initialized) {
        if (command == "initialize") {
            initialize();
        } else {
            std::cout << "Please run \"initialize\" first.\n";
        }
        return true;
    }

    std::string option = words.size() > 1 ? toLower(words[1]) : "";
    if (command == "initialize") {
        std::cout << "Already initialized.\n";
    } else if (command == "screen" && (option == "-s" || option == "-r")) {
        std::string name = textAfterWords(line, 2);
        if (name.empty()) {
            std::cout << "Usage: screen " << option << " <process name>\n";
        } else if (option == "-s") {
            return screenCreate(name);
        } else {
            return screenResume(name);
        }
    } else if ((command == "screen" && option == "-ls") || command == "screen-ls") {
        screenList();
    } else if (command == "screen") {
        std::cout << "Usage: screen -s <process name> | screen -r <process name> | screen -ls\n";
    } else if (command == "scheduler-start" || command == "scheduler-test") {
        // "scheduler-test" is the command's former name in the spec.
        std::cout << (scheduler->startBatchGeneration() ? "Scheduler started generating processes.\n"
                                                        : "Scheduler is already generating processes.\n");
    } else if (command == "scheduler-stop") {
        std::cout << (scheduler->stopBatchGeneration() ? "Scheduler stopped generating processes.\n"
                                                       : "Scheduler is not generating processes.\n");
    } else if (command == "report-util") {
        reportUtil();
    } else {
        std::cout << "Command not recognized.\n";
    }
    return true;
}

void Console::initialize() {
    std::string error;
    if (!loadConfig("config.txt", config, error)) {
        std::cout << "Could not initialize: " << error << "\n";
        return;
    }
    scheduler = std::make_unique<Scheduler>(config, std::random_device{}());
    scheduler->start();
    initialized = true;
    std::cout << "Initialized: num-cpu " << config.numCpu << ", scheduler "
              << (config.scheduler == SchedulerType::RR ? "rr" : "fcfs") << ", quantum-cycles "
              << config.quantumCycles << ", batch-process-freq " << config.batchProcessFreq << ", min-ins "
              << config.minIns << ", max-ins " << config.maxIns << ", delay-per-exec " << config.delayPerExec << "\n";
}

bool Console::screenCreate(const std::string& name) {
    std::optional<ProcessView> existing = scheduler->view(name);
    if (existing && existing->finished) {
        std::cout << "Process " << name << " already exists.\n";
        return true;
    }
    // A name that is still running opens that process: the spec has users open processes made
    // by scheduler-start with "screen -s <process name>".
    if (!existing) {
        scheduler->createProcess(name);
    }
    return attach(name);
}

bool Console::screenResume(const std::string& name) {
    std::optional<ProcessView> process = scheduler->view(name);
    if (!process || process->finished) {
        std::cout << "Process " << name << " not found.\n";
        return true;
    }
    return attach(name);
}

void Console::screenList() const {
    std::cout << utilizationReport() << "\n";
}

void Console::reportUtil() const {
    const std::string fileName = "csopesy-log.txt";
    std::ofstream file(fileName);
    if (!file) {
        std::cout << "Could not write " << fileName << ".\n";
        return;
    }
    file << utilizationReport();
    file.close();
    std::cout << "Report generated at " << std::filesystem::absolute(fileName).generic_string() << "!\n";
}

bool Console::attach(const std::string& name) {
    clearScreen();
    printProcessInfo(*scheduler->view(name));

    std::string line;
    while (true) {
        std::cout << kPrompt;
        if (!std::getline(std::cin, line)) {
            return false;
        }
        std::string command = toLower(trim(line));
        if (command == "exit") {
            clearScreen();
            printBanner();
            return true;
        }
        if (command == "process-smi") {
            printProcessInfo(*scheduler->view(name));
        } else if (!command.empty()) {
            std::cout << "Command not recognized.\n";
        }
    }
}

void Console::printProcessInfo(const ProcessView& process) const {
    std::cout << "\nProcess name: " << process.name << "\n"
              << "ID: " << process.id << "\n"
              << "Logs:\n";
    for (const LogEntry& log : process.logs) {
        std::cout << "(" << formatTimestamp(log.time) << ") Core:" << log.core << " \"" << log.text << "\"\n";
    }
    std::cout << "\n";
    if (process.finished) {
        std::cout << "Finished!\n";
    } else {
        std::cout << "Current instruction line: " << process.executed << "\n"
                  << "Lines of code: " << process.total << "\n";
    }
    std::cout << "\n";
}

std::string Console::utilizationReport() const {
    Utilization usage = scheduler->utilization();
    size_t nameWidth = 0;
    for (const ProcessRow& row : usage.running) {
        nameWidth = std::max(nameWidth, row.name.size());
    }
    for (const ProcessRow& row : usage.finished) {
        nameWidth = std::max(nameWidth, row.name.size());
    }
    int nameColumn = static_cast<int>(nameWidth) + 2;

    std::ostringstream text;
    text << std::left << "CPU utilization: " << formatPercent(usage.coresUsed, usage.coreCount) << "\n"
         << "Cores used: " << usage.coresUsed << "\n"
         << "Cores available: " << usage.coreCount - usage.coresUsed << "\n"
         << "\n"
         << kDivider << "\n"
         << "Running processes:\n";
    for (const ProcessRow& row : usage.running) {
        text << std::setw(nameColumn) << row.name << "(" << formatTimestamp(row.createdAt) << ")  Core: "
             << std::setw(4) << row.core << row.executed << " / " << row.total << "\n";
    }
    text << "\n"
         << "Finished processes:\n";
    for (const ProcessRow& row : usage.finished) {
        text << std::setw(nameColumn) << row.name << "(" << formatTimestamp(row.createdAt) << ")  Finished  "
             << row.total << " / " << row.total << "\n";
    }
    text << kDivider << "\n";
    return text.str();
}
