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

// Flattens the CFG to one printable line per quad, in block order, which is
// the form the Processing panel diffs. Block labels are included so a reader
// can see control flow, matching what --dump-ir shows.
std::vector<std::string> snapshot(const CFG& cfg) {
    std::vector<std::string> out;
    for (const auto& b : cfg.blocks())
        for (const Quad& q : b.code)
            out.push_back(q.toString());
    return out;
}
} // namespace

int PassManager::run(CFG& cfg, int sweepCap, TraceRecorder* recorder,
                     const std::string& functionName) {
    report_.clear();
    if (recorder) recorder->beginFunction(functionName, snapshot(cfg));
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
        if (recorder) recorder->beginSweep(sweep);
        for (auto& p : passes_) {
            int before = countInstructions(cfg);
            std::vector<std::string> irBefore;
            if (recorder) irBefore = snapshot(cfg);
            bool changed = p->run(cfg);
            int after = countInstructions(cfg);
            report_.push_back(PassReport{p->name(), p->transformCount(), before, after});
            if (recorder) {
                TracePass tp;
                tp.pass         = p->name();
                tp.description  = p->description();
                tp.precondition = p->requiresAnalysis();
                tp.transforms   = p->transformCount();
                tp.instrBefore  = before;
                tp.instrAfter   = after;
                tp.changed      = changed;
                tp.edits        = p->traceEdits();
                // Snapshots only when something moved; an inert pass would
                // otherwise duplicate the previous listing for no benefit.
                if (changed) {
                    tp.irBefore = std::move(irBefore);
                    tp.irAfter  = snapshot(cfg);
                }
                recorder->recordPass(std::move(tp));
            }
            if (changed) anyChanged = true;
        }
    }
    if (recorder) recorder->endFunction(sweep, !anyChanged, snapshot(cfg));
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

void registerAllPasses(PassManager& pm, std::set<std::string> globals) {
    pm.add(makeConstantFoldingPass());
    pm.add(makeConstantPropagationPass());
    pm.add(makeCopyPropagationPass());
    pm.add(makeCommonSubexprEliminationPass());
    pm.add(makeDeadCodeEliminationPass(std::move(globals)));
    pm.add(makeUnreachableCodeEliminationPass());
    pm.add(makeStrengthReductionPass());
    pm.add(makeLoopInvariantCodeMotionPass());
    pm.add(makePeepholePass());
}

} // namespace minic
