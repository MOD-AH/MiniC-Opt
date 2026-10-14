// MiniC-Opt — basic blocks and control-flow graph construction
// Owner: Member 3 (System Designer / Core Algorithms) / Step 5
//
// Leader algorithm (Aho et al. 8.4): every LABEL quad is a leader (labels
// are the only thing a GOTO/IF_FALSE ever targets, in this IR), and so is
// whatever instruction immediately follows a GOTO or IF_FALSE. LABEL quads
// stay as the first instruction of the block they lead, rather than being
// stripped out — that is what lets edge-wiring below find "the block this
// label names" by simply checking each block's first instruction.
#include "cfg.h"
#include <algorithm>
#include <unordered_map>

namespace minic {

CFG::CFG(const IRFunction& fn) : fnName_(fn.name) {
    const std::vector<Quad>& code = fn.code;
    entry_ = 0;

    // A dedicated, always-empty entry block that by construction can
    // never have a predecessor. Without it, a function whose very
    // first quad is also a loop header's LABEL (e.g. a do-while with
    // nothing before it, or just an uninitialized first local) makes
    // entry_ coincide with that header, so the loop's own back edge
    // becomes an incoming edge on the entry block. solve()'s forward
    // boundary condition (dataflow.cpp) then force-overwrites IN[entry]
    // to the boundary value on every iteration, silently discarding
    // whatever the back edge would have contributed — the header never
    // sees its own loop-carried facts. A real ENTRY node with no preds
    // (Aho et al. 8.4) sidesteps this: the header becomes an ordinary
    // interior block and is meet-computed like any other.
    BasicBlock entryB;
    entryB.id = 0;
    blocks_.push_back(std::move(entryB));
    if (code.empty()) return;

    std::set<size_t> leaderSet;
    leaderSet.insert(0);
    for (size_t i = 0; i < code.size(); ++i) {
        if (code[i].op == Op::LABEL) leaderSet.insert(i);
        // RET ends a block same as GOTO/IF_FALSE: whatever follows it
        // (dead code after an early return, or just the next sibling
        // statement in source that never runs) must start a new block
        // rather than being treated as though it executes straight
        // through — otherwise a block can contain an instruction after
        // its own exit, and every pass that assumes straight-line
        // execution within a block (CSE's availability set in
        // particular) can merge facts across what's really two
        // disconnected pieces of code.
        if ((code[i].op == Op::GOTO || code[i].op == Op::IF_FALSE || code[i].op == Op::RET)
            && i + 1 < code.size())
            leaderSet.insert(i + 1);
    }
    std::vector<size_t> leaders(leaderSet.begin(), leaderSet.end());   // std::set is already sorted

    std::unordered_map<std::string, int> labelToBlock;
    blocks_.reserve(leaders.size() + 1);
    for (size_t bi = 0; bi < leaders.size(); ++bi) {
        size_t start = leaders[bi];
        size_t end   = (bi + 1 < leaders.size()) ? leaders[bi + 1] : code.size();
        BasicBlock b;
        b.id = static_cast<int>(bi) + 1;   // +1: id 0 is the synthetic entry block above
        b.code.assign(code.begin() + static_cast<long>(start), code.begin() + static_cast<long>(end));
        if (!b.code.empty() && b.code.front().op == Op::LABEL)
            labelToBlock[b.code.front().result.name] = b.id;
        blocks_.push_back(std::move(b));
    }
    blocks_[0].succs.push_back(1);   // entry falls straight through into the real first block
    blocks_[1].preds.push_back(0);

    for (size_t bi = 1; bi < blocks_.size(); ++bi) {
        BasicBlock& b = blocks_[bi];
        if (b.code.empty()) continue;
        const Quad& last = b.code.back();
        if (last.op == Op::GOTO) {
            auto it = labelToBlock.find(last.result.name);
            if (it != labelToBlock.end()) b.succs.push_back(it->second);
        } else if (last.op == Op::IF_FALSE) {
            auto it = labelToBlock.find(last.result.name);
            if (it != labelToBlock.end()) b.succs.push_back(it->second);          // condition false
            if (bi + 1 < blocks_.size()) b.succs.push_back(static_cast<int>(bi + 1));  // condition true (falls through)
        } else if (last.op == Op::RET) {
            // function exit: no successors
        } else if (bi + 1 < blocks_.size()) {
            b.succs.push_back(static_cast<int>(bi + 1));   // straight-line fallthrough into the next leader
        }
    }
    for (size_t bi = 1; bi < blocks_.size(); ++bi)
        for (int s : blocks_[bi].succs)
            blocks_[static_cast<size_t>(s)].preds.push_back(static_cast<int>(bi));
}

// Direct iterative dominator computation (Cooper/Torczon-style fixed
// point), not routed through the generic solve() below — see the note at
// the end of ir.h / the plan: dominance is naturally expressed as
// Dom[n] = {n} ∪ (∩ Dom[p] for p in preds(n)), which doesn't fit the
// gen/kill shape solve() is built around, so it gets its own direct loop.
std::vector<std::set<int>> CFG::computeDominators() const {
    size_t n = blocks_.size();
    std::vector<std::set<int>> dom(n);
    if (n == 0) return dom;

    std::set<int> allNodes;
    for (size_t i = 0; i < n; ++i) allNodes.insert(static_cast<int>(i));
    for (size_t i = 0; i < n; ++i)
        dom[i] = (static_cast<int>(i) == entry_) ? std::set<int>{entry_} : allNodes;

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < n; ++i) {
            if (static_cast<int>(i) == entry_) continue;
            const auto& preds = blocks_[i].preds;
            if (preds.empty()) continue;   // unreachable block: leave at the conservative "all nodes"

            std::set<int> newDom;
            bool first = true;
            for (int p : preds) {
                if (first) { newDom = dom[static_cast<size_t>(p)]; first = false; continue; }
                std::set<int> tmp;
                std::set_intersection(newDom.begin(), newDom.end(),
                                       dom[static_cast<size_t>(p)].begin(), dom[static_cast<size_t>(p)].end(),
                                       std::inserter(tmp, tmp.begin()));
                newDom = std::move(tmp);
            }
            newDom.insert(static_cast<int>(i));
            if (newDom != dom[i]) { dom[i] = std::move(newDom); changed = true; }
        }
    }
    return dom;
}

