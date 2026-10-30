// MiniC-Opt — P9: peephole optimization
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// Precondition: none beyond the CFG's own edges (pattern matching on the
// control-transfer instructions themselves) — the textbook description
// ("pattern matching post-linearisation") in Table 13/README's summary.
// Implemented at the quad level, on the CFG that feeds ir_utils.h's
// flatten(), rather than on a separately emitted assembly-style listing:
// this keeps peephole inside the same differential-execution safety net
// every other pass runs under (src/backend's linearised/stack-slot
// listing, added alongside this pass, is explicitly NOT covered by that
// gate — see its own file header for why).
//
//   (i)  goto-chain collapsing: `goto L1` where L1 turns out to be
//        nothing but `goto L2` gets redirected straight to L2 — the
//        worked README example (`goto L1; L1: goto L2` -> `goto L2`).
//        Followed to a fixed point (a chain of three or more collapses
//        in one run()), with a visited-set cycle guard so a source
//        program's own `L1: goto L1;` infinite loop cannot infinite-loop
//        the OPTIMIZER itself. Applied to both GOTO's and IF_FALSE's jump
//        target — an IF_FALSE branch that lands on a pure redirect is
//        just as collapsible as an unconditional one.
//   (ii) dead-goto removal: once (i) has run, a `goto` whose (possibly
//        just-redirected) target is the physically next block in
//        ir_utils.h's flatten() order is deleted outright — falling
//        through already does exactly what it did.
//
// Redirecting a jump changes which block is actually reached, so the
// CFG's own succs/preds bookkeeping is kept in sync here (removeEdge/
// addEdge below) — later passes in the same or a later sweep (LICM's
// dominators, P6's reachability) read those fields directly and would
// silently make wrong decisions against a stale graph otherwise.
#include "opt_passes.h"
#include "ir_utils.h"
#include <algorithm>
#include <set>
#include <unordered_set>

