#include "search_server.h"
#include <iostream>
#include <list>
#include "log_duration.h"
using std::string;
using std::string_view;
//SearchServer::SearchServer(){}

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)){
}

void SearchServer::AddDocument(int document_id, const string_view& document, DocumentStatus status,
                 const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document.data());
    const double inv_word_count = 1.0 / words.size();
    set<string> doc_words;
    for (const string& word : words) {
        buffer_.insert(word);
        word_to_document_freqs_[*buffer_.find(word)][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][*buffer_.find(word)] += inv_word_count;
        doc_words.insert(word);
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int, DocumentStatus document_status, int) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string_view& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view& raw_query,
                                                    int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(__pstl::execution::sequenced_policy, const std::string_view& raw_query, int document_id) const
{

//    return MatchDocument(raw_query,document_id);
    if (documents_.count(document_id) == 0 ) throw std::out_of_range("ID no correct");

    const auto query = ParseQueryExec(raw_query);

    vector<std::string_view> matched_words;
    vector<std::string_view> prew_matched_words(query.plus_words.size());

    bool status_minus = std::any_of(std::execution::seq, query.minus_words.begin(), query.minus_words.end(),[&](const auto& word){
        return (word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id));
    });
    if ( status_minus ||  query.plus_words.size() == 0u ) return {matched_words, documents_.at(document_id).status};

    std::copy_if(std::execution::seq, query.plus_words.begin(), query.plus_words.end(), prew_matched_words.begin(),
             [&](const auto& word) {
                return word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id);});
    for (auto b = prew_matched_words.begin() ; b != prew_matched_words.end();++b){
        if ( *b!="" && ( find(matched_words.begin(),matched_words.end(), *b) == matched_words.end() ) ) {
            matched_words.push_back(word_to_document_freqs_.find(*b)->first);
        }
    }
    sort(matched_words.begin(),matched_words.end());
    return {matched_words, documents_.at(document_id).status};
}

tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(__pstl::execution::parallel_policy, const std::string_view& raw_query, int document_id) const
{
//    return MatchDocument(raw_query,document_id);
    if (documents_.count(document_id) == 0 ) throw std::out_of_range("ID no correct");

    const auto query = ParseQueryExec(raw_query);

    vector<std::string_view> matched_words;
    vector<std::string_view> prew_matched_words(query.plus_words.size());

    bool status_minus = std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),[&](const auto& word){
        return (word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id));
    });
    if ( status_minus ||  query.plus_words.size() == 0u ) return {matched_words, documents_.at(document_id).status};

    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), prew_matched_words.begin(),
             [&](const auto& word) {
                return word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id);});
    for (auto b = prew_matched_words.begin() ; b != prew_matched_words.end();++b){
        if ( *b!="" && ( find(matched_words.begin(),matched_words.end(), *b) == matched_words.end() ) ) {
            matched_words.push_back(word_to_document_freqs_.find(*b)->first);
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {

    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    int rating_sum = std::accumulate(ratings.begin(),ratings.end(),0);
    return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(const string& text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + text + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::QueryWordExec SearchServer::ParseQueryWord(const string_view& text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
//    cout << "word:" << text << endl;
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(string(word))) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }
    if (is_minus ) return {text.substr(1), is_minus, IsStopWord(string(word))};
    return {text, is_minus, IsStopWord(string(word))};
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query result;
    for (const string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

SearchServer::Query_par SearchServer::ParseQueryExec(const std::string_view &text) const
{
    Query_par query;
    for ( const std::string_view& word : SplitIntoWords(text) ){
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                if (find(query.minus_words.begin(),query.minus_words.end(),query_word.data) == query.minus_words.end())
                    query.minus_words.push_back(move(query_word.data));
            } else {
                if (find(query.plus_words.begin(),query.plus_words.end(),query_word.data) == query.plus_words.end())

                query.plus_words.push_back(move(query_word.data));
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

set<int>::iterator SearchServer::begin()
{
    return document_ids_.begin();
}

set<int>::iterator SearchServer::end()
{
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    if ( document_to_word_freqs_.count(document_id) > 0 )
    {
        return document_to_word_freqs_.at(document_id);
    }
    return empty_word_freqs_;
}

void SearchServer::RemoveDocument(int document_id)
{
    if ( documents_.count(document_id) == 0 ) return;

    for ( const auto& [word,r] : document_to_word_freqs_[document_id])
    {
        word_to_document_freqs_.at(word.data()).erase(document_id);
    }
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(__pstl::execution::sequenced_policy, int document_id)
{
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(__pstl::execution::parallel_policy, int document_id)
{
    if (!document_ids_.count(document_id)) {
        return;
    }
    std::for_each(std::execution::par, word_to_document_freqs_.begin(), word_to_document_freqs_.end(), [&document_id](auto& element) {
        if (element.second.count(document_id)) {
            element.second.erase(document_id);
        }
        });
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}


