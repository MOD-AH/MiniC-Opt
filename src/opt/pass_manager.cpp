// MiniC-Opt — pass manager (implementation)
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Step 6
//
// enableOnly/disableAll are implemented by filtering passes_ itself,
// rather than by adding a separate "enabled" flag to the frozen
// PassManager class in pass.h — main.cpp builds one PassManager per
// invocation (see --opt=/--no-opt), so there is never a need to recover a
// disabled pass within the same run, and this keeps pass.h's already-
// frozen layout untouched.
#include "pass.h"
#include "opt_passes.h"
#include <algorithm>

namespace minic {

namespace {
int countInstructions(const CFG& cfg) {
    int total = 0;
    for (const auto& b : cfg.blocks()) total += static_cast<int>(b.code.size());
    return total;
}
} // namespace

int PassManager::run(CFG& cfg, int sweepCap) {
    report_.clear();
    int sweep = 0;
    bool anyChanged = true;
    // Passes enable one another (constant propagation exposes folding,
    // copy propagation exposes further propagation opportunities), so a
    // single sequential pass over the list leaves work undone — sweep
    // repeatedly until a full sweep changes nothing, exactly as pass.h's
    // class comment specifies.
    while (anyChanged && sweep < sweepCap) {
        anyChanged = false;
        ++sweep;
        for (auto& p : passes_) {
            int before = countInstructions(cfg);
            bool changed = p->run(cfg);
            int after = countInstructions(cfg);
            report_.push_back(PassReport{p->name(), p->transformCount(), before, after});
            if (changed) anyChanged = true;
        }
    }
    return sweep;
}

void PassManager::enableOnly(const std::vector<std::string>& names) {
    std::vector<std::unique_ptr<Pass>> kept;
    for (auto& p : passes_)
        if (std::find(names.begin(), names.end(), p->name()) != names.end())
            kept.push_back(std::move(p));
    passes_ = std::move(kept);
}

void PassManager::disableAll() {
    passes_.clear();
}

void registerAllPasses(PassManager& pm) {
    pm.add(makeConstantFoldingPass());
    pm.add(makeConstantPropagationPass());
    pm.add(makeCopyPropagationPass());
    pm.add(makeCommonSubexprEliminationPass());
    pm.add(makeDeadCodeEliminationPass());
    pm.add(makeUnreachableCodeEliminationPass());
    pm.add(makeStrengthReductionPass());
    pm.add(makeLoopInvariantCodeMotionPass());
    pm.add(makePeepholePass());
}

} // namespace minic
