#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& server)
{
    set<int> remove_id;
    set<set<string>> words_docs;
    for ( auto& it : server)
    {
        if ( remove_id.count(it.first) > 1 ) continue;

        set<string> words_doc;
        for ( const auto& [word, r] : server.GetWordFrequencies(it.first) )
        {
            words_doc.insert(word);
        }
        if ( words_docs.count(words_doc) > 0 )
        {
            remove_id.insert(it.first);
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
