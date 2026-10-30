// MiniC-Opt — recursive-descent / Pratt parser (implementation)
// Owner: Member 1 (Front End)
#include "parser.h"

namespace minic {

// ---- small free helpers ---------------------------------------------

static bool isAssignOpTok(Tok t) {
    switch (t) {
        case Tok::OP_ASSIGN: case Tok::OP_PLUS_ASSIGN: case Tok::OP_MINUS_ASSIGN:
        case Tok::OP_STAR_ASSIGN: case Tok::OP_SLASH_ASSIGN: case Tok::OP_PERCENT_ASSIGN:
        case Tok::OP_AND_ASSIGN: case Tok::OP_OR_ASSIGN: case Tok::OP_XOR_ASSIGN:
        case Tok::OP_SHL_ASSIGN: case Tok::OP_SHR_ASSIGN:
            return true;
        default:
            return false;
    }
}

static AssignOp toAssignOp(Tok t) {
    switch (t) {
        case Tok::OP_PLUS_ASSIGN:  return AssignOp::PLUS_ASSIGN;
        case Tok::OP_MINUS_ASSIGN: return AssignOp::MINUS_ASSIGN;
        case Tok::OP_STAR_ASSIGN:  return AssignOp::STAR_ASSIGN;
        case Tok::OP_SLASH_ASSIGN: return AssignOp::SLASH_ASSIGN;
        case Tok::OP_PERCENT_ASSIGN: return AssignOp::PERCENT_ASSIGN;
        case Tok::OP_AND_ASSIGN:   return AssignOp::AND_ASSIGN;
        case Tok::OP_OR_ASSIGN:    return AssignOp::OR_ASSIGN;
        case Tok::OP_XOR_ASSIGN:   return AssignOp::XOR_ASSIGN;
        case Tok::OP_SHL_ASSIGN:   return AssignOp::SHL_ASSIGN;
        case Tok::OP_SHR_ASSIGN:   return AssignOp::SHR_ASSIGN;
        default:                   return AssignOp::ASSIGN;
    }
}

// The lexer's CHAR_LIT lexeme is exactly what it scanned: opening quote,
// one character OR a backslash escape, closing quote (Lexer::scanCharLiteral).
static char decodeCharLiteral(const std::string& lex) {
    if (lex.size() >= 3 && lex[1] == '\\') {
        switch (lex[2]) {
            case 'n':  return '\n';
            case 't':  return '\t';
            case 'r':  return '\r';
            case '0':  return '\0';
            case '\\': return '\\';
            case '\'': return '\'';
            default:   return lex[2];
        }
    }
    if (lex.size() >= 2) return lex[1];
    return '\0';
}

// ---- construction & token-stream helpers --------------------------------

Parser::Parser(std::vector<Token> tokens, std::string filename)
    : toks_(std::move(tokens)), file_(std::move(filename)) {}

const Token& Parser::peek(size_t off) const {
    size_t idx = pos_ + off;
    if (idx >= toks_.size()) return toks_.back();   // toks_ always ends in END_OF_FILE
    return toks_[idx];
}

const Token& Parser::previous() const { return toks_[pos_ - 1]; }
bool Parser::isAtEnd() const { return peek().type == Tok::END_OF_FILE; }
bool Parser::check(Tok t) const { return peek().type == t; }

const Token& Parser::advance() {
    if (!isAtEnd()) ++pos_;
    return previous();
}

bool Parser::match(Tok t) {
    if (!check(t)) return false;
    advance();
    return true;
}

const Token& Parser::expect(Tok t, const std::string& what) {
    if (check(t)) return advance();
    errorHere("expected " + what + " but found '" + peek().lexeme + "'");
}

void Parser::errorHere(const std::string& msg) {
    const Token& t = peek();
    sink_.add(t.line, t.col, msg);
    throw ParseError{};
}

// Panic-mode recovery: consume the offending token, then skip forward to
// the next plausible restart point — a ';' just consumed, an unconsumed
// '}', a token that starts a new statement, or a type keyword that starts
// a new declaration — so one bad construct costs one diagnostic, not the
// rest of the file.
void Parser::synchronize() {
    if (!isAtEnd()) advance();
    while (!isAtEnd()) {
        if (previous().type == Tok::SEMI) return;
        switch (peek().type) {
            case Tok::RBRACE:
            case Tok::KW_IF: case Tok::KW_WHILE: case Tok::KW_FOR: case Tok::KW_DO:
            case Tok::KW_RETURN: case Tok::KW_BREAK: case Tok::KW_CONTINUE:
            case Tok::KW_INT: case Tok::KW_CHAR: case Tok::KW_FLOAT: case Tok::KW_VOID:
            case Tok::KW_STRUCT: case Tok::KW_TYPEDEF: case Tok::KW_ENUM: case Tok::KW_SWITCH:
            case Tok::LBRACE:
                return;
            default:
                break;
        }
        advance();
    }
}

// ---- declarations --------------------------------------------------------

bool Parser::atTypeSpecStart() const {
    switch (peek().type) {
        case Tok::KW_INT: case Tok::KW_CHAR: case Tok::KW_FLOAT: case Tok::KW_VOID:
        case Tok::KW_LONG: case Tok::KW_SHORT: case Tok::KW_UNSIGNED:
        case Tok::KW_SIGNED: case Tok::KW_DOUBLE:
        case Tok::KW_CONST: case Tok::KW_STATIC: case Tok::KW_EXTERN:
        case Tok::KW_STRUCT: case Tok::KW_ENUM:
            return true;
        case Tok::IDENT:
            return ctx_.typedefs.count(peek().lexeme) > 0;
        default:
            return false;
    }
}

TypeSpec Parser::parseTypeSpec() {
    // Qualifiers that carry no meaning here are consumed and ignored:
    //   const/static/extern  -> ignored
    //   double -> float      (the interpreter's float is double)
    // Integer width and signedness are real: short/int/long and unsigned set
    // TypeSpec::intBits / isUnsigned, and arithmetic wraps accordingly.
    // Width and signedness words combine in any order: `unsigned long int`,
    // `long unsigned`, `short`. They are tallied, then applied after the loop.
    ParseContext::TypeWords w;
    sawStatic_ = false;

    for (;;) {
        switch (peek().type) {
            case Tok::KW_STATIC: sawStatic_ = true; advance(); continue;
            case Tok::KW_CONST: case Tok::KW_EXTERN:
                advance();
                continue;
            case Tok::KW_UNSIGNED: w.sawUnsigned = true; w.sawIntWord = true; w.t.base = BaseType::INT; w.sawAny = true; advance(); continue;
            case Tok::KW_SIGNED:   w.sawIntWord = true; w.t.base = BaseType::INT; w.sawAny = true; advance(); continue;
            case Tok::KW_LONG:     w.sawLong = true;  w.sawIntWord = true; w.t.base = BaseType::INT; w.sawAny = true; advance(); continue;
            case Tok::KW_SHORT:    w.sawShort = true; w.sawIntWord = true; w.t.base = BaseType::INT; w.sawAny = true; advance(); continue;
            case Tok::KW_INT:   w.t.base = BaseType::INT;   w.sawAny = true; w.sawIntWord = true; advance(); continue;
            case Tok::KW_CHAR:  w.t.base = BaseType::CHAR;  w.sawAny = true; advance(); break;
            case Tok::KW_FLOAT: w.t.base = BaseType::FLOAT; w.sawAny = true; advance(); break;
            case Tok::KW_DOUBLE:w.t.base = BaseType::FLOAT; w.t.isDouble = true; w.sawAny = true; advance(); break;
            case Tok::KW_VOID:  w.t.base = BaseType::VOID;  w.sawAny = true; advance(); break;
            case Tok::KW_STRUCT: advance(); w.t = parseStructSpecifier(); w.sawAny = true; break;
            case Tok::KW_ENUM:   advance(); parseEnumSpecifier(); w.t = TypeSpec{}; w.sawAny = true; break;
            case Tok::IDENT:
                // A typedef name is a type only where a type is expected; the
                // `!sawAny` guard keeps `unsigned x` from treating x as one.
                if (!w.sawAny && ctx_.typedefs.count(peek().lexeme)) {
                    w.t = ctx_.typedefs[peek().lexeme]; w.sawAny = true; advance();
                }
                break;
            default: break;
        }
        break;
    }
    if (!w.sawAny) errorHere("expected a type (int, char, float, void, struct or a typedef name)");
    return ctx_.finishType(w);
}

// struct [Name] [ { fields } ]. A bare `struct Name` is a reference (it may
// precede the definition, which is what lets a node hold a pointer to its own
// type); a body defines the layout. Field offsets are assigned here, once, so
// every later stage just reads them.
TypeSpec Parser::parseStructSpecifier() {
    std::string name;
    const Token& at = peek();
    if (check(Tok::IDENT)) name = advance().lexeme;
    int id;
    if (check(Tok::LBRACE)) {
        id = ctx_.beginStruct(name, at.line, at.col);
        try {
            advance();   // '{'
            while (!check(Tok::RBRACE) && !isAtEnd()) {
                TypeSpec ft = parseTypeSpec();
                do {
                    int stars = parseStars();
                    const Token& fn = expect(Tok::IDENT, "a field name");
                    TypeSpec fty = ft;
                    fty.ptrDepth += stars;
                    parseArrayDims(fty, false);
                    ctx_.addField(fty, fn);
                } while (match(Tok::COMMA));
                expect(Tok::SEMI, "';'");
            }
            expect(Tok::RBRACE, "'}'");
        } catch (const ParseError&) {
            ctx_.abandonStruct();
            throw;
        }
        ctx_.finishStruct();
    } else {
        if (name.empty()) errorHere("expected a struct name or '{' after 'struct'");
        id = ctx_.structReference(name, at.line, at.col);
    }
    TypeSpec t;
    t.base = BaseType::STRUCT;
    t.structId = id;
    return t;
}

// enum [Name] [ { A, B = 5, C } ]. The constants become plain integers at the
// point of use (see parsePrimary); the enum type itself is int.
void Parser::parseEnumSpecifier() {
    if (check(Tok::IDENT)) advance();
    if (!match(Tok::LBRACE)) return;
    ctx_.resetEnum();
    while (!check(Tok::RBRACE) && !isAtEnd()) {
        const Token& nm = expect(Tok::IDENT, "an enumerator name");
        bool hasValue = match(Tok::OP_ASSIGN);
        long long v = hasValue ? parseConstInt("an enumerator value") : 0;
        ctx_.addEnumerator(nm.lexeme, hasValue, v);
        if (!match(Tok::COMMA)) break;
    }
    expect(Tok::RBRACE, "'}'");
}

// typedef <type> [*...] Name [dims] { , ... } ;
void Parser::parseTypedef() {
    advance();   // 'typedef'
    TypeSpec base = parseTypeSpec();
    do {
        int stars = parseStars();
        if (check(Tok::LPAREN)) errorHere("typedefs of function pointers are not supported");
        const Token& nm = expect(Tok::IDENT, "a type name");
        TypeSpec t = base;
        t.ptrDepth += stars;
        parseArrayDims(t, false);
        ctx_.typedefs[nm.lexeme] = t;
    } while (match(Tok::COMMA));
    expect(Tok::SEMI, "';'");
}

long long Parser::parseConstInt(const char* what) {
    const Token& at = peek();
    auto e = parseTernary();
    return ctx_.constInt(*e, at.line, at.col, what);
}

void Parser::parseArrayDims(TypeSpec& t, bool allowEmptyFirst) {
    bool first = true;
    while (check(Tok::LBRACKET)) {
        const Token& lb = advance();
        ParseContext::Dim d;
        d.line = lb.line; d.col = lb.col;
        if (!check(Tok::RBRACKET)) {
            d.exprLine = peek().line; d.exprCol = peek().col;
            d.size = parseTernary();
        }
        ctx_.applyDim(t, d, first, allowEmptyFirst);
        expect(Tok::RBRACKET, "']'");
        first = false;
    }
}

// ---- brace initializers ----------------------------------------------------
//
// `struct P a[2] = {{1, 2}, {3, 4}};` and `int m[2][3] = {{1,2,3},{4,5,6}}`
// are parsed into a tree and then flattened against the declared type into
// exactly one expression per interpreter slot, padding with zeros. Sema, IRGen
// and the optimizer therefore never see a nested initializer.

Parser::InitNode Parser::parseInitNode() {
    InitNode n;
    n.line = peek().line; n.col = peek().col;
    if (match(Tok::LBRACE)) {
        n.isList = true;
        while (!check(Tok::RBRACE) && !isAtEnd()) {
            n.kids.push_back(parseInitNode());
            if (!match(Tok::COMMA)) break;
        }
        expect(Tok::RBRACE, "'}'");
    } else {
        n.expr = parseInitExpr();
    }
    return n;
}

int Parser::parseStars() {
    int n = 0;
    while (match(Tok::OP_STAR)) {
        ++n;
        while (check(Tok::KW_CONST)) advance();      // `int * const p`: the qualifier carries no meaning here
    }
    return n;
}

bool Parser::startsTypeName(size_t off) const {
    switch (peek(off).type) {
        case Tok::KW_INT: case Tok::KW_CHAR: case Tok::KW_FLOAT: case Tok::KW_VOID:
        case Tok::KW_LONG: case Tok::KW_SHORT: case Tok::KW_UNSIGNED: case Tok::KW_SIGNED:
        case Tok::KW_DOUBLE: case Tok::KW_CONST: case Tok::KW_STRUCT: case Tok::KW_ENUM:
            return true;
        case Tok::IDENT:
            return ctx_.typedefs.count(peek(off).lexeme) > 0;
        default:
            return false;
    }
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto prog = std::make_unique<Program>();
    while (!isAtEnd()) {
        try {
            parseTopLevelDeclaration(prog->decls);
        } catch (const ParseError&) {
            synchronize();
        }
    }
    prog->structs = std::move(ctx_.structs);
    return prog;
}

void Parser::parseTopLevelDeclaration(std::vector<std::unique_ptr<Decl>>& out) {
    if (check(Tok::KW_TYPEDEF)) { parseTypedef(); return; }
    if (!atTypeSpecStart())
        errorHere("expected a declaration (starting with int, char, float, void, struct or a typedef name)");

    TypeSpec type = parseTypeSpec();
    if (match(Tok::SEMI)) return;           // `struct S { ... };` / `enum E { ... };` defines a type only
    int stars = parseStars();
    const Token& nameTok = expect(Tok::IDENT, "an identifier");

    if (check(Tok::LPAREN)) {
        TypeSpec returnType = type;
        returnType.ptrDepth += stars;
        // A prototype -- a parameter list ending in ';' rather than a body --
        // is consumed and discarded. Functions are collected before any body
        // is checked, so a forward declaration carries no information this
        // compiler needs; rejecting it only breaks ordinary C style.
        size_t save = pos_;
        bool isPrototype = false;
        int depth = 0;
        for (size_t i = pos_; i < toks_.size(); ++i) {
            if (toks_[i].type == Tok::LPAREN) ++depth;
            else if (toks_[i].type == Tok::RPAREN) {
                if (--depth == 0) {
                    isPrototype = (i + 1 < toks_.size() && toks_[i + 1].type == Tok::SEMI);
                    break;
                }
            }
        }
        if (isPrototype) {
            while (!check(Tok::SEMI) && !isAtEnd()) advance();
            expect(Tok::SEMI, "';'");
            return;
        }
        pos_ = save;
        out.push_back(parseFuncDeclRest(returnType, nameTok));
    } else {
        for (auto& d : parseVarDeclDeclarators(type, stars, nameTok))
            out.push_back(std::move(d));
    }
}

std::unique_ptr<VarDecl> Parser::parseOneDeclarator(TypeSpec type, const Token& nameTok) {
    // `int a[] = {...}` leaves the first length empty (-1) until the
    // initializer is known (ParseContext::declarator).
    parseArrayDims(type, true);
    ParseContext::Initializer init;
    if (match(Tok::OP_ASSIGN)) {
        if (check(Tok::LBRACE)) {
            init.kind = ParseContext::Initializer::LIST;
            init.list = parseInitNode();
        } else if (type.isArray && check(Tok::STRING_LIT)) {
            init.kind = ParseContext::Initializer::STRING;
            init.str = advance();
        } else {
            init.kind = ParseContext::Initializer::EXPR;
            init.expr = parseInitExpr();
        }
    }
    return ctx_.declarator(type, nameTok, std::move(init));
}

// declaration ::= type_spec IDENT [...] [ = initializer ] { "," IDENT [...] [ = initializer ] } ";"
// desugared into one VarDecl per name — see the note on VarDecl in ast.h.
std::vector<std::unique_ptr<VarDecl>> Parser::parseVarDeclDeclarators(TypeSpec baseType, int firstStars, const Token& firstName) {
    std::vector<std::unique_ptr<VarDecl>> result;
    TypeSpec first = baseType;
    first.ptrDepth += firstStars;
    result.push_back(parseOneDeclarator(first, firstName));
    while (match(Tok::COMMA)) {
        // `int *p, q;` -- the star binds to the declarator, so q is a plain int.
        int stars = parseStars();
        const Token& nameTok = expect(Tok::IDENT, "an identifier");
        TypeSpec t = baseType;
        t.ptrDepth += stars;
        result.push_back(parseOneDeclarator(t, nameTok));
    }
    expect(Tok::SEMI, "';'");
    return result;
}

std::unique_ptr<FuncDecl> Parser::parseFuncDeclRest(TypeSpec returnType, const Token& nameTok) {
    auto fn = std::make_unique<FuncDecl>(returnType, nameTok.lexeme);
    fn->line = nameTok.line;
    fn->col  = nameTok.col;

    expect(Tok::LPAREN, "'('");
    if (check(Tok::KW_VOID) && peek(1).type == Tok::RPAREN) advance();   // f(void) means no parameters
    if (!check(Tok::RPAREN)) {
        do {
            TypeSpec ptype = parseTypeSpec();
            ptype.ptrDepth += parseStars();
            const Token& pname = expect(Tok::IDENT, "a parameter name");
            Param p;
            p.type = ptype;
            p.name = pname.lexeme;
            p.line = pname.line;
            p.col  = pname.col;
            if (check(Tok::LBRACKET)) {
                // `int a[]`, `int a[10]` and `int m[][4]` all pass a pointer;
                // the first length is ignored but the inner ones are kept so
                // m[i][j] knows the row width.
                parseArrayDims(p.type, true);
                p.type.arraySize = -1;
            }
            fn->params.push_back(std::move(p));
        } while (match(Tok::COMMA));
    }
    expect(Tok::RPAREN, "')'");
    fn->body = parseCompoundStmt();
    return fn;
}

// ---- statements -----------------------------------------------------------

std::unique_ptr<CompoundStmt> Parser::parseCompoundStmt() {
    const Token& lb = expect(Tok::LBRACE, "'{'");
    auto cs = std::make_unique<CompoundStmt>();
    cs->line = lb.line;
    cs->col  = lb.col;

    // compound_stmt ::= "{" { var_decl | statement } "}"
    //
    // Relaxed from the original "{ var_decl } { statement }" (the C89 rule),
    // which rejected the extremely common `int base = n * 8;` appearing after
    // a statement. A declaration found here is wrapped in a DeclStmt and kept
    // in the statement list, so its initializer still runs at the point it was
    // written -- hoisting it into `locals` would evaluate it too early.
    while (!check(Tok::RBRACE) && !isAtEnd()) {
        try {
            if (check(Tok::KW_TYPEDEF)) {
                parseTypedef();
            } else if (atTypeSpecStart()) {
                parseLocalDeclaration(cs->stmts);
            } else if (auto s = parseStatement()) {
                cs->stmts.push_back(std::move(s));
            }
        } catch (const ParseError&) {
            // An error found AT this block's '}' (`return }`, `x = 1 }`) must
            // leave the brace for the block: consuming it would keep the
            // function open and turn everything after it into a cascade of
            // bogus errors. The loop condition then closes the block normally.
            if (!check(Tok::RBRACE)) synchronize();
        }
    }
    expect(Tok::RBRACE, "'}'");
    return cs;
}

bool Parser::parseLocalDeclaration(std::vector<std::unique_ptr<Stmt>>& out) {
    TypeSpec type = parseTypeSpec();
    bool isStatic = sawStatic_;
    if (match(Tok::SEMI)) return false;     // a bare `struct S { ... };` inside a function
    int stars = parseStars();
    const Token& nameTok = expect(Tok::IDENT, "an identifier");
    for (auto& d : parseVarDeclDeclarators(type, stars, nameTok)) {
        d->isStatic = isStatic;
        auto ds = std::make_unique<DeclStmt>();
        ds->line = d->line;
        ds->col  = d->col;
        ds->decl = std::move(d);
        out.push_back(std::move(ds));
    }
    return true;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    // `name:` at the start of a statement is a label. (`x ? a : b;` starts with
    // an identifier too, but its second token is '?', never ':'.)
    if (check(Tok::IDENT) && peek(1).type == Tok::OP_COLON) {
        const Token& nm = advance();
        advance();                                   // ':'
        std::unique_ptr<Stmt> inner;
        if (check(Tok::RBRACE)) {
            inner = std::make_unique<ExprStmt>(nullptr);   // `label: }` -- an empty statement
            inner->line = nm.line; inner->col = nm.col;
        } else {
            inner = parseStatement();
        }
        auto ls = std::make_unique<LabelStmt>(nm.lexeme, std::move(inner));
        ls->line = nm.line; ls->col = nm.col;
        return ls;
    }
    switch (peek().type) {
        case Tok::KW_SWITCH:   return parseSwitchStmt();
        case Tok::KW_GOTO:     return parseGotoStmt();
        case Tok::LBRACE:      return parseCompoundStmt();
        case Tok::KW_IF:       return parseIfStmt();
        case Tok::KW_WHILE:    return parseWhileStmt();
        case Tok::KW_FOR:      return parseForStmt();
        case Tok::KW_DO:       return parseDoStmt();
        case Tok::KW_RETURN:   return parseReturnStmt();
        case Tok::KW_BREAK:    return parseBreakStmt();
        case Tok::KW_CONTINUE: return parseContinueStmt();
        default:                return parseExprStmt();
    }
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
    const Token& t = peek();
    std::unique_ptr<Expr> e;
    if (!check(Tok::SEMI)) e = parseExpression();
    expect(Tok::SEMI, "';'");
    auto s = std::make_unique<ExprStmt>(std::move(e));
    s->line = t.line; s->col = t.col;
    return s;
}

// Dangling else: binds to the nearest unmatched 'if' — the standard rule,
// obtained here for free because the recursive call for the 'then' branch
// always runs (and consumes any 'else' it can) before this function looks
// for its own 'else'.
std::unique_ptr<Stmt> Parser::parseIfStmt() {
    const Token& kw = expect(Tok::KW_IF, "'if'");
    expect(Tok::LPAREN, "'('");
    auto cond = parseExpression();
    expect(Tok::RPAREN, "')'");
    auto thenS = parseStatement();
    std::unique_ptr<Stmt> elseS;
    if (match(Tok::KW_ELSE)) elseS = parseStatement();
    auto s = std::make_unique<IfStmt>(std::move(cond), std::move(thenS), std::move(elseS));
    s->line = kw.line; s->col = kw.col;
    return s;
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    const Token& kw = expect(Tok::KW_WHILE, "'while'");
    expect(Tok::LPAREN, "'('");
    auto cond = parseExpression();
    expect(Tok::RPAREN, "')'");
    auto body = parseStatement();
    auto s = std::make_unique<WhileStmt>(std::move(cond), std::move(body));
    s->line = kw.line; s->col = kw.col;
    return s;
}

std::unique_ptr<Stmt> Parser::parseDoStmt() {
    const Token& kw = expect(Tok::KW_DO, "'do'");
    auto body = parseStatement();
    expect(Tok::KW_WHILE, "'while'");
    expect(Tok::LPAREN, "'('");
    auto cond = parseExpression();
    expect(Tok::RPAREN, "')'");
    expect(Tok::SEMI, "';'");
    auto s = std::make_unique<DoStmt>(std::move(body), std::move(cond));
    s->line = kw.line; s->col = kw.col;
    return s;
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    const Token& kw = expect(Tok::KW_FOR, "'for'");
    expect(Tok::LPAREN, "'('");
    std::unique_ptr<Expr> init, cond, step;

    // `for (int i = 0; ...)` is desugared into
    //     { int i = 0; for (; ...) body }
    // rather than extending ForStmt to hold a declaration. The synthetic
    // block gives the counter exactly the scope C gives it -- visible to the
    // loop, gone afterwards -- and needs no change to Sema, IRGen or the
    // printer, which all already handle a block containing a DeclStmt.
    std::vector<std::unique_ptr<Stmt>> loopDecls;
    if (atTypeSpecStart()) {
        TypeSpec dt = parseTypeSpec();
        int dstars = parseStars();
        const Token& dn = expect(Tok::IDENT, "an identifier");
        for (auto& d : parseVarDeclDeclarators(dt, dstars, dn)) {
            auto ds = std::make_unique<DeclStmt>();
            ds->line = d->line; ds->col = d->col;
            ds->decl = std::move(d);
            loopDecls.push_back(std::move(ds));
        }
        // parseVarDeclDeclarators consumed the ';' that ends the init clause.
        // (The condition may be empty: `for (int i = 0; ; i++)`. This path
        // used to be skipped when it was, which lost the declaration and read
        // `i++` as the condition.)
        if (!check(Tok::SEMI)) cond = parseExpression();
        expect(Tok::SEMI, "';'");
        if (!check(Tok::RPAREN)) step = parseExpression();
        expect(Tok::RPAREN, "')'");
        auto body0 = parseStatement();
        auto inner = std::make_unique<ForStmt>(nullptr, std::move(cond), std::move(step), std::move(body0));
        inner->line = kw.line; inner->col = kw.col;
        auto block = std::make_unique<CompoundStmt>();
        block->line = kw.line; block->col = kw.col;
        for (auto& d : loopDecls) block->stmts.push_back(std::move(d));
        block->stmts.push_back(std::move(inner));
        return block;
    }

    if (!check(Tok::SEMI)) init = parseExpression();
    expect(Tok::SEMI, "';'");
    if (!check(Tok::SEMI)) cond = parseExpression();
    expect(Tok::SEMI, "';'");
    if (!check(Tok::RPAREN)) step = parseExpression();
    expect(Tok::RPAREN, "')'");
    auto body = parseStatement();
    auto s = std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(step), std::move(body));
    s->line = kw.line; s->col = kw.col;
    return s;
}

