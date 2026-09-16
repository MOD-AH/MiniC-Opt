// MiniC-Opt — small shared IR-inspection helpers
// Owner: Member 3 (System Designer / Core Algorithms) / Step 5-6
//
// "What does this quad read, and what does it write" is needed by the
// data-flow instantiations (dataflow.cpp) and, starting Step 6, by the
// constant/copy propagation passes as well — both need to tell a "use" of
// a name apart from a "definition" of one. Written once here rather than
// three times.
#pragma once
#include "ir.h"
#include "cfg.h"
#include <string>
#include <vector>

namespace minic {

// Concatenates every block's code in block-id order. Block ids are
// assigned in cfg.cpp in strictly increasing source order and the leader
// partition covers the function's instruction range with no gaps or
// overlaps, so this is a lossless round trip back to a single linear
// IRFunction::code list — the form --dump-ir, --run and the differential-
// execution gate all expect — as long as a pass rewrote operands in place
// without inserting or deleting instructions (true of P1-P3; a pass that
// ever adds/removes quads would need this revisited).
inline std::vector<Quad> flatten(const CFG& cfg) {
    std::vector<Quad> out;
    for (const auto& b : cfg.blocks())
        for (const auto& q : b.code) out.push_back(q);
    return out;
}

inline bool isNameOperand(const Operand& o) {
    return o.kind == OperandKind::VARIABLE || o.kind == OperandKind::TEMPORARY;
}

inline bool isBinaryExprOp(Op op) {
    switch (op) {
        case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
        case Op::LT: case Op::LE: case Op::GT: case Op::GE: case Op::EQ: case Op::NE:
        case Op::AND: case Op::OR:
            return true;
        default:
            return false;
    }
}

// Every operand a quad *reads*, filtered to VARIABLE/TEMPORARY. Notably:
// STORE_INDEX reads its `result` slot too (it names the array being
// mutated through — the array binding itself isn't overwritten, only one
// element of the storage it refers to), and IF_FALSE/PARAM/RET read arg1.
inline std::vector<Operand> readsOf(const Quad& q) {
    std::vector<Operand> r;
    auto add = [&](const Operand& o) { if (isNameOperand(o)) r.push_back(o); };
    switch (q.op) {
        case Op::COPY: case Op::NEG: case Op::NOT: case Op::TO_INT: case Op::TO_FLOAT:
            add(q.arg1); break;
        case Op::LOAD_INDEX:
            add(q.arg1); add(q.arg2); break;
        case Op::STORE_INDEX:
            add(q.arg1); add(q.arg2); add(q.result); break;
        case Op::IF_FALSE: case Op::PARAM:
            add(q.arg1); break;
        case Op::RET:
            add(q.arg1); break;
        case Op::LABEL: case Op::GOTO: case Op::CALL: case Op::NOP: case Op::ALLOC_ARRAY:
            break;
        default:   // ADD SUB MUL DIV MOD LT LE GT GE EQ NE AND OR
            add(q.arg1); add(q.arg2); break;
    }
    return r;
}

// Mutable counterpart of readsOf: pointers to the same slots, so a
// propagation pass can rewrite a use in place. Keep the two switches in
// sync — this one exists only because C++ has no single function that is
// both a const view and a mutable view of the same fields.
inline std::vector<Operand*> readSlotsOf(Quad& q) {
    std::vector<Operand*> r;
    auto add = [&](Operand& o) { if (isNameOperand(o)) r.push_back(&o); };
    switch (q.op) {
        case Op::COPY: case Op::NEG: case Op::NOT: case Op::TO_INT: case Op::TO_FLOAT:
            add(q.arg1); break;
        case Op::LOAD_INDEX:
            add(q.arg1); add(q.arg2); break;
        case Op::STORE_INDEX:
            add(q.arg1); add(q.arg2); add(q.result); break;
        case Op::IF_FALSE: case Op::PARAM:
            add(q.arg1); break;
        case Op::RET:
            add(q.arg1); break;
        case Op::LABEL: case Op::GOTO: case Op::CALL: case Op::NOP: case Op::ALLOC_ARRAY:
            break;
        default:
            add(q.arg1); add(q.arg2); break;
    }
    return r;
}

// The operand a quad *writes*, if any (STORE_INDEX writes no name — it
// mutates through an existing one, per readsOf's comment above).
inline bool writesOf(const Quad& q, Operand& out) {
    switch (q.op) {
        case Op::LABEL: case Op::GOTO: case Op::IF_FALSE: case Op::PARAM:
        case Op::RET: case Op::NOP: case Op::STORE_INDEX:
            return false;
        case Op::CALL:
            if (q.result.isNone()) return false;
            out = q.result; return isNameOperand(out);
        default:
            out = q.result; return isNameOperand(out);
    }
}

inline std::string operandKey(const Operand& o) {
    switch (o.kind) {
        case OperandKind::VARIABLE: case OperandKind::TEMPORARY: case OperandKind::LABEL_REF:
            return o.name;
        case OperandKind::INT_CONST:   return std::to_string(o.ival);
        case OperandKind::FLOAT_CONST: return std::to_string(o.fval);
        case OperandKind::NONE:        return "";
    }
    return "";
}

inline std::string opTag(Op op) {
    switch (op) {
        case Op::ADD: return "+";  case Op::SUB: return "-";
        case Op::MUL: return "*";  case Op::DIV: return "/"; case Op::MOD: return "%";
        case Op::LT: return "<";   case Op::LE: return "<="; case Op::GT: return ">"; case Op::GE: return ">=";
        case Op::EQ: return "=="; case Op::NE: return "!=";
        case Op::AND: return "&&"; case Op::OR: return "||";
        default: return "?";
    }
}

} // namespace minic
