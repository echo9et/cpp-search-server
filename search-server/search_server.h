#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <numeric>
#include <math.h>
#include <execution>
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include <future>
//using std::vector;
//using std::map;
//using std::set;
//using std::invalid_argument;
//using std::tuple;
//using std::string_literals::operator""s;
//using std::string_view;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;
static const std::map<int, std::set<string>> document_id_words_;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const string& stop_words_text);

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
                     const vector<int>& ratings) ;

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const std::string_view& raw_query,
                                      DocumentPredicate document_predicate) const;

    vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

    template <typename DocumentPredicate, class ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query,
                                      DocumentPredicate document_predicate) const;

    template <class ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentStatus status) const;

    template <class ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query) const;

    int GetDocumentCount() const ;

    std::tuple<vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query,
                                                        int document_id) const ;

    std::tuple<vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy, const std::string_view& raw_query,
                                                        int document_id) const ;

    std::tuple<vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy, const std::string_view& raw_query,
                                                        int document_id) const ;

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy, int document_id );

private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<string, std::less<>> buffer_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    const std::set<string> stop_words_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const string& word) const;

    static bool IsValidWord(const string& word);

    vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct QueryWordExec {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    //QueryWord ParseQueryWord(const string& text) const;

    QueryWordExec ParseQueryWord(const std::string_view& text) const;

    struct Query {
        vector<std::string_view> plus_words;
        vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view& text) const;

    double ComputeWordInverseDocumentFreq(const string& word) const;

    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate, class ExecutionPolicy>
    vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
                                      DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }

template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query,
                                  DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query.data());

    auto matched_documents = FindAllDocuments(std::execution::seq, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             return lhs.relevance > rhs.relevance
                 || (abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                  DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate, class ExecutionPolicy>
vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
                                  DocumentPredicate document_predicate) const {

        std::vector<Document> matched_documents;

        ConcurrentMap<int, double> document_to_relevance(100);

        std::for_each(policy,
                query.plus_words.begin(), query.plus_words.end(),
                [this, &document_to_relevance, &document_predicate](std::string_view word)
                {
                    if (word_to_document_freqs_.count(word) != 0) {
                        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                            const auto& document_data = documents_.at(document_id);
                            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                                document_to_relevance[document_id] += term_freq * inverse_document_freq;
                            }
                         }
                     }
                }
        );

        std::for_each(policy,
                query.minus_words.begin(), query.minus_words.end(),
                [this, &document_to_relevance](std::string_view word)
                {
                    if (word_to_document_freqs_.count(word) != 0)
                    {
                        for (const auto [document_id, _] : word_to_document_freqs_.at(word))
                        {
                            document_to_relevance.Erase(document_id);
                        }
                    }
                }
        );

        for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
        {
            matched_documents.emplace_back(
                Document ( document_id, relevance, documents_.at(document_id).rating )
            );
        }

        return matched_documents;
    }



template <typename DocumentPredicate, class ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query,
                                  DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(policy, matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance
                         || (abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <class ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentStatus status) const
{
    return FindTopDocuments(policy,
        raw_query, [status](int, DocumentStatus document_status, int) {
            return document_status == status;
        });
}

template <class ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
