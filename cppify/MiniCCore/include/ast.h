// MiniC-Opt — abstract syntax tree
// Owner: Member 1 (Front End)
//
// Design: a small polymorphic class hierarchy (Expr / Stmt / Decl, all
// deriving from Node) walked with a classic double-dispatch Visitor. Chosen
// over std::variant so that semantic analysis (Step 3) and IR generation
// (Step 4) can each be written as one self-contained Visitor implementation
// over the same tree, instead of every pass growing its own switch over a
// tag. Node shapes follow docs/grammar.ebnf directly — see that file for
// the authoritative grammar each node corresponds to.
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace minic {

// ---- shared types ----------------------------------------------------

enum class BaseType { INT, CHAR, FLOAT, VOID, STRUCT };

struct TypeSpec {
    // A constructor (rather than aggregate init) so the many existing
    // `TypeSpec{base, false, -1}` call sites stay valid as fields are added.
    TypeSpec() = default;
    TypeSpec(BaseType b, bool arr, int size, int ptr = 0, int sid = -1)
        : base(b), isArray(arr), arraySize(size), ptrDepth(ptr), structId(sid) {}
    BaseType base      = BaseType::INT;
    bool     isArray   = false;
    int      arraySize = -1;   // -1: no size given (e.g. a `T x[]` parameter)
    // Number of '*' on the (element) type: `int **pp` has ptrDepth 2. For an
    // array this describes the ELEMENT, so `int *a[4]` is isArray with
    // ptrDepth 1. Declared last so every existing positional initialiser
    // `TypeSpec{base, false, -1}` still means "no pointers".
    int      ptrDepth  = 0;
    // Index into StructTable::structs when base == STRUCT, else -1.
    int      structId  = -1;
    // Inner dimensions of a multi-dimensional array: `int a[3][4]` has
    // arraySize 3 and extraDims {4}. The whole thing is one flat run of slots;
    // a[i] is the address of row i, not a load. A pointer to a row, which is
    // what such an array decays to, keeps the same extraDims. (An array of
    // pointers with extra dimensions is not supported.)
    std::vector<int> extraDims;
    // Integer width and signedness. intBits applies to base INT (short 16,
    // int 32, long and long long 64); a CHAR is always 8 bits. isUnsigned
    // applies to both. Pointers, floats and structs ignore them.
    int      intBits    = 32;
    bool     isUnsigned = false;
    // base FLOAT only: `double` (8 bytes) rather than `float` (4). Both are a
    // C double at run time; this decides sizeof and struct layout.
    bool     isDouble   = false;
};

// A pointer here is an array reference plus an offset, so an array and a
// pointer to its element are interchangeable once an array is used as a value.
inline bool isPointerType(const TypeSpec& t) { return !t.isArray && t.ptrDepth > 0; }
inline bool isPointerLike(const TypeSpec& t)  { return t.isArray || t.ptrDepth > 0; }
// `int a[5]` used as a value is `int *`.
inline TypeSpec decayed(TypeSpec t) {
    if (t.isArray) { t.isArray = false; t.arraySize = -1; ++t.ptrDepth; }
    return t;
}
inline TypeSpec pointeeOf(TypeSpec t) {
    t = decayed(t);
    if (t.ptrDepth > 0) --t.ptrDepth;
    if (!t.extraDims.empty()) {            // pointer to a row: the pointee is the row array
        t.isArray   = true;
        t.arraySize = t.extraDims.front();
        t.extraDims.erase(t.extraDims.begin());
    }
    return t;
}
inline TypeSpec addressOfType(TypeSpec t) { t.isArray = false; t.arraySize = -1; ++t.ptrDepth; return t; }
inline bool isStructValue(const TypeSpec& t) { return t.base == BaseType::STRUCT && t.ptrDepth == 0 && !t.isArray; }
inline bool isIntegerType(const TypeSpec& t) {
    return !isPointerLike(t) && (t.base == BaseType::INT || t.base == BaseType::CHAR);
}
inline int intWidth(const TypeSpec& t) { return t.base == BaseType::CHAR ? 8 : t.intBits; }
inline TypeSpec intType(int bits, bool uns) {
    TypeSpec t{BaseType::INT, false, -1};
    t.intBits = bits; t.isUnsigned = uns;
    return t;
}
// Two integer types are the same C type when width and signedness match;
// char and an 8-bit int never arise separately, so base need not be compared.
inline bool sameIntType(const TypeSpec& a, const TypeSpec& b) {
    return intWidth(a) == intWidth(b) && a.isUnsigned == b.isUnsigned;
}

// ---- struct layout ---------------------------------------------------------
//
// Two sizes are tracked for every type. SLOTS are what the interpreter really
// stores: every scalar and pointer is one slot, so a struct is a flat run of
// slots and `s.f` is a load at a fixed slot offset. BYTES are what a C
// programmer sees through sizeof (char 1, int 4, pointer 8, with the usual
// alignment padding). Keeping both lets `sizeof(struct S)` print the number a
// real compiler would while the machine underneath stays simple.
struct FieldInfo {
    std::string name;
    TypeSpec    type;
    int         slotOffset = 0;
    int         byteOffset = 0;
};

struct StructInfo {
    std::string             name;
    std::vector<FieldInfo>  fields;
    int  slots    = 0;
    int  bytes    = 0;
    int  align    = 1;
    bool complete = false;
};

struct StructTable {
    std::vector<StructInfo> structs;

    int find(const std::string& name) const {
        for (size_t i = 0; i < structs.size(); ++i)
            if (!name.empty() && structs[i].name == name) return static_cast<int>(i);
        return -1;
    }
    int declare(const std::string& name) {
        structs.push_back(StructInfo{});
        structs.back().name = name;
        return static_cast<int>(structs.size()) - 1;
    }
    const FieldInfo* field(int id, const std::string& name) const {
        if (id < 0 || id >= static_cast<int>(structs.size())) return nullptr;
        for (const auto& f : structs[id].fields) if (f.name == name) return &f;
        return nullptr;
    }
    // Slots one VALUE of type t occupies (a whole array counts all elements).
    int slotsOf(const TypeSpec& t) const {
        long long one = 1;
        if (t.ptrDepth == 0 && t.base == BaseType::STRUCT && t.structId >= 0) one = structs[t.structId].slots;
        if (!t.isArray) return static_cast<int>(one);
        long long n = t.arraySize < 0 ? 1 : t.arraySize;
        for (int d : t.extraDims) n *= (d < 0 ? 1 : d);
        return static_cast<int>(n * one);
    }
    int elemBytes(const TypeSpec& t) const {
        if (t.ptrDepth > 0) return 8;
        switch (t.base) {
            case BaseType::CHAR:   return 1;
            case BaseType::STRUCT: return t.structId >= 0 ? structs[t.structId].bytes : 0;
            case BaseType::VOID:   return 1;
            case BaseType::INT:    return t.intBits / 8;
            case BaseType::FLOAT:  return t.isDouble ? 8 : 4;
            default:               return 4;
        }
    }
    int elemAlign(const TypeSpec& t) const {
        if (t.ptrDepth > 0) return 8;
        switch (t.base) {
            case BaseType::CHAR:   return 1;
            case BaseType::STRUCT: return t.structId >= 0 ? structs[t.structId].align : 1;
            case BaseType::VOID:   return 1;
            case BaseType::INT:    return t.intBits / 8;
            case BaseType::FLOAT:  return t.isDouble ? 8 : 4;
            default:               return 4;
        }
    }
    int bytesOf(const TypeSpec& t) const {
        long long one = elemBytes(t);
        if (!t.isArray) return static_cast<int>(one);
        long long n = t.arraySize < 0 ? 1 : t.arraySize;
        for (int d : t.extraDims) n *= (d < 0 ? 1 : d);
        return static_cast<int>(n * one);
    }
};

enum class BinOp  { ADD, SUB, MUL, DIV, MOD, LT, LE, GT, GE, EQ, NE, LOGAND, LOGOR,
                    BITAND, BITOR, BITXOR, SHL, SHR };
enum class UnOp   { NEG, POS, NOT, BITNOT, ADDR, DEREF, PRE_INC, PRE_DEC, POST_INC, POST_DEC };
enum class AssignOp { ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN, PERCENT_ASSIGN,
                      AND_ASSIGN, OR_ASSIGN, XOR_ASSIGN, SHL_ASSIGN, SHR_ASSIGN };

const char* toString(BinOp op);
const char* toString(UnOp op);
const char* toString(AssignOp op);
const char* toString(BaseType t);

// ---- forward declarations of every concrete node ---------------------

struct IntLit; struct FloatLit; struct CharLit; struct StringLit; struct IdentExpr;
struct IndexExpr; struct CallExpr; struct UnaryExpr; struct BinaryExpr; struct AssignExpr; struct ConvertExpr;
struct CastExpr; struct MemberExpr; struct SizeofExpr; struct CondExpr; struct CommaExpr;
struct SwitchStmt; struct GotoStmt; struct LabelStmt;

struct ExprStmt; struct DeclStmt; struct CompoundStmt; struct IfStmt; struct WhileStmt;
struct ForStmt; struct DoStmt; struct ReturnStmt; struct BreakStmt; struct ContinueStmt;

struct VarDecl; struct FuncDecl; struct Program;

// ---- visitor -----------------------------------------------------------
// One visit() overload per concrete node. A pass implements this once and
// gets a compile error if a node kind is ever added and left unhandled.

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(IntLit&)     = 0;
    virtual void visit(FloatLit&)   = 0;
    virtual void visit(CharLit&)    = 0;
    virtual void visit(StringLit&)  = 0;
    virtual void visit(IdentExpr&)  = 0;
    virtual void visit(IndexExpr&)  = 0;
    virtual void visit(CallExpr&)   = 0;
    virtual void visit(UnaryExpr&)  = 0;
    virtual void visit(BinaryExpr&) = 0;
    virtual void visit(AssignExpr&)  = 0;
    virtual void visit(ConvertExpr&) = 0;
    virtual void visit(CastExpr&)    = 0;
    virtual void visit(MemberExpr&)  = 0;
    virtual void visit(SizeofExpr&)  = 0;
    virtual void visit(CondExpr&)    = 0;
    virtual void visit(CommaExpr&)   = 0;

    virtual void visit(ExprStmt&)     = 0;
    virtual void visit(DeclStmt&)     = 0;
    virtual void visit(CompoundStmt&) = 0;
    virtual void visit(IfStmt&)       = 0;
    virtual void visit(WhileStmt&)    = 0;
    virtual void visit(ForStmt&)      = 0;
    virtual void visit(DoStmt&)       = 0;
    virtual void visit(ReturnStmt&)   = 0;
    virtual void visit(BreakStmt&)    = 0;
    virtual void visit(ContinueStmt&) = 0;
    virtual void visit(SwitchStmt&)   = 0;
    virtual void visit(GotoStmt&)     = 0;
    virtual void visit(LabelStmt&)    = 0;

    virtual void visit(VarDecl&)  = 0;
    virtual void visit(FuncDecl&) = 0;
    virtual void visit(Program&)  = 0;
};

