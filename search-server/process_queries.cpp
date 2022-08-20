#include "process_queries.h"
#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> res;
    res.resize(queries.size());
    /*
    for (const std::string& query : queries) {
        res.push_back(search_server.FindTopDocuments(query));
    } */

    std::transform(std::execution::par, queries.begin(), queries.end(), res.begin(), [&search_server](const std::string& query) {
        return search_server.FindTopDocuments(query);
        });

    return res;
}
std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    std::list<Document> res;
    //res.resize(queries.size());

    for (auto& a : ProcessQueries(search_server, queries)) {
        for (auto& b : a) {
            res.push_back(b);
        }
    }

    return res;
}
