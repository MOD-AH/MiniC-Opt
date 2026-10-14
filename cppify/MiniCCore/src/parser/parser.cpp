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
    : toks_(std::move(tokens)), file_(std::move(filename)) {
    // The names <stdbool.h>, <stddef.h> and <stdint.h> would have supplied.
    // Every one of them is an integer here.
    for (const char* n : {"bool", "size_t", "ssize_t", "int8_t", "int16_t", "int32_t", "int64_t",
                          "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t",
                          "ptrdiff_t", "time_t", "clock_t", "FILE"})
        typedefs_[n] = TypeSpec{};
}

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
            return typedefs_.count(peek().lexeme) > 0;
        default:
            return false;
    }
}

TypeSpec Parser::parseTypeSpec() {
    // MiniC has exactly four types. Ordinary C spells many declarations with
    // qualifiers and width modifiers that carry no meaning here, so rather
    // than reject the whole declaration they are consumed and folded:
    //   const/static/extern  -> ignored
    //   long/short/unsigned/signed -> int   (the interpreter's int is 64-bit)
    //   double -> float                     (the interpreter's float is double)
    // This is a deliberate widening of what parses, not of what the language
    // can express: the resulting program behaves exactly as the folded type.
    TypeSpec t;
    bool sawAny = false;

    for (;;) {
        switch (peek().type) {
            case Tok::KW_CONST: case Tok::KW_STATIC: case Tok::KW_EXTERN:
                advance();
                continue;
            case Tok::KW_UNSIGNED: case Tok::KW_SIGNED:
            case Tok::KW_LONG:     case Tok::KW_SHORT:
                t.base = BaseType::INT; sawAny = true; advance();
                continue;             // `unsigned long int` collapses to int
            case Tok::KW_INT:   t.base = BaseType::INT;   sawAny = true; advance(); continue;
            case Tok::KW_CHAR:  t.base = BaseType::CHAR;  sawAny = true; advance(); break;
            case Tok::KW_FLOAT: t.base = BaseType::FLOAT; sawAny = true; advance(); break;
            case Tok::KW_DOUBLE:t.base = BaseType::FLOAT; sawAny = true; advance(); break;
            case Tok::KW_VOID:  t.base = BaseType::VOID;  sawAny = true; advance(); break;
            case Tok::KW_STRUCT: advance(); t = parseStructSpecifier(); sawAny = true; break;
            case Tok::KW_ENUM:   advance(); parseEnumSpecifier(); t = TypeSpec{}; sawAny = true; break;
            case Tok::IDENT:
                // A typedef name is a type only where a type is expected; the
                // `!sawAny` guard keeps `unsigned x` from treating x as one.
                if (!sawAny && typedefs_.count(peek().lexeme)) {
                    t = typedefs_[peek().lexeme]; sawAny = true; advance();
                }
                break;
            default: break;
        }
        break;
    }
    if (!sawAny) errorHere("expected a type (int, char, float, void, struct or a typedef name)");
    return t;
}

