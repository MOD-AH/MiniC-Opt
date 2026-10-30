// Cppify — re2c lexer specification (roadmap 4.1)
//
// This file is the source; lexer_re2c.cpp is generated from it by re2c 4.6
// (`re2c -W -o src/lexer/lexer_re2c.cpp src/lexer/lexer_re2c.re`) and
// committed, so building needs no re2c. run_all regenerates it when re2c is
// installed and fails if the committed copy is stale.
//
// re2c turns the rules below into a direct-coded DFA: states become labels,
// transitions become `switch`es and `goto`s on the next byte, and the longest
// match wins (ties go to the rule written first, which is how a keyword beats
// the identifier rule). The hand-written Lexer does the same job with
// explicit peek()/advance() code. The two must produce identical token
// streams and diagnostics on every test file (checked by run_all).
//
// What is not token recognition stays in plain C++ around the DFA, written to
// match the hand-written lexer exactly: lookahead after a number (suffixes,
// a malformed tail), collecting a directive line, dropping lines inside a
// false #if, macro expansion. Directive meaning is the shared Preprocessor.
#include "lexer_re2c.h"
#include <cctype>

namespace minic {
namespace {

bool blank(char c) { return c == ' ' || c == '\t' || c == '\r'; }
bool identChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

// The text of a string literal with its escapes decoded.
std::string decode(const char* p, const char* e) {
    std::string v;
    while (p < e) {
        char ch = *p++;
        if (ch != '\\') { v += ch; continue; }
        if (p >= e) break;   // a trailing backslash at the end of input escapes nothing
        char x = *p++;
        switch (x) {
            case 'n':  v += '\n'; break;
            case 't':  v += '\t'; break;
            case 'r':  v += '\r'; break;
            case '0':  v += '\0'; break;
            default:   v += x;    break;   // \\ \" \' and anything else: the character itself
        }
    }
    return v;
}

}  // namespace

Re2cLexer::Re2cLexer(std::string source, std::string filename)
    : src_(std::move(source)), file_(std::move(filename)) {}

void Re2cLexer::moveTo(const char* p) {
    for (const char* q = cur_; q < p; ++q)
        if (*q == '\n') { ++line_; lineStart_ = q + 1; }
    cur_ = p;
}

bool Re2cLexer::atLineStart(const char* p) const {
    const char* base = src_.c_str();
    while (p > base && blank(p[-1])) --p;
    return p == base || p[-1] == '\n';
}

// Collects one directive line exactly as Lexer::defineMacroFromLine does:
// backslash-newlines joined, comments replaced by a space, the newline left.
void Re2cLexer::directive(const char* p, int l, int c) {
    const char* end = src_.c_str() + src_.size();
    std::string text;
    for (;;) {
        char ch = p < end ? *p : '\0';
        if (ch == '\0' || ch == '\n') break;
        if (ch == '\\' && (p[1] == '\n' || (p[1] == '\r' && p[2] == '\n'))) {
            p += p[1] == '\r' ? 3 : 2;
            text += ' ';
            continue;
        }
        if (ch == '/' && p[1] == '/') {
            while (p < end && *p != '\n' && *p != '\0') ++p;
            break;
        }
        if (ch == '/' && p[1] == '*') {
            p += 2;
            while (p < end && *p != '\0' && !(*p == '*' && p[1] == '/')) ++p;
            if (p < end && *p != '\0') p += 2;
            text += ' ';
            continue;
        }
        text += ch;
        ++p;
    }
    moveTo(p);
    pp_.directive(text, l, c, errors_);
}

void Re2cLexer::expandMacro(const std::string& name, int l, int c, std::vector<Token>& out) {
    Re2cLexer sub(pp_.body(name), file_);
    sub.pp_ = pp_.forExpansionOf(name);
    std::vector<Token> toks = sub.tokenize();
    for (const auto& e : sub.errors_) errors_.push_back({l, c, e.message});
    for (auto& t : toks) {
        if (t.type == Tok::END_OF_FILE) break;
        t.line = l;
        t.col = c;
        out.push_back(std::move(t));
    }
}

std::vector<Token> Re2cLexer::tokenize() {
    std::vector<Token> out;
    const char* end = src_.c_str() + src_.size();
    cur_ = lineStart_ = src_.c_str();
    line_ = 1;
    for (;;) {
        const char* tok = cur_;
        const int l = line_, c = static_cast<int>(tok - lineStart_) + 1;
        auto emit = [&](Tok t, std::string lexeme) { out.push_back(Token{t, std::move(lexeme), l, c}); };
        auto error = [&](const std::string& m) { errors_.push_back({l, c, m}); };
        if (tok >= end) {
            pp_.finish(errors_);
            emit(Tok::END_OF_FILE, "<end of file>");
            return out;
        }
        // Inside a false #if branch, a line that is not a directive is
        // dropped unlexed. Blanks go to the whitespace rule first, so an
        // indented directive is still recognised.
        if (pp_.skipping() && !blank(*tok) && *tok != '\n' && *tok != '\0' && !(*tok == '#' && atLineStart(tok))) {
            const char* p = tok;
            while (p < end && *p != '\n' && *p != '\0') ++p;
            if (p < end && *p == '\n') ++p;
            moveTo(p);
            continue;
        }
        const char* YYCURSOR = tok;
        const char* YYMARKER = tok;
        /*!re2c
            // unsigned: with a signed char, bytes 0x80-0xFF compare negative
            // and fall out of classes like [^\x00] (found by lexfuzz.py).
            re2c:define:YYCTYPE = "unsigned char";
            re2c:yyfill:enable = 0;
            re2c:indent:top = 2;

            D = [0-9];
            L = [a-zA-Z_];
            H = [0-9a-fA-F];
            E = [eE] [+-]? D+;

            [ \t\r\n]+                                 { moveTo(YYCURSOR); continue; }
            "//" [^\n\x00]*                            { moveTo(YYCURSOR); continue; }
            "/*" ([^*\x00] | "*"+ [^*/\x00])* "*"+ "/" { moveTo(YYCURSOR); continue; }
            "/*" {
                error("unterminated block comment");
                const char* p = YYCURSOR;
                while (p < end && *p != '\0') ++p;
                moveTo(p);
                continue;
            }
            "#" {
                if (atLineStart(tok)) { directive(tok, l, c); continue; }
                error("unexpected character '#'");
                emit(Tok::ERROR, "#");
                moveTo(YYCURSOR);
                continue;
            }

