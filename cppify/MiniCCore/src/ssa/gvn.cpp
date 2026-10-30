// Cppify — dominator-tree value numbering (Briggs, Cooper and Simpson 1997)
//
// Walk the dominator tree keeping a scoped table from "operation on these
// values" to the SSA name that first computed it. A block can reuse any
// entry made in a block that dominates it, because the earlier computation
// has certainly run, with the same operands, whenever the later one is
// reached. Copies are folded into the numbering, so `x = y` makes every use
// of x a use of y. P4 does the same thing within one block only.
#include "ssa.h"
#include "ssa_util.h"
#include <functional>
#include <map>
#include <unordered_map>

namespace minic {
namespace {

bool commutative(Op op) {
    switch (op) {
        case Op::ADD: case Op::MUL: case Op::EQ: case Op::NE:
        case Op::BITAND: case Op::BITOR: case Op::BITXOR: case Op::AND: case Op::OR:
            return true;
        default: return false;
    }
}

class GVN : public Pass {
public:
    std::string name() const override { return "gvn"; }
    std::string description() const override {
        return "global value numbering: reuse a computation already made in a dominating block";
    }
    std::string requiresAnalysis() const override { return "SSA, dominator tree"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        std::set<std::string> originals;
        for (const auto& kv : cfg.ssaOrigin) originals.insert(kv.second);
        // A value that cannot change while the function runs: a constant, an
        // SSA name, or an original name (in SSA form it is never assigned).
        // A global or an array name can change, so it is never a key.
        auto immutable = [&](const Operand& o) {
            return o.isConst() || (isNameOperand(o) && (cfg.ssaOrigin.count(o.name) || originals.count(o.name)));
        };
        std::unordered_map<std::string, Operand> repl;
        std::function<Operand(const Operand&)> resolve = [&](const Operand& o) -> Operand {
            if (!isNameOperand(o)) return o;
            auto it = repl.find(o.name);
            if (it == repl.end()) return o;
            Operand r = resolve(it->second);
            it->second = r;
            return r;
        };
        auto key = [](const Operand& o) {
            return std::to_string(static_cast<int>(o.kind)) + ":" + operandKey(o);
        };

        // A constant must never replace an array base (LOAD/STORE need a
        // pointer), so a copy of a constant into a name used as a base stays.
        std::set<std::string> baseNames;
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code) {
                if (q.op == Op::LOAD_INDEX || q.op == Op::COPY_BLOCK) baseNames.insert(q.arg1.name);
                if (q.op == Op::STORE_INDEX || q.op == Op::COPY_BLOCK) baseNames.insert(q.result.name);
            }

