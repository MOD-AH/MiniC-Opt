// Cppify — shared preprocessor state (see include/preprocessor.h)
#include "preprocessor.h"
#include <algorithm>
#include <cctype>

namespace minic {

Preprocessor::Preprocessor() {
    // What <stddef.h>, <stdbool.h>, <limits.h> and <stdio.h> would have given.
    macros_ = {
        {"NULL", "0"}, {"true", "1"}, {"false", "0"}, {"EOF", "(-1)"},
        {"INT_MAX", "2147483647"}, {"INT_MIN", "(-2147483647-1)"},
        {"M_PI", "3.14159265358979323846"}, {"RAND_MAX", "32767"},
        {"CHAR_BIT", "8"}, {"LONG_MAX", "9223372036854775807"}, {"LLONG_MAX", "9223372036854775807"},
        {"UINT_MAX", "4294967295"}, {"DBL_MAX", "1.7976931348623157e308"}, {"FLT_MAX", "3.40282346638528859812e+38"},
        {"stdin", "((void *)0)"}, {"stdout", "((void *)1)"}, {"stderr", "((void *)2)"},
        {"EXIT_SUCCESS", "0"}, {"EXIT_FAILURE", "1"},
    };
}

Preprocessor Preprocessor::forExpansionOf(const std::string& name) const {
    Preprocessor p;
    p.macros_ = macros_;
    p.macros_.erase(name);
    return p;
}

void Preprocessor::finish(std::vector<Diagnostic>& errors) {
    for (const CondFrame& f : conds_) errors.push_back({f.line, f.col, "unterminated #if (missing #endif)"});
    conds_.clear();
}

// #include is IGNORED, not honoured: there are no headers to find and the
// built-ins are always in scope. `#define NAME text` (object-like), #undef
// and the conditional directives are honoured; anything else is dropped.
void Preprocessor::directive(const std::string& text, int dl, int dc, std::vector<Diagnostic>& errors) {
    auto error = [&](const std::string& m) { errors.push_back({dl, dc, m}); };
    size_t i = 1;                                       // skip '#'
    while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
    size_t ks = i;
    while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i]))) ++i;
    std::string kw = text.substr(ks, i - ks);
    std::string rest = text.substr(i);
    auto firstWord = [&]() {
        size_t a = rest.find_first_not_of(" \t");
        if (a == std::string::npos) return std::string();
        size_t b = a;
        while (b < rest.size() && (std::isalnum(static_cast<unsigned char>(rest[b])) || rest[b] == '_')) ++b;
        return rest.substr(a, b - a);
    };

    // Conditionals are tracked even inside a skipped region, so nesting
    // stays balanced; only an active region's #define/#undef take effect.
    if (kw == "ifdef" || kw == "ifndef" || kw == "if") {
        bool cond = false;
        if (kw == "if") {
            int v = skipping() ? 0 : evalIfCondition(rest);
            if (v < 0) error("unsupported #if condition; only an integer or defined(NAME) is understood");
            cond = v > 0;
        } else {
            cond = macros_.count(firstWord()) > 0;
            if (kw == "ifndef") cond = !cond;
        }
        bool active = !skipping() && cond;
        conds_.push_back({active, active, false, dl, dc});
        return;
    }
    if (kw == "elif" || kw == "else" || kw == "endif") {
        if (conds_.empty()) { error("#" + kw + " without a matching #if"); return; }
        CondFrame& f = conds_.back();
        if (kw == "endif") { conds_.pop_back(); return; }
        if (f.sawElse) { error("#" + kw + " after #else"); return; }
        bool cond = true;
        if (kw == "elif") {
            int v = (f.taken || !parentActive()) ? 0 : evalIfCondition(rest);
            if (v < 0) error("unsupported #elif condition; only an integer or defined(NAME) is understood");
            cond = v > 0;
        } else {
            f.sawElse = true;
        }
        f.active = parentActive() && !f.taken && cond;
        if (f.active) f.taken = true;
        return;
    }
    if (skipping()) return;
    if (kw == "undef") { macros_.erase(firstWord()); return; }
    if (kw != "define") return;
    i = ks + 6;                                         // just past "define"
    while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
    size_t s = i;
    while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) ++i;
    std::string name = text.substr(s, i - s);
    if (name.empty()) return;
    if (i < text.size() && text[i] == '(') {
        error("function-like macro '" + name + "' is not supported; use an ordinary function or an object-like #define");
        return;
    }
    while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r')) ++i;
    std::string body = text.substr(i);
    while (!body.empty() && (body.back() == ' ' || body.back() == '\t' || body.back() == '\r')) body.pop_back();
    macros_[name] = body;
}

// The two #if forms a single-file program realistically uses: an integer
// (`#if 0` to comment code out) and defined(NAME), optionally negated.
int Preprocessor::evalIfCondition(const std::string& expr) const {
    std::string e;
    for (char ch : expr) if (ch != ' ' && ch != '\t' && ch != '\r') e += ch;
    bool neg = false;
    while (!e.empty() && e[0] == '!') { neg = !neg; e.erase(0, 1); }
    int v = -1;
    if (!e.empty() && std::all_of(e.begin(), e.end(), [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); })) {
        v = std::stoll(e) != 0 ? 1 : 0;
    } else if (e.rfind("defined", 0) == 0) {
        std::string n = e.substr(7);
        if (n.size() >= 2 && n.front() == '(' && n.back() == ')') n = n.substr(1, n.size() - 2);
        bool ident = !n.empty() && std::all_of(n.begin(), n.end(), [](char ch) {
            return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_'; });
        if (ident) v = macros_.count(n) ? 1 : 0;
    }
    if (v < 0) return -1;
    return neg ? 1 - v : v;
}

}  // namespace minic
