// MiniC-Opt — P8: loop-invariant code motion
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// Precondition: dominators, back edges, natural loops (all Step 5), plus
// the single-preheader shape loop_util.h restricts itself to.
//
// A quad `r = <safe op> operand(s)` inside a loop is hoisted to the
// preheader when:
//   1. every read operand is either a constant or a name never written
//      anywhere in the loop's block set (classic invariance — see the
//      Annexure C worked example, where `a` and `b` are invariant because
//      only the loop's preheader-side code ever assigns them);
//   2. `r` itself is written exactly once in the whole loop (the usual
//      "no other definition could reach a use with a different value"
//      precondition — a variable reused as scratch for something else
//      each iteration must never be hoisted);
//   3. the defining block DOMINATES the loop's tail block, i.e. it runs
//      on *every* iteration, not just some — hoisting a computation that
//      only ran on some iterations (behind an inner if) into a preheader
//      that always runs would change behavior the first time the loop
//      runs zero times or takes a path that used to skip it.
//   (isSafeToRelocate in loop_util.h independently rules out anything
//   that could trap — DIV/MOD by zero, an out-of-bounds LOAD_INDEX —
//   since hoisting one of those out from behind the loop's own condition
//   check would make it execute even on a zero-iteration run.)
//
// Hoisting one instruction can make another, previously non-invariant
// one become invariant in turn (its operand was only "loop-defined"
// because of the instruction just removed) — handled by re-scanning the
// loop to a local fixed point within run(), the same style P2/P3/P4 use
// for their own within-block chains.
#include "opt_passes.h"
#include "loop_util.h"
#include <set>

namespace minic {
namespace {

int countDefs(CFG& cfg, const std::set<int>& blocks, const std::string& name) {
    int n = 0;
    for (int bi : blocks)
        for (const Quad& q : cfg.block(bi).code) {
            Operand w;
            if (writesOf(q, w) && w.name == name) ++n;
        }
    return n;
}

class LoopInvariantCodeMotion : public Pass {
public:
    std::string name() const override { return "licm"; }
    std::string description() const override {
        return "loop-invariant code motion: hoist a pure computation whose operands never "
               "change in the loop to a single usable preheader block";
    }
    std::string requiresAnalysis() const override { return "dominators, back edges, natural loops"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        std::vector<NaturalLoop> loops = findNaturalLoops(cfg);
        if (loops.empty()) return false;
        std::vector<std::set<int>> dom = cfg.computeDominators();

        for (const NaturalLoop& lp : loops) {
            if (lp.preheader < 0) continue;   // no usable single preheader — leave this loop alone
            bool changed = true;
            while (changed) {
                changed = false;
                std::set<std::string> definedInLoop = namesDefinedIn(cfg, lp.blocks);
                for (int bi : lp.blocks) {
                    if (!dom[static_cast<size_t>(lp.tail)].count(bi)) continue;   // doesn't run every iteration
                    BasicBlock& b = cfg.block(bi);
                    for (size_t qi = 0; qi < b.code.size(); ++qi) {
                        Quad& q = b.code[qi];
                        if (!isSafeToRelocate(q.op)) continue;
                        Operand w;
                        if (!writesOf(q, w)) continue;
                        bool invariant = true;
                        for (const Operand& r : readsOf(q))
                            if (definedInLoop.count(r.name)) { invariant = false; break; }
                        if (!invariant) continue;
                        if (countDefs(cfg, lp.blocks, w.name) != 1) continue;

                        Quad hoisted = q;
                        b.code.erase(b.code.begin() + static_cast<long>(qi));
                        appendToPreheader(cfg, lp.preheader, {hoisted});
                        definedInLoop.erase(w.name);
                        ++count_;
                        changed = true;
                        break;   // block's code vector shifted; restart the scan
                    }
                    if (changed) break;
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

std::unique_ptr<Pass> makeLoopInvariantCodeMotionPass() { return std::make_unique<LoopInvariantCodeMotion>(); }

} // namespace minic