std::vector<std::pair<int, int>> CFG::findBackEdges() const {
    std::vector<std::set<int>> dom = computeDominators();
    std::vector<std::pair<int, int>> edges;
    for (size_t tail = 0; tail < blocks_.size(); ++tail)
        for (int head : blocks_[tail].succs)
            if (dom[tail].count(head)) edges.emplace_back(static_cast<int>(tail), head);
    return edges;
}

std::set<int> CFG::naturalLoop(int tail, int head) const {
    std::set<int> loop{head, tail};
    std::vector<int> worklist;
    if (tail != head) worklist.push_back(tail);
    while (!worklist.empty()) {
        int n = worklist.back();
        worklist.pop_back();
        for (int p : blocks_[static_cast<size_t>(n)].preds) {
            if (loop.insert(p).second) worklist.push_back(p);
        }
    }
    return loop;
}

std::set<int> CFG::reachableFromEntry() const {
    std::set<int> visited;
    if (blocks_.empty()) return visited;
    std::vector<int> stack{entry_};
    visited.insert(entry_);
    while (!stack.empty()) {
        int n = stack.back();
        stack.pop_back();
        for (int s : blocks_[static_cast<size_t>(n)].succs)
            if (visited.insert(s).second) stack.push_back(s);
    }
    return visited;
}

std::string CFG::toDot() const {
    std::string out = "digraph \"" + fnName_ + "\" {\n  node [shape=box, fontname=\"monospace\", fontsize=10];\n";
    for (const auto& b : blocks_) {
        out += "  B" + std::to_string(b.id) + " [label=\"B" + std::to_string(b.id) + ":\\l";
        for (const auto& q : b.code) {
            std::string line = q.toString();
            for (char& c : line) if (c == '"') c = '\'';   // our toString never emits '"', but stay defensive
            out += line + "\\l";
        }
        out += "\"];\n";
    }
    for (const auto& b : blocks_)
        for (int s : b.succs)
            out += "  B" + std::to_string(b.id) + " -> B" + std::to_string(s) + ";\n";
    out += "}\n";
    return out;
}

} // namespace minic
