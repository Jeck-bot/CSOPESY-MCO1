#include "Console.h"

#include <iostream>

void Console::run() {
    printBanner();

    std::string line;
    while (true) {
        std::cout << "root:\\> ";
        if (!std::getline(std::cin, line) || line == "exit") {
            break;
        }
        handleCommand(line);
    }
}

void Console::printBanner() const {
    std::cout << R"ART(  ____   ____     ___    ____    _____   ____   __   __
 / ___| / ___|   / _ \  |  _ \  | ____| / ___|  \ \ / /
| |     \___ \  | | | | | |_) | |  _|   \___ \   \ V /
| |___   ___) | | |_| | |  __/  | |___   ___) |   | |
 \____| |____/   \___/  |_|     |_____| |____/    |_|
)ART";
    // TODO: replace the placeholder with the group members ("Last, First").
    std::cout << "-------------------------------------------------------\n"
              << "Welcome to CSOPESY Emulator!\n"
              << "\n"
              << "Developers:\n"
              << "Last Name, First Name\n"
              << "\n"
              << "Last updated: 11-04-2026\n"
              << "-------------------------------------------------------\n"
              << "\n";
}

void Console::handleCommand(const std::string& line) {
    // TODO: until "initialize" has run, recognize nothing except "exit". After that, dispatch
    // initialize, screen -s/-r/-ls, scheduler-start, scheduler-stop and report-util.
}

void Console::initialize() {
    // TODO: loadConfig("config.txt", ...), then create and start the scheduler.
}

void Console::screenCreate(const std::string& name) {
    // TODO: create the process, clear the console, and attach to it.
}

void Console::screenResume(const std::string& name) {
    // TODO: attach if the process exists and has not finished, else print "Process <name> not found."
}

void Console::screenList() const {
    // TODO: print utilizationReport().
}

void Console::reportUtil() const {
    // TODO: write utilizationReport() to csopesy-log.txt and print where it was saved.
}

void Console::attach(const std::shared_ptr<Process>& process) {
    // TODO: clear the console, show printProcessInfo(), then handle process-smi until "exit".
}

void Console::printProcessInfo(const Process& process) const {
    // TODO: name, ID and logs, then "Current instruction line" and "Lines of code", or "Finished!".
}

std::string Console::utilizationReport() const {
    // TODO: CPU utilization, cores used and available, then running and finished process rows.
    return "";
}
