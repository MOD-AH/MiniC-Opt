// Unit tests for roadmap Phase 1-2 infrastructure: the IR verifier, the
// analysis cache, pipeline configuration, and the SSA round trip.
#include "test_helpers.h"
#include "ir_utils.h"
#include "opt_passes.h"
#include "pipeline.h"
#include "ssa.h"
#include "verify.h"

#include <string>
#include <vector>

using namespace minic;

namespace {

const char* kLoop =
    "int main() { int i; int s; i = 0; s = 0;"
    " while (i < 4) { if (i > 1) s = s + i; i = i + 1; } return s; }";

CFG loopCfg() { return CFG(th::funcOf(kLoop)); }

bool mentions(const std::vector<std::string>& errs, const std::string& what) {
    for (const auto& e : errs) if (e.find(what) != std::string::npos) return true;
    return false;
}

// The first block that ends in the given opcode.
int blockEndingIn(const CFG& c, Op op) {
    for (const auto& b : c.blocks())
        if (!b.code.empty() && b.code.back().op == op) return b.id;
    return -1;
}

std::string irText(const CFG& c) {
    std::string s;
    for (const Quad& q : flatten(c)) s += q.toString() + "\n";
    return s;
}

}  // namespace

// ---- verifier ------------------------------------------------------------

TEST(verify_accepts_generated_ir) {
    CHECK(verifyCFG(loopCfg()).empty());
}

TEST(verify_rejects_branch_to_missing_label) {
    CFG c = loopCfg();
    int b = blockEndingIn(c, Op::GOTO);
    CHECK(b >= 0);
    c.block(b).code.back().result.name = "NOWHERE";
    CHECK(mentions(verifyCFG(c), "does not exist"));
}

TEST(verify_rejects_stale_successors) {
    CFG c = loopCfg();
    int b = blockEndingIn(c, Op::IF_FALSE);
    CHECK(b >= 0);
    // Turn the branch into a jump without updating succs/preds.
    c.block(b).code.back().op = Op::GOTO;
    c.block(b).code.back().arg1 = Operand{};
    CHECK(mentions(verifyCFG(c), "transfers control"));
}

TEST(verify_rejects_control_transfer_mid_block) {
    CFG c = loopCfg();
    int b = blockEndingIn(c, Op::GOTO);
    Quad nop;
    nop.op = Op::NOP;
    c.block(b).code.push_back(nop);
    CHECK(mentions(verifyCFG(c), "not the last instruction"));
}

TEST(verify_rejects_temporary_read_before_written) {
    CFG c = loopCfg();
    Quad q;
    q.op = Op::COPY;
    q.arg1.kind = OperandKind::TEMPORARY;
    q.arg1.name = "t_never_written";
    q.result.kind = OperandKind::VARIABLE;
    q.result.name = "s#1";
    c.block(1).code.insert(c.block(1).code.begin(), q);
    CHECK(mentions(verifyCFG(c), "read before it is written"));
}

TEST(verify_rejects_preds_that_do_not_mirror_succs) {
    CFG c = loopCfg();
    c.block(1).preds.push_back(1);
    CHECK(mentions(verifyCFG(c), "has no edge to it"));
}

// ---- SSA -----------------------------------------------------------------

TEST(ssa_construct_gives_verified_ssa) {
    CFG c = loopCfg();
    auto p = makeSSAConstructPass();
    p->run(c);
    CHECK(c.inSSA);
    CHECK(verifyCFG(c).empty());
    int phis = 0;
    for (const auto& b : c.blocks()) phis += static_cast<int>(b.phis.size());
    CHECK(phis >= 2);   // i and s both merge at the loop header
}

TEST(ssa_verify_rejects_second_definition) {
    CFG c = loopCfg();
    makeSSAConstructPass()->run(c);
    // Duplicate the first SSA definition, right after itself.
    bool duplicated = false;
    for (const auto& b : c.blocks()) {
        for (size_t qi = 0; qi < b.code.size() && !duplicated; ++qi) {
            Operand w;
            if (writesOf(b.code[qi], w) && c.ssaOrigin.count(w.name)) {
                Quad copy = b.code[qi];
                c.block(b.id).code.insert(c.block(b.id).code.begin() + static_cast<long>(qi) + 1, copy);
                duplicated = true;
            }
        }
        if (duplicated) break;
    }
    CHECK(duplicated);
    CHECK(mentions(verifyCFG(c), "defined more than once"));
}

TEST(ssa_verify_rejects_use_not_dominated) {
    CFG c = loopCfg();
    makeSSAConstructPass()->run(c);
    // Read the loop body's version of s in the entry block, before it exists.
    std::string late;
    for (const auto& kv : c.ssaOrigin) if (kv.second.rfind("s#", 0) == 0) late = kv.first;
    Quad q;
    q.op = Op::COPY;
    q.arg1.kind = OperandKind::VARIABLE;
    q.arg1.name = late;
    q.result.kind = OperandKind::VARIABLE;
    q.result.name = "g";
    c.block(1).code.insert(c.block(1).code.begin(), q);
    CHECK(mentions(verifyCFG(c), "not dominated") || mentions(verifyCFG(c), "never defined"));
}

