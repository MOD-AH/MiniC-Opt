// MiniC-Opt — hand-written DFA lexical analyser (implementation)
// Owner: Member 1 (Front End)
#include "lexer.h"
#include <unordered_map>
#include <cctype>

namespace minic {

const char* tokName(Tok t) {
    switch (t) {
        case Tok::IDENT:            return "IDENT";
        case Tok::INT_LIT:          return "INT_LIT";
        case Tok::FLOAT_LIT:        return "FLOAT_LIT";
        case Tok::CHAR_LIT:         return "CHAR_LIT";
        case Tok::KW_INT:           return "KW_INT";
        case Tok::KW_CHAR:          return "KW_CHAR";
        case Tok::KW_FLOAT:         return "KW_FLOAT";
        case Tok::KW_VOID:          return "KW_VOID";
        case Tok::KW_IF:            return "KW_IF";
        case Tok::KW_ELSE:          return "KW_ELSE";
        case Tok::KW_WHILE:         return "KW_WHILE";
        case Tok::KW_FOR:           return "KW_FOR";
        case Tok::KW_DO:            return "KW_DO";
        case Tok::KW_RETURN:        return "KW_RETURN";
        case Tok::KW_BREAK:         return "KW_BREAK";
        case Tok::KW_CONTINUE:      return "KW_CONTINUE";
        case Tok::OP_PLUS:          return "OP_PLUS";
        case Tok::OP_MINUS:         return "OP_MINUS";
        case Tok::OP_STAR:          return "OP_STAR";
        case Tok::OP_SLASH:         return "OP_SLASH";
        case Tok::OP_PERCENT:       return "OP_PERCENT";
        case Tok::OP_ASSIGN:        return "OP_ASSIGN";
        case Tok::OP_PLUS_ASSIGN:   return "OP_PLUS_ASSIGN";
        case Tok::OP_MINUS_ASSIGN:  return "OP_MINUS_ASSIGN";
        case Tok::OP_STAR_ASSIGN:   return "OP_STAR_ASSIGN";
        case Tok::OP_SLASH_ASSIGN:  return "OP_SLASH_ASSIGN";
        case Tok::OP_EQ:            return "OP_EQ";
        case Tok::OP_NE:            return "OP_NE";
        case Tok::OP_LT:            return "OP_LT";
        case Tok::OP_LE:            return "OP_LE";
        case Tok::OP_GT:            return "OP_GT";
        case Tok::OP_GE:            return "OP_GE";
        case Tok::OP_AND:           return "OP_AND";
        case Tok::OP_OR:            return "OP_OR";
        case Tok::OP_NOT:           return "OP_NOT";
        case Tok::OP_INC:           return "OP_INC";
        case Tok::OP_DEC:           return "OP_DEC";
        case Tok::LPAREN:           return "LPAREN";
        case Tok::RPAREN:           return "RPAREN";
        case Tok::LBRACE:           return "LBRACE";
        case Tok::RBRACE:           return "RBRACE";
        case Tok::LBRACKET:         return "LBRACKET";
        case Tok::RBRACKET:         return "RBRACKET";
        case Tok::COMMA:            return "COMMA";
        case Tok::SEMI:             return "SEMI";
        case Tok::END_OF_FILE:      return "EOF";
        case Tok::ERROR:            return "ERROR";
    }
    return "UNKNOWN";
}

// Keyword table — consulted only after a maximal identifier has been scanned,
// which is what makes "intx" one IDENT rather than KW_INT followed by IDENT.
static const std::unordered_map<std::string, Tok>& keywords() {
    static const std::unordered_map<std::string, Tok> kw = {
        {"int", Tok::KW_INT},         {"char", Tok::KW_CHAR},
        {"float", Tok::KW_FLOAT},     {"void", Tok::KW_VOID},
        {"if", Tok::KW_IF},           {"else", Tok::KW_ELSE},
        {"while", Tok::KW_WHILE},     {"for", Tok::KW_FOR},
        {"do", Tok::KW_DO},           {"return", Tok::KW_RETURN},
        {"break", Tok::KW_BREAK},     {"continue", Tok::KW_CONTINUE},
    };
    return kw;
}

Lexer::Lexer(std::string source, std::string filename)
    : src_(std::move(source)), file_(std::move(filename)) {}

char Lexer::peek(size_t off) const {
    size_t i = pos_ + off;
    return i < src_.size() ? src_[i] : '\0';
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; } else { ++col_; }
    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    advance();
    return true;
}

void Lexer::error(int l, int c, const std::string& msg) {
    errors_.push_back({l, c, msg});
}

Token Lexer::make(Tok t, const std::string& lex, int l, int c) const {
    return Token{t, lex, l, c};
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {          // line comment
            while (peek() != '\n' && peek() != '\0') advance();
        } else if (c == '/' && peek(1) == '*') {          // block comment
            int sl = line_, sc = col_;
            advance(); advance();
            bool closed = false;
            while (peek() != '\0') {
                if (peek() == '*' && peek(1) == '/') { advance(); advance(); closed = true; break; }
                advance();
            }
            if (!closed) error(sl, sc, "unterminated block comment");
        } else {
            return;
        }
    }
}

