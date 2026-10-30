// Cppify — SSA construction and destruction (see include/ssa.h)
#include "ssa.h"
#include "ssa_util.h"
#include <functional>
#include <map>
#include <unordered_map>

namespace minic {
namespace {

// Names SSA renames: temporaries and locals, minus anything an ALLOC_ARRAY
// defines (memory, not a value).
struct Candidates {
    std::set<std::string> memory;
    explicit Candidates(const CFG& cfg) {
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code)
                if (q.op == Op::ALLOC_ARRAY) memory.insert(q.result.name);
    }
    bool operator()(const Operand& o) const {
        if (!isNameOperand(o) || memory.count(o.name)) return false;
        return o.kind == OperandKind::TEMPORARY || o.name.find('#') != std::string::npos;
    }
};

class SSAConstruct : public Pass {
public:
    std::string name() const override { return "ssa"; }
    std::string description() const override {
        return "SSA construction: phis at iterated dominance frontiers, then renaming along the dominator tree";
    }
    std::string requiresAnalysis() const override { return "dominators, dominance frontiers, live variables"; }
    bool preservesCFG() const override { return false; }   // clears unreachable blocks

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        int cleared = clearUnreachable(cfg);
        if (cleared) {
            TraceEdit e;
            e.kind = "block-delete";
            e.before = std::to_string(cleared) + " unreachable block(s)";
            e.reason = "A block no path from entry reaches has no place in the dominator tree, so it "
                       "cannot be renamed; it can never run, so it is removed first.";
            edits_.push_back(std::move(e));
        }
        Candidates candidate(cfg);
        DomTree dt(cfg);
        const size_t n = cfg.blocks().size();

        // Dominance frontiers (Cooper, Harvey and Kennedy's two-finger walk).
        std::vector<std::set<int>> df(n);
        for (int b : dt.reachable) {
            const auto& preds = cfg.block(b).preds;
            if (preds.size() < 2) continue;
            for (int p : preds)
                for (int r = p; r >= 0 && r != dt.idom[static_cast<size_t>(b)]; r = dt.idom[static_cast<size_t>(r)])
                    df[static_cast<size_t>(r)].insert(b);
        }

        // Pruned: a phi only where the variable is live on entry to the block.
        // Without that, a dead phi can name a value that does not exist on
        // some edge (a temporary never written there), which is harmless at
        // run time but makes the IR claim to read it. The cheap semi-pruned
        // filter (names read in a block before written there) runs first.
        std::set<std::string> crossesBlocks;
        std::map<std::string, std::set<int>> defBlocks;
        std::map<std::string, Operand> proto;
        for (int b : dt.reachable) {
            std::set<std::string> written;
            for (const Quad& q : cfg.block(b).code) {
                for (const Operand& r : readsOf(q))
                    if (candidate(r) && !written.count(r.name)) crossesBlocks.insert(r.name);
                Operand w;
                if (writesOf(q, w) && candidate(w)) {
                    written.insert(w.name);
                    defBlocks[w.name].insert(b);
                    proto[w.name] = w;
                }
            }
        }

        const DataFlowResult& live = analyses().liveVariables(cfg);
        std::vector<std::vector<std::string>> phiVar(n);
        for (const auto& kv : defBlocks) {
            const std::string& var = kv.first;
            if (!crossesBlocks.count(var)) continue;
            std::vector<int> work(kv.second.begin(), kv.second.end());
            std::set<int> queued(kv.second.begin(), kv.second.end()), hasPhi;
            while (!work.empty()) {
                int x = work.back();
                work.pop_back();
                for (int y : df[static_cast<size_t>(x)]) {
                    if (!live.in[static_cast<size_t>(y)].count(var)) continue;
                    if (!hasPhi.insert(y).second) continue;
                    BasicBlock& yb = cfg.block(y);
                    Phi phi;
                    phi.result = proto[var];
                    phi.args.assign(yb.preds.size(), proto[var]);
                    yb.phis.push_back(std::move(phi));
                    phiVar[static_cast<size_t>(y)].push_back(var);
                    ++count_;
                    if (queued.insert(y).second) work.push_back(y);
                }
            }
        }

