// Cppify — basic alias analysis (roadmap 2.5), for the SSA passes
//
// Memory in this IR is an array object; a pointer is (object, offset). The
// question an optimizer asks is "can this store change what that load
// reads?". The answer here is deliberately simple and sound:
//
//   * Each ALLOC_ARRAY in the function is its own object. A name assigned
//     from it (copied, or offset by pointer arithmetic) points into it.
//   * Two names known to point into different objects never alias.
//   * Anything else (a parameter, a global, a pointer loaded from memory or
//     returned by a call) is unknown, and may alias anything.
//   * A call clobbers all memory, except a fixed list of library functions
//     that only read memory or touch none.
//
// Not attempted: offsets within one object (a[0] vs a[1]), and global
// arrays, whose allocation lives in globalInit, outside the function.
#pragma once
#include "cfg.h"
#include "ir_utils.h"
#include <map>
#include <set>
#include <string>

namespace minic {

class AliasInfo {
public:
    explicit AliasInfo(const CFG& cfg) {
        std::map<std::string, int> defs;
        for (const auto& b : cfg.blocks()) {
            for (const Phi& phi : b.phis) ++defs[phi.result.name];
            for (const Quad& q : b.code) {
                Operand w;
                if (writesOf(q, w)) ++defs[w.name];
            }
        }
        for (const auto& b : cfg.blocks())
            for (const Quad& q : b.code)
                if (q.op == Op::ALLOC_ARRAY && q.arg1.kind == OperandKind::INT_CONST) {
                    object_[q.result.name] = q.result.name;
                    if (defs[q.result.name] == 1) size_[q.result.name] = q.arg1.ival;
                }
        // A pointer derived from a known object by a copy or by pointer
        // arithmetic points into the same object. Only single-definition
        // names qualify (in SSA, every renamed value).
        for (bool changed = true; changed; ) {
            changed = false;
            for (const auto& b : cfg.blocks())
                for (const Quad& q : b.code) {
                    if (!isNameOperand(q.result) || defs[q.result.name] != 1 || object_.count(q.result.name)) continue;
                    bool derives = q.op == Op::COPY ||
                                   ((q.op == Op::ADD || q.op == Op::SUB) && !isNameOperand(q.arg2));
                    if (!derives || !isNameOperand(q.arg1)) continue;
                    auto it = object_.find(q.arg1.name);
                    if (it == object_.end()) continue;
                    object_[q.result.name] = it->second;
                    changed = true;
                }
        }
    }

    // The object `name` points into, or "" if unknown.
    std::string objectOf(const std::string& name) const {
        auto it = object_.find(name);
        return it == object_.end() ? "" : it->second;
    }

    bool mayAlias(const std::string& a, const std::string& b) const {
        std::string oa = objectOf(a), ob = objectOf(b);
        return oa.empty() || ob.empty() || oa == ob;
    }

    // Slots in the object `name` itself names (not a derived pointer), or -1.
    long long allocSize(const std::string& name) const {
        auto it = size_.find(name);
        return it == size_.end() ? -1 : it->second;
    }

    static bool callWritesMemory(const Quad& call) {
        static const std::set<std::string> readOnly = {
            "printf", "puts", "putchar", "print_int", "print_float",
            "strlen", "strcmp", "strncmp", "strchr", "strrchr", "strstr", "strspn", "strcspn", "memcmp",
            "atoi", "atol", "atof", "abs", "labs", "fabs", "sqrt", "cbrt", "pow", "sin", "cos", "tan",
            "exp", "log", "log10", "log2", "floor", "ceil", "round", "trunc", "fmod", "fmin", "fmax", "hypot",
            "isdigit", "isalpha", "isalnum", "isspace", "isupper", "islower", "ispunct", "isxdigit",
            "toupper", "tolower"};
        return !readOnly.count(call.arg1.name);
    }

private:
    std::map<std::string, std::string> object_;
    std::map<std::string, long long> size_;
};

}  // namespace minic
