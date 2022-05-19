#include "request_queue.h"

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {

    const auto& answer = search_server_.FindTopDocuments(raw_query,document_predicate);

    if (answer.empty()) {
        ++count_empty_answer_;
        requests_.push_back(true);
    }
    else {
        requests_.push_back(false);
    }
    //добавили уже ответ, поэтому -1
    if ( requests_.size() - 1  == min_in_day_ ){
        if ( requests_.at(0) ) --count_empty_answer_;
        requests_.pop_front();
    }

    return answer;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status]( int, DocumentStatus document_status, int ) {
        return document_status == status;
    });
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);

}

int RequestQueue::GetNoResultRequests() const {
    return count_empty_answer_;
}