// Into SSA and straight back out never costs an instruction: every phi copy
// coalesces away here, and so does the temporary in `t2 = s + i; s = t2`.
TEST(ssa_round_trip_adds_nothing) {
    CFG before = loopCfg();
    CFG c = loopCfg();
    makeSSAConstructPass()->run(c);
    makeSSADestructPass()->run(c);
    CHECK(!c.inSSA);
    CHECK(verifyCFG(c).empty());
    CHECK(flatten(c).size() < flatten(before).size());
    CHECK(irText(c).find(".phi") == std::string::npos);
    CHECK(irText(c).find("s#1 = s#1 + i#0") != std::string::npos);
}

// ---- analysis cache ------------------------------------------------------

TEST(analysis_cache_reuses_until_invalidated) {
    CFG c = loopCfg();
    AnalysisManager am;
    am.dominators(c);
    am.dominators(c);
    CHECK_EQ(am.counts().at("dominators").computed, 1);
    CHECK_EQ(am.counts().at("dominators").reused, 1);
    am.invalidate(false);                 // code changed, edges did not
    am.dominators(c);
    CHECK_EQ(am.counts().at("dominators").computed, 1);
    am.invalidate(true);                  // edges changed
    am.dominators(c);
    CHECK_EQ(am.counts().at("dominators").computed, 2);
}

TEST(analysis_cache_drops_dataflow_on_any_change) {
    CFG c = loopCfg();
    AnalysisManager am;
    am.liveVariables(c);
    am.invalidate(false);
    am.liveVariables(c);
    CHECK_EQ(am.counts().at("live variables").computed, 2);
}

// ---- pipeline configuration ----------------------------------------------

TEST(pipeline_presets_resolve) {
    for (const auto& name : presetNames()) {
        Pipeline p;
        std::string err;
        CHECK(presetPipeline(name, p, err));
    }
}

TEST(pipeline_optimized_wraps_ssa_group) {
    Pipeline p;
    std::string err;
    CHECK(presetPipeline("optimized", p, err));
    CHECK_EQ(p.groups.size(), size_t(2));
    CHECK(p.groups[0].ssa);
    CHECK(!p.groups[1].ssa);
}

TEST(pipeline_rejects_unknown_pass) {
    Pipeline p;
    std::string err;
    CHECK(!parsePipelineConfig(R"({"groups":[{"passes":["fold","bogus"]}]})", p, err));
    CHECK(err.find("unknown pass 'bogus'") != std::string::npos);
}

TEST(pipeline_rejects_mixed_ssa_and_tac_group) {
    Pipeline p;
    std::string err;
    CHECK(!parsePipelineConfig(R"({"groups":[{"passes":["sccp","dce"]}]})", p, err));
    CHECK(err.find("mixes SSA passes") != std::string::npos);
}

TEST(pipeline_rejects_planned_technique) {
    Pipeline p;
    std::string err;
    CHECK(!parsePipelineConfig(R"({"groups":[{"passes":["inline"]}]})", p, err));
    CHECK(err.find("Planned") != std::string::npos);
}

TEST(pipeline_rejects_bad_repeat_and_keys) {
    Pipeline p;
    std::string err;
    CHECK(!parsePipelineConfig(R"({"groups":[{"passes":["fold"],"repeat":0}]})", p, err));
    CHECK(!parsePipelineConfig(R"({"groups":[{"passes":["fold"],"times":2}]})", p, err));
    CHECK(!parsePipelineConfig(R"({"preset":"fastest"})", p, err));
    CHECK(!parsePipelineConfig(R"({"groups":[{"passes":["fold"]}]})" "x", p, err));
}

TEST(pipeline_keeps_order_and_repeats) {
    Pipeline p;
    std::string err;
    CHECK(parsePipelineConfig(R"({"groups":[{"passes":["constprop","fold","constprop"],"repeat":1}]})", p, err));
    CFG c = loopCfg();
    PipelineRun run;
    runPipeline(c, p, {}, "main", true, nullptr, run);
    std::vector<std::string> order;
    for (const auto& r : run.report) order.push_back(r.pass);
    CHECK_EQ(order.size(), size_t(3));
    CHECK_EQ(order[0], std::string("constprop"));
    CHECK_EQ(order[1], std::string("fold"));
    CHECK_EQ(order[2], std::string("constprop"));
}

// ---- registry --------------------------------------------------------------

TEST(registry_available_entries_name_a_test) {
    for (const auto& t : techniqueRegistry())
        if (t.status == "Available") CHECK(!t.test.empty());
}

TEST(registry_every_pass_is_registered) {
    for (const char* n : {"fold", "constprop", "copyprop", "cse", "dce", "unreachable", "strength", "licm",
                          "peephole", "sccp", "gvn", "licm-ssa", "ssa-dce"}) {
        bool found = false;
        for (const auto& t : techniqueRegistry()) if (t.id == n && t.status == "Available") found = true;
        CHECK(found);
    }
}
