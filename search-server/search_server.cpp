#include "search_server.h"

using namespace std::string_literals;

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    size_t size = words.size();
    if (size == 0)
        return;

    const double inv_word_count = 1.0 / size;

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);

    std::set<std::string> s;
    for (const auto& w : words)
        s.emplace(w);

    if (!words_to_id_.count(s))
    {
        std::set<int> n;
        n.insert(document_id);
        words_to_id_[s].insert({ document_id });
    }
    else {
        words_to_id_.at(s).insert(document_id);
    }    
    // я пробовал сделать так до этого, но почему то компил€тор в тренажере ругалс€ на любые действи€ по типу words_to_id_.count(<set>)

    for (const std::string& word : words) {   
        word_to_document_freqs_[word][document_id] += inv_word_count;
        //document_to_word_freqs_[document_id].insert(word);
    }
}

/*void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    } 
    const auto words = SplitIntoWordsNoStop(document);

    size_t size = words.size();
    if (size == 0)
        return;

    const double inv_word_count = 1.0 / size;

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);

    std::set<std::string> s;
    for (const auto& w : words)
        s.emplace(w); 
    
    // я пробовал сделать map<set<string>, set<int>> duplicate words_id Ќо почему то компил€тор в тренажЄре не может искать по сету слов, в отличии от моего
for (auto& [id, w] : document_to_word_freqs_) {
    if (w == s) {
        bool flag = true;
        //for (auto i : duplicates_id) {
        int duplicate = 0;
        for (int i = 0; i < duplicates_id.size(); ++i) {
            if (duplicates_id[i].count(id) > 0) {
                duplicate = i;

                flag = false;
                break;
            }
        }
        if (flag) {
            duplicates_id.push_back({ document_id, id });
        }
        else {
            duplicates_id[duplicate].insert(document_id);
        }

        break;
    }
}
document_to_word_freqs_.emplace(document_id, s);

for (const std::string& word : words) {
    word_to_document_freqs_[word][document_id] += inv_word_count;
    //document_to_word_freqs_[document_id].insert(word);
}
}*/

void SearchServer::RemoveDocument(int document_id) {
    
    if ((document_id < 0) || (documents_.count(document_id) == 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }

    for (auto [w, ids] : words_to_id_) {
        if (ids.count(document_id) > 0)
        {
            if (ids.size() == 1)
            {
                words_to_id_.erase(w);
                break;
            }
            if (ids.size() > 1)
            {
                ids.erase(document_id);
                break;
            }
        }
    }

    std::vector<std::string> v;
    for (auto& [j, ii] : word_to_document_freqs_) {
        ii.erase(document_id);
        if (word_to_document_freqs_.at(j).empty())
            v.push_back(j);
    }
    for (const auto& s : v)
        word_to_document_freqs_.erase(s);

    documents_.erase(document_id);
    document_ids_.erase(document_id); 
}

std::set<int> SearchServer::GetDuplicates() const {
    std::set<int> res;
    
    for (const auto& [w, id] : words_to_id_) {
        if (id.size() > 1){
            for (auto i = ++id.begin(); i != id.end(); ++i) {
                res.insert(*i);
            }
        }
    } 
    
    return res; 
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return (int)documents_.size();
}
/*
int SearchServer::GetDocumentId(int index) const {
    return document_ids_.at(index);
} */

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + word + " is invalid"s);
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

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + text + " is invalid"s);
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

std::set<int>::const_iterator SearchServer::begin() const{
    return document_ids_.begin();
}
std::set<int>::const_iterator SearchServer::end() const{
    return document_ids_.end();
} 

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    //std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    static std::map<std::string, double> result;
    result.clear();

    for (const auto& [word, id_freq] : word_to_document_freqs_) {
        for (const auto& [id, freq] : id_freq) {
            if (id == document_id) {
                //std::pair<std::string, double> r = { word, freq };
                result.insert({ word, freq });
            }
        }
    }

    return result;
}