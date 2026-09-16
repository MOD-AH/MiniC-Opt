// MiniC-Opt — IR generation (implementation)
// Owner: Member 1 (Front End) / Step 4
//
// Two design points that don't fit in a header comment:
//
// 1. Scope resolution. Sema (Step 3) writes a unique storageOffset onto
//    every VarDecl/Param, unique *per function* across every nested block
//    (SymbolTable::nextOffset() is only reset once, at the start of each
//    FuncDecl — see symbol.h). IRGen reuses that number to turn a source
//    name into a collision-free TAC name, "name#offset", and rebuilds the
//    same enterScope/exitScope nesting Sema used (parameter scope, then
//    one push per CompoundStmt) so that at any point in the tree, looking
//    a name up in IRGen's own scope stack finds the same declaration Sema
//    resolved it to — including a local that shadows an outer local or a
//    global. A name not found in any local scope is a global, emitted
//    under its own bare (already-unique) name.
//
// 2. Initializer scoping order. Sema checks a VarDecl's initializer
//    *before* declaring the name (sema.cpp, Sema::visit(VarDecl&)), so
//    `int x = x;` resolves the inner `x` against whatever `x` an outer
//    scope already has (or reports undeclared) — the same rule C itself
//    uses for block-scope initializers. genVarDeclInto mirrors that order
//    exactly: the initializer is generated before the name is declared.
#include "irgen.h"
#include <utility>

namespace minic {

namespace {

Op toIrOp(BinOp op) {
    switch (op) {
        case BinOp::ADD: return Op::ADD; case BinOp::SUB: return Op::SUB;
        case BinOp::MUL: return Op::MUL; case BinOp::DIV: return Op::DIV;
        case BinOp::MOD: return Op::MOD;
        case BinOp::LT: return Op::LT;   case BinOp::LE: return Op::LE;
        case BinOp::GT: return Op::GT;   case BinOp::GE: return Op::GE;
        case BinOp::EQ: return Op::EQ;   case BinOp::NE: return Op::NE;
        case BinOp::LOGAND: return Op::AND;   // unreachable: visit(BinaryExpr&) handles these itself
        case BinOp::LOGOR:  return Op::OR;    // unreachable
    }
    return Op::NOP;
}

Op assignOpToIr(AssignOp op) {
    switch (op) {
        case AssignOp::PLUS_ASSIGN:  return Op::ADD;
        case AssignOp::MINUS_ASSIGN: return Op::SUB;
        case AssignOp::STAR_ASSIGN:  return Op::MUL;
        case AssignOp::SLASH_ASSIGN: return Op::DIV;
        case AssignOp::ASSIGN:       return Op::NOP;   // unused: ASSIGN is handled before this is called
    }
    return Op::NOP;
}

Operand intConst(long long v) {
    Operand o; o.kind = OperandKind::INT_CONST; o.ival = v; return o;
}

std::string qualify(const std::string& name, int offset) {
    return name + "#" + std::to_string(offset);
}

} // namespace

IRProgram IRGen::generate(Program& prog) {
    program_ = IRProgram{};
    prog.accept(*this);
    return std::move(program_);
}

Operand IRGen::newTemp() {
    if (cur_) return cur_->newTemp();
    Operand o; o.kind = OperandKind::TEMPORARY; o.name = "gt" + std::to_string(globalTempCounter_++);
    return o;
}

Operand IRGen::newLabel() {
    if (cur_) return cur_->newLabel();
    Operand o; o.kind = OperandKind::LABEL_REF; o.name = "GL" + std::to_string(globalLabelCounter_++);
    return o;
}

void IRGen::emit(Op op, Operand a1, Operand a2, Operand res, int line) {
    Quad q;
    q.op = op; q.arg1 = std::move(a1); q.arg2 = std::move(a2); q.result = std::move(res);
    q.srcLine = line;
    code_->push_back(std::move(q));
}

void IRGen::pushScope() { scopes_.emplace_back(); }
void IRGen::popScope()  { scopes_.pop_back(); }

std::string IRGen::declareLocal(const std::string& name, int storageOffset) {
    std::string q = qualify(name, storageOffset);
    scopes_.back()[name] = q;
    return q;
}

Operand IRGen::resolveName(const std::string& name, int /*line*/, int /*col*/) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            Operand o; o.kind = OperandKind::VARIABLE; o.name = found->second;
            return o;
        }
    }
    // Not in any function-local scope: a global. Sema already guaranteed
    // this resolves to something real (or reported an error, in which case
    // generation was never reached — main.cpp gates --dump-ir/--run on a
    // clean --check first).
    Operand o; o.kind = OperandKind::VARIABLE; o.name = name;
    return o;
}

