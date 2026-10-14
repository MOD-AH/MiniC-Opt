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
        case BinOp::BITAND: return Op::BITAND; case BinOp::BITOR: return Op::BITOR;
        case BinOp::BITXOR: return Op::BITXOR; case BinOp::SHL:   return Op::SHL;
        case BinOp::SHR:    return Op::SHR;
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
        case AssignOp::PERCENT_ASSIGN: return Op::MOD;
        case AssignOp::AND_ASSIGN: return Op::BITAND;  case AssignOp::OR_ASSIGN:  return Op::BITOR;
        case AssignOp::XOR_ASSIGN: return Op::BITXOR;  case AssignOp::SHL_ASSIGN: return Op::SHL;
        case AssignOp::SHR_ASSIGN: return Op::SHR;
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
    boxed_.clear();
    structs_ = &prog.structs;
    userFns_.clear();
    for (auto& d : prog.decls)
        if (auto* fd = dynamic_cast<FuncDecl*>(d.get())) userFns_.insert(fd->name);
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
        Operand v = resolveName(id.name, id.line, id.col);
        if (!boxed_.count(v.name)) return v;   // a plain array or pointer variable
    }
    // Any other pointer-valued expression: `(p + 1)[0]`, `pp[i][j]`, a
    // boxed pointer variable (needs a LOAD first), a call returning a pointer.
    return genExpr(base);
}

Operand IRGen::zeroOf(const TypeSpec& t) {
    Operand o;
    if (t.base == BaseType::FLOAT && t.ptrDepth == 0) { o.kind = OperandKind::FLOAT_CONST; o.fval = 0.0; }
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
    Operand v = resolveName(n.name, n.line, n.col);
    if (boxed_.count(v.name)) {
        Operand t = newTemp();
        emit(Op::LOAD_INDEX, v, intConst(0), t, n.line);
        lastValue_ = t;
        return;
    }
    lastValue_ = v;
}

Operand IRGen::scaleIndex(Operand idx, int slotCount, int line) {
    if (slotCount == 1) return idx;
    if (idx.kind == OperandKind::INT_CONST) return intConst(idx.ival * slotCount);
    Operand t = newTemp();
    emit(Op::MUL, idx, intConst(slotCount), t, line);
    return t;
}

Operand IRGen::addConst(Operand idx, long long k, int line) {
    if (k == 0) return idx;
    if (idx.kind == OperandKind::INT_CONST) return intConst(idx.ival + k);
    Operand t = newTemp();
    emit(Op::ADD, idx, intConst(k), t, line);
    return t;
}

Operand IRGen::pointerFrom(const Addr& a, int line) {
    if (a.index.kind == OperandKind::INT_CONST && a.index.ival == 0) return a.base;
    Operand t = newTemp();
    emit(Op::ADD, a.base, a.index, t, line);
    return t;
}

Operand IRGen::copyOfAggregate(Operand srcPtr, const TypeSpec& t, int line) {
    int n = slots(t);
    Operand fresh = newTemp();
    emit(Op::ALLOC_ARRAY, intConst(n), {}, fresh, line);
    emit(Op::COPY_BLOCK, srcPtr, intConst(n), fresh, line);
    return fresh;
}

IRGen::Addr IRGen::genAddr(Expr& e) {
    switch (e.kind) {
        case NodeKind::IdentExpr: {
            auto& id = static_cast<IdentExpr&>(e);
            return Addr{resolveName(id.name, id.line, id.col), intConst(0)};
        }
        case NodeKind::IndexExpr: {
            auto& ie = static_cast<IndexExpr&>(e);
            Operand base = resolveArrayBase(*ie.base);
            Operand ix   = genExpr(*ie.index);           // evaluated exactly once
            return Addr{base, scaleIndex(ix, slots(ie.resolvedType), e.line)};
        }
        case NodeKind::MemberExpr: {
            auto& me = static_cast<MemberExpr&>(e);
            if (me.arrow) return Addr{genExpr(*me.base), intConst(me.slotOffset)};
            Addr inner = genAddr(*me.base);
            return Addr{inner.base, addConst(inner.index, me.slotOffset, e.line)};
        }
        case NodeKind::UnaryExpr: {
            auto& ue = static_cast<UnaryExpr&>(e);
            if (ue.op == UnOp::DEREF) return Addr{genExpr(*ue.operand), intConst(0)};
            break;
        }
        default: break;
    }
    // Not an lvalue (a call returning a struct, a parenthesised ?:): its
    // value is already a pointer to where the result lives.
    return Addr{genExpr(e), intConst(0)};
}

