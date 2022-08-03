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

#include "log_duration.h"
#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"

// ------- МАКРОСЫ ДЛЯ ТЕСТА ----------
using namespace std::string_literals;
const double DELTA = 1e-5;

template <typename T>
void RunTestImpl(T t, const std::string& s) {
    t();
    std::cerr << s << " OK" << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": ";
        std::cout << "ASSERT_EQUAL("s << t_str << ", " << u_str << ") failed: ";
        std::cout << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cout << " Hint: " << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cout << file << "(" << line << "): " << func << ": ";
        std::cout << "ASSERT(" << expr_str << ") failed.";
        if (!hint.empty()) {
            std::cout << " Hint: " << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    SearchServer server1(""s);
    server1.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server1.FindTopDocuments("in");
    ASSERT(found_docs.size() == 1);
    const Document& doc0 = found_docs[0];
    ASSERT(doc0.id == doc_id);

    SearchServer server2("in the"s);
    server2.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT(server2.FindTopDocuments("in").empty());

}
void TestExcludeMinusWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const std::string content2 = "cat and dog in the home";

    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -city");
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id2);
    }

}

//При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestFindDocumentWithSearchWords() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto [a1, b1] = server.MatchDocument("cat -city", doc_id);
    ASSERT(a1.size() == 0);

    const auto [a2, b2] = server.MatchDocument("cat -dog", doc_id);

    ASSERT(a2.size() == 1);
}

//  Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortRating() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const std::string content2 = "cat and dog in the home";

    const int doc_id3 = 44;
    const std::string content3 = "in the box";

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat dog box");
    ASSERT(found_docs.size() == 3);
    ASSERT(found_docs[0].id == doc_id3);
    ASSERT(found_docs[1].id == doc_id2);
    ASSERT(found_docs[2].id == doc_id);
}

//  Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestComputeAverageRating() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 11 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("cat dog");
    ASSERT(found_docs[0].rating == (1 + 2 + 11) / 3);
}
// Поиск документов, имеющих заданный статус.

void TestStatus() {

    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const std::string content2 = "cat and dog in the home";
    const std::vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const std::string content3 = "cat and dog in the black box";
    const std::vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings);
    const auto found_docs = server.FindTopDocuments("cat", DocumentStatus::IRRELEVANT);
    ASSERT(found_docs.size() == 1);
    ASSERT(found_docs[0].id == 43);
    const auto found_docs2 = server.FindTopDocuments("cat", DocumentStatus::REMOVED);
    ASSERT(found_docs.size() == 1);
}
// Корректное вычисление релевантности найденных документов

void TestCountingRelevansIsCorrect() {

    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const std::string content2 = "cat and dog in the home";
    const std::vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const std::string content3 = "cat and dog in the black box";
    const std::vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("cat", DocumentStatus::ACTUAL);
    ASSERT(found_docs[0].relevance == 0);

    const auto found_docs1 = server.FindTopDocuments("dog", DocumentStatus::ACTUAL);
    ASSERT(found_docs1[0].relevance - (double)found_docs1.size() / 3.0 < DELTA);
}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestPredicate() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const std::string content2 = "cat and dog in the home";
    const std::vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const std::string content3 = "cat in the box";
    const std::vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("cat", [](int document_id, DocumentStatus status, int rating)
        { return document_id % 2 == 0; });

    ASSERT(found_docs[0].id == 42);
    ASSERT(found_docs[1].id == 44);
}

void TestDocuments() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const std::string content2 = "cat and dog in the home";
    const std::vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const std::string content3 = "cat in the box";
    const std::vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

    const auto [a1, b1] = server.MatchDocument("cat -dog", doc_id);
    ASSERT(a1.size() > 0);
    ASSERT(a1[0] == "cat");
    const auto [a2, b2] = server.MatchDocument("cat -dog", doc_id2);
    ASSERT(a2.size() == 0);
}

void TestSearchServer() {
    RUN_TEST(TestDocuments);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestFindDocumentWithSearchWords);

    RUN_TEST(TestSortRating);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestStatus);
    RUN_TEST(TestCountingRelevansIsCorrect);
}
// --------- Окончание модульных тестов поисковой системы -----------
