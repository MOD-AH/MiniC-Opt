// Cppify — loop-invariant code motion on SSA, with alias facts (roadmap 2.6)
//
// In SSA an operand is invariant iff its single definition is outside the
// loop, so no "defined exactly once in the loop" or "dominates the tail"
// bookkeeping is needed (compare P8): moving a value's only definition to the
// preheader cannot change which definition any use sees, and the preheader
// dominates the whole loop. What still matters is that executing the hoisted
// instruction early, possibly on an entry where the loop body never runs,
// changes nothing observable:
//
//   * pure arithmetic that cannot trap: always safe (P8 allows this too);
//   * DIV/MOD by a non-zero constant, a shift by an in-range constant: safe,
//     since they cannot trap either (P8 refuses all of these);
//   * a load: only if no store or call in the loop can change the memory it
//     reads (alias.h), and it cannot trap where the original would not:
//     either the index is a constant inside a local array of known size, or
//     the load sits in the loop header ahead of any effect, so the header's
//     first run would have executed it, with the same operands, anyway.
//
// Loops with no single preheader are skipped, as in P7/P8 (loop_util.h).
#include "ssa.h"
#include "ssa_util.h"
#include "alias.h"
#include "int_semantics.h"
#include "../opt/loop_util.h"
#include <map>

namespace minic {
namespace {

bool nonTrappingArith(const Quad& q) {
    if (isSafeToRelocate(q.op)) return true;
    if (q.op == Op::DIV || q.op == Op::MOD)
        return (q.arg2.kind == OperandKind::INT_CONST && q.arg2.ival != 0) ||
               (q.arg2.kind == OperandKind::FLOAT_CONST && q.arg2.fval != 0.0);
    if (q.op == Op::SHL || q.op == Op::SHR)
        return q.arg2.kind == OperandKind::INT_CONST && q.arg2.ival >= 0 && q.arg2.ival < intBits(q.ty);
    return false;
}

// An instruction a hoisted load must not overtake: one with an effect, or
// one that may trap first (its error would then no longer be the one seen).
bool ordersLoads(const Quad& q) {
    switch (q.op) {
        case Op::CALL: case Op::STORE_INDEX: case Op::COPY_BLOCK: case Op::LOAD_INDEX:
        case Op::DIV: case Op::MOD: case Op::SHL: case Op::SHR:
            return true;
        default:
            return false;
    }
}

class SSALICM : public Pass {
public:
    std::string name() const override { return "licm-ssa"; }
    std::string description() const override {
        return "loop-invariant code motion on SSA: hoist invariant arithmetic, and loads that no "
               "store or call in the loop can change";
    }
    std::string requiresAnalysis() const override { return "SSA, dominators, natural loops, alias analysis"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        const auto& dom = analyses().dominators(cfg);
        AliasInfo alias(cfg);
        std::map<std::string, int> allocBlock;
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code) if (q.op == Op::ALLOC_ARRAY) allocBlock[q.result.name] = b.id;
        for (const NaturalLoop& lp : findNaturalLoops(cfg, dom)) {
            if (lp.preheader < 0) continue;
            std::set<std::string> definedIn;
            bool clobbered = false;
            std::vector<std::string> storeBases;
            for (int bi : lp.blocks) {
                const BasicBlock& b = cfg.block(bi);
                for (const Phi& phi : b.phis) definedIn.insert(phi.result.name);
                for (const Quad& q : b.code) {
                    Operand w;
                    if (writesOf(q, w)) definedIn.insert(w.name);
                    if (q.op == Op::CALL && AliasInfo::callWritesMemory(q)) clobbered = true;
                    if (q.op == Op::STORE_INDEX || q.op == Op::COPY_BLOCK) storeBases.push_back(q.result.name);
                }
            }
            bool anyCall = false;
            for (int bi : lp.blocks)
                for (const Quad& q : cfg.block(bi).code) if (q.op == Op::CALL) anyCall = true;
            // A global can be written by any call, so it is invariant only in
            // a loop without calls.
            auto invariant = [&](const Operand& o) {
                if (!isNameOperand(o)) return true;
                if (definedIn.count(o.name)) return false;
                bool global = o.kind == OperandKind::VARIABLE && o.name.find('#') == std::string::npos;
                return !(global && anyCall);
            };

            for (bool moved = true; moved; ) {
                moved = false;
                for (int bi : lp.blocks) {
                    BasicBlock& b = cfg.block(bi);
                    bool effectBefore = false;
                    for (size_t qi = 0; qi < b.code.size(); ++qi) {
                        Quad& q = b.code[qi];
                        Operand w;
                        bool ok = writesOf(q, w) && cfg.ssaOrigin.count(w.name);
                        for (const Operand& r : readsOf(q)) ok = ok && invariant(r);
                        std::string why;
                        if (ok && nonTrappingArith(q)) {
                            why = isSafeToRelocate(q.op)
                                ? "Every operand is defined outside the loop, so the value is the same on every "
                                  "iteration; the operation cannot trap, so computing it once before the loop is safe."
                                : "Every operand is defined outside the loop, and the divisor or shift count is a "
                                  "constant the operation accepts, so it cannot trap and is computed once before the "
                                  "loop. P8 refuses every division and shift.";
                        } else if (ok && q.op == Op::LOAD_INDEX && loadHoistable(q, bi, lp, effectBefore, clobbered,
                                                                                 storeBases, alias, allocBlock, dom)) {
                            why = "The array and index are defined outside the loop, and no store or call in the "
                                  "loop can write the array this load reads (alias analysis), so it reads the "
                                  "same value every iteration. It also cannot trap earlier than before. P8 never "
                                  "moves a load.";
                        }
                        if (ordersLoads(q)) effectBefore = true;
                        if (why.empty()) continue;
                        TraceEdit e;
                        e.before = q.toString();
                        e.after = q.toString() + "   (moved to preheader B" + std::to_string(lp.preheader) + ")";
                        e.kind = "hoist";
                        e.sourceLine = q.srcLine;
                        e.reason = why;
                        edits_.push_back(std::move(e));
                        Quad hoisted = q;
                        b.code.erase(b.code.begin() + static_cast<long>(qi));
                        insertBeforeTerminator(cfg.block(lp.preheader), hoisted);
                        definedIn.erase(w.name);
                        ++count_;
                        moved = true;
                        break;
                    }
                    if (moved) break;
                }
            }
        }
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;

    static bool loadHoistable(const Quad& q, int block, const NaturalLoop& lp, bool effectBefore, bool clobbered,
                              const std::vector<std::string>& storeBases, const AliasInfo& alias,
                              const std::map<std::string, int>& allocBlock, const std::vector<std::set<int>>& dom) {
        if (clobbered) return false;
        for (const auto& s : storeBases) if (alias.mayAlias(s, q.arg1.name)) return false;
        // In bounds only if the allocation has certainly happened by the time
        // the preheader ends: its block dominates the preheader.
        long long size = alias.allocSize(q.arg1.name);
        auto ab = allocBlock.find(q.arg1.name);
        bool allocated = ab != allocBlock.end() && dom[static_cast<size_t>(lp.preheader)].count(ab->second);
        bool inBounds = allocated && size >= 0 && q.arg2.kind == OperandKind::INT_CONST &&
                        q.arg2.ival >= 0 && q.arg2.ival < size;
        bool firstInHeader = block == lp.head && !effectBefore;
        return inBounds || firstInHeader;
    }
};

}  // namespace

std::unique_ptr<Pass> makeSSALICMPass() { return std::make_unique<SSALICM>(); }

}  // namespace minic
