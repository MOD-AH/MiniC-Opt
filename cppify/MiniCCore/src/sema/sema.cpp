// MiniC-Opt — semantic analysis and type checking (implementation)
// Owner: Member 2 (Background / Requirements Analyst)
#include "sema.h"
#include <algorithm>

namespace minic {

static bool isVoidValue(const TypeSpec& t) {
    return t.base == BaseType::VOID && t.ptrDepth == 0 && !t.isArray;
}

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

// One letter per type: v void, i int, f float, S char*, P void* (any pointer).
// "ret:params". Both the return and every parameter use the same alphabet.
const std::unordered_map<std::string, Sema::LibFn>& Sema::libFns() {
    static const std::unordered_map<std::string, LibFn> table = [] {
        auto ty = [](char ch) {
            switch (ch) {
                case 'v': return TypeSpec{BaseType::VOID, false, -1};
                case 'f': return TypeSpec{BaseType::FLOAT, false, -1};
                case 'S': return TypeSpec{BaseType::CHAR, false, -1, 1};
                case 'P': return TypeSpec{BaseType::VOID, false, -1, 1};
                default:  return TypeSpec{BaseType::INT, false, -1};
            }
        };
        static const char* defs[][2] = {
            {"strlen","i:S"},{"strcpy","S:SS"},{"strncpy","S:SSi"},{"strcat","S:SS"},{"strncat","S:SSi"},
            {"strcmp","i:SS"},{"strncmp","i:SSi"},{"strchr","S:Si"},{"strrchr","S:Si"},{"strstr","S:SS"},
            {"strcspn","i:SS"},{"strspn","i:SS"},{"strdup","S:S"},
            {"memset","P:Pii"},{"memcpy","P:PPi"},{"memmove","P:PPi"},{"memcmp","i:PPi"},
            {"atoi","i:S"},{"atol","i:S"},{"atof","f:S"},
            {"abs","i:i"},{"labs","i:i"},{"fabs","f:f"},{"sqrt","f:f"},{"cbrt","f:f"},{"pow","f:ff"},
            {"sin","f:f"},{"cos","f:f"},{"tan","f:f"},{"asin","f:f"},{"acos","f:f"},{"atan","f:f"},{"atan2","f:ff"},
            {"sinh","f:f"},{"cosh","f:f"},{"tanh","f:f"},
            {"exp","f:f"},{"log","f:f"},{"log10","f:f"},{"log2","f:f"},
            {"floor","f:f"},{"ceil","f:f"},{"round","f:f"},{"trunc","f:f"},{"fmod","f:ff"},
            {"fmin","f:ff"},{"fmax","f:ff"},{"hypot","f:ff"},
            {"rand","i:"},{"srand","v:i"},{"time","i:P"},{"clock","i:"},
            {"malloc","P:i"},{"calloc","P:ii"},{"realloc","P:Pi"},{"free","v:P"},
            {"exit","v:i"},{"assert","v:i"},
            {"putchar","i:i"},{"getchar","i:"},{"puts","i:S"},{"gets","S:S"},{"fgets","S:SiP"},
            {"fputs","i:SP"},{"fputc","i:iP"},{"putc","i:iP"},{"fflush","i:P"},
            {"toupper","i:i"},{"tolower","i:i"},{"isdigit","i:i"},{"isalpha","i:i"},{"isalnum","i:i"},
            {"isspace","i:i"},{"isupper","i:i"},{"islower","i:i"},{"ispunct","i:i"},{"isxdigit","i:i"},
        };
        std::unordered_map<std::string, LibFn> t;
        for (auto& d : defs) {
            std::string sig = d[1];
            LibFn f;
            f.ret = ty(sig[0]);
            for (size_t i = 2; i < sig.size(); ++i) f.params.push_back(ty(sig[i]));
            t[d[0]] = std::move(f);
        }
        return t;
    }();
    return table;
}

void Sema::registerSignatures(Program& prog) {
    for (auto& d : prog.decls) {
        auto* fn = dynamic_cast<FuncDecl*>(d.get());
        if (!fn) continue;
        if (builtins().count(fn->name) || fn->name == "printf") {
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

void Sema::checkConstantZeroDivisor(const BinaryExpr& n, const char* opSym) {
    if (n.rhs->kind == NodeKind::IntLit) {
        auto* lit = static_cast<const IntLit*>(n.rhs.get());
        if (lit->value == 0)
            sink_.add(n.line, n.col, std::string("division by zero in constant expression ('") + opSym + "' by literal 0)");
    } else if (n.rhs->kind == NodeKind::FloatLit) {
        auto* lit = static_cast<const FloatLit*>(n.rhs.get());
        if (lit->value == 0.0)
            sink_.add(n.line, n.col, std::string("division by zero in constant expression ('") + opSym + "' by literal 0.0)");
    }
}

bool Sema::typesCompatible(TypeSpec a, TypeSpec b) const {
    if (a.isArray != b.isArray) return false;
    if (a.base == BaseType::VOID || b.base == BaseType::VOID) return a.base == b.base;
    return true;   // any int/char/float combination is compatible via implicit conversion
}

std::string Sema::structName(int id) const {
    if (!structs_ || id < 0 || id >= static_cast<int>(structs_->structs.size())) return "?";
    const std::string& n = structs_->structs[id].name;
    return n.empty() ? "<anonymous>" : n;
}

void Sema::flatSlotTypes(const TypeSpec& t, std::vector<TypeSpec>& out) const {
    if (t.isArray) {
        TypeSpec e = pointeeOf(t);
        int n = t.arraySize < 0 ? 1 : t.arraySize;
        for (int i = 0; i < n; ++i) flatSlotTypes(e, out);
    } else if (isStructValue(t) && structs_) {
        for (const auto& f : structs_->structs[t.structId].fields) flatSlotTypes(f.type, out);
    } else {
        out.push_back(t);
    }
}

// An if/while/for/do condition must be a single value: a struct has no truth value.
void Sema::checkScalarCondition(Expr& cond) {
    TypeSpec t = checkExpr(cond);
    if (isStructValue(t))
        sink_.add(cond.line, cond.col, "a struct cannot be used as a condition");
    else if (t.base == BaseType::VOID && t.ptrDepth == 0 && !t.isArray)
        sink_.add(cond.line, cond.col, "a void value cannot be used as a condition");
}

bool Sema::isNullLiteral(const Expr* e) {
    return e && e->kind == NodeKind::IntLit && static_cast<const IntLit*>(e)->value == 0;
}

bool Sema::assignable(TypeSpec to, TypeSpec from, const Expr* fromExpr) const {
    auto voidValue = [](const TypeSpec& t) {
        return t.base == BaseType::VOID && t.ptrDepth == 0 && !t.isArray;
    };
    // A void VALUE is only ever compatible with another void value.
    if (voidValue(to) || voidValue(from)) return voidValue(to) && voidValue(from);

    // A struct is assigned whole, and only to the same struct.
    if (isStructValue(to) || isStructValue(from))
        return isStructValue(to) && isStructValue(from) && to.structId == from.structId;

    // An array used as a value is a pointer to its element, which is what
    // makes `int a[]` and `int *p` interchangeable as parameters.
    TypeSpec t = decayed(to), f = decayed(from);
    bool tp = t.ptrDepth > 0, fp = f.ptrDepth > 0;

    if (!tp && !fp) return true;           // numeric to numeric: implicit conversion
    if (tp && fp) {
        if (t.ptrDepth == f.ptrDepth && t.base == f.base
            && (t.base != BaseType::STRUCT || t.structId == f.structId)
            && t.extraDims == f.extraDims) return true;
        // void* is the generic pointer: it converts to and from any other.
        if ((t.base == BaseType::VOID && t.ptrDepth == 1) ||
            (f.base == BaseType::VOID && f.ptrDepth == 1)) return true;
        return false;
    }
    if (tp && !fp) return isNullLiteral(fromExpr);   // only the literal 0 is a null pointer
    return false;                                    // a pointer never converts to a number
}

void Sema::wrapConversionIfNeeded(std::unique_ptr<Expr>& slot, TypeSpec target) {
    if (!slot) return;
    if (isPointerLike(slot->resolvedType) || isPointerLike(target)) return;   // pointers carry no numeric conversion
    if (slot->resolvedType.base == target.base) return;
    if (slot->resolvedType.base == BaseType::VOID || target.base == BaseType::VOID) return;
    if (slot->resolvedType.base == BaseType::STRUCT || target.base == BaseType::STRUCT) return;
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

// A string literal is a `char *` to a NUL-terminated array. As the format of
// printf and scanf it stays a constant; anywhere else IRGen builds the array.
void Sema::visit(StringLit& n) {
    n.resolvedType = TypeSpec{BaseType::CHAR, false, -1, 1};
}

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
    // Both an array and a pointer can be subscripted: p[i] is *(p + i).
    if (!isPointerLike(baseType))
        sink_.add(n.line, n.col, "subscripted value is not an array or pointer");
    // VOID was missing here (every other scalar-context check in this
    // file — BinaryExpr, AssignExpr, UnaryExpr inc/dec — excludes it).
    // a[print_int(1)] used to pass this check, and IRGen then handed
    // LOAD_INDEX/STORE_INDEX the NONE-kind, empty-name operand a void
    // call produces as its "value".
    if (isPointerLike(idxType) || idxType.base == BaseType::FLOAT
        || (idxType.base == BaseType::VOID && idxType.ptrDepth == 0))
        sink_.add(n.line, n.col, "array index must be an integral expression");
    // The element of an array keeps the array's pointer depth; the element
    // behind a pointer is one level shallower.
    // Through pointeeOf, so a row of a 2-D array and a struct element both
    // come out right: a[i] of `int a[3][4]` is an int[4], not an int.
    TypeSpec dec = decayed(baseType);
    if (dec.ptrDepth == 1 && dec.base == BaseType::VOID) {
        sink_.add(n.line, n.col, "cannot subscript a void pointer");
        n.resolvedType = TypeSpec{BaseType::INT, false, -1};
        return;
    }
    n.resolvedType = isPointerLike(baseType) ? pointeeOf(baseType) : TypeSpec{BaseType::INT, false, -1};
}

// printf, fprintf, sprintf, snprintf, scanf, sscanf, fscanf: a fixed prefix,
// then a format string, then any number of further arguments whose count and
// types depend on that format. The format is not parsed here, but a scanf
// argument that is not an address is certain to be wrong, so that is checked.
bool Sema::checkFormatFamily(CallExpr& n) {
    struct Fam { const char* name; const char* fixed; int fmtIdx; bool isScan; bool returnsInt; };
    static const Fam fams[] = {
        {"printf",  "",   0, false, false}, {"fprintf", "P",  1, false, false},
        {"sprintf", "S",  1, false, true},  {"snprintf","Si", 2, false, true},
        {"scanf",   "",   0, true,  true},  {"sscanf",  "S",  1, true,  true},
        {"fscanf",  "P",  1, true,  true},
    };
    const Fam* fam = nullptr;
    for (const auto& f : fams) if (n.callee == f.name) fam = &f;
    if (!fam) return false;

    size_t fixedN = std::string(fam->fixed).size();
    size_t need = static_cast<size_t>(fam->fmtIdx) + 1;
    n.resolvedType = fam->returnsInt ? TypeSpec{BaseType::INT, false, -1} : TypeSpec{BaseType::VOID, false, -1};
    if (n.args.size() < need) {
        sink_.add(n.line, n.col, "'" + n.callee + "' needs " + std::string(fixedN ? "its destination and " : "")
                                 + "a format string");
        for (auto& a : n.args) checkExpr(*a);
        return true;
    }
    for (size_t i = 0; i < n.args.size(); ++i) {
        Expr& a = *n.args[i];
        if (static_cast<int>(i) == fam->fmtIdx) {
            if (a.kind == NodeKind::StringLit) static_cast<StringLit&>(a).isFormat = true;
            TypeSpec ft = checkExpr(a);
            if (!assignable(TypeSpec{BaseType::CHAR, false, -1, 1}, ft, &a))
                sink_.add(a.line, a.col, "the format argument of '" + n.callee + "' must be a string");
            continue;
        }
        TypeSpec at = checkExpr(a);
        if (i < fixedN) {
            char want = fam->fixed[i];
            TypeSpec wt = want == 'S' ? TypeSpec{BaseType::CHAR, false, -1, 1}
                        : want == 'P' ? TypeSpec{BaseType::VOID, false, -1, 1}
                                      : TypeSpec{BaseType::INT, false, -1};
            if (!assignable(wt, at, &a))
                sink_.add(a.line, a.col, "argument " + std::to_string(i + 1) + " to '" + n.callee + "' has the wrong type");
            else wrapConversionIfNeeded(n.args[i], wt);
            continue;
        }
        if (isStructValue(at)) sink_.add(a.line, a.col, "a struct cannot be passed to " + n.callee + "; use its fields");
        else if (isVoidValue(at)) sink_.add(a.line, a.col, "a void value cannot be passed to " + n.callee);
        else if (fam->isScan && !isPointerLike(at))
            sink_.add(a.line, a.col, n.callee + " stores into its arguments, so each needs an address: write &name");
    }
    return true;
}

void Sema::visit(CallExpr& n) {
    // A user-defined function always wins over a library function of the same name.
    Symbol* userFn = lookupFunctionSymbol(n.callee);
    if (!userFn && checkFormatFamily(n)) return;

    auto libIt = libFns().find(n.callee);
    const LibFn* lib = (!userFn && libIt != libFns().end()) ? &libIt->second : nullptr;

    auto biIt = builtins().find(n.callee);
    const BuiltinSig* bi = (!userFn && !lib && biIt != builtins().end()) ? &biIt->second : nullptr;
    Symbol* fn = userFn;

    if (!bi && !fn && !lib) {
        sink_.add(n.line, n.col, "call to undeclared function '" + n.callee + "'");
        for (auto& a : n.args) checkExpr(*a);      // still check args for further diagnostics
        n.resolvedType = TypeSpec{BaseType::INT, false, -1};
        return;
    }

    std::vector<TypeSpec> paramTypes;
    TypeSpec retType;
    if (lib) {
        paramTypes = lib->params;
        retType = lib->ret;
    } else if (bi) {
        for (auto b : bi->params) paramTypes.push_back(TypeSpec{b, false, -1});
        retType = TypeSpec{bi->ret, false, -1};
    } else {
        paramTypes = fn->paramTypes;
        retType = fn->type;          // the WHOLE type, so `int *f()` returns a pointer
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
        if (!assignable(pt, at, n.args[i].get()))
            sink_.add(n.args[i]->line, n.args[i]->col,
                       "argument " + std::to_string(i + 1) + " to '" + n.callee + "' has the wrong type");
        else
            wrapConversionIfNeeded(n.args[i], pt);
    }
    for (size_t i = checkCount; i < n.args.size(); ++i) checkExpr(*n.args[i]);

    n.resolvedType = retType;
}

// The forms an assignment, ++/-- or & may act on: something that names storage.
static bool isLvalueShape(const Expr& e) {
    if (e.kind == NodeKind::IdentExpr || e.kind == NodeKind::IndexExpr || e.kind == NodeKind::MemberExpr) return true;
    if (e.kind == NodeKind::UnaryExpr)
        return static_cast<const UnaryExpr&>(e).op == UnOp::DEREF;
    return false;
}

void Sema::visit(UnaryExpr& n) {
    TypeSpec ot = checkExpr(*n.operand);
    switch (n.op) {
        case UnOp::ADDR: {
            if (!isLvalueShape(*n.operand))
                sink_.add(n.line, n.col, "operand of '&' must be a variable, array element or dereferenced pointer");
            // `&x` on a plain scalar means x must live in memory. Flag its
            // declaration so IRGen boxes it; every access then goes through
            // loads and stores, which the optimizer never reorders or deletes.
            if (n.operand->kind == NodeKind::IdentExpr) {
                auto& id = static_cast<IdentExpr&>(*n.operand);
                Symbol* sym = symtab_.lookup(id.name);
                if (sym && sym->addrFlag && !sym->type.isArray && !isStructValue(sym->type)) *sym->addrFlag = true;
            }
            n.resolvedType = ot.isArray ? decayed(ot) : addressOfType(ot);
            break;
        }
        case UnOp::DEREF: {
            TypeSpec p = decayed(ot);
            if (p.ptrDepth == 0) {
                sink_.add(n.line, n.col, "operand of '*' must be a pointer");
                n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            } else if (p.base == BaseType::VOID && p.ptrDepth == 1) {
                sink_.add(n.line, n.col, "cannot dereference a void pointer");
                n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            } else {
                n.resolvedType = pointeeOf(p);
            }
            break;
        }
        case UnOp::NOT:
            if (ot.isArray || isStructValue(ot)) sink_.add(n.line, n.col, "operand of '!' must be a scalar value");
            n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            break;
        case UnOp::BITNOT:
            // Bitwise complement is integral-only: there is no meaningful
            // bit pattern to flip in a float here.
            if (isPointerLike(ot) || ot.base == BaseType::FLOAT || isVoidValue(ot) || isStructValue(ot))
                sink_.add(n.line, n.col, "operand of '~' must be integral");
            n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            break;
        case UnOp::NEG:
        case UnOp::POS:
            if (isPointerLike(ot) || isVoidValue(ot) || isStructValue(ot))
                sink_.add(n.line, n.col, std::string("operand of unary '") + toString(n.op) + "' must be numeric");
            n.resolvedType = (ot.base == BaseType::CHAR && ot.ptrDepth == 0) ? TypeSpec{BaseType::INT, false, -1}
                                                                              : TypeSpec{ot.base, false, -1};
            break;
        case UnOp::PRE_INC: case UnOp::PRE_DEC:
        case UnOp::POST_INC: case UnOp::POST_DEC:
            if (!isLvalueShape(*n.operand))
                sink_.add(n.line, n.col, "operand of increment/decrement must be a variable or array element");
            // A pointer may be incremented (it steps one element); an array name or void may not.
            if (ot.isArray || isVoidValue(ot) || isStructValue(ot))
                sink_.add(n.line, n.col, "operand of increment/decrement must be numeric");
            n.resolvedType = ot;
            n.resolvedType.isArray = false;
            break;
    }
}

void Sema::visit(BinaryExpr& n) {
    TypeSpec lt = checkExpr(*n.lhs);
    TypeSpec rt = checkExpr(*n.rhs);
    if (isVoidValue(lt) || isVoidValue(rt))
        sink_.add(n.line, n.col, std::string("invalid operands to binary '") + toString(n.op) + "'");
    if (isStructValue(lt) || isStructValue(rt)) {
        sink_.add(n.line, n.col, std::string("invalid operands to binary '") + toString(n.op)
                                 + "': a struct is not a number (compare or combine its fields)");
        n.resolvedType = TypeSpec{BaseType::INT, false, -1};
        return;
    }

    // Pointer arithmetic. An array is a pointer once used as a value, so
    // `a + 1`, `p - q` and `p == 0` all land here. Anything else applied to a
    // pointer (`p * 2`, `p & 1`) is meaningless and reported.
    if (isPointerLike(lt) || isPointerLike(rt)) {
        bool lp = isPointerLike(lt), rp = isPointerLike(rt);
        TypeSpec L = decayed(lt), R = decayed(rt);
        auto integral = [](const TypeSpec& t) { return !isPointerLike(t) && t.base != BaseType::FLOAT && t.base != BaseType::VOID; };
        n.resolvedType = TypeSpec{BaseType::INT, false, -1};
        switch (n.op) {
            case BinOp::ADD:
                if (lp && rp) { sink_.add(n.line, n.col, "cannot add two pointers"); n.resolvedType = L; }
                else if (!integral(lp ? rt : lt))
                    sink_.add(n.line, n.col, "pointer arithmetic needs an integer operand");
                else n.resolvedType = lp ? L : R;
                break;
            case BinOp::SUB:
                if (lp && rp) { /* pointer difference: an int */ }
                else if (!lp) sink_.add(n.line, n.col, "cannot subtract a pointer from a number");
                else if (!integral(rt)) sink_.add(n.line, n.col, "pointer arithmetic needs an integer operand");
                else n.resolvedType = L;
                break;
            case BinOp::LT: case BinOp::LE: case BinOp::GT: case BinOp::GE:
            case BinOp::EQ: case BinOp::NE:
            case BinOp::LOGAND: case BinOp::LOGOR:
                break;
            default:
                sink_.add(n.line, n.col, std::string("invalid operands to binary '") + toString(n.op) + "'");
        }
        return;
    }

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
        case BinOp::MOD: {
            // C restricts % to integral operands; it was routed through
            // the same usualArith path as the other arithmetic ops with
            // no additional check, so `5.5 % 2.0` passed sema clean and
            // reached the interpreter as a float MOD, which has no
            // defined meaning here.
            if (lt.base == BaseType::FLOAT || rt.base == BaseType::FLOAT)
                sink_.add(n.line, n.col, "operands of '%' must be integral");
            checkConstantZeroDivisor(n, "%");
            TypeSpec result = usualArith(lt, rt);
            wrapConversionIfNeeded(n.lhs, result);
            wrapConversionIfNeeded(n.rhs, result);
            n.resolvedType = result;
            break;
        }
        case BinOp::BITAND: case BinOp::BITOR: case BinOp::BITXOR:
        case BinOp::SHL:    case BinOp::SHR: {
            // Bitwise operators act on bit patterns, which a float does not
            // have here. Without this check they fell into the generic
            // arithmetic branch below, so `1.5 & 2` type-checked as a FLOAT
            // operation and the interpreter -- which reads .ival -- silently
            // computed garbage instead of reporting an error.
            if (lt.base == BaseType::FLOAT || rt.base == BaseType::FLOAT)
                sink_.add(n.line, n.col, std::string("operands of '") + toString(n.op)
                                         + "' must be integral");
            // char promotes to int, as with the arithmetic operators.
            n.resolvedType = TypeSpec{BaseType::INT, false, -1};
            break;
        }
        case BinOp::DIV:
            checkConstantZeroDivisor(n, "/");
            [[fallthrough]];
        default: {   // ADD SUB MUL DIV
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
    // An invalid target shape (not an IdentExpr/IndexExpr/dereference) was
    // already diagnosed by the parser (Step 2) — don't duplicate that message.
    if (isLvalueShape(*n.target)) {
        bool floatOperand = (tt.base == BaseType::FLOAT && !isPointerLike(tt))
                         || (vt.base == BaseType::FLOAT && !isPointerLike(vt));
        if ((n.op == AssignOp::PERCENT_ASSIGN || n.op == AssignOp::AND_ASSIGN
             || n.op == AssignOp::OR_ASSIGN || n.op == AssignOp::XOR_ASSIGN
             || n.op == AssignOp::SHL_ASSIGN || n.op == AssignOp::SHR_ASSIGN)
            && floatOperand)
            sink_.add(n.line, n.col, std::string("operands of '") + toString(n.op) + "' must be integral");
        if (tt.isArray) {
            sink_.add(n.line, n.col, "cannot assign to an array; assign to an element instead");
        } else if (isStructValue(tt) && n.op != AssignOp::ASSIGN) {
            sink_.add(n.line, n.col, std::string("invalid operator '") + toString(n.op) + "' on a struct");
        } else if (n.op == AssignOp::ASSIGN) {
            if (!assignable(tt, vt, n.value.get()))
                sink_.add(n.line, n.col, "type mismatch in assignment");
            else
                wrapConversionIfNeeded(n.value, tt);
        } else if (isPointerType(tt)) {
            // `p += n` / `p -= n` step a pointer by n elements; nothing else
            // makes sense on one.
            if (n.op != AssignOp::PLUS_ASSIGN && n.op != AssignOp::MINUS_ASSIGN)
                sink_.add(n.line, n.col, std::string("invalid operator '") + toString(n.op) + "' on a pointer");
            else if (isPointerLike(vt) || vt.base == BaseType::FLOAT)
                sink_.add(n.line, n.col, "type mismatch in assignment");
        } else if (isPointerLike(vt) || isStructValue(vt) || !typesCompatible(tt, vt)) {
            sink_.add(n.line, n.col, "type mismatch in assignment");
        }
        // For += -= *= /=, C computes `x = (T)(x op y)`: the usual
        // arithmetic conversion applies to the OPERATION (x's current
        // type against y's), and only the final RESULT narrows back to
        // T. Converting y to T here, before the op, narrows the wrong
        // operand at the wrong time — `int x=5; float y=-2.9; x+=y;`
        // should give 2 (5 + -2.9 = 2.1, truncated), not 3 (5 + (int)-2.9
        // = 5 + -2). Leaving n.value at its own natural type and letting
        // IRGen narrow the op's result instead (see IRGen::visit(AssignExpr&))
        // fixes this; it's a no-op whenever T already matches the wider
        // type (the common case), which is why hand-traced examples with
        // same-type operands never caught it.
    }
    n.resolvedType = tt;
}

// An explicit cast. Numeric casts are real conversions (IRGen emits TO_INT /
// TO_FLOAT); a cast to or from a pointer only changes what the checker sees.
void Sema::visit(CastExpr& n) {
    TypeSpec ot = checkExpr(*n.operand);
    if (isVoidValue(ot))
        sink_.add(n.line, n.col, "cannot cast a void value");
    else if (isStructValue(ot) || isStructValue(n.target))
        sink_.add(n.line, n.col, "cannot cast to or from a struct");
    else if (ot.isArray && !isPointerType(decayed(n.target)) && !isVoidValue(n.target))
        sink_.add(n.line, n.col, "cannot cast an array to a non-pointer type");
    n.resolvedType = n.target;
}

void Sema::visit(MemberExpr& n) {
    TypeSpec bt = checkExpr(*n.base);
    int id = -1;
    n.resolvedType = TypeSpec{BaseType::INT, false, -1};
    TypeSpec dec = decayed(bt);
    bool ptrToStruct = dec.ptrDepth == 1 && dec.base == BaseType::STRUCT && dec.extraDims.empty();
    if (n.arrow) {
        if (ptrToStruct) id = dec.structId;
        else if (isStructValue(bt))
            sink_.add(n.line, n.col, "'->' needs a pointer; this is a struct value, so use '.'");
        else
            sink_.add(n.line, n.col, "'->' needs a pointer to a struct");
    } else {
        if (isStructValue(bt)) id = bt.structId;
        else if (ptrToStruct)
            sink_.add(n.line, n.col, "'.' needs a struct value; this is a pointer, so use '->'");
        else
            sink_.add(n.line, n.col, "'.' needs a struct");
    }
    if (id < 0 || !structs_) return;
    if (!structs_->structs[id].complete) {
        sink_.add(n.line, n.col, "struct '" + structName(id) + "' is incomplete");
        return;
    }
    const FieldInfo* f = structs_->field(id, n.field);
    if (!f) {
        sink_.add(n.line, n.col, "struct '" + structName(id) + "' has no field '" + n.field + "'");
        return;
    }
    n.slotOffset = f->slotOffset;
    n.resolvedType = f->type;
}

void Sema::visit(SizeofExpr& n) {
    n.resolvedType = TypeSpec{BaseType::INT, false, -1};
    if (n.ofType) {
        if (n.type.base == BaseType::STRUCT && n.type.ptrDepth == 0 && structs_
            && !structs_->structs[n.type.structId].complete)
            sink_.add(n.line, n.col, "sizeof applied to incomplete struct '" + structName(n.type.structId) + "'");
        return;
    }
    TypeSpec t = checkExpr(*n.operand);
    if (!structs_) return;
    // An array parameter is really a pointer, and sizeof reports that.
    if (t.isArray && t.arraySize < 0) n.value = 8;
    else n.value = structs_->bytesOf(t);
}

void Sema::visit(CondExpr& n) {
    checkScalarCondition(*n.cond);
    TypeSpec a = checkExpr(*n.thenExpr);
    TypeSpec b = checkExpr(*n.elseExpr);
    if (isVoidValue(a) || isVoidValue(b)) {
        if (!(isVoidValue(a) && isVoidValue(b)))
            sink_.add(n.line, n.col, "the branches of '?:' must both have a value or both be void");
        n.resolvedType = TypeSpec{BaseType::VOID, false, -1};
    } else if (isStructValue(a) || isStructValue(b)) {
        if (!(isStructValue(a) && isStructValue(b) && a.structId == b.structId))
            sink_.add(n.line, n.col, "the branches of '?:' have different struct types");
        n.resolvedType = a;
    } else if (isPointerLike(a) || isPointerLike(b)) {
        TypeSpec A = decayed(a), B = decayed(b);
        if (assignable(A, B, n.elseExpr.get()))      n.resolvedType = A;
        else if (assignable(B, A, n.thenExpr.get())) n.resolvedType = B;
        else {
            sink_.add(n.line, n.col, "the branches of '?:' have incompatible pointer types");
            n.resolvedType = A;
        }
    } else {
        TypeSpec r = usualArith(a, b);
        wrapConversionIfNeeded(n.thenExpr, r);
        wrapConversionIfNeeded(n.elseExpr, r);
        n.resolvedType = r;
    }
}

void Sema::visit(CommaExpr& n) {
    checkExpr(*n.lhs);
    n.resolvedType = checkExpr(*n.rhs);
}

void Sema::visit(ConvertExpr& n) {
    // Sema is the only thing that creates these; nothing walks back over a
    // wrapped subtree during the same pass. Implemented for completeness —
    // Visitor requires every node kind to be handled.
    if (n.operand) checkExpr(*n.operand);
    n.resolvedType = n.targetType;
}

// ---- statements -----------------------------------------------------------

void Sema::visit(DeclStmt& n) { n.decl->accept(*this); }

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
    checkScalarCondition(*n.cond);
    n.thenStmt->accept(*this);
    if (n.elseStmt) n.elseStmt->accept(*this);
}

void Sema::visit(WhileStmt& n) {
    checkScalarCondition(*n.cond);
    n.body->accept(*this);
}

void Sema::visit(DoStmt& n) {
    n.body->accept(*this);
    checkScalarCondition(*n.cond);
}

void Sema::visit(ForStmt& n) {
    // for's own header is not a new scope in MiniC's grammar (init/cond/step
    // are plain expressions, not declarations), so nothing to push here —
    // the body, if it's a CompoundStmt, pushes its own scope as usual.
    if (n.init) checkExpr(*n.init);
    if (n.cond) checkScalarCondition(*n.cond);
    if (n.step) checkExpr(*n.step);
    n.body->accept(*this);
}

void Sema::visit(ReturnStmt& n) {
    if (n.value) {
        TypeSpec vt = checkExpr(*n.value);
        if (isVoidValue(currentReturnType_)) {
            sink_.add(n.line, n.col, "'return' with a value in a function returning void");
        } else if (!assignable(currentReturnType_, vt, n.value.get())) {
            sink_.add(n.line, n.col,
                       std::string("return type mismatch: function returns ") + toString(currentReturnType_.base));
        } else {
            wrapConversionIfNeeded(n.value, currentReturnType_);
        }
    } else if (!isVoidValue(currentReturnType_)) {
        sink_.add(n.line, n.col,
                   std::string("'return;' with no value in a function returning ") + toString(currentReturnType_.base));
    }
}

void Sema::visit(SwitchStmt& n) {
    TypeSpec t = checkExpr(*n.cond);
    if (isPointerLike(t) || isStructValue(t) || t.base == BaseType::FLOAT || isVoidValue(t))
        sink_.add(n.cond->line, n.cond->col, "a switch needs an integer expression");
    bool sawDefault = false;
    std::vector<long long> seen;
    symtab_.enterScope();          // one block scope shared by every case, as in C
    for (auto& cs : n.cases) {
        if (cs.isDefault) {
            if (sawDefault) sink_.add(cs.line, cs.col, "duplicate 'default' in switch");
            sawDefault = true;
        } else {
            for (long long v : seen)
                if (v == cs.value) { sink_.add(cs.line, cs.col, "duplicate case value " + std::to_string(cs.value)); break; }
            seen.push_back(cs.value);
        }
        for (auto& s : cs.stmts) s->accept(*this);
    }
    symtab_.exitScope();
}

void Sema::visit(GotoStmt& n) { pendingGotos_.push_back({n.label, n.line, n.col}); }

void Sema::visit(LabelStmt& n) {
    if (definedLabels_.count(n.label))
        sink_.add(n.line, n.col, "label '" + n.label + "' is defined twice in this function");
    definedLabels_[n.label] = n.line;
    n.stmt->accept(*this);
}

void Sema::visit(BreakStmt&)    {}
void Sema::visit(ContinueStmt&) {}

// ---- declarations ---------------------------------------------------------

void Sema::visit(VarDecl& n) {
    if (n.type.base == BaseType::VOID && n.type.ptrDepth == 0)
        sink_.add(n.line, n.col, "variable '" + n.name + "' declared void");

    if (isStructValue(n.type) && structs_ && !structs_->structs[n.type.structId].complete)
        sink_.add(n.line, n.col, "variable '" + n.name + "' has incomplete type 'struct " + structName(n.type.structId) + "'");

    bool aggregate = isStructValue(n.type)
        || (n.type.isArray && (!n.type.extraDims.empty()
                               || (n.type.base == BaseType::STRUCT && n.type.ptrDepth == 0)));

    if (aggregate) {
        // The parser flattened any brace initializer to exactly one
        // expression per slot, so each is checked against its slot's type.
        if (n.init && n.type.isArray) {
            sink_.add(n.line, n.col, "array '" + n.name + "' cannot be initialized from a scalar expression; use { ... }");
            checkExpr(*n.init);
        } else if (n.init) {
            TypeSpec it = checkExpr(*n.init);
            if (!assignable(n.type, it, n.init.get()))
                sink_.add(n.line, n.col, "type mismatch initializing '" + n.name + "'");
        }
        std::vector<TypeSpec> slots;
        flatSlotTypes(n.type, slots);
        if (!n.initList.empty() && slots.size() != n.initList.size())
            sink_.add(n.line, n.col, "initializer of '" + n.name + "' does not match its layout");
        for (size_t i = 0; i < n.initList.size(); ++i) {
            TypeSpec et = checkExpr(*n.initList[i]);
            if (i >= slots.size()) continue;
            if (!assignable(slots[i], et, n.initList[i].get()))
                sink_.add(n.initList[i]->line, n.initList[i]->col, "initializer type does not match the field it initializes");
            else
                wrapConversionIfNeeded(n.initList[i], slots[i]);
        }
    } else if (n.type.isArray) {
        if (n.init) {
            sink_.add(n.line, n.col,
                       "array '" + n.name + "' cannot be initialized from a scalar expression; use { ... }");
            checkExpr(*n.init);
        }
        TypeSpec elemType{n.type.base, false, -1, n.type.ptrDepth};
        for (auto& e : n.initList) {
            TypeSpec et = checkExpr(*e);
            if (!assignable(elemType, et, e.get()))
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
            if (!assignable(n.type, it, n.init.get()))
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
    sym.addrFlag = &n.addressTaken;
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
        sym.addrFlag = &p.addressTaken;
        if (symtab_.declare(sym))
            p.storageOffset = offset;
        else
            sink_.add(p.line, p.col, "redeclaration of parameter '" + p.name + "'");
    }

    TypeSpec savedReturn = currentReturnType_;
    currentReturnType_ = n.returnType;
    definedLabels_.clear();
    pendingGotos_.clear();

    if (n.body) {
        n.body->accept(*this);   // CompoundStmt pushes its own nested scope
        if (!isVoidValue(n.returnType) && !stmtDefinitelyReturns(n.body.get()))
            sink_.add(n.line, n.col,
                       "function '" + n.name + "' does not return a value on every path (missing return)");
    }

    for (const auto& g : pendingGotos_)
        if (!definedLabels_.count(g.label))
            sink_.add(g.line, g.col, "label '" + g.label + "' is used but never defined");

    currentReturnType_ = savedReturn;
    symtab_.exitScope();
}

void Sema::visit(Program& n) {
    structs_ = &n.structs;
    registerSignatures(n);
    for (auto& d : n.decls) d->accept(*this);
}

} // namespace minic
