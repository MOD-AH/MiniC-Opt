// MiniC-Opt — backend: linearisation and stack-slot assignment
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
//
// The one stage in this project that does NOT run under the differential-
// execution CI gate, and that is a deliberate, documented scope choice,
// not an oversight: MiniC-Opt has no second execution engine for a real
// or pseudo machine-code target — the reference interpreter (interp.h)
// is the ONLY oracle, and it runs the TAC quadruples directly. Emitting
// this backend's stack-slot pseudo-assembly is therefore a straight,
// mechanical transcription of already-verified IR — replace each
// variable/temporary name with a small integer slot number, in first-
// appearance order, and print each quad in a slot-addressed textual
// form — with no new control flow, no new arithmetic, and no new
// decisions of its own. Its correctness rests on that transcription
// being faithful, not on a second differential check; see docs/test_plan
// for exactly this caveat.
#pragma once
#include "ir.h"
#include <string>

namespace minic {

// Slot-addressed pseudo-assembly text for one function: params get the
// first slots (in declaration order), then every other VARIABLE/
// TEMPORARY name gets the next free slot in first-appearance order
// through the function's own code. Constants and labels are printed
// literally; every other operand is printed as `[slot]`.
std::string emitFunction(const IRFunction& fn);

// emitFunction for every function in the program, globalInit first.
std::string emitProgram(const IRProgram& program);

} // namespace minic
