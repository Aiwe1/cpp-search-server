// search_server_s4_t2_solution.cpp

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

#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }";
    return out;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }" << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "
        << "document_id = " << document_id << ", "
        << "status = " << static_cast<int>(status) << ", "
        << "words =";
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка добавления документа " << document_id << ": " << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "Результаты поиска по запросу: " << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка поиска: " << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "Матчинг документов по запросу: " << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Ошибка матчинга документов на запрос " << query << ": " << e.what() << std::endl;
    }
}

int main() {
    SearchServer search_server(std::string("and in at"));
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, std::string("curly cat curly tail"), DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, std::string("curly dog and fancy collar"), DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, std::string("big cat fancy collar "), DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, std::string("big dog sparrow Eugene"), DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, std::string("big dog sparrow Vasiliy"), DocumentStatus::ACTUAL, { 1, 1, 1 });

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest(std::string("empty request"));
    }
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest(std::string("curly dog"));
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest(std::string("big collar"));
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest(std::string("sparrow"));
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    return 0;
}