        cfg.ssaOrigin.clear();
        std::map<std::string, int> version;
        std::map<std::string, std::vector<std::string>> stacks;
        auto current = [&](const std::string& v) {
            auto it = stacks.find(v);
            return (it == stacks.end() || it->second.empty()) ? v : it->second.back();
        };
        auto fresh = [&](const std::string& v) {
            std::string nm = v + "." + std::to_string(++version[v]);
            cfg.ssaOrigin[nm] = v;
            stacks[v].push_back(nm);
            return nm;
        };
        std::function<void(int)> rename = [&](int b) {
            BasicBlock& blk = cfg.block(b);
            std::vector<std::string> pushed;
            for (size_t k = 0; k < blk.phis.size(); ++k) {
                const std::string& v = phiVar[static_cast<size_t>(b)][k];
                blk.phis[k].result.name = fresh(v);
                pushed.push_back(v);
            }
            for (Quad& q : blk.code) {
                for (Operand* r : readSlotsOf(q))
                    if (candidate(*r)) r->name = current(r->name);
                Operand w;
                if (writesOf(q, w) && candidate(w)) {
                    q.result.name = fresh(w.name);
                    pushed.push_back(w.name);
                }
            }
            std::set<int> seen;
            for (int s : blk.succs) {
                if (!seen.insert(s).second) continue;
                BasicBlock& sb = cfg.block(s);
                for (size_t j = 0; j < sb.preds.size(); ++j) {
                    if (sb.preds[j] != b) continue;
                    for (size_t k = 0; k < sb.phis.size(); ++k)
                        sb.phis[k].args[j].name = current(phiVar[static_cast<size_t>(s)][k]);
                }
            }
            for (int c : dt.children[static_cast<size_t>(b)]) rename(c);
            for (const std::string& v : pushed) stacks[v].pop_back();
        };
        rename(cfg.entry());
        cfg.inSSA = true;

