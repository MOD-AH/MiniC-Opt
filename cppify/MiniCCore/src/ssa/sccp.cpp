// Cppify — sparse conditional constant propagation (Wegman and Zadeck 1991)
//
// Each SSA name has a lattice value: TOP (no evidence yet), a constant, or
// BOTTOM (varies). Blocks start unreachable and edges are marked executable
// only when a branch that can actually go that way is reached. A phi meets
// only the arguments on executable edges. Optimistic: a loop-carried value
// that only ever turns out constant is proved constant, which the
// pessimistic P2 (it needs exactly one reaching definition) cannot do.
//
// Constant evaluation is foldBinary/foldUnary, the interpreter's own
// semantics; an operation that would trap stays BOTTOM and runs as before.
#include "opt_passes.h"
#include "ssa.h"
#include "ssa_util.h"
#include <deque>
#include <map>

namespace minic {
namespace {

struct Lat {
    enum Kind { TOP, CONST, BOTTOM } kind = TOP;
    Operand c;   // INT_CONST or FLOAT_CONST when kind == CONST
};

bool sameConst(const Operand& a, const Operand& b) {
    if (a.kind != b.kind) return false;
    return a.kind == OperandKind::INT_CONST ? a.ival == b.ival : a.fval == b.fval;
}

// Meets b into a; returns true if a changed.
bool meetInto(Lat& a, const Lat& b) {
    if (b.kind == Lat::TOP || a.kind == Lat::BOTTOM) return false;
    if (a.kind == Lat::TOP) { a = b; return true; }
    if (b.kind == Lat::BOTTOM || !sameConst(a.c, b.c)) { a.kind = Lat::BOTTOM; return true; }
    return false;
}

bool truthy(const Operand& c) {
    return c.kind == OperandKind::FLOAT_CONST ? c.fval != 0.0 : c.ival != 0;
}

class SCCP : public Pass {
public:
    std::string name() const override { return "sccp"; }
    std::string description() const override {
        return "sparse conditional constant propagation: find every value that is constant on all "
               "executable paths, and every branch that can only go one way";
    }
    std::string requiresAnalysis() const override { return "SSA def-use chains, executable edges"; }
    bool preservesCFG() const override { return false; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        const size_t n = cfg.blocks().size();
        std::map<std::string, Lat> val;
        auto valueOf = [&](const Operand& o) -> Lat {
            Lat l;
            if (o.kind == OperandKind::INT_CONST || o.kind == OperandKind::FLOAT_CONST) {
                l.kind = Lat::CONST; l.c = o; return l;
            }
            // Only SSA names have one definition to evaluate; everything else
            // (globals, memory, a variable's value on entry) is unknown.
            if (!isNameOperand(o) || !cfg.ssaOrigin.count(o.name)) { l.kind = Lat::BOTTOM; return l; }
            auto it = val.find(o.name);
            return it == val.end() ? l : it->second;
        };

        // Uses of each SSA name: (block, index), index -1 meaning the block's phis.
        std::map<std::string, std::vector<std::pair<int, int>>> uses;
        for (size_t bi = 0; bi < n; ++bi) {
            const BasicBlock& b = cfg.block(static_cast<int>(bi));
            for (const Phi& phi : b.phis)
                for (const Operand& a : phi.args)
                    if (isNameOperand(a)) uses[a.name].push_back({static_cast<int>(bi), -1});
            for (size_t qi = 0; qi < b.code.size(); ++qi)
                for (const Operand& r : readsOf(b.code[qi]))
                    uses[r.name].push_back({static_cast<int>(bi), static_cast<int>(qi)});
        }

        std::set<std::pair<int, int>> execEdge;
        std::vector<bool> execBlock(n, false);
        std::deque<std::pair<int, int>> flowWork;
        std::deque<std::pair<int, int>> ssaWork;
        flowWork.push_back({-1, cfg.entry()});

        auto lower = [&](const std::string& nm, const Lat& l) {
            Lat& cur = val[nm];
            if (meetInto(cur, l))
                for (const auto& u : uses[nm]) ssaWork.push_back(u);
        };
        auto evalPhis = [&](int b) {
            BasicBlock& blk = cfg.block(b);
            for (const Phi& phi : blk.phis) {
                Lat acc;
                for (size_t j = 0; j < phi.args.size(); ++j)
                    if (execEdge.count({blk.preds[j], b})) meetInto(acc, valueOf(phi.args[j]));
                lower(phi.result.name, acc);
            }
        };
        auto markEdge = [&](int from, int to) {
            if (execEdge.insert({from, to}).second) flowWork.push_back({from, to});
        };
        auto evalQuad = [&](int b, int qi) {
            const BasicBlock& blk = cfg.block(b);
            const Quad& q = blk.code[static_cast<size_t>(qi)];
            bool last = static_cast<size_t>(qi) + 1 == blk.code.size();
            if (q.op == Op::IF_FALSE) {
                Lat c = valueOf(q.arg1);
                if (c.kind == Lat::TOP) return;
                // succs is [target-if-false, fallthrough-if-true].
                int tgt = blk.succs.empty() ? -1 : blk.succs[0];
                int ft  = blk.succs.size() > 1 ? blk.succs[1] : tgt;
                if (c.kind == Lat::BOTTOM) { for (int s : blk.succs) markEdge(b, s); return; }
                markEdge(b, truthy(c.c) ? ft : tgt);
                return;
            }
            if (last && q.op != Op::RET) for (int s : blk.succs) markEdge(b, s);
            Operand w;
            if (!writesOf(q, w) || !cfg.ssaOrigin.count(w.name)) return;
            Lat out;
            out.kind = Lat::BOTTOM;
            if (q.op == Op::COPY) {
                out = valueOf(q.arg1);
            } else if (isBinaryExprOp(q.op)) {
                Lat a = valueOf(q.arg1), c = valueOf(q.arg2);
                if (a.kind == Lat::TOP || c.kind == Lat::TOP) out.kind = Lat::TOP;
                else if (a.kind == Lat::CONST && c.kind == Lat::CONST && foldBinary(q.op, a.c, c.c, q.ty, out.c))
                    out.kind = Lat::CONST;
            } else if (q.op == Op::NEG || q.op == Op::NOT || q.op == Op::BITNOT ||
                       q.op == Op::TO_INT || q.op == Op::TO_FLOAT) {
                Lat a = valueOf(q.arg1);
                if (a.kind == Lat::TOP) out.kind = Lat::TOP;
                else if (a.kind == Lat::CONST && foldUnary(q.op, a.c, q.ty, out.c)) out.kind = Lat::CONST;
            }
            if (out.kind != Lat::TOP) lower(w.name, out);
        };

        while (!flowWork.empty() || !ssaWork.empty()) {
            while (!flowWork.empty()) {
                int to = flowWork.front().second;
                flowWork.pop_front();
                evalPhis(to);
                if (execBlock[static_cast<size_t>(to)]) continue;
                execBlock[static_cast<size_t>(to)] = true;
                const BasicBlock& blk = cfg.block(to);
                for (size_t qi = 0; qi < blk.code.size(); ++qi) evalQuad(to, static_cast<int>(qi));
                if (blk.code.empty()) for (int s : blk.succs) markEdge(to, s);
            }
            while (!ssaWork.empty()) {
                auto u = ssaWork.front();
                ssaWork.pop_front();
                if (!execBlock[static_cast<size_t>(u.first)]) continue;
                if (u.second < 0) evalPhis(u.first);
                else evalQuad(u.first, u.second);
            }
        }

        // ---- rewrite ----------------------------------------------------------
        auto constOf = [&](const Operand& o, Operand& c) {
            if (!isNameOperand(o) || !cfg.ssaOrigin.count(o.name)) return false;
            auto it = val.find(o.name);
            if (it == val.end() || it->second.kind != Lat::CONST) return false;
            c = it->second.c;
            return true;
        };
        for (size_t bi = 0; bi < n; ++bi) {
            int b = static_cast<int>(bi);
            if (!execBlock[bi]) continue;
            BasicBlock& blk = cfg.block(b);
            // Phi arguments are left as names: a constant there saves nothing
            // (out-of-SSA would turn it back into a copy) and stops the copy
            // from being coalesced away.
            for (size_t qi = 0; qi < blk.code.size(); ++qi) {
                Quad& q = blk.code[qi];
                std::string before = q.toString();
                Operand w, c;
                // A value proved constant: compute it now instead of at run time.
                if (writesOf(q, w) && q.op != Op::CALL && q.op != Op::LOAD_INDEX && q.op != Op::ALLOC_ARRAY &&
                    constOf(w, c) && !(q.op == Op::COPY && q.arg1.isConst())) {
                    Quad nq;
                    nq.op = Op::COPY;
                    nq.arg1 = c;
                    nq.result = q.result;
                    nq.srcLine = q.srcLine;
                    q = nq;
                    note(b, qi, before, q.toString(), "fold",
                         "Every operand is constant on every path that can execute, so the value is "
                         "computed now instead of at run time.");
                    continue;
                }
                bool changed = false;
                // Base pointers stay names: LOAD/STORE/COPY_BLOCK need an array.
                std::vector<Operand*> slots;
                switch (q.op) {
                    case Op::LOAD_INDEX:  slots = {&q.arg2}; break;
                    case Op::STORE_INDEX: slots = {&q.arg1, &q.arg2}; break;
                    case Op::COPY_BLOCK:  break;
                    default:              slots = readSlotsOf(q);
                }
                for (Operand* s : slots)
                    if (constOf(*s, c)) { *s = c; changed = true; }
                if (changed)
                    note(b, qi, before, q.toString(), "operand-substitution",
                         "The SSA value used here has one definition, and that definition is constant on "
                         "every executable path, so the use is replaced by the constant.");
            }
            // A branch only one way can go becomes a jump (or nothing).
            if (!blk.code.empty() && blk.code.back().op == Op::IF_FALSE && blk.succs.size() == 2) {
                Quad& br = blk.code.back();
                int tgt = blk.succs[0], ft = blk.succs[1];
                bool toTgt = execEdge.count({b, tgt}) > 0, toFt = execEdge.count({b, ft}) > 0;
                if (toTgt != toFt) {
                    std::string before = br.toString();
                    if (toTgt) {
                        br.op = Op::GOTO;
                        br.arg1 = Operand{};
                        removeEdgeSSA(cfg, b, ft);
                    } else {
                        blk.code.pop_back();
                        removeEdgeSSA(cfg, b, tgt);
                    }
                    note(b, blk.code.size(), before, toTgt ? blk.code.back().toString() : "", "branch-fold",
                         std::string("The condition is the same constant on every executable path, so the branch ") +
                         (toTgt ? "is always taken and becomes a jump." : "is never taken and is removed."));
                }
            }
        }
        int dead = clearUnreachable(cfg);
        if (dead) {
            TraceEdit e;
            e.kind = "block-delete";
            e.before = std::to_string(dead) + " block(s)";
            e.reason = "No executable edge reaches these blocks, so they can never run.";
            edits_.push_back(std::move(e));
            count_ += dead;
        }
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;

    void note(int b, size_t qi, const std::string& before, const std::string& after,
              const char* kind, const std::string& why) {
        TraceEdit e;
        e.quadIndex = static_cast<int>(qi);
        e.before = before;
        e.after = after;
        e.kind = kind;
        e.reason = why + " (B" + std::to_string(b) + ")";
        edits_.push_back(std::move(e));
        ++count_;
    }
};

}  // namespace

std::unique_ptr<Pass> makeSCCPPass() { return std::make_unique<SCCP>(); }

}  // namespace minic
