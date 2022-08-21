#include "search_server.h"

using namespace std::string_literals;

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    
    const auto words = SplitIntoWordsNoStop(document);
    

    const size_t size = words.size();

    double inv_word_count = 0.0;
    if (size != 0)
        inv_word_count = 1.0 / size;

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
    
    std::set<std::string_view> s;
    for (const auto w : words)
        s.emplace(w);

    if (words_to_id_.count(s) == 0) {
        words_to_id_[s].insert({ document_id });
    }
    else {
        words_to_id_.at(s).insert(document_id);
    } 

    for (const std::string_view word : words) {
        word_to_document_freqs_[std::string{ word }][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
}
void SearchServer::RemoveDocument(int document_id) {

    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }

    auto& word_freq = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> temp;
    temp.resize(word_freq.size());

    std::transform(std::execution::seq, word_freq.begin(), word_freq.end(), temp.begin(),
        [&](const std::pair<std::string_view, double>& pointer) {
            return (pointer.first);
        });

    std::for_each(std::execution::seq, temp.begin(), temp.end(), [&](std::string_view word) {
        word_to_document_freqs_[std::string{ word }].erase(document_id);
        });


    document_to_word_freqs_.erase(document_id);

    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy p, int document_id) {
    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    auto& word_freq = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> temp;
    temp.resize(word_freq.size());

    std::transform(std::execution::seq, word_freq.begin(), word_freq.end(), temp.begin(),
        [&](const std::pair<std::string_view, double>& pointer) {
            return (pointer.first);
        });

    std::for_each(std::execution::seq, temp.begin(), temp.end(), [&](std::string_view word) {
        word_to_document_freqs_[std::string{ word }].erase(document_id);
        });


    document_to_word_freqs_.erase(document_id);

    documents_.erase(document_id);
    document_ids_.erase(document_id);
}
void SearchServer::RemoveDocument(std::execution::parallel_policy p, int document_id) {
    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    auto& word_freq = document_to_word_freqs_.at(document_id);
    std::vector<const std::string_view*> temp;
    temp.reserve(word_freq.size());

    for (const auto [w, __] : word_freq) {
        temp.push_back(&w);
    }

    std::for_each(std::execution::par, temp.begin(), temp.end(), [&](const std::string_view* word) {
        word_to_document_freqs_[std::string{* word }].erase(document_id);
        });

    document_to_word_freqs_.erase(document_id);

    documents_.erase(document_id);
    document_ids_.erase(document_id);
}


std::set<int> SearchServer::GetDuplicates() const {
    std::set<int> res;

    for (const auto& [w, id] : words_to_id_) {
        if (id.size() > 1) {
            for (auto i = ++id.begin(); i != id.end(); ++i) {
                res.insert(*i);
            }
        }
    }

    return res;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
    const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
    const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
    const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
    const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    return (int)documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, false);
    if (query.plus_words.empty())
    {
        throw std::invalid_argument("invalid argument");
        return { {}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string{ word }).count(document_id)) {
            //matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string{ word }).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy p, const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, false);
    if (query.plus_words.empty())
    {
        throw std::invalid_argument("invalid argument");
        return { {}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string{ word }).count(document_id)) {
            //matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string{ word }).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    
    return { matched_words, documents_.at(document_id).status };
}
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy p, const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, true);
    if (query.plus_words.empty())
    {
        throw std::invalid_argument("invalid argument");
        return { {}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());

    if (std::any_of(p, query.minus_words.begin(), query.minus_words.end(), [&](auto& word) {
        return word_to_document_freqs_.count(std::string{ word }) != 0 && word_to_document_freqs_.at(std::string{ word }).count(document_id); })) {
        matched_words.clear();
        return { {}, documents_.at(document_id).status };
    }
    auto end = std::copy_if(p, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [&](auto& word) {
            return word_to_document_freqs_.count(std::string{ word }) != 0 && word_to_document_freqs_.at(std::string{ word }).count(document_id);
        });
    matched_words.resize(end - matched_words.begin());

    std::sort(p, matched_words.begin(), matched_words.end());
    end = std::unique(p, matched_words.begin(), matched_words.end());
    matched_words.resize(end - matched_words.begin());

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + (std::string)word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }

    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + (std::string)text + " is invalid"s);
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool is_par) const {
    Query result;

    if (!is_par) {
        std::set<std::string_view> plus, minus;
        for (const std::string_view word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    minus.insert(query_word.data);
                }
                else {
                    plus.insert(query_word.data);
                }
            }
        }

        for (const auto i : minus)
            result.minus_words.push_back(i);
        for (const auto i : plus)
            result.plus_words.push_back(i);
    }
    else {
        for (const std::string_view word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.push_back(query_word.data);
                }
                else {
                    result.plus_words.push_back(query_word.data);
                }
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(std::string{ word }).size());
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}
std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    //std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    static std::map<std::string_view, double> result;
    result.clear();

    for (const auto [word, id_freq] : word_to_document_freqs_) {
        for (const auto [id, freq] : id_freq) {
            if (id == document_id) {
                //std::pair<std::string, double> r = { word, freq };
                result.insert({ word, freq });
            }
        }
    }

    return result;
}