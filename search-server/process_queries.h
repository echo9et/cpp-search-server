#pragma once
#include <list>
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& server,const std::vector<std::string> queries);
std::vector<Document> ProcessQueriesJoined(const SearchServer& server,const std::vector<std::string>& queries);
