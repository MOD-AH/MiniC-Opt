// Cppify — SSA form and the passes that work on it (roadmap Phase 2)
//
// SSA is entered and left once per pipeline group (src/opt/pipeline.cpp), so
// every pass between the two sees a function in which each value has exactly
// one definition. That is what makes the passes below simpler and stronger
// than P1-P9: a use names its definition directly, with no reaching-
// definitions analysis and no "has the source been overwritten since?" check.
//
// What is renamed: temporaries and locals (names containing '#'). Not
// renamed: globals (another function may read them), and any name an
// ALLOC_ARRAY defines (an array or a boxed, address-taken scalar). Those live
// in memory, which SSA does not model; alias.h reasons about them instead.
#pragma once
#include "pass.h"
#include <memory>

namespace minic {

// Cytron et al. (1991): phis at the iterated dominance frontier of each
// variable's definitions, semi-pruned (Briggs: only for names read in some
// block before being written there), then renaming along the dominator tree.
// Blocks unreachable from entry are cleared first, since they have no
// dominator-tree position to rename them from.
std::unique_ptr<Pass> makeSSAConstructPass();

// Sreedhar et al. method I: each phi gets a fresh variable, copied into at
// the end of every predecessor and out of at the top of the phi's block.
// That is correct even when an SSA pass has made versions of one variable
// overlap (the lost-copy and swap problems), but costs two copies per phi;
// so it is followed by Chaitin-style coalescing, which merges the two ends
// of a copy whenever their live ranges do not interfere and deletes the
// copy. On code no SSA pass touched this gives back the original names.
std::unique_ptr<Pass> makeSSADestructPass();

// Wegman-Zadeck sparse conditional constant propagation. Subsumes P1
// (folding), P2 (constant propagation) and P6 (branch folding) and is
// stronger than running them to a fixed point: it assumes blocks are
// unreachable until proven otherwise, so a constant that only flows around
// a loop is still found.
std::unique_ptr<Pass> makeSCCPPass();

// Dominator-tree value numbering (Briggs, Cooper and Simpson 1997). A
// computation equal to one in a dominating block is replaced by the earlier
// result, across blocks, which P4 (same-block only) cannot do.
std::unique_ptr<Pass> makeGVNPass();

// Mark-sweep dead-code elimination: start from instructions with an effect
// outside the function's SSA values and keep only what they transitively
// use. Removes dead cycles (a value used only to update itself) that
// liveness-based P5 keeps alive.
std::unique_ptr<Pass> makeSSADCEPass();

// Loop-invariant code motion on SSA, using alias.h for memory. Beyond P8, it
// hoists loads no store or call in the loop can affect, and division or shift
// by a constant that cannot trap.
std::unique_ptr<Pass> makeSSALICMPass();

}  // namespace minic