        for (int b : dt.preorder)
            for (const Phi& phi : cfg.block(b).phis) {
                TraceEdit e;
                e.kind = "phi-insert";
                e.after = phiText(phi) + "   (B" + std::to_string(b) + ")";
                e.reason = "'" + cfg.ssaOrigin[phi.result.name] + "' is assigned in more than one "
                           "block, and B" + std::to_string(b) + " is in the dominance frontier of one "
                           "of those assignments: different definitions can reach it, so a phi "
                           "selects the one that matches the edge taken.";
                edits_.push_back(std::move(e));
            }
        return true;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
};

class SSADestruct : public Pass {
public:
    std::string name() const override { return "out-of-ssa"; }
    std::string description() const override {
        return "SSA destruction: replace each phi by copies on its incoming edges, then coalesce "
               "copies whose two names do not interfere";
    }
    std::string requiresAnalysis() const override { return "live variables (interference)"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        if (!cfg.inSSA) return false;
        std::map<std::string, std::string>& origin = cfg.ssaOrigin;
        auto base = [&](const std::string& nm) {
            auto it = origin.find(nm);
            return it == origin.end() ? nm : it->second;
        };

        // 0. A phi nothing reads would become copies that do nothing; drop
        // those first (repeatedly: a phi read only by a dead phi is dead).
        for (bool dropped = true; dropped; ) {
            dropped = false;
            std::set<std::string> read;
            for (const auto& b : cfg.blocks()) {
                for (const Phi& phi : b.phis)
                    for (const Operand& a : phi.args) if (isNameOperand(a) && a.name != phi.result.name) read.insert(a.name);
                for (const Quad& q : b.code) for (const Operand& r : readsOf(q)) read.insert(r.name);
            }
            for (auto& bref : cfg.blocks()) {
                BasicBlock& blk = cfg.block(bref.id);
                size_t before = blk.phis.size();
                blk.phis.erase(std::remove_if(blk.phis.begin(), blk.phis.end(),
                                              [&](const Phi& phi) { return !read.count(phi.result.name); }),
                               blk.phis.end());
                if (blk.phis.size() != before) dropped = true;
            }
        }

        // 1. Phis to copies (Sreedhar method I).
        int phiVars = 0;
        for (auto& b : cfg.blocks()) {
            BasicBlock& blk = cfg.block(b.id);
            if (blk.phis.empty()) continue;
            std::vector<Quad> heads;
            for (const Phi& phi : blk.phis) {
                Operand p = phi.result;
                p.name = base(phi.result.name) + ".phi" + std::to_string(++phiVars);
                origin[p.name] = base(phi.result.name);
                for (size_t j = 0; j < phi.args.size(); ++j) {
                    Quad c;
                    c.op = Op::COPY;
                    c.arg1 = phi.args[j];
                    c.result = p;
                    insertBeforeTerminator(cfg.block(blk.preds[j]), c);
                }
                Quad h;
                h.op = Op::COPY;
                h.arg1 = p;
                h.result = phi.result;
                heads.push_back(h);
                ++count_;
            }
            blk.code.insert(blk.code.begin() + static_cast<long>(bodyStart(blk)), heads.begin(), heads.end());
            blk.phis.clear();
        }

        // 2. Liveness, for interference. A name not in `origin` and not an
        // original is never renamed, so it is ignored.
        std::set<std::string> originals;
        for (const auto& kv : origin) originals.insert(kv.second);
        auto tracked = [&](const Operand& o) {
            return isNameOperand(o) && (origin.count(o.name) || originals.count(o.name));
        };
        const size_t n = cfg.blocks().size();
        std::vector<std::set<std::string>> use(n), def(n), liveIn(n), liveOut(n);
        for (size_t bi = 0; bi < n; ++bi)
            for (const Quad& q : cfg.block(static_cast<int>(bi)).code) {
                for (const Operand& r : readsOf(q))
                    if (tracked(r) && !def[bi].count(r.name)) use[bi].insert(r.name);
                Operand w;
                if (writesOf(q, w) && tracked(w)) def[bi].insert(w.name);
            }
        for (bool changed = true; changed; ) {
            changed = false;
            for (size_t bi = n; bi-- > 0; ) {
                std::set<std::string> out;
                for (int s : cfg.block(static_cast<int>(bi)).succs)
                    out.insert(liveIn[static_cast<size_t>(s)].begin(), liveIn[static_cast<size_t>(s)].end());
                std::set<std::string> in = use[bi];
                for (const auto& x : out) if (!def[bi].count(x)) in.insert(x);
                if (in != liveIn[bi] || out != liveOut[bi]) {
                    liveIn[bi] = std::move(in);
                    liveOut[bi] = std::move(out);
                    changed = true;
                }
            }
        }
        // Names of one variable can share storage, and so can a temporary and
        // anything else: GVN and SCCP leave a variable's value living in a
        // temporary (`t1 = i + 1` with the copy `i = t1` folded away), and
        // without merging the two the phi copy `i = t1` would stay on every
        // edge. Two different source variables are never merged.
        std::map<std::string, OperandKind> kindOf;
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code) {
                std::vector<Operand> ops = readsOf(q);
                Operand w;
                if (writesOf(q, w)) ops.push_back(w);
                for (const auto& o : ops) if (tracked(o)) kindOf[o.name] = o.kind;
            }
        auto isTemp = [&](const std::string& nm) { return kindOf[nm] == OperandKind::TEMPORARY; };
        std::map<std::string, std::set<std::string>> adj;
        auto addEdge = [&](const std::string& a, const std::string& b) {
            if (a == b || (base(a) != base(b) && !isTemp(a) && !isTemp(b))) return;
            adj[a].insert(b);
            adj[b].insert(a);
        };
        for (size_t bi = 0; bi < n; ++bi) {
            const BasicBlock& b = cfg.block(static_cast<int>(bi));
            std::set<std::string> live = liveOut[bi];
            for (size_t qi = b.code.size(); qi-- > 0; ) {
                const Quad& q = b.code[qi];
                Operand w;
                if (writesOf(q, w) && tracked(w)) {
                    // Chaitin: `a = b` does not make a interfere with b, since
                    // both hold the same value from here on.
                    bool isCopy = q.op == Op::COPY && isNameOperand(q.arg1);
                    for (const auto& l : live)
                        if (!(isCopy && l == q.arg1.name)) addEdge(w.name, l);
                    live.erase(w.name);
                }
                for (const Operand& r : readsOf(q)) if (tracked(r)) live.insert(r.name);
            }
        }
        // Everything live on entry is defined there at once (parameters,
        // uninitialized locals), so those names all interfere.
        const auto& entryLive = liveIn[static_cast<size_t>(cfg.entry())];
        for (const auto& a : entryLive) for (const auto& b : entryLive) addEdge(a, b);

