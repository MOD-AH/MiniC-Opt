// Cppify — register bytecode VM (see include/vm.h)
#include "vm.h"
#include "int_semantics.h"
#include <algorithm>
#include <memory>
#include <unordered_map>

namespace minic {
namespace {

using RuntimeError = Interp::RuntimeError;

size_t checkedSlot(const Value& base, const Value& idx) {
    if (!base.arr) {
        if (base.kind == Value::Kind::INT && base.ival == 0) throw RuntimeError{"null pointer dereference"};
        throw RuntimeError{"internal error: indexing a non-pointer value"};
    }
    long long at = base.off + idx.ival;
    if (at < 0 || static_cast<size_t>(at) >= base.arr->size()) throw RuntimeError{"array index out of bounds"};
    return static_cast<size_t>(at);
}

}  // namespace

void VM::compile(const IRProgram& program) {
    functions_.clear();
    builtins_.clear();
    consts_.clear();
    std::unordered_map<std::string, int> fnIndex, globalIndex, builtinIndex;
    for (size_t i = 0; i < program.functions.size(); ++i) fnIndex[program.functions[i].name] = static_cast<int>(i) + 1;

    auto constant = [&](const Operand& o) {
        consts_.push_back(o.kind == OperandKind::INT_CONST ? Value::makeInt(o.ival)
                          : o.kind == OperandKind::FLOAT_CONST ? Value::makeFloat(o.fval)
                          : Value::makeString(o.sval));
        return static_cast<int>(consts_.size()) - 1;
    };

    auto compileFn = [&](const std::string& name, const std::vector<std::string>& params,
                         const std::vector<Quad>& code) {
        Function f;
        f.name = name;
        std::unordered_map<std::string, int> reg;
        for (const auto& p : params) reg.emplace(p, static_cast<int>(reg.size()));
        f.params = static_cast<int>(params.size());
        // Where each label lands once LABEL and NOP are gone.
        std::unordered_map<std::string, int> label;
        int pc = 0;
        for (const Quad& q : code) {
            if (q.op == Op::LABEL) label[q.result.name] = pc;
            else if (q.op != Op::NOP) ++pc;
        }
        // The same name routing as Interp::evalOperand: a temporary or a
        // '#' name is per call, any other name is global.
        auto slot = [&](const Operand& o) {
            Slot s;
            switch (o.kind) {
                case OperandKind::INT_CONST: case OperandKind::FLOAT_CONST: case OperandKind::STRING_CONST:
                    s.where = Where::CONST; s.index = constant(o); break;
                case OperandKind::TEMPORARY:
                    s.where = Where::REG; s.index = reg.emplace(o.name, static_cast<int>(reg.size())).first->second; break;
                case OperandKind::VARIABLE:
                    if (o.name.find('#') != std::string::npos) {
                        s.where = Where::REG; s.index = reg.emplace(o.name, static_cast<int>(reg.size())).first->second;
                    } else {
                        s.where = Where::GLOBAL;
                        s.index = globalIndex.emplace(o.name, static_cast<int>(globalIndex.size())).first->second;
                    }
                    break;
                default: break;
            }
            return s;
        };
        auto jump = [&](const Operand& l) {
            auto it = label.find(l.name);
            return it == label.end() ? -1 : it->second;
        };
        for (const Quad& q : code) {
            if (q.op == Op::LABEL || q.op == Op::NOP) continue;
            Inst in;
            in.op = q.op;
            in.ty = q.ty;
            switch (q.op) {
                case Op::GOTO:
                    in.target = jump(q.result);
                    break;
                case Op::IF_FALSE:
                    in.a = slot(q.arg1);
                    in.target = jump(q.result);
                    break;
                case Op::CALL: {
                    auto fi = fnIndex.find(q.arg1.name);
                    if (fi != fnIndex.end()) in.target = fi->second;
                    else in.target = -1 - builtinIndex.emplace(q.arg1.name, static_cast<int>(builtinIndex.size())).first->second;
                    in.argc = static_cast<std::int32_t>(q.arg2.ival);
                    in.r = slot(q.result);
                    break;
                }
                default:
                    in.a = slot(q.arg1);
                    in.b = slot(q.arg2);
                    in.r = slot(q.result);
            }
            f.code.push_back(in);
        }
        f.registers = static_cast<int>(reg.size());
        functions_.push_back(std::move(f));
    };

    compileFn("<global init>", {}, program.globalInit);
    for (const IRFunction& fn : program.functions) compileFn(fn.name, fn.params, fn.code);
    builtins_.assign(builtinIndex.size(), "");
    for (const auto& kv : builtinIndex) builtins_[static_cast<size_t>(kv.second)] = kv.first;
    globals_.assign(globalIndex.size(), Value::makeInt(0));
}

Value VM::run(const IRProgram& program) {
    compile(program);
    lib_.resetLibrary();
    pending_.clear();
    try {
        std::vector<Value> initRegs(static_cast<size_t>(functions_[0].registers), Value::makeInt(0));
        execute(0, initRegs);
        for (size_t i = 1; i < functions_.size(); ++i)
            if (functions_[i].name == "main") {
                std::vector<Value> regs(static_cast<size_t>(functions_[i].registers), Value::makeInt(0));
                return execute(static_cast<int>(i), regs);
            }
        throw RuntimeError{"no function named 'main'"};
    } catch (const Interp::ProgramExit& e) {
        return Value::makeInt(e.code);   // exit(n) ends the program with status n
    }
}

// Mirrors Interp::execCode opcode for opcode; only the operand access and
// the jumps differ.
Value VM::execute(int fi, std::vector<Value>& regs) {
    static const Value none = Value::makeInt(0);
    const Function& f = functions_[static_cast<size_t>(fi)];
    const Inst* code = f.code.data();
    const size_t n = f.code.size();
    auto get = [&](const Slot& s) -> const Value& {
        switch (s.where) {
            case Where::REG:    return regs[static_cast<size_t>(s.index)];
            case Where::GLOBAL: return globals_[static_cast<size_t>(s.index)];
            case Where::CONST:  return consts_[static_cast<size_t>(s.index)];
            default:            return none;
        }
    };
    auto put = [&](const Slot& s, Value v) {
        if (s.where == Where::REG) regs[static_cast<size_t>(s.index)] = std::move(v);
        else if (s.where == Where::GLOBAL) globals_[static_cast<size_t>(s.index)] = std::move(v);
        else throw RuntimeError{"internal error: operand kind is not storable"};
    };
    auto jumpTo = [](std::int32_t t) -> size_t {
        if (t < 0) throw RuntimeError{"internal error: jump to undefined label"};
        return static_cast<size_t>(t);
    };

    size_t pc = 0;
    while (pc < n) {
        const Inst& in = code[pc++];
        switch (in.op) {
            case Op::GOTO:
                pc = jumpTo(in.target);
                break;
            case Op::IF_FALSE:
                if (!get(in.a).truthy()) pc = jumpTo(in.target);
                break;
            case Op::COPY:
                put(in.r, get(in.a));
                break;
            case Op::NEG: {
                const Value& v = get(in.a);
                put(in.r, v.kind == Value::Kind::FLOAT ? Value::makeFloat(-v.fval)
                          : Value::makeInt(wrapInt(static_cast<long long>(0 - static_cast<unsigned long long>(v.ival)), in.ty)));
                break;
            }
            case Op::NOT:
                put(in.r, Value::makeInt(get(in.a).truthy() ? 0 : 1));
                break;
            case Op::BITNOT:
                put(in.r, Value::makeInt(wrapInt(~get(in.a).ival, in.ty)));
                break;
            case Op::TO_INT: {
                const Value& v = get(in.a);
                put(in.r, Value::makeInt(wrapInt(v.kind == Value::Kind::FLOAT ? static_cast<long long>(v.fval) : v.ival, in.ty)));
                break;
            }
            case Op::TO_FLOAT: {
                const Value& v = get(in.a);
                put(in.r, Value::makeFloat(v.kind == Value::Kind::FLOAT ? v.fval : static_cast<double>(v.ival)));
                break;
            }
            case Op::ALLOC_ARRAY: {
                long long size = get(in.a).ival;
                put(in.r, Value::makeArray(std::make_shared<std::vector<Value>>(
                              static_cast<size_t>(size < 0 ? 0 : size), Value::makeInt(0))));
                break;
            }
            case Op::LOAD_INDEX: {
                const Value& base = get(in.a);
                put(in.r, (*base.arr)[checkedSlot(base, get(in.b))]);
                break;
            }
            case Op::STORE_INDEX: {
                const Value& base = get(in.r);   // result[arg1] = arg2
                size_t at = checkedSlot(base, get(in.a));
                (*base.arr)[at] = get(in.b);
                break;
            }
            case Op::COPY_BLOCK: {
                Value src = get(in.a), dst = get(in.r);
                long long count = get(in.b).ival;
                if (!dst.arr || !src.arr) {
                    bool nullish = (!dst.arr && dst.kind == Value::Kind::INT && dst.ival == 0) ||
                                   (!src.arr && src.kind == Value::Kind::INT && src.ival == 0);
                    throw RuntimeError{nullish ? "null pointer dereference" : "internal error: copying a non-pointer value"};
                }
                if (count > 0) {
                    if (src.off < 0 || static_cast<size_t>(src.off + count) > src.arr->size() ||
                        dst.off < 0 || static_cast<size_t>(dst.off + count) > dst.arr->size())
                        throw RuntimeError{"array index out of bounds"};
                    std::vector<Value> tmp(src.arr->begin() + src.off, src.arr->begin() + src.off + count);
                    std::copy(tmp.begin(), tmp.end(), dst.arr->begin() + dst.off);
                }
                break;
            }
            case Op::PARAM:
                pending_.push_back(get(in.a));
                break;
            case Op::CALL: {
                if (in.argc < 0 || static_cast<size_t>(in.argc) > pending_.size())
                    throw RuntimeError{"internal error: call has " + std::to_string(in.argc) +
                                       " pending argument(s), only " + std::to_string(pending_.size()) + " available"};
                std::vector<Value> args(pending_.end() - in.argc, pending_.end());
                pending_.erase(pending_.end() - in.argc, pending_.end());
                Value result;
                if (in.target > 0) {
                    const Function& callee = functions_[static_cast<size_t>(in.target)];
                    std::vector<Value> frame(static_cast<size_t>(callee.registers), Value::makeInt(0));
                    for (int i = 0; i < callee.params && static_cast<size_t>(i) < args.size(); ++i)
                        frame[static_cast<size_t>(i)] = std::move(args[static_cast<size_t>(i)]);
                    result = execute(in.target, frame);
                } else {
                    result = lib_.callBuiltin(builtins_[static_cast<size_t>(-1 - in.target)], args);
                }
                if (in.r.where != Where::NONE) put(in.r, std::move(result));
                break;
            }
            case Op::RET:
                return in.a.where == Where::NONE ? Value::makeInt(0) : get(in.a);
            default:   // ADD SUB MUL DIV MOD LT LE GT GE EQ NE AND OR BITAND BITOR BITXOR SHL SHR
                put(in.r, Interp::applyBinary(in.op, get(in.a), get(in.b), in.ty));
        }
    }
    return Value::makeInt(0);   // fell off the end: implicit return
}

}  // namespace minic
