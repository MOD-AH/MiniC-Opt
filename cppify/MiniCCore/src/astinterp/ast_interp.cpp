// Cppify — AST reference interpreter (see include/ast_interp.h)
#include "ast_interp.h"
#include "int_semantics.h"
#include <algorithm>

namespace minic {
namespace {

using RuntimeError = Interp::RuntimeError;

IntTy intTyOf(const TypeSpec& t) {
    if (!isIntegerType(t)) return IntTy::NONE;
    switch (intWidth(t)) {
        case 8:  return t.isUnsigned ? IntTy::U8  : IntTy::I8;
        case 16: return t.isUnsigned ? IntTy::U16 : IntTy::I16;
        case 32: return t.isUnsigned ? IntTy::U32 : IntTy::I32;
        default: return t.isUnsigned ? IntTy::U64 : IntTy::I64;
    }
}

bool isAggregate(const TypeSpec& t) { return t.isArray || isStructValue(t); }
bool isVoidValue(const TypeSpec& t) { return t.base == BaseType::VOID && t.ptrDepth == 0 && !t.isArray; }
bool isFloat(const TypeSpec& t) { return t.base == BaseType::FLOAT && t.ptrDepth == 0 && !t.isArray; }

Value zeroOf(const TypeSpec& t) { return isFloat(t) ? Value::makeFloat(0.0) : Value::makeInt(0); }

Op binOp(BinOp op) {
    switch (op) {
        case BinOp::ADD: return Op::ADD;      case BinOp::SUB: return Op::SUB;
        case BinOp::MUL: return Op::MUL;      case BinOp::DIV: return Op::DIV;
        case BinOp::MOD: return Op::MOD;      case BinOp::LT:  return Op::LT;
        case BinOp::LE:  return Op::LE;       case BinOp::GT:  return Op::GT;
        case BinOp::GE:  return Op::GE;       case BinOp::EQ:  return Op::EQ;
        case BinOp::NE:  return Op::NE;       case BinOp::BITAND: return Op::BITAND;
        case BinOp::BITOR: return Op::BITOR;  case BinOp::BITXOR: return Op::BITXOR;
        case BinOp::SHL: return Op::SHL;      case BinOp::SHR: return Op::SHR;
        default:         return Op::NOP;      // && and || short-circuit; never get here
    }
}

Op compoundOp(AssignOp op) {
    switch (op) {
        case AssignOp::PLUS_ASSIGN:    return Op::ADD;    case AssignOp::MINUS_ASSIGN: return Op::SUB;
        case AssignOp::STAR_ASSIGN:    return Op::MUL;    case AssignOp::SLASH_ASSIGN: return Op::DIV;
        case AssignOp::PERCENT_ASSIGN: return Op::MOD;    case AssignOp::AND_ASSIGN:   return Op::BITAND;
        case AssignOp::OR_ASSIGN:      return Op::BITOR;  case AssignOp::XOR_ASSIGN:   return Op::BITXOR;
        case AssignOp::SHL_ASSIGN:     return Op::SHL;    case AssignOp::SHR_ASSIGN:   return Op::SHR;
        default:                       return Op::NOP;
    }
}

// The element of the object `base` points into, `index` slots on.
std::vector<Value>::size_type slotIndex(const Value& base, long long index) {
    if (!base.arr) {
        if (base.kind == Value::Kind::INT && base.ival == 0) throw RuntimeError{"null pointer dereference"};
        throw RuntimeError{"internal error: indexing a non-pointer value"};
    }
    long long at = base.off + index;
    if (at < 0 || static_cast<size_t>(at) >= base.arr->size()) throw RuntimeError{"array index out of bounds"};
    return static_cast<size_t>(at);
}

Value makePointer(std::shared_ptr<std::vector<Value>> s) { return Value::makeArray(std::move(s)); }

}  // namespace

// ---- program ------------------------------------------------------------

Value AstInterp::run(Program& prog) {
    structs_ = &prog.structs;
    lib_.resetLibrary();
    for (auto& d : prog.decls)
        if (auto* f = dynamic_cast<FuncDecl*>(d.get())) {
            funcs_[f->name] = f;
            if (f->body) collectLabels(f->body.get());
        }
    try {
        // Globals in source order, then static locals in the order their
        // functions appear: the order C's static initialization happens in
        // (their initializers are constants, so only this order is visible).
        for (auto& d : prog.decls)
            if (auto* v = dynamic_cast<VarDecl*>(d.get())) globals_[v->name] = declare(*v, true);
        std::vector<Stmt*> work;
        for (auto& d : prog.decls)
            if (auto* f = dynamic_cast<FuncDecl*>(d.get()); f && f->body) {
                work.assign(1, f->body.get());
                while (!work.empty()) {
                    Stmt* s = work.back();
                    work.pop_back();
                    switch (s->kind) {
                        case NodeKind::CompoundStmt: {
                            auto& c = static_cast<CompoundStmt&>(*s);
                            for (auto& l : c.locals) if (l->isStatic) statics_[l.get()] = declare(*l, true);
                            for (auto it = c.stmts.rbegin(); it != c.stmts.rend(); ++it) work.push_back(it->get());
                            break;
                        }
                        case NodeKind::DeclStmt: {
                            auto& v = *static_cast<DeclStmt&>(*s).decl;
                            if (v.isStatic) statics_[&v] = declare(v, true);
                            break;
                        }
                        case NodeKind::IfStmt: {
                            auto& i = static_cast<IfStmt&>(*s);
                            if (i.elseStmt) work.push_back(i.elseStmt.get());
                            work.push_back(i.thenStmt.get());
                            break;
                        }
                        case NodeKind::WhileStmt: work.push_back(static_cast<WhileStmt&>(*s).body.get()); break;
                        case NodeKind::ForStmt:   work.push_back(static_cast<ForStmt&>(*s).body.get()); break;
                        case NodeKind::DoStmt:    work.push_back(static_cast<DoStmt&>(*s).body.get()); break;
                        case NodeKind::LabelStmt: work.push_back(static_cast<LabelStmt&>(*s).stmt.get()); break;
                        case NodeKind::SwitchStmt: {
                            auto& sw = static_cast<SwitchStmt&>(*s);
                            for (auto c = sw.cases.rbegin(); c != sw.cases.rend(); ++c)
                                for (auto it = c->stmts.rbegin(); it != c->stmts.rend(); ++it) work.push_back(it->get());
                            break;
                        }
                        default: break;
                    }
                }
            }
        auto m = funcs_.find("main");
        if (m == funcs_.end()) throw RuntimeError{"no function named 'main'"};
        return callFunction(*m->second, {});
    } catch (const Interp::ProgramExit& e) {
        return Value::makeInt(e.code);
    }
}

Value AstInterp::callFunction(FuncDecl& f, std::vector<Value> args) {
    std::vector<std::unordered_map<std::string, Binding>> saved;
    saved.swap(scopes_);
    scopes_.emplace_back();
    for (size_t i = 0; i < f.params.size(); ++i) {
        Binding b;
        b.store = std::make_shared<std::vector<Value>>(1, i < args.size() ? args[i] : Value::makeInt(0));
        scopes_.back()[f.params[i].name] = b;
    }
    Flow fl = f.body ? execCompound(*f.body) : Flow::NORMAL;
    Value result = fl == Flow::RETURN ? retVal_ : Value::makeInt(0);
    if (fl == Flow::GOTO) throw RuntimeError{"internal error: goto to unknown label '" + gotoLabel_ + "'"};
    scopes_.swap(saved);
    return result;
}

AstInterp::Binding* AstInterp::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    auto g = globals_.find(name);
    return g == globals_.end() ? nullptr : &g->second;
}

