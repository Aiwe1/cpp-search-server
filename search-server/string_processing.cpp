#include "string_processing.h"

std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}
/*
std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
} */

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    text.remove_prefix(std::min(text.find_first_not_of(" "), text.size())); // до первого символа
    while (!text.empty()) {
        size_t position_first_space = text.find(' ');
        std::string_view tmp_substr = text.substr(0, position_first_space);
        result.push_back(tmp_substr);
        text.remove_prefix(tmp_substr.size());
        text.remove_prefix(std::min(text.find_first_not_of(" "), text.size())); // до первого символа
    }
    return result;
} 