std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    const Token& kw = expect(Tok::KW_RETURN, "'return'");
    std::unique_ptr<Expr> val;
    if (!check(Tok::SEMI)) val = parseExpression();
    expect(Tok::SEMI, "';'");
    auto s = std::make_unique<ReturnStmt>(std::move(val));
    s->line = kw.line; s->col = kw.col;
    return s;
}

std::unique_ptr<Stmt> Parser::parseBreakStmt() {
    const Token& kw = expect(Tok::KW_BREAK, "'break'");
    expect(Tok::SEMI, "';'");
    auto s = std::make_unique<BreakStmt>();
    s->line = kw.line; s->col = kw.col;
    return s;
}

std::unique_ptr<Stmt> Parser::parseGotoStmt() {
    const Token& kw = expect(Tok::KW_GOTO, "'goto'");
    const Token& lab = expect(Tok::IDENT, "a label name");
    expect(Tok::SEMI, "';'");
    auto s = std::make_unique<GotoStmt>(lab.lexeme);
    s->line = kw.line; s->col = kw.col;
    return s;
}

// switch (e) { case K: ... default: ... }
std::unique_ptr<Stmt> Parser::parseSwitchStmt() {
    const Token& kw = expect(Tok::KW_SWITCH, "'switch'");
    expect(Tok::LPAREN, "'('");
    auto sw = std::make_unique<SwitchStmt>();
    sw->line = kw.line; sw->col = kw.col;
    sw->cond = parseExpression();
    expect(Tok::RPAREN, "')'");
    expect(Tok::LBRACE, "'{'");
    while (!check(Tok::RBRACE) && !isAtEnd()) {
        try {
            if (check(Tok::KW_CASE) || check(Tok::KW_DEFAULT)) {
                SwitchCase sc;
                const Token& lab = advance();
                sc.line = lab.line; sc.col = lab.col;
                if (lab.type == Tok::KW_DEFAULT) sc.isDefault = true;
                else sc.value = parseConstInt("a case label");
                expect(Tok::OP_COLON, "':'");
                sw->cases.push_back(std::move(sc));
            } else {
                if (sw->cases.empty())
                    errorHere("statement before the first 'case' or 'default' in a switch");
                if (check(Tok::KW_TYPEDEF)) parseTypedef();
                else if (atTypeSpecStart()) parseLocalDeclaration(sw->cases.back().stmts);
                else if (auto s = parseStatement()) sw->cases.back().stmts.push_back(std::move(s));
            }
        } catch (const ParseError&) {
            if (!check(Tok::RBRACE)) synchronize();   // same reason as in a compound statement
        }
    }
    expect(Tok::RBRACE, "'}'");
    return sw;
}