// Initializers are evaluated before the name is declared, so `int x = x;`
// reads an outer x, as C requires.
AstInterp::Binding AstInterp::declare(VarDecl& d, bool runInit) {
    const TypeSpec& t = d.type;
    Binding b;
    if (t.isArray || isStructValue(t)) {
        Value src;
        bool copyInit = runInit && d.init;
        if (copyInit) src = eval(*d.init);
        std::vector<Value> vals;
        if (runInit) for (auto& e : d.initList) vals.push_back(eval(*e));
        // An array of scalars sized by its initializer takes the list's
        // length; everything else is laid out by the struct table.
        bool scalarArray = t.isArray && t.extraDims.empty() && !(t.base == BaseType::STRUCT && t.ptrDepth == 0);
        long long n = scalarArray ? (t.arraySize >= 0 ? t.arraySize : static_cast<long long>(d.initList.size()))
                                  : slots(t);
        b.store = std::make_shared<std::vector<Value>>(static_cast<size_t>(n), Value::makeInt(0));
        b.aggregate = true;
        if (copyInit) copyBlock(src, makePointer(b.store), n);
        for (size_t i = 0; i < vals.size(); ++i) store(Addr{makePointer(b.store), static_cast<long long>(i)}, vals[i]);
        // C zero-fills what a short initializer list leaves out; a float
        // array's zeros are floats.
        if (scalarArray) {
            TypeSpec elem{t.base, false, -1, t.ptrDepth};
            for (long long i = static_cast<long long>(vals.size()); i < n; ++i)
                (*b.store)[static_cast<size_t>(i)] = zeroOf(elem);
        }
        return b;
    }
    Value v = (runInit && d.init) ? eval(*d.init) : zeroOf(t);
    b.store = std::make_shared<std::vector<Value>>(1, v);
    return b;
}

