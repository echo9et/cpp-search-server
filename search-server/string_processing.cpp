#include "string_processing.h"
#include "string_view"
#include <iostream>

using std::string_view;
using std::vector;
//
// Не могу оставить одну, при передаче стоп слов серверу во время инцилизации важно получить имено String
std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}
//если со стринга, я переобразую в View то будет работать только в рамках функции а слова тянутьяс с std::set<string, std::less<>> buffer_; или вас не понял.
std::vector<std::string_view> SplitIntoWords(const std::string_view& text) {
    std::vector<std::string_view> words;
    size_t b = 0u, s = 0u;
    for (const char c : text) {
        if (c == ' ') {
            if (s != 0u) {
                words.push_back(text.substr(b,s));
                b += s + 1;
                s = 0u;
            }
            else{
                ++b;
            }
        } else {
            ++s;
        }
    }
    if (s != 0u) {
        words.push_back(text.substr(b,s));
    }
    return words;
}
