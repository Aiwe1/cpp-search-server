#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    auto result = ss.FindTopDocuments(raw_query, status);

    CountRequests(!result.empty());

    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    auto result = ss.FindTopDocuments(raw_query);

    CountRequests(!result.empty());

    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_requests_;
}

void RequestQueue::CountRequests(bool empty_find)
{
    QueryResult result_empty;
    result_empty.is_empty = true;
    if (empty_find)
        result_empty.is_empty = false;

    requests_.push_back(result_empty);

    ++all_requests_;
    if (all_requests_ > min_in_day_)
    {
        if (!requests_.front().is_empty && result_empty.is_empty)
        {
            ++empty_requests_;
        }
        else if (requests_.front().is_empty && !result_empty.is_empty)
        {
            --empty_requests_;
        }
        requests_.pop_front();
    }
    else {
        if (result_empty.is_empty)
            ++empty_requests_;
    }

}