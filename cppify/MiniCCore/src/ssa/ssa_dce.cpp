// Cppify — mark-sweep dead-code elimination on SSA (Cytron et al. 1991, §7.1)
//
// Liveness-based P5 deletes a definition only when nothing reads it later.
// A value that feeds only its own next iteration (`x = x + 1` in a loop,
// with x never used after) is always read later, by itself, so P5 keeps it
// forever. Here an instruction is kept only if something with an effect
// outside the SSA values needs it: a store, a call, a branch, a return, a
// write to a global or to memory, or an operation that may trap. Everything
// else is dead unless one of those transitively uses it.
//
// Branches are always kept (no control dependence), so an empty loop stays.
#include "ssa.h"
#include "ssa_util.h"
#include "int_semantics.h"
#include <map>

namespace minic {
namespace {

// True if the quad may stop the program with a runtime error, so deleting
// it would change behaviour. DIV/MOD and shifts are safe with a constant
// divisor or count the interpreter accepts.
bool mayTrap(const Quad& q) {
    switch (q.op) {
        case Op::DIV: case Op::MOD:
            if (q.arg2.kind == OperandKind::INT_CONST) return q.arg2.ival == 0;
            if (q.arg2.kind == OperandKind::FLOAT_CONST) return q.arg2.fval == 0.0;
            return true;
        case Op::SHL: case Op::SHR:
            return !(q.arg2.kind == OperandKind::INT_CONST && q.arg2.ival >= 0 && q.arg2.ival < intBits(q.ty));
        case Op::LOAD_INDEX:
            return true;
        default:
            return false;
    }
}

class SSADCE : public Pass {
public:
    std::string name() const override { return "ssa-dce"; }
    std::string description() const override {
        return "mark-sweep dead-code elimination: keep only what an effect transitively needs";
    }
    std::string requiresAnalysis() const override { return "SSA def-use chains"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        // Where each SSA name is defined: (block, index), index -1-k for phi k.
        std::map<std::string, std::pair<int, int>> def;
        for (const auto& b : cfg.blocks()) {
            for (size_t k = 0; k < b.phis.size(); ++k) def[b.phis[k].result.name] = {b.id, -1 - static_cast<int>(k)};
            for (size_t qi = 0; qi < b.code.size(); ++qi) {
                Operand w;
                if (writesOf(b.code[qi], w) && cfg.ssaOrigin.count(w.name)) def[w.name] = {b.id, static_cast<int>(qi)};
            }
        }
        std::set<std::string> live;
        std::vector<std::string> work;
        auto use = [&](const Operand& o) {
            if (isNameOperand(o) && def.count(o.name) && live.insert(o.name).second) work.push_back(o.name);
        };
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code) {
                Operand w;
                bool pureSsaDef = writesOf(q, w) && cfg.ssaOrigin.count(w.name) && q.op != Op::CALL && !mayTrap(q);
                if (!pureSsaDef) for (const Operand& r : readsOf(q)) use(r);
                if (!pureSsaDef && writesOf(q, w)) use(w);
            }
        while (!work.empty()) {
            std::string nm = work.back();
            work.pop_back();
            auto d = def[nm];
            const BasicBlock& b = cfg.block(d.first);
            if (d.second < 0) {
                for (const Operand& a : b.phis[static_cast<size_t>(-1 - d.second)].args) use(a);
            } else {
                for (const Operand& r : readsOf(b.code[static_cast<size_t>(d.second)])) use(r);
            }
        }
        for (auto& bref : cfg.blocks()) {
            BasicBlock& b = cfg.block(bref.id);
            std::vector<Phi> phis;
            for (Phi& phi : b.phis) {
                if (live.count(phi.result.name)) { phis.push_back(std::move(phi)); continue; }
                note(b.id, phiText(phi));
            }
            b.phis = std::move(phis);
            std::vector<Quad> code;
            for (Quad& q : b.code) {
                Operand w;
                if (writesOf(q, w) && def.count(w.name) && !live.count(w.name)) { note(b.id, q.toString()); continue; }
                code.push_back(std::move(q));
            }
            b.code = std::move(code);
        }
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;

    void note(int b, const std::string& before) {
        TraceEdit e;
        e.before = before;
        e.kind = "delete";
        e.reason = "No store, call, branch, return or global write uses this value, directly or "
                   "through other values, so it can never affect the program (B" + std::to_string(b) + ").";
        edits_.push_back(std::move(e));
        ++count_;
    }
};

}  // namespace

std::unique_ptr<Pass> makeSSADCEPass() { return std::make_unique<SSADCE>(); }

}  // namespace minic
