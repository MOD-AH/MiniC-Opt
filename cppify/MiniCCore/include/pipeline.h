// Cppify — configurable optimization pipeline and technique registry
// (roadmap 1.3, 1.4)
//
// A pipeline is a list of groups. Each group sweeps its passes in the order
// given, either to a fixed point or a fixed number of times. A group of SSA
// passes is wrapped in SSA construction and destruction by the backend; the
// user never lists those, and cannot mix SSA and plain-TAC passes in one
// group (a TAC pass would not understand phis). Every pipeline, whether a
// preset, a --config file or an --opt= list, goes through resolvePipeline(),
// so a bad configuration is rejected with a reason before anything runs.
#pragma once
#include "pass.h"
#include "trace.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace minic {

struct PipelineGroup {
    std::vector<std::string> passes;
    bool fixpoint = true;   // sweep until nothing changes (at most 32 sweeps) ...
    int  sweeps   = 32;     // ... or exactly this many when fixpoint is false
    bool ssa      = false;  // set by resolvePipeline
};

struct Pipeline {
    std::string preset = "custom";
    std::vector<PipelineGroup> groups;

    std::string describe() const;   // the effective order, for --print-pipeline
    std::string toJson() const;     // the same, for --trace-json
};

// correctness (no passes), balanced (~-O1), classic (P1-P9, the original
// compiler's pipeline), optimized (~-O2: SSA passes, then P1-P9 without P7;
// see presetPipeline for why).
const std::vector<std::string>& presetNames();
bool presetPipeline(const std::string& name, Pipeline& out, std::string& err);

// A --config file: {"preset": "<name>"} or
// {"groups": [{"passes": ["sccp", "gvn"], "repeat": "fixpoint" | <n>}, ...]}.
bool parsePipelineConfig(const std::string& json, Pipeline& out, std::string& err);

// --opt=a,b,c: the named passes in their canonical order, SSA passes in one
// group ahead of the TAC passes. Kept for compatibility with the original
// compiler's flag; `ssa` alone means construct and destruct only.
bool pipelineFromPassList(const std::vector<std::string>& names, Pipeline& out, std::string& err);

// Checks names, availability and grouping, and marks SSA groups.
bool resolvePipeline(Pipeline& p, std::string& err);

struct PipelineRun {
    std::vector<PassManager::PassReport> report;
    std::map<std::string, AnalysisManager::Counts> analyses;
    int  sweeps = 0;
    bool fixedPoint = true;
};

// Runs a resolved pipeline on one function's CFG. `globals` goes to P5 (see
// makeDeadCodeEliminationPass). With `verify`, the IR is checked before the
// first pass and after every pass; a violation throws VerifyError.
void runPipeline(CFG& cfg, const Pipeline& p, const std::set<std::string>& globals,
                 const std::string& function, bool verify, TraceRecorder* recorder, PipelineRun& out);

// ---- technique registry -----------------------------------------------------
// The single list of what Cppify implements, stage by stage. The app reads it
// from --list-techniques; registry.json in the repo is that output, checked in.
struct Technique {
    std::string id;
    std::string name;
    int stage = 0;                       // 1 lexer ... 7 execution
    std::string kind;                    // lexer, parser, semantic, ir, analysis, transform-tac, transform-ssa, engine, check
    std::string status;                  // Available, Planned
    std::vector<std::string> requires;
    std::vector<std::string> conflictsWith;
    std::string description;
    std::string test;                    // the test that pins it (Available only)
};
const std::vector<Technique>& techniqueRegistry();
std::string registryJson();

}  // namespace minic
