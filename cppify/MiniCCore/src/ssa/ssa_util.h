// Cppify — helpers shared by the SSA passes (src/ssa/*.cpp only)
#pragma once
#include "cfg.h"
#include "ir_utils.h"
#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace minic {

// The immediate-dominator tree, derived from CFG::computeDominators(): the
// immediate dominator of b is the strict dominator with the largest
// dominator set of its own (the closest one).
struct DomTree {
    std::vector<int> idom;                    // -1 for entry and unreachable blocks
    std::vector<std::vector<int>> children;
    std::set<int> reachable;
    std::vector<int> preorder;                // reachable blocks, parents first

    explicit DomTree(const CFG& cfg) : DomTree(cfg, cfg.computeDominators()) {}
    DomTree(const CFG& cfg, const std::vector<std::set<int>>& dom) {
        size_t n = cfg.blocks().size();
        idom.assign(n, -1);
        children.assign(n, {});
        reachable = cfg.reachableFromEntry();
        for (int b : reachable) {
            if (b == cfg.entry()) continue;
            int best = -1;
            for (int d : dom[static_cast<size_t>(b)])
                if (d != b && (best < 0 || dom[static_cast<size_t>(d)].size() > dom[static_cast<size_t>(best)].size()))
                    best = d;
            idom[static_cast<size_t>(b)] = best;
            if (best >= 0) children[static_cast<size_t>(best)].push_back(b);
        }
        std::vector<int> stack{cfg.entry()};
        while (!stack.empty()) {
            int b = stack.back();
            stack.pop_back();
            preorder.push_back(b);
            const auto& ch = children[static_cast<size_t>(b)];
            for (auto it = ch.rbegin(); it != ch.rend(); ++it) stack.push_back(*it);
        }
    }

    bool dominates(int a, int b) const {
        for (int x = b; x >= 0; x = idom[static_cast<size_t>(x)])
            if (x == a) return true;
        return false;
    }
};

// Removes one edge from -> to, together with the phi arguments that flowed
// along it, keeping every phi's args aligned with preds.
inline void removeEdgeSSA(CFG& cfg, int from, int to) {
    BasicBlock& f = cfg.block(from);
    auto si = std::find(f.succs.begin(), f.succs.end(), to);
    if (si != f.succs.end()) f.succs.erase(si);
    BasicBlock& t = cfg.block(to);
    auto pi = std::find(t.preds.begin(), t.preds.end(), from);
    if (pi == t.preds.end()) return;
    size_t j = static_cast<size_t>(pi - t.preds.begin());
    t.preds.erase(pi);
    for (Phi& phi : t.phis) phi.args.erase(phi.args.begin() + static_cast<long>(j));
}

// Empties every block not reachable from entry and disconnects it. Returns
// how many blocks held code.
inline int clearUnreachable(CFG& cfg) {
    std::set<int> reach = cfg.reachableFromEntry();
    int cleared = 0;
    for (size_t bi = 0; bi < cfg.blocks().size(); ++bi) {
        int id = static_cast<int>(bi);
        if (reach.count(id)) continue;
        BasicBlock& b = cfg.block(id);
        if (!b.code.empty() || !b.phis.empty()) ++cleared;
        for (int s : std::vector<int>(b.succs)) removeEdgeSSA(cfg, id, s);
        for (int p : std::vector<int>(b.preds)) removeEdgeSSA(cfg, p, id);
        b.code.clear();
        b.phis.clear();
    }
    return cleared;
}

// Inserts `q` at the end of block b, before its control transfer if any.
inline void insertBeforeTerminator(BasicBlock& b, const Quad& q) {
    bool term = !b.code.empty() &&
        (b.code.back().op == Op::GOTO || b.code.back().op == Op::IF_FALSE || b.code.back().op == Op::RET);
    b.code.insert(term ? b.code.end() - 1 : b.code.end(), q);
}

// Where a block's own instructions start: after its LABEL, if it has one.
inline size_t bodyStart(const BasicBlock& b) {
    return (!b.code.empty() && b.code.front().op == Op::LABEL) ? 1 : 0;
}

inline std::string phiText(const Phi& phi) {
    std::string s = phi.result.name + " = phi(";
    for (size_t i = 0; i < phi.args.size(); ++i) {
        if (i) s += ", ";
        s += operandKey(phi.args[i]);
    }
    return s + ")";
}

}  // namespace minic
