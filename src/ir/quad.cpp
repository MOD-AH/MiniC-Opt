// MiniC-Opt — TAC operand/quad printing, temp and label allocation
// (Step 4: implements the pieces include/ir.h only declared.)
#include "ir.h"
#include <sstream>

namespace minic {

Operand IRFunction::newTemp() {
    Operand o;
    o.kind = OperandKind::TEMPORARY;
    o.name = "t" + std::to_string(tempCounter++);
    return o;
}

Operand IRFunction::newLabel() {
    Operand o;
    o.kind = OperandKind::LABEL_REF;
    o.name = "L" + std::to_string(labelCounter++);
    return o;
}

// Symbolic infix form for the operators that have one, matching the style
// docs/worked_example.md was hand-written in ("i < n", not "i LT n") — this
// is purely --dump-ir's presentation; nothing downstream parses this text
// back (the pass manager and interpreter always work on the Quad struct
// itself, never its printed form).
static std::string opSymbol(Op op) {
    switch (op) {
        case Op::ADD: return "+";  case Op::SUB: return "-";
        case Op::MUL: return "*";  case Op::DIV: return "/";
        case Op::MOD: return "%";
        case Op::LT: return "<";   case Op::LE: return "<=";
        case Op::GT: return ">";   case Op::GE: return ">=";
        case Op::EQ: return "=="; case Op::NE: return "!=";
        case Op::AND: return "&&"; case Op::OR: return "||";
        default: return "?";
    }
}

static std::string opName(Op op) {
    switch (op) {
        case Op::NEG: return "-";  case Op::NOT: return "!";
        case Op::COPY: return "COPY";
        case Op::LOAD_INDEX: return "LOAD_INDEX";
        case Op::STORE_INDEX: return "STORE_INDEX";
        case Op::TO_INT: return "(int)";
        case Op::TO_FLOAT: return "(float)";
        case Op::ALLOC_ARRAY: return "ALLOC_ARRAY";
        case Op::LABEL: return "LABEL";
        case Op::GOTO: return "GOTO";
        case Op::IF_FALSE: return "IF_FALSE";
        case Op::PARAM: return "PARAM";
        case Op::CALL: return "CALL";
        case Op::RET: return "RET";
        case Op::NOP: return "NOP";
        default: return "?";
    }
}

// Renders an operand the way the worked example / Annexure C style does:
// bare name for variables/temporaries/labels, literal text for constants.
static std::string operandStr(const Operand& o) {
    switch (o.kind) {
        case OperandKind::NONE:        return "";
        case OperandKind::VARIABLE:    return o.name;
        case OperandKind::TEMPORARY:   return o.name;
        case OperandKind::LABEL_REF:   return o.name;
        case OperandKind::INT_CONST:   return std::to_string(o.ival);
        case OperandKind::FLOAT_CONST: {
            std::ostringstream ss;
            ss << o.fval;
            return ss.str();
        }
    }
    return "";
}

std::string Quad::toString() const {
    std::ostringstream ss;
    switch (op) {
        case Op::LABEL:
            ss << operandStr(result) << ":";
            return ss.str();
        case Op::GOTO:
            ss << "goto " << operandStr(result);
            return ss.str();
        case Op::IF_FALSE:
            ss << "ifFalse " << operandStr(arg1) << " goto " << operandStr(result);
            return ss.str();
        case Op::PARAM:
            ss << "param " << operandStr(arg1);
            return ss.str();
        case Op::CALL:
            if (!result.isNone()) ss << operandStr(result) << " = ";
            ss << "call " << operandStr(arg1) << ", " << operandStr(arg2);
            return ss.str();
        case Op::RET:
            ss << "ret";
            if (!arg1.isNone()) ss << " " << operandStr(arg1);
            return ss.str();
        case Op::NOP:
            return "nop";
        case Op::COPY:
            ss << operandStr(result) << " = " << operandStr(arg1);
            return ss.str();
        case Op::NEG: case Op::NOT:
            ss << operandStr(result) << " = " << opName(op) << operandStr(arg1);
            return ss.str();
        case Op::TO_INT: case Op::TO_FLOAT:
            ss << operandStr(result) << " = " << opName(op) << " " << operandStr(arg1);
            return ss.str();
        case Op::ALLOC_ARRAY:
            ss << operandStr(result) << " = alloc[" << operandStr(arg1) << "]";
            return ss.str();
        case Op::LOAD_INDEX:
            ss << operandStr(result) << " = " << operandStr(arg1) << "[" << operandStr(arg2) << "]";
            return ss.str();
        case Op::STORE_INDEX:
            ss << operandStr(result) << "[" << operandStr(arg1) << "] = " << operandStr(arg2);
            return ss.str();
        default:   // ADD SUB MUL DIV MOD LT LE GT GE EQ NE AND OR
            ss << operandStr(result) << " = " << operandStr(arg1) << " " << opSymbol(op)
               << " " << operandStr(arg2);
            return ss.str();
    }
}

} // namespace minic
