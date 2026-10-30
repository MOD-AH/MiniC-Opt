// MiniC-Opt — hand-written DFA lexical analyser (implementation)
// Owner: Member 1 (Front End)
#include "lexer.h"
#include <algorithm>
#include <unordered_map>
#include <cctype>

namespace minic {

const char* tokName(Tok t) {
    switch (t) {
        case Tok::IDENT:            return "IDENT";
        case Tok::INT_LIT:          return "INT_LIT";
        case Tok::FLOAT_LIT:        return "FLOAT_LIT";
        case Tok::CHAR_LIT:         return "CHAR_LIT";
        case Tok::STRING_LIT:       return "STRING_LIT";
        case Tok::KW_INT:           return "KW_INT";
        case Tok::KW_LONG:          return "KW_LONG";
        case Tok::KW_SHORT:         return "KW_SHORT";
        case Tok::KW_UNSIGNED:      return "KW_UNSIGNED";
        case Tok::KW_SIGNED:        return "KW_SIGNED";
        case Tok::KW_DOUBLE:        return "KW_DOUBLE";
        case Tok::KW_CONST:         return "KW_CONST";
        case Tok::KW_STATIC:        return "KW_STATIC";
        case Tok::KW_EXTERN:        return "KW_EXTERN";
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
        case Tok::KW_STRUCT:        return "KW_STRUCT";
        case Tok::KW_TYPEDEF:       return "KW_TYPEDEF";
        case Tok::KW_ENUM:          return "KW_ENUM";
        case Tok::KW_SIZEOF:        return "KW_SIZEOF";
        case Tok::KW_SWITCH:        return "KW_SWITCH";
        case Tok::KW_CASE:          return "KW_CASE";
        case Tok::KW_DEFAULT:       return "KW_DEFAULT";
        case Tok::KW_GOTO:          return "KW_GOTO";
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
        case Tok::OP_PERCENT_ASSIGN: return "OP_PERCENT_ASSIGN";
        case Tok::OP_AND_ASSIGN:    return "OP_AND_ASSIGN";
        case Tok::OP_OR_ASSIGN:     return "OP_OR_ASSIGN";
        case Tok::OP_XOR_ASSIGN:    return "OP_XOR_ASSIGN";
        case Tok::OP_SHL_ASSIGN:    return "OP_SHL_ASSIGN";
        case Tok::OP_SHR_ASSIGN:    return "OP_SHR_ASSIGN";
        case Tok::OP_EQ:            return "OP_EQ";
        case Tok::OP_NE:            return "OP_NE";
        case Tok::OP_LT:            return "OP_LT";
        case Tok::OP_LE:            return "OP_LE";
        case Tok::OP_GT:            return "OP_GT";
        case Tok::OP_GE:            return "OP_GE";
        case Tok::OP_AND:           return "OP_AND";
        case Tok::OP_OR:            return "OP_OR";
        case Tok::OP_NOT:           return "OP_NOT";
        case Tok::OP_BITAND:        return "OP_BITAND";
        case Tok::OP_BITOR:         return "OP_BITOR";
        case Tok::OP_BITXOR:        return "OP_BITXOR";
        case Tok::OP_BITNOT:        return "OP_BITNOT";
        case Tok::OP_SHL:           return "OP_SHL";
        case Tok::OP_SHR:           return "OP_SHR";
        case Tok::OP_INC:           return "OP_INC";
        case Tok::OP_DEC:           return "OP_DEC";
        case Tok::OP_DOT:           return "OP_DOT";
        case Tok::OP_ARROW:         return "OP_ARROW";
        case Tok::OP_QUESTION:      return "OP_QUESTION";
        case Tok::OP_COLON:         return "OP_COLON";
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
        {"long", Tok::KW_LONG},       {"short", Tok::KW_SHORT},
        {"unsigned", Tok::KW_UNSIGNED}, {"signed", Tok::KW_SIGNED},
        {"double", Tok::KW_DOUBLE},
        {"const", Tok::KW_CONST},     {"static", Tok::KW_STATIC},
        {"extern", Tok::KW_EXTERN},
        {"float", Tok::KW_FLOAT},     {"void", Tok::KW_VOID},
        {"if", Tok::KW_IF},           {"else", Tok::KW_ELSE},
        {"while", Tok::KW_WHILE},     {"for", Tok::KW_FOR},
        {"do", Tok::KW_DO},           {"return", Tok::KW_RETURN},
        {"break", Tok::KW_BREAK},     {"continue", Tok::KW_CONTINUE},
        {"struct", Tok::KW_STRUCT},   {"typedef", Tok::KW_TYPEDEF},
        {"enum", Tok::KW_ENUM},       {"sizeof", Tok::KW_SIZEOF},
        {"switch", Tok::KW_SWITCH},   {"case", Tok::KW_CASE},
        {"default", Tok::KW_DEFAULT}, {"goto", Tok::KW_GOTO},
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

// True when everything between the previous newline and the cursor is
// blank, i.e. the '#' starts its own line. A '#' anywhere else is still an
// unexpected character, so this does not quietly swallow stray punctuation.
bool Lexer::onlyBlanksBeforeOnThisLine() const {
    size_t i = pos_;
    while (i > 0) {
        char ch = src_[i - 1];
        if (ch == '\n') break;
        if (ch != ' ' && ch != '\t' && ch != '\r') return false;
        --i;
    }
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        char c = peek();
        if (pp_.skipping() && c != '\0' && c != ' ' && c != '\t' && c != '\r' && c != '\n'
                && !(c == '#' && onlyBlanksBeforeOnThisLine())) {
            // Inside a false #if branch: drop the rest of the line unlexed.
            // Leading blanks are consumed first (the branch below), so an
            // indented `  #endif` is still seen as a directive.
            while (peek() != '\n' && peek() != '\0') advance();
            if (peek() == '\n') advance();
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {          // line comment
            while (peek() != '\n' && peek() != '\0') advance();
        } else if (c == '#' && onlyBlanksBeforeOnThisLine()) {   // preprocessor line
            // #include is IGNORED, not honoured: there are no headers to
            // find and the built-ins are always in scope. `#define NAME text`
            // (object-like), #undef and the conditional directives are
            // honoured; anything else is skipped like a comment.
            defineMacroFromLine();
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

// Consumes one preprocessor line, joining backslash-newlines and replacing
// comments by a space, and hands it to the shared Preprocessor.
void Lexer::defineMacroFromLine() {
    int dl = line_, dc = col_;
    std::string text;
    for (;;) {
        char ch = peek();
        if (ch == '\0') break;
        if (ch == '\n') break;
        if (ch == '\\' && (peek(1) == '\n' || (peek(1) == '\r' && peek(2) == '\n'))) {
            advance();                                  // the backslash
            if (peek() == '\r') advance();
            advance();                                  // the newline
            text += ' ';
            continue;
        }
        if (ch == '/' && peek(1) == '/') { while (peek() != '\n' && peek() != '\0') advance(); break; }
        if (ch == '/' && peek(1) == '*') {
            advance(); advance();
            while (peek() != '\0' && !(peek() == '*' && peek(1) == '/')) advance();
            if (peek() != '\0') { advance(); advance(); }
            text += ' ';
            continue;
        }
        text += advance();
    }
    pp_.directive(text, dl, dc, errors_);
}

// Replaces one use of a macro by the tokens of its body. The body is lexed
// by a nested lexer that does not know the macro itself, which is what stops
// `#define X X + 1` from expanding forever.
void Lexer::expandMacro(const std::string& name, int l, int c, std::vector<Token>& out) {
    Lexer sub(pp_.body(name), file_);
    sub.pp_ = pp_.forExpansionOf(name);
    std::vector<Token> toks = sub.tokenize();
    for (const auto& e : sub.errors_) errors_.push_back({l, c, e.message});
    for (auto& t : toks) {
        if (t.type == Tok::END_OF_FILE) break;
        t.line = l; t.col = c;
        out.push_back(std::move(t));
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

    // Hexadecimal: 0x / 0X followed by hex digits. Normalised to its decimal
    // value in the lexeme so every later stage keeps treating INT_LIT as a
    // plain base-10 string and nothing downstream needs to know about bases.
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')
        && std::isxdigit(static_cast<unsigned char>(peek(2)))) {
        advance(); advance();                     // consume "0x"
        std::string digits;
        while (std::isxdigit(static_cast<unsigned char>(peek()))) digits += advance();
        std::string suffix;
        while (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L') suffix += advance();
        if (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
            error(l, c, "malformed numeric literal '0x" + digits + std::string(1, peek()) + "'");
            while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
            return make(Tok::ERROR, "0x" + digits, l, c);
        }
        return make(Tok::INT_LIT, std::to_string(std::stoull(digits, nullptr, 16)) + suffix, l, c);
    }

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
    // C type suffixes (3.14f, 10u, 100L, 7ul) say how wide a constant is.
    // An integer suffix stays on the lexeme so the parser can give the
    // literal its C type; a float suffix carries nothing (float and double
    // are one type here) and is dropped.
    if (isFloat) {
        if (peek() == 'f' || peek() == 'F') advance();
    } else {
        while (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L') lex += advance();
    }
    if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
        error(l, c, "malformed numeric literal '" + lex + std::string(1, peek()) + "'");
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') lex += advance();
        return make(Tok::ERROR, lex, l, c);
    }
    return make(isFloat ? Tok::FLOAT_LIT : Tok::INT_LIT, lex, l, c);
}

// STRING_LIT ::= '"' { any char except '"' or newline, with \escapes } '"'
// The token's lexeme holds the DECODED text, so later stages never re-parse
// escapes. An unterminated literal is reported at the opening quote and the
// scanner resumes at the end of the line rather than swallowing the file.
Token Lexer::scanStringLiteral() {
    int l = line_, c = col_;
    advance();                       // opening quote
    std::string value;
    bool closed = false;
    while (peek() != '\0' && peek() != '\n') {
        char ch = advance();
        if (ch == '"') { closed = true; break; }
        if (ch == '\\') {
            // A backslash as the very last byte escapes nothing; reading on
            // would run past the end of the source (found by lexfuzz.py).
            if (pos_ >= src_.size()) break;
            char e = advance();
            switch (e) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case 'r':  value += '\r'; break;
                case '0':  value += '\0'; break;
                case '\\': value += '\\'; break;
                case '"':  value += '"';  break;
                case '\'': value += '\''; break;
                default:   value += e;    break;
            }
            continue;
        }
        value += ch;
    }
    if (!closed) error(l, c, "unterminated string literal");
    return make(Tok::STRING_LIT, value, l, c);
}

Token Lexer::scanCharLiteral() {
    int l = line_, c = col_;
    std::string lex;
    lex += advance();                       // opening quote
    if (peek() == '\\') { lex += advance(); if (pos_ < src_.size()) lex += advance(); }
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
        case '-': if (match('>')) return make(Tok::OP_ARROW, "->", l, c);
                  if (match('-')) return make(Tok::OP_DEC, "--", l, c);
                  if (match('=')) return make(Tok::OP_MINUS_ASSIGN, "-=", l, c);
                  return make(Tok::OP_MINUS, "-", l, c);
        case '*': if (match('=')) return make(Tok::OP_STAR_ASSIGN, "*=", l, c);
                  return make(Tok::OP_STAR, "*", l, c);
        case '/': if (match('=')) return make(Tok::OP_SLASH_ASSIGN, "/=", l, c);
                  return make(Tok::OP_SLASH, "/", l, c);
        case '%': if (match('=')) return make(Tok::OP_PERCENT_ASSIGN, "%=", l, c);
                  return make(Tok::OP_PERCENT, "%", l, c);
        case '=': if (match('=')) return make(Tok::OP_EQ, "==", l, c);
                  return make(Tok::OP_ASSIGN, "=", l, c);
        case '!': if (match('=')) return make(Tok::OP_NE, "!=", l, c);
                  return make(Tok::OP_NOT, "!", l, c);
        case '<': if (match('<')) {
                      if (match('=')) return make(Tok::OP_SHL_ASSIGN, "<<=", l, c);
                      return make(Tok::OP_SHL, "<<", l, c);
                  }
                  if (match('=')) return make(Tok::OP_LE, "<=", l, c);
                  return make(Tok::OP_LT, "<", l, c);
        case '>': if (match('>')) {
                      if (match('=')) return make(Tok::OP_SHR_ASSIGN, ">>=", l, c);
                      return make(Tok::OP_SHR, ">>", l, c);
                  }
                  if (match('=')) return make(Tok::OP_GE, ">=", l, c);
                  return make(Tok::OP_GT, ">", l, c);
        case '&': if (match('&')) return make(Tok::OP_AND, "&&", l, c);
                  if (match('=')) return make(Tok::OP_AND_ASSIGN, "&=", l, c);
                  return make(Tok::OP_BITAND, "&", l, c);
        case '|': if (match('|')) return make(Tok::OP_OR, "||", l, c);
                  if (match('=')) return make(Tok::OP_OR_ASSIGN, "|=", l, c);
                  return make(Tok::OP_BITOR, "|", l, c);
        case '^': if (match('=')) return make(Tok::OP_XOR_ASSIGN, "^=", l, c);
                  return make(Tok::OP_BITXOR, "^", l, c);
        case '~': return make(Tok::OP_BITNOT, "~", l, c);
        case '(': return make(Tok::LPAREN, "(", l, c);
        case ')': return make(Tok::RPAREN, ")", l, c);
        case '{': return make(Tok::LBRACE, "{", l, c);
        case '}': return make(Tok::RBRACE, "}", l, c);
        case '[': return make(Tok::LBRACKET, "[", l, c);
        case ']': return make(Tok::RBRACKET, "]", l, c);
        case ',': return make(Tok::COMMA, ",", l, c);
        case '.': return make(Tok::OP_DOT, ".", l, c);
        case '?': return make(Tok::OP_QUESTION, "?", l, c);
        case ':': return make(Tok::OP_COLON, ":", l, c);
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
            pp_.finish(errors_);
            out.push_back(make(Tok::END_OF_FILE, "<end of file>", line_, col_));
            return out;
        }
        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            Token t = scanIdentifierOrKeyword();
            if (t.type == Tok::IDENT && pp_.isMacro(t.lexeme)) expandMacro(t.lexeme, t.line, t.col, out);
            else out.push_back(std::move(t));
        }
        else if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))))
            out.push_back(scanNumber());
        else if (c == '\'')
            out.push_back(scanCharLiteral());
        else if (c == '"')
            out.push_back(scanStringLiteral());
        else
            out.push_back(scanOperatorOrPunct());
    }
}

} // namespace minic