std::unique_ptr<Stmt> Parser::parseContinueStmt() {
    const Token& kw = expect(Tok::KW_CONTINUE, "'continue'");
    expect(Tok::SEMI, "';'");
    auto s = std::make_unique<ContinueStmt>();
    s->line = kw.line; s->col = kw.col;
    return s;
}

// ---- expressions: precedence-climbing, lowest precedence first -----------
// Levels follow docs/grammar.ebnf exactly: assignment (lowest) > logical_or
// > logical_and > equality > relational > additive > multiplicative > unary
// > postfix > primary (highest).

std::unique_ptr<Expr> Parser::parseExpression() {
    // Checked before the counter is raised, so the throw needs no unwinding
    // of its own; the guard below restores the depth on the normal path and
    // on the ParseError thrown by any nested errorHere().
    if (exprDepth_ >= kMaxExprDepth)
        errorHere("expression nests too deeply (limit " +
                  std::to_string(kMaxExprDepth) + ")");
    struct DepthGuard {
        int& d;
        explicit DepthGuard(int& dd) : d(dd) { ++d; }
        ~DepthGuard() { --d; }
    } guard(exprDepth_);
    auto e = parseAssignment();
    // The comma operator: evaluate the left side, discard it, yield the right.
    // Only here: call arguments, initializer elements and declarator lists use
    // parseInitExpr, where a comma separates items instead.
    while (check(Tok::COMMA)) {
        const Token& op = advance();
        auto r = parseAssignment();
        auto ce = std::make_unique<CommaExpr>(std::move(e), std::move(r));
        ce->line = op.line; ce->col = op.col;
        e = std::move(ce);
    }
    return e;
}

