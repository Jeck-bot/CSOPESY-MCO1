#include "Config.h"

bool loadConfig(const std::string& path, Config& config, std::string& error) {
    // TODO: read each "key value" line, strip quotes from the scheduler value,
    // accept both "delay-per-exec" and "delays-per-exec", and check the ranges.
    error = "Could not read " + path;
    return false;
}
