#pragma once
#include <stack>
#include <string>

#include "search_server.h"

using std::string;
using std::deque;

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server): search_server_( search_server ){
        ;
    }
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);

    vector<Document> AddFindRequest(const string& raw_query);

    int GetNoResultRequests() const;

private:

    //если исходить не из задания, то ниже deque<boo> мне не нужна
    deque<bool> requests_;
    size_t count_empty_answer_ = 0;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    // возможно, здесь вам понадобится что-то ещё
};