Operand IRGen::resolveArrayBase(Expr& base) {
    if (base.kind == NodeKind::IdentExpr) {
        auto& id = static_cast<IdentExpr&>(base);
        return resolveName(id.name, id.line, id.col);
    }
    // MiniC has no array-valued expression other than a bare identifier
    // (no array assignment, no array-returning calls) — this path is a
    // defensive fallback for a shape Sema itself would already reject.
    return genExpr(base);
}

Operand IRGen::zeroOf(const TypeSpec& t) {
    Operand o;
    if (t.base == BaseType::FLOAT) { o.kind = OperandKind::FLOAT_CONST; o.fval = 0.0; }
    else                           { o.kind = OperandKind::INT_CONST;   o.ival = 0;   }
    return o;
}

// ---- expressions ----------------------------------------------------

void IRGen::visit(IntLit& n) {
    Operand o; o.kind = OperandKind::INT_CONST; o.ival = n.value;
    lastValue_ = o;
}
void IRGen::visit(FloatLit& n) {
    Operand o; o.kind = OperandKind::FLOAT_CONST; o.fval = n.value;
    lastValue_ = o;
}
void IRGen::visit(CharLit& n) {
    // Chars are integral for every arithmetic purpose in MiniC (usual
    // arithmetic conversions promote char to int) — represented directly
    // as an INT_CONST, no separate CHAR_CONST kind needed in ir.h.
    lastValue_ = intConst(static_cast<long long>(static_cast<unsigned char>(n.value)));
}
void IRGen::visit(IdentExpr& n) {
    lastValue_ = resolveName(n.name, n.line, n.col);
}

void IRGen::visit(IndexExpr& n) {
    Operand base = resolveArrayBase(*n.base);
    Operand idx  = genExpr(*n.index);
    Operand t    = newTemp();
    emit(Op::LOAD_INDEX, base, idx, t, n.line);
    lastValue_ = t;
}

void IRGen::visit(CallExpr& n) {
    // Arguments are evaluated AND queued (PARAM) one at a time, strictly in
    // order, rather than evaluate-all-then-queue-all: if an argument is
    // itself a call (f(g(x))), its own PARAM/CALL pair must be fully
    // emitted — and, at interpret time, fully consumed — before this
    // call's own PARAM quads resume. Left to right, that's exactly what
    // "evaluate this arg, immediately PARAM it, move to the next" gives.
    for (auto& a : n.args) {
        Operand v = genExpr(*a);
        emit(Op::PARAM, v, {}, {}, a->line);
    }
    Operand callee; callee.kind = OperandKind::LABEL_REF; callee.name = n.callee;
    Operand argc = intConst(static_cast<long long>(n.args.size()));
    Operand result;
    if (n.resolvedType.base != BaseType::VOID) result = newTemp();
    emit(Op::CALL, callee, argc, result, n.line);
    lastValue_ = result;
}

