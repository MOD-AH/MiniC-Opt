// MiniC-Opt — token definitions
// Owner: Member 1 (Front End)
#pragma once
#include <string>
#include <cstdint>

namespace minic {

enum class Tok {
    // literals & identifiers
    IDENT, INT_LIT, FLOAT_LIT, CHAR_LIT, STRING_LIT,
    // keywords
    KW_INT, KW_CHAR, KW_FLOAT, KW_VOID,
    // Accepted and folded onto the four real types / ignored entirely, so
    // ordinary C declarations parse. See Parser::parseTypeSpec.
    KW_LONG, KW_SHORT, KW_UNSIGNED, KW_SIGNED, KW_DOUBLE,
    KW_CONST, KW_STATIC, KW_EXTERN,
    KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_DO,
    KW_RETURN, KW_BREAK, KW_CONTINUE,
    KW_STRUCT, KW_TYPEDEF, KW_ENUM, KW_SIZEOF, KW_SWITCH, KW_CASE, KW_DEFAULT, KW_GOTO,
    // operators
    OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH, OP_PERCENT,
    OP_ASSIGN, OP_PLUS_ASSIGN, OP_MINUS_ASSIGN, OP_STAR_ASSIGN, OP_SLASH_ASSIGN, OP_PERCENT_ASSIGN,
    OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN, OP_SHL_ASSIGN, OP_SHR_ASSIGN,
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE,
    OP_AND, OP_OR, OP_NOT,
    // Bitwise. Previously rejected by the lexer by name; now first-class.
    OP_BITAND, OP_BITOR, OP_BITXOR, OP_BITNOT, OP_SHL, OP_SHR,
    OP_INC, OP_DEC,
    OP_DOT, OP_ARROW, OP_QUESTION, OP_COLON,
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