        // 3. Coalesce the two ends of each copy, phi copies first. A class is
        // named after its variable if it has one (then it is a VARIABLE),
        // else after its temporary.
        std::map<std::string, std::string> parent, classBase;
        std::map<std::string, bool> classIsVar;
        for (const auto& kv : kindOf) {
            classBase[kv.first] = base(kv.first);
            classIsVar[kv.first] = kv.second != OperandKind::TEMPORARY;
        }
        std::function<std::string(const std::string&)> find = [&](const std::string& x) -> std::string {
            auto it = parent.find(x);
            if (it == parent.end() || it->second == x) return x;
            return it->second = find(it->second);
        };
        std::vector<std::pair<std::string, std::string>> copies, later;
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code)
                if (q.op == Op::COPY && tracked(q.result) && tracked(q.arg1)) {
                    bool phiCopy = q.arg1.name.find(".phi") != std::string::npos ||
                                   q.result.name.find(".phi") != std::string::npos;
                    (phiCopy ? copies : later).emplace_back(q.result.name, q.arg1.name);
                }
        copies.insert(copies.end(), later.begin(), later.end());
        for (const auto& c : copies) {
            std::string a = find(c.first), b = find(c.second);
            if (a == b || adj[a].count(b)) continue;
            if (classIsVar[a] && classIsVar[b] && classBase[a] != classBase[b]) continue;
            // The root names the class: an original name if there is one, else a variable.
            if ((originals.count(b) && !originals.count(a)) || (classIsVar[b] && !classIsVar[a])) std::swap(a, b);
            parent[b] = a;
            for (const auto& x : adj[b]) {
                adj[x].erase(b);
                adj[x].insert(a);
                adj[a].insert(x);
            }
            adj.erase(b);
        }

        // 4. Name the classes. Classes that never interfere can share a name,
        // as two live ranges can share a register; so each class takes the
        // first of its variable's name, name.1, name.2, ... that no
        // interfering class holds. A class holding an original name keeps it.
        std::vector<std::string> order;
        std::set<std::string> seen;
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code) {
                std::vector<Operand> ops = readsOf(q);
                Operand w;
                if (writesOf(q, w)) ops.push_back(w);
                for (const auto& o : ops) if (tracked(o) && seen.insert(o.name).second) order.push_back(o.name);
            }
        std::map<std::string, std::string> finalName;   // class root -> name
        for (const auto& nm : order) {
            std::string root = find(nm);
            if (originals.count(root)) finalName[root] = root;
        }
        for (const auto& nm : order) {
            std::string root = find(nm);
            if (finalName.count(root)) continue;
            const std::string bs = classBase[root];
            std::set<std::string> blocked;
            for (const auto& x : adj[root]) {
                auto it = finalName.find(x);
                if (it != finalName.end()) blocked.insert(it->second);
            }
            std::string chosen = bs;
            for (int k = 1; blocked.count(chosen); ++k) chosen = bs + "." + std::to_string(k);
            finalName[root] = chosen;
        }
        std::map<std::string, OperandKind> finalKind;
        for (const auto& nm : order) {
            std::string root = find(nm);
            finalName[nm] = finalName[root];
            finalKind[nm] = classIsVar[root] ? OperandKind::VARIABLE : OperandKind::TEMPORARY;
        }

        // 5. Rename, and drop the copies coalescing made redundant.
        int removed = 0;
        auto renameOp = [&](Operand& o) {
            if (!tracked(o)) return;
            o.kind = finalKind[o.name];
            o.name = finalName[o.name];
        };
        for (auto& bref : cfg.blocks()) {
            BasicBlock& b = cfg.block(bref.id);
            std::vector<Quad> kept;
            for (Quad& q : b.code) {
                for (Operand* r : readSlotsOf(q)) renameOp(*r);
                Operand w;
                if (writesOf(q, w)) renameOp(q.result);
                if (q.op == Op::COPY && isNameOperand(q.arg1) && q.arg1.name == q.result.name &&
                    q.arg1.kind == q.result.kind) { ++removed; continue; }
                kept.push_back(q);
            }
            b.code = std::move(kept);
        }
        cfg.inSSA = false;
        cfg.ssaOrigin.clear();

        TraceEdit e;
        e.kind = "out-of-ssa";
        e.before = std::to_string(count_) + " phi(s)";
        e.after = std::to_string(removed) + " copies coalesced away";
        e.reason = "Each phi became a copy at the end of every predecessor plus one at the top of its "
                   "block. Two names joined by a copy whose live ranges never overlap can share one "
                   "variable, and the copy between them then does nothing and is deleted.";
        edits_.push_back(std::move(e));
        return true;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
};

}  // namespace

std::unique_ptr<Pass> makeSSAConstructPass() { return std::make_unique<SSAConstruct>(); }
std::unique_ptr<Pass> makeSSADestructPass() { return std::make_unique<SSADestruct>(); }

}  // namespace minic