// ---- memory -----------------------------------------------------------------

Value AstInterp::load(const Addr& a) { return (*a.base.arr)[slotIndex(a.base, a.index)]; }
void AstInterp::store(const Addr& a, const Value& v) { (*a.base.arr)[slotIndex(a.base, a.index)] = v; }

Value AstInterp::pointerFrom(const Addr& a) {
    if (a.index == 0) return a.base;
    return Interp::applyBinary(Op::ADD, a.base, Value::makeInt(a.index));
}

void AstInterp::copyBlock(const Value& src, const Value& dst, long long n) {
    if (!dst.arr || !src.arr) {
        bool nullish = (!dst.arr && dst.kind == Value::Kind::INT && dst.ival == 0) ||
                       (!src.arr && src.kind == Value::Kind::INT && src.ival == 0);
        throw RuntimeError{nullish ? "null pointer dereference" : "internal error: copying a non-pointer value"};
    }
    if (n <= 0) return;
    if (src.off < 0 || static_cast<size_t>(src.off + n) > src.arr->size() ||
        dst.off < 0 || static_cast<size_t>(dst.off + n) > dst.arr->size())
        throw RuntimeError{"array index out of bounds"};
    std::vector<Value> tmp(src.arr->begin() + src.off, src.arr->begin() + src.off + n);
    std::copy(tmp.begin(), tmp.end(), dst.arr->begin() + dst.off);
}

// A struct passed or returned by value: the receiver gets its own slots.
Value AstInterp::copyOf(const Value& src, const TypeSpec& t) {
    long long n = slots(t);
    Value fresh = makePointer(std::make_shared<std::vector<Value>>(static_cast<size_t>(n), Value::makeInt(0)));
    copyBlock(src, fresh, n);
    return fresh;
}

// C's conversion between arithmetic types. A pointer is relabelled, never
// converted; an integer narrows by wrapping to the target's width.
Value AstInterp::convert(const Value& v, const TypeSpec& from, const TypeSpec& to) {
    if (isPointerLike(from) || to.ptrDepth > 0 || to.base == BaseType::VOID) return v;
    bool fromF = from.base == BaseType::FLOAT, toF = to.base == BaseType::FLOAT;
    if (fromF == toF && (toF || sameIntType(from, to))) return v;
    if (toF) return Value::makeFloat(v.kind == Value::Kind::FLOAT ? v.fval : static_cast<double>(v.ival));
    return Value::makeInt(wrapInt(v.kind == Value::Kind::FLOAT ? static_cast<long long>(v.fval) : v.ival, intTyOf(to)));
}

