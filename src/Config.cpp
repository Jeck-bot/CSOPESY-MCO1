#include "Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>

#include "Utils.h"

namespace {

constexpr uint64_t kMaxValue = 4294967296ULL; // 2^32, the largest value the spec allows

// Parses a decimal number within [low, high] into value.
bool parseNumber(const std::string& key, const std::string& text, uint64_t low, uint64_t high,
                 uint64_t& value, std::string& error) {
    bool digitsOnly = !text.empty() && text.size() <= 19 &&
                      std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
    if (digitsOnly) {
        value = std::stoull(text);
        if (value >= low && value <= high) {
            return true;
        }
    }
    error = "Invalid " + key + " \"" + text + "\": must be from " + std::to_string(low) + " to " +
            std::to_string(high) + ".";
    return false;
}

std::string withoutQuotes(const std::string& text) {
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

} // namespace

bool parseConfig(std::istream& input, Config& config, std::string& error) {
    Config result;
    std::string key;
    while (input >> key) {
        std::string value;
        if (!(input >> value)) {
            error = "Missing value for " + key + ".";
            return false;
        }
        key = toLower(key);

        uint64_t number = 0;
        if (key == "num-cpu") {
            if (!parseNumber(key, value, 1, 128, number, error)) {
                return false;
            }
            result.numCpu = static_cast<uint32_t>(number);
        } else if (key == "scheduler") {
            std::string name = toLower(withoutQuotes(value));
            if (name == "fcfs") {
                result.scheduler = SchedulerType::FCFS;
            } else if (name == "rr") {
                result.scheduler = SchedulerType::RR;
            } else {
                error = "Invalid scheduler \"" + value + "\": must be \"fcfs\" or \"rr\".";
                return false;
            }
        } else if (key == "quantum-cycles") {
            if (!parseNumber(key, value, 1, kMaxValue, result.quantumCycles, error)) {
                return false;
            }
        } else if (key == "batch-process-freq") {
            if (!parseNumber(key, value, 1, kMaxValue, result.batchProcessFreq, error)) {
                return false;
            }
        } else if (key == "min-ins") {
            if (!parseNumber(key, value, 1, kMaxValue, result.minIns, error)) {
                return false;
            }
        } else if (key == "max-ins") {
            if (!parseNumber(key, value, 1, kMaxValue, result.maxIns, error)) {
                return false;
            }
        } else if (key == "delay-per-exec" || key == "delays-per-exec") {
            // The spec's table spells it "delays-per-exec"; its sample file uses "delay-per-exec".
            if (!parseNumber(key, value, 0, kMaxValue, result.delayPerExec, error)) {
                return false;
            }
        }
        // Unknown keys are ignored.
    }

    if (result.minIns > result.maxIns) {
        error = "min-ins (" + std::to_string(result.minIns) + ") cannot be greater than max-ins (" +
                std::to_string(result.maxIns) + ").";
        return false;
    }
    config = result;
    return true;
}

bool loadConfig(const std::string& path, Config& config, std::string& error) {
    std::ifstream file(path);
    if (!file) {
        error = "Could not open " + path + ".";
        return false;
    }
    return parseConfig(file, config, error);
}