            "int"      { emit(Tok::KW_INT, "int");           moveTo(YYCURSOR); continue; }
            "char"     { emit(Tok::KW_CHAR, "char");         moveTo(YYCURSOR); continue; }
            "long"     { emit(Tok::KW_LONG, "long");         moveTo(YYCURSOR); continue; }
            "short"    { emit(Tok::KW_SHORT, "short");       moveTo(YYCURSOR); continue; }
            "unsigned" { emit(Tok::KW_UNSIGNED, "unsigned"); moveTo(YYCURSOR); continue; }
            "signed"   { emit(Tok::KW_SIGNED, "signed");     moveTo(YYCURSOR); continue; }
            "double"   { emit(Tok::KW_DOUBLE, "double");     moveTo(YYCURSOR); continue; }
            "const"    { emit(Tok::KW_CONST, "const");       moveTo(YYCURSOR); continue; }
            "static"   { emit(Tok::KW_STATIC, "static");     moveTo(YYCURSOR); continue; }
            "extern"   { emit(Tok::KW_EXTERN, "extern");     moveTo(YYCURSOR); continue; }
            "float"    { emit(Tok::KW_FLOAT, "float");       moveTo(YYCURSOR); continue; }
            "void"     { emit(Tok::KW_VOID, "void");         moveTo(YYCURSOR); continue; }
            "if"       { emit(Tok::KW_IF, "if");             moveTo(YYCURSOR); continue; }
            "else"     { emit(Tok::KW_ELSE, "else");         moveTo(YYCURSOR); continue; }
            "while"    { emit(Tok::KW_WHILE, "while");       moveTo(YYCURSOR); continue; }
            "for"      { emit(Tok::KW_FOR, "for");           moveTo(YYCURSOR); continue; }
            "do"       { emit(Tok::KW_DO, "do");             moveTo(YYCURSOR); continue; }
            "return"   { emit(Tok::KW_RETURN, "return");     moveTo(YYCURSOR); continue; }
            "break"    { emit(Tok::KW_BREAK, "break");       moveTo(YYCURSOR); continue; }
            "continue" { emit(Tok::KW_CONTINUE, "continue"); moveTo(YYCURSOR); continue; }
            "struct"   { emit(Tok::KW_STRUCT, "struct");     moveTo(YYCURSOR); continue; }
            "typedef"  { emit(Tok::KW_TYPEDEF, "typedef");   moveTo(YYCURSOR); continue; }
            "enum"     { emit(Tok::KW_ENUM, "enum");         moveTo(YYCURSOR); continue; }
            "sizeof"   { emit(Tok::KW_SIZEOF, "sizeof");     moveTo(YYCURSOR); continue; }
            "switch"   { emit(Tok::KW_SWITCH, "switch");     moveTo(YYCURSOR); continue; }
            "case"     { emit(Tok::KW_CASE, "case");         moveTo(YYCURSOR); continue; }
            "default"  { emit(Tok::KW_DEFAULT, "default");   moveTo(YYCURSOR); continue; }
            "goto"     { emit(Tok::KW_GOTO, "goto");         moveTo(YYCURSOR); continue; }