// ---- expressions --------------------------------------------------------

AstInterp::Addr AstInterp::addr(Expr& e) {
    switch (e.kind) {
        case NodeKind::IdentExpr: {
            // An array or struct name already is a pointer to its slots (or,
            // for a parameter, holds one); a scalar's cell is its own slot.
            if (isAggregate(e.resolvedType)) return Addr{eval(e), 0};
            Binding* b = lookup(static_cast<IdentExpr&>(e).name);
            if (!b) throw RuntimeError{"internal error: unbound name '" + static_cast<IdentExpr&>(e).name + "'"};
            return Addr{makePointer(b->store), 0};
        }
        case NodeKind::IndexExpr: {
            auto& ie = static_cast<IndexExpr&>(e);
            Value base = eval(*ie.base);
            Value ix = eval(*ie.index);
            return Addr{base, ix.ival * slots(ie.resolvedType)};
        }
        case NodeKind::MemberExpr: {
            auto& me = static_cast<MemberExpr&>(e);
            if (me.arrow) return Addr{eval(*me.base), me.slotOffset};
            Addr inner = addr(*me.base);
            return Addr{inner.base, inner.index + me.slotOffset};
        }
        case NodeKind::UnaryExpr: {
            auto& ue = static_cast<UnaryExpr&>(e);
            if (ue.op == UnOp::DEREF) return Addr{eval(*ue.operand), 0};
            break;
        }
        default: break;
    }
    // Not an lvalue (a call returning a struct, a ?: of structs): its value
    // is already a pointer to where the result lives.
    return Addr{eval(e), 0};
}

Value AstInterp::eval(Expr& e) {
    switch (e.kind) {
        case NodeKind::IntLit:   return Value::makeInt(static_cast<IntLit&>(e).value);
        case NodeKind::FloatLit: return Value::makeFloat(static_cast<FloatLit&>(e).value);
        case NodeKind::CharLit:
            return Value::makeInt(static_cast<signed char>(static_cast<CharLit&>(e).value));   // char is signed
        case NodeKind::StringLit: {
            auto& s = static_cast<StringLit&>(e);
            if (s.isFormat) return Value::makeString(s.value);
            auto chars = std::make_shared<std::vector<Value>>(s.value.size() + 1, Value::makeInt(0));
            for (size_t i = 0; i < s.value.size(); ++i)
                (*chars)[i] = Value::makeInt(static_cast<signed char>(s.value[i]));
            return makePointer(chars);
        }
        case NodeKind::IdentExpr: {
            auto& id = static_cast<IdentExpr&>(e);
            Binding* b = lookup(id.name);
            if (!b) throw RuntimeError{"internal error: unbound name '" + id.name + "'"};
            return b->aggregate ? makePointer(b->store) : (*b->store)[0];
        }
        case NodeKind::IndexExpr:
        case NodeKind::MemberExpr: {
            Addr a = addr(e);
            return isAggregate(e.resolvedType) ? pointerFrom(a) : load(a);
        }
        case NodeKind::CallExpr:   return evalCall(static_cast<CallExpr&>(e));
        case NodeKind::UnaryExpr:  return evalUnary(static_cast<UnaryExpr&>(e));
        case NodeKind::BinaryExpr: return evalBinary(static_cast<BinaryExpr&>(e));
        case NodeKind::AssignExpr: return evalAssign(static_cast<AssignExpr&>(e));
        case NodeKind::CastExpr: {
            auto& c = static_cast<CastExpr&>(e);
            return convert(eval(*c.operand), c.operand->resolvedType, c.target);
        }
        case NodeKind::ConvertExpr: {
            // Sema inserts one only where the types differ, so it always converts.
            auto& c = static_cast<ConvertExpr&>(e);
            Value v = eval(*c.operand);
            if (c.targetType.base == BaseType::FLOAT)
                return Value::makeFloat(v.kind == Value::Kind::FLOAT ? v.fval : static_cast<double>(v.ival));
            return Value::makeInt(wrapInt(v.kind == Value::Kind::FLOAT ? static_cast<long long>(v.fval) : v.ival,
                                          intTyOf(c.targetType)));
        }
        case NodeKind::SizeofExpr: return Value::makeInt(static_cast<SizeofExpr&>(e).value);   // operand not evaluated
        case NodeKind::CondExpr: {
            auto& c = static_cast<CondExpr&>(e);
            Value v = eval(*c.cond).truthy() ? eval(*c.thenExpr) : eval(*c.elseExpr);
            return isVoidValue(c.resolvedType) ? Value::makeInt(0) : v;
        }
        case NodeKind::CommaExpr: {
            auto& c = static_cast<CommaExpr&>(e);
            eval(*c.lhs);
            return eval(*c.rhs);
        }
        default:
            throw RuntimeError{"internal error: not an expression"};
    }
}

