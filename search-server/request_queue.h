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
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        auto result = ss.FindTopDocuments(raw_query, document_predicate);

        QueryResult q;
        q.is_empty = true;
        if (!result.empty())
            q.is_empty = false;

        requests_.push_back(q);

        ++all;
        if (all > min_in_day_)
        {
            if (!requests_.front().is_empty && q.is_empty)
            {
                ++empty_;
            }
            else if (requests_.front().is_empty && !q.is_empty)
            {
                --empty_;
            }
            requests_.pop_front();
        }
        else {
            if (q.is_empty)
                ++empty_;
        }

        return result;
    }

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
    int all = 0;
    int empty_ = 0;
    const SearchServer& ss;
    // возможно, здесь вам понадобится что-то ещё
};