void IRGen::visit(UnaryExpr& n) {
    switch (n.op) {
        case UnOp::POS:
            lastValue_ = genExpr(*n.operand);   // kept as a distinct AST node for --dump-ast; no-op in TAC
            return;
        case UnOp::NEG: {
            Operand v = genExpr(*n.operand);
            Operand t = newTemp();
            emit(Op::NEG, v, {}, t, n.line);
            lastValue_ = t;
            return;
        }
        case UnOp::NOT: {
            Operand v = genExpr(*n.operand);
            Operand t = newTemp();
            emit(Op::NOT, v, {}, t, n.line);
            lastValue_ = t;
            return;
        }
        case UnOp::PRE_INC: case UnOp::PRE_DEC:
        case UnOp::POST_INC: case UnOp::POST_DEC: {
            bool isInc = (n.op == UnOp::PRE_INC || n.op == UnOp::POST_INC);
            bool isPre = (n.op == UnOp::PRE_INC || n.op == UnOp::PRE_DEC);
            Op deltaOp = isInc ? Op::ADD : Op::SUB;
            Operand one = intConst(1);

            if (n.operand->kind == NodeKind::IdentExpr) {
                auto& id = static_cast<IdentExpr&>(*n.operand);
                Operand var  = resolveName(id.name, id.line, id.col);
                Operand oldT = newTemp();
                emit(Op::COPY, var, {}, oldT, n.line);
                Operand newT = newTemp();
                emit(deltaOp, oldT, one, newT, n.line);
                emit(Op::COPY, newT, {}, var, n.line);
                lastValue_ = isPre ? newT : oldT;
            } else if (n.operand->kind == NodeKind::IndexExpr) {
                // Index evaluated exactly once and reused for both the load
                // and the store, so a[f()]++ doesn't call f() twice.
                auto& idxE = static_cast<IndexExpr&>(*n.operand);
                Operand base = resolveArrayBase(*idxE.base);
                Operand ix   = genExpr(*idxE.index);
                Operand oldT = newTemp();
                emit(Op::LOAD_INDEX, base, ix, oldT, n.line);
                Operand newT = newTemp();
                emit(deltaOp, oldT, one, newT, n.line);
                emit(Op::STORE_INDEX, ix, newT, base, n.line);
                lastValue_ = isPre ? newT : oldT;
            } else {
                lastValue_ = genExpr(*n.operand);   // defensive: Sema already rejects this shape
            }
            return;
        }
    }
}

void IRGen::visit(BinaryExpr& n) {
    if (n.op == BinOp::LOGAND || n.op == BinOp::LOGOR) {
        // Materialize a short-circuited 0/1 through the same jump-code
        // machinery conditionals use (genCondJumpFalse), rather than
        // eagerly evaluating both sides with Op::AND/OR — this keeps
        // `&&`/`||` short-circuiting even when used as a plain value
        // (`int ok = (n != 0 && x / n > 1);`), matching C, and it means
        // there is exactly one place (genCondJump{True,False}) that knows
        // how to translate LOGAND/LOGOR.
        Operand result    = newTemp();
        Operand falseLabel = newLabel();
        Operand endLabel   = newLabel();
        genCondJumpFalse(n, falseLabel);
        emit(Op::COPY, intConst(1), {}, result, n.line);
        emit(Op::GOTO, {}, {}, endLabel, n.line);
        emit(Op::LABEL, {}, {}, falseLabel, n.line);
        emit(Op::COPY, intConst(0), {}, result, n.line);
        emit(Op::LABEL, {}, {}, endLabel, n.line);
        lastValue_ = result;
        return;
    }
    Operand l = genExpr(*n.lhs);
    Operand r = genExpr(*n.rhs);
    Operand t = newTemp();
    emit(toIrOp(n.op), l, r, t, n.line);
    lastValue_ = t;
}