Value AstInterp::evalCall(CallExpr& c) {
    // memset/memcpy/memmove/memcmp count bytes in C; memory here is counted
    // in slots, one per element, so the count is converted using the
    // pointer's element type.
    bool byteCounted = !funcs_.count(c.callee) && c.args.size() == 3 &&
        (c.callee == "memset" || c.callee == "memcpy" || c.callee == "memmove" || c.callee == "memcmp");
    std::vector<Value> args;
    for (size_t i = 0; i < c.args.size(); ++i) {
        Expr& a = *c.args[i];
        Value v = eval(a);
        if (byteCounted && i == 2) {
            TypeSpec pointee = pointeeOf(decayed(c.args[0]->resolvedType));
            bool known = !(pointee.base == BaseType::VOID && pointee.ptrDepth == 0);
            int bytesPer = known ? structs_->bytesOf(pointee) : 1, slotsPer = known ? slots(pointee) : 1;
            if (bytesPer > 1) v = Interp::applyBinary(Op::DIV, v, Value::makeInt(bytesPer));
            if (slotsPer > 1) v = Interp::applyBinary(Op::MUL, v, Value::makeInt(slotsPer));
        }
        if (isStructValue(a.resolvedType)) v = copyOf(v, a.resolvedType);   // by value
        args.push_back(v);
    }
    auto f = funcs_.find(c.callee);
    if (f != funcs_.end()) return callFunction(*f->second, std::move(args));
    return lib_.callBuiltin(c.callee, args);
}

Value AstInterp::evalUnary(UnaryExpr& u) {
    const TypeSpec& ot = u.operand->resolvedType;
    switch (u.op) {
        case UnOp::POS: return eval(*u.operand);
        case UnOp::NEG: {
            Value v = convert(eval(*u.operand), ot, u.resolvedType);
            if (v.kind == Value::Kind::FLOAT) return Value::makeFloat(-v.fval);
            return Value::makeInt(wrapInt(static_cast<long long>(0 - static_cast<unsigned long long>(v.ival)),
                                          intTyOf(u.resolvedType)));
        }
        case UnOp::BITNOT: {
            Value v = convert(eval(*u.operand), ot, u.resolvedType);
            return Value::makeInt(wrapInt(~v.ival, intTyOf(u.resolvedType)));
        }
        case UnOp::NOT: return Value::makeInt(eval(*u.operand).truthy() ? 0 : 1);
        case UnOp::ADDR: return pointerFrom(addr(*u.operand));
        case UnOp::DEREF: {
            Addr a = addr(u);
            return isAggregate(u.resolvedType) ? pointerFrom(a) : load(a);
        }
        case UnOp::PRE_INC: case UnOp::PRE_DEC: case UnOp::POST_INC: case UnOp::POST_DEC: {
            bool inc = u.op == UnOp::PRE_INC || u.op == UnOp::POST_INC;
            bool pre = u.op == UnOp::PRE_INC || u.op == UnOp::PRE_DEC;
            // A pointer steps by one element, and x++ on a char or short is
            // x = (T)(x + 1): the add wraps to x's own type.
            Value one = Value::makeInt(isPointerType(ot) ? slots(pointeeOf(ot)) : 1);
            Addr where = addr(*u.operand);   // located once: a[f()]++ calls f once
            Value old = load(where);
            Value now = Interp::applyBinary(inc ? Op::ADD : Op::SUB, old, one, intTyOf(ot));
            store(where, now);
            return pre ? now : old;
        }
    }
    return Value::makeInt(0);
}