namespace minic {
namespace {

// Redirects the edge from `from` to `to` in place, replacing `to` with
// `newTo` at whatever INDEX it already occupies in `from`'s succs list,
// rather than erase-then-append. P6's branch-folding (and cfg.cpp's own
// construction) both rely on an IF_FALSE block's succs staying ordered
// as [branch-target-when-false, fallthrough-when-true] — erasing the old
// entry and pushing the new one at the end would silently swap that
// order for exactly the blocks this pass touches.
void redirectEdge(CFG& cfg, int from, int oldTo, int newTo) {
    BasicBlock& f = cfg.block(from);
    for (int& s : f.succs) if (s == oldTo) s = newTo;
    BasicBlock& o = cfg.block(oldTo);
    o.preds.erase(std::remove(o.preds.begin(), o.preds.end(), from), o.preds.end());
    cfg.block(newTo).preds.push_back(from);
}

// The block's own address, as a label a GOTO/IF_FALSE can name. Creates
// one (a fresh name, inserted as the block's new first instruction) if
// it doesn't already have one — not every block starts with a real
// LABEL quad (cfg.cpp only emits one for a block that was already a jump
// target when the CFG was built), but redirecting a jump here means it
// needs to become addressable by name now.
std::string ensureLabel(CFG& cfg, int blockId, int& freshCounter) {
    BasicBlock& b = cfg.block(blockId);
    if (!b.code.empty() && b.code.front().op == Op::LABEL) return b.code.front().result.name;

    std::unordered_set<std::string> used;
    for (const auto& blk : cfg.blocks())
        for (const Quad& q : blk.code)
            if (q.op == Op::LABEL) used.insert(q.result.name);
    std::string name;
    do { name = "__pp" + std::to_string(freshCounter++); } while (used.count(name));

    Quad lbl; lbl.op = Op::LABEL; lbl.result.kind = OperandKind::LABEL_REF; lbl.result.name = name;
    b.code.insert(b.code.begin(), lbl);
    return name;
}

// True iff `blockId`'s code is nothing but a single unconditional GOTO
// (its own leading LABEL, if any, skipped) — a pure redirect.
bool isPureGoto(const CFG& cfg, int blockId, int& targetBlock) {
    const BasicBlock& b = cfg.block(blockId);
    size_t i = 0;
    if (i < b.code.size() && b.code[i].op == Op::LABEL) ++i;
    if (i + 1 != b.code.size() || b.code[i].op != Op::GOTO) return false;
    if (b.succs.size() != 1) return false;
    targetBlock = b.succs.front();
    return true;
}

// Follows a chain of pure-redirect blocks starting at `start` to its
// final non-redirect destination, guarding against a cycle.
int resolveChain(const CFG& cfg, int start) {
    std::set<int> visited;
    int cur = start;
    while (visited.insert(cur).second) {
        int next;
        if (!isPureGoto(cfg, cur, next)) return cur;
        cur = next;
    }
    return start;   // cycle: leave the original target alone
}

class Peephole : public Pass {
public:
    std::string name() const override { return "peephole"; }
    std::string description() const override {
        return "peephole: collapse a jump-to-a-jump chain, then drop a goto to the "
               "physically next block";
    }
    std::string requiresAnalysis() const override { return "none (pattern match on control transfers)"; }
    bool preservesCFG() const override { return false; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        int fresh = 0;
        size_t n = cfg.blocks().size();

        // (i) chain collapsing
        for (size_t bi = 0; bi < n; ++bi) {
            BasicBlock& b = cfg.block(static_cast<int>(bi));
            if (b.code.empty()) continue;
            Quad& term = b.code.back();
            if (term.op != Op::GOTO && term.op != Op::IF_FALSE) continue;

            int oldTarget = (term.op == Op::GOTO) ? (b.succs.empty() ? -1 : b.succs.front())
                                                   : (b.succs.empty() ? -1 : b.succs.front());
            if (oldTarget < 0) continue;
            int finalTarget = resolveChain(cfg, oldTarget);
            if (finalTarget == oldTarget) continue;

            std::string finalLabel = ensureLabel(cfg, finalTarget, fresh);
            std::string beforeTxt = term.toString();
            term.result.kind = OperandKind::LABEL_REF;
            term.result.name = finalLabel;
            redirectEdge(cfg, static_cast<int>(bi), oldTarget, finalTarget);
            ++count_;
            {
                TraceEdit e;
                e.quadIndex = static_cast<int>(b.code.size()) - 1;
                e.before = beforeTxt;
                e.after = term.toString();
                e.kind = "jump-threading";
                e.sourceLine = term.srcLine;
                e.reason = "This jump landed on a block whose only instruction is another "
                           "jump, so it is retargeted straight at the final destination. The "
                           "successor is replaced in place rather than erased and re-appended, "
                           "which preserves the succs[] ordering that branch folding relies on.";
                edits_.push_back(std::move(e));
            }
        }

        // (ii) dead-goto removal: a goto to the physically next block.
        for (size_t bi = 0; bi + 1 < n; ++bi) {
            BasicBlock& b = cfg.block(static_cast<int>(bi));
            if (b.code.empty() || b.code.back().op != Op::GOTO) continue;
            if (b.succs.size() != 1 || b.succs.front() != static_cast<int>(bi) + 1) continue;
            {
                TraceEdit e;
                e.quadIndex = static_cast<int>(b.code.size()) - 1;
                e.before = b.code.back().toString();
                e.after = "";
                e.kind = "goto-delete";
                e.sourceLine = b.code.back().srcLine;
                e.reason = "This jump targets the block that physically follows it, so control "
                           "reaches the same place by falling through. The instruction is "
                           "redundant and is removed.";
                edits_.push_back(std::move(e));
            }
            b.code.pop_back();
            ++count_;
        }

        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
};

} // namespace

std::unique_ptr<Pass> makePeepholePass() { return std::make_unique<Peephole>(); }

} // namespace minic