// ---- node base -----------------------------------------------------------

enum class NodeKind {
    IntLit, FloatLit, CharLit, StringLit, IdentExpr, IndexExpr, CallExpr, UnaryExpr, BinaryExpr, AssignExpr, ConvertExpr, CastExpr,
    MemberExpr, SizeofExpr, CondExpr, CommaExpr,
    SwitchStmt, GotoStmt, LabelStmt,
    ExprStmt, DeclStmt, CompoundStmt, IfStmt, WhileStmt, ForStmt, DoStmt, ReturnStmt, BreakStmt, ContinueStmt,
    VarDecl, FuncDecl, Program
};

struct Node {
    NodeKind kind;
    int line = 0, col = 0;
    explicit Node(NodeKind k) : kind(k) {}
    virtual ~Node() = default;
    virtual void accept(Visitor& v) = 0;
};

struct Expr : Node {
    using Node::Node;
    // Filled in by Sema (Step 3). Meaningless before analyze() runs.
    TypeSpec resolvedType;
};
struct Stmt : Node { using Node::Node; };
struct Decl : Node { using Node::Node; };

// ---- expressions -----------------------------------------------------

struct IntLit : Expr {
    long long value;
    // From a u/U and l/L/ll/LL suffix; Sema picks the literal's C type from them.
    bool unsignedSuffix = false;
    bool longSuffix     = false;
    explicit IntLit(long long v) : Expr(NodeKind::IntLit), value(v) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct FloatLit : Expr {
    double value;
    explicit FloatLit(double v) : Expr(NodeKind::FloatLit), value(v) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct CharLit : Expr {
    char value;
    explicit CharLit(char v) : Expr(NodeKind::CharLit), value(v) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// A string literal. Deliberately NOT a general value type: there is no
// string type in MiniC, no assignment, no indexing, no concatenation. It
// exists so printf has a format to work from, and sema rejects it anywhere
// else. Adding a real string type would ripple through the type system, the
// IR operand model and all nine optimization passes.
struct StringLit : Expr {
    // Set by Sema when this literal is the format argument of printf/scanf and
    // friends. IRGen keeps such a literal as a constant operand; any other use
    // of a string literal becomes a real char array (see IRGen::visit(StringLit&)).
    bool isFormat = false;
    std::string value;        // already decoded by the lexer
    explicit StringLit(std::string v) : Expr(NodeKind::StringLit), value(std::move(v)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// A bare identifier used as an rvalue or as the base of an lvalue.
struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(std::string n) : Expr(NodeKind::IdentExpr), name(std::move(n)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// base[index] — an array element, usable as an lvalue.
struct IndexExpr : Expr {
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> b, std::unique_ptr<Expr> i)
        : Expr(NodeKind::IndexExpr), base(std::move(b)), index(std::move(i)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> args;
    explicit CallExpr(std::string c) : Expr(NodeKind::CallExpr), callee(std::move(c)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// Prefix -, !, ++, -- and postfix ++, --. POS ("+x") is kept as an explicit
// node rather than folded away in the parser, so it stays visible in
// --dump-ast even though every pass is free to treat it as identity.
struct UnaryExpr : Expr {
    UnOp op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(UnOp o, std::unique_ptr<Expr> e) : Expr(NodeKind::UnaryExpr), op(o), operand(std::move(e)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct BinaryExpr : Expr {
    BinOp op;
    std::unique_ptr<Expr> lhs, rhs;
    BinaryExpr(BinOp o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : Expr(NodeKind::BinaryExpr), op(o), lhs(std::move(l)), rhs(std::move(r)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// target (=, +=, -=, *=, /=) value.  target must be IdentExpr or IndexExpr —
// enforced by sema (Step 3), not by the AST itself.
struct AssignExpr : Expr {
    AssignOp op;
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
    // Compound ops only: the type `target op value` is computed in (set by
    // Sema), before the result converts back to the target's type.
    TypeSpec opType;
    AssignExpr(AssignOp o, std::unique_ptr<Expr> t, std::unique_ptr<Expr> v)
        : Expr(NodeKind::AssignExpr), op(o), target(std::move(t)), value(std::move(v)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// Inserted by Sema (Step 3), never by the parser: wraps an operand whose
// static type differs from the type its context requires (an int literal
// used where a float is expected, a char promoted to int, ...), recording
// the usual-arithmetic-conversion the report's methodology calls for.
// operand->resolvedType is the source type; this node's own resolvedType
// (set by Sema alongside targetType) is the destination type — the two
// are always equal by construction; targetType is kept as the explicit,
// readable record of intent.
struct ConvertExpr : Expr {
    std::unique_ptr<Expr> operand;
    TypeSpec targetType;
    ConvertExpr(std::unique_ptr<Expr> e, TypeSpec t)
        : Expr(NodeKind::ConvertExpr), operand(std::move(e)), targetType(t) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- statements --------------------------------------------------------

// expr may be null: expr_stmt ::= [expression] ";" permits a bare ';'.
// An explicit C cast: (float)x, (int)f, (int *)p. Unlike ConvertExpr this is
// written by the programmer, so it is produced by the parser, not by Sema.
// Numeric casts become TO_INT / TO_FLOAT; pointer casts change only the type
// the checker sees, since a pointer carries no width here.
struct CastExpr : Expr {
    TypeSpec target;
    std::unique_ptr<Expr> operand;
    CastExpr(TypeSpec t, std::unique_ptr<Expr> e)
        : Expr(NodeKind::CastExpr), target(t), operand(std::move(e)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// base.field or base->field. Sema fills slotOffset (where the field starts
// inside the struct) and resolvedType (the field's own type).
struct MemberExpr : Expr {
    std::unique_ptr<Expr> base;
    std::string field;
    bool arrow;
    int  slotOffset = 0;
    MemberExpr(std::unique_ptr<Expr> b, std::string f, bool a)
        : Expr(NodeKind::MemberExpr), base(std::move(b)), field(std::move(f)), arrow(a) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// sizeof(type) or sizeof expr. A compile-time constant: Sema computes value.
struct SizeofExpr : Expr {
    bool ofType = false;
    TypeSpec type;
    std::unique_ptr<Expr> operand;
    long long value = 0;
    SizeofExpr() : Expr(NodeKind::SizeofExpr) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// cond ? thenExpr : elseExpr
struct CondExpr : Expr {
    std::unique_ptr<Expr> cond, thenExpr, elseExpr;
    CondExpr(std::unique_ptr<Expr> c, std::unique_ptr<Expr> t, std::unique_ptr<Expr> e)
        : Expr(NodeKind::CondExpr), cond(std::move(c)), thenExpr(std::move(t)), elseExpr(std::move(e)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// lhs , rhs — evaluates lhs, discards it, yields rhs.
struct CommaExpr : Expr {
    std::unique_ptr<Expr> lhs, rhs;
    CommaExpr(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : Expr(NodeKind::CommaExpr), lhs(std::move(l)), rhs(std::move(r)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    explicit ExprStmt(std::unique_ptr<Expr> e) : Stmt(NodeKind::ExprStmt), expr(std::move(e)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// compound_stmt ::= "{" { var_decl } { statement } "}" — the grammar
// requires every local declaration before the first statement, so the two
// lists are kept separate rather than interleaved.
// A declaration appearing in statement position. MiniC originally required
// every local to precede every statement (the C89 rule), which meant a block
// could keep declarations and statements in two separate lists. Allowing a
// declaration anywhere means ORDER matters -- `int b = a * 2;` after `a = 5;`
// must see the assigned value -- so a declaration now travels inside the
// statement list rather than being hoisted to the top of the block.
struct DeclStmt : Stmt {
    std::unique_ptr<VarDecl> decl;
    DeclStmt() : Stmt(NodeKind::DeclStmt) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct CompoundStmt : Stmt {
    std::vector<std::unique_ptr<VarDecl>> locals;
    std::vector<std::unique_ptr<Stmt>>    stmts;
    CompoundStmt() : Stmt(NodeKind::CompoundStmt) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> thenStmt;
    std::unique_ptr<Stmt> elseStmt;   // nullable
    IfStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> e)
        : Stmt(NodeKind::IfStmt), cond(std::move(c)), thenStmt(std::move(t)), elseStmt(std::move(e)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;
    WhileStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> b)
        : Stmt(NodeKind::WhileStmt), cond(std::move(c)), body(std::move(b)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// do body while (cond);
struct DoStmt : Stmt {
    std::unique_ptr<Stmt> body;
    std::unique_ptr<Expr> cond;
    DoStmt(std::unique_ptr<Stmt> b, std::unique_ptr<Expr> c)
        : Stmt(NodeKind::DoStmt), body(std::move(b)), cond(std::move(c)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// for (init; cond; step) body — all three clauses are plain expressions
// (no declaration-in-for), matching grammar.ebnf; each may be null.
struct ForStmt : Stmt {
    std::unique_ptr<Expr> init, cond, step;
    std::unique_ptr<Stmt> body;
    ForStmt(std::unique_ptr<Expr> i, std::unique_ptr<Expr> c, std::unique_ptr<Expr> s, std::unique_ptr<Stmt> b)
        : Stmt(NodeKind::ForStmt), init(std::move(i)), cond(std::move(c)), step(std::move(s)), body(std::move(b)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// value is null for a bare `return;` in a void function.
struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    explicit ReturnStmt(std::unique_ptr<Expr> v) : Stmt(NodeKind::ReturnStmt), value(std::move(v)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct BreakStmt : Stmt {
    BreakStmt() : Stmt(NodeKind::BreakStmt) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct ContinueStmt : Stmt {
    ContinueStmt() : Stmt(NodeKind::ContinueStmt) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// switch (cond) { case K: ... default: ... }. Each case keeps its own
// statement list; control falls through from one to the next unless a break
// intervenes, exactly as in C.
struct SwitchCase {
    bool isDefault = false;
    long long value = 0;
    std::vector<std::unique_ptr<Stmt>> stmts;
    int line = 0, col = 0;
};
struct SwitchStmt : Stmt {
    std::unique_ptr<Expr> cond;
    std::vector<SwitchCase> cases;
    SwitchStmt() : Stmt(NodeKind::SwitchStmt) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct GotoStmt : Stmt {
    std::string label;
    explicit GotoStmt(std::string l) : Stmt(NodeKind::GotoStmt), label(std::move(l)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// `name: stmt`
struct LabelStmt : Stmt {
    std::string label;
    std::unique_ptr<Stmt> stmt;
    LabelStmt(std::string l, std::unique_ptr<Stmt> s)
        : Stmt(NodeKind::LabelStmt), label(std::move(l)), stmt(std::move(s)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// ---- declarations -------------------------------------------------------

// One name per VarDecl. `var_decl`'s comma-separated declarator list in the
// grammar is desugared by the parser (Step 2) into one VarDecl per name —
// the AST itself never represents the comma list.
struct VarDecl : Decl {
    TypeSpec type;
    std::string name;
    std::unique_ptr<Expr> init;                    // scalar initializer, or null
    std::vector<std::unique_ptr<Expr>> initList;    // brace-list initializer, or empty
    int storageOffset = -1;                         // assigned by Sema (Step 3)
    // Set by Sema when `&x` appears anywhere for this scalar. IRGen then keeps
    // the variable in a one-slot box so every access goes through memory, which
    // the optimizer never reorders or deletes -- see IRGen::genVarDeclInto.
    bool addressTaken = false;
    // `static` on a LOCAL: one storage cell for the whole run, initialized
    // once. IRGen lowers it to a global under a function-qualified name.
    bool isStatic = false;
    VarDecl(TypeSpec t, std::string n) : Decl(NodeKind::VarDecl), type(t), name(std::move(n)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// A function parameter. Not a Node/not visited — it carries no executable
// content of its own, just a (type, name) pair consumed by FuncDecl.
struct Param {
    TypeSpec type;
    std::string name;
    int line = 0, col = 0;
    int storageOffset = -1;                          // assigned by Sema (Step 3)
    bool addressTaken = false;                       // see VarDecl::addressTaken
};

struct FuncDecl : Decl {
    TypeSpec returnType;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<CompoundStmt> body;
    FuncDecl(TypeSpec rt, std::string n) : Decl(NodeKind::FuncDecl), returnType(rt), name(std::move(n)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// The translation unit: top-level VarDecls and FuncDecls, in source order.
struct Program : Node {
    std::vector<std::unique_ptr<Decl>> decls;
    StructTable structs;      // filled by the parser; read by Sema and IRGen
    Program() : Node(NodeKind::Program) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

} // namespace minic
