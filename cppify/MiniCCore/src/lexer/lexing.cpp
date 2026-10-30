// Cppify — choose a lexer (see include/lexing.h)
#include "lexing.h"
#include "lexer.h"
#include "lexer_re2c.h"

namespace minic {

LexResult lexSource(LexerKind kind, const std::string& source, const std::string& filename) {
    LexResult r;
    if (kind == LexerKind::RE2C) {
        Re2cLexer lex(source, filename);
        r.tokens = lex.tokenize();
        r.errors = lex.errors();
    } else {
        Lexer lex(source, filename);
        r.tokens = lex.tokenize();
        r.errors = lex.errors();
    }
    return r;
}

}  // namespace minic