void IRGen::visit(AssignExpr& n) {
    // RHS is evaluated before the target's storage location, for both
    // target shapes below — MiniC has no test relying on the opposite
    // order, and this matches how the target's *value* (needed only for
    // +=/-=/*=//=) is read afterward, right before the store.
    Operand v = genExpr(*n.value);

    if (n.target->kind == NodeKind::IdentExpr) {
        auto& id = static_cast<IdentExpr&>(*n.target);
        Operand var = resolveName(id.name, id.line, id.col);
        if (n.op == AssignOp::ASSIGN) {
            emit(Op::COPY, v, {}, var, n.line);
            lastValue_ = v;
        } else {
            Operand t = newTemp();
            emit(assignOpToIr(n.op), var, v, t, n.line);
            emit(Op::COPY, t, {}, var, n.line);
            lastValue_ = t;
        }
    } else if (n.target->kind == NodeKind::IndexExpr) {
        auto& idxE = static_cast<IndexExpr&>(*n.target);
        Operand base = resolveArrayBase(*idxE.base);
        Operand ix   = genExpr(*idxE.index);   // evaluated exactly once
        if (n.op == AssignOp::ASSIGN) {
            emit(Op::STORE_INDEX, ix, v, base, n.line);
            lastValue_ = v;
        } else {
            Operand oldT = newTemp();
            emit(Op::LOAD_INDEX, base, ix, oldT, n.line);
            Operand t = newTemp();
            emit(assignOpToIr(n.op), oldT, v, t, n.line);
            emit(Op::STORE_INDEX, ix, t, base, n.line);
            lastValue_ = t;
        }
    } else {
        lastValue_ = v;   // defensive: Sema already rejects this target shape
    }
}

void IRGen::visit(ConvertExpr& n) {
    Operand src = genExpr(*n.operand);
    Operand t = newTemp();
    Op op = (n.targetType.base == BaseType::FLOAT) ? Op::TO_FLOAT : Op::TO_INT;
    emit(op, src, {}, t, n.line);
    lastValue_ = t;
}

// ---- statements ----------------------------------------------------

void IRGen::visit(ExprStmt& n) {
    if (n.expr) genExpr(*n.expr);   // evaluated for side effect; value discarded
}

void IRGen::visit(CompoundStmt& n) {
    pushScope();
    for (auto& d : n.locals) d->accept(*this);
    for (auto& s : n.stmts)  s->accept(*this);
    popScope();
}

void IRGen::visit(IfStmt& n) {
    if (n.elseStmt) {
        Operand elseLabel = newLabel();
        Operand endLabel  = newLabel();
        genCondJumpFalse(*n.cond, elseLabel);
        n.thenStmt->accept(*this);
        emit(Op::GOTO, {}, {}, endLabel, n.line);
        emit(Op::LABEL, {}, {}, elseLabel, n.line);
        n.elseStmt->accept(*this);
        emit(Op::LABEL, {}, {}, endLabel, n.line);
    } else {
        Operand endLabel = newLabel();
        genCondJumpFalse(*n.cond, endLabel);
        n.thenStmt->accept(*this);
        emit(Op::LABEL, {}, {}, endLabel, n.line);
    }
}

void IRGen::visit(WhileStmt& n) {
    Operand startLabel = newLabel();
    Operand endLabel   = newLabel();
    emit(Op::LABEL, {}, {}, startLabel, n.line);
    genCondJumpFalse(*n.cond, endLabel);
    loopStack_.push_back({endLabel, startLabel});
    n.body->accept(*this);
    loopStack_.pop_back();
    emit(Op::GOTO, {}, {}, startLabel, n.line);
    emit(Op::LABEL, {}, {}, endLabel, n.line);
}

void IRGen::visit(DoStmt& n) {
    Operand startLabel = newLabel();
    Operand contLabel  = newLabel();
    Operand endLabel   = newLabel();
    emit(Op::LABEL, {}, {}, startLabel, n.line);
    loopStack_.push_back({endLabel, contLabel});
    n.body->accept(*this);
    loopStack_.pop_back();
    emit(Op::LABEL, {}, {}, contLabel, n.line);
    genCondJumpTrue(*n.cond, startLabel);
    emit(Op::LABEL, {}, {}, endLabel, n.line);
}