            L (L | D)* {
                std::string name(tok, YYCURSOR);
                moveTo(YYCURSOR);
                if (pp_.isMacro(name)) expandMacro(name, l, c, out);
                else emit(Tok::IDENT, name);
                continue;
            }

            "0" [xX] H+ {
                std::string digits(tok + 2, YYCURSOR), suffix;
                const char* p = YYCURSOR;
                while (p < end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L')) suffix += *p++;
                if (p < end && identChar(*p)) {
                    error("malformed numeric literal '0x" + digits + std::string(1, *p) + "'");
                    while (p < end && identChar(*p)) ++p;
                    moveTo(p);
                    emit(Tok::ERROR, "0x" + digits);
                    continue;
                }
                moveTo(p);
                emit(Tok::INT_LIT, std::to_string(std::stoull(digits, nullptr, 16)) + suffix);
                continue;
            }
            D+ "." D+ E? | D+ E | "." D+ E? {
                std::string lex(tok, YYCURSOR);
                const char* p = YYCURSOR;
                if (p < end && (*p == 'f' || *p == 'F')) ++p;   // float and double are one type: dropped
                if (p < end && (std::isalpha(static_cast<unsigned char>(*p)) || *p == '_')) {
                    error("malformed numeric literal '" + lex + std::string(1, *p) + "'");
                    while (p < end && identChar(*p)) lex += *p++;
                    moveTo(p);
                    emit(Tok::ERROR, lex);
                    continue;
                }
                moveTo(p);
                emit(Tok::FLOAT_LIT, lex);
                continue;
            }
            D+ {
                std::string lex(tok, YYCURSOR);
                const char* p = YYCURSOR;
                while (p < end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L')) lex += *p++;
                if (p < end && (std::isalpha(static_cast<unsigned char>(*p)) || *p == '_')) {
                    error("malformed numeric literal '" + lex + std::string(1, *p) + "'");
                    while (p < end && identChar(*p)) lex += *p++;
                    moveTo(p);
                    emit(Tok::ERROR, lex);
                    continue;
                }
                moveTo(p);
                emit(Tok::INT_LIT, lex);
                continue;
            }

            "'" ("\\" [^\x00] | [^'\\\x00])? "'" { emit(Tok::CHAR_LIT, std::string(tok, YYCURSOR)); moveTo(YYCURSOR); continue; }
            "'" ("\\" [^\x00] | "\\" | [^'\\\x00])? {
                error("unterminated character literal");
                emit(Tok::ERROR, std::string(tok, YYCURSOR));
                moveTo(YYCURSOR);
                continue;
            }
            ["] ([^"\\\n\x00] | "\\" [^\x00])* ["] { emit(Tok::STRING_LIT, decode(tok + 1, YYCURSOR - 1)); moveTo(YYCURSOR); continue; }
            ["] ([^"\\\n\x00] | "\\" [^\x00])* "\\"? {
                error("unterminated string literal");
                emit(Tok::STRING_LIT, decode(tok + 1, YYCURSOR));
                moveTo(YYCURSOR);
                continue;
            }

            "++"  { emit(Tok::OP_INC, "++");            moveTo(YYCURSOR); continue; }
            "+="  { emit(Tok::OP_PLUS_ASSIGN, "+=");    moveTo(YYCURSOR); continue; }
            "+"   { emit(Tok::OP_PLUS, "+");            moveTo(YYCURSOR); continue; }
            "->"  { emit(Tok::OP_ARROW, "->");          moveTo(YYCURSOR); continue; }
            "--"  { emit(Tok::OP_DEC, "--");            moveTo(YYCURSOR); continue; }
            "-="  { emit(Tok::OP_MINUS_ASSIGN, "-=");   moveTo(YYCURSOR); continue; }
            "-"   { emit(Tok::OP_MINUS, "-");           moveTo(YYCURSOR); continue; }
            "*="  { emit(Tok::OP_STAR_ASSIGN, "*=");    moveTo(YYCURSOR); continue; }
            "*"   { emit(Tok::OP_STAR, "*");            moveTo(YYCURSOR); continue; }
            "/="  { emit(Tok::OP_SLASH_ASSIGN, "/=");   moveTo(YYCURSOR); continue; }
            "/"   { emit(Tok::OP_SLASH, "/");           moveTo(YYCURSOR); continue; }
            "%="  { emit(Tok::OP_PERCENT_ASSIGN, "%="); moveTo(YYCURSOR); continue; }
            "%"   { emit(Tok::OP_PERCENT, "%");         moveTo(YYCURSOR); continue; }
            "=="  { emit(Tok::OP_EQ, "==");             moveTo(YYCURSOR); continue; }
            "="   { emit(Tok::OP_ASSIGN, "=");          moveTo(YYCURSOR); continue; }
            "!="  { emit(Tok::OP_NE, "!=");             moveTo(YYCURSOR); continue; }
            "!"   { emit(Tok::OP_NOT, "!");             moveTo(YYCURSOR); continue; }
            "<<=" { emit(Tok::OP_SHL_ASSIGN, "<<=");    moveTo(YYCURSOR); continue; }
            "<<"  { emit(Tok::OP_SHL, "<<");            moveTo(YYCURSOR); continue; }
            "<="  { emit(Tok::OP_LE, "<=");             moveTo(YYCURSOR); continue; }
            "<"   { emit(Tok::OP_LT, "<");              moveTo(YYCURSOR); continue; }
            ">>=" { emit(Tok::OP_SHR_ASSIGN, ">>=");    moveTo(YYCURSOR); continue; }
            ">>"  { emit(Tok::OP_SHR, ">>");            moveTo(YYCURSOR); continue; }
            ">="  { emit(Tok::OP_GE, ">=");             moveTo(YYCURSOR); continue; }
            ">"   { emit(Tok::OP_GT, ">");              moveTo(YYCURSOR); continue; }
            "&&"  { emit(Tok::OP_AND, "&&");            moveTo(YYCURSOR); continue; }
            "&="  { emit(Tok::OP_AND_ASSIGN, "&=");     moveTo(YYCURSOR); continue; }
            "&"   { emit(Tok::OP_BITAND, "&");          moveTo(YYCURSOR); continue; }
            "||"  { emit(Tok::OP_OR, "||");             moveTo(YYCURSOR); continue; }
            "|="  { emit(Tok::OP_OR_ASSIGN, "|=");      moveTo(YYCURSOR); continue; }
            "|"   { emit(Tok::OP_BITOR, "|");           moveTo(YYCURSOR); continue; }
            "^="  { emit(Tok::OP_XOR_ASSIGN, "^=");     moveTo(YYCURSOR); continue; }
            "^"   { emit(Tok::OP_BITXOR, "^");          moveTo(YYCURSOR); continue; }
            "~"   { emit(Tok::OP_BITNOT, "~");          moveTo(YYCURSOR); continue; }
            "("   { emit(Tok::LPAREN, "(");             moveTo(YYCURSOR); continue; }
            ")"   { emit(Tok::RPAREN, ")");             moveTo(YYCURSOR); continue; }
            "{"   { emit(Tok::LBRACE, "{");             moveTo(YYCURSOR); continue; }
            "}"   { emit(Tok::RBRACE, "}");             moveTo(YYCURSOR); continue; }
            "["   { emit(Tok::LBRACKET, "[");           moveTo(YYCURSOR); continue; }
            "]"   { emit(Tok::RBRACKET, "]");           moveTo(YYCURSOR); continue; }
            ","   { emit(Tok::COMMA, ",");              moveTo(YYCURSOR); continue; }
            "."   { emit(Tok::OP_DOT, ".");             moveTo(YYCURSOR); continue; }
            "?"   { emit(Tok::OP_QUESTION, "?");        moveTo(YYCURSOR); continue; }
            ":"   { emit(Tok::OP_COLON, ":");           moveTo(YYCURSOR); continue; }
            ";"   { emit(Tok::SEMI, ";");               moveTo(YYCURSOR); continue; }

            [^] {
                error(std::string("unexpected character '") + *tok + "'");
                emit(Tok::ERROR, std::string(1, *tok));
                moveTo(YYCURSOR);
                continue;
            }
        */
    }
}

}  // namespace minic
