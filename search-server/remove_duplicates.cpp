#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& server)
{
    set<int> remove_id;
    set<set<string>> words_docs;
    for (std::vector<int>::iterator it = server.begin(); it != server.end(); ++it )
    {
        if ( remove_id.count(*it) > 1 ) continue;

        set<string> words_doc;
        for ( auto& [word, r] : server.GetWordFrequencies(*it) )
        {
            words_doc.insert(word);
        }
        if ( words_docs.count(words_doc) > 0 )
        {
            remove_id.insert(*it);
        }
        else
        {
            words_docs.insert(words_doc);
        }
    }

    for ( auto& id : remove_id )
    {
        std::cout << "Found duplicate document id "s << id << std::endl;
        server.RemoveDocument(id);
    }
}