std::unique_ptr<Expr> Parser::parseInitExpr() {
    if (exprDepth_ >= kMaxExprDepth)
        errorHere("expression nests too deeply (limit " +
                  std::to_string(kMaxExprDepth) + ")");
    struct DepthGuard {
        int& d;
        explicit DepthGuard(int& dd) : d(dd) { ++d; }
        ~DepthGuard() { --d; }
    } guard(exprDepth_);
    return parseAssignment();
}

// conditional ::= logical_or [ "?" expression ":" conditional ]
std::unique_ptr<Expr> Parser::parseTernary() {
    auto cond = parseLogicalOr();
    if (!check(Tok::OP_QUESTION)) return cond;
    const Token& q = advance();
    auto a = parseExpression();
    expect(Tok::OP_COLON, "':'");
    auto b = parseTernary();
    auto e = std::make_unique<CondExpr>(std::move(cond), std::move(a), std::move(b));
    e->line = q.line; e->col = q.col;
    return e;
}

// assignment ::= lvalue ("=" | "+=" | "-=" | "*=" | "/=") assignment | logical_or
// Parsed by first parsing the full lower-precedence expression, then
// re-interpreting it as an lvalue only if an assignment operator follows —
// this is what the grammar file means by "implemented by precedence
// climbing" rather than a literal predictive lvalue-first production.
std::unique_ptr<Expr> Parser::parseAssignment() {
    const Token& start = peek();
    auto left = parseTernary();
    if (isAssignOpTok(peek().type)) {
        bool isDeref = left->kind == NodeKind::UnaryExpr
                    && static_cast<UnaryExpr*>(left.get())->op == UnOp::DEREF;
        if (left->kind != NodeKind::IdentExpr && left->kind != NodeKind::IndexExpr
            && left->kind != NodeKind::MemberExpr && !isDeref) {
            sink_.add(start.line, start.col,
                       "left-hand side of assignment must be a variable, array element, struct field or dereferenced pointer");
        }
        Tok opTok = advance().type;
        auto right = parseAssignment();   // right-associative
        auto e = std::make_unique<AssignExpr>(toAssignOp(opTok), std::move(left), std::move(right));
        e->line = start.line; e->col = start.col;
        return e;
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (check(Tok::OP_OR)) {
        const Token& op = advance();
        auto right = parseLogicalAnd();
        auto e = std::make_unique<BinaryExpr>(BinOp::LOGOR, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseBitOr();
    while (check(Tok::OP_AND)) {
        const Token& op = advance();
        auto right = parseBitOr();
        auto e = std::make_unique<BinaryExpr>(BinOp::LOGAND, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

// bitwise_or ::= bitwise_xor { "|" bitwise_xor }
std::unique_ptr<Expr> Parser::parseBitOr() {
    auto left = parseBitXor();
    while (check(Tok::OP_BITOR)) {
        const Token& op = advance();
        auto right = parseBitXor();
        auto e = std::make_unique<BinaryExpr>(BinOp::BITOR, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

// bitwise_xor ::= bitwise_and { "^" bitwise_and }
std::unique_ptr<Expr> Parser::parseBitXor() {
    auto left = parseBitAnd();
    while (check(Tok::OP_BITXOR)) {
        const Token& op = advance();
        auto right = parseBitAnd();
        auto e = std::make_unique<BinaryExpr>(BinOp::BITXOR, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

// bitwise_and ::= equality { "&" equality }
std::unique_ptr<Expr> Parser::parseBitAnd() {
    auto left = parseEquality();
    while (check(Tok::OP_BITAND)) {
        const Token& op = advance();
        auto right = parseEquality();
        auto e = std::make_unique<BinaryExpr>(BinOp::BITAND, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseRelational();
    while (check(Tok::OP_EQ) || check(Tok::OP_NE)) {
        const Token& op = advance();
        BinOp b = (op.type == Tok::OP_EQ) ? BinOp::EQ : BinOp::NE;
        auto right = parseRelational();
        auto e = std::make_unique<BinaryExpr>(b, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

// shift ::= additive { ("<<" | ">>") additive }
std::unique_ptr<Expr> Parser::parseShift() {
    auto left = parseAdditive();
    while (check(Tok::OP_SHL) || check(Tok::OP_SHR)) {
        const Token& op = advance();
        BinOp b = (op.type == Tok::OP_SHL) ? BinOp::SHL : BinOp::SHR;
        auto right = parseAdditive();
        auto e = std::make_unique<BinaryExpr>(b, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseRelational() {
    auto left = parseShift();
    while (check(Tok::OP_LT) || check(Tok::OP_LE) || check(Tok::OP_GT) || check(Tok::OP_GE)) {
        const Token& op = advance();
        BinOp b = op.type == Tok::OP_LT ? BinOp::LT : op.type == Tok::OP_LE ? BinOp::LE
                : op.type == Tok::OP_GT ? BinOp::GT : BinOp::GE;
        auto right = parseShift();
        auto e = std::make_unique<BinaryExpr>(b, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(Tok::OP_PLUS) || check(Tok::OP_MINUS)) {
        const Token& op = advance();
        BinOp b = (op.type == Tok::OP_PLUS) ? BinOp::ADD : BinOp::SUB;
        auto right = parseMultiplicative();
        auto e = std::make_unique<BinaryExpr>(b, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(Tok::OP_STAR) || check(Tok::OP_SLASH) || check(Tok::OP_PERCENT)) {
        const Token& op = advance();
        BinOp b = op.type == Tok::OP_STAR ? BinOp::MUL : op.type == Tok::OP_SLASH ? BinOp::DIV : BinOp::MOD;
        auto right = parseUnary();
        auto e = std::make_unique<BinaryExpr>(b, std::move(left), std::move(right));
        e->line = op.line; e->col = op.col;
        left = std::move(e);
    }
    return left;
}

// unary ::= ("!" | "-" | "+" | "++" | "--") unary | postfix — right-recursive,
// so `--!x` etc. parse (semantics of that combination is sema's problem).
std::unique_ptr<Expr> Parser::parseUnary() {
    // A cast: `(` followed by a type name. `(x)` and `(1 + 2)` do not start
    // with a type keyword, so they fall through to parsePrimary as before.
    if (check(Tok::LPAREN) && startsTypeName(1)) {
        const Token& lp = advance();
        TypeSpec target = parseTypeSpec();
        target.ptrDepth += parseStars();
        expect(Tok::RPAREN, "')'");
        auto operand = parseUnary();
        auto c = std::make_unique<CastExpr>(target, std::move(operand));
        c->line = lp.line; c->col = lp.col;
        return c;
    }

    if (check(Tok::KW_SIZEOF)) {
        const Token& kw = advance();
        auto sz = std::make_unique<SizeofExpr>();
        sz->line = kw.line; sz->col = kw.col;
        if (check(Tok::LPAREN) && startsTypeName(1)) {
            advance();
            sz->ofType = true;
            sz->type = parseTypeSpec();
            sz->type.ptrDepth += parseStars();
            parseArrayDims(sz->type, false);
            expect(Tok::RPAREN, "')'");
            sz->value = ctx_.sizeofType(sz->type);
        } else {
            sz->operand = parseUnary();
        }
        return sz;
    }

    UnOp op;
    switch (peek().type) {
        case Tok::OP_BITAND: op = UnOp::ADDR;  break;     // address-of
        case Tok::OP_STAR:   op = UnOp::DEREF; break;     // dereference
        case Tok::OP_NOT:    op = UnOp::NOT;    break;
        case Tok::OP_BITNOT: op = UnOp::BITNOT; break;
        case Tok::OP_MINUS: op = UnOp::NEG;     break;
        case Tok::OP_PLUS:  op = UnOp::POS;     break;
        case Tok::OP_INC:   op = UnOp::PRE_INC; break;
        case Tok::OP_DEC:   op = UnOp::PRE_DEC; break;
        default:
            return parsePostfix();
    }
    const Token& opTok = advance();
    auto operand = parseUnary();
    auto e = std::make_unique<UnaryExpr>(op, std::move(operand));
    e->line = opTok.line; e->col = opTok.col;
    return e;
}

// postfix ::= primary { "[" expression "]" | "(" [args] ")" | "++" | "--" }
std::unique_ptr<Expr> Parser::parsePostfix() {
    auto e = parsePrimary();
    for (;;) {
        if (check(Tok::LBRACKET)) {
            int l = e->line, c = e->col;
            advance();
            auto idx = parseExpression();
            expect(Tok::RBRACKET, "']'");
            auto ie = std::make_unique<IndexExpr>(std::move(e), std::move(idx));
            ie->line = l; ie->col = c;
            e = std::move(ie);
        } else if (check(Tok::LPAREN)) {
            int l = e->line, c = e->col;
            std::string callee;
            if (e->kind == NodeKind::IdentExpr) callee = static_cast<IdentExpr*>(e.get())->name;
            else sink_.add(l, c, "'(' follows an expression that is not a function name");
            advance();
            auto args = check(Tok::RPAREN) ? std::vector<std::unique_ptr<Expr>>{} : parseArgs();
            expect(Tok::RPAREN, "')'");
            auto ce = std::make_unique<CallExpr>(callee);
            ce->args = std::move(args);
            ce->line = l; ce->col = c;
            e = std::move(ce);
        } else if (check(Tok::OP_DOT) || check(Tok::OP_ARROW)) {
            bool arrow = check(Tok::OP_ARROW);
            int l = e->line, c = e->col;
            advance();
            const Token& fld = expect(Tok::IDENT, "a field name");
            auto me = std::make_unique<MemberExpr>(std::move(e), fld.lexeme, arrow);
            me->line = l; me->col = c;
            e = std::move(me);
        } else if (check(Tok::OP_INC)) {
            int l = e->line, c = e->col;
            advance();
            auto ue = std::make_unique<UnaryExpr>(UnOp::POST_INC, std::move(e));
            ue->line = l; ue->col = c;
            e = std::move(ue);
        } else if (check(Tok::OP_DEC)) {
            int l = e->line, c = e->col;
            advance();
            auto ue = std::make_unique<UnaryExpr>(UnOp::POST_DEC, std::move(e));
            ue->line = l; ue->col = c;
            e = std::move(ue);
        } else {
            break;
        }
    }
    return e;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    const Token& t = peek();
    switch (t.type) {
        case Tok::IDENT: {
            advance();
            auto ec = ctx_.enumConsts.find(t.lexeme);
            if (ec != ctx_.enumConsts.end()) {
                auto lit = std::make_unique<IntLit>(ec->second);
                lit->line = t.line; lit->col = t.col;
                return lit;
            }
            auto e = std::make_unique<IdentExpr>(t.lexeme);
            e->line = t.line; e->col = t.col;
            return e;
        }
        case Tok::INT_LIT: {
            advance();
            // stoull, not stoll: an unsigned literal may exceed LLONG_MAX.
            auto e = std::make_unique<IntLit>(static_cast<long long>(std::stoull(t.lexeme)));
            for (char ch : t.lexeme) {
                if (ch == 'u' || ch == 'U') e->unsignedSuffix = true;
                if (ch == 'l' || ch == 'L') e->longSuffix = true;
            }
            e->line = t.line; e->col = t.col;
            return e;
        }
        case Tok::FLOAT_LIT: {
            advance();
            auto e = std::make_unique<FloatLit>(std::stod(t.lexeme));
            e->line = t.line; e->col = t.col;
            return e;
        }
        case Tok::CHAR_LIT: {
            advance();
            auto e = std::make_unique<CharLit>(decodeCharLiteral(t.lexeme));
            e->line = t.line; e->col = t.col;
            return e;
        }
        case Tok::STRING_LIT: {
            advance();
            // The lexeme is the already-decoded text (see scanStringLiteral).
            auto e = std::make_unique<StringLit>(t.lexeme);
            e->line = t.line; e->col = t.col;
            return e;
        }
        case Tok::LPAREN: {
            advance();
            auto e = parseExpression();
            expect(Tok::RPAREN, "')'");
            return e;
        }
        default:
            errorHere("expected an expression but found '" + t.lexeme + "'");
    }
}

std::vector<std::unique_ptr<Expr>> Parser::parseArgs() {
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(parseInitExpr());
    while (match(Tok::COMMA)) args.push_back(parseInitExpr());
    return args;
}

} // namespace minic
