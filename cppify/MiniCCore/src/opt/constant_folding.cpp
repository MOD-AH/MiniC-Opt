// MiniC-Opt — P1: constant folding
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Step 6
//
// Precondition: none — this is purely syntactic (Table 13's own framing).
// Rewrite: any quad computing a binary or unary operator over compile-time
// constant operand(s) is replaced with a COPY of the already-computed
// constant result, e.g. `t1 = 4 * 5` -> `t1 = 20`.
#include "opt_passes.h"
#include "ir_utils.h"
#include "int_semantics.h"

namespace minic {
namespace {

Operand mkI(long long v) { Operand o; o.kind = OperandKind::INT_CONST;   o.ival = v; return o; }
Operand mkF(double v)    { Operand o; o.kind = OperandKind::FLOAT_CONST; o.fval = v; return o; }
bool isConst(const Operand& o) { return o.kind == OperandKind::INT_CONST || o.kind == OperandKind::FLOAT_CONST; }
double asD(const Operand& o)   { return o.kind == OperandKind::FLOAT_CONST ? o.fval : static_cast<double>(o.ival); }
bool truthyConst(const Operand& o) { return o.kind == OperandKind::FLOAT_CONST ? (o.fval != 0.0) : (o.ival != 0); }
}  // namespace

// Mirrors Interp::applyBinary/NEG/NOT/TO_INT/TO_FLOAT exactly (see
// interp.cpp) — constant folding is a compile-time evaluation of the same
// semantics the interpreter applies at run time. The two must never
// diverge: a folded program that computed a different answer than the
// unoptimized one would slip straight past the differential-execution
// gate this whole pipeline exists to feed.
bool foldBinary(Op op, const Operand& a, const Operand& b, IntTy ty, Operand& out) {
    if (op == Op::AND) { out = mkI((truthyConst(a) && truthyConst(b)) ? 1 : 0); return true; }
    if (op == Op::OR)  { out = mkI((truthyConst(a) || truthyConst(b)) ? 1 : 0); return true; }
    bool useFloat = (a.kind == OperandKind::FLOAT_CONST || b.kind == OperandKind::FLOAT_CONST);
    if (useFloat) {
        // Bitwise ops and % are integral-only; a float here would mean sema
        // let something through, so refuse to fold rather than invent a
        // result the interpreter would not produce.
        double x = asD(a), y = asD(b);
        switch (op) {
            case Op::ADD: out = mkF(x + y); return true;
            case Op::SUB: out = mkF(x - y); return true;
            case Op::MUL: out = mkF(x * y); return true;
            case Op::DIV:
                if (y == 0.0) return false;   // the interpreter traps on this; leave it to run time
                out = mkF(x / y); return true;
            case Op::LT: out = mkI(x <  y); return true;
            case Op::LE: out = mkI(x <= y); return true;
            case Op::GT: out = mkI(x >  y); return true;
            case Op::GE: out = mkI(x >= y); return true;
            case Op::EQ: out = mkI(x == y); return true;
            case Op::NE: out = mkI(x != y); return true;
            default: return false;
        }
    }
    // Where the run would trap (zero divisor, bad shift) leave the quad for
    // run time, same as unoptimized.
    long long v = 0;
    if (intBinary(op, a.ival, b.ival, ty, v)) return false;
    out = mkI(v);
    return true;
}

bool foldUnary(Op op, const Operand& a, IntTy ty, Operand& out) {
    bool isF = (a.kind == OperandKind::FLOAT_CONST);
    switch (op) {
        case Op::NEG:
            out = isF ? mkF(-a.fval) : mkI(wrapInt(static_cast<long long>(0 - static_cast<unsigned long long>(a.ival)), ty));
            return true;
        case Op::NOT: out = mkI(truthyConst(a) ? 0 : 1); return true;
        case Op::BITNOT:
            if (isF) return false;
            out = mkI(wrapInt(~a.ival, ty)); return true;
        case Op::TO_INT:
            out = mkI(wrapInt(isF ? static_cast<long long>(a.fval) : a.ival, ty));
            return true;
        case Op::TO_FLOAT:
            out = mkF(isF ? a.fval : static_cast<double>(a.ival));
            return true;
        default: return false;
    }
}

namespace {

class ConstantFolding : public Pass {
public:
    std::string name() const override { return "fold"; }
    std::string description() const override {
        return "constant folding: replace an operation on compile-time constant operand(s) with its computed result";
    }
    std::string requiresAnalysis() const override { return "none (syntactic)"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        for (size_t bi = 0; bi < cfg.blocks().size(); ++bi) {
            auto& blockCode = cfg.block(static_cast<int>(bi)).code;
            for (size_t qi = 0; qi < blockCode.size(); ++qi) {
                Quad& q = blockCode[qi];
                Operand result;
                bool folded = false;
                if (isBinaryExprOp(q.op) && isConst(q.arg1) && isConst(q.arg2))
                    folded = foldBinary(q.op, q.arg1, q.arg2, q.ty, result);
                else if ((q.op == Op::NEG || q.op == Op::NOT || q.op == Op::BITNOT
                          || q.op == Op::TO_INT || q.op == Op::TO_FLOAT)
                         && isConst(q.arg1))
                    folded = foldUnary(q.op, q.arg1, q.ty, result);
                if (folded) {
                    std::string beforeTxt = q.toString();
                    q.op = Op::COPY;
                    q.arg1 = result;
                    q.arg2 = Operand{};
                    q.ty = IntTy::NONE;   // the constant is already in range
                    ++count_;
                    TraceEdit e;
                    e.quadIndex = static_cast<int>(qi);
                    e.before = beforeTxt;
                    e.after = q.toString();
                    e.kind = "fold";
                    e.sourceLine = q.srcLine;
                    e.reason = "Both operands are compile-time constants, so the "
                               "operation is evaluated now and replaced by its result. "
                               "The arithmetic mirrors Interp::applyBinary exactly, so "
                               "folding and interpreting can never disagree.";
                    edits_.push_back(std::move(e));
                }
            }
        }
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
};

} // namespace

std::unique_ptr<Pass> makeConstantFoldingPass() { return std::make_unique<ConstantFolding>(); }

} // namespace minic