        DomTree dt(cfg, analyses().dominators(cfg));
        std::unordered_map<std::string, std::string> table;
        std::function<void(int)> walk = [&](int b) {
            BasicBlock& blk = cfg.block(b);
            std::vector<std::string> added;

            std::vector<Phi> keptPhis;
            for (Phi& phi : blk.phis) {
                for (Operand& a : phi.args) a = resolve(a);
                // A phi whose arguments are all one value (or itself) is that value.
                const Operand* same = nullptr;
                bool trivial = true;
                for (const Operand& a : phi.args) {
                    if (isNameOperand(a) && a.name == phi.result.name) continue;
                    if (same && key(*same) != key(a)) { trivial = false; break; }
                    same = &a;
                }
                if (trivial && same && !(same->isConst() && baseNames.count(phi.result.name))) {
                    repl[phi.result.name] = *same;
                    note(b, phiText(phi), "", "phi-delete",
                         "Every argument of this phi is the same value, so the phi is that value.");
                    continue;
                }
                std::string k = "phi@" + std::to_string(b);
                for (const Operand& a : phi.args) k += "," + key(a);
                auto it = table.find(k);
                if (it != table.end()) {
                    for (const auto& kp : keptPhis)
                        if (kp.result.name == it->second) repl[phi.result.name] = kp.result;
                    note(b, phiText(phi), "", "phi-delete",
                         "Another phi in this block selects the same value on every edge.");
                    continue;
                }
                table[k] = phi.result.name;
                added.push_back(k);
                keptPhis.push_back(phi);
            }
            blk.phis = std::move(keptPhis);

            std::vector<Quad> kept;
            for (Quad& q : blk.code) {
                std::string before = q.toString();
                // Base pointers must stay names, so a constant never replaces one.
                for (Operand* s : readSlotsOf(q)) {
                    Operand r = resolve(*s);
                    bool baseSlot = (q.op == Op::LOAD_INDEX && s == &q.arg1) ||
                                    ((q.op == Op::STORE_INDEX || q.op == Op::COPY_BLOCK) && s == &q.result) ||
                                    (q.op == Op::COPY_BLOCK && s == &q.arg1);
                    if (!(baseSlot && r.isConst())) *s = r;
                }
                Operand w;
                bool ssaDef = writesOf(q, w) && cfg.ssaOrigin.count(w.name);
                if (ssaDef && q.op == Op::COPY && immutable(q.arg1) && !(q.arg1.isConst() && baseNames.count(w.name))) {
                    repl[w.name] = q.arg1;
                    note(b, before, "", "copy-fold",
                         "'" + w.name + "' is a copy of a value that never changes, so every use of it "
                         "now reads that value directly and the copy is deleted.");
                    continue;
                }
                bool pure = isBinaryExprOp(q.op) || q.op == Op::NEG || q.op == Op::NOT ||
                            q.op == Op::BITNOT || q.op == Op::TO_INT || q.op == Op::TO_FLOAT;
                if (ssaDef && pure && immutable(q.arg1) && (q.arg2.isNone() || immutable(q.arg2))) {
                    std::string a = key(q.arg1), c = q.arg2.isNone() ? "" : key(q.arg2);
                    if (commutative(q.op) && c < a) std::swap(a, c);
                    std::string k = std::to_string(static_cast<int>(q.op)) + "/" +
                                    std::to_string(static_cast<int>(q.ty)) + "(" + a + "," + c + ")";
                    auto it = table.find(k);
                    if (it != table.end()) {
                        Operand leader = q.result;
                        leader.name = it->second;
                        repl[w.name] = leader;
                        note(b, before, "", "redundancy",
                             "The same operation on the same values was already computed into '" +
                             it->second + "' in a block that dominates this one, so that result is "
                             "reused and this computation is deleted.");
                        continue;
                    }
                    table[k] = w.name;
                    added.push_back(k);
                }
                kept.push_back(q);
            }
            blk.code = std::move(kept);

            for (int c : dt.children[static_cast<size_t>(b)]) walk(c);
            for (const auto& k : added) table.erase(k);
        };
        walk(cfg.entry());

        // Uses visited before their replacement was known (phi arguments on
        // back edges, mostly) are resolved here.
        for (auto& bref : cfg.blocks()) {
            BasicBlock& b = cfg.block(bref.id);
            for (Phi& phi : b.phis) for (Operand& a : phi.args) a = resolve(a);
            for (Quad& q : b.code)
                for (Operand* s : readSlotsOf(q)) {
                    Operand r = resolve(*s);
                    bool baseSlot = (q.op == Op::LOAD_INDEX && s == &q.arg1) ||
                                    ((q.op == Op::STORE_INDEX || q.op == Op::COPY_BLOCK) && s == &q.result) ||
                                    (q.op == Op::COPY_BLOCK && s == &q.arg1);
                    if (!(baseSlot && r.isConst())) *s = r;
                }
        }
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;

    void note(int b, const std::string& before, const std::string& after, const char* kind,
              const std::string& why) {
        TraceEdit e;
        e.before = before;
        e.after = after;
        e.kind = kind;
        e.reason = why + " (B" + std::to_string(b) + ")";
        edits_.push_back(std::move(e));
        ++count_;
    }
};

}  // namespace

std::unique_ptr<Pass> makeGVNPass() { return std::make_unique<GVN>(); }

}  // namespace minic
