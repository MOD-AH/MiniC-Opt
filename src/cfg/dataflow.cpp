// MiniC-Opt — generic iterative worklist data-flow solver, plus its three
// instantiations (reaching definitions, available expressions, live
// variables). Dominators are deliberately NOT a fourth instantiation here
// — see the note in cfg.cpp.
// Owner: Member 3 (System Designer / Core Algorithms) / Step 5
//
// Read/write classification per opcode (isNameOperand, readsOf, writesOf,
// isBinaryExprOp, operandKey, opTag) lives in ir_utils.h — shared with the
// Step 6 optimization passes, which need the exact same "is this a use or
// a definition" answers to know what they're licensed to rewrite.
#include "cfg.h"
#include "dataflow_analyses.h"
#include "ir_utils.h"
#include <algorithm>
#include <iterator>
#include <unordered_map>

namespace minic {

// ---- generic solver ---------------------------------------------------

void solve(CFG& cfg, const DataFlowSpec& spec, int iterationCap) {
    size_t n = cfg.blocks().size();
    if (n == 0) return;
    bool forward   = (spec.direction == Direction::FORWARD);
    bool unionMeet = (spec.meet == Meet::UNION);

    // Seed. The boundary block(s) get their fixed value on the "input"
    // side (IN for forward, OUT for backward); every block's *computed*
    // side starts at the meet's identity element — ∅ for union (the
    // conservative bottom, safe regardless of visiting order, growing
    // monotonically to the fixed point) or spec.universe for intersection
    // (see the note on DataFlowSpec::universe in cfg.h for why ∅ would be
    // wrong there).
    for (size_t i = 0; i < n; ++i) {
        BasicBlock& b = cfg.block(static_cast<int>(i));
        bool isBoundary = forward ? (static_cast<int>(i) == cfg.entry()) : b.succs.empty();
        if (forward) {
            b.in  = isBoundary ? spec.boundary : (unionMeet ? std::set<int>{} : spec.universe);
            b.out.clear();
        } else {
            b.out = isBoundary ? spec.boundary : (unionMeet ? std::set<int>{} : spec.universe);
            b.in.clear();
        }
    }

    auto meetInto = [&](std::set<int>& acc, const std::set<int>& incoming, bool first) {
        if (first) { acc = incoming; return; }
        std::set<int> tmp;
        if (unionMeet)
            std::set_union(acc.begin(), acc.end(), incoming.begin(), incoming.end(), std::inserter(tmp, tmp.begin()));
        else
            std::set_intersection(acc.begin(), acc.end(), incoming.begin(), incoming.end(), std::inserter(tmp, tmp.begin()));
        acc = std::move(tmp);
    };

    int iter = 0;
    bool changed = true;
    while (changed && iter < iterationCap) {
        changed = false;
        ++iter;
        for (size_t i = 0; i < n; ++i) {
            int id = static_cast<int>(i);
            BasicBlock& b = cfg.block(id);
            bool isBoundary = forward ? (id == cfg.entry()) : b.succs.empty();

            if (forward) {
                if (!isBoundary && !b.preds.empty()) {
                    std::set<int> newIn;
                    bool first = true;
                    for (int p : b.preds) { meetInto(newIn, cfg.block(p).out, first); first = false; }
                    b.in = std::move(newIn);
                }
                std::set<int> diff, newOut;
                std::set_difference(b.in.begin(), b.in.end(), b.kill.begin(), b.kill.end(), std::inserter(diff, diff.begin()));
                std::set_union(b.gen.begin(), b.gen.end(), diff.begin(), diff.end(), std::inserter(newOut, newOut.begin()));
                if (newOut != b.out) { b.out = std::move(newOut); changed = true; }
            } else {
                if (!isBoundary && !b.succs.empty()) {
                    std::set<int> newOut;
                    bool first = true;
                    for (int s : b.succs) { meetInto(newOut, cfg.block(s).in, first); first = false; }
                    b.out = std::move(newOut);
                }
                std::set<int> diff, newIn;
                std::set_difference(b.out.begin(), b.out.end(), b.kill.begin(), b.kill.end(), std::inserter(diff, diff.begin()));
                std::set_union(b.gen.begin(), b.gen.end(), diff.begin(), diff.end(), std::inserter(newIn, newIn.begin()));
                if (newIn != b.in) { b.in = std::move(newIn); changed = true; }
            }
        }
    }
}

// ---- reaching definitions (forward, union) -----------------------------

ReachingDefFacts computeReachingDefsRaw(CFG& cfg) {
    size_t n = cfg.blocks().size();
    ReachingDefFacts facts;
    facts.defIdOfQuad.resize(n);

    std::unordered_map<std::string, std::vector<int>> allDefsOfVar;
    for (size_t bi = 0; bi < n; ++bi) {
        const BasicBlock& b = cfg.block(static_cast<int>(bi));
        facts.defIdOfQuad[bi].assign(b.code.size(), -1);
        for (size_t qi = 0; qi < b.code.size(); ++qi) {
            Operand w;
            if (!writesOf(b.code[qi], w)) continue;
            int id = static_cast<int>(facts.defVar.size());
            facts.defVar.push_back(w.name);
            facts.defQuad.push_back(b.code[qi]);
            allDefsOfVar[w.name].push_back(id);
            facts.defIdOfQuad[bi][qi] = id;
        }
    }

    for (size_t bi = 0; bi < n; ++bi) {
        BasicBlock& b = cfg.block(static_cast<int>(bi));
        std::unordered_map<std::string, int> lastLocal;
        std::set<int> gen;
        for (size_t qi = 0; qi < b.code.size(); ++qi) {
            int id = facts.defIdOfQuad[bi][qi];
            if (id < 0) continue;
            const std::string& v = facts.defVar[static_cast<size_t>(id)];
            auto it = lastLocal.find(v);
            if (it != lastLocal.end()) gen.erase(it->second);   // this block's own earlier def of v is now stale
            lastLocal[v] = id;
            gen.insert(id);
        }
        std::set<int> kill;
        for (auto& kv : lastLocal)
            for (int otherId : allDefsOfVar[kv.first])
                if (otherId != kv.second) kill.insert(otherId);
        b.gen = std::move(gen);
        b.kill = std::move(kill);
    }

    DataFlowSpec spec;
    spec.direction = Direction::FORWARD;
    spec.meet = Meet::UNION;
    spec.boundary = {};   // no definitions reach the function's entry from outside it
    solve(cfg, spec);

    facts.in.resize(n); facts.out.resize(n);
    for (size_t bi = 0; bi < n; ++bi) {
        const BasicBlock& b = cfg.block(static_cast<int>(bi));
        facts.in[bi] = b.in;
        facts.out[bi] = b.out;
    }
    return facts;
}

DataFlowResult computeReachingDefs(CFG& cfg, const IRFunction&) {
    ReachingDefFacts facts = computeReachingDefsRaw(cfg);
    DataFlowResult result;
    size_t n = facts.in.size();
    result.in.resize(n); result.out.resize(n);
    for (size_t bi = 0; bi < n; ++bi) {
        for (int id : facts.in[bi])
            result.in[bi].insert(facts.defVar[static_cast<size_t>(id)] + " := " + facts.defQuad[static_cast<size_t>(id)].toString());
        for (int id : facts.out[bi])
            result.out[bi].insert(facts.defVar[static_cast<size_t>(id)] + " := " + facts.defQuad[static_cast<size_t>(id)].toString());
    }
    return result;
}

// ---- available expressions (forward, intersection) ---------------------

DataFlowResult computeAvailableExpressions(CFG& cfg, const IRFunction&) {
    size_t n = cfg.blocks().size();

    std::unordered_map<std::string, int> exprIdOf;
    std::vector<std::string> exprDesc;
    std::unordered_map<std::string, std::vector<int>> exprsUsingVar;   // varName -> expr ids depending on it

    auto internExpr = [&](const Quad& q) -> int {
        std::string key = opTag(q.op) + ":" + operandKey(q.arg1) + "," + operandKey(q.arg2);
        auto it = exprIdOf.find(key);
        if (it != exprIdOf.end()) return it->second;
        int id = static_cast<int>(exprDesc.size());
        exprIdOf[key] = id;
        exprDesc.push_back(operandKey(q.arg1) + " " + opTag(q.op) + " " + operandKey(q.arg2));
        if (isNameOperand(q.arg1)) exprsUsingVar[q.arg1.name].push_back(id);
        if (isNameOperand(q.arg2)) exprsUsingVar[q.arg2.name].push_back(id);
        return id;
    };

    // Universe = every distinct qualifying expression anywhere in the
    // function (available expressions asks "has THIS expression already
    // been computed", so the universe must be built before gen/kill can be
    // computed for any single block).
    std::set<int> universe;
    for (size_t bi = 0; bi < n; ++bi)
        for (const Quad& q : cfg.block(static_cast<int>(bi)).code)
            if (isBinaryExprOp(q.op)) universe.insert(internExpr(q));

    for (size_t bi = 0; bi < n; ++bi) {
        BasicBlock& b = cfg.block(static_cast<int>(bi));
        std::set<int> avail;
        for (const Quad& q : b.code) {
            if (isBinaryExprOp(q.op)) avail.insert(internExpr(q));   // tentative gen
            Operand w;
            if (writesOf(q, w)) {
                auto it = exprsUsingVar.find(w.name);
                if (it != exprsUsingVar.end())
                    for (int eid : it->second) avail.erase(eid);   // kill, including a same-instruction self-reference
            }
        }
        std::set<int> killed;
        for (const Quad& q : b.code) {
            Operand w;
            if (!writesOf(q, w)) continue;
            auto it = exprsUsingVar.find(w.name);
            if (it == exprsUsingVar.end()) continue;
            for (int eid : it->second) if (!avail.count(eid)) killed.insert(eid);
        }
        b.gen = std::move(avail);
        b.kill = std::move(killed);
    }

    DataFlowSpec spec;
    spec.direction = Direction::FORWARD;
    spec.meet = Meet::INTERSECTION;
    spec.boundary = {};        // nothing has been computed yet at function entry
    spec.universe = universe;  // seeds every other block's initial OUT (see cfg.h)
    solve(cfg, spec);

    DataFlowResult result;
    result.in.resize(n); result.out.resize(n);
    for (size_t bi = 0; bi < n; ++bi) {
        const BasicBlock& b = cfg.block(static_cast<int>(bi));
        for (int id : b.in)  result.in[bi].insert(exprDesc[static_cast<size_t>(id)]);
        for (int id : b.out) result.out[bi].insert(exprDesc[static_cast<size_t>(id)]);
    }
    return result;
}

// ---- live variables (backward, union) -----------------------------------

DataFlowResult computeLiveVariables(CFG& cfg, const IRFunction&) {
    size_t n = cfg.blocks().size();

    std::unordered_map<std::string, int> idOfVar;
    std::vector<std::string> varName;
    auto internVar = [&](const std::string& name) -> int {
        auto it = idOfVar.find(name);
        if (it != idOfVar.end()) return it->second;
        int id = static_cast<int>(varName.size());
        idOfVar[name] = id;
        varName.push_back(name);
        return id;
    };

    for (size_t bi = 0; bi < n; ++bi) {
        BasicBlock& b = cfg.block(static_cast<int>(bi));
        std::set<int> used;
        std::set<std::string> definedSoFar;
        for (const Quad& q : b.code) {
            for (const Operand& r : readsOf(q))
                if (!definedSoFar.count(r.name)) used.insert(internVar(r.name));
            Operand w;
            if (writesOf(q, w)) definedSoFar.insert(w.name);
        }
        std::set<int> def;
        for (const std::string& v : definedSoFar) def.insert(internVar(v));
        b.gen = std::move(used);
        b.kill = std::move(def);
    }

    DataFlowSpec spec;
    spec.direction = Direction::BACKWARD;
    spec.meet = Meet::UNION;
    spec.boundary = {};   // nothing is live after any return
    solve(cfg, spec);

    DataFlowResult result;
    result.in.resize(n); result.out.resize(n);
    for (size_t bi = 0; bi < n; ++bi) {
        const BasicBlock& b = cfg.block(static_cast<int>(bi));
        for (int id : b.in)  result.in[bi].insert(varName[static_cast<size_t>(id)]);
        for (int id : b.out) result.out[bi].insert(varName[static_cast<size_t>(id)]);
    }
    return result;
}

} // namespace minic
