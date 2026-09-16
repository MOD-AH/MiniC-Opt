// MiniC-Opt — shared walk for P2 (constant propagation) and P3 (copy
// propagation)
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Step 6
//
// Both passes are licensed by reaching definitions and have the same
// shape: at each use of a variable, if EXACTLY ONE definition reaches
// that point, and that definition's own right-hand side is of the kind
// this pass is willing to propagate (a constant for P2, another name for
// P3), replace the use with that right-hand side. They differ in two
// ways, both threaded through the shared walk below:
//
//   1. `eligible` — a constant for P2, a name for P3.
//   2. Soundness for a NAME source is stricter than for a constant. A
//      constant can never go stale: propagating `t1 = 4` past any amount
//      of other code is always safe. A copy `x = y` is only safe to
//      propagate into a use as long as `y` has not been reassigned
//      between the definition and that use — reaching definitions alone
//      only guarantees x's OWN definition is unique and un-killed, it
//      says nothing about y. This was caught for real: gcd.c's loop body
//      is `t#2 = b#1; t1 = a#0 % b#1; b#1 = t1; a#0 = t#2;` — naive
//      copy propagation replaced the last line's use of t#2 with b#1,
//      but b#1 had already been reassigned two lines earlier in the very
//      same block, so the substitution silently used the NEW b#1 instead
//      of the value t#2 actually captured. The differential-execution
//      check (unoptimized vs `--opt=copyprop` on the same 12 benchmarks)
//      is what surfaced this — exactly the failure mode that check
//      exists to catch.
//
// The fix implemented below: (a) the walk tracks, for every reaching
// definition currently in scope, whether it originated *within this same
// block's own replay* or flowed in from a predecessor untouched, and (b)
// whenever any variable is redefined, every currently-tracked copy fact
// whose SOURCE is that variable is invalidated immediately — so a stale
// copy can never survive past the redefinition that broke it. P3 then
// additionally restricts itself to same-block-local facts only: sound but
// conservative, since a source redefinition happening in some other block
// on the path between definition and use is real but is not tracked by
// this per-block replay. P2 has no such restriction, having no such
// hazard.
//
// This is an implementation-detail header for src/opt/*.cpp — not
// published under include/, since nothing outside this directory needs
// it (quote-includes search the including file's own directory first,
// which is why "propagation_util.h" resolves without an -I flag here).
#pragma once
#include "cfg.h"
#include "ir_utils.h"
#include "dataflow_analyses.h"
#include <functional>
#include <set>
#include <unordered_map>

namespace minic {

// eligible(definingQuad, definitionIsLocalToThisBlock, out) -> substitute?
inline int propagateInBlock(BasicBlock& block, const std::set<int>& blockIn,
                             const std::vector<std::string>& defVar, const std::vector<Quad>& defQuad,
                             const std::vector<int>& defIdOfQuadInBlock,
                             const std::function<bool(const Quad&, bool, Operand&)>& eligible) {
    std::unordered_map<std::string, std::set<int>> current;
    for (int id : blockIn) current[defVar[static_cast<size_t>(id)]].insert(id);
    std::set<std::string> definedLocally;   // vars whose `current` entry was set by THIS block's own replay

    int changes = 0;
    for (size_t qi = 0; qi < block.code.size(); ++qi) {
        Quad& q = block.code[qi];
        for (Operand* slot : readSlotsOf(q)) {
            auto it = current.find(slot->name);
            if (it == current.end() || it->second.size() != 1) continue;   // no def, or ambiguous (merge point)
            int defId = *it->second.begin();
            bool isLocal = definedLocally.count(slot->name) > 0;
            Operand replacement;
            if (eligible(defQuad[static_cast<size_t>(defId)], isLocal, replacement)) {
                *slot = replacement;
                ++changes;
            }
        }
        int myDefId = defIdOfQuadInBlock[qi];
        if (myDefId >= 0) {
            const std::string& myVar = defVar[static_cast<size_t>(myDefId)];
            // This definition (1) replaces whatever previously tracked
            // myVar, and (2) invalidates any OTHER tracked copy fact whose
            // source is myVar — see the soundness note above.
            for (auto it = current.begin(); it != current.end(); ) {
                if (it->first == myVar) { it = current.erase(it); continue; }
                bool stale = false;
                for (int id : it->second) {
                    const Quad& dq = defQuad[static_cast<size_t>(id)];
                    if (dq.op == Op::COPY && isNameOperand(dq.arg1) && dq.arg1.name == myVar) { stale = true; break; }
                }
                if (stale) it = current.erase(it); else ++it;
            }
            current[myVar] = {myDefId};
            definedLocally.insert(myVar);
        }
    }
    return changes;
}

// Runs propagateInBlock over every block of `cfg`, using a freshly
// recomputed reaching-definitions analysis (the CFG may have changed
// since the last sweep — see PassManager::run) — this is the piece both
// ConstantPropagation::run and CopyPropagation::run share verbatim.
inline int propagateOverCfg(CFG& cfg, const std::function<bool(const Quad&, bool, Operand&)>& eligible) {
    ReachingDefFacts facts = computeReachingDefsRaw(cfg);
    int total = 0;
    for (size_t bi = 0; bi < cfg.blocks().size(); ++bi) {
        total += propagateInBlock(cfg.block(static_cast<int>(bi)), facts.in[bi],
                                   facts.defVar, facts.defQuad, facts.defIdOfQuad[bi], eligible);
    }
    return total;
}

} // namespace minic
