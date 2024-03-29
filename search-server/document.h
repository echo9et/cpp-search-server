#pragma once
#include <ostream>
#include <string>

using std::ostream;

struct Document {
    Document() = default;
    Document(int id, double relevance, int rating);
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};


enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

ostream& operator<<(ostream& out, const Document& document);
