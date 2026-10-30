// Cppify — re2c-generated lexer (roadmap 4.1). Same interface and, by test,
// the same output as the hand-written Lexer; see src/lexer/lexer_re2c.re.
#pragma once
#include "diagnostic.h"
#include "preprocessor.h"
#include "token.h"
#include <string>
#include <vector>

namespace minic {

class Re2cLexer {
public:
    Re2cLexer(std::string source, std::string filename);
    std::vector<Token> tokenize();   // always ends with END_OF_FILE
    const std::vector<Diagnostic>& errors() const { return errors_; }

private:
    std::string src_;
    std::string file_;
    std::vector<Diagnostic> errors_;
    Preprocessor pp_;
    const char* cur_ = nullptr;        // next unread character
    const char* lineStart_ = nullptr;  // first character of cur_'s line
    int line_ = 1;

    void moveTo(const char* p);        // advance cur_, counting newlines passed
    bool atLineStart(const char* p) const;
    void directive(const char* p, int line, int col);
    void expandMacro(const std::string& name, int line, int col, std::vector<Token>& out);
};

}  // namespace minic
