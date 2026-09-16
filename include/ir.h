// MiniC-Opt — three-address code intermediate representation
// Owner: Member 3 (System Designer / Core Algorithms)
//
// STATUS: DRAFT — circulated for team review ahead of the Week-4 freeze.
// Once frozen, changes require approval from Member 1 (IR generation) and
// Member 4 (pass manager, interpreter), who consume this interface.
#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace minic {

enum class Op {
    // binary
    ADD, SUB, MUL, DIV, MOD,
    LT, LE, GT, GE, EQ, NE,
    AND, OR,
    // unary
    NEG, NOT,
    // data movement
    COPY,                 // result = arg1
    LOAD_INDEX,           // result = arg1[arg2]
    STORE_INDEX,          // result[arg1] = arg2
    // Step 4 addition (IR generation, minimal + documented — see note at
    // end of file): the usual-arithmetic-conversion nodes Sema inserts
    // (ConvertExpr) need *some* opcode to carry out at runtime, and there
    // is no way to do that correctly with COPY alone, because a TAC
    // Operand carries no static type — only the interpreter's runtime
    // Value does. Two explicit, narrow opcodes are cheaper and clearer
    // than one generic CAST plus an out-of-band type tag.
    TO_INT,                // result = (int)   arg1   (truncates float; int/char passthrough)
    TO_FLOAT,               // result = (float) arg1   (promotes int/char)
    // Step 4 addition: local/global array storage needs an explicit
    // allocation point in TAC (there is no other instruction that could
    // plausibly mean "reserve N slots and bind them to this name").
    ALLOC_ARRAY,            // result = alloc arg1 slots  (arg1 = INT_CONST size)
    // control flow
    LABEL,                // label: (result names the label)
    GOTO,                 // goto result
    IF_FALSE,             // ifFalse arg1 goto result
    // calls
    PARAM,                // param arg1
    CALL,                 // result = call arg1, <argc in arg2>
    RET,                  // ret [arg1]
    NOP
};

enum class OperandKind { NONE, VARIABLE, TEMPORARY, INT_CONST, FLOAT_CONST, LABEL_REF };

struct Operand {
    OperandKind kind = OperandKind::NONE;
    std::string name;         // VARIABLE / TEMPORARY / LABEL_REF
    long long   ival = 0;     // INT_CONST
    double      fval = 0.0;   // FLOAT_CONST

    bool isConst() const {
        return kind == OperandKind::INT_CONST || kind == OperandKind::FLOAT_CONST;
    }
    bool isNone() const { return kind == OperandKind::NONE; }
};

// A quadruple: (op, arg1, arg2, result)
struct Quad {
    Op      op;
    Operand arg1;
    Operand arg2;
    Operand result;
    int     srcLine = 0;      // for diagnostics and before/after listings

    std::string toString() const;   // src/ir/quad.cpp
};

// A whole function's linear instruction list, prior to CFG construction.
struct IRFunction {
    std::string       name;
    std::vector<std::string> params;
    std::vector<Quad> code;
    int               tempCounter  = 0;
    int               labelCounter = 0;

    Operand newTemp();
    Operand newLabel();
};

struct IRProgram {
    std::vector<IRFunction> functions;
    // Step 4 addition: MiniC's grammar allows top-level (global) VarDecls,
    // but IRFunction only models one function's body. globalInit holds the
    // linear init code for every global (ALLOC_ARRAY / COPY / STORE_INDEX
    // quads, exactly as a function body would use), run once by the
    // interpreter before main() — the natural TAC analogue of C's static
    // initialization. None of the 12 committed benchmarks declare a global,
    // so this is exercised by construction rather than by the benchmark
    // suite; it exists so the front end doesn't silently mishandle a
    // grammar-legal program.
    std::vector<Quad> globalInit;
};

} // namespace minic

// ---------------------------------------------------------------------
// Step 4 note (IR generation, src/ir/irgen.cpp): three additions were made
// to this otherwise-frozen interface — Op::TO_INT/TO_FLOAT, Op::ALLOC_ARRAY,
// and IRProgram::globalInit. Each is additive (nothing existing changed or
// was removed), narrow, and documented at its point of addition above. Per
// the freeze note at the top of this file, called out explicitly here
// rather than made silently, matching the same policy Step 5 follows for
// its one deliberate deviation (direct dominator computation).