void IRGen::visit(ForStmt& n) {
    if (n.init) genExpr(*n.init);
    Operand startLabel = newLabel();
    Operand contLabel  = newLabel();
    Operand endLabel   = newLabel();
    emit(Op::LABEL, {}, {}, startLabel, n.line);
    if (n.cond) genCondJumpFalse(*n.cond, endLabel);
    loopStack_.push_back({endLabel, contLabel});
    n.body->accept(*this);
    loopStack_.pop_back();
    // continue lands here — before the step, matching C (continue still
    // runs the loop's increment before re-testing the condition).
    emit(Op::LABEL, {}, {}, contLabel, n.line);
    if (n.step) genExpr(*n.step);
    emit(Op::GOTO, {}, {}, startLabel, n.line);
    emit(Op::LABEL, {}, {}, endLabel, n.line);
}

void IRGen::visit(ReturnStmt& n) {
    if (n.value) {
        Operand v = genExpr(*n.value);
        emit(Op::RET, v, {}, {}, n.line);
    } else {
        emit(Op::RET, {}, {}, {}, n.line);
    }
}

void IRGen::visit(BreakStmt& n) {
    if (!loopStack_.empty()) emit(Op::GOTO, {}, {}, loopStack_.back().breakLabel, n.line);
    // Sema does not currently validate break/continue nesting (Step 3
    // scope); an empty loopStack_ here would mean a program no diagnostic
    // caught, so this silently no-ops rather than crashing generation.
}

void IRGen::visit(ContinueStmt& n) {
    if (!loopStack_.empty()) emit(Op::GOTO, {}, {}, loopStack_.back().continueLabel, n.line);
}

// ---- short-circuit conditional codegen ------------------------------

void IRGen::genCondJumpFalse(Expr& cond, const Operand& falseLabel) {
    if (cond.kind == NodeKind::BinaryExpr) {
        auto& be = static_cast<BinaryExpr&>(cond);
        if (be.op == BinOp::LOGAND) {
            genCondJumpFalse(*be.lhs, falseLabel);
            genCondJumpFalse(*be.rhs, falseLabel);
            return;
        }
        if (be.op == BinOp::LOGOR) {
            Operand trueLabel = newLabel();
            genCondJumpTrue(*be.lhs, trueLabel);
            genCondJumpFalse(*be.rhs, falseLabel);
            emit(Op::LABEL, {}, {}, trueLabel, cond.line);
            return;
        }
    }
    if (cond.kind == NodeKind::UnaryExpr) {
        auto& ue = static_cast<UnaryExpr&>(cond);
        if (ue.op == UnOp::NOT) {
            genCondJumpTrue(*ue.operand, falseLabel);
            return;
        }
    }
    Operand v = genExpr(cond);
    emit(Op::IF_FALSE, v, {}, falseLabel, cond.line);
}

void IRGen::genCondJumpTrue(Expr& cond, const Operand& trueLabel) {
    if (cond.kind == NodeKind::BinaryExpr) {
        auto& be = static_cast<BinaryExpr&>(cond);
        if (be.op == BinOp::LOGAND) {
            Operand falseLabel = newLabel();
            genCondJumpFalse(*be.lhs, falseLabel);
            genCondJumpTrue(*be.rhs, trueLabel);
            emit(Op::LABEL, {}, {}, falseLabel, cond.line);
            return;
        }
        if (be.op == BinOp::LOGOR) {
            genCondJumpTrue(*be.lhs, trueLabel);
            genCondJumpTrue(*be.rhs, trueLabel);
            return;
        }
    }
    if (cond.kind == NodeKind::UnaryExpr) {
        auto& ue = static_cast<UnaryExpr&>(cond);
        if (ue.op == UnOp::NOT) {
            genCondJumpFalse(*ue.operand, trueLabel);
            return;
        }
    }
    // No IF_TRUE opcode exists (ir.h only has IF_FALSE) — synthesize one
    // from IF_FALSE + GOTO. Three quads instead of one, but every leaf
    // condition test in the codebase goes through this or the IF_FALSE
    // branch above, so it stays in exactly one place.
    Operand v = genExpr(cond);
    Operand skip = newLabel();
    emit(Op::IF_FALSE, v, {}, skip, cond.line);
    emit(Op::GOTO, {}, {}, trueLabel, cond.line);
    emit(Op::LABEL, {}, {}, skip, cond.line);
}

