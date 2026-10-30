// Cppify — AST reference interpreter (roadmap 3.1)
//
// The differential gate runs one interpreter twice, on unoptimized and on
// optimized IR. Both runs start from the same IRGen output, so a bug in
// IRGen (a wrong label, a loop that skips its step on `continue`, a struct
// passed by reference instead of copied) produces the same wrong answer
// twice and passes. This interpreter walks Sema's typed AST directly and
// never sees IR, so it gives a second answer that IRGen cannot influence.
//
// It works out the meaning of each construct from the AST's types and
// shares only the C library, the integer and pointer arithmetic primitive
// (Interp::applyBinary, whose semantics the clang oracle checks) and the
// memory model of the quad interpreter: a scalar is one slot, a struct is a
// flat run of slots, a pointer is (array, offset). The memory model must be
// shared for outputs to be comparable at all (printf's %p prints offsets).
//
// Control flow is structured recursion. `goto` jumps by unwinding to the
// nearest enclosing statement that contains the label and re-entering it in
// a "seeking" mode that skips everything up to the label.
#pragma once
#include "ast.h"
#include "interp.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace minic {

class AstInterp {
public:
    // Runs `main` of a program that passed Sema. Returns main's value (an
    // exit(n) inside the program returns n). Throws Interp::RuntimeError on
    // the same runtime errors as the quad interpreter.
    Value run(Program& prog);

private:
    using Store = std::shared_ptr<std::vector<Value>>;
    // A variable's storage. A scalar (including a pointer, or a parameter of
    // any type) is one slot holding its value; an array or a struct value is
    // its slots, and the variable's value is a pointer to them.
    struct Binding {
        Store store;
        bool aggregate = false;
    };
    // A memory cell: element `index` of the object `base` points into.
    struct Addr {
        Value base;
        long long index = 0;
    };
    enum class Flow { NORMAL, BREAK, CONTINUE, RETURN, GOTO };

    const StructTable* structs_ = nullptr;
    std::unordered_map<std::string, FuncDecl*> funcs_;
    std::unordered_map<std::string, Binding> globals_;
    std::unordered_map<const VarDecl*, Binding> statics_;
    std::vector<std::unordered_map<std::string, Binding>> scopes_;   // the running function's
    std::map<const Stmt*, std::set<std::string>> labelsIn_;          // labels defined inside each statement
    Interp lib_;

    Value retVal_;
    std::string gotoLabel_;
    bool seeking_ = false;   // re-entering statements to reach gotoLabel_

    // expressions
    Value eval(Expr& e);
    Addr addr(Expr& e);
    Value evalCall(CallExpr& c);
    Value evalUnary(UnaryExpr& u);
    Value evalBinary(BinaryExpr& b);
    Value evalAssign(AssignExpr& a);
    Value load(const Addr& a);
    void store(const Addr& a, const Value& v);
    Value pointerFrom(const Addr& a);
    Value copyOf(const Value& src, const TypeSpec& t);
    void copyBlock(const Value& src, const Value& dst, long long n);
    Value convert(const Value& v, const TypeSpec& from, const TypeSpec& to);
    int slots(const TypeSpec& t) const { return structs_->slotsOf(t); }

    // statements
    Flow exec(Stmt& s);
    Flow execCompound(CompoundStmt& c);
    Flow execSwitch(SwitchStmt& s);
    bool contains(const Stmt* s, const std::string& label) const;
    const std::set<std::string>& collectLabels(Stmt* s);

    // names
    Binding declare(VarDecl& d, bool runInit);
    Binding* lookup(const std::string& name);
    Value callFunction(FuncDecl& f, std::vector<Value> args);
};

}  // namespace minic
