// MiniC-Opt — P1: constant folding
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Step 6
//
// Precondition: none — this is purely syntactic (Table 13's own framing).
// Rewrite: any quad computing a binary or unary operator over compile-time
// constant operand(s) is replaced with a COPY of the already-computed
// constant result, e.g. `t1 = 4 * 5` -> `t1 = 20`.
#include "opt_passes.h"
#include "ir_utils.h"

namespace minic {
namespace {

Operand mkI(long long v) { Operand o; o.kind = OperandKind::INT_CONST;   o.ival = v; return o; }
Operand mkF(double v)    { Operand o; o.kind = OperandKind::FLOAT_CONST; o.fval = v; return o; }
bool isConst(const Operand& o) { return o.kind == OperandKind::INT_CONST || o.kind == OperandKind::FLOAT_CONST; }
double asD(const Operand& o)   { return o.kind == OperandKind::FLOAT_CONST ? o.fval : static_cast<double>(o.ival); }
bool truthyConst(const Operand& o) { return o.kind == OperandKind::FLOAT_CONST ? (o.fval != 0.0) : (o.ival != 0); }

// Mirrors Interp::applyBinary/NEG/NOT/TO_INT/TO_FLOAT exactly (see
// interp.cpp) — constant folding is a compile-time evaluation of the same
// semantics the interpreter applies at run time. The two must never
// diverge: a folded program that computed a different answer than the
// unoptimized one would slip straight past the differential-execution
// gate this whole pipeline exists to feed.
bool foldBinary(Op op, const Operand& a, const Operand& b, Operand& out) {
    bool useFloat = (a.kind == OperandKind::FLOAT_CONST || b.kind == OperandKind::FLOAT_CONST);
    switch (op) {
        case Op::ADD: out = useFloat ? mkF(asD(a) + asD(b)) : mkI(a.ival + b.ival); return true;
        case Op::SUB: out = useFloat ? mkF(asD(a) - asD(b)) : mkI(a.ival - b.ival); return true;
        case Op::MUL: out = useFloat ? mkF(asD(a) * asD(b)) : mkI(a.ival * b.ival); return true;
        case Op::DIV:
            if (useFloat) { out = mkF(asD(a) / asD(b)); return true; }
            if (b.ival == 0) return false;   // leave division-by-zero to run time, same as unoptimized
            out = mkI(a.ival / b.ival); return true;
        case Op::MOD:
            if (b.ival == 0) return false;
            out = mkI(a.ival % b.ival); return true;
        case Op::LT: out = mkI(useFloat ? (asD(a) <  asD(b)) : (a.ival <  b.ival)); return true;
        case Op::LE: out = mkI(useFloat ? (asD(a) <= asD(b)) : (a.ival <= b.ival)); return true;
        case Op::GT: out = mkI(useFloat ? (asD(a) >  asD(b)) : (a.ival >  b.ival)); return true;
        case Op::GE: out = mkI(useFloat ? (asD(a) >= asD(b)) : (a.ival >= b.ival)); return true;
        case Op::EQ: out = mkI(useFloat ? (asD(a) == asD(b)) : (a.ival == b.ival)); return true;
        case Op::NE: out = mkI(useFloat ? (asD(a) != asD(b)) : (a.ival != b.ival)); return true;
        case Op::AND: out = mkI((truthyConst(a) && truthyConst(b)) ? 1 : 0); return true;
        case Op::OR:  out = mkI((truthyConst(a) || truthyConst(b)) ? 1 : 0); return true;
        default: return false;
    }
}

bool foldUnary(Op op, const Operand& a, Operand& out) {
    switch (op) {
        case Op::NEG: out = (a.kind == OperandKind::FLOAT_CONST) ? mkF(-a.fval) : mkI(-a.ival); return true;
        case Op::NOT: out = mkI(truthyConst(a) ? 0 : 1); return true;
        case Op::TO_INT:
            out = mkI(a.kind == OperandKind::FLOAT_CONST ? static_cast<long long>(a.fval) : a.ival);
            return true;
        case Op::TO_FLOAT:
            out = mkF(a.kind == OperandKind::FLOAT_CONST ? a.fval : static_cast<double>(a.ival));
            return true;
        default: return false;
    }
}

class ConstantFolding : public Pass {
public:
    std::string name() const override { return "fold"; }
    std::string description() const override {
        return "constant folding: replace an operation on compile-time constant operand(s) with its computed result";
    }
    std::string requiresAnalysis() const override { return "none (syntactic)"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        for (size_t bi = 0; bi < cfg.blocks().size(); ++bi) {
            for (Quad& q : cfg.block(static_cast<int>(bi)).code) {
                Operand result;
                bool folded = false;
                if (isBinaryExprOp(q.op) && isConst(q.arg1) && isConst(q.arg2))
                    folded = foldBinary(q.op, q.arg1, q.arg2, result);
                else if ((q.op == Op::NEG || q.op == Op::NOT || q.op == Op::TO_INT || q.op == Op::TO_FLOAT)
                         && isConst(q.arg1))
                    folded = foldUnary(q.op, q.arg1, result);
                if (folded) {
                    q.op = Op::COPY;
                    q.arg1 = result;
                    q.arg2 = Operand{};
                    ++count_;
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