// IDENT ::= [A-Za-z_] [A-Za-z0-9_]*
Token Lexer::scanIdentifierOrKeyword() {
    int l = line_, c = col_;
    std::string lex;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
        lex += advance();
    auto it = keywords().find(lex);
    return make(it != keywords().end() ? it->second : Tok::IDENT, lex, l, c);
}

// INT_LIT   ::= digit+
// FLOAT_LIT ::= digit+ '.' digit* ( [eE] [+-]? digit+ )?
Token Lexer::scanNumber() {
    int l = line_, c = col_;
    std::string lex;
    bool isFloat = false;
    while (std::isdigit(static_cast<unsigned char>(peek()))) lex += advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        isFloat = true;
        lex += advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) lex += advance();
    }
    if (peek() == 'e' || peek() == 'E') {
        size_t save = 1;
        if (peek(1) == '+' || peek(1) == '-') save = 2;
        if (std::isdigit(static_cast<unsigned char>(peek(save)))) {
            isFloat = true;
            lex += advance();
            if (peek() == '+' || peek() == '-') lex += advance();
            while (std::isdigit(static_cast<unsigned char>(peek()))) lex += advance();
        }
    }
    if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
        error(l, c, "malformed numeric literal '" + lex + std::string(1, peek()) + "'");
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') lex += advance();
        return make(Tok::ERROR, lex, l, c);
    }
    return make(isFloat ? Tok::FLOAT_LIT : Tok::INT_LIT, lex, l, c);
}

Token Lexer::scanCharLiteral() {
    int l = line_, c = col_;
    std::string lex;
    lex += advance();                       // opening quote
    if (peek() == '\\') { lex += advance(); lex += advance(); }
    else if (peek() != '\'' && peek() != '\0') { lex += advance(); }
    if (peek() == '\'') { lex += advance(); return make(Tok::CHAR_LIT, lex, l, c); }
    error(l, c, "unterminated character literal");
    return make(Tok::ERROR, lex, l, c);
}

// Maximal munch: two-character operators are tested before one-character ones.
Token Lexer::scanOperatorOrPunct() {
    int l = line_, c = col_;
    char ch = advance();
    switch (ch) {
        case '+': if (match('+')) return make(Tok::OP_INC, "++", l, c);
                  if (match('=')) return make(Tok::OP_PLUS_ASSIGN, "+=", l, c);
                  return make(Tok::OP_PLUS, "+", l, c);
        case '-': if (match('-')) return make(Tok::OP_DEC, "--", l, c);
                  if (match('=')) return make(Tok::OP_MINUS_ASSIGN, "-=", l, c);
                  return make(Tok::OP_MINUS, "-", l, c);
        case '*': if (match('=')) return make(Tok::OP_STAR_ASSIGN, "*=", l, c);
                  return make(Tok::OP_STAR, "*", l, c);
        case '/': if (match('=')) return make(Tok::OP_SLASH_ASSIGN, "/=", l, c);
                  return make(Tok::OP_SLASH, "/", l, c);
        case '%': return make(Tok::OP_PERCENT, "%", l, c);
        case '=': if (match('=')) return make(Tok::OP_EQ, "==", l, c);
                  return make(Tok::OP_ASSIGN, "=", l, c);
        case '!': if (match('=')) return make(Tok::OP_NE, "!=", l, c);
                  return make(Tok::OP_NOT, "!", l, c);
        case '<': if (match('=')) return make(Tok::OP_LE, "<=", l, c);
                  return make(Tok::OP_LT, "<", l, c);
        case '>': if (match('=')) return make(Tok::OP_GE, ">=", l, c);
                  return make(Tok::OP_GT, ">", l, c);
        case '&': if (match('&')) return make(Tok::OP_AND, "&&", l, c);
                  error(l, c, "unexpected '&' (bitwise operators are outside the MiniC subset)");
                  return make(Tok::ERROR, "&", l, c);
        case '|': if (match('|')) return make(Tok::OP_OR, "||", l, c);
                  error(l, c, "unexpected '|' (bitwise operators are outside the MiniC subset)");
                  return make(Tok::ERROR, "|", l, c);
        case '(': return make(Tok::LPAREN, "(", l, c);
        case ')': return make(Tok::RPAREN, ")", l, c);
        case '{': return make(Tok::LBRACE, "{", l, c);
        case '}': return make(Tok::RBRACE, "}", l, c);
        case '[': return make(Tok::LBRACKET, "[", l, c);
        case ']': return make(Tok::RBRACKET, "]", l, c);
        case ',': return make(Tok::COMMA, ",", l, c);
        case ';': return make(Tok::SEMI, ";", l, c);
        default:
            error(l, c, std::string("unexpected character '") + ch + "'");
            return make(Tok::ERROR, std::string(1, ch), l, c);
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    for (;;) {
        skipWhitespaceAndComments();
        if (pos_ >= src_.size()) {
            out.push_back(make(Tok::END_OF_FILE, "<end of file>", line_, col_));
            return out;
        }
        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
            out.push_back(scanIdentifierOrKeyword());
        else if (std::isdigit(static_cast<unsigned char>(c)))
            out.push_back(scanNumber());
        else if (c == '\'')
            out.push_back(scanCharLiteral());
        else
            out.push_back(scanOperatorOrPunct());
    }
}

} // namespace minic
