#pragma once

#include "search_server.h"

void RemoveDuplicates(SearchServer& search_server) {
    for (const int document_id : search_server.GetDuplicates()) {
        search_server.RemoveDocument(document_id);
        std::cout << "Found duplicate document id "s << document_id << std::endl;
    }
}