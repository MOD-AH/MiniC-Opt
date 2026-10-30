// MiniC-Opt — factories for the Step 6 optimization passes
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Step 6
#pragma once
#include "pass.h"
#include <memory>
#include <set>
#include <string>

namespace minic {

// Compile-time evaluation of one operation on constant operands, with the
// interpreter's exact semantics (src/opt/constant_folding.cpp). False when
// the operation would trap at run time, or has no constant form; the caller
// must then leave it to run time. Shared by P1 and SCCP.
bool foldBinary(Op op, const Operand& a, const Operand& b, IntTy ty, Operand& out);
bool foldUnary(Op op, const Operand& a, IntTy ty, Operand& out);

// P1. Precondition: none (syntactic) — replaces an operation on two (or
// one) compile-time constants with its computed constant result.
std::unique_ptr<Pass> makeConstantFoldingPass();

// P2. Precondition: reaching definitions — replaces a use of a variable
// with a constant when exactly one definition reaches that use and that
// definition is itself `v = CONST`.
std::unique_ptr<Pass> makeConstantPropagationPass();

// P3. Precondition: reaching definitions — replaces a use of a variable
// with another name when exactly one definition reaches that use and
// that definition is itself a plain copy `v = w`.
std::unique_ptr<Pass> makeCopyPropagationPass();

// P4. Precondition: available expressions (same-block locality — see
// src/opt/common_subexpr.cpp) — reuses a redundant same-block binary-
// expression recomputation via a COPY of its earlier result.
std::unique_ptr<Pass> makeCommonSubexprEliminationPass();

// P5. Precondition: live variables — deletes a pure computation whose
// result is never live afterward.
// `globals` names every variable with program lifetime (derived from
// IRProgram::globalInit, which writes each one exactly once). A store to a
// global can never be proved dead by this pass: liveness here is
// intra-procedural, so it cannot see the read that another function
// performs afterwards. Passing the set empty therefore risks deleting a
// function whose only effect is writing a global.
std::unique_ptr<Pass> makeDeadCodeEliminationPass(std::set<std::string> globals = {});

// P6. Precondition: CFG reachability — folds a branch whose condition has
// become a compile-time constant, then deletes any block no longer
// reachable from entry.
std::unique_ptr<Pass> makeUnreachableCodeEliminationPass();

// P7. Precondition: induction variables (single-preheader loops only —
// see src/opt/loop_util.h) — replaces `i*K` recomputed every iteration
// with an incrementally-maintained mirror variable.
std::unique_ptr<Pass> makeStrengthReductionPass();

// P8. Precondition: dominators, back edges, natural loops (single-
// preheader loops only) — hoists a loop-invariant, side-effect-free
// computation to the loop's preheader.
std::unique_ptr<Pass> makeLoopInvariantCodeMotionPass();

// P9. Precondition: none — collapses a jump-to-a-jump chain, then drops
// a goto to the physically next block.
std::unique_ptr<Pass> makePeepholePass();

// Adds P1-P9 to `pm` in report order (fold, constprop, copyprop, cse,
// dce, unreachable, strength, licm, peephole) — the order passes are
// added in does not fix the order they run in relative to each other's
// effects (PassManager::run sweeps all of them repeatedly to a fixed
// point), but it is the order --help and the metrics table list them in.
// `globals` is forwarded to P5; see makeDeadCodeEliminationPass above for
// why omitting it is unsafe for any program that declares a global.
void registerAllPasses(PassManager& pm, std::set<std::string> globals = {});

} // namespace minic
