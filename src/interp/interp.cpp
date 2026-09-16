// MiniC-Opt — reference interpreter (implementation)
// Owner: Member 4 (Testing / Pass Manager) / Step 4
//
// Variable partitioning: IRGen (src/ir/irgen.cpp) guarantees every local
// name is qualified as "name#offset" and every global is a bare name with
// no '#' — so a VARIABLE operand's own spelling says unambiguously which
// map it lives in (this frame, or globals_). TEMPORARY operands are always
// this call's own scratch storage. That is the only thing this file relies
// on to route reads and writes correctly; see evalOperand/store below.
#include "interp.h"
#include <iostream>

namespace minic {

Value Interp::evalOperand(const Operand& o, std::unordered_map<std::string, Value>& frame) {
    switch (o.kind) {
        case OperandKind::INT_CONST:   return Value::makeInt(o.ival);
        case OperandKind::FLOAT_CONST: return Value::makeFloat(o.fval);
        case OperandKind::TEMPORARY: {
            auto it = frame.find(o.name);
            return it != frame.end() ? it->second : Value::makeInt(0);
        }
        case OperandKind::VARIABLE: {
            if (o.name.find('#') != std::string::npos) {
                auto it = frame.find(o.name);
                return it != frame.end() ? it->second : Value::makeInt(0);
            }
            auto it = globals_.find(o.name);
            return it != globals_.end() ? it->second : Value::makeInt(0);
        }
        default:
            throw RuntimeError{"internal error: operand kind is not evaluable"};
    }
}

void Interp::store(const Operand& o, const Value& v, std::unordered_map<std::string, Value>& frame) {
    switch (o.kind) {
        case OperandKind::TEMPORARY:
            frame[o.name] = v;
            return;
        case OperandKind::VARIABLE:
            if (o.name.find('#') != std::string::npos) frame[o.name] = v;
            else                                        globals_[o.name] = v;
            return;
        default:
            throw RuntimeError{"internal error: operand kind is not storable"};
    }
}

Value Interp::applyBinary(Op op, const Value& a, const Value& b) {
    bool useFloat = (a.kind == Value::Kind::FLOAT || b.kind == Value::Kind::FLOAT);
    switch (op) {
        case Op::ADD: return useFloat ? Value::makeFloat(a.asDouble() + b.asDouble())
                                       : Value::makeInt(a.ival + b.ival);
        case Op::SUB: return useFloat ? Value::makeFloat(a.asDouble() - b.asDouble())
                                       : Value::makeInt(a.ival - b.ival);
        case Op::MUL: return useFloat ? Value::makeFloat(a.asDouble() * b.asDouble())
                                       : Value::makeInt(a.ival * b.ival);
        case Op::DIV:
            if (useFloat) return Value::makeFloat(a.asDouble() / b.asDouble());
            if (b.ival == 0) throw RuntimeError{"division by zero"};
            return Value::makeInt(a.ival / b.ival);
        case Op::MOD:
            if (b.ival == 0) throw RuntimeError{"modulo by zero"};
            return Value::makeInt(a.ival % b.ival);   // MOD is integer-only in MiniC's grammar
        case Op::LT: return Value::makeInt(useFloat ? (a.asDouble() <  b.asDouble()) : (a.ival <  b.ival));
        case Op::LE: return Value::makeInt(useFloat ? (a.asDouble() <= b.asDouble()) : (a.ival <= b.ival));
        case Op::GT: return Value::makeInt(useFloat ? (a.asDouble() >  b.asDouble()) : (a.ival >  b.ival));
        case Op::GE: return Value::makeInt(useFloat ? (a.asDouble() >= b.asDouble()) : (a.ival >= b.ival));
        case Op::EQ: return Value::makeInt(useFloat ? (a.asDouble() == b.asDouble()) : (a.ival == b.ival));
        case Op::NE: return Value::makeInt(useFloat ? (a.asDouble() != b.asDouble()) : (a.ival != b.ival));
        case Op::AND: return Value::makeInt((a.truthy() && b.truthy()) ? 1 : 0);
        case Op::OR:  return Value::makeInt((a.truthy() || b.truthy()) ? 1 : 0);
        default:
            throw RuntimeError{"internal error: not a binary opcode"};
    }
}

Value Interp::callBuiltin(const std::string& name, std::vector<Value>& args) {
    if (name == "print_int") {
        std::cout << (args.empty() ? 0 : args[0].ival) << "\n";
        return Value::makeInt(0);
    }
    if (name == "print_float") {
        std::cout << (args.empty() ? 0.0 : args[0].asDouble()) << "\n";
        return Value::makeInt(0);
    }
    if (name == "print_char") {
        std::cout << static_cast<char>(args.empty() ? 0 : args[0].ival);
        return Value::makeInt(0);
    }
    if (name == "read_int") {
        long long v = 0;
        std::cin >> v;
        return Value::makeInt(v);
    }
    throw RuntimeError{"call to undefined function '" + name + "'"};
}

Interp::ExecResult Interp::execCode(const std::vector<Quad>& code, std::unordered_map<std::string, Value>& frame) {
    std::unordered_map<std::string, size_t> labelMap;
    for (size_t i = 0; i < code.size(); ++i)
        if (code[i].op == Op::LABEL) labelMap[code[i].result.name] = i;

    size_t pc = 0;
    while (pc < code.size()) {
        const Quad& q = code[pc];
        // Review 3 addition (--metrics dynamic instruction count): every
        // executed quad counts except LABEL, which is a pure marker that
        // performs no work — matching docs/worked_example.md's own hand
        // count (125 dynamic instructions for its 13-iteration loop),
        // which was worked out on exactly that convention.
        if (q.op != Op::LABEL) ++instructionsExecuted;
        switch (q.op) {
            case Op::LABEL:
            case Op::NOP:
                break;

            case Op::GOTO:
                pc = labelMap.at(q.result.name);
                continue;

            case Op::IF_FALSE: {
                Value v = evalOperand(q.arg1, frame);
                if (!v.truthy()) { pc = labelMap.at(q.result.name); continue; }
                break;
            }

            case Op::COPY:
                store(q.result, evalOperand(q.arg1, frame), frame);
                break;

            case Op::NEG: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result,
                      v.kind == Value::Kind::FLOAT ? Value::makeFloat(-v.fval) : Value::makeInt(-v.ival),
                      frame);
                break;
            }
            case Op::NOT: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result, Value::makeInt(v.truthy() ? 0 : 1), frame);
                break;
            }
            case Op::TO_INT: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result,
                      Value::makeInt(v.kind == Value::Kind::FLOAT ? static_cast<long long>(v.fval) : v.ival),
                      frame);
                break;
            }
            case Op::TO_FLOAT: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result,
                      Value::makeFloat(v.kind == Value::Kind::FLOAT ? v.fval : static_cast<double>(v.ival)),
                      frame);
                break;
            }
            case Op::ALLOC_ARRAY: {
                Value sz = evalOperand(q.arg1, frame);
                auto vec = std::make_shared<std::vector<Value>>(
                    static_cast<size_t>(sz.ival < 0 ? 0 : sz.ival), Value::makeInt(0));
                store(q.result, Value::makeArray(std::move(vec)), frame);
                break;
            }
            case Op::LOAD_INDEX: {
                Value base = evalOperand(q.arg1, frame);
                Value idx  = evalOperand(q.arg2, frame);
                if (!base.arr) throw RuntimeError{"internal error: indexing a non-array value"};
                if (idx.ival < 0 || static_cast<size_t>(idx.ival) >= base.arr->size())
                    throw RuntimeError{"array index out of bounds"};
                store(q.result, (*base.arr)[static_cast<size_t>(idx.ival)], frame);
                break;
            }
            case Op::STORE_INDEX: {
                // result[arg1] = arg2 — result names the array (see ir.h).
                Value base = evalOperand(q.result, frame);
                Value idx  = evalOperand(q.arg1, frame);
                Value val  = evalOperand(q.arg2, frame);
                if (!base.arr) throw RuntimeError{"internal error: indexing a non-array value"};
                if (idx.ival < 0 || static_cast<size_t>(idx.ival) >= base.arr->size())
                    throw RuntimeError{"array index out of bounds"};
                (*base.arr)[static_cast<size_t>(idx.ival)] = val;
                break;
            }

            case Op::PARAM:
                pendingParams_.push_back(evalOperand(q.arg1, frame));
                break;

            case Op::CALL: {
                const std::string& callee = q.arg1.name;
                long long argc = q.arg2.ival;
                std::vector<Value> args;
                if (argc > 0) {
                    args.assign(pendingParams_.end() - argc, pendingParams_.end());
                    pendingParams_.erase(pendingParams_.end() - argc, pendingParams_.end());
                }
                Value result;
                auto fit = funcsByName_.find(callee);
                if (fit != funcsByName_.end()) {
                    ExecResult r = execFunction(*fit->second, std::move(args));
                    result = r.returned ? r.value : Value::makeInt(0);
                } else {
                    result = callBuiltin(callee, args);
                }
                if (!q.result.isNone()) store(q.result, result, frame);
                break;
            }

            case Op::RET: {
                Value v = q.arg1.isNone() ? Value::makeInt(0) : evalOperand(q.arg1, frame);
                return {true, v};
            }

            default:   // ADD SUB MUL DIV MOD LT LE GT GE EQ NE AND OR
                store(q.result, applyBinary(q.op, evalOperand(q.arg1, frame), evalOperand(q.arg2, frame)), frame);
                break;
        }
        ++pc;
    }
    return {false, Value::makeInt(0)};   // fell off the end: implicit return (void functions)
}

Interp::ExecResult Interp::execFunction(const IRFunction& fn, std::vector<Value> args) {
    std::unordered_map<std::string, Value> frame;
    for (size_t i = 0; i < fn.params.size(); ++i)
        frame[fn.params[i]] = (i < args.size() ? args[i] : Value::makeInt(0));
    return execCode(fn.code, frame);
}

Value Interp::run(const IRProgram& program, const std::string& entry) {
    instructionsExecuted = 0;
    funcsByName_.clear();
    for (const auto& fn : program.functions) funcsByName_[fn.name] = &fn;
    globals_.clear();
    pendingParams_.clear();

    std::unordered_map<std::string, Value> initFrame;   // only temps ever land here; see file header
    execCode(program.globalInit, initFrame);

    auto it = funcsByName_.find(entry);
    if (it == funcsByName_.end()) throw RuntimeError{"no function named '" + entry + "'"};
    ExecResult r = execFunction(*it->second, {});
    return r.returned ? r.value : Value::makeInt(0);
}

} // namespace minic