Value AstInterp::evalBinary(BinaryExpr& b) {
    if (b.op == BinOp::LOGAND) return Value::makeInt(eval(*b.lhs).truthy() && eval(*b.rhs).truthy() ? 1 : 0);
    if (b.op == BinOp::LOGOR)  return Value::makeInt(eval(*b.lhs).truthy() || eval(*b.rhs).truthy() ? 1 : 0);
    Value l = eval(*b.lhs), r = eval(*b.rhs);
    const TypeSpec& lt = b.lhs->resolvedType;
    bool lp = isPointerLike(lt), rp = isPointerLike(b.rhs->resolvedType);
    if ((b.op == BinOp::ADD || b.op == BinOp::SUB) && (lp || rp)) {
        // The integer side counts elements; a pointer counts slots.
        long long elem = slots(pointeeOf(decayed(lp ? lt : b.rhs->resolvedType)));
        if (lp && rp) {
            Value d = Interp::applyBinary(Op::SUB, l, r);
            return elem == 1 ? d : Interp::applyBinary(Op::DIV, d, Value::makeInt(elem));
        }
        Value& n = lp ? r : l;
        if (elem != 1) n = Interp::applyBinary(Op::MUL, n, Value::makeInt(elem));
    }
    bool compare = b.op == BinOp::LT || b.op == BinOp::LE || b.op == BinOp::GT ||
                   b.op == BinOp::GE || b.op == BinOp::EQ || b.op == BinOp::NE;
    // A shift's left operand is promoted to the result type; the other
    // operators already had theirs converted by Sema. A comparison computes
    // in its operands' type, not in its int result.
    if (b.op == BinOp::SHL || b.op == BinOp::SHR) l = convert(l, lt, b.resolvedType);
    IntTy ty = (lp || rp) ? IntTy::NONE : intTyOf(compare ? lt : b.resolvedType);
    return Interp::applyBinary(binOp(b.op), l, r, ty);
}

Value AstInterp::evalAssign(AssignExpr& a) {
    Value v = eval(*a.value);   // the value is computed before the target is located
    const TypeSpec& tt = a.target->resolvedType;
    if (isStructValue(tt)) {
        Value dst = pointerFrom(addr(*a.target));
        copyBlock(v, dst, slots(tt));
        return dst;
    }
    Addr where = addr(*a.target);
    if (a.op == AssignOp::ASSIGN) {
        store(where, v);
        return v;
    }
    // x op= v computes x op v at a.opType (the usual arithmetic conversions,
    // set by Sema), then converts the result back to x's type. A shift
    // converts only x. A pointer steps by elements.
    bool intOp = isIntegerType(a.opType) && !isPointerType(tt);
    bool shift = a.op == AssignOp::SHL_ASSIGN || a.op == AssignOp::SHR_ASSIGN;
    if (intOp && !shift) v = convert(v, a.value->resolvedType, a.opType);
    if (isPointerType(tt)) {
        long long elem = slots(pointeeOf(tt));
        if (elem != 1) v = Interp::applyBinary(Op::MUL, v, Value::makeInt(elem));
    }
    Value cur = load(where);
    if (intOp) cur = convert(cur, tt, a.opType);
    Value r = Interp::applyBinary(compoundOp(a.op), cur, v, intOp ? intTyOf(a.opType) : IntTy::NONE);
    if (!isFloat(tt) && isFloat(a.value->resolvedType))
        r = Value::makeInt(wrapInt(r.kind == Value::Kind::FLOAT ? static_cast<long long>(r.fval) : r.ival, intTyOf(tt)));
    else if (isIntegerType(tt) && isIntegerType(a.opType))
        r = convert(r, a.opType, tt);
    store(where, r);
    return r;
}

// ---- statements -----------------------------------------------------------

