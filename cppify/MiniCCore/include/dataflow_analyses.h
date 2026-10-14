// MiniC-Opt — the three instantiations of the generic data-flow solver
// Owner: Member 3 (System Designer / Core Algorithms) / Step 5
//
// solve() (cfg.h/src/cfg/dataflow.cpp) only knows sets of small integers
// and a gen/kill pair per block; it has no idea what a "definition", an
// "expression" or a "variable" is. Each function below is the missing
// half: it builds that block's universe (assigning each fact — a
// definition site, an expression, a variable name — a small integer id),
// fills BasicBlock::gen/kill from the IRFunction's actual quads, calls
// solve(), and decodes the resulting IN/OUT integer sets back into
// readable strings for --dump-dataflow and for hand spot-checking against
// docs/worked_example.md.
#pragma once
#include "cfg.h"
#include "ir.h"
#include <set>
#include <string>
#include <vector>

namespace minic {

// Per-block IN/OUT sets, already decoded to readable strings, in the same
// order as cfg.blocks().
struct DataFlowResult {
    std::vector<std::set<std::string>> in;
    std::vector<std::set<std::string>> out;
};

// Forward, union. Fact = one definition site ("var := <quad text>").
// IN[entry] = ∅: nothing reaches the function before it starts.
DataFlowResult computeReachingDefs(CFG& cfg, const IRFunction& fn);

// The same analysis, undecoded — Step 6's constant/copy propagation (P2,
// P3) need the actual defining Quad for each reaching definition (to
// check "is this a copy of a constant" / "is this a copy of another
// name"), not a printable description of it. defVar/defQuad are indexed
// by definition id; defIdOfQuad[bi][qi] is that quad's own definition id
// (or -1) — handed back so a pass can update its local "what reaches
// here right now" set as it walks forward through a block, in step with
// exactly the same ids the solved in/out sets use, instead of re-deriving
// them. in/out are indexed by block id, same order as cfg.blocks().
struct ReachingDefFacts {
    std::vector<std::string> defVar;
    std::vector<Quad> defQuad;
    std::vector<std::vector<int>> defIdOfQuad;
    std::vector<std::set<int>> in, out;
};
ReachingDefFacts computeReachingDefsRaw(CFG& cfg);

// Forward, intersection. Fact = one syntactic expression ("a#0 + b#1").
// Restricted to the classic redundancy-prone case — a binary
// arithmetic/relational/logical quad assigned to a variable or temp — the
// same scope Annexure C's worked example targets (P4 common-subexpression
// elimination). IN[entry] = ∅.
DataFlowResult computeAvailableExpressions(CFG& cfg, const IRFunction& fn);

// Backward, union. Fact = one variable name. OUT[exit] = ∅ for every block
// with no successors (every `return`, so every reachable exit point).
DataFlowResult computeLiveVariables(CFG& cfg, const IRFunction& fn);

} // namespace minic
