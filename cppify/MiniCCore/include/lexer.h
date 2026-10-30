// MiniC-Opt — hand-written DFA lexical analyser
// Owner: Member 1 (Front End)
//
// No generator (no Flex). The scanner is an explicit character-driven DFA
// applying the longest-match (maximal munch) rule, with a keyword table
// consulted after an identifier is recognised.
#pragma once
#include "token.h"
#include "diagnostic.h"
#include "preprocessor.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace minic {

class Lexer {
public:
    Lexer(std::string source, std::string filename);

    // Scan the whole input. Always terminates with an END_OF_FILE token.
    std::vector<Token> tokenize();

    const std::vector<Diagnostic>& errors() const { return errors_; }

private:
    std::string src_;
    std::string file_;
    size_t      pos_  = 0;
    int         line_ = 1;
    int         col_  = 1;
    std::vector<Diagnostic> errors_;

    // Macros and conditional compilation (shared with the re2c lexer).
    // A macro is expanded token by token at the point of use, never
    // textually, so it can't change how a literal or identifier is split.
    Preprocessor pp_;
    void defineMacroFromLine();
    void expandMacro(const std::string& name, int l, int c, std::vector<Token>& out);

    char peek(size_t off = 0) const;
    char advance();
    bool match(char expected);
    void skipWhitespaceAndComments();
    bool onlyBlanksBeforeOnThisLine() const;
    void error(int l, int c, const std::string& msg);

    Token make(Tok t, const std::string& lex, int l, int c) const;
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanCharLiteral();
    Token scanStringLiteral();
    Token scanOperatorOrPunct();
};

} // namespace minic
