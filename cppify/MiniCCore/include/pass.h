// MiniC-Opt — optimization pass interface and pass manager
// Owner: Member 4 (Implementation, Testing and Planning Coordinator)
//
// STATUS: DRAFT — circulated for team review ahead of the Week-4 freeze.
//
// CONTRACT FOR EVERY PASS
//   Each pass must be documented as:  precondition (from a named analysis)
//   ⇒ rewrite.  A transformation that is not licensed by an analysis result
//   does not belong in this codebase.
#pragma once
#include "analysis_manager.h"
#include "cfg.h"
#include "trace.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace minic {

class Pass {
public:
    virtual ~Pass() = default;

    virtual std::string name()        const = 0;   // e.g. "licm"
    virtual std::string description() const = 0;   // one line, shown by --help

    // Which analysis licenses this pass. Documentation and a sanity check.
    virtual std::string requiresAnalysis() const = 0;

    // Transform the CFG. Return true if anything changed.
    virtual bool run(CFG& cfg) = 0;

    // How many transformations the last run() performed — this is the number
    // reported per pass in the metrics table.
    virtual int transformCount() const = 0;

    // Cppify fork only. Per-rewrite detail for the Processing panel. A pass
    // that has not been instrumented returns an empty vector and still
    // reports its count normally, so instrumentation can land one pass at a
    // time without the others regressing.
    const std::vector<TraceEdit>& traceEdits() const { return edits_; }

    // False if run() may add, remove or retarget a CFG edge. A pass that
    // keeps the edges lets cached dominators survive its changes.
    virtual bool preservesCFG() const { return true; }

    // Set by the PassManager before run(). A pass driven directly, as the
    // unit tests do, gets a fresh manager per request instead, so it never
    // sees a stale result.
    void setAnalysisManager(AnalysisManager* am) { am_ = am; }

protected:
    AnalysisManager& analyses() {
        if (am_) return *am_;
        own_ = std::make_unique<AnalysisManager>();
        return *own_;
    }

    // Passes clear this at the top of run(), alongside resetting their count,
    // and append one entry per rewrite they actually perform.
    std::vector<TraceEdit> edits_;

private:
    AnalysisManager* am_ = nullptr;
    std::unique_ptr<AnalysisManager> own_;
};

// Runs the configured pipeline REPEATEDLY until a full sweep changes nothing
// (a fixed point). Passes enable one another — constant propagation exposes
// folding, copy propagation exposes dead code, LICM exposes further common
// subexpressions — so a single sequential sweep leaves work undone.
class PassManager {
public:
    void add(std::unique_ptr<Pass> p) { passes_.push_back(std::move(p)); }

    // Returns the number of sweeps performed: at most `sweepCap`, fewer if a
    // sweep changes nothing. `recorder` is Cppify-fork only: pass nullptr (the
    // default) and behaviour is bit-for-bit what the original terminal
    // compiler does. Trace sweeps are numbered from `firstSweep`, so several
    // pipeline groups run on one function read as one continuous trace.
    int run(CFG& cfg, int sweepCap = 32, TraceRecorder* recorder = nullptr, int firstSweep = 1);

    // True if the last run() stopped because a sweep changed nothing, rather
    // than because it hit the cap.
    bool converged() const { return converged_; }

    // Run verifyCFG() after every pass; a violation throws VerifyError naming
    // the pass. `function` only labels the message.
    void setVerify(bool on, const std::string& function = "") { verify_ = on; function_ = function; }

    // --opt=licm,cse   /   --no-opt
    void enableOnly(const std::vector<std::string>& names);
    void disableAll();

    struct PassReport { std::string pass; int transforms; int instrBefore; int instrAfter; };
    const std::vector<PassReport>& report() const { return report_; }
    const std::map<std::string, AnalysisManager::Counts>& analysisCounts() const { return analysisCounts_; }

private:
    std::vector<std::unique_ptr<Pass>> passes_;
    std::vector<PassReport> report_;
    std::map<std::string, AnalysisManager::Counts> analysisCounts_;
    bool converged_ = false;
    bool verify_ = false;
    std::string function_;
};

} // namespace minic
