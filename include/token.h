// MiniC-Opt — token definitions
// Owner: Member 1 (Front End)
#pragma once
#include <string>
#include <cstdint>

namespace minic {

enum class Tok {
    // literals & identifiers
    IDENT, INT_LIT, FLOAT_LIT, CHAR_LIT,
    // keywords
    KW_INT, KW_CHAR, KW_FLOAT, KW_VOID,
    KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_DO,
    KW_RETURN, KW_BREAK, KW_CONTINUE,
    // operators
    OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH, OP_PERCENT,
    OP_ASSIGN, OP_PLUS_ASSIGN, OP_MINUS_ASSIGN, OP_STAR_ASSIGN, OP_SLASH_ASSIGN,
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE,
    OP_AND, OP_OR, OP_NOT,
    OP_INC, OP_DEC,
    // punctuation
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, SEMI,
    // control
    END_OF_FILE, ERROR
};

const char* tokName(Tok t);

struct Token {
    Tok         type;
    std::string lexeme;
    int         line;
    int         col;
};

} // namespace minic
