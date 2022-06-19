#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    auto result = ss.FindTopDocuments(raw_query, status);

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

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    auto result = ss.FindTopDocuments(raw_query);

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

int RequestQueue::GetNoResultRequests() const {
    return empty_;
}