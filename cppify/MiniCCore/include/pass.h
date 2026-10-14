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
#include "cfg.h"
#include "trace.h"
#include <string>
#include <vector>
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

protected:
    // Passes clear this at the top of run(), alongside resetting their count,
    // and append one entry per rewrite they actually perform.
    std::vector<TraceEdit> edits_;
};

// Runs the configured pipeline REPEATEDLY until a full sweep changes nothing
// (a fixed point). Passes enable one another — constant propagation exposes
// folding, copy propagation exposes dead code, LICM exposes further common
// subexpressions — so a single sequential sweep leaves work undone.
class PassManager {
public:
    void add(std::unique_ptr<Pass> p) { passes_.push_back(std::move(p)); }

    // Returns the number of sweeps performed. `recorder` is Cppify-fork
    // only: pass nullptr (the default) and behaviour is bit-for-bit what the
    // original terminal compiler does.
    int run(CFG& cfg, int sweepCap = 32, TraceRecorder* recorder = nullptr,
            const std::string& functionName = "");

    // --opt=licm,cse   /   --no-opt
    void enableOnly(const std::vector<std::string>& names);
    void disableAll();

    struct PassReport { std::string pass; int transforms; int instrBefore; int instrAfter; };
    const std::vector<PassReport>& report() const { return report_; }

private:
    std::vector<std::unique_ptr<Pass>> passes_;
    std::vector<PassReport> report_;
};

} // namespace minic
