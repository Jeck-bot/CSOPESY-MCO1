#pragma once

#include <memory>
#include <string>

#include "Config.h"
#include "Scheduler.h"

// The main menu console and the process screens: shows the UI, recognizes commands, and runs them.
class Console {
public:
    // Prints the banner and reads commands until "exit" or the end of input.
    void run();

private:
    void printBanner() const;

    // Runs one main menu command. Returns false when the console should close.
    bool handleCommand(const std::string& line);

    void initialize();                          // initialize
    bool screenCreate(const std::string& name); // screen -s <name>
    bool screenResume(const std::string& name); // screen -r <name>
    void screenList() const;                    // screen -ls
    void reportUtil() const;                    // report-util

    // Process screen: handles process-smi until "exit". Returns false at the end of input.
    bool attach(const std::string& name);
    void printProcessInfo(const ProcessView& process) const;

    // Text shared by screen -ls and report-util
    std::string utilizationReport() const;

    bool initialized = false;
    Config config;
    std::unique_ptr<Scheduler> scheduler;
};
