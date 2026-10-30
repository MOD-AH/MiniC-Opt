// Cppify — declaration semantics shared by both parsers (see include/parse_context.h)
//
// Moved out of the hand-written parser unchanged in behaviour; the comments
// that explain each rule moved with it.
#include "parse_context.h"
#include <algorithm>

namespace minic {

ParseContext::ParseContext(DiagnosticSink& sink) : sink_(sink) {
    // The names <stdbool.h>, <stddef.h> and <stdint.h> would have supplied.
    // Every one of them is an integer here.
    for (const char* n : {"bool", "size_t", "ssize_t", "int8_t", "int16_t", "int32_t", "int64_t",
                          "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t",
                          "ptrdiff_t", "time_t", "clock_t", "FILE"})
        typedefs[n] = TypeSpec{};
}

TypeSpec ParseContext::finishType(const TypeWords& w) const {
    TypeSpec t = w.t;
    if (w.sawIntWord && t.base == BaseType::INT) {
        t.intBits = w.sawShort ? 16 : w.sawLong ? 64 : 32;   // LP64, as on macOS: long is 64 bits
        t.isUnsigned = w.sawUnsigned;
    } else if (t.base == BaseType::CHAR) {
        t.isUnsigned = w.sawUnsigned;                         // plain char is signed on Apple platforms
    }
    return t;
}

// ---- structs ----------------------------------------------------------------
//
// struct [Name] [ { fields } ]. A bare `struct Name` is a reference (it may
// precede the definition, which is what lets a node hold a pointer to its own
// type); a body defines the layout. Field offsets are assigned here, once, so
// every later stage just reads them.

int ParseContext::structReference(const std::string& name, int, int) {
    int id = structs.find(name);
    return id >= 0 ? id : structs.declare(name);
}

int ParseContext::beginStruct(const std::string& name, int line, int col) {
    int id = name.empty() ? -1 : structs.find(name);
    if (id >= 0 && structs.structs[id].complete)
        sink_.add(line, col, "redefinition of 'struct " + name + "'");
    if (id < 0) id = structs.declare(name);
    open_.push_back(OpenStruct{id, {}});
    return id;
}

void ParseContext::addField(TypeSpec fty, const Token& fn) {
    OpenStruct& s = open_.back();
    if (fty.base == BaseType::STRUCT && fty.ptrDepth == 0 && !structs.structs[fty.structId].complete)
        sink_.add(fn.line, fn.col, "field '" + fn.lexeme + "' has incomplete type 'struct " +
                  structs.structs[fty.structId].name + "'");
    if (fty.base == BaseType::VOID && fty.ptrDepth == 0)
        sink_.add(fn.line, fn.col, "field '" + fn.lexeme + "' declared void");
    int al = structs.elemAlign(fty);
    s.byteOff = (s.byteOff + al - 1) / al * al;
    FieldInfo f;
    f.name = fn.lexeme; f.type = fty; f.slotOffset = s.slotOff; f.byteOffset = s.byteOff;
    for (const auto& other : s.fields)
        if (other.name == f.name) sink_.add(fn.line, fn.col, "duplicate field '" + f.name + "'");
    s.fields.push_back(f);
    s.slotOff += structs.slotsOf(fty);
    s.byteOff += structs.bytesOf(fty);
    if (al > s.maxAlign) s.maxAlign = al;
}

void ParseContext::finishStruct() {
    OpenStruct s = std::move(open_.back());
    open_.pop_back();
    StructInfo& si = structs.structs[s.id];
    si.fields = std::move(s.fields);
    si.slots  = s.slotOff;
    si.align  = s.maxAlign;
    si.bytes  = (s.byteOff + s.maxAlign - 1) / s.maxAlign * s.maxAlign;
    si.complete = true;
}

// enum [Name] [ { A, B = 5, C } ]. The constants become plain integers at the
// point of use (see the parsers' primary expressions); the enum type is int.
void ParseContext::addEnumerator(const std::string& name, bool hasValue, long long value) {
    if (hasValue) nextEnum_ = value;
    enumConsts[name] = nextEnum_++;
}

// ---- constants ------------------------------------------------------------------

// Folds an expression made only of literals and operators. Array sizes, case
// labels and enumerator values must be known when the program is compiled.
bool ParseContext::evalConst(const Expr& e, long long& out) const {
    switch (e.kind) {
        case NodeKind::IntLit:  out = static_cast<const IntLit&>(e).value; return true;
        case NodeKind::CharLit: out = static_cast<signed char>(static_cast<const CharLit&>(e).value); return true;
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

long long ParseContext::constInt(const Expr& e, int line, int col, const char* what) {
    long long v = 0;
    if (!evalConst(e, v)) {
        sink_.add(line, col, std::string(what) + " must be a constant integer expression");
        return 1;
    }
    return v;
}

void ParseContext::applyDims(TypeSpec& t, std::vector<Dim>& dims, bool allowEmptyFirst) {
    for (size_t i = 0; i < dims.size(); ++i) applyDim(t, dims[i], i == 0, allowEmptyFirst);
}

void ParseContext::applyDim(TypeSpec& t, Dim& d, bool first, bool allowEmptyFirst) {
    long long n = -1;
    if (!d.size) {
        if (!(first && allowEmptyFirst))
            sink_.add(d.line, d.col, "only the first array dimension may be left empty");
    } else {
        n = constInt(*d.size, d.exprLine, d.exprCol, "an array size");
        if (n <= 0) { sink_.add(d.line, d.col, "array size must be positive"); n = 1; }
    }
    if (first) { t.isArray = true; t.arraySize = static_cast<int>(n); }
    else        t.extraDims.push_back(static_cast<int>(n));
}

// ---- brace initializers ---------------------------------------------------------
//
// `struct P a[2] = {{1, 2}, {3, 4}};` and `int m[2][3] = {{1,2,3},{4,5,6}}`
// are parsed into a tree and then flattened against the declared type into
// exactly one expression per interpreter slot, padding with zeros. Sema, IRGen
// and the optimizer therefore never see a nested initializer.

void ParseContext::zeroFill(const TypeSpec& t, std::vector<std::unique_ptr<Expr>>& out, int line, int col) {
    if (t.isArray) {
        TypeSpec e = pointeeOf(t);
        int n = t.arraySize < 0 ? 1 : t.arraySize;
        for (int i = 0; i < n; ++i) zeroFill(e, out, line, col);
    } else if (isStructValue(t)) {
        for (const auto& f : structs.structs[t.structId].fields) zeroFill(f.type, out, line, col);
    } else {
        std::unique_ptr<Expr> z;
        if (t.base == BaseType::FLOAT && t.ptrDepth == 0) z = std::make_unique<FloatLit>(0.0);
        else                                               z = std::make_unique<IntLit>(0);
        z->line = line; z->col = col;
        out.push_back(std::move(z));
    }
}

void ParseContext::flattenOne(const TypeSpec& t, std::vector<InitNode>& kids, size_t& cur,
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

void ParseContext::flattenAgg(const TypeSpec& t, std::vector<InitNode>& kids, size_t& cur,
                              std::vector<std::unique_ptr<Expr>>& out, int line, int col) {
    if (t.isArray) {
        TypeSpec e = pointeeOf(t);
        int n = t.arraySize < 0 ? 1 : t.arraySize;
        for (int i = 0; i < n; ++i) flattenOne(e, kids, cur, out, line, col);
    } else {
        for (const auto& f : structs.structs[t.structId].fields)
            flattenOne(f.type, kids, cur, out, line, col);
    }
}

void ParseContext::flattenInit(const TypeSpec& t, InitNode& init, std::vector<std::unique_ptr<Expr>>& out) {
    size_t cur = 0;
    flattenAgg(t, init.kids, cur, out, init.line, init.col);
    if (cur < init.kids.size()) sink_.add(init.line, init.col, "excess elements in initializer");
}

// ---- declarators ------------------------------------------------------------------

std::unique_ptr<VarDecl> ParseContext::declarator(TypeSpec type, const Token& nameTok, Initializer init) {
    auto decl = std::make_unique<VarDecl>(type, nameTok.lexeme);
    decl->line = nameTok.line;
    decl->col  = nameTok.col;
    // `int a[] = {...}` left the first length empty (-1) until the
    // initializer is known.

    // An array of structs, or any multi-dimensional array, is initialised
    // through the flattening path; a plain 1-D array of scalars keeps the
    // simple one (a short list is zero-padded later, by IRGen).
    bool aggregate = isStructValue(decl->type)
        || (decl->type.isArray && (!decl->type.extraDims.empty()
                                   || (decl->type.base == BaseType::STRUCT && decl->type.ptrDepth == 0)));

    if (init.kind == Initializer::LIST) {
        InitNode& root = init.list;
        if (aggregate) {
            if (decl->type.isArray && decl->type.arraySize < 0) {
                // Length from the number of top-level elements (rounded up
                // when the braces were elided).
                TypeSpec e = pointeeOf(decl->type);
                size_t perElem = 1;
                bool allLists = !root.kids.empty();
                for (auto& k : root.kids) if (!k.isList) allLists = false;
                if (!allLists) perElem = static_cast<size_t>(std::max(1, structs.slotsOf(e)));
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
    } else if (init.kind == Initializer::STRING) {
        // `char s[] = "abc";` is shorthand for { 'a', 'b', 'c', 0 }. It is
        // expanded here into exactly that list, so sema, IRGen and the
        // optimizer see an ordinary array initializer and need no notion
        // of a string at all.
        const Token& str = init.str;
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
    } else if (init.kind == Initializer::EXPR) {
        decl->init = std::move(init.expr);
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

Param ParseContext::parameter(TypeSpec type, const Token& name, std::vector<Dim>& dims) {
    Param p;
    p.type = type;
    p.name = name.lexeme;
    p.line = name.line;
    p.col  = name.col;
    if (!dims.empty()) {
        // The first length is ignored but the inner ones are kept so m[i][j]
        // knows the row width.
        applyDims(p.type, dims, true);
        p.type.arraySize = -1;
    }
    return p;
}

}  // namespace minic
