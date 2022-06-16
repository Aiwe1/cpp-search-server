#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <numeric>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double DELTA = 1e-5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        for (const char c : str)
            if (c >= 0 && c <= 31)
            {
                throw invalid_argument("Не допустимые символы в стоп словах.");
            }

        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    // Defines an invalid document id
    // You can refer this constant as SearchServer::INVALID_DOCUMENT_ID
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) 
    {
        
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("не верный ID");
            return;
        }
        vector<string> words;
        if (!SplitIntoWordsNoStop(document, words)) {
            throw invalid_argument("не верные слова в документе");
            return;
        }

        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        document_ids_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const
    {
        vector<Document> result;
        //result.clear();
        Query query;
        if (!ParseQuery(raw_query, query)) {
            throw invalid_argument("не допустимый запрос.");
            return result; /////!!!!!!!!!!!
        }
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        // Exchange matched_documents and result instead of deep copying
        result.swap(matched_documents);
        return result;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query,
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (index >= 0 && index < GetDocumentCount()) {
            return document_ids_[index];
        }
        throw out_of_range("индекс выходит за пределы.");
        return INVALID_DOCUMENT_ID;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        tuple<vector<string>, DocumentStatus> result;
        // Empty result by initializing it with default constructed tuple
        result = {};
        Query query;
        if (!ParseQuery(raw_query, query)) {
            throw invalid_argument("не допустимый запрос.");
            return result; ///!!!!!!!!!!!
        }
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        result = { matched_words, documents_.at(document_id).status };
        return result;
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    [[nodiscard]] bool SplitIntoWordsNoStop(const string& text, vector<string>& result) const {
        result.clear();
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                return false;
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        result.swap(words);
        return true;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    [[nodiscard]] bool ParseQueryWord(string text, QueryWord& result) const {
        // Empty result by initializing it with default constructed QueryWord
        result = {};

        if (text.empty()) {
            return false;
        }
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
            return false;
        }

        result = QueryWord{ text, is_minus, IsStopWord(text) };
        return true;
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    [[nodiscard]] bool ParseQuery(const string& text, Query& result) const {
        // Empty result by initializing it with default constructed Query
        result = {};
        for (const string& word : SplitIntoWords(text)) {
            QueryWord query_word;
            if (!ParseQueryWord(word, query_word)) {
                return false;
            }
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(query_word.data);
                }
                else {
                    result.plus_words.insert(query_word.data);
                }
            }
        }
        return true;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

// ------- МАКРОСЫ ДЛЯ ТЕСТА ----------
template <typename T>
void RunTestImpl(T t, const string& s) {
    t();
    cerr << s << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server1(""s);
    server1.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server1.FindTopDocuments("in"s);
    ASSERT(found_docs.size() == 1);
    const Document& doc0 = found_docs[0];
    ASSERT(doc0.id == doc_id);

    SearchServer server2("in the"s);
    server2.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT(server2.FindTopDocuments("in"s).empty());

}
void TestExcludeMinusWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "cat and dog in the home"s;

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat -city"s);
    ASSERT(found_docs.size() == 1);
    const Document& doc0 = found_docs[0];
    ASSERT(doc0.id == doc_id2);
}

//При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestFindDocumentWithSearchWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto [a1, b1] = server.MatchDocument("cat -city"s, doc_id);
    ASSERT(a1.size() == 0);

    const auto [a2, b2] = server.MatchDocument("cat -dog"s, doc_id);

    ASSERT(a2.size() == 1);
}

//  Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortRating() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "cat and dog in the home"s;

    const int doc_id3 = 44;
    const string content3 = "in the box"s;

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat dog box"s);
    ASSERT(found_docs.size() == 3);
    ASSERT(found_docs[0].id == doc_id3);
    ASSERT(found_docs[1].id == doc_id2);
    ASSERT(found_docs[2].id == doc_id);
}

//  Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestComputeAverageRating() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 11 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("cat dog"s);
    ASSERT(found_docs[0].rating == (1 + 2 + 11) / 3);
}
// Поиск документов, имеющих заданный статус.

void TestStatus() {

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "cat and dog in the home"s;
    const vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const string content3 = "cat and dog in the black box"s;
    const vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings);
    const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
    ASSERT(found_docs.size() == 1);
    ASSERT(found_docs[0].id == 43);
    const auto found_docs2 = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
    ASSERT(found_docs.size() == 1);
}
// Корректное вычисление релевантности найденных документов

void TestCountingRelevansIsCorrect() {

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "cat and dog in the home"s;
    const vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const string content3 = "cat and dog in the black box"s;
    const vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL);
    ASSERT(found_docs[0].relevance == 0);

    const auto found_docs1 = server.FindTopDocuments("dog"s, DocumentStatus::ACTUAL);
    ASSERT(found_docs1[0].relevance - (double)found_docs1.size() / 3.0 < DELTA);
}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestPredicate() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "cat and dog in the home"s;
    const vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const string content3 = "cat in the box"s;
    const vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating)
        { return document_id % 2 == 0; });

    ASSERT(found_docs[0].id == 42);
    ASSERT(found_docs[1].id == 44);
}

void TestDocuments() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "cat and dog in the home"s;
    const vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const string content3 = "cat in the box"s;
    const vector<int> ratings3 = { 4, 4, 4 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings);

    const auto [a1, b1] = server.MatchDocument("cat city"s, doc_id);
    ASSERT_EQUAL((a1.size()), 2);
    ASSERT(a1[0] == "cat"s);
    ASSERT(a1[1] == "city"s);
    const auto [a2, b2] = server.MatchDocument("cat -dog"s, doc_id2);
    ASSERT_EQUAL((a2.size()), 0);
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



// ------------ Пример использования ----------------

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

int main() {
    setlocale(LC_ALL, "Russian");
    TestSearchServer();
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);
}