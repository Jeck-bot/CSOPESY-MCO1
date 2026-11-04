#pragma once

#include <ctime>
#include <string>
#include <vector>

// Formats a time as "MM/DD/YYYY hh:mm:ssAM", the format used in process logs and reports.
std::string formatTimestamp(std::time_t time);

// Removes leading and trailing whitespace.
std::string trim(const std::string& text);

// Splits text into whitespace-separated words.
std::vector<std::string> splitWords(const std::string& text);

std::string toLower(std::string text);
