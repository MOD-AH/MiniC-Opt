// MiniC-Opt — P3: copy propagation
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Step 6
//
// Precondition: reaching definitions (the same analysis P2 uses, applied
// to a different kind of definition). Rewrite: a use of v is replaced by
// w when exactly one definition reaches that use and that definition is
// itself a plain copy `v = w` (w a name, not a constant — that case is
// P2's job so the two passes never fight over the same rewrite).
#include "opt_passes.h"
#include "propagation_util.h"

namespace minic {
namespace {

class CopyPropagation : public Pass {
public:
    std::string name() const override { return "copyprop"; }
    std::string description() const override {
        return "copy propagation: replace a use with its source name when its one reaching definition is a plain copy `v = w`";
    }
    std::string requiresAnalysis() const override { return "reaching definitions"; }

    bool run(CFG& cfg) override {
        // Restricted to same-block-local reaching definitions only — see
        // propagation_util.h's file header for why a cross-block copy
        // fact cannot be soundly propagated by this per-block replay: a
        // redefinition of the copy's SOURCE in some other block on the
        // path to the use would not be seen. Same-block facts are safe
        // because propagateInBlock invalidates a tracked copy the moment
        // its own source is redefined, so nothing stale ever survives to
        // reach the eligibility check below.
        count_ = propagateOverCfg(cfg, [](const Quad& def, bool isLocal, Operand& out) {
            if (!isLocal) return false;
            if (def.op != Op::COPY) return false;
            if (!isNameOperand(def.arg1)) return false;   // a constant source is P2's job, not this pass's
            out = def.arg1;
            return true;
        });
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
};

} // namespace

std::unique_ptr<Pass> makeCopyPropagationPass() { return std::make_unique<CopyPropagation>(); }

} // namespace minic
