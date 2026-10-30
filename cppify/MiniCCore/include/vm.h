// Cppify — register bytecode VM (roadmap 3.2)
//
// The quad interpreter executes IR as written: every operand is a name,
// looked up in a per-call std::unordered_map<std::string, Value>, and every
// jump searches a label table. The VM compiles each function once, before
// running anything:
//
//   * every local and temporary becomes a register, an index into a flat
//     per-call std::vector<Value>; every global an index into one global
//     array; every constant an index into a constant pool;
//   * every label becomes the index of the instruction it marks, and LABEL
//     and NOP disappear;
//   * every call names its callee by index, or as a library function.
//
// Execution is one loop with a switch on the opcode. Semantics are not
// reimplemented: arithmetic is Interp::applyBinary, the library is
// Interp::callBuiltin, and runtime errors carry the same messages, so the VM
// must agree with --run byte for byte (checked by run_all, the clang oracle
// and the fuzzer). Whether it is faster is measured by scripts/bench.py.
#pragma once
#include "interp.h"
#include "ir.h"
#include <cstdint>
#include <string>
#include <vector>

namespace minic {

class VM {
public:
    // Compiles and runs `program` (any IR: unoptimized or optimized),
    // returning main's value as Interp::run does.
    Value run(const IRProgram& program);

private:
    enum class Where : std::uint8_t { NONE, REG, GLOBAL, CONST };
    struct Slot {
        Where where = Where::NONE;
        std::int32_t index = 0;
    };
    struct Inst {
        Op op = Op::NOP;
        IntTy ty = IntTy::NONE;
        Slot a, b, r;
        std::int32_t target = 0;   // jump destination, callee, or argc (see compile)
        std::int32_t argc = 0;
    };
    struct Function {
        std::string name;
        std::vector<Inst> code;
        int registers = 0;
        int params = 0;            // registers 0..params-1 receive the arguments
    };

    std::vector<Function> functions_;   // functions_[0] is globalInit
    std::vector<std::string> builtins_;  // callee names that are library functions
    std::vector<Value> consts_;
    std::vector<Value> globals_;
    std::vector<Value> pending_;         // PARAM values waiting for their CALL
    Interp lib_;

    void compile(const IRProgram& program);
    Value execute(int fn, std::vector<Value>& regs);
};

}  // namespace minic
