// MiniC-Opt — reference interpreter
// Owner: Member 4 (Testing / Pass Manager) / Step 4
//
// Direct execution of an IRFunction's quadruples over an activation-record
// stack, one std::unordered_map<name, Value> per call. This is the
// correctness oracle the report's Objective O2 and the differential-
// execution CI gate (Step 6) both depend on: every optimization pass must
// produce IR that this interpreter still evaluates identically.
//
// Value is a small tagged union (INT / FLOAT / ARRAY). An ARRAY Value
// holds a std::shared_ptr<std::vector<Value>> rather than the vector
// itself, so that copying a Value — which is all parameter passing ever
// does — gives scalars C's pass-by-value semantics for free (the ints and
// floats are copied) and gives arrays MiniC's pass-by-reference-to-caller-
// storage semantics for free too (the shared_ptr is copied, the
// underlying vector is not): binary_search.c and linear_search.c both
// depend on a callee's writes/reads through an array parameter reaching
// the caller's own storage, and this is the whole mechanism that makes it
// correct without any special-casing at the call site.
#pragma once
#include "ir.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minic {

struct Value {
    enum class Kind { INT, FLOAT, ARRAY };
    Kind kind = Kind::INT;
    long long ival = 0;
    double fval = 0.0;
    std::shared_ptr<std::vector<Value>> arr;   // Kind::ARRAY only

    static Value makeInt(long long v)   { Value r; r.kind = Kind::INT;   r.ival = v; return r; }
    static Value makeFloat(double v)    { Value r; r.kind = Kind::FLOAT; r.fval = v; return r; }
    static Value makeArray(std::shared_ptr<std::vector<Value>> a) {
        Value r; r.kind = Kind::ARRAY; r.arr = std::move(a); return r;
    }

    double asDouble() const { return kind == Kind::FLOAT ? fval : static_cast<double>(ival); }
    bool   truthy()   const { return kind == Kind::FLOAT ? (fval != 0.0) : (ival != 0); }
};

class Interp {
public:
    struct RuntimeError { std::string message; };

    // Runs program.globalInit once, then calls `entry` with no arguments
    // and returns its value (0 if it falls off the end without a `return`).
    // Throws RuntimeError on an undefined callee, an out-of-bounds array
    // access, or division/modulo by zero.
    Value run(const IRProgram& program, const std::string& entry = "main");

    // Review 3 addition (--metrics): total quads actually executed by the
    // most recent run() call, LABEL excluded (see execCode in interp.cpp).
    // Purely observational — nothing reads this back into the interpreter
    // itself, so it doesn't touch the semantics Step 4 already froze.
    long long instructionsExecuted = 0;

private:
    struct ExecResult { bool returned; Value value; };

    std::unordered_map<std::string, const IRFunction*> funcsByName_;
    std::vector<Value> pendingParams_;              // queued by PARAM, consumed by the next CALL
    std::unordered_map<std::string, Value> globals_;

    ExecResult execCode(const std::vector<Quad>& code, std::unordered_map<std::string, Value>& frame);
    ExecResult execFunction(const IRFunction& fn, std::vector<Value> args);

    Value evalOperand(const Operand& o, std::unordered_map<std::string, Value>& frame);
    void  store(const Operand& o, const Value& v, std::unordered_map<std::string, Value>& frame);
    Value applyBinary(Op op, const Value& a, const Value& b);
    Value callBuiltin(const std::string& name, std::vector<Value>& args);
};

} // namespace minic
