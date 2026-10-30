// Cppify fork — C integer semantics, shared by the interpreter and constant
// folding. Both used to carry their own copy of the arithmetic; with widths
// and signedness in play that is too easy to let drift, and a fold that
// disagrees with the run is exactly what the differential gate exists to
// catch. So there is one definition, here.
//
// Values live in a long long. A value of type T is always kept in T's range:
// signed types sign-extended, unsigned types zero-extended (u64 uses all 64
// bits, so it looks negative in the long long). That invariant is what lets
// plain signed 64-bit compare, divide and shift give the right answer for
// every type except u64, which is the one case that needs unsigned ops.
#pragma once
#include "ir.h"

namespace minic {

inline int intBits(IntTy t) {
    switch (t) {
        case IntTy::I8:  case IntTy::U8:  return 8;
        case IntTy::I16: case IntTy::U16: return 16;
        case IntTy::I32: case IntTy::U32: return 32;
        default:                          return 64;
    }
}
inline bool isUnsignedTy(IntTy t) {
    return t == IntTy::U8 || t == IntTy::U16 || t == IntTy::U32 || t == IntTy::U64;
}

// Reduce v modulo 2^bits into T's range. Signed overflow is undefined in C;
// this wraps, which is what clang's output does in practice at -O0.
inline long long wrapInt(long long v, IntTy t) {
    int bits = intBits(t);
    if (bits == 64) return v;
    unsigned long long mask = (1ULL << bits) - 1;
    unsigned long long u = static_cast<unsigned long long>(v) & mask;
    if (!isUnsignedTy(t) && (u >> (bits - 1)) & 1ULL) u |= ~mask;   // sign-extend
    return static_cast<long long>(u);
}

// One integer binary operation at type t. Returns nullptr on success, or the
// runtime error message where C is undefined and we trap instead (zero
// divisor, shift count outside [0, width)). Arithmetic goes through unsigned
// long long so the C++ itself never overflows.
inline const char* intBinary(Op op, long long a, long long b, IntTy t, long long& out) {
    using U = unsigned long long;
    const bool uns = isUnsignedTy(t);
    const U ua = static_cast<U>(a), ub = static_cast<U>(b);
    auto cmp = [&](bool lt, bool eq) -> long long {
        bool less = uns ? ua < ub : a < b;
        return (lt && less) || (eq && a == b) ? 1 : 0;
    };
    switch (op) {
        case Op::ADD: out = static_cast<long long>(ua + ub); break;
        case Op::SUB: out = static_cast<long long>(ua - ub); break;
        case Op::MUL: out = static_cast<long long>(ua * ub); break;
        case Op::DIV:
        case Op::MOD:
            if (b == 0) return op == Op::DIV ? "division by zero" : "modulo by zero";
            if (uns) out = static_cast<long long>(op == Op::DIV ? ua / ub : ua % ub);
            else if (b == -1) out = op == Op::DIV ? static_cast<long long>(0 - ua) : 0;   // LLONG_MIN / -1
            else out = op == Op::DIV ? a / b : a % b;
            break;
        case Op::BITAND: out = a & b; break;
        case Op::BITOR:  out = a | b; break;
        case Op::BITXOR: out = a ^ b; break;
        case Op::SHL:
        case Op::SHR:
            if (b < 0 || b >= intBits(t)) return "shift count out of range";
            if (op == Op::SHL) out = static_cast<long long>(ua << b);
            else out = uns ? static_cast<long long>(ua >> b) : (a >> b);
            break;
        // Comparisons yield an int 0/1; t only says how to read the operands.
        case Op::LT: out = cmp(true, false);  return nullptr;
        case Op::LE: out = cmp(true, true);   return nullptr;
        case Op::GT: out = 1 - cmp(true, true);  return nullptr;
        case Op::GE: out = 1 - cmp(true, false); return nullptr;
        case Op::EQ: out = a == b; return nullptr;
        case Op::NE: out = a != b; return nullptr;
        default: return "internal error: not an integer binary opcode";
    }
    out = wrapInt(out, t);
    return nullptr;
}

inline const char* intTyName(IntTy t) {
    switch (t) {
        case IntTy::I8: return "i8";   case IntTy::U8: return "u8";
        case IntTy::I16: return "i16"; case IntTy::U16: return "u16";
        case IntTy::I32: return "i32"; case IntTy::U32: return "u32";
        case IntTy::I64: return "i64"; case IntTy::U64: return "u64";
        default: return "";
    }
}

} // namespace minic
