#pragma once

#include <memory>
#include <string>

#include "Config.h"
#include "Process.h"
#include "Scheduler.h"

// The main menu console and the process screens: shows the UI, recognizes commands, and runs them.
class Console {
public:
    // Prints the banner and reads commands until "exit".
    void run();

private:
    void printBanner() const;
    void handleCommand(const std::string& line);

    // Main menu commands
    void initialize();                          // initialize
    void screenCreate(const std::string& name); // screen -s <name>
    void screenResume(const std::string& name); // screen -r <name>
    void screenList() const;                    // screen -ls
    void reportUtil() const;                    // report-util

    // Process screen: handles process-smi until "exit"
    void attach(const std::shared_ptr<Process>& process);
    void printProcessInfo(const Process& process) const;

    // Text shared by screen -ls and report-util
    std::string utilizationReport() const;

    bool initialized = false;
    Config config;
    std::unique_ptr<Scheduler> scheduler;
};
