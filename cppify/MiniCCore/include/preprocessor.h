// Cppify — the preprocessor state both lexers share (roadmap 4.1)
//
// Scanning (which characters make a token) is what the hand-written lexer
// and the re2c-generated one each do their own way. What a directive MEANS
// is not scanning: the macro table, the #if stack and the #if condition
// language are kept here, once, and both lexers hand it each directive line.
// So comparing the two lexers compares their scanning, and a fix to the
// directive language reaches both.
#pragma once
#include "diagnostic.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace minic {

class Preprocessor {
public:
    Preprocessor();   // with the predefined macros (NULL, INT_MAX, EOF, ...)

    // Inside a false #if branch: the lexer drops every line except directives.
    bool skipping() const { return !conds_.empty() && !conds_.back().active; }

    // One directive line as the lexer collected it: from the '#', with
    // backslash-newlines and comments each replaced by a space. (line, col)
    // is where the '#' is, for diagnostics.
    void directive(const std::string& text, int line, int col, std::vector<Diagnostic>& errors);

    // At end of input: every #if still open is an error.
    void finish(std::vector<Diagnostic>& errors);

    bool isMacro(const std::string& name) const { return macros_.count(name) > 0; }
    const std::string& body(const std::string& name) const { return macros_.at(name); }

    // The state a nested lexer expanding `name` starts from: the same macros
    // except `name` itself (so `#define X X + 1` cannot recurse), no open #if.
    Preprocessor forExpansionOf(const std::string& name) const;

private:
    struct CondFrame { bool active; bool taken; bool sawElse; int line, col; };
    std::unordered_map<std::string, std::string> macros_;
    std::vector<CondFrame> conds_;

    bool parentActive() const { return conds_.size() < 2 || conds_[conds_.size() - 2].active; }
    int  evalIfCondition(const std::string& expr) const;   // 1 true, 0 false, -1 unsupported
};

}  // namespace minic
