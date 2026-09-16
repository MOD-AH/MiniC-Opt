// MiniC-Opt — backend: linearisation and stack-slot assignment
// Owner: Member 4 (Implementation, Testing and Planning Coordinator) / Review 3
// See include/backend.h for why this stage sits outside the differential-
// execution gate.
#include "backend.h"
#include <sstream>
#include <unordered_map>

namespace minic {
namespace {

class SlotAssigner {
public:
    std::string slotOf(const std::string& name) {
        auto it = slots_.find(name);
        if (it != slots_.end()) return "[" + std::to_string(it->second) + "]";
        int id = next_++;
        slots_[name] = id;
        return "[" + std::to_string(id) + "]";
    }

private:
    std::unordered_map<std::string, int> slots_;
    int next_ = 0;
};

std::string operandText(const Operand& o, SlotAssigner& slots) {
    switch (o.kind) {
        case OperandKind::VARIABLE:
        case OperandKind::TEMPORARY:
            return slots.slotOf(o.name);
        case OperandKind::LABEL_REF:
            return o.name;
        case OperandKind::INT_CONST:
            return "#" + std::to_string(o.ival);
        case OperandKind::FLOAT_CONST: {
            std::ostringstream ss; ss << "#" << o.fval; return ss.str();
        }
        case OperandKind::NONE:
            return "";
    }
    return "";
}

std::string opMnemonic(Op op) {
    switch (op) {
        case Op::ADD: return "ADD";   case Op::SUB: return "SUB";
        case Op::MUL: return "MUL";   case Op::DIV: return "DIV"; case Op::MOD: return "MOD";
        case Op::LT: return "LT";     case Op::LE: return "LE";
        case Op::GT: return "GT";     case Op::GE: return "GE";
        case Op::EQ: return "EQ";     case Op::NE: return "NE";
        case Op::AND: return "AND";   case Op::OR: return "OR";
        case Op::NEG: return "NEG";   case Op::NOT: return "NOT";
        case Op::COPY: return "MOV";
        case Op::LOAD_INDEX: return "LOAD";
        case Op::STORE_INDEX: return "STORE";
        case Op::TO_INT: return "CVTI";
        case Op::TO_FLOAT: return "CVTF";
        case Op::ALLOC_ARRAY: return "ALLOC";
        case Op::LABEL: return "LABEL";
        case Op::GOTO: return "GOTO";
        case Op::IF_FALSE: return "IFFALSE";
        case Op::PARAM: return "PARAM";
        case Op::CALL: return "CALL";
        case Op::RET: return "RET";
        case Op::NOP: return "NOP";
    }
    return "?";
}

std::string emitQuad(const Quad& q, SlotAssigner& slots) {
    std::ostringstream ss;
    switch (q.op) {
        case Op::LABEL:
            ss << q.result.name << ":";
            return ss.str();
        case Op::GOTO:
            ss << "  GOTO " << q.result.name;
            return ss.str();
        case Op::IF_FALSE:
            ss << "  IFFALSE " << operandText(q.arg1, slots) << ", " << q.result.name;
            return ss.str();
        case Op::PARAM:
            ss << "  PARAM " << operandText(q.arg1, slots);
            return ss.str();
        case Op::CALL:
            ss << "  CALL ";
            if (!q.result.isNone()) ss << operandText(q.result, slots) << ", ";
            ss << q.arg1.name << ", " << q.arg2.ival;
            return ss.str();
        case Op::RET:
            ss << "  RET";
            if (!q.arg1.isNone()) ss << " " << operandText(q.arg1, slots);
            return ss.str();
        case Op::NOP:
            return "  NOP";
        case Op::COPY:
            ss << "  MOV " << operandText(q.result, slots) << ", " << operandText(q.arg1, slots);
            return ss.str();
        case Op::NEG: case Op::NOT: case Op::TO_INT: case Op::TO_FLOAT:
            ss << "  " << opMnemonic(q.op) << " " << operandText(q.result, slots)
               << ", " << operandText(q.arg1, slots);
            return ss.str();
        case Op::ALLOC_ARRAY:
            ss << "  ALLOC " << operandText(q.result, slots) << ", " << operandText(q.arg1, slots);
            return ss.str();
        case Op::LOAD_INDEX:
            ss << "  LOAD " << operandText(q.result, slots) << ", "
               << operandText(q.arg1, slots) << "[" << operandText(q.arg2, slots) << "]";
            return ss.str();
        case Op::STORE_INDEX:
            ss << "  STORE " << operandText(q.result, slots) << "[" << operandText(q.arg1, slots)
               << "], " << operandText(q.arg2, slots);
            return ss.str();
        default:   // ADD SUB MUL DIV MOD LT LE GT GE EQ NE AND OR
            ss << "  " << opMnemonic(q.op) << " " << operandText(q.result, slots) << ", "
               << operandText(q.arg1, slots) << ", " << operandText(q.arg2, slots);
            return ss.str();
    }
}

} // namespace

std::string emitFunction(const IRFunction& fn) {
    std::ostringstream out;
    SlotAssigner slots;
    for (const std::string& p : fn.params) slots.slotOf(p);   // params claim the first slots
    out << fn.name << ":\n";
    for (const Quad& q : fn.code) out << emitQuad(q, slots) << "\n";
    return out.str();
}

std::string emitProgram(const IRProgram& program) {
    std::ostringstream out;
    if (!program.globalInit.empty()) {
        SlotAssigner slots;
        out << "; -- global init --\n";
        for (const Quad& q : program.globalInit) out << emitQuad(q, slots) << "\n";
    }
    for (const IRFunction& fn : program.functions) out << "\n" << emitFunction(fn);
    return out.str();
}

} // namespace minic
