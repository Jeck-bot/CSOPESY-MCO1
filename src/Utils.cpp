#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

std::string formatTimestamp(std::time_t time) {
    std::tm local{};
    localtime_s(&local, &time);
    char text[32];
    std::strftime(text, sizeof(text), "%m/%d/%Y %I:%M:%S%p", &local);
    return text;
}

std::string trim(const std::string& text) {
    const char* whitespace = " \t\r\n\f\v";
    size_t start = text.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = text.find_last_not_of(whitespace);
    return text.substr(start, end - start + 1);
}

std::vector<std::string> splitWords(const std::string& text) {
    std::istringstream stream(text);
    std::vector<std::string> words;
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}