IRGen::LValue IRGen::genLValue(Expr& e) {
    LValue lv;
    if (e.kind == NodeKind::IdentExpr) {
        auto& id = static_cast<IdentExpr&>(e);
        Operand var = resolveName(id.name, id.line, id.col);
        if (boxed_.count(var.name)) { lv.isMem = true; lv.base = var; lv.index = intConst(0); }
        else                        { lv.var = var; }
        return lv;
    }
    Addr a = genAddr(e);
    lv.isMem = true; lv.base = a.base; lv.index = a.index;
    return lv;
}

void IRGen::visit(IndexExpr& n) {
    Addr a = genAddr(n);
    // A row of a 2-D array or a struct element is not loaded: the expression
    // IS the address, and a further [j] or .field continues from it.
    if (isAggregate(n.resolvedType)) { lastValue_ = pointerFrom(a, n.line); return; }
    Operand t = newTemp();
    emit(Op::LOAD_INDEX, a.base, a.index, t, n.line);
    lastValue_ = t;
}

void IRGen::visit(MemberExpr& n) {
    Addr a = genAddr(n);
    if (isAggregate(n.resolvedType)) { lastValue_ = pointerFrom(a, n.line); return; }
    Operand t = newTemp();
    emit(Op::LOAD_INDEX, a.base, a.index, t, n.line);
    lastValue_ = t;
}

void IRGen::visit(SizeofExpr& n) { lastValue_ = intConst(n.value); }

void IRGen::visit(CommaExpr& n) {
    genExpr(*n.lhs);                 // evaluated for its side effects only
    lastValue_ = genExpr(*n.rhs);
}

void IRGen::visit(CondExpr& n) {
    bool hasValue = !(n.resolvedType.base == BaseType::VOID && n.resolvedType.ptrDepth == 0 && !n.resolvedType.isArray);
    Operand result = hasValue ? newTemp() : Operand{};
    Operand elseLabel = newLabel();
    Operand endLabel  = newLabel();
    genCondJumpFalse(*n.cond, elseLabel);
    Operand a = genExpr(*n.thenExpr);
    if (hasValue) emit(Op::COPY, a, {}, result, n.line);
    emit(Op::GOTO, {}, {}, endLabel, n.line);
    emit(Op::LABEL, {}, {}, elseLabel, n.line);
    Operand b = genExpr(*n.elseExpr);
    if (hasValue) emit(Op::COPY, b, {}, result, n.line);
    emit(Op::LABEL, {}, {}, endLabel, n.line);
    lastValue_ = result;
}

void IRGen::visit(StringLit& n) {
    if (n.isFormat) {
        // The format of printf/scanf: read by the interpreter, never indexed.
        Operand o;
        o.kind = OperandKind::STRING_CONST;
        o.sval = n.value;
        lastValue_ = o;
        return;
    }
    // Anywhere else a string literal is a real `char *`: a fresh array of the
    // characters plus the terminating 0 (fresh storage is already zero, so the
    // terminator needs no store).
    Operand t = newTemp();
    emit(Op::ALLOC_ARRAY, intConst(static_cast<long long>(n.value.size()) + 1), {}, t, n.line);
    for (size_t i = 0; i < n.value.size(); ++i)
        if (n.value[i] != '\0')
            emit(Op::STORE_INDEX, intConst(static_cast<long long>(i)),
                 intConst(static_cast<unsigned char>(n.value[i])), t, n.line);
    lastValue_ = t;
}

