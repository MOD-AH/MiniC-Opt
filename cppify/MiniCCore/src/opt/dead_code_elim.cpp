// MiniC-Opt — P5: dead-code elimination
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// Precondition: live variables (backward, union — already implemented for
// --dump-dataflow=live in Step 5). Rewrite: delete a quad that assigns a
// name which is not live immediately after it, i.e. whose value is never
// read on any path forward from that point.
//
// Safety-first eligibility list: only a quad whose deletion has NO effect
// other than "this value is not computed" is a candidate. Excluded on
// purpose, even though writesOf() would say "yes, this is a name write":
//   - CALL       a call may have an observable side effect (print_int,
//                 read_int) even when its result is discarded; the
//                 language gives no way to tell a pure call from an
//                 impure one, so none are ever removed.
//   - DIV, MOD    can trap (division/modulo by zero) at run time; deleting
//                 a dead-but-trapping computation would silently make a
//                 program that used to raise a runtime error succeed
//                 instead — a real behavior change, not just "one fewer
//                 instruction".
//   - LOAD_INDEX  an out-of-bounds read is a runtime error in this
//                 language (see interp.cpp); same trap-hiding concern.
//   - ALLOC_ARRAY / STORE_INDEX / LABEL / GOTO / IF_FALSE / PARAM / RET
//                 either writesOf() already says "no name written"
//                 (STORE_INDEX, the control/call-protocol ops) or the op
//                 defines the binding an array's later STORE_INDEX/
//                 LOAD_INDEX instructions read *through* (ALLOC_ARRAY) —
//                 left alone rather than reasoned about here.
//
// The backward per-block walk below also lets a chain of dead assignments
// (`a = b; b = c;` with neither read again) collapse within one run() —
// once the later write is found dead, its reads are never folded into
// `live`, so an earlier write feeding only that dead instruction goes
// dead too, in the same pass.
#include "opt_passes.h"
#include "dataflow_analyses.h"
#include "ir_utils.h"
#include <set>

namespace minic {
namespace {

bool isDceEligible(Op op) {
    switch (op) {
        case Op::ADD: case Op::SUB: case Op::MUL:
        case Op::LT: case Op::LE: case Op::GT: case Op::GE: case Op::EQ: case Op::NE:
        case Op::AND: case Op::OR:
        case Op::NEG: case Op::NOT:
        // Bitwise ops cannot trap, so a dead one is deletable. SHL and SHR are
        // deliberately NOT here: an out-of-range shift count traps at run
        // time, exactly like DIV by zero, so deleting a dead one would turn a
        // program that errors into one that succeeds.
        case Op::BITAND: case Op::BITOR: case Op::BITXOR: case Op::BITNOT:
        case Op::COPY:
        case Op::TO_INT: case Op::TO_FLOAT:
            return true;
        default:   // DIV, MOD, SHL, SHR, LOAD_INDEX, CALL, ALLOC_ARRAY, control/call-protocol ops
            return false;
    }
}

class DeadCodeElimination : public Pass {
public:
    explicit DeadCodeElimination(std::set<std::string> globals)
        : globals_(std::move(globals)) {}

    std::string name() const override { return "dce"; }
    std::string description() const override {
        return "dead-code elimination: delete a pure computation whose result is never live afterward";
    }
    std::string requiresAnalysis() const override { return "live variables"; }

    bool run(CFG& cfg) override {
        count_ = 0;
        edits_.clear();
        // From the analysis cache, which is dropped whenever an earlier pass
        // changed the IR, so this always reflects the CFG as it is now.
        const DataFlowResult& live = analyses().liveVariables(cfg);
        for (size_t bi = 0; bi < cfg.blocks().size(); ++bi) {
            BasicBlock& b = cfg.block(static_cast<int>(bi));
            std::set<std::string> liveNow = live.out[bi];
            std::vector<bool> keep(b.code.size(), true);
            for (size_t qi = b.code.size(); qi-- > 0; ) {
                Quad& q = b.code[qi];
                Operand w;
                // A global is treated as live at every point. computeLiveVariables
                // is intra-procedural and sets OUT[exit] = {}, so a store to a
                // global that this function never reads back looks dead here even
                // though another function reads it later — which is how the whole
                // body of a `void bump() { counter = counter + 1; }` came to be
                // deleted. Nothing intra-procedural can prove such a store dead,
                // so the pass simply never claims one.
                if (writesOf(q, w) && isDceEligible(q.op) && !liveNow.count(w.name)
                        && !globals_.count(w.name)) {
                    keep[qi] = false;
                    ++count_;
                    {
                        TraceEdit e;
                        e.quadIndex = static_cast<int>(qi);
                        e.before = q.toString();
                        e.after = "";
                        e.kind = "delete";
                        e.sourceLine = q.srcLine;
                        e.reason = "'" + w.name + "' is not live after this instruction: no "
                                   "later instruction reads the value before it is overwritten "
                                   "or the function returns. The computation is pure and cannot "
                                   "trap, so deleting it changes nothing observable.";
                        edits_.push_back(std::move(e));
                    }
                    continue;   // deleted: its reads never execute, don't add them to liveNow
                }
                if (writesOf(q, w)) liveNow.erase(w.name);
                for (const Operand& r : readsOf(q)) liveNow.insert(r.name);
            }
            std::vector<Quad> kept;
            kept.reserve(b.code.size());
            for (size_t qi = 0; qi < b.code.size(); ++qi)
                if (keep[qi]) kept.push_back(std::move(b.code[qi]));
            b.code = std::move(kept);
        }
        return count_ > 0;
    }
    int transformCount() const override { return count_; }

private:
    int count_ = 0;
    std::set<std::string> globals_;
};

} // namespace

std::unique_ptr<Pass> makeDeadCodeEliminationPass(std::set<std::string> globals) {
    return std::make_unique<DeadCodeElimination>(std::move(globals));
}

} // namespace minic
