// MiniC-Opt — IR generation (syntax-directed translation to TAC)
// Owner: Member 1 (Front End) / Step 4
//
// Translates a Sema-annotated AST (Program::decls, with every Expr's
// resolvedType filled in and every ConvertExpr already spliced in by Sema)
// into an IRProgram of quadruples. Implemented as a Visitor, exactly like
// AstPrinter and Sema, so the three passes stay structurally comparable.
//
// Two design points worth calling out (both explained at length in the
// implementation file, src/ir/irgen.cpp):
//   - IRGen re-derives lexical scoping itself, in lockstep with how Sema
//     walked the tree, to turn each local name into a collision-free TAC
//     name ("name#offset", offset = the VarDecl/Param::storageOffset Sema
//     already computed uniquely per function). This is what makes shadowed
//     locals in nested blocks safe without carrying a symbol table forward
//     from Sema.
//   - Boolean short-circuit codegen (&&, ||, !) is only used in conditional
//     *context* (if/while/for/do conditions), via genCondJumpTrue/False,
//     matching Annexure C. A bare boolean-valued expression evaluates
//     eagerly (Op::AND/OR) — MiniC's usual arithmetic conversions apply to
//     Op::AND/OR the same way they do to Op::ADD etc.
#pragma once
#include "ast.h"
#include "ir.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minic {

class IRGen : public Visitor {
public:
    // Runs generation over an already Sema-checked Program. Undefined
    // result if `prog` still has outstanding diagnostics — callers (see
    // main.cpp) are expected to gate on Sema::analyze() returning true
    // first, matching how --check already reports before continuing.
    IRProgram generate(Program& prog);

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
    IRProgram program_;
    IRFunction* cur_ = nullptr;          // current function's code, or nullptr for global init
    std::vector<Quad>* code_ = nullptr;  // where emit() appends: &cur_->code, or &program_.globalInit

    // Scope resolution, rebuilt per function (mirrors Sema's SymbolTable
    // push/pop structure exactly — see irgen.cpp for why).
    std::vector<std::unordered_map<std::string, std::string>> scopes_;
    std::unordered_set<std::string> globalNames_;

    // Scalars whose address is taken (`&x`) live in a one-slot array instead
    // of a plain variable, so a pointer to them is just (that array, 0).
    // Keyed by the TAC name (local "x#3" or bare global). Every access to a
    // boxed name goes through LOAD_INDEX/STORE_INDEX, which the optimizer
    // never reorders, merges or deletes — that is what keeps `*p = 5; x`
    // honest without any alias analysis.
    std::unordered_set<std::string> boxed_;
    std::unordered_set<std::string> userFns_;   // functions the program defines itself

    // Where an assignment-like expression writes: a plain variable, or a
    // memory cell named by (base, index) for a[i], *p and boxed scalars.
    struct LValue {
        bool isMem = false;
        Operand var;            // !isMem
        Operand base, index;    // isMem
    };
    LValue genLValue(Expr& e);
    Operand scaleIndex(Operand idx, int slots, int line);

    // Struct layout, owned by the Program being generated.
    const StructTable* structs_ = nullptr;
    int slots(const TypeSpec& t) const { return structs_ ? structs_->slotsOf(t) : 1; }
    static bool isAggregate(const TypeSpec& t) { return t.isArray || isStructValue(t); }

    // The memory cell an lvalue-shaped expression names: element `index` of
    // the object `base` points at. Nothing is loaded.
    struct Addr { Operand base, index; };
    Addr    genAddr(Expr& e);
    Operand pointerFrom(const Addr& a, int line);   // base + index as a pointer value
    Operand addConst(Operand idx, long long k, int line);
    Operand copyOfAggregate(Operand srcPtr, const TypeSpec& t, int line);   // fresh copy, for by-value passing

    // visit() is void (double dispatch), so expression codegen threads its
    // result Operand through this instead of a return value.
    Operand lastValue_;
    Operand genExpr(Expr& e) { e.accept(*this); return lastValue_; }

    // break/continue targets of the innermost enclosing loop.
    struct LoopCtx { Operand breakLabel, continueLabel; };
    std::vector<LoopCtx> loopStack_;

    // Temp/label allocation delegates to the current IRFunction when
    // there is one; a global initializer (cur_ == nullptr) gets its own
    // small counters so --dump-ir never has to special-case globalInit.
    int globalTempCounter_ = 0;
    int globalLabelCounter_ = 0;
    Operand newTemp();
    Operand newLabel();

    void emit(Op op, Operand a1, Operand a2, Operand res, int line, IntTy ty = IntTy::NONE);
    static IntTy irTy(const TypeSpec& t);
    Operand convertValue(Operand v, const TypeSpec& from, const TypeSpec& to, int line);
    void pushScope();
    void popScope();
    std::string declareLocal(const std::string& name, int storageOffset);
    Operand resolveName(const std::string& name, int line, int col);   // -> VARIABLE operand
    Operand resolveArrayBase(Expr& base);                              // IdentExpr only, by contract

    void genVarDeclInto(VarDecl& n, bool isGlobal);
    void genFunctionBody(FuncDecl& n);

    // Short-circuit conditional codegen (see class comment above).
    void genCondJumpFalse(Expr& cond, const Operand& falseLabel);
    void genCondJumpTrue(Expr& cond, const Operand& trueLabel);

    static Operand zeroOf(const TypeSpec& t);
};

} // namespace minic