void IRGen::visit(CallExpr& n) {
    // Arguments are evaluated AND queued (PARAM) one at a time, strictly in
    // order, rather than evaluate-all-then-queue-all: if an argument is
    // itself a call (f(g(x))), its own PARAM/CALL pair must be fully
    // emitted — and, at interpret time, fully consumed — before this
    // call's own PARAM quads resume. Left to right, that's exactly what
    // "evaluate this arg, immediately PARAM it, move to the next" gives.
    // memset/memcpy/memmove/memcmp count BYTES in C; the interpreter's memory
    // is in slots, one per element. Convert once here, where the pointer's
    // element type is known: n bytes of int is n/4 elements.
    bool byteCounted = !userFns_.count(n.callee) && n.args.size() == 3 &&
        (n.callee == "memset" || n.callee == "memcpy" || n.callee == "memmove" || n.callee == "memcmp");
    size_t argIdx = 0;
    for (auto& a : n.args) {
        Operand v = genExpr(*a);
        if (byteCounted && argIdx == 2) {
            TypeSpec pointee = pointeeOf(decayed(n.args[0]->resolvedType));
            bool known = !(pointee.base == BaseType::VOID && pointee.ptrDepth == 0);
            int bytesPer = known ? structs_->bytesOf(pointee) : 1, slotsPer = known ? slots(pointee) : 1;
            if (bytesPer > 1) {
                Operand q = newTemp();
                emit(Op::DIV, v, intConst(bytesPer), q, a->line);
                v = q;
            }
            if (slotsPer > 1) {
                Operand q = newTemp();
                emit(Op::MUL, v, intConst(slotsPer), q, a->line);
                v = q;
            }
        }
        ++argIdx;
        // A struct is passed BY VALUE: the callee gets its own copy, so its
        // writes never reach the caller's variable. (Arrays and pointers are
        // passed as the reference they already are.)
        if (isStructValue(a->resolvedType)) v = copyOfAggregate(v, a->resolvedType, a->line);
        emit(Op::PARAM, v, {}, {}, a->line);
    }
    Operand callee; callee.kind = OperandKind::LABEL_REF; callee.name = n.callee;
    Operand argc = intConst(static_cast<long long>(n.args.size()));
    Operand result;
    if (n.resolvedType.base != BaseType::VOID || n.resolvedType.ptrDepth > 0) result = newTemp();
    emit(Op::CALL, callee, argc, result, n.line);
    lastValue_ = result;
}

