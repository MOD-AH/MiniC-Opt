// MiniC-Opt — P6: unreachable-code elimination
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// Precondition: CFG reachability (CFG::reachableFromEntry(), already
// implemented in Step 5) plus the branch's own condition operand having
// become a compile-time constant — usually the result of P2 constant
// propagation reaching an IF_FALSE's arg1 (readsOf/readSlotsOf already
// treat IF_FALSE's arg1 as an ordinary read slot, so P2 propagates into
// it with no changes needed here).
//
// Two sub-rewrites, both operating on BasicBlock's public succs/preds/
// code fields directly (cfg.h's frozen interface exposes these as plain
// data, not through an edge-mutation API — there is nothing to extend):
//
//   (a) Branch folding. A block ending in `ifFalse C goto L` where C is
//       now a literal constant has a statically-known direction:
//         C is false (0)  -> always jumps.    Replace with `goto L`,
//                            and sever the (now provably dead) fallthrough
//                            edge.
//         C is true (any nonzero) -> never jumps. Delete the ifFalse
//                            instruction outright (falling through is
//                            already exactly what happens with no
//                            instruction there), and sever the dead
//                            branch-target edge.
//       cfg.cpp's own leader-algorithm construction always pushes the
//       branch-target block first and the fallthrough block second for an
//       IF_FALSE terminator (see the comment there), so succs[0]/succs[1]
//       identify them without needing a separate label lookup.
//
//   (b) Dead-block elimination. Once an edge is severed, a block may no
//       longer be reachable from entry. Any such block's code is cleared
//       (dropping it to zero footprint in ir_utils.h's flatten()) and its
//       id is removed from every remaining block's preds/succs, keeping
//       the graph internally consistent for whatever pass or analysis
//       runs next. This also catches the source-level `if (0) { ... }`
//       case once its own guard has been folded by an earlier sweep.
//
// Both sub-rewrites are safe to run to a fixed point within one run():
// folding a branch can newly disconnect a block, and disconnecting a
// block can newly make ITS successors unreachable in turn.
#include "opt_passes.h"
#include "ir_utils.h"
#include <algorithm>

namespace minic {
namespace {

bool isConst(const Operand& o) {
    return o.kind == OperandKind::INT_CONST || o.kind == OperandKind::FLOAT_CONST;
}
bool truthy(const Operand& o) {
    return o.kind == OperandKind::FLOAT_CONST ? (o.fval != 0.0) : (o.ival != 0);
}

void removeEdge(CFG& cfg, int from, int to) {
    BasicBlock& f = cfg.block(from);
    BasicBlock& t = cfg.block(to);
    f.succs.erase(std::remove(f.succs.begin(), f.succs.end(), to), f.succs.end());
    t.preds.erase(std::remove(t.preds.begin(), t.preds.end(), from), t.preds.end());
}

class UnreachableCodeElimination : public Pass {
public:
    std::string name() const override { return "unreachable"; }
    std::string description() const override {
        return "unreachable-code elimination: fold a branch with a constant condition, "
               "then delete any block no longer reachable from entry";
    }
    std::string requiresAnalysis() const override { return "CFG reachability"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        size_t n = cfg.blocks().size();

        // (a) Branch folding.
        for (size_t bi = 0; bi < n; ++bi) {
            BasicBlock& b = cfg.block(static_cast<int>(bi));
            if (b.code.empty() || b.code.back().op != Op::IF_FALSE) continue;
            Quad& term = b.code.back();
            if (!isConst(term.arg1)) continue;
            if (b.succs.size() != 2) continue;   // already folded, or a malformed block — leave alone
            int branchTarget = b.succs[0];
            int fallthrough   = b.succs[1];
            if (!truthy(term.arg1)) {
                // condition is false: always taken.
                term.op = Op::GOTO;
                term.arg1 = Operand{};
                removeEdge(cfg, static_cast<int>(bi), fallthrough);
            } else {
                // condition is true: never taken.
                b.code.pop_back();
                removeEdge(cfg, static_cast<int>(bi), branchTarget);
            }
            ++count_;
        }

        // (b) Dead-block elimination, to a local fixed point (severing one
        // block's edges can strand its own successors in turn).
        bool shrunk = true;
        while (shrunk) {
            shrunk = false;
            std::set<int> reachable = cfg.reachableFromEntry();
            for (size_t bi = 0; bi < n; ++bi) {
                int id = static_cast<int>(bi);
                if (reachable.count(id)) continue;
                BasicBlock& b = cfg.block(id);
                if (b.code.empty() && b.succs.empty()) continue;   // already cleared
                for (int s : std::vector<int>(b.succs)) removeEdge(cfg, id, s);
                if (!b.code.empty()) { b.code.clear(); ++count_; }
                shrunk = true;
            }
        }

        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
};

} // namespace

std::unique_ptr<Pass> makeUnreachableCodeEliminationPass() { return std::make_unique<UnreachableCodeElimination>(); }

} // namespace minic