const std::set<std::string>& AstInterp::collectLabels(Stmt* s) {
    std::set<std::string> out;
    auto add = [&](Stmt* c) { if (c) { const auto& in = collectLabels(c); out.insert(in.begin(), in.end()); } };
    switch (s->kind) {
        case NodeKind::CompoundStmt: for (auto& c : static_cast<CompoundStmt&>(*s).stmts) add(c.get()); break;
        case NodeKind::IfStmt: {
            auto& i = static_cast<IfStmt&>(*s);
            add(i.thenStmt.get());
            add(i.elseStmt.get());
            break;
        }
        case NodeKind::WhileStmt: add(static_cast<WhileStmt&>(*s).body.get()); break;
        case NodeKind::ForStmt:   add(static_cast<ForStmt&>(*s).body.get()); break;
        case NodeKind::DoStmt:    add(static_cast<DoStmt&>(*s).body.get()); break;
        case NodeKind::LabelStmt: {
            auto& l = static_cast<LabelStmt&>(*s);
            out.insert(l.label);
            add(l.stmt.get());
            break;
        }
        case NodeKind::SwitchStmt:
            for (auto& c : static_cast<SwitchStmt&>(*s).cases) for (auto& st : c.stmts) add(st.get());
            break;
        default: break;
    }
    return labelsIn_[s] = std::move(out);
}

bool AstInterp::contains(const Stmt* s, const std::string& label) const {
    auto it = labelsIn_.find(s);
    return it != labelsIn_.end() && it->second.count(label);
}

AstInterp::Flow AstInterp::execCompound(CompoundStmt& c) {
    scopes_.emplace_back();
    for (auto& l : c.locals) scopes_.back()[l->name] = l->isStatic ? statics_[l.get()] : declare(*l, !seeking_);
    Flow result = Flow::NORMAL;
    for (size_t i = 0; i < c.stmts.size(); ) {
        Flow f = exec(*c.stmts[i]);
        if (f == Flow::GOTO && contains(&c, gotoLabel_)) {
            seeking_ = true;   // re-enter this block, skipping to the label
            i = 0;
            continue;
        }
        if (f != Flow::NORMAL) { result = f; break; }
        ++i;
    }
    scopes_.pop_back();
    return result;
}

AstInterp::Flow AstInterp::execSwitch(SwitchStmt& s) {
    auto caseHas = [&](const SwitchCase& c) {
        for (auto& st : c.stmts) if (contains(st.get(), gotoLabel_)) return true;
        return false;
    };
    size_t start = s.cases.size();
    if (seeking_) {
        for (size_t i = 0; i < s.cases.size(); ++i) if (caseHas(s.cases[i])) { start = i; break; }
    } else {
        Value v = eval(*s.cond);
        for (size_t i = 0; i < s.cases.size() && start == s.cases.size(); ++i)
            if (!s.cases[i].isDefault && v.ival == s.cases[i].value) start = i;
        if (start == s.cases.size())
            for (size_t i = 0; i < s.cases.size(); ++i) if (s.cases[i].isDefault) start = i;
    }
    scopes_.emplace_back();
    Flow result = Flow::NORMAL;
    for (size_t i = start; i < s.cases.size() && result == Flow::NORMAL; ++i)
        for (size_t j = 0; j < s.cases[i].stmts.size(); ++j) {
            Flow f = exec(*s.cases[i].stmts[j]);
            if (f == Flow::GOTO) {
                bool inside = false;
                for (size_t k = 0; k < s.cases.size(); ++k) if (caseHas(s.cases[k])) { inside = true; i = k; }
                if (inside) { seeking_ = true; j = static_cast<size_t>(-1); continue; }
            }
            if (f == Flow::BREAK) { result = Flow::BREAK; break; }
            if (f != Flow::NORMAL) { result = f; break; }
        }
    scopes_.pop_back();
    return result == Flow::BREAK ? Flow::NORMAL : result;
}

