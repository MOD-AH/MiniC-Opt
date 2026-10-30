// Cppify — analysis cache (roadmap 1.2)
//
// Before this, every pass recomputed what it needed from scratch: constprop
// and copyprop each solved reaching definitions on the same IR, LICM built
// dominators twice per run, and the last, no-change sweep of the fixed point
// recomputed everything only to confirm nothing moved. A pass now asks the
// manager; the PassManager drops cached results after a pass reports a
// change. Dominators depend only on the edges, so they survive a pass that
// declares it left the edges alone (Pass::preservesCFG).
#pragma once
#include "cfg.h"
#include "dataflow_analyses.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace minic {

class AnalysisManager {
public:
    struct Counts { int computed = 0; int reused = 0; };

    const std::vector<std::set<int>>& dominators(const CFG& cfg) {
        return get(dom_, "dominators", [&] { return cfg.computeDominators(); });
    }
    const ReachingDefFacts& reachingDefs(CFG& cfg) {
        return get(reach_, "reaching definitions", [&] { return computeReachingDefsRaw(cfg, true); });
    }
    const DataFlowResult& liveVariables(CFG& cfg) {
        return get(live_, "live variables", [&] { return computeLiveVariables(cfg, IRFunction{}); });
    }

    void invalidate(bool edgesChanged) {
        reach_.reset();
        live_.reset();
        if (edgesChanged) dom_.reset();
    }

    const std::map<std::string, Counts>& counts() const { return counts_; }

private:
    template <class T, class F>
    const T& get(std::unique_ptr<T>& slot, const char* name, F compute) {
        Counts& c = counts_[name];
        if (slot) { ++c.reused; return *slot; }
        ++c.computed;
        slot = std::make_unique<T>(compute());
        return *slot;
    }

    std::unique_ptr<std::vector<std::set<int>>> dom_;
    std::unique_ptr<ReachingDefFacts> reach_;
    std::unique_ptr<DataFlowResult> live_;
    std::map<std::string, Counts> counts_;
};

}  // namespace minic
