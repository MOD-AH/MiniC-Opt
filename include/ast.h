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

enum class BaseType { INT, CHAR, FLOAT, VOID };

struct TypeSpec {
    BaseType base      = BaseType::INT;
    bool     isArray   = false;
    int      arraySize = -1;   // -1: no size given (e.g. a `T x[]` parameter)
};

enum class BinOp  { ADD, SUB, MUL, DIV, MOD, LT, LE, GT, GE, EQ, NE, LOGAND, LOGOR };
enum class UnOp   { NEG, POS, NOT, PRE_INC, PRE_DEC, POST_INC, POST_DEC };
enum class AssignOp { ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN };

const char* toString(BinOp op);
const char* toString(UnOp op);
const char* toString(AssignOp op);
const char* toString(BaseType t);

// ---- forward declarations of every concrete node ---------------------

struct IntLit; struct FloatLit; struct CharLit; struct IdentExpr;
struct IndexExpr; struct CallExpr; struct UnaryExpr; struct BinaryExpr; struct AssignExpr; struct ConvertExpr;

struct ExprStmt; struct CompoundStmt; struct IfStmt; struct WhileStmt;
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
    virtual void visit(IdentExpr&)  = 0;
    virtual void visit(IndexExpr&)  = 0;
    virtual void visit(CallExpr&)   = 0;
    virtual void visit(UnaryExpr&)  = 0;
    virtual void visit(BinaryExpr&) = 0;
    virtual void visit(AssignExpr&)  = 0;
    virtual void visit(ConvertExpr&) = 0;

    virtual void visit(ExprStmt&)     = 0;
    virtual void visit(CompoundStmt&) = 0;
    virtual void visit(IfStmt&)       = 0;
    virtual void visit(WhileStmt&)    = 0;
    virtual void visit(ForStmt&)      = 0;
    virtual void visit(DoStmt&)       = 0;
    virtual void visit(ReturnStmt&)   = 0;
    virtual void visit(BreakStmt&)    = 0;
    virtual void visit(ContinueStmt&) = 0;

    virtual void visit(VarDecl&)  = 0;
    virtual void visit(FuncDecl&) = 0;
    virtual void visit(Program&)  = 0;
};

// ---- node base -----------------------------------------------------------

enum class NodeKind {
    IntLit, FloatLit, CharLit, IdentExpr, IndexExpr, CallExpr, UnaryExpr, BinaryExpr, AssignExpr, ConvertExpr,
    ExprStmt, CompoundStmt, IfStmt, WhileStmt, ForStmt, DoStmt, ReturnStmt, BreakStmt, ContinueStmt,
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
struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    explicit ExprStmt(std::unique_ptr<Expr> e) : Stmt(NodeKind::ExprStmt), expr(std::move(e)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// compound_stmt ::= "{" { var_decl } { statement } "}" — the grammar
// requires every local declaration before the first statement, so the two
// lists are kept separate rather than interleaved.
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
    Program() : Node(NodeKind::Program) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

} // namespace minic
