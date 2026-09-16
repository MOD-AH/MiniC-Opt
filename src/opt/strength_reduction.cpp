// MiniC-Opt — P7: strength reduction (induction-variable multiply removal)
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// Precondition: induction variables, detected on top of the natural-loop
// shape loop_util.h restricts itself to (single usable preheader).
//
// The classic textbook transform: `i * K` (K a compile-time constant)
// recomputed every iteration is replaced by a variable `m` that is
// maintained *incrementally* — `m += step*K` inserted immediately after
// i's own update — instead of multiplied out fresh each time.
//
// A basic induction variable `i` is one written exactly once in the whole
// loop, by exactly `i = i + C` or `i = i - C` for an integer constant C
// (checked directly, not derived from reaching definitions — this is a
// syntactic pattern match on the loop's own block set, mirroring how
// loop_util.h's own preconditions are checked structurally rather than
// through the generic solver).
//
// Soundness argument for why the mirror variable needs no separate
// "does the use come before or after the update in this iteration" case
// split (the obvious-looking complication): `m`'s only update instruction
// is inserted as the very next instruction after i's own update, in the
// same block, guarded by nothing i's update isn't already guarded by. So
// for every point in the loop, m == i * K holds continuously except in
// the (unreachable-from-outside) gap between those two adjacent
// instructions — meaning a rewritten use of `i * K` anywhere in the loop,
// no matter where it sits relative to the update, reads back exactly the
// value i had at that same program point, times K. m is initialized in
// the preheader from i's own value at loop entry (`m = i * K`, one
// multiply, run once), so it costs nothing that wasn't already being
// paid, and every recurring `i * K` inside the loop becomes one ADD
// instead of one MUL.
//
// Restricted, like P8, to loops with a single usable preheader; a loop
// without one is left untouched rather than attempted unsoundly.
#include "opt_passes.h"
#include "loop_util.h"
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace minic {
namespace {

bool isIntConst(const Operand& o, long long& out) {
    if (o.kind != OperandKind::INT_CONST) return false;
    out = o.ival;
    return true;
}

// `i = i + C` or `i = i - C`. IRGen never fuses an assignment's
// arithmetic into the same quad as the store (see irgen.cpp: an
// assignment statement always evaluates its RHS into a temp first, then
// COPYs that temp into the target) -- so this is actually a two-quad
// shape, `tX = i +/- C` immediately followed by `i = tX`, and that is
// what gets matched: `atIdx` is the index of the COPY (the update's
// *observable* point -- where `i`'s new value actually lands), and the
// arithmetic is read out of the quad immediately before it.
bool isBasicInductionStep(const std::vector<Quad>& code, size_t atIdx, const std::string& i, long long& step) {
    if (atIdx == 0 || atIdx >= code.size()) return false;
    const Quad& copyQ = code[atIdx];
    if (copyQ.op != Op::COPY) return false;
    Operand w;
    if (!writesOf(copyQ, w) || w.name != i) return false;
    if (!isNameOperand(copyQ.arg1)) return false;
    const std::string& tX = copyQ.arg1.name;

    const Quad& addQ = code[atIdx - 1];
    if (addQ.op != Op::ADD && addQ.op != Op::SUB) return false;
    Operand w2;
    if (!writesOf(addQ, w2) || w2.name != tX) return false;
    long long c = 0;
    if (isNameOperand(addQ.arg1) && addQ.arg1.name == i && isIntConst(addQ.arg2, c)) {
        step = (addQ.op == Op::ADD) ? c : -c;
        return true;
    }
    if (addQ.op == Op::ADD && isNameOperand(addQ.arg2) && addQ.arg2.name == i && isIntConst(addQ.arg1, c)) {
        step = c;
        return true;
    }
    return false;
}

// `i * K` or `K * i`, K a compile-time integer constant.
bool isMultiplyByConst(const Quad& q, const std::string& i, long long& k) {
    if (q.op != Op::MUL) return false;
    if (isNameOperand(q.arg1) && q.arg1.name == i && isIntConst(q.arg2, k)) return true;
    if (isNameOperand(q.arg2) && q.arg2.name == i && isIntConst(q.arg1, k)) return true;
    return false;
}

std::string freshName(CFG& cfg, int counter) {
    std::string cand;
    std::unordered_set<std::string> used;
    for (const auto& b : cfg.blocks())
        for (const Quad& q : b.code) {
            if (isNameOperand(q.arg1)) used.insert(q.arg1.name);
            if (isNameOperand(q.arg2)) used.insert(q.arg2.name);
            if (isNameOperand(q.result)) used.insert(q.result.name);
        }
    int n = counter;
    do { cand = "__sr" + std::to_string(n++); } while (used.count(cand));
    return cand;
}

Operand mkVar(const std::string& name) { Operand o; o.kind = OperandKind::TEMPORARY; o.name = name; return o; }
Operand mkI(long long v) { Operand o; o.kind = OperandKind::INT_CONST; o.ival = v; return o; }

class StrengthReduction : public Pass {
public:
    std::string name() const override { return "strength"; }
    std::string description() const override {
        return "strength reduction: replace i*K, recomputed every iteration, with an "
               "incrementally-maintained mirror variable";
    }
    std::string requiresAnalysis() const override { return "induction variables"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        int fresh = 0;
        for (const NaturalLoop& lp : findNaturalLoops(cfg)) {
            if (lp.preheader < 0) continue;
            // Find every basic induction variable: exactly one write in
            // the loop, of the `i = i +/- C` shape.
            std::set<std::string> writtenOnce;
            for (auto& kv : countAll(cfg, lp.blocks)) if (kv.second == 1) writtenOnce.insert(kv.first);

            for (const std::string& iName : writtenOnce) {
                int updateBlock = -1, updateIdx = -1;
                long long step = 0;
                for (int bi : lp.blocks) {
                    BasicBlock& b = cfg.block(bi);
                    for (size_t qi = 0; qi < b.code.size(); ++qi) {
                        long long s;
                        if (isBasicInductionStep(b.code, qi, iName, s)) {
                            updateBlock = bi; updateIdx = static_cast<int>(qi); step = s;
                        }
                    }
                }
                if (updateBlock < 0) continue;   // the one write to iName wasn't a simple +/- C step

                // Every distinct K this induction variable is multiplied
                // by anywhere in the loop (excluding the update itself).
                std::set<long long> ks;
                for (int bi : lp.blocks)
                    for (size_t qi = 0; qi < cfg.block(bi).code.size(); ++qi) {
                        if (bi == updateBlock && static_cast<int>(qi) == updateIdx) continue;
                        long long k;
                        if (isMultiplyByConst(cfg.block(bi).code[qi], iName, k)) ks.insert(k);
                    }
                if (ks.empty()) continue;

                for (long long k : ks) {
                    std::string m = freshName(cfg, fresh++);
                    // Preheader: m = i * K  (i's value at loop entry).
                    Quad init; init.op = Op::MUL; init.arg1 = mkVar(iName); init.arg2 = mkI(k); init.result = mkVar(m);
                    appendToPreheader(cfg, lp.preheader, {init});
                    // Immediately after i's own update: m = m + step*K.
                    Quad bump; bump.op = Op::ADD; bump.arg1 = mkVar(m); bump.arg2 = mkI(step * k); bump.result = mkVar(m);
                    cfg.block(updateBlock).code.insert(cfg.block(updateBlock).code.begin() + updateIdx + 1, bump);

                    // Rewrite every `i*K`/`K*i` in the loop to `= m`. The
                    // insert above only shifted instructions *after*
                    // updateIdx, so updateIdx itself still names the
                    // induction update; updateIdx+1 names the bump we
                    // just inserted (neither can itself match
                    // isMultiplyByConst — both are ADD/SUB, never MUL —
                    // this exclusion is defensive, not load-bearing).
                    for (int bi : lp.blocks) {
                        BasicBlock& b = cfg.block(bi);
                        for (size_t qi = 0; qi < b.code.size(); ++qi) {
                            if (bi == updateBlock && (static_cast<int>(qi) == updateIdx || static_cast<int>(qi) == updateIdx + 1))
                                continue;   // the induction update, and the bump we just inserted
                            long long kk;
                            if (isMultiplyByConst(b.code[qi], iName, kk) && kk == k) {
                                b.code[qi].op = Op::COPY;
                                b.code[qi].arg1 = mkVar(m);
                                b.code[qi].arg2 = Operand{};
                                ++count_;
                            }
                        }
                    }
                }
            }
        }
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;

    static std::unordered_map<std::string, int> countAll(CFG& cfg, const std::set<int>& blocks) {
        std::unordered_map<std::string, int> n;
        for (int bi : blocks)
            for (const Quad& q : cfg.block(bi).code) {
                Operand w;
                if (writesOf(q, w)) ++n[w.name];
            }
        return n;
    }
};

} // namespace

std::unique_ptr<Pass> makeStrengthReductionPass() { return std::make_unique<StrengthReduction>(); }

} // namespace minic
