// Cppify — a minimal JSON reader, for --config files only.
//
// The project takes no third-party dependencies, and a pipeline file needs
// objects, arrays, strings, integers and booleans, nothing more. Numbers are
// read as integers; anything malformed is an error with its byte offset,
// never a guess.
#pragma once
#include <cctype>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace minic {

struct Json {
    enum class Kind { NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT } kind = Kind::NUL;
    bool b = false;
    long long num = 0;
    std::string str;
    std::vector<Json> arr;
    std::vector<std::pair<std::string, Json>> obj;   // keeps the file's key order

    const Json* get(const std::string& key) const {
        for (const auto& kv : obj) if (kv.first == key) return &kv.second;
        return nullptr;
    }
};

class JsonReader {
public:
    explicit JsonReader(const std::string& text) : s_(text) {}

    // Parses the whole text as one value. On failure returns false and sets err.
    bool parse(Json& out, std::string& err) {
        bool ok = value(out);
        if (ok) {
            ws();
            if (i_ != s_.size()) ok = fail("trailing characters");
        }
        if (!ok) err = err_ + " at byte " + std::to_string(i_);
        return ok;
    }

private:
    const std::string& s_;
    size_t i_ = 0;
    std::string err_;

    bool fail(const std::string& m) { if (err_.empty()) err_ = m; return false; }
    void ws() { while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_; }
    bool lit(const char* w) {
        size_t n = std::char_traits<char>::length(w);
        if (s_.compare(i_, n, w) != 0) return false;
        i_ += n;
        return true;
    }

    bool value(Json& v) {
        ws();
        if (i_ >= s_.size()) return fail("unexpected end of input");
        char c = s_[i_];
        if (c == '{') return object(v);
        if (c == '[') return array(v);
        if (c == '"') { v.kind = Json::Kind::STRING; return string(v.str); }
        if (lit("true"))  { v.kind = Json::Kind::BOOL; v.b = true;  return true; }
        if (lit("false")) { v.kind = Json::Kind::BOOL; v.b = false; return true; }
        if (lit("null"))  { v.kind = Json::Kind::NUL; return true; }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            size_t start = i_;
            if (c == '-') ++i_;
            while (i_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[i_]))) ++i_;
            if (i_ < s_.size() && (s_[i_] == '.' || s_[i_] == 'e' || s_[i_] == 'E'))
                return fail("only integer numbers are accepted");
            if (i_ == start + (c == '-' ? 1 : 0)) return fail("malformed number");
            v.kind = Json::Kind::NUMBER;
            v.num = std::stoll(s_.substr(start, i_ - start));
            return true;
        }
        return fail(std::string("unexpected character '") + c + "'");
    }

    bool string(std::string& out) {
        ++i_;   // opening quote
        while (i_ < s_.size() && s_[i_] != '"') {
            char c = s_[i_++];
            if (c != '\\') { out += c; continue; }
            if (i_ >= s_.size()) break;
            char e = s_[i_++];
            switch (e) {
                case '"': case '\\': case '/': out += e; break;
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                default: return fail("unsupported escape in string");
            }
        }
        if (i_ >= s_.size()) return fail("unterminated string");
        ++i_;
        return true;
    }

    bool array(Json& v) {
        v.kind = Json::Kind::ARRAY;
        ++i_;
        ws();
        if (i_ < s_.size() && s_[i_] == ']') { ++i_; return true; }
        for (;;) {
            Json e;
            if (!value(e)) return false;
            v.arr.push_back(std::move(e));
            ws();
            if (i_ < s_.size() && s_[i_] == ',') { ++i_; continue; }
            if (i_ < s_.size() && s_[i_] == ']') { ++i_; return true; }
            return fail("expected ',' or ']'");
        }
    }

    bool object(Json& v) {
        v.kind = Json::Kind::OBJECT;
        ++i_;
        ws();
        if (i_ < s_.size() && s_[i_] == '}') { ++i_; return true; }
        for (;;) {
            ws();
            if (i_ >= s_.size() || s_[i_] != '"') return fail("expected a key");
            std::string key;
            if (!string(key)) return false;
            ws();
            if (i_ >= s_.size() || s_[i_] != ':') return fail("expected ':'");
            ++i_;
            Json e;
            if (!value(e)) return false;
            v.obj.emplace_back(std::move(key), std::move(e));
            ws();
            if (i_ < s_.size() && s_[i_] == ',') { ++i_; continue; }
            if (i_ < s_.size() && s_[i_] == '}') { ++i_; return true; }
            return fail("expected ',' or '}'");
        }
    }
};

}  // namespace minic
