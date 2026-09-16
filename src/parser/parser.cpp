// MiniC-Opt — recursive-descent / Pratt parser (implementation)
// Owner: Member 1 (Front End)
#include "parser.h"

namespace minic {

// ---- small free helpers ---------------------------------------------

static bool isAssignOpTok(Tok t) {
    switch (t) {
        case Tok::OP_ASSIGN: case Tok::OP_PLUS_ASSIGN: case Tok::OP_MINUS_ASSIGN:
        case Tok::OP_STAR_ASSIGN: case Tok::OP_SLASH_ASSIGN:
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
            return true;
        default:
            return false;
    }
}

TypeSpec Parser::parseTypeSpec() {
    TypeSpec t;
    switch (peek().type) {
        case Tok::KW_INT:   t.base = BaseType::INT;   advance(); break;
        case Tok::KW_CHAR:  t.base = BaseType::CHAR;  advance(); break;
        case Tok::KW_FLOAT: t.base = BaseType::FLOAT; advance(); break;
        case Tok::KW_VOID:  t.base = BaseType::VOID;  advance(); break;
        default:
            errorHere("expected a type (int, char, float or void)");
    }
    return t;
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
    return prog;
}

void Parser::parseTopLevelDeclaration(std::vector<std::unique_ptr<Decl>>& out) {
    if (!atTypeSpecStart())
        errorHere("expected a declaration (starting with int, char, float or void)");

    TypeSpec type = parseTypeSpec();
    const Token& nameTok = expect(Tok::IDENT, "an identifier");

    if (check(Tok::LPAREN)) {
        out.push_back(parseFuncDeclRest(type, nameTok));
    } else {
        for (auto& d : parseVarDeclDeclarators(type, nameTok))
            out.push_back(std::move(d));
    }
}

std::unique_ptr<VarDecl> Parser::parseOneDeclarator(TypeSpec type, const Token& nameTok) {
    auto decl = std::make_unique<VarDecl>(type, nameTok.lexeme);
    decl->line = nameTok.line;
    decl->col  = nameTok.col;

    if (match(Tok::LBRACKET)) {
        const Token& sizeTok = expect(Tok::INT_LIT, "an array size");
        decl->type.isArray   = true;
        decl->type.arraySize = std::stoi(sizeTok.lexeme);
        expect(Tok::RBRACKET, "']'");
    }
    if (match(Tok::OP_ASSIGN)) {
        if (match(Tok::LBRACE)) {
            decl->initList.push_back(parseExpression());
            while (match(Tok::COMMA)) decl->initList.push_back(parseExpression());
            expect(Tok::RBRACE, "'}'");
        } else {
            decl->init = parseExpression();
        }
    }
    return decl;
}

// declaration ::= type_spec IDENT [...] [ = initializer ] { "," IDENT [...] [ = initializer ] } ";"
// desugared into one VarDecl per name — see the note on VarDecl in ast.h.
std::vector<std::unique_ptr<VarDecl>> Parser::parseVarDeclDeclarators(TypeSpec type, const Token& firstName) {
    std::vector<std::unique_ptr<VarDecl>> result;
    result.push_back(parseOneDeclarator(type, firstName));
    while (match(Tok::COMMA)) {
        const Token& nameTok = expect(Tok::IDENT, "an identifier");
        result.push_back(parseOneDeclarator(type, nameTok));
    }
    expect(Tok::SEMI, "';'");
    return result;
}

std::unique_ptr<FuncDecl> Parser::parseFuncDeclRest(TypeSpec returnType, const Token& nameTok) {
    auto fn = std::make_unique<FuncDecl>(returnType, nameTok.lexeme);
    fn->line = nameTok.line;
    fn->col  = nameTok.col;

    expect(Tok::LPAREN, "'('");
    if (!check(Tok::RPAREN)) {
        do {
            TypeSpec ptype = parseTypeSpec();
            const Token& pname = expect(Tok::IDENT, "a parameter name");
            Param p;
            p.type = ptype;
            p.name = pname.lexeme;
            p.line = pname.line;
            p.col  = pname.col;
            if (match(Tok::LBRACKET)) {
                expect(Tok::RBRACKET, "']'");
                p.type.isArray   = true;
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

    // compound_stmt ::= "{" { var_decl } { statement } "}" — all locals
    // must precede all statements, so we stop scanning for locals the
    // moment the next token doesn't start a type_spec.
    while (atTypeSpecStart()) {
        try {
            TypeSpec type = parseTypeSpec();
            const Token& nameTok = expect(Tok::IDENT, "an identifier");
            for (auto& d : parseVarDeclDeclarators(type, nameTok))
                cs->locals.push_back(std::move(d));
        } catch (const ParseError&) {
            synchronize();
        }
    }
    while (!check(Tok::RBRACE) && !isAtEnd()) {
        try {
            if (auto s = parseStatement()) cs->stmts.push_back(std::move(s));
        } catch (const ParseError&) {
            synchronize();
        }
    }
    expect(Tok::RBRACE, "'}'");
    return cs;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    switch (peek().type) {
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

std::unique_ptr<Expr> Parser::parseExpression() { return parseAssignment(); }

// assignment ::= lvalue ("=" | "+=" | "-=" | "*=" | "/=") assignment | logical_or
// Parsed by first parsing the full lower-precedence expression, then
// re-interpreting it as an lvalue only if an assignment operator follows —
// this is what the grammar file means by "implemented by precedence
// climbing" rather than a literal predictive lvalue-first production.
std::unique_ptr<Expr> Parser::parseAssignment() {
    const Token& start = peek();
    auto left = parseLogicalOr();
    if (isAssignOpTok(peek().type)) {
        if (left->kind != NodeKind::IdentExpr && left->kind != NodeKind::IndexExpr) {
            sink_.add(start.line, start.col,
                       "left-hand side of assignment must be a variable or array element");
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
    auto left = parseEquality();
    while (check(Tok::OP_AND)) {
        const Token& op = advance();
        auto right = parseEquality();
        auto e = std::make_unique<BinaryExpr>(BinOp::LOGAND, std::move(left), std::move(right));
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

std::unique_ptr<Expr> Parser::parseRelational() {
    auto left = parseAdditive();
    while (check(Tok::OP_LT) || check(Tok::OP_LE) || check(Tok::OP_GT) || check(Tok::OP_GE)) {
        const Token& op = advance();
        BinOp b = op.type == Tok::OP_LT ? BinOp::LT : op.type == Tok::OP_LE ? BinOp::LE
                : op.type == Tok::OP_GT ? BinOp::GT : BinOp::GE;
        auto right = parseAdditive();
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
    UnOp op;
    switch (peek().type) {
        case Tok::OP_NOT:   op = UnOp::NOT;     break;
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
            auto e = std::make_unique<IdentExpr>(t.lexeme);
            e->line = t.line; e->col = t.col;
            return e;
        }
        case Tok::INT_LIT: {
            advance();
            auto e = std::make_unique<IntLit>(std::stoll(t.lexeme));
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
    args.push_back(parseExpression());
    while (match(Tok::COMMA)) args.push_back(parseExpression());
    return args;
}

} // namespace minic
