// MiniC-Opt (Cppify fork) — optimization trace recording
//
// This header exists only in the vendored copy that backs the Cppify macOS
// app. The original terminal compiler is untouched.
//
// Why this exists: PassManager::PassReport records {pass, transforms,
// instrBefore, instrAfter} — counts only. That is enough for --metrics but
// not enough to explain an optimization to a human, which needs to know
// WHICH instruction changed and WHY it was legal to change it. The "why"
// has to come from the pass itself, because the pass is the only thing that
// knows which analysis fact it tested. A UI diffing before/after IR could
// guess, and a guess dressed as an explanation is worse than no explanation.
//
// Recording is opt-in: with no recorder attached the passes and the manager
// behave exactly as before, which is what keeps the 597/950 baseline valid.
#pragma once
#include <string>
#include <vector>

namespace minic {

// One individual rewrite performed by one pass.
struct TraceEdit {
    std::string function;
    int         quadIndex  = -1;   // index within the flattened function
    std::string before;            // quad text before the rewrite ("" if inserted)
    std::string after;             // quad text after  the rewrite ("" if deleted)
    std::string kind;              // "fold", "delete", "operand-substitution", ...
    std::string reason;            // written by the pass, naming the licensing fact
    int         sourceLine = 0;    // originating line in the C source, 0 if unknown
};

// One pass, run once, within one sweep.
struct TracePass {
    std::string pass;              // "constprop"
    std::string title;             // "Constant Propagation"
    std::string precondition;      // the analysis that licenses it
    std::string description;       // the pass's own one-line description()
    int  transforms  = 0;
    int  instrBefore = 0;
    int  instrAfter  = 0;
    bool changed     = false;
    // Snapshots are only taken when the pass actually changed something;
    // an untouched pass would otherwise duplicate the previous listing for
    // no benefit. On worked_example.c that is ~10 real snapshots out of 36
    // pass invocations.
    std::vector<std::string> irBefore;
    std::vector<std::string> irAfter;
    std::vector<TraceEdit>   edits;
};

struct TraceSweep {
    int sweep = 0;
    std::vector<TracePass> passes;
};

// Everything recorded for one function's optimization.
struct FunctionTrace {
    std::string name;
    std::vector<TraceSweep> sweeps;
    int  sweepCount        = 0;
    bool fixedPointReached = false;
    std::vector<std::string> irInitial;
    std::vector<std::string> irFinal;
};

class TraceRecorder {
public:
    void beginFunction(const std::string& name, std::vector<std::string> irInitial) {
        functions_.push_back(FunctionTrace{});
        functions_.back().name = name;
        functions_.back().irInitial = std::move(irInitial);
    }
    void endFunction(int sweepCount, bool fixedPoint, std::vector<std::string> irFinal) {
        if (functions_.empty()) return;
        functions_.back().sweepCount = sweepCount;
        functions_.back().fixedPointReached = fixedPoint;
        functions_.back().irFinal = std::move(irFinal);
    }
    void beginSweep(int n) {
        if (functions_.empty()) return;
        functions_.back().sweeps.push_back(TraceSweep{n, {}});
    }
    void recordPass(TracePass p) {
        if (functions_.empty() || functions_.back().sweeps.empty()) return;
        functions_.back().sweeps.back().passes.push_back(std::move(p));
    }
    const std::vector<FunctionTrace>& functions() const { return functions_; }

private:
    std::vector<FunctionTrace> functions_;
};

}  // namespace minic
