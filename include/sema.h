// MiniC-Opt — semantic analysis and type checking
// Owner: Member 2 (Background / Requirements Analyst)
//
// A single Visitor pass over the parsed AST. It populates a SymbolTable
// scope by scope, reports the six diagnostic categories the report commits
// to (undeclared identifier, redeclaration in scope, type mismatch,
// call-arity mismatch, non-array subscripted, missing return), and
// annotates the AST in place for IR generation (Step 4) to consume:
//   - every Expr's `resolvedType` (ast.h)
//   - every VarDecl/Param's `storageOffset` (ast.h)
//   - a ConvertExpr node wherever an implicit arithmetic conversion applies
// print_int/print_float/print_char/read_int are not grammar keywords —
// they're recognised here, by name, as an ordinary CallExpr whose callee
// happens to match a fixed built-in table, rather than as new lexer
// keywords or new grammar productions.
#pragma once
#include "ast.h"
#include "symbol.h"
#include "diagnostic.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace minic {

class Sema : public Visitor {
public:
    // Returns true iff analysis produced zero diagnostics.
    bool analyze(Program& prog);
    const std::vector<Diagnostic>& errors() const { return sink_.all(); }

    void visit(IntLit&) override;
    void visit(FloatLit&) override;
    void visit(CharLit&) override;
    void visit(IdentExpr&) override;
    void visit(IndexExpr&) override;
    void visit(CallExpr&) override;
    void visit(UnaryExpr&) override;
    void visit(BinaryExpr&) override;
    void visit(AssignExpr&) override;
    void visit(ConvertExpr&) override;

    void visit(ExprStmt&) override;
    void visit(CompoundStmt&) override;
    void visit(IfStmt&) override;
    void visit(WhileStmt&) override;
    void visit(ForStmt&) override;
    void visit(DoStmt&) override;
    void visit(ReturnStmt&) override;
    void visit(BreakStmt&) override;
    void visit(ContinueStmt&) override;

    void visit(VarDecl&) override;
    void visit(FuncDecl&) override;
    void visit(Program&) override;

private:
    SymbolTable    symtab_;
    DiagnosticSink sink_;

    // Function signatures live in their own flat table, not in symtab_ —
    // MiniC has no nested/local functions, so they aren't scoped the way
    // variables are, and pre-registering them (registerSignatures, run
    // before any body is checked) is what lets forward and mutually
    // recursive calls resolve regardless of source order.
    std::unordered_map<std::string, Symbol> functions_;

    TypeSpec currentReturnType_{BaseType::INT, false, -1};

    struct BuiltinSig { std::vector<BaseType> params; BaseType ret; };
    static const std::unordered_map<std::string, BuiltinSig>& builtins();

    void registerSignatures(Program& prog);
    Symbol* lookupFunctionSymbol(const std::string& name);

    // Visits `e`, returning what the visit stored in e.resolvedType — the
    // Visitor interface's visit() is void, so this is how callers get a
    // usable result back out of double dispatch.
    TypeSpec checkExpr(Expr& e);

    // Splices a ConvertExpr in front of *slot if its resolved type differs
    // from `target` (both scalar, both non-void) — a no-op otherwise.
    void wrapConversionIfNeeded(std::unique_ptr<Expr>& slot, TypeSpec target);

    static TypeSpec usualArith(TypeSpec a, TypeSpec b);
    bool typesCompatible(TypeSpec a, TypeSpec b) const;

    // Conservative structural "does this statement return on every path it
    // can fall into" check — Review 2 scope, not the CFG-based reachability
    // Step 5 will eventually make possible. Loops are always treated as
    // "does not definitely return", even a do-while that always runs once.
    bool stmtDefinitelyReturns(const Stmt* s) const;
};

} // namespace minic
