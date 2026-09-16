// MiniC-Opt — semantic analysis and type checking (implementation)
// Owner: Member 2 (Background / Requirements Analyst)
#include "sema.h"
#include <algorithm>

namespace minic {

// ---- entry point ----------------------------------------------------

bool Sema::analyze(Program& prog) {
    prog.accept(*this);
    return sink_.empty();
}

// ---- built-ins & function signatures --------------------------------

const std::unordered_map<std::string, Sema::BuiltinSig>& Sema::builtins() {
    static const std::unordered_map<std::string, BuiltinSig> table = {
        {"print_int",   {{BaseType::INT},   BaseType::VOID}},
        {"print_float", {{BaseType::FLOAT}, BaseType::VOID}},
        {"print_char",  {{BaseType::CHAR},  BaseType::VOID}},
        {"read_int",    {{},                BaseType::INT}},
    };
    return table;
}

void Sema::registerSignatures(Program& prog) {
    for (auto& d : prog.decls) {
        auto* fn = dynamic_cast<FuncDecl*>(d.get());
        if (!fn) continue;
        if (builtins().count(fn->name)) {
            sink_.add(fn->line, fn->col,
                       "function '" + fn->name + "' shadows a built-in of the same name");
            continue;
        }
        if (functions_.count(fn->name)) {
            sink_.add(fn->line, fn->col, "redeclaration of function '" + fn->name + "'");
            continue;
        }
        Symbol sym;
        sym.kind = SymbolKind::FUNCTION;
        sym.type = fn->returnType;
        sym.name = fn->name;
        for (auto& p : fn->params) sym.paramTypes.push_back(p.type);
        functions_.emplace(fn->name, std::move(sym));
    }
}

Symbol* Sema::lookupFunctionSymbol(const std::string& name) {
    auto it = functions_.find(name);
    return it == functions_.end() ? nullptr : &it->second;
}

// ---- helpers ----------------------------------------------------------

TypeSpec Sema::checkExpr(Expr& e) {
    e.accept(*this);
    return e.resolvedType;
}

TypeSpec Sema::usualArith(TypeSpec a, TypeSpec b) {
    BaseType ab = (a.base == BaseType::VOID) ? BaseType::INT : a.base;   // recovery: treat as int
    BaseType bb = (b.base == BaseType::VOID) ? BaseType::INT : b.base;
    if (ab == BaseType::FLOAT || bb == BaseType::FLOAT) return TypeSpec{BaseType::FLOAT, false, -1};
    return TypeSpec{BaseType::INT, false, -1};   // char promotes to int; int stays int
}

bool Sema::typesCompatible(TypeSpec a, TypeSpec b) const {
    if (a.isArray != b.isArray) return false;
    if (a.base == BaseType::VOID || b.base == BaseType::VOID) return a.base == b.base;
    return true;   // any int/char/float combination is compatible via implicit conversion
}

void Sema::wrapConversionIfNeeded(std::unique_ptr<Expr>& slot, TypeSpec target) {
    if (!slot) return;
    if (slot->resolvedType.isArray || target.isArray) return;
    if (slot->resolvedType.base == target.base) return;
    if (slot->resolvedType.base == BaseType::VOID || target.base == BaseType::VOID) return;
    auto conv = std::make_unique<ConvertExpr>(std::move(slot), target);
    conv->line = conv->operand->line; conv->col = conv->operand->col;
    conv->resolvedType = target;
    slot = std::move(conv);
}

bool Sema::stmtDefinitelyReturns(const Stmt* s) const {
    if (!s) return false;
    switch (s->kind) {
        case NodeKind::ReturnStmt:
            return true;
        case NodeKind::CompoundStmt: {
            auto* cs = static_cast<const CompoundStmt*>(s);
            for (auto& st : cs->stmts)
                if (stmtDefinitelyReturns(st.get())) return true;
            return false;
        }
        case NodeKind::IfStmt: {
            auto* is = static_cast<const IfStmt*>(s);
            return is->elseStmt
                && stmtDefinitelyReturns(is->thenStmt.get())
                && stmtDefinitelyReturns(is->elseStmt.get());
        }
        default:
            // while/for/do-while: conservatively "does not definitely
            // return" even for a do-while, which always runs its body at
            // least once — that refinement needs real CFG reachability
            // (Step 5), not this structural pre-pass.
            return false;
    }
}

// ---- expressions --------------------------------------------------------

void Sema::visit(IntLit& n)   { n.resolvedType = TypeSpec{BaseType::INT,   false, -1}; }
void Sema::visit(FloatLit& n) { n.resolvedType = TypeSpec{BaseType::FLOAT, false, -1}; }
void Sema::visit(CharLit& n)  { n.resolvedType = TypeSpec{BaseType::CHAR,  false, -1}; }

void Sema::visit(IdentExpr& n) {
    Symbol* sym = symtab_.lookup(n.name);
    if (!sym) {
        sink_.add(n.line, n.col, "undeclared identifier '" + n.name + "'");
        n.resolvedType = TypeSpec{BaseType::INT, false, -1};
        return;
    }
    if (sym->kind == SymbolKind::FUNCTION) {
        sink_.add(n.line, n.col,
                   "function '" + n.name + "' used as a value (MiniC has no function pointers)");
        n.resolvedType = TypeSpec{BaseType::INT, false, -1};
        return;
    }
    n.resolvedType = sym->type;
}

void Sema::visit(IndexExpr& n) {
    TypeSpec baseType = checkExpr(*n.base);
    TypeSpec idxType  = checkExpr(*n.index);
    if (!baseType.isArray)
        sink_.add(n.line, n.col, "subscripted value is not an array");
    if (idxType.isArray || idxType.base == BaseType::FLOAT)
        sink_.add(n.line, n.col, "array index must be an integral expression");
    n.resolvedType = TypeSpec{baseType.base, false, -1};
}

void Sema::visit(CallExpr& n) {
    auto biIt = builtins().find(n.callee);
    const BuiltinSig* bi = (biIt != builtins().end()) ? &biIt->second : nullptr;
    Symbol* fn = bi ? nullptr : lookupFunctionSymbol(n.callee);

    if (!bi && !fn) {
        sink_.add(n.line, n.col, "call to undeclared function '" + n.callee + "'");
        for (auto& a : n.args) checkExpr(*a);      // still check args for further diagnostics
        n.resolvedType = TypeSpec{BaseType::INT, false, -1};
        return;
    }

    std::vector<TypeSpec> paramTypes;
    BaseType retType;
    if (bi) {
        for (auto b : bi->params) paramTypes.push_back(TypeSpec{b, false, -1});
        retType = bi->ret;
    } else {
        paramTypes = fn->paramTypes;
        retType = fn->type.base;
    }

    if (n.args.size() != paramTypes.size()) {
        sink_.add(n.line, n.col,
                   "'" + n.callee + "' expects " + std::to_string(paramTypes.size())
                   + " argument" + (paramTypes.size() == 1 ? "" : "s")
                   + ", got " + std::to_string(n.args.size()));
    }

    size_t checkCount = std::min(n.args.size(), paramTypes.size());
    for (size_t i = 0; i < checkCount; ++i) {
        TypeSpec at = checkExpr(*n.args[i]);
        TypeSpec pt = paramTypes[i];
        if (!typesCompatible(at, pt))
            sink_.add(n.args[i]->line, n.args[i]->col,
                       "argument " + std::to_string(i + 1) + " to '" + n.callee + "' has the wrong type");
        else
            wrapConversionIfNeeded(n.args[i], pt);
    }
    for (size_t i = checkCount; i < n.args.size(); ++i) checkExpr(*n.args[i]);

    n.resolvedType = TypeSpec{retType, false, -1};
}

void Sema::visit(UnaryExpr& n) {
    TypeSpec ot = checkExpr(*n.operand);
    switch (n.op) {
        case UnOp::NOT:
            if (ot.isArray) sink_.add(n.line, n.col, "operand of '!' must be a scalar value");
            n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            break;
        case UnOp::NEG:
        case UnOp::POS:
            if (ot.isArray || ot.base == BaseType::VOID)
                sink_.add(n.line, n.col, std::string("operand of unary '") + toString(n.op) + "' must be numeric");
            n.resolvedType = (ot.base == BaseType::CHAR) ? TypeSpec{BaseType::INT, false, -1}
                                                          : TypeSpec{ot.base, false, -1};
            break;
        case UnOp::PRE_INC: case UnOp::PRE_DEC:
        case UnOp::POST_INC: case UnOp::POST_DEC:
            if (n.operand->kind != NodeKind::IdentExpr && n.operand->kind != NodeKind::IndexExpr)
                sink_.add(n.line, n.col, "operand of increment/decrement must be a variable or array element");
            if (ot.isArray || ot.base == BaseType::VOID)
                sink_.add(n.line, n.col, "operand of increment/decrement must be numeric");
            n.resolvedType = TypeSpec{ot.base, false, -1};
            break;
    }
}

void Sema::visit(BinaryExpr& n) {
    TypeSpec lt = checkExpr(*n.lhs);
    TypeSpec rt = checkExpr(*n.rhs);
    if (lt.isArray || rt.isArray || lt.base == BaseType::VOID || rt.base == BaseType::VOID)
        sink_.add(n.line, n.col, std::string("invalid operands to binary '") + toString(n.op) + "'");

    switch (n.op) {
        case BinOp::LOGAND:
        case BinOp::LOGOR:
            n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            break;
        case BinOp::LT: case BinOp::LE: case BinOp::GT: case BinOp::GE:
        case BinOp::EQ: case BinOp::NE: {
            TypeSpec cmp = usualArith(lt, rt);
            wrapConversionIfNeeded(n.lhs, cmp);
            wrapConversionIfNeeded(n.rhs, cmp);
            n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            break;
        }
        default: {   // ADD SUB MUL DIV MOD
            TypeSpec result = usualArith(lt, rt);
            wrapConversionIfNeeded(n.lhs, result);
            wrapConversionIfNeeded(n.rhs, result);
            n.resolvedType = result;
            break;
        }
    }
}

void Sema::visit(AssignExpr& n) {
    TypeSpec tt = checkExpr(*n.target);
    TypeSpec vt = checkExpr(*n.value);
    // An invalid target shape (not an IdentExpr/IndexExpr) was already
    // diagnosed by the parser (Step 2) — don't duplicate that message.
    if (n.target->kind == NodeKind::IdentExpr || n.target->kind == NodeKind::IndexExpr) {
        if (tt.isArray)
            sink_.add(n.line, n.col, "cannot assign to an array; assign to an element instead");
        else if (!typesCompatible(tt, vt))
            sink_.add(n.line, n.col, "type mismatch in assignment");
        else
            wrapConversionIfNeeded(n.value, tt);
    }
    n.resolvedType = tt;
}

void Sema::visit(ConvertExpr& n) {
    // Sema is the only thing that creates these; nothing walks back over a
    // wrapped subtree during the same pass. Implemented for completeness —
    // Visitor requires every node kind to be handled.
    if (n.operand) checkExpr(*n.operand);
    n.resolvedType = n.targetType;
}

// ---- statements -----------------------------------------------------------

void Sema::visit(ExprStmt& n) {
    if (n.expr) checkExpr(*n.expr);
}

void Sema::visit(CompoundStmt& n) {
    symtab_.enterScope();
    for (auto& d : n.locals) d->accept(*this);
    for (auto& s : n.stmts)  s->accept(*this);
    symtab_.exitScope();
}

void Sema::visit(IfStmt& n) {
    checkExpr(*n.cond);
    n.thenStmt->accept(*this);
    if (n.elseStmt) n.elseStmt->accept(*this);
}

void Sema::visit(WhileStmt& n) {
    checkExpr(*n.cond);
    n.body->accept(*this);
}

void Sema::visit(DoStmt& n) {
    n.body->accept(*this);
    checkExpr(*n.cond);
}

void Sema::visit(ForStmt& n) {
    // for's own header is not a new scope in MiniC's grammar (init/cond/step
    // are plain expressions, not declarations), so nothing to push here —
    // the body, if it's a CompoundStmt, pushes its own scope as usual.
    if (n.init) checkExpr(*n.init);
    if (n.cond) checkExpr(*n.cond);
    if (n.step) checkExpr(*n.step);
    n.body->accept(*this);
}

void Sema::visit(ReturnStmt& n) {
    if (n.value) {
        TypeSpec vt = checkExpr(*n.value);
        if (currentReturnType_.base == BaseType::VOID) {
            sink_.add(n.line, n.col, "'return' with a value in a function returning void");
        } else if (!typesCompatible(vt, currentReturnType_)) {
            sink_.add(n.line, n.col,
                       std::string("return type mismatch: function returns ") + toString(currentReturnType_.base));
        } else {
            wrapConversionIfNeeded(n.value, currentReturnType_);
        }
    } else if (currentReturnType_.base != BaseType::VOID) {
        sink_.add(n.line, n.col,
                   std::string("'return;' with no value in a function returning ") + toString(currentReturnType_.base));
    }
}

void Sema::visit(BreakStmt&)    {}
void Sema::visit(ContinueStmt&) {}

// ---- declarations ---------------------------------------------------------

void Sema::visit(VarDecl& n) {
    if (n.type.base == BaseType::VOID)
        sink_.add(n.line, n.col, "variable '" + n.name + "' declared void");

    if (n.type.isArray) {
        if (n.init) {
            sink_.add(n.line, n.col,
                       "array '" + n.name + "' cannot be initialized from a scalar expression; use { ... }");
            checkExpr(*n.init);
        }
        TypeSpec elemType{n.type.base, false, -1};
        for (auto& e : n.initList) {
            TypeSpec et = checkExpr(*e);
            if (!typesCompatible(elemType, et))
                sink_.add(e->line, e->col, "initializer type does not match array element type");
            else
                wrapConversionIfNeeded(e, elemType);
        }
        if (n.type.arraySize >= 0 && static_cast<int>(n.initList.size()) > n.type.arraySize)
            sink_.add(n.line, n.col,
                       "too many initializers for array '" + n.name + "' of size " + std::to_string(n.type.arraySize));
    } else {
        if (!n.initList.empty())
            sink_.add(n.line, n.col, "'" + n.name + "' is not an array; cannot use a { ... } initializer");
        if (n.init) {
            TypeSpec it = checkExpr(*n.init);
            if (!typesCompatible(n.type, it))
                sink_.add(n.line, n.col, "type mismatch initializing '" + n.name + "'");
            else
                wrapConversionIfNeeded(n.init, n.type);
        }
    }

    int offset = symtab_.nextOffset();
    Symbol sym;
    sym.kind = SymbolKind::VARIABLE;
    sym.type = n.type;
    sym.name = n.name;
    sym.offset = offset;
    if (symtab_.declare(sym))
        n.storageOffset = offset;
    else
        sink_.add(n.line, n.col, "redeclaration of '" + n.name + "' in this scope");
}

void Sema::visit(FuncDecl& n) {
    symtab_.enterScope();      // parameter scope
    symtab_.resetOffsets();

    for (auto& p : n.params) {
        int offset = symtab_.nextOffset();
        Symbol sym;
        sym.kind = SymbolKind::PARAMETER;
        sym.type = p.type;
        sym.name = p.name;
        sym.offset = offset;
        if (symtab_.declare(sym))
            p.storageOffset = offset;
        else
            sink_.add(p.line, p.col, "redeclaration of parameter '" + p.name + "'");
    }

    TypeSpec savedReturn = currentReturnType_;
    currentReturnType_ = n.returnType;

    if (n.body) {
        n.body->accept(*this);   // CompoundStmt pushes its own nested scope
        if (n.returnType.base != BaseType::VOID && !stmtDefinitelyReturns(n.body.get()))
            sink_.add(n.line, n.col,
                       "function '" + n.name + "' does not return a value on every path (missing return)");
    }

    currentReturnType_ = savedReturn;
    symtab_.exitScope();
}

void Sema::visit(Program& n) {
    registerSignatures(n);
    for (auto& d : n.decls) d->accept(*this);
}

} // namespace minic
