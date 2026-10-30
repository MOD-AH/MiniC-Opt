// Cppify — IR verifier (roadmap 1.1)
//
// A pass that leaves the IR malformed usually still runs: the interpreter
// tolerates a lot (an unset temporary reads as 0, a stale succs[] list is
// never consulted at run time). The damage shows up later, in some other
// pass that trusted the broken invariant, far from the cause. The verifier
// checks the invariants every pass relies on, after every pass, so a bug is
// reported against the pass that introduced it.
#pragma once
#include "cfg.h"
#include <string>
#include <vector>

namespace minic {

// Returns one message per violation; empty means the CFG is well formed.
//   * LABEL only first in a block; GOTO/IF_FALSE/RET only last.
//   * Every branch target names a label that exists, exactly once.
//   * succs[] matches what the block's last instruction does, and preds[]
//     is its exact inverse (as multisets, since phi arguments are matched
//     to preds by position).
//   * Every operand slot an opcode reads or writes is filled.
//   * No temporary is read on a path where it was never written.
//   * In SSA form (cfg.inSSA): one definition per SSA name, none for an
//     original name, one phi argument per predecessor, and every use
//     dominated by its definition.
std::vector<std::string> verifyCFG(const CFG& cfg);

// Thrown by PassManager when verification after a pass fails.
struct VerifyError { std::string message; };

}  // namespace minic
