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
};

} // namespace minic
