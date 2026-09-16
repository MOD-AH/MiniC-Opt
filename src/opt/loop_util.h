// MiniC-Opt — shared natural-loop helpers for P7/P8
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// Both loop-invariant code motion (P8) and strength reduction (P7) need
// the same two structural facts, built on top of Step 5's existing
// CFG::findBackEdges()/naturalLoop()/computeDominators():
//
//   1. The loop's block set (naturalLoop(tail, head), already generic).
//   2. A single, unambiguous PREHEADER to place new code in — a block
//      that executes exactly once per entry into the loop, before the
//      header, on every path. cfg.h's frozen interface has no notion of
//      inserting a new block (blocks_ is filled once, in the constructor,
//      from the leader algorithm, with no public mutator to grow it), so
//      rather than extend that interface, both passes here use a
//      textbook simplification: only hoist/reduce for a loop whose header
//      already has exactly one predecessor outside the loop, AND that
//      predecessor's only successor is the header itself. In that shape,
//      the predecessor already behaves exactly like a preheader block,
//      so new instructions can simply be appended to its existing code
//      (before its terminator, if it has an explicit one) with no CFG
//      surgery at all. A loop entered from more than one place, or whose
//      sole outside predecessor also branches somewhere else, is left
//      untouched by both passes — a documented, conservative scope
//      limit, in the same spirit as Step 5's direct-dominator deviation.
#pragma once
#include "cfg.h"
#include "ir_utils.h"
#include <algorithm>
#include <set>
#include <vector>

namespace minic {

struct NaturalLoop {
    int tail = -1, head = -1;
    std::set<int> blocks;      // includes head and tail
    int preheader = -1;        // -1 if no usable single preheader exists
};

// Every natural loop in the CFG, one per back edge (a loop with multiple
// back edges into the same header — e.g. two continue-like paths — is
// reported once per back edge, each sharing the same `blocks` set built
// from that edge's own tail; naturalLoop's own union-of-preds construction
// already merges every path back to any tail supplied).
inline std::vector<NaturalLoop> findNaturalLoops(const CFG& cfg) {
    std::vector<NaturalLoop> loops;
    for (const auto& edge : cfg.findBackEdges()) {
        NaturalLoop lp;
        lp.tail = edge.first;
        lp.head = edge.second;
        lp.blocks = cfg.naturalLoop(lp.tail, lp.head);

        std::vector<int> outsidePreds;
        for (int p : cfg.block(lp.head).preds)
            if (!lp.blocks.count(p)) outsidePreds.push_back(p);
        if (outsidePreds.size() == 1) {
            int cand = outsidePreds.front();
            const BasicBlock& cb = cfg.block(cand);
            if (cb.succs.size() == 1 && cb.succs.front() == lp.head)
                lp.preheader = cand;
        }
        loops.push_back(std::move(lp));
    }
    return loops;
}

// Appends `code` to the end of `preheader`'s instruction list, before its
// terminating control-transfer instruction if it has one (GOTO/IF_FALSE/
// RET) — anything else (a straight fallthrough into the header, the most
// common shape) just gets the new code appended at the very end.
inline void appendToPreheader(CFG& cfg, int preheaderId, const std::vector<Quad>& code) {
    BasicBlock& b = cfg.block(preheaderId);
    bool hasExplicitTerminator = !b.code.empty() &&
        (b.code.back().op == Op::GOTO || b.code.back().op == Op::IF_FALSE || b.code.back().op == Op::RET);
    if (hasExplicitTerminator) {
        Quad term = b.code.back();
        b.code.pop_back();
        for (const Quad& q : code) b.code.push_back(q);
        b.code.push_back(term);
    } else {
        for (const Quad& q : code) b.code.push_back(q);
    }
}

// Every name written anywhere in the given block set — used by P8 to
// decide whether an operand is loop-invariant (unwritten anywhere in the
// loop) and by P7 to confirm a candidate induction variable has exactly
// one write site in the loop.
inline std::set<std::string> namesDefinedIn(const CFG& cfg, const std::set<int>& blocks) {
    std::set<std::string> out;
    for (int bi : blocks) {
        for (const Quad& q : cfg.block(bi).code) {
            Operand w;
            if (writesOf(q, w)) out.insert(w.name);
        }
    }
    return out;
}

// Ops safe to relocate across the loop-entry boundary (P8) or to treat as
// a well-understood induction step (P7): no side effect, and — the part
// that actually matters for hoisting specifically — cannot trap. DIV/MOD
// (divide-by-zero) and LOAD_INDEX (out-of-bounds) are excluded for the
// same reason P5 excludes them from dead-code elimination: moving or
// deleting a computation that might raise a runtime error is a behavior
// change, not just an instruction-count change, if the loop that used to
// guard it ever executes zero times.
inline bool isSafeToRelocate(Op op) {
    switch (op) {
        case Op::COPY:
        case Op::ADD: case Op::SUB: case Op::MUL:
        case Op::LT: case Op::LE: case Op::GT: case Op::GE: case Op::EQ: case Op::NE:
        case Op::AND: case Op::OR:
        case Op::NEG: case Op::NOT:
        case Op::TO_INT: case Op::TO_FLOAT:
            return true;
        default:
            return false;
    }
}

} // namespace minic