// struct [Name] [ { fields } ]. A bare `struct Name` is a reference (it may
// precede the definition, which is what lets a node hold a pointer to its own
// type); a body defines the layout. Field offsets are assigned here, once, so
// every later stage just reads them.
TypeSpec Parser::parseStructSpecifier() {
    std::string name;
    const Token& at = peek();
    if (check(Tok::IDENT)) name = advance().lexeme;
    int id = name.empty() ? -1 : structs_.find(name);
    if (check(Tok::LBRACE)) {
        if (id >= 0 && structs_.structs[id].complete)
            sink_.add(at.line, at.col, "redefinition of 'struct " + name + "'");
        if (id < 0) id = structs_.declare(name);
        advance();   // '{'
        std::vector<FieldInfo> fields;
        int slotOff = 0, byteOff = 0, maxAlign = 1;
        while (!check(Tok::RBRACE) && !isAtEnd()) {
            TypeSpec ft = parseTypeSpec();
            do {
                int stars = parseStars();
                const Token& fn = expect(Tok::IDENT, "a field name");
                TypeSpec fty = ft;
                fty.ptrDepth += stars;
                parseArrayDims(fty, false);
                if (fty.base == BaseType::STRUCT && fty.ptrDepth == 0
                    && !structs_.structs[fty.structId].complete)
                    sink_.add(fn.line, fn.col, "field '" + fn.lexeme + "' has incomplete type 'struct "
                              + structs_.structs[fty.structId].name + "'");
                if (fty.base == BaseType::VOID && fty.ptrDepth == 0)
                    sink_.add(fn.line, fn.col, "field '" + fn.lexeme + "' declared void");
                int al = structs_.elemAlign(fty);
                byteOff = (byteOff + al - 1) / al * al;
                FieldInfo f;
                f.name = fn.lexeme; f.type = fty; f.slotOffset = slotOff; f.byteOffset = byteOff;
                for (const auto& other : fields)
                    if (other.name == f.name)
                        sink_.add(fn.line, fn.col, "duplicate field '" + f.name + "'");
                fields.push_back(f);
                slotOff += structs_.slotsOf(fty);
                byteOff += structs_.bytesOf(fty);
                if (al > maxAlign) maxAlign = al;
            } while (match(Tok::COMMA));
            expect(Tok::SEMI, "';'");
        }
        expect(Tok::RBRACE, "'}'");
        StructInfo& si = structs_.structs[id];
        si.fields = std::move(fields);
        si.slots  = slotOff;
        si.align  = maxAlign;
        si.bytes  = (byteOff + maxAlign - 1) / maxAlign * maxAlign;
        si.complete = true;
    } else {
        if (name.empty()) errorHere("expected a struct name or '{' after 'struct'");
        if (id < 0) id = structs_.declare(name);
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
    long long next = 0;
    while (!check(Tok::RBRACE) && !isAtEnd()) {
        const Token& nm = expect(Tok::IDENT, "an enumerator name");
        if (match(Tok::OP_ASSIGN)) next = parseConstInt("an enumerator value");
        enumConsts_[nm.lexeme] = next++;
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
        typedefs_[nm.lexeme] = t;
    } while (match(Tok::COMMA));
    expect(Tok::SEMI, "';'");
}

// Folds an expression made only of literals and operators. Array sizes, case
// labels and enumerator values must be known when the program is compiled.
bool Parser::evalConst(const Expr& e, long long& out) const {
    switch (e.kind) {
        case NodeKind::IntLit:  out = static_cast<const IntLit&>(e).value; return true;
        case NodeKind::CharLit: out = static_cast<unsigned char>(static_cast<const CharLit&>(e).value); return true;
        case NodeKind::SizeofExpr: {
            const auto& s = static_cast<const SizeofExpr&>(e);
            if (!s.ofType) return false;
            out = s.value; return true;
        }
        case NodeKind::CastExpr: return evalConst(*static_cast<const CastExpr&>(e).operand, out);
        case NodeKind::UnaryExpr: {
            const auto& u = static_cast<const UnaryExpr&>(e);
            long long v;
            if (!evalConst(*u.operand, v)) return false;
            switch (u.op) {
                case UnOp::NEG: out = -v; return true;
                case UnOp::POS: out = v;  return true;
                case UnOp::NOT: out = !v; return true;
                case UnOp::BITNOT: out = ~v; return true;
                default: return false;
            }
        }
        case NodeKind::CondExpr: {
            const auto& q = static_cast<const CondExpr&>(e);
            long long c, a, b;
            if (!evalConst(*q.cond, c) || !evalConst(*q.thenExpr, a) || !evalConst(*q.elseExpr, b)) return false;
            out = c ? a : b; return true;
        }
        case NodeKind::BinaryExpr: {
            const auto& b = static_cast<const BinaryExpr&>(e);
            long long l, r;
            if (!evalConst(*b.lhs, l) || !evalConst(*b.rhs, r)) return false;
            switch (b.op) {
                case BinOp::ADD: out = l + r; return true;
                case BinOp::SUB: out = l - r; return true;
                case BinOp::MUL: out = l * r; return true;
                case BinOp::DIV: if (r == 0) return false; out = l / r; return true;
                case BinOp::MOD: if (r == 0) return false; out = l % r; return true;
                case BinOp::LT: out = l <  r; return true;
                case BinOp::LE: out = l <= r; return true;
                case BinOp::GT: out = l >  r; return true;
                case BinOp::GE: out = l >= r; return true;
                case BinOp::EQ: out = l == r; return true;
                case BinOp::NE: out = l != r; return true;
                case BinOp::LOGAND: out = l && r; return true;
                case BinOp::LOGOR:  out = l || r; return true;
                case BinOp::BITAND: out = l & r; return true;
                case BinOp::BITOR:  out = l | r; return true;
                case BinOp::BITXOR: out = l ^ r; return true;
                case BinOp::SHL: if (r < 0 || r >= 63) return false; out = l << r; return true;
                case BinOp::SHR: if (r < 0 || r >= 63) return false; out = l >> r; return true;
            }
            return false;
        }
        default: return false;
    }
}

long long Parser::parseConstInt(const char* what) {
    const Token& at = peek();
    auto e = parseTernary();
    long long v = 0;
    if (!evalConst(*e, v)) {
        sink_.add(at.line, at.col, std::string(what) + " must be a constant integer expression");
        return 1;
    }
    return v;
}

void Parser::parseArrayDims(TypeSpec& t, bool allowEmptyFirst) {
    bool first = true;
    while (check(Tok::LBRACKET)) {
        const Token& lb = advance();
        long long n = -1;
        if (check(Tok::RBRACKET)) {
            if (!(first && allowEmptyFirst))
                sink_.add(lb.line, lb.col, "only the first array dimension may be left empty");
        } else {
            n = parseConstInt("an array size");
            if (n <= 0) { sink_.add(lb.line, lb.col, "array size must be positive"); n = 1; }
        }
        expect(Tok::RBRACKET, "']'");
        if (first) { t.isArray = true; t.arraySize = static_cast<int>(n); }
        else        t.extraDims.push_back(static_cast<int>(n));
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

void Parser::zeroFill(const TypeSpec& t, std::vector<std::unique_ptr<Expr>>& out, int line, int col) {
    if (t.isArray) {
        TypeSpec e = pointeeOf(t);
        int n = t.arraySize < 0 ? 1 : t.arraySize;
        for (int i = 0; i < n; ++i) zeroFill(e, out, line, col);
    } else if (isStructValue(t)) {
        for (const auto& f : structs_.structs[t.structId].fields) zeroFill(f.type, out, line, col);
    } else {
        std::unique_ptr<Expr> z;
        if (t.base == BaseType::FLOAT && t.ptrDepth == 0) z = std::make_unique<FloatLit>(0.0);
        else                                               z = std::make_unique<IntLit>(0);
        z->line = line; z->col = col;
        out.push_back(std::move(z));
    }
}

void Parser::flattenOne(const TypeSpec& t, std::vector<InitNode>& kids, size_t& cur,
                        std::vector<std::unique_ptr<Expr>>& out, int line, int col) {
    bool agg = t.isArray || isStructValue(t);
    if (cur >= kids.size()) { zeroFill(t, out, line, col); return; }
    InitNode& k = kids[cur];
    if (!agg) {
        ++cur;
        if (k.isList) {                       // `{5}` where a scalar is expected
            if (k.kids.empty() || k.kids[0].isList) { zeroFill(t, out, k.line, k.col); }
            else out.push_back(std::move(k.kids[0].expr));
        } else {
            out.push_back(std::move(k.expr));
        }
        return;
    }
    if (k.isList) {
        ++cur;
        size_t c2 = 0;
        flattenAgg(t, k.kids, c2, out, k.line, k.col);
        if (c2 < k.kids.size()) sink_.add(k.line, k.col, "excess elements in initializer");
        return;
    }
    // A string literal fills a char array.
    if (t.isArray && t.base == BaseType::CHAR && t.ptrDepth == 0 && t.extraDims.empty()
        && k.expr && k.expr->kind == NodeKind::StringLit) {
        const std::string& s = static_cast<StringLit&>(*k.expr).value;
        int n = t.arraySize < 0 ? static_cast<int>(s.size()) + 1 : t.arraySize;
        for (int i = 0; i < n; ++i) {
            auto ch = std::make_unique<CharLit>(i < static_cast<int>(s.size()) ? s[i] : '\0');
            ch->line = k.line; ch->col = k.col;
            out.push_back(std::move(ch));
        }
        ++cur;
        return;
    }
    if (isStructValue(t)) {
        sink_.add(k.line, k.col, "a struct inside an initializer must be written with braces");
        ++cur;
        zeroFill(t, out, k.line, k.col);
        return;
    }
    // Braces elided: `int m[2][2] = {1, 2, 3, 4}` -- keep consuming scalars.
    flattenAgg(t, kids, cur, out, line, col);
}

void Parser::flattenAgg(const TypeSpec& t, std::vector<InitNode>& kids, size_t& cur,
                        std::vector<std::unique_ptr<Expr>>& out, int line, int col) {
    if (t.isArray) {
        TypeSpec e = pointeeOf(t);
        int n = t.arraySize < 0 ? 1 : t.arraySize;
        for (int i = 0; i < n; ++i) flattenOne(e, kids, cur, out, line, col);
    } else {
        for (const auto& f : structs_.structs[t.structId].fields)
            flattenOne(f.type, kids, cur, out, line, col);
    }
}

void Parser::flattenInit(const TypeSpec& t, InitNode& init, std::vector<std::unique_ptr<Expr>>& out) {
    size_t cur = 0;
    flattenAgg(t, init.kids, cur, out, init.line, init.col);
    if (cur < init.kids.size()) sink_.add(init.line, init.col, "excess elements in initializer");
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
            return typedefs_.count(peek(off).lexeme) > 0;
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
    prog->structs = std::move(structs_);
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
    auto decl = std::make_unique<VarDecl>(type, nameTok.lexeme);
    decl->line = nameTok.line;
    decl->col  = nameTok.col;

    // `int a[] = {...}` leaves the first length empty (-1) until the
    // initializer is known.
    parseArrayDims(decl->type, true);

    // An array of structs, or any multi-dimensional array, is initialised
    // through the flattening path; a plain 1-D array of scalars keeps the
    // simple one (a short list is zero-padded later, by IRGen).
    bool aggregate = isStructValue(decl->type)
        || (decl->type.isArray && (!decl->type.extraDims.empty()
                                   || (decl->type.base == BaseType::STRUCT && decl->type.ptrDepth == 0)));

    if (match(Tok::OP_ASSIGN)) {
        if (check(Tok::LBRACE)) {
            InitNode root = parseInitNode();
            if (aggregate) {
                if (decl->type.isArray && decl->type.arraySize < 0) {
                    // Length from the number of top-level elements (rounded up
                    // when the braces were elided).
                    TypeSpec e = pointeeOf(decl->type);
                    size_t perElem = 1;
                    bool allLists = !root.kids.empty();
                    for (auto& k : root.kids) if (!k.isList) allLists = false;
                    if (!allLists) perElem = static_cast<size_t>(std::max(1, structs_.slotsOf(e)));
                    decl->type.arraySize = static_cast<int>(
                        allLists ? root.kids.size() : (root.kids.size() + perElem - 1) / perElem);
                    if (decl->type.arraySize <= 0) decl->type.arraySize = 1;
                }
                flattenInit(decl->type, root, decl->initList);
            } else {
                for (auto& k : root.kids) {
                    if (k.isList) {
                        sink_.add(k.line, k.col, "unexpected '{' in the initializer of a scalar or one-dimensional array");
                        continue;
                    }
                    decl->initList.push_back(std::move(k.expr));
                }
            }
        } else if (decl->type.isArray && check(Tok::STRING_LIT)) {
            // `char s[] = "abc";` is shorthand for { 'a', 'b', 'c', 0 }. It is
            // expanded here into exactly that list, so sema, IRGen and the
            // optimizer see an ordinary array initializer and need no notion
            // of a string at all.
            const Token& str = advance();
            for (char ch : str.lexeme) {
                auto cl = std::make_unique<CharLit>(ch);
                cl->line = str.line; cl->col = str.col;
                decl->initList.push_back(std::move(cl));
            }
            // C drops the terminator when the literal exactly fills the array.
            bool exactFit = decl->type.arraySize >= 0
                         && static_cast<int>(str.lexeme.size()) == decl->type.arraySize;
            if (!exactFit) {
                auto nul = std::make_unique<CharLit>('\0');
                nul->line = str.line; nul->col = str.col;
                decl->initList.push_back(std::move(nul));
            }
        } else {
            decl->init = parseInitExpr();
        }
    }

    // Resolve an inferred length. Without an initializer there is nothing to
    // infer it from; that is reported here, with the array's own position,
    // rather than left to surface later as a confusing zero-sized allocation.
    if (decl->type.isArray && decl->type.arraySize < 0) {
        if (!decl->initList.empty()) {
            decl->type.arraySize = static_cast<int>(decl->initList.size());
        } else {
            sink_.add(nameTok.line, nameTok.col,
                      "array '" + nameTok.lexeme + "' has no size and no initializer to infer one from");
            decl->type.arraySize = 1;       // recovery value; the error above already fails the build
        }
    }
    return decl;
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
            synchronize();
        }
    }
    expect(Tok::RBRACE, "'}'");
    return cs;
}

bool Parser::parseLocalDeclaration(std::vector<std::unique_ptr<Stmt>>& out) {
    TypeSpec type = parseTypeSpec();
    if (match(Tok::SEMI)) return false;     // a bare `struct S { ... };` inside a function
    int stars = parseStars();
    const Token& nameTok = expect(Tok::IDENT, "an identifier");
    for (auto& d : parseVarDeclDeclarators(type, stars, nameTok)) {
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
        if (!check(Tok::SEMI)) {
            if (!check(Tok::SEMI)) cond = check(Tok::SEMI) ? nullptr : parseExpression();
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
            synchronize();
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
            sz->value = structs_.bytesOf(sz->type);
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
            auto ec = enumConsts_.find(t.lexeme);
            if (ec != enumConsts_.end()) {
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
