#include "process_queries.h"
#include <execution>
#include <algorithm>

struct document_size{
    document_size() { size = 0u;}
    void operator()(const vector<Document>& answer) { size += answer.size(); }
    size_t size;
};
size_t getSizeDocs(const std::vector<std::vector<Document>> & docs){
    return std::for_each(docs.begin(), docs.end(), document_size()).size;
}

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& server, const std::vector<std::string> queries)
{
    std::vector<std::vector<Document>> docs(queries.size());
    std::transform(std::execution::par,
                   queries.begin(),
                   queries.end(),
                   docs.begin(),
                   [&server](auto& query){
        return server.FindTopDocuments(query);}
    );
    return docs;
}

std::vector<Document> ProcessQueriesJoined(
        const SearchServer& server,
        const std::vector<std::string>& queries){


    const std::vector<std::vector<Document>> docs(ProcessQueries(server, queries));

    std::vector<Document> out_docs;
    out_docs.reserve(getSizeDocs(docs));

    for ( const auto& ansewer : docs )
        for ( const auto& doc : ansewer)
            out_docs.push_back(std::move(doc));
    return out_docs;
}
