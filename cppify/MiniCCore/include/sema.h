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
    void visit(StringLit&) override;

    void visit(IdentExpr&) override;
    void visit(IndexExpr&) override;
    void visit(CallExpr&) override;
    void visit(UnaryExpr&) override;
    void visit(BinaryExpr&) override;
    void visit(AssignExpr&) override;
    void visit(ConvertExpr&) override;
    void visit(CastExpr&) override;
    void visit(MemberExpr&) override;
    void visit(SizeofExpr&) override;
    void visit(CondExpr&) override;
    void visit(CommaExpr&) override;

    void visit(ExprStmt&) override;
    void visit(DeclStmt&) override;
    void visit(CompoundStmt&) override;
    void visit(IfStmt&) override;
    void visit(WhileStmt&) override;
    void visit(ForStmt&) override;
    void visit(DoStmt&) override;
    void visit(ReturnStmt&) override;
    void visit(BreakStmt&) override;
    void visit(ContinueStmt&) override;
    void visit(SwitchStmt&) override;
    void visit(GotoStmt&) override;
    void visit(LabelStmt&) override;

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
    const StructTable* structs_ = nullptr;      // set from Program; owned by the AST

    // goto/label bookkeeping, per function: a label is visible throughout the
    // function it is written in, including above its own definition.
    struct PendingGoto { std::string label; int line, col; };
    std::unordered_map<std::string, int> definedLabels_;
    std::vector<PendingGoto> pendingGotos_;

    // One scalar type per interpreter slot of `t`, in memory order. Used to
    // check a flattened brace initializer element by element.
    void flatSlotTypes(const TypeSpec& t, std::vector<TypeSpec>& out) const;
    std::string structName(int id) const;
    void checkScalarCondition(Expr& cond);

    struct BuiltinSig { std::vector<BaseType> params; BaseType ret; };
    static const std::unordered_map<std::string, BuiltinSig>& builtins();

    // The C library subset: strings, math, memory, character tests, console
    // I/O. A user function of the same name wins, so a program that writes
    // its own strlen still compiles and runs its own.
    struct LibFn { TypeSpec ret; std::vector<TypeSpec> params; };
    static const std::unordered_map<std::string, LibFn>& libFns();
    bool checkFormatFamily(CallExpr& n);

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
    static TypeSpec promote(TypeSpec t);
    // Division/modulo by a literal 0 (IntLit 0, or FloatLit 0.0 for /)
    // is statically detectable instead of deferring it to a runtime
    // crash - diagnoses it the same way the other five categories do.
    void checkConstantZeroDivisor(const BinaryExpr& n, const char* opSym);
    bool typesCompatible(TypeSpec a, TypeSpec b) const;
    // May a value of type `from` be stored into a `to`? Handles pointers: an
    // array decays to a pointer, void* converts to and from any pointer, and
    // the literal 0 is a null pointer. `fromExpr` is only consulted for that
    // literal-0 case and may be null.
    bool assignable(TypeSpec to, TypeSpec from, const Expr* fromExpr) const;
    static bool isNullLiteral(const Expr* e);

    // Conservative structural "does this statement return on every path it
    // can fall into" check — Review 2 scope, not the CFG-based reachability
    // Step 5 will eventually make possible. Loops are always treated as
    // "does not definitely return", even a do-while that always runs once.
    bool stmtDefinitelyReturns(const Stmt* s) const;
};

} // namespace minic