void IRGen::visit(UnaryExpr& n) {
    switch (n.op) {
        case UnOp::POS:
            lastValue_ = genExpr(*n.operand);   // kept as a distinct AST node for --dump-ast; no-op in TAC
            return;
        case UnOp::BITNOT: {
            Operand v = genExpr(*n.operand);
            Operand t = newTemp();
            emit(Op::BITNOT, v, {}, t, n.line);
            lastValue_ = t;
            return;
        }
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
        case UnOp::ADDR: {
            // &x on a boxed scalar or an array/struct variable is the name
            // itself; &a[i], &s.f, &p->f and &*p are base + offset.
            Addr a = genAddr(*n.operand);
            lastValue_ = pointerFrom(a, n.line);
            return;
        }
        case UnOp::DEREF: {
            Addr a = genAddr(n);
            if (isAggregate(n.resolvedType)) { lastValue_ = pointerFrom(a, n.line); return; }
            Operand t = newTemp();
            emit(Op::LOAD_INDEX, a.base, a.index, t, n.line);
            lastValue_ = t;
            return;
        }
        case UnOp::PRE_INC: case UnOp::PRE_DEC:
        case UnOp::POST_INC: case UnOp::POST_DEC: {
            bool isInc = (n.op == UnOp::PRE_INC || n.op == UnOp::POST_INC);
            bool isPre = (n.op == UnOp::PRE_INC || n.op == UnOp::PRE_DEC);
            Op deltaOp = isInc ? Op::ADD : Op::SUB;
            // A pointer steps by one element; a number steps by one.
            const TypeSpec& ot = n.operand->resolvedType;
            Operand one = intConst(isPointerType(ot) ? slots(pointeeOf(ot)) : 1);

            // The target is located once (an index expression with a side
            // effect, a[f()]++, must not run f twice) and then read and
            // written through the same LValue.
            LValue lv = genLValue(*n.operand);
            Operand oldT = newTemp();
            if (lv.isMem) emit(Op::LOAD_INDEX, lv.base, lv.index, oldT, n.line);
            else          emit(Op::COPY, lv.var, {}, oldT, n.line);
            Operand newT = newTemp();
            emit(deltaOp, oldT, one, newT, n.line);
            if (lv.isMem) emit(Op::STORE_INDEX, lv.index, newT, lv.base, n.line);
            else          emit(Op::COPY, newT, {}, lv.var, n.line);
            lastValue_ = isPre ? newT : oldT;
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
    bool lp = isPointerLike(n.lhs->resolvedType), rp = isPointerLike(n.rhs->resolvedType);
    if ((n.op == BinOp::ADD || n.op == BinOp::SUB) && (lp || rp)) {
        // Pointer arithmetic: the integer side counts ELEMENTS, the
        // interpreter's pointers count slots, so scale by the element size.
        // A pointer difference is the reverse: slots -> elements.
        int elem = slots(pointeeOf(decayed(lp ? n.lhs->resolvedType : n.rhs->resolvedType)));
        if (lp && rp) {
            Operand t = newTemp();
            emit(Op::SUB, l, r, t, n.line);
            if (elem != 1) { Operand q = newTemp(); emit(Op::DIV, t, intConst(elem), q, n.line); t = q; }
            lastValue_ = t;
            return;
        }
        if (lp) r = scaleIndex(r, elem, n.line);
        else    l = scaleIndex(l, elem, n.line);
    }
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

    // For a compound op, Sema deliberately leaves n.value at its own
    // natural type rather than pre-narrowing it to the target's type
    // (see Sema::visit(AssignExpr&)) — the operation itself must run at
    // whatever type the usual arithmetic conversions give x op y, and
    // only the RESULT narrows back to the target's type. The interpreter
    // already picks FLOAT vs INT per-operation from each Value's actual
    // kind, so leaving v untouched makes the op itself correct; this
    // helper is the "narrow the result back down" half of that — the
    // one spot it's actually needed is int/char target, float value.
    auto narrowResultIfNeeded = [&](Operand t) {
        const TypeSpec& tt = n.target->resolvedType;
        const TypeSpec& vt = n.value->resolvedType;
        bool targetIsFloat = (tt.base == BaseType::FLOAT && tt.ptrDepth == 0);
        bool valueIsFloat  = (vt.base == BaseType::FLOAT && vt.ptrDepth == 0);
        if (!targetIsFloat && valueIsFloat) {
            Operand narrowed = newTemp();
            emit(Op::TO_INT, t, {}, narrowed, n.line);
            return narrowed;
        }
        return t;
    };

    if (isStructValue(n.target->resolvedType)) {
        // Whole-struct assignment copies every slot of the source over the target.
        Addr a = genAddr(*n.target);
        Operand dst = pointerFrom(a, n.line);
        emit(Op::COPY_BLOCK, v, intConst(slots(n.target->resolvedType)), dst, n.line);
        lastValue_ = dst;
        return;
    }
    LValue lv = genLValue(*n.target);
    // `p += n` / `p -= n` count elements, like p + n.
    if (n.op != AssignOp::ASSIGN && isPointerType(n.target->resolvedType))
        v = scaleIndex(v, slots(pointeeOf(n.target->resolvedType)), n.line);

    if (!lv.isMem) {
        if (n.op == AssignOp::ASSIGN) {
            emit(Op::COPY, v, {}, lv.var, n.line);
            lastValue_ = v;
        } else {
            Operand t = newTemp();
            emit(assignOpToIr(n.op), lv.var, v, t, n.line);
            t = narrowResultIfNeeded(t);
            emit(Op::COPY, t, {}, lv.var, n.line);
            lastValue_ = t;
        }
    } else if (n.op == AssignOp::ASSIGN) {
        emit(Op::STORE_INDEX, lv.index, v, lv.base, n.line);
        lastValue_ = v;
    } else {
        Operand oldT = newTemp();
        emit(Op::LOAD_INDEX, lv.base, lv.index, oldT, n.line);
        Operand t = newTemp();
        emit(assignOpToIr(n.op), oldT, v, t, n.line);
        t = narrowResultIfNeeded(t);
        emit(Op::STORE_INDEX, lv.index, t, lv.base, n.line);
        lastValue_ = t;
    }
}

void IRGen::visit(CastExpr& n) {
    Operand v = genExpr(*n.operand);
    const TypeSpec& from = n.operand->resolvedType;
    const TypeSpec& to   = n.target;
    // Only a number<->number cast changes the value. Anything touching a
    // pointer (or void) just re-labels the same bits for the type checker.
    if (isPointerLike(from) || to.ptrDepth > 0 || to.base == BaseType::VOID) { lastValue_ = v; return; }
    bool fromFloat = (from.base == BaseType::FLOAT), toFloat = (to.base == BaseType::FLOAT);
    if (fromFloat == toFloat) { lastValue_ = v; return; }
    Operand t = newTemp();
    emit(toFloat ? Op::TO_FLOAT : Op::TO_INT, v, {}, t, n.line);
    lastValue_ = t;
}

void IRGen::visit(ConvertExpr& n) {
    Operand src = genExpr(*n.operand);
    Operand t = newTemp();
    Op op = (n.targetType.base == BaseType::FLOAT) ? Op::TO_FLOAT : Op::TO_INT;
    emit(op, src, {}, t, n.line);
    lastValue_ = t;
}

// ---- statements ----------------------------------------------------

void IRGen::visit(DeclStmt& n) { n.decl->accept(*this); }

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
        if (isStructValue(n.value->resolvedType)) v = copyOfAggregate(v, n.value->resolvedType, n.line);
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
    if (!loopStack_.empty() && !loopStack_.back().continueLabel.isNone())
        emit(Op::GOTO, {}, {}, loopStack_.back().continueLabel, n.line);
}

void IRGen::visit(SwitchStmt& n) {
    Operand v = genExpr(*n.cond);
    Operand endLabel = newLabel();
    std::vector<Operand> caseLabels;
    int defaultIdx = -1;
    for (size_t i = 0; i < n.cases.size(); ++i) {
        caseLabels.push_back(newLabel());
        if (n.cases[i].isDefault) defaultIdx = static_cast<int>(i);
    }
    // The dispatch: one test per case, in order. `t = v != K` is false exactly
    // when v == K, so IF_FALSE jumps straight to the case with no extra goto.
    for (size_t i = 0; i < n.cases.size(); ++i) {
        if (n.cases[i].isDefault) continue;
        Operand t = newTemp();
        emit(Op::NE, v, intConst(n.cases[i].value), t, n.cases[i].line);
        emit(Op::IF_FALSE, t, {}, caseLabels[i], n.cases[i].line);
    }
    emit(Op::GOTO, {}, {}, defaultIdx >= 0 ? caseLabels[defaultIdx] : endLabel, n.line);

    // `break` leaves the switch; `continue` still belongs to the enclosing loop.
    Operand outerContinue = loopStack_.empty() ? Operand{} : loopStack_.back().continueLabel;
    loopStack_.push_back({endLabel, outerContinue});
    pushScope();
    for (size_t i = 0; i < n.cases.size(); ++i) {
        emit(Op::LABEL, {}, {}, caseLabels[i], n.cases[i].line);   // falls through into the next case
        for (auto& s : n.cases[i].stmts) s->accept(*this);
    }
    popScope();
    loopStack_.pop_back();
    emit(Op::LABEL, {}, {}, endLabel, n.line);
}

namespace {
Operand userLabel(const std::string& name) {
    Operand o; o.kind = OperandKind::LABEL_REF; o.name = "U_" + name; return o;
}
}

void IRGen::visit(GotoStmt& n) { emit(Op::GOTO, {}, {}, userLabel(n.label), n.line); }

void IRGen::visit(LabelStmt& n) {
    emit(Op::LABEL, {}, {}, userLabel(n.label), n.line);
    n.stmt->accept(*this);
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

    bool aggregate = isStructValue(n.type)
        || (n.type.isArray && (!n.type.extraDims.empty()
                               || (n.type.base == BaseType::STRUCT && n.type.ptrDepth == 0)));
    if (aggregate) {
        // A struct, an array of structs or a multi-dimensional array: one flat
        // run of slots. Values are evaluated before the name is declared (the
        // same old-scope rule as everywhere), then stored slot by slot.
        Operand srcPtr;
        bool copyInit = static_cast<bool>(n.init);
        if (copyInit) srcPtr = genExpr(*n.init);
        std::vector<Operand> vals;
        vals.reserve(n.initList.size());
        for (auto& e : n.initList) vals.push_back(genExpr(*e));

        if (isGlobal) globalNames_.insert(n.name);
        else          declareLocal(n.name, n.storageOffset);

        int total = slots(n.type);
        emit(Op::ALLOC_ARRAY, intConst(total), {}, slot, n.line);
        if (copyInit) emit(Op::COPY_BLOCK, srcPtr, intConst(total), slot, n.line);
        for (size_t i = 0; i < vals.size(); ++i) {
            // Fresh storage is already zero, so zero entries (the padding the
            // parser adds) cost nothing.
            bool zero = (vals[i].kind == OperandKind::INT_CONST && vals[i].ival == 0)
                     || (vals[i].kind == OperandKind::FLOAT_CONST && vals[i].fval == 0.0);
            if (!zero) emit(Op::STORE_INDEX, intConst(static_cast<long long>(i)), vals[i], slot, n.line);
        }
    } else if (n.type.isArray) {
        std::vector<Operand> vals;
        vals.reserve(n.initList.size());
        for (auto& e : n.initList) vals.push_back(genExpr(*e));   // old-scope resolution, see file header

        if (isGlobal) globalNames_.insert(n.name);
        else          declareLocal(n.name, n.storageOffset);

        int size = (n.type.arraySize >= 0) ? n.type.arraySize : static_cast<int>(vals.size());
        emit(Op::ALLOC_ARRAY, intConst(size), {}, slot, n.line);
        for (size_t i = 0; i < vals.size(); ++i)
            emit(Op::STORE_INDEX, intConst(static_cast<long long>(i)), vals[i], slot, n.line);
        // `int a[5] = {1, 2};` is legal MiniC (Sema only rejects too MANY
        // initializers, never too few) and C requires the remaining
        // elements to be zero-initialized. ALLOC_ARRAY itself makes no
        // such promise — it just reserves N slots — so without this loop
        // indices [vals.size(), size) were left however the interpreter's
        // array allocation happens to leave them, which is deterministic
        // today (std::vector value-init already zeroes them) but was
        // never an explicit, IR-level guarantee the way every other
        // "no undefined behavior to reproduce differentially" spot in
        // this function is.
        for (int i = static_cast<int>(vals.size()); i < size; ++i)
            emit(Op::STORE_INDEX, intConst(i), zeroOf(TypeSpec{n.type.base, false, -1, n.type.ptrDepth}), slot, n.line);
    } else {
        bool hasInit = static_cast<bool>(n.init);
        Operand val;
        if (hasInit) val = genExpr(*n.init);   // old-scope resolution, see file header

        if (isGlobal) globalNames_.insert(n.name);
        else          declareLocal(n.name, n.storageOffset);

        if (!hasInit) val = zeroOf(n.type);    // deterministic zero-init: no undefined behavior to
                                                // reproduce differentially between optimized/unoptimized runs
        if (n.addressTaken) {
            boxed_.insert(slotName);
            emit(Op::ALLOC_ARRAY, intConst(1), {}, slot, n.line);
            emit(Op::STORE_INDEX, intConst(0), val, slot, n.line);
        } else {
            emit(Op::COPY, val, {}, slot, n.line);
        }
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
        if (p.addressTaken && !p.type.isArray) {
            // The caller's value arrives under a throwaway name and is moved
            // into a one-slot box that `&p` can point at.
            std::string incoming = qualify(p.name + "@in", p.storageOffset);
            cur_->params.push_back(incoming);
            Operand box;  box.kind = OperandKind::VARIABLE; box.name = q;
            Operand src;  src.kind = OperandKind::VARIABLE; src.name = incoming;
            boxed_.insert(q);
            emit(Op::ALLOC_ARRAY, intConst(1), {}, box, n.line);
            emit(Op::STORE_INDEX, intConst(0), src, box, n.line);
        } else {
            cur_->params.push_back(q);
        }
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