// ---- declarations ----------------------------------------------------

void IRGen::genVarDeclInto(VarDecl& n, bool isGlobal) {
    std::string slotName = isGlobal ? n.name : qualify(n.name, n.storageOffset);
    Operand slot; slot.kind = OperandKind::VARIABLE; slot.name = slotName;

    if (n.type.isArray) {
        std::vector<Operand> vals;
        vals.reserve(n.initList.size());
        for (auto& e : n.initList) vals.push_back(genExpr(*e));   // old-scope resolution, see file header

        if (isGlobal) globalNames_.insert(n.name);
        else          declareLocal(n.name, n.storageOffset);

        int size = (n.type.arraySize >= 0) ? n.type.arraySize : static_cast<int>(vals.size());
        emit(Op::ALLOC_ARRAY, intConst(size), {}, slot, n.line);
        for (size_t i = 0; i < vals.size(); ++i)
            emit(Op::STORE_INDEX, intConst(static_cast<long long>(i)), vals[i], slot, n.line);
    } else {
        bool hasInit = static_cast<bool>(n.init);
        Operand val;
        if (hasInit) val = genExpr(*n.init);   // old-scope resolution, see file header

        if (isGlobal) globalNames_.insert(n.name);
        else          declareLocal(n.name, n.storageOffset);

        if (!hasInit) val = zeroOf(n.type);    // deterministic zero-init: no undefined behavior to
                                                // reproduce differentially between optimized/unoptimized runs
        emit(Op::COPY, val, {}, slot, n.line);
    }
}

void IRGen::visit(VarDecl& n) {
    genVarDeclInto(n, cur_ == nullptr);
}

void IRGen::genFunctionBody(FuncDecl& n) {
    IRFunction fn;
    fn.name = n.name;
    program_.functions.push_back(std::move(fn));
    cur_  = &program_.functions.back();
    code_ = &cur_->code;

    scopes_.clear();
    pushScope();   // parameter scope — mirrors Sema::visit(FuncDecl&)
    for (auto& p : n.params) {
        std::string q = declareLocal(p.name, p.storageOffset);
        cur_->params.push_back(q);
    }

    if (n.body) n.body->accept(*this);   // CompoundStmt pushes its own nested scope

    popScope();
    cur_  = nullptr;
    code_ = nullptr;
}

void IRGen::visit(FuncDecl& n) {
    if (!n.body) return;   // prototype-only declaration; not produced by this grammar/parser today
    genFunctionBody(n);
}

void IRGen::visit(Program& n) {
    // Pass 1: register every global name up front, so a function defined
    // earlier in the file can still resolve a global declared later —
    // same reasoning as Sema::registerSignatures, applied to globals.
    for (auto& d : n.decls)
        if (auto* vd = dynamic_cast<VarDecl*>(d.get())) globalNames_.insert(vd->name);

    // Pass 2: generate global initializers into program_.globalInit.
    cur_  = nullptr;
    code_ = &program_.globalInit;
    scopes_.clear();
    for (auto& d : n.decls)
        if (auto* vd = dynamic_cast<VarDecl*>(d.get())) genVarDeclInto(*vd, true);
    code_ = nullptr;

    // Pass 3: generate every function body.
    for (auto& d : n.decls)
        if (auto* fn = dynamic_cast<FuncDecl*>(d.get())) fn->accept(*this);
}

} // namespace minic
