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

#include "log_duration.h"
#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "test_example_functions.h"

using namespace std::string_literals;

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument& e) {
        std::cout << "������ ���������� ��������� "s << document_id << ": " << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "���������� ������ �� �������: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "������ ������: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "������� ���������� �� �������: "s << query << std::endl;
        /*
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        } */

        for (const auto& id : search_server){
            const auto [words, status] = search_server.MatchDocument(query, id);
            PrintMatchDocumentResult(id, words, status);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "������ �������� ���������� �� ������ "s << query << ": "s << e.what() << std::endl;
    }
} 

void RemoveDuplicates(SearchServer& search_server) {
    for (const int document_id : search_server.GetDuplicates()) {
        search_server.RemoveDocument(document_id);
        std::cout << "Found duplicate document id "s << document_id << std::endl;
    }
} 

int main() {
    TestSearchServer();
    SearchServer search_server("and with"s);
    //SearchServer search_server(""s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });
    //search_server.RemoveDocument(1);
    // �������� ��������� 2, ����� �����
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ������� ������ � ����-������, ������� ����������
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ��������� ���� ����� ��, ������� ���������� ��������� 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ���������� ����� �����, ���������� �� ��������
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ��������� ���� ����� ��, ��� � id 6, �������� �� ������ �������, ������� ����������
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ���� �� ��� �����, �� �������� ����������
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ����� �� ������ ����������, �� �������� ����������
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });
    
    std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server);
    std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    
    //TestSearchServer();
}

/*
int main() {

    LOG_DURATION_STREAM("Operation time", std::cout);
    SearchServer search_server(std::string("and in at"));
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, std::string("curly cat curly tail"), DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, std::string("curly dog and fancy collar"), DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, std::string("big cat fancy collar "), DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, std::string("big dog sparrow Eugene"), DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, std::string("big dog sparrow Vasiliy"), DocumentStatus::ACTUAL, { 1, 1, 1 });
    search_server.AddDocument(6, std::string("curly cat curly tail"), DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(7, std::string("curly cat curly tail"), DocumentStatus::ACTUAL, { 7, 2, 7 });

    
    RemoveDuplicates(search_server);

    // 1439 �������� � ������� �����������
   // for (int i = 0; i < 1439; ++i) {
   //     request_queue.AddFindRequest(std::string("empty request"));
   // }
    
    {
        auto& aa = search_server.GetWordFrequencies(1);
        if (!aa.empty()) {
            for (const auto& i : aa) {
                std::cout << i.first << i.second << std::endl;
            }
        }
    }

    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    // ��� ��� 1439 �������� � ������� �����������
    request_queue.AddFindRequest(std::string("curly dog"));
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    // ����� �����, ������ ������ ������, 1438 �������� � ������� �����������
    request_queue.AddFindRequest(std::string("big collar"));
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    // ������ ������ ������, 1437 �������� � ������� �����������
    request_queue.AddFindRequest(std::string("sparrow"));
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    return 0;
} */