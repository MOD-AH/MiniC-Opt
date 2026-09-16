// MiniC-Opt — P4: common subexpression elimination
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// Precondition: available expressions (forward, intersection — Step 5
// already builds the cross-block version for --dump-dataflow=avail). This
// pass, like P3 copy propagation, is deliberately restricted to a single
// forward walk local to one block, for the same reason P3 restricts
// itself to isLocal facts (see propagation_util.h's file comment):
// availability alone tells you an expression's OPERANDS haven't changed
// since it was last computed, but it does NOT tell you which variable
// currently still holds that value — a second, separate piece of
// bookkeeping the textbook cross-block solver in dataflow.cpp does not
// carry (its DataFlowResult is decoded to description strings for
// --dump-dataflow, not to a "holder variable" per fact). Tracking that
// holder correctly only within one block's own forward replay avoids
// having to reconcile which variable would hold the same expression
// arriving from two different predecessor paths.
//
// Rewrite: the second computation of an expression already available in
// this block — because its operands have not been redefined since — is
// replaced with a COPY of whichever variable that first computation wrote
// into (`t2 = a * b` -> `t2 = t`, exactly Annexure C's worked example).
// Precisely the same reactive-invalidation discipline as P3: whenever
// this block (re)defines a name, any tracked fact whose LHS operand, RHS
// operand, or holder matches that name is dropped immediately.
#include "opt_passes.h"
#include "ir_utils.h"
#include <string>
#include <unordered_map>

namespace minic {
namespace {

struct AvailFact { Operand a, b, holder; };

class CommonSubexprElimination : public Pass {
public:
    std::string name() const override { return "cse"; }
    std::string description() const override {
        return "common subexpression elimination: reuse a same-block computation "
               "whose operands have not changed since";
    }
    std::string requiresAnalysis() const override { return "available expressions"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        for (size_t bi = 0; bi < cfg.blocks().size(); ++bi)
            runOnBlock(cfg.block(static_cast<int>(bi)));
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;

    static void invalidate(std::unordered_map<std::string, AvailFact>& avail, const std::string& redefined) {
        for (auto it = avail.begin(); it != avail.end(); ) {
            const AvailFact& f = it->second;
            bool stale = (isNameOperand(f.a) && f.a.name == redefined) ||
                         (isNameOperand(f.b) && f.b.name == redefined) ||
                         (isNameOperand(f.holder) && f.holder.name == redefined);
            if (stale) it = avail.erase(it); else ++it;
        }
    }

    void runOnBlock(BasicBlock& b) {
        std::unordered_map<std::string, AvailFact> avail;   // exprKey -> fact
        for (Quad& q : b.code) {
            if (isBinaryExprOp(q.op)) {
                std::string key = opTag(q.op) + ":" + operandKey(q.arg1) + "," + operandKey(q.arg2);
                auto it = avail.find(key);
                bool reuse = (it != avail.end());
                Operand holder = reuse ? it->second.holder : Operand{};
                if (isNameOperand(q.result)) invalidate(avail, q.result.name);
                if (reuse) {
                    q.op = Op::COPY;
                    q.arg1 = holder;
                    q.arg2 = Operand{};
                    ++count_;
                } else if (isNameOperand(q.result)) {
                    avail[key] = AvailFact{q.arg1, q.arg2, q.result};
                }
                continue;
            }
            Operand w;
            if (writesOf(q, w)) invalidate(avail, w.name);
        }
    }
};

} // namespace

std::unique_ptr<Pass> makeCommonSubexprEliminationPass() { return std::make_unique<CommonSubexprElimination>(); }

} // namespace minic