AstInterp::Flow AstInterp::exec(Stmt& s) {
    if (seeking_ && !contains(&s, gotoLabel_)) {
        // Jumping over a declaration still brings the name into scope, with
        // its current value if this block already declared it.
        if (s.kind == NodeKind::DeclStmt) {
            VarDecl& d = *static_cast<DeclStmt&>(s).decl;
            if (!scopes_.back().count(d.name))
                scopes_.back()[d.name] = d.isStatic ? statics_[&d] : declare(d, false);
        }
        return Flow::NORMAL;
    }
    switch (s.kind) {
        case NodeKind::ExprStmt: {
            auto& e = static_cast<ExprStmt&>(s);
            if (e.expr) eval(*e.expr);
            return Flow::NORMAL;
        }
        case NodeKind::DeclStmt: {
            VarDecl& d = *static_cast<DeclStmt&>(s).decl;
            scopes_.back()[d.name] = d.isStatic ? statics_[&d] : declare(d, true);
            return Flow::NORMAL;
        }
        case NodeKind::CompoundStmt: return execCompound(static_cast<CompoundStmt&>(s));
        case NodeKind::IfStmt: {
            auto& i = static_cast<IfStmt&>(s);
            bool takeThen = seeking_ ? contains(i.thenStmt.get(), gotoLabel_) : eval(*i.cond).truthy();
            if (takeThen) return exec(*i.thenStmt);
            return i.elseStmt ? exec(*i.elseStmt) : Flow::NORMAL;
        }
        case NodeKind::WhileStmt: {
            auto& w = static_cast<WhileStmt&>(s);
            for (;;) {
                if (!seeking_ && !eval(*w.cond).truthy()) break;
                Flow f = exec(*w.body);
                if (f == Flow::BREAK) break;
                if (f == Flow::GOTO && contains(w.body.get(), gotoLabel_)) { seeking_ = true; continue; }
                if (f != Flow::NORMAL && f != Flow::CONTINUE) return f;
            }
            return Flow::NORMAL;
        }
        case NodeKind::DoStmt: {
            auto& d = static_cast<DoStmt&>(s);
            for (;;) {
                Flow f = exec(*d.body);
                if (f == Flow::BREAK) break;
                if (f == Flow::GOTO && contains(d.body.get(), gotoLabel_)) { seeking_ = true; continue; }
                if (f != Flow::NORMAL && f != Flow::CONTINUE) return f;
                if (!eval(*d.cond).truthy()) break;
            }
            return Flow::NORMAL;
        }
        case NodeKind::ForStmt: {
            auto& fs = static_cast<ForStmt&>(s);
            if (!seeking_ && fs.init) eval(*fs.init);
            for (;;) {
                if (!seeking_ && fs.cond && !eval(*fs.cond).truthy()) break;
                Flow f = exec(*fs.body);
                if (f == Flow::BREAK) break;
                if (f == Flow::GOTO && contains(fs.body.get(), gotoLabel_)) { seeking_ = true; continue; }
                if (f != Flow::NORMAL && f != Flow::CONTINUE) return f;
                if (fs.step) eval(*fs.step);   // `continue` still runs the step
            }
            return Flow::NORMAL;
        }
        case NodeKind::ReturnStmt: {
            auto& r = static_cast<ReturnStmt&>(s);
            if (!r.value) { retVal_ = Value::makeInt(0); return Flow::RETURN; }
            retVal_ = eval(*r.value);
            if (isStructValue(r.value->resolvedType)) retVal_ = copyOf(retVal_, r.value->resolvedType);
            return Flow::RETURN;
        }
        case NodeKind::BreakStmt:    return Flow::BREAK;
        case NodeKind::ContinueStmt: return Flow::CONTINUE;
        case NodeKind::SwitchStmt:   return execSwitch(static_cast<SwitchStmt&>(s));
        case NodeKind::GotoStmt:
            gotoLabel_ = static_cast<GotoStmt&>(s).label;
            return Flow::GOTO;
        case NodeKind::LabelStmt: {
            auto& l = static_cast<LabelStmt&>(s);
            if (seeking_ && l.label == gotoLabel_) seeking_ = false;
            return exec(*l.stmt);
        }
        default:
            throw RuntimeError{"internal error: not a statement"};
    }
}

}  // namespace minic
