#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& server)
{
    std::set<int> remove_id;
    std::set<std::set<string>> words_docs;
    for ( const int id : server)
    {
        if ( remove_id.count(id) > 1 ) continue;

        std::set<string> words_doc;
        for ( const auto& [word, r] : server.GetWordFrequencies( id ) )
        {
            words_doc.insert(word.data());
        }
        if ( words_docs.count(words_doc) > 0 )
        {
            remove_id.insert(id);
        }
        else
        {
            words_docs.insert(words_doc);
        }
    }

    for ( const auto& id : remove_id )
    {
        std::cout << "Found duplicate document id "s << id << std::endl;
        server.RemoveDocument(id);
    }
}
