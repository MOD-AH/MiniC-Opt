// Cppify — configurable optimization pipeline and technique registry
// (see include/pipeline.h)
#include "pipeline.h"
#include "json_lite.h"
#include "opt_passes.h"
#include "ssa.h"
#include "trace_json.h"
#include "verify.h"
#include <algorithm>
#include <sstream>

namespace minic {
namespace {

const std::vector<std::string> kTacOrder = {"fold", "constprop", "copyprop", "cse", "dce",
                                            "unreachable", "strength", "licm", "peephole"};
const std::vector<std::string> kSsaOrder = {"sccp", "gvn", "licm-ssa", "ssa-dce"};

bool isSsaPass(const std::string& n) {
    return std::find(kSsaOrder.begin(), kSsaOrder.end(), n) != kSsaOrder.end();
}

std::unique_ptr<Pass> makePass(const std::string& n, const std::set<std::string>& globals) {
    if (n == "fold")        return makeConstantFoldingPass();
    if (n == "constprop")   return makeConstantPropagationPass();
    if (n == "copyprop")    return makeCopyPropagationPass();
    if (n == "cse")         return makeCommonSubexprEliminationPass();
    if (n == "dce")         return makeDeadCodeEliminationPass(globals);
    if (n == "unreachable") return makeUnreachableCodeEliminationPass();
    if (n == "strength")    return makeStrengthReductionPass();
    if (n == "licm")        return makeLoopInvariantCodeMotionPass();
    if (n == "peephole")    return makePeepholePass();
    if (n == "sccp")        return makeSCCPPass();
    if (n == "gvn")         return makeGVNPass();
    if (n == "licm-ssa")    return makeSSALICMPass();
    if (n == "ssa-dce")     return makeSSADCEPass();
    return nullptr;
}

// Called only outside SSA form (before the first group, after the last).
std::vector<std::string> snapshot(const CFG& cfg) {
    std::vector<std::string> out;
    for (const auto& b : cfg.blocks())
        for (const Quad& q : b.code) out.push_back(q.toString());
    return out;
}

void verifyOrThrow(const CFG& cfg, const std::string& when) {
    std::vector<std::string> errs = verifyCFG(cfg);
    if (errs.empty()) return;
    std::string msg = "IR verifier failed " + when + ":";
    for (const auto& e : errs) msg += "\n  " + e;
    throw VerifyError{msg};
}

std::string groupLine(const PipelineGroup& g) {
    std::string list;
    for (size_t i = 0; i < g.passes.size(); ++i) list += (i ? ", " : "") + g.passes[i];
    std::string body = g.passes.empty() ? "" :
        "[" + list + "] " + (g.fixpoint ? "to a fixed point (at most 32 sweeps)"
                                        : (g.sweeps == 1 ? "once" : std::to_string(g.sweeps) + " sweeps"));
    if (!g.ssa) return body;
    return "ssa -> " + (body.empty() ? std::string() : body + " -> ") + "out-of-ssa";
}

}  // namespace

const std::vector<std::string>& presetNames() {
    static const std::vector<std::string> names = {"correctness", "balanced", "classic", "optimized"};
    return names;
}

bool presetPipeline(const std::string& name, Pipeline& out, std::string& err) {
    out = Pipeline{};
    out.preset = name;
    if (name == "correctness") return true;
    if (name == "balanced") {
        out.groups.push_back({{"fold", "constprop", "copyprop", "dce", "unreachable", "peephole"}, true, 32, false});
    } else if (name == "classic") {
        out.groups.push_back({kTacOrder, true, 32, false});
    } else if (name == "optimized") {
        // P7 is left out. The interpreter charges every quad the same, so
        // trading `i * K` for an add saves nothing and the mirror's update
        // costs one more instruction per iteration: measured on the 75
        // programs, optimized ran 61 more instructions with it than without
        // (classic: 50 more). On hardware, where a multiply costs more than
        // an add, it would earn its place; classic keeps it unchanged.
        // (Numbers: docs/benchmarks.md.)
        std::vector<std::string> tac;
        for (const auto& n : kTacOrder) if (n != "strength") tac.push_back(n);
        out.groups.push_back({kSsaOrder, true, 32, false});
        out.groups.push_back({tac, true, 32, false});
    } else {
        err = "unknown preset '" + name + "' (presets: correctness, balanced, classic, optimized)";
        return false;
    }
    return resolvePipeline(out, err);
}

bool parsePipelineConfig(const std::string& text, Pipeline& out, std::string& err) {
    Json root;
    JsonReader reader(text);
    if (!reader.parse(root, err)) { err = "config is not valid JSON: " + err; return false; }
    if (root.kind != Json::Kind::OBJECT) { err = "config must be a JSON object"; return false; }
    for (const auto& kv : root.obj)
        if (kv.first != "preset" && kv.first != "groups") {
            err = "unknown config key '" + kv.first + "' (expected \"preset\" or \"groups\")";
            return false;
        }
    const Json* preset = root.get("preset");
    const Json* groups = root.get("groups");
    if (preset && groups) { err = "give either \"preset\" or \"groups\", not both"; return false; }
    if (preset) {
        if (preset->kind != Json::Kind::STRING) { err = "\"preset\" must be a string"; return false; }
        return presetPipeline(preset->str, out, err);
    }
    if (!groups || groups->kind != Json::Kind::ARRAY || groups->arr.empty()) {
        err = "\"groups\" must be a non-empty array";
        return false;
    }
    out = Pipeline{};
    for (size_t gi = 0; gi < groups->arr.size(); ++gi) {
        const Json& g = groups->arr[gi];
        std::string at = "group " + std::to_string(gi + 1) + ": ";
        if (g.kind != Json::Kind::OBJECT) { err = at + "must be an object"; return false; }
        PipelineGroup pg;
        for (const auto& kv : g.obj) {
            if (kv.first == "passes") {
                if (kv.second.kind != Json::Kind::ARRAY) { err = at + "\"passes\" must be an array"; return false; }
                for (const Json& p : kv.second.arr) {
                    if (p.kind != Json::Kind::STRING) { err = at + "pass names must be strings"; return false; }
                    pg.passes.push_back(p.str);
                }
            } else if (kv.first == "repeat") {
                if (kv.second.kind == Json::Kind::STRING && kv.second.str == "fixpoint") {
                    pg.fixpoint = true;
                } else if (kv.second.kind == Json::Kind::NUMBER && kv.second.num >= 1 && kv.second.num <= 32) {
                    pg.fixpoint = false;
                    pg.sweeps = static_cast<int>(kv.second.num);
                } else {
                    err = at + "\"repeat\" must be \"fixpoint\" or a number from 1 to 32";
                    return false;
                }
            } else {
                err = at + "unknown key '" + kv.first + "' (expected \"passes\" or \"repeat\")";
                return false;
            }
        }
        out.groups.push_back(std::move(pg));
    }
    return resolvePipeline(out, err);
}

bool pipelineFromPassList(const std::vector<std::string>& names, Pipeline& out, std::string& err) {
    out = Pipeline{};
    for (const auto& n : names) {
        bool known = n == "ssa" || isSsaPass(n) ||
                     std::find(kTacOrder.begin(), kTacOrder.end(), n) != kTacOrder.end();
        if (!known) {
            // Let resolvePipeline produce the message, so it says why.
            out.groups.push_back({{n}, true, 32, false});
            return resolvePipeline(out, err);
        }
    }
    auto has = [&](const std::string& n) { return std::find(names.begin(), names.end(), n) != names.end(); };
    PipelineGroup ssa, tac;
    for (const auto& n : kSsaOrder) if (has(n)) ssa.passes.push_back(n);
    for (const auto& n : kTacOrder) if (has(n)) tac.passes.push_back(n);
    if (has("ssa") || !ssa.passes.empty()) { ssa.ssa = true; out.groups.push_back(ssa); }
    if (!tac.passes.empty()) out.groups.push_back(tac);
    return resolvePipeline(out, err);
}

bool resolvePipeline(Pipeline& p, std::string& err) {
    for (size_t gi = 0; gi < p.groups.size(); ++gi) {
        PipelineGroup& g = p.groups[gi];
        std::string at = "group " + std::to_string(gi + 1) + ": ";
        std::vector<std::string> kept, ssaNames, tacNames;
        bool wantsSsa = g.ssa;
        for (const auto& n : g.passes) {
            if (n == "ssa") { wantsSsa = true; continue; }   // implicit; accepted for --opt=ssa
            const Technique* t = nullptr;
            for (const auto& x : techniqueRegistry()) if (x.id == n) t = &x;
            if (!t || (t->kind != "transform-tac" && t->kind != "transform-ssa")) {
                err = at + "unknown pass '" + n + "' (run --list-techniques for the list)";
                return false;
            }
            if (t->status != "Available") {
                err = at + "'" + n + "' is " + t->status + ", not implemented yet";
                return false;
            }
            (t->kind == "transform-ssa" ? ssaNames : tacNames).push_back(n);
            for (const auto& c : t->conflictsWith)
                if (std::find(g.passes.begin(), g.passes.end(), c) != g.passes.end()) {
                    err = at + "'" + n + "' conflicts with '" + c + "'";
                    return false;
                }
            kept.push_back(n);
        }
        if (!ssaNames.empty() && !tacNames.empty()) {
            err = at + "mixes SSA passes (" + ssaNames.front() + ") with plain-TAC passes (" +
                  tacNames.front() + "); a TAC pass cannot see phis, so put them in separate groups";
            return false;
        }
        if (kept.empty() && !wantsSsa) { err = at + "has no passes"; return false; }
        g.passes = std::move(kept);
        g.ssa = wantsSsa || !ssaNames.empty();
    }
    return true;
}

std::string Pipeline::describe() const {
    std::ostringstream os;
    os << "pipeline: " << preset << "\n";
    if (groups.empty()) os << "  (no passes: the IR runs exactly as generated)\n";
    for (size_t i = 0; i < groups.size(); ++i) os << "  " << (i + 1) << ". " << groupLine(groups[i]) << "\n";
    return os.str();
}

std::string Pipeline::toJson() const {
    std::ostringstream os;
    os << "{\"preset\":";
    writeJsonString(os, preset);
    os << ",\"groups\":[";
    for (size_t i = 0; i < groups.size(); ++i) {
        const PipelineGroup& g = groups[i];
        if (i) os << ',';
        os << "{\"passes\":[";
        for (size_t j = 0; j < g.passes.size(); ++j) { if (j) os << ','; writeJsonString(os, g.passes[j]); }
        os << "],\"fixpoint\":" << (g.fixpoint ? "true" : "false") << ",\"sweeps\":" << g.sweeps
           << ",\"ssa\":" << (g.ssa ? "true" : "false") << ",\"order\":";
        writeJsonString(os, groupLine(g));
        os << '}';
    }
    os << "]}";
    return os.str();
}

void runPipeline(CFG& cfg, const Pipeline& p, const std::set<std::string>& globals,
                 const std::string& function, bool verify, TraceRecorder* recorder, PipelineRun& out) {
    if (p.groups.empty()) return;
    if (verify) verifyOrThrow(cfg, "on the IR generated for '" + function + "', before any pass");
    if (recorder) recorder->beginFunction(function, snapshot(cfg));
    int nextSweep = 1;
    auto runGroup = [&](std::vector<std::unique_ptr<Pass>> passes, int cap) {
        PassManager pm;
        for (auto& ps : passes) pm.add(std::move(ps));
        pm.setVerify(verify, function);
        int s = pm.run(cfg, cap, recorder, nextSweep);
        nextSweep += s;
        out.sweeps += s;
        out.report.insert(out.report.end(), pm.report().begin(), pm.report().end());
        for (const auto& kv : pm.analysisCounts()) {
            out.analyses[kv.first].computed += kv.second.computed;
            out.analyses[kv.first].reused += kv.second.reused;
        }
        return pm.converged();
    };
    for (const PipelineGroup& g : p.groups) {
        if (g.ssa) {
            std::vector<std::unique_ptr<Pass>> v;
            v.push_back(makeSSAConstructPass());
            runGroup(std::move(v), 1);
        }
        if (!g.passes.empty()) {
            std::vector<std::unique_ptr<Pass>> v;
            for (const auto& n : g.passes) v.push_back(makePass(n, globals));
            bool converged = runGroup(std::move(v), g.fixpoint ? 32 : g.sweeps);
            if (g.fixpoint && !converged) out.fixedPoint = false;
        }
        if (g.ssa) {
            std::vector<std::unique_ptr<Pass>> v;
            v.push_back(makeSSADestructPass());
            runGroup(std::move(v), 1);
        }
    }
    if (recorder) recorder->endFunction(out.sweeps, out.fixedPoint, snapshot(cfg));
}

// ---- registry -----------------------------------------------------------------

const std::vector<Technique>& techniqueRegistry() {
    static const std::vector<Technique> reg = {
        {"lexer.handwritten", "Hand-written scanner", 1, "lexer", "Available", {}, {"lexer.re2c"},
         "Character-at-a-time scanner with maximal munch (--lexer=handwritten).", "tests/unit/test_lexer.cpp"},
        {"lexer.preprocessor", "Preprocessor subset", 1, "lexer", "Available", {"lexer.handwritten"}, {},
         "Object-like #define, #undef, #ifdef/#ifndef/#if/#elif/#else/#endif on integers and defined().",
         "tests/programs/preproc_cond.c"},
        {"lexer.re2c", "re2c direct-coded DFA scanner", 1, "lexer", "Available", {}, {"lexer.handwritten"},
         "The default lexer: generated by re2c 4.6, token-for-token identical to the hand-written one.",
         "scripts/lexfuzz.py"},
        {"parser.recursive-descent", "Recursive descent with Pratt expressions", 2, "parser", "Available", {},
         {"parser.bison"}, "Hand-written parser producing the typed AST.", "tests/unit/ast_smoke_gcd.cpp"},
        {"parser.recovery", "Panic-mode error recovery", 2, "parser", "Available", {"parser.recursive-descent"}, {},
         "Resynchronizes at statement boundaries: one diagnostic per fault.",
         "tests/negative/syn_rec01_return_missing_expr.c"},
        {"parser.bison", "Bison IELR(1) parser", 2, "parser", "Available", {}, {"parser.recursive-descent"},
         "Generated by Bison 3.8 (--parser=bison); the same AST, different error recovery, so not the default.",
         "src/parser/parser_bison.y"},
        {"sema.types", "Type checking and C integer conversions", 3, "semantic", "Available", {}, {},
         "Scoped symbol table, integer promotions and usual arithmetic conversions (LP64).",
         "tests/programs/int_widths.c"},
        {"sema.formats", "printf/scanf format checking", 3, "semantic", "Available", {}, {},
         "Checks format strings against their argument types.", "tests/negative/sem32_scanf_without_address.c"},
        {"ir.tac", "Typed three-address code", 4, "ir", "Available", {}, {},
         "Quadruples carrying the C integer type they compute in.", "tests/golden"},
        {"ir.verifier", "IR verifier", 4, "check", "Available", {"ir.tac"}, {},
         "Block shape, branch targets, edge consistency, use-before-def, and SSA dominance.",
         "tests/unit/test_pipeline.cpp"},
        {"ssa", "SSA construction and destruction", 4, "ir", "Available", {"analysis.dominators"}, {},
         "Cytron et al. phi placement and renaming; Sreedhar copies plus coalescing on the way out.",
         "tests/ssa/swap_and_lost_copy.c"},
        {"analysis.cfg", "Basic blocks and control-flow graph", 5, "analysis", "Available", {"ir.tac"}, {},
         "Leader algorithm with a synthetic entry block.", "tests/unit/test_symbol_cfg.cpp"},
        {"analysis.reaching", "Reaching definitions", 5, "analysis", "Available", {"analysis.cfg"}, {},
         "Forward, union; on the generic monotone solver.", "tests/unit/test_dataflow_ir.cpp"},
        {"analysis.available", "Available expressions", 5, "analysis", "Available", {"analysis.cfg"}, {},
         "Forward, intersection.", "tests/unit/test_dataflow_ir.cpp"},
        {"analysis.liveness", "Live variables", 5, "analysis", "Available", {"analysis.cfg"}, {},
         "Backward, union.", "tests/unit/test_dataflow_ir.cpp"},
        {"analysis.dominators", "Dominators and natural loops", 5, "analysis", "Available", {"analysis.cfg"}, {},
         "Iterative dominator sets, immediate-dominator tree, back edges, natural loops.",
         "tests/unit/test_symbol_cfg.cpp"},
        {"analysis.alias", "Basic alias analysis", 5, "analysis", "Available", {"ssa"}, {},
         "Distinct allocations never alias; unknown pointers may alias anything; calls clobber memory.",
         "tests/ssa/licm_hoists_load.c"},
        {"analysis.cache", "Analysis manager", 5, "analysis", "Available", {}, {},
         "Caches analyses between passes; dropped when a pass changes the IR.",
         "tests/unit/test_pipeline.cpp"},
        {"fold", "P1 constant folding", 6, "transform-tac", "Available", {}, {},
         "Evaluates operations on constants.", "tests/opt/fold_add.c"},
        {"constprop", "P2 constant propagation", 6, "transform-tac", "Available", {"analysis.reaching"}, {},
         "Replaces a use with a constant when that is its only reaching definition.", "tests/opt/constprop_basic.c"},
        {"copyprop", "P3 copy propagation", 6, "transform-tac", "Available", {"analysis.reaching"}, {},
         "Replaces a use with the source of a same-block copy.", "tests/opt/copyprop_chain.c"},
        {"cse", "P4 common subexpression elimination", 6, "transform-tac", "Available", {"analysis.available"}, {},
         "Reuses a same-block recomputation.", "tests/opt/cse_basic.c"},
        {"dce", "P5 dead-code elimination", 6, "transform-tac", "Available", {"analysis.liveness"}, {},
         "Deletes a pure computation whose result is never live.", "tests/opt/dce_basic.c"},
        {"unreachable", "P6 unreachable-code elimination", 6, "transform-tac", "Available", {"analysis.cfg"}, {},
         "Folds constant branches and deletes unreachable blocks.", "tests/opt/unreachable_basic.c"},
        {"strength", "P7 strength reduction", 6, "transform-tac", "Available", {"analysis.dominators"}, {},
         "Replaces i*K in a loop with an incrementally maintained variable.", "tests/opt/strength_basic.c"},
        {"licm", "P8 loop-invariant code motion", 6, "transform-tac", "Available", {"analysis.dominators"}, {},
         "Hoists pure invariant computations to the preheader.", "tests/opt/licm_basic.c"},
        {"peephole", "P9 jump peephole", 6, "transform-tac", "Available", {}, {},
         "Collapses jump chains and drops gotos to the next block.", "tests/opt/peephole_basic.c"},
        {"sccp", "Sparse conditional constant propagation", 6, "transform-ssa", "Available", {"ssa"}, {},
         "Wegman-Zadeck; subsumes P1, P2 and P6.", "tests/ssa/sccp_loop_constant.c"},
        {"gvn", "Global value numbering", 6, "transform-ssa", "Available", {"ssa", "analysis.dominators"}, {},
         "Dominator-tree scoped value numbering; removes cross-block redundancy.", "tests/ssa/gvn_cross_block.c"},
        {"licm-ssa", "Loop-invariant code motion on SSA", 6, "transform-ssa", "Available",
         {"ssa", "analysis.alias", "analysis.dominators"}, {},
         "Hoists invariant computations and loads no store or call in the loop can change.",
         "tests/ssa/licm_hoists_load.c"},
        {"ssa-dce", "Mark-sweep dead-code elimination", 6, "transform-ssa", "Available", {"ssa"}, {},
         "Keeps only what effects transitively use; removes dead cycles.", "tests/ssa/dce_dead_cycle.c"},
        {"inline", "Function inlining", 6, "transform-ssa", "Planned", {"ssa"}, {}, "Not implemented.", ""},
        {"loop-unroll", "Loop unrolling", 6, "transform-ssa", "Planned", {"ssa"}, {}, "Not implemented.", ""},
        {"vectorize", "Vectorization", 6, "transform-ssa", "Planned", {}, {},
         "Needs a machine target; Cppify has none.", ""},
        {"engine.quad-interp", "Quad interpreter", 7, "engine", "Available", {"ir.tac"}, {},
         "Executes the IR directly; runs the program unoptimized and optimized.",
         "tests/unit/test_dataflow_ir.cpp"},
        {"engine.ast-interp", "AST reference interpreter", 7, "engine", "Available", {"sema.types"}, {},
         "Runs the typed AST with no IR (--run-ast), so an IRGen bug cannot pass by agreeing with itself.",
         "tests/programs/goto_shapes.c"},
        {"engine.bytecode-vm", "Register bytecode VM", 7, "engine", "Available", {"ir.tac"}, {},
         "Compiles IR to registers and resolved jumps, switch dispatch (--run-vm); about 4x the quad "
         "interpreter's speed on bench/programs.", "scripts/bench.py"},
        {"check.differential", "Differential execution", 7, "check", "Available", {}, {},
         "Unoptimized and optimized runs must print the same bytes.", "scripts/differential.sh"},
        {"check.clang-oracle", "Clang oracle", 7, "check", "Available", {}, {},
         "Every test program must behave as it does when built by clang.", "scripts/clang_oracle.py"},
    };
    return reg;
}

std::string registryJson() {
    std::ostringstream os;
    auto list = [&](const std::vector<std::string>& v) {
        os << '[';
        for (size_t i = 0; i < v.size(); ++i) { if (i) os << ','; writeJsonString(os, v[i]); }
        os << ']';
    };
    os << "[\n";
    const auto& reg = techniqueRegistry();
    for (size_t i = 0; i < reg.size(); ++i) {
        const Technique& t = reg[i];
        os << "  {\"id\":";            writeJsonString(os, t.id);
        os << ",\"name\":";            writeJsonString(os, t.name);
        os << ",\"stage\":" << t.stage;
        os << ",\"kind\":";            writeJsonString(os, t.kind);
        os << ",\"status\":";          writeJsonString(os, t.status);
        os << ",\"requires\":";        list(t.requires);
        os << ",\"conflictsWith\":";   list(t.conflictsWith);
        os << ",\"description\":";     writeJsonString(os, t.description);
        os << ",\"test\":";            writeJsonString(os, t.test);
        os << '}' << (i + 1 < reg.size() ? "," : "") << "\n";
    }
    os << "]\n";
    return os.str();
}

}  // namespace minic
