// MiniC-Opt — P2: constant propagation
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Step 6
//
// Precondition: reaching definitions. Rewrite: a use of v is replaced by
// CONST when exactly one definition reaches that use and that definition
// is itself `v = CONST` (a COPY quad — as produced by IRGen's own literal
// codegen, and by P1 constant folding once it fires). See
// propagation_util.h for the shared per-instruction reaching-def replay
// this and copy_propagation.cpp both drive with different `eligible`
// predicates.
#include "opt_passes.h"
#include "propagation_util.h"

namespace minic {
namespace {

class ConstantPropagation : public Pass {
public:
    std::string name() const override { return "constprop"; }
    std::string description() const override {
        return "constant propagation: replace a use with its value when its one reaching definition is `v = CONST`";
    }
    std::string requiresAnalysis() const override { return "reaching definitions"; }

    bool run(CFG& cfg) override {
        edits_.clear();
        // No locality restriction: a constant can never be invalidated by
        // another variable's redefinition, so a cross-block reaching
        // definition is just as safe to propagate as a same-block one.
        count_ = propagateOverCfg(cfg, analyses().reachingDefs(cfg), [](const Quad& def, bool /*isLocal*/, Operand& out) {
            if (def.op != Op::COPY) return false;
            if (def.arg1.kind != OperandKind::INT_CONST && def.arg1.kind != OperandKind::FLOAT_CONST) return false;
            out = def.arg1;
            return true;
        },
        &edits_, "operand-substitution",
        [](const Quad& def, const std::string& used) {
            return "'" + used + "' has exactly one reaching definition at this point (" +
                   def.toString() + "), and that definition assigns a compile-time constant, "
                   "so the use is replaced by the constant itself. A constant can never go "
                   "stale, so no locality restriction is needed.";
        });

        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
};

} // namespace

std::unique_ptr<Pass> makeConstantPropagationPass() { return std::make_unique<ConstantPropagation>(); }

} // namespace minic
