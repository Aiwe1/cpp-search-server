#pragma once

#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <deque>

#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : ss(search_server) {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        // определите, что должно быть в структуре
        bool is_empty;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int all_requests_ = 0;
    int empty_requests_ = 0;
    const SearchServer& ss;
    // возможно, здесь вам понадобится что-то ещё
    void CountRequests(bool empty_find);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto result = ss.FindTopDocuments(raw_query, document_predicate);

    CountRequests(!result.empty());

    return result;
}
