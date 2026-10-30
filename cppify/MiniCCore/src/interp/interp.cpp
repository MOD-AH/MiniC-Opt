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
#include "int_semantics.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

namespace minic {

Value Interp::evalOperand(const Operand& o, std::unordered_map<std::string, Value>& frame) {
    switch (o.kind) {
        case OperandKind::INT_CONST:   return Value::makeInt(o.ival);
        case OperandKind::FLOAT_CONST: return Value::makeFloat(o.fval);
        case OperandKind::STRING_CONST: return Value::makeString(o.sval);
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

// Pointer operands. A pointer is (array, offset); NULL is the integer 0.
// Only the operations C defines on pointers are accepted — everything else
// is an internal error, because Sema has already rejected it.
static Value applyPointerBinary(Op op, const Value& a, const Value& b) {
    bool ap = a.kind == Value::Kind::ARRAY, bp = b.kind == Value::Kind::ARRAY;
    auto withOff = [](const Value& p, long long off) {
        Value r = p; r.off = off; return r;
    };
    switch (op) {
        case Op::ADD:
            if (ap && !bp) return withOff(a, a.off + b.ival);
            if (bp && !ap) return withOff(b, b.off + a.ival);
            break;
        case Op::SUB:
            if (ap && !bp) return withOff(a, a.off - b.ival);
            if (ap && bp) {
                if (a.arr != b.arr)
                    throw Interp::RuntimeError{"subtraction of pointers into different objects"};
                return Value::makeInt(a.off - b.off);
            }
            break;
        case Op::EQ: case Op::NE: {
            bool same = ap && bp ? (a.arr == b.arr && a.off == b.off) : false;
            // A pointer never equals an integer: NULL is 0 and no live
            // pointer is.
            return Value::makeInt((op == Op::EQ) == same ? 1 : 0);
        }
        case Op::LT: case Op::LE: case Op::GT: case Op::GE: {
            if (!(ap && bp) || a.arr != b.arr)
                throw Interp::RuntimeError{"ordering comparison of unrelated pointers"};
            bool r = op == Op::LT ? a.off <  b.off : op == Op::LE ? a.off <= b.off
                   : op == Op::GT ? a.off >  b.off : a.off >= b.off;
            return Value::makeInt(r ? 1 : 0);
        }
        case Op::AND: return Value::makeInt((a.truthy() && b.truthy()) ? 1 : 0);
        case Op::OR:  return Value::makeInt((a.truthy() || b.truthy()) ? 1 : 0);
        default: break;
    }
    throw Interp::RuntimeError{"internal error: invalid operation on a pointer"};
}

Value Interp::applyBinary(Op op, const Value& a, const Value& b, IntTy ty) {
    if (a.kind == Value::Kind::ARRAY || b.kind == Value::Kind::ARRAY)
        return applyPointerBinary(op, a, b);
    if (op == Op::AND) return Value::makeInt((a.truthy() && b.truthy()) ? 1 : 0);
    if (op == Op::OR)  return Value::makeInt((a.truthy() || b.truthy()) ? 1 : 0);
    bool useFloat = (a.kind == Value::Kind::FLOAT || b.kind == Value::Kind::FLOAT);
    // Bitwise ops never get here with a float (sema rejects it), so only the
    // arithmetic and comparison ops have a float form.
    if (useFloat && op != Op::BITAND && op != Op::BITOR && op != Op::BITXOR
                 && op != Op::SHL && op != Op::SHR && op != Op::MOD) {
        double x = a.asDouble(), y = b.asDouble();
        switch (op) {
            case Op::ADD: return Value::makeFloat(x + y);
            case Op::SUB: return Value::makeFloat(x - y);
            case Op::MUL: return Value::makeFloat(x * y);
            case Op::DIV:
                if (y == 0.0) throw RuntimeError{"division by zero"};
                return Value::makeFloat(x / y);
            case Op::LT: return Value::makeInt(x <  y);
            case Op::LE: return Value::makeInt(x <= y);
            case Op::GT: return Value::makeInt(x >  y);
            case Op::GE: return Value::makeInt(x >= y);
            case Op::EQ: return Value::makeInt(x == y);
            case Op::NE: return Value::makeInt(x != y);
            default: throw RuntimeError{"internal error: not a binary opcode"};
        }
    }
    // Integers: one definition shared with constant folding (int_semantics.h).
    // A negative or over-wide shift and a zero divisor trap rather than
    // compute, so the unoptimized and optimized runs agree; the optimizer
    // treats DIV/MOD/SHL/SHR as may-trap for the same reason.
    long long out = 0;
    if (const char* err = intBinary(op, a.ival, b.ival, ty, out)) throw RuntimeError{err};
    return Value::makeInt(out);
}

// ---- C library support ---------------------------------------------------
//
// A "char *" here is an ARRAY Value (shared storage + offset) whose slots hold
// the character codes, ending in a 0 slot. Everything below reads and writes
// through that representation, bounds-checking every access: a missing NUL or
// an undersized buffer is a reported runtime error, never silent corruption.

namespace {


long long asInt(const Value& v) {
    return v.kind == Value::Kind::FLOAT ? static_cast<long long>(v.fval) : v.ival;
}

bool isNullValue(const Value& v) { return !v.arr && v.kind == Value::Kind::INT && v.ival == 0; }

std::string cstr(const Value& v) {
    if (v.kind == Value::Kind::STRING) return v.sval;
    if (!v.arr) {
        if (isNullValue(v)) throw Interp::RuntimeError{"null pointer passed where a string was expected"};
        throw Interp::RuntimeError{"internal error: a non-pointer was used as a string"};
    }
    std::string s;
    for (long long i = v.off;; ++i) {
        if (i < 0 || static_cast<size_t>(i) >= v.arr->size())
            throw Interp::RuntimeError{"string is not NUL-terminated (read past the end of its array)"};
        long long ch = asInt((*v.arr)[static_cast<size_t>(i)]);
        if (ch == 0) break;
        s += static_cast<char>(ch);
    }
    return s;
}

Value* slotAt(const Value& p, long long delta, const char* what) {
    if (!p.arr) {
        if (isNullValue(p)) throw Interp::RuntimeError{"null pointer dereference"};
        throw Interp::RuntimeError{std::string("internal error: ") + what + " needs a pointer"};
    }
    long long at = p.off + delta;
    if (at < 0 || static_cast<size_t>(at) >= p.arr->size())
        throw Interp::RuntimeError{"array index out of bounds"};
    return &(*p.arr)[static_cast<size_t>(at)];
}

// Writes s and a terminating NUL starting at p.
void writeString(const Value& p, const std::string& s, long long limit = -1) {
    long long n = static_cast<long long>(s.size());
    if (limit >= 0 && n + 1 > limit) n = limit > 0 ? limit - 1 : 0;
    // Stored as C's signed char, as the program would see the bytes.
    for (long long i = 0; i < n; ++i) *slotAt(p, i, "string write") = Value::makeInt(static_cast<signed char>(s[static_cast<size_t>(i)]));
    if (limit != 0) *slotAt(p, n, "string write") = Value::makeInt(0);
}

Value pointerPlus(const Value& p, long long delta) { Value r = p; r.off += delta; return r; }

std::string padTo(std::string s, int width, bool left) {
    if (width > 0 && static_cast<int>(s.size()) < width)
        s = left ? s + std::string(static_cast<size_t>(width) - s.size(), ' ')
                 : std::string(static_cast<size_t>(width) - s.size(), ' ') + s;
    return s;
}

// A real printf: flags, width, precision, length modifiers and the common
// conversions. Numeric conversions are delegated to snprintf so rounding and
// layout match the C library exactly.
std::string formatPrintf(const std::string& fmt, const std::vector<Value>& args, size_t argi) {
    std::string out;
    auto nextArg = [&](bool& ok) -> const Value& {
        static const Value none;
        if (argi >= args.size()) { ok = false; return none; }
        ok = true;
        return args[argi++];
    };
    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] != '%') { out += fmt[i]; continue; }
        size_t start = i++;
        if (i >= fmt.size()) { out += '%'; break; }
        if (fmt[i] == '%') { out += '%'; continue; }

        std::string flags;
        while (i < fmt.size() && std::strchr("-+ #0", fmt[i])) flags += fmt[i++];
        int width = -1, prec = -1;
        bool bad = false;
        if (i < fmt.size() && fmt[i] == '*') {
            bool ok; width = static_cast<int>(asInt(nextArg(ok))); bad |= !ok; ++i;
            if (width < 0) { flags += '-'; width = -width; }
        } else if (i < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[i]))) {
            width = 0;
            while (i < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[i]))) width = width * 10 + (fmt[i++] - '0');
        }
        if (i < fmt.size() && fmt[i] == '.') {
            ++i; prec = 0;
            if (i < fmt.size() && fmt[i] == '*') { bool ok; prec = static_cast<int>(asInt(nextArg(ok))); bad |= !ok; ++i; }
            else while (i < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[i]))) prec = prec * 10 + (fmt[i++] - '0');
        }
        int lcount = 0;
        while (i < fmt.size() && std::strchr("hlLqjzt", fmt[i])) { if (fmt[i] == 'l' || fmt[i] == 'q' || fmt[i] == 'j' || fmt[i] == 'z' || fmt[i] == 't') ++lcount; ++i; }
        if (i >= fmt.size() || bad) { out += fmt.substr(start); break; }
        char conv = fmt[i];
        bool ok = true;
        bool left = flags.find('-') != std::string::npos;

        std::string spec = "%" + flags + (width >= 0 ? std::to_string(width) : "") + (prec >= 0 ? "." + std::to_string(prec) : "");
        char buf[512];
        switch (conv) {
            case 'd': case 'i': {
                const Value& a = nextArg(ok);
                if (!ok) { out += fmt.substr(start, i - start + 1); break; }
                snprintf(buf, sizeof buf, (spec + "lld").c_str(), asInt(a));
                out += buf; break;
            }
            case 'u': case 'x': case 'X': case 'o': {
                const Value& a = nextArg(ok);
                if (!ok) { out += fmt.substr(start, i - start + 1); break; }
                long long v = asInt(a);
                // Without an l modifier the value is a 32-bit int in C.
                unsigned long long u = (lcount == 0 && v < 0 && v >= -2147483648LL)
                    ? static_cast<unsigned long long>(v) & 0xffffffffULL : static_cast<unsigned long long>(v);
                snprintf(buf, sizeof buf, (spec + "ll" + conv).c_str(), u);
                out += buf; break;
            }
            case 'c': {
                const Value& a = nextArg(ok);
                if (!ok) { out += fmt.substr(start, i - start + 1); break; }
                out += padTo(std::string(1, static_cast<char>(asInt(a))), width, left); break;
            }
            case 's': {
                const Value& a = nextArg(ok);
                if (!ok) { out += fmt.substr(start, i - start + 1); break; }
                std::string s = isNullValue(a) ? "(null)" : cstr(a);
                if (prec >= 0 && static_cast<int>(s.size()) > prec) s.resize(static_cast<size_t>(prec));
                out += padTo(s, width, left); break;
            }
            case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A': {
                const Value& a = nextArg(ok);
                if (!ok) { out += fmt.substr(start, i - start + 1); break; }
                snprintf(buf, sizeof buf, (spec + conv).c_str(), a.asDouble());
                out += buf; break;
            }
            case 'p': {
                const Value& a = nextArg(ok);
                if (!ok) { out += fmt.substr(start, i - start + 1); break; }
                if (isNullValue(a)) out += padTo("(nil)", width, left);
                else { snprintf(buf, sizeof buf, "0x%llx", 0x1000ULL + static_cast<unsigned long long>(a.off) * 4ULL); out += padTo(buf, width, left); }
                break;
            }
            default:
                out += fmt.substr(start, i - start + 1);
        }
    }
    return out;
}

// scanf and sscanf over any input stream. Returns the number of values
// stored, or -1 (EOF) if the input ended before the first conversion.
int doScan(std::istream& is, const std::string& fmt, const std::vector<Value>& args, size_t argi) {
    int assigned = 0;
    auto skipWs = [&] { while (is.peek() != EOF && std::isspace(is.peek())) is.get(); };
    for (size_t i = 0; i < fmt.size(); ++i) {
        char f = fmt[i];
        if (std::isspace(static_cast<unsigned char>(f))) { skipWs(); continue; }
        if (f != '%') {
            int ch = is.peek();
            if (ch == EOF) return assigned ? assigned : -1;
            if (ch != static_cast<unsigned char>(f)) return assigned;
            is.get(); continue;
        }
        if (++i >= fmt.size()) break;
        if (fmt[i] == '%') { skipWs(); if (is.peek() != '%') return assigned; is.get(); continue; }
        bool suppress = false;
        if (fmt[i] == '*') { suppress = true; ++i; }
        int width = 0;
        while (i < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[i]))) width = width * 10 + (fmt[i++] - '0');
        while (i < fmt.size() && std::strchr("hlLqjzt", fmt[i])) ++i;
        if (i >= fmt.size()) break;
        char conv = fmt[i];

        auto target = [&]() -> Value {
            if (argi >= args.size()) throw Interp::RuntimeError{"scanf has more conversions than arguments"};
            return args[argi++];
        };
        auto putScalar = [&](Value v) {
            if (suppress) return;
            Value p = target();
            if (!p.arr) throw Interp::RuntimeError{"scanf needs the address of each variable: write &name"};
            *slotAt(p, 0, "scanf") = v;
            ++assigned;
        };
        switch (conv) {
            case 'd': case 'i': case 'u': case 'x': case 'X': case 'o': {
                skipWs();
                if (is.peek() == EOF) return assigned ? assigned : -1;
                long long v = 0;
                if (conv == 'x' || conv == 'X') is >> std::hex >> v >> std::dec;
                else if (conv == 'o')           is >> std::oct >> v >> std::dec;
                else                            is >> v;
                if (is.fail()) { is.clear(); return assigned; }
                putScalar(Value::makeInt(v));
                break;
            }
            case 'f': case 'e': case 'E': case 'g': case 'G': {
                skipWs();
                if (is.peek() == EOF) return assigned ? assigned : -1;
                double v = 0;
                is >> v;
                if (is.fail()) { is.clear(); return assigned; }
                putScalar(Value::makeFloat(v));
                break;
            }
            case 'c': {
                int ch = is.get();
                if (ch == EOF) { is.clear(); return assigned ? assigned : -1; }
                putScalar(Value::makeInt(static_cast<signed char>(ch)));   // into a char
                break;
            }
            case 's': {
                skipWs();
                if (is.peek() == EOF) return assigned ? assigned : -1;
                std::string word;
                while (is.peek() != EOF && !std::isspace(is.peek()) && (width == 0 || static_cast<int>(word.size()) < width))
                    word += static_cast<char>(is.get());
                if (!suppress) { Value p = target(); writeString(p, word); ++assigned; }
                break;
            }
            default:
                throw Interp::RuntimeError{std::string("scanf conversion '%") + conv + "' is not supported"};
        }
    }
    return assigned;
}

} // namespace

Value Interp::callBuiltin(const std::string& name, std::vector<Value>& args) {
    auto arg = [&](size_t i) -> const Value& {
        if (i >= args.size()) throw RuntimeError{"internal error: too few arguments to " + name};
        return args[i];
    };
    auto I = [&](long long v) { return Value::makeInt(v); };
    auto F = [&](double v) { return Value::makeFloat(v); };

    // ---- formatted output / input --------------------------------------
    if (name == "printf") {
        std::string s = formatPrintf(cstr(arg(0)), args, 1);
        std::cout << s;
        return I(static_cast<long long>(s.size()));
    }
    if (name == "fprintf") {
        std::string s = formatPrintf(cstr(arg(1)), args, 2);
        (asInt(arg(0)) == 2 ? std::cerr : std::cout) << s;
        return I(static_cast<long long>(s.size()));
    }
    if (name == "sprintf" || name == "snprintf") {
        bool sized = name == "snprintf";
        std::string s = formatPrintf(cstr(arg(sized ? 2 : 1)), args, sized ? 3 : 2);
        writeString(arg(0), s, sized ? asInt(arg(1)) : -1);
        return I(static_cast<long long>(s.size()));
    }
    if (name == "scanf")  return I(doScan(std::cin, cstr(arg(0)), args, 1));
    if (name == "fscanf") return I(doScan(std::cin, cstr(arg(1)), args, 2));
    if (name == "sscanf") { std::istringstream is(cstr(arg(0))); return I(doScan(is, cstr(arg(1)), args, 2)); }

    // ---- the original built-ins ---------------------------------------------
    if (name == "print_int")   { std::cout << asInt(args.empty() ? I(0) : args[0]) << "\n"; return I(0); }
    if (name == "print_float") { std::cout << (args.empty() ? 0.0 : args[0].asDouble()) << "\n"; return I(0); }
    if (name == "print_char")  { std::cout << static_cast<char>(asInt(args.empty() ? I(0) : args[0])); return I(0); }
    if (name == "read_int") {
        long long v = 0;
        // cin's failbit is sticky; surface it as an error so a program cannot
        // silently keep reading zeros after the input ends.
        if (!(std::cin >> v)) throw RuntimeError{"read_int: end of input or malformed integer"};
        return I(v);
    }

    // ---- characters and lines ---------------------------------------------
    if (name == "putchar") { std::cout << static_cast<char>(asInt(arg(0))); return I(asInt(arg(0))); }
    if (name == "putc" || name == "fputc") { (asInt(arg(1)) == 2 ? std::cerr : std::cout) << static_cast<char>(asInt(arg(0))); return I(asInt(arg(0))); }
    if (name == "getchar") { int ch = std::cin.get(); if (ch == EOF) { std::cin.clear(); return I(-1); } return I(ch); }
    if (name == "puts")    { std::cout << cstr(arg(0)) << "\n"; return I(1); }
    if (name == "fputs")   { (asInt(arg(1)) == 2 ? std::cerr : std::cout) << cstr(arg(0)); return I(1); }
    if (name == "fflush")  { std::cout.flush(); return I(0); }
    if (name == "gets") {
        std::string line;
        if (!std::getline(std::cin, line)) { std::cin.clear(); return I(0); }
        writeString(arg(0), line);
        return arg(0);
    }
    if (name == "fgets") {
        long long n = asInt(arg(1));
        if (n <= 0) return I(0);
        std::string line;
        int ch;
        while (static_cast<long long>(line.size()) < n - 1 && (ch = std::cin.get()) != EOF) {
            line += static_cast<char>(ch);
            if (ch == '\n') break;
        }
        if (line.empty()) { std::cin.clear(); return I(0); }
        writeString(arg(0), line, n);
        return arg(0);
    }

    // ---- strings --------------------------------------------------------------
    if (name == "strlen") return I(static_cast<long long>(cstr(arg(0)).size()));
    if (name == "strcpy") { writeString(arg(0), cstr(arg(1))); return arg(0); }
    if (name == "strncpy") {
        std::string s = cstr(arg(1));
        long long n = asInt(arg(2));
        for (long long i = 0; i < n; ++i)
            *slotAt(arg(0), i, "strncpy") = I(i < static_cast<long long>(s.size()) ? static_cast<signed char>(s[static_cast<size_t>(i)]) : 0);
        return arg(0);
    }
    if (name == "strcat") { std::string d = cstr(arg(0)); writeString(pointerPlus(arg(0), static_cast<long long>(d.size())), cstr(arg(1))); return arg(0); }
    if (name == "strncat") {
        std::string d = cstr(arg(0)), s = cstr(arg(1));
        long long n = std::min<long long>(asInt(arg(2)), static_cast<long long>(s.size()));
        writeString(pointerPlus(arg(0), static_cast<long long>(d.size())), s.substr(0, static_cast<size_t>(n)));
        return arg(0);
    }
    if (name == "strcmp" || name == "strncmp") {
        std::string a = cstr(arg(0)), b = cstr(arg(1));
        if (name == "strncmp") { size_t n = static_cast<size_t>(std::max<long long>(0, asInt(arg(2)))); a = a.substr(0, n); b = b.substr(0, n); }
        size_t k = 0;
        while (k < a.size() && k < b.size() && a[k] == b[k]) ++k;
        int ca = k < a.size() ? static_cast<unsigned char>(a[k]) : 0;
        int cb = k < b.size() ? static_cast<unsigned char>(b[k]) : 0;
        return I(ca - cb);
    }
    if (name == "strchr" || name == "strrchr") {
        std::string s = cstr(arg(0));
        char want = static_cast<char>(asInt(arg(1)));
        size_t at = name == "strchr" ? s.find(want) : s.rfind(want);
        if (want == '\0') at = s.size();
        return at == std::string::npos ? I(0) : pointerPlus(arg(0), static_cast<long long>(at));
    }
    if (name == "strstr") {
        size_t at = cstr(arg(0)).find(cstr(arg(1)));
        return at == std::string::npos ? I(0) : pointerPlus(arg(0), static_cast<long long>(at));
    }
    if (name == "strcspn" || name == "strspn") {
        std::string s = cstr(arg(0)), set = cstr(arg(1));
        size_t k = 0;
        while (k < s.size() && ((set.find(s[k]) == std::string::npos) == (name == "strcspn"))) ++k;
        return I(static_cast<long long>(k));
    }
    if (name == "strdup") {
        std::string s = cstr(arg(0));
        auto vec = std::make_shared<std::vector<Value>>(s.size() + 1, I(0));
        for (size_t i = 0; i < s.size(); ++i) (*vec)[i] = I(static_cast<signed char>(s[i]));
        return Value::makeArray(vec);
    }
    if (name == "atoi" || name == "atol") return I(std::strtoll(cstr(arg(0)).c_str(), nullptr, 10));
    if (name == "atof") return F(std::strtod(cstr(arg(0)).c_str(), nullptr));

    // ---- memory ---------------------------------------------------------------
    if (name == "malloc" || name == "calloc") {
        long long n = name == "calloc" ? asInt(arg(0)) * asInt(arg(1)) : asInt(arg(0));
        if (n < 0 || n > 50000000) return I(0);                       // allocation failure -> NULL
        return Value::makeArray(std::make_shared<std::vector<Value>>(static_cast<size_t>(std::max<long long>(n, 1)), I(0)));
    }
    if (name == "realloc") {
        long long n = asInt(arg(1));
        if (n < 0 || n > 50000000) return I(0);
        auto fresh = std::make_shared<std::vector<Value>>(static_cast<size_t>(std::max<long long>(n, 1)), I(0));
        const Value& old = arg(0);
        if (old.arr) {
            size_t have = old.arr->size() > static_cast<size_t>(old.off) ? old.arr->size() - static_cast<size_t>(old.off) : 0;
            for (size_t i = 0; i < std::min(have, fresh->size()); ++i) (*fresh)[i] = (*old.arr)[static_cast<size_t>(old.off) + i];
        }
        return Value::makeArray(fresh);
    }
    if (name == "free") return I(0);            // storage is reclaimed automatically
    if (name == "memset") {
        long long n = asInt(arg(2));
        for (long long i = 0; i < n; ++i) *slotAt(arg(0), i, "memset") = I(asInt(arg(1)));
        return arg(0);
    }
    if (name == "memcpy" || name == "memmove") {
        long long n = asInt(arg(2));
        std::vector<Value> tmp;
        for (long long i = 0; i < n; ++i) tmp.push_back(*slotAt(arg(1), i, name.c_str()));
        for (long long i = 0; i < n; ++i) *slotAt(arg(0), i, name.c_str()) = tmp[static_cast<size_t>(i)];
        return arg(0);
    }
    if (name == "memcmp") {
        long long n = asInt(arg(2));
        for (long long i = 0; i < n; ++i) {
            long long a = asInt(*slotAt(arg(0), i, "memcmp")), b = asInt(*slotAt(arg(1), i, "memcmp"));
            if (a != b) return I(a < b ? -1 : 1);
        }
        return I(0);
    }

    // ---- math and misc --------------------------------------------------------
    if (name == "abs" || name == "labs") return I(std::llabs(asInt(arg(0))));
    if (name == "fabs")  return F(std::fabs(arg(0).asDouble()));
    if (name == "sqrt")  return F(std::sqrt(arg(0).asDouble()));
    if (name == "cbrt")  return F(std::cbrt(arg(0).asDouble()));
    if (name == "pow")   return F(std::pow(arg(0).asDouble(), arg(1).asDouble()));
    if (name == "sin")   return F(std::sin(arg(0).asDouble()));
    if (name == "cos")   return F(std::cos(arg(0).asDouble()));
    if (name == "tan")   return F(std::tan(arg(0).asDouble()));
    if (name == "asin")  return F(std::asin(arg(0).asDouble()));
    if (name == "acos")  return F(std::acos(arg(0).asDouble()));
    if (name == "atan")  return F(std::atan(arg(0).asDouble()));
    if (name == "atan2") return F(std::atan2(arg(0).asDouble(), arg(1).asDouble()));
    if (name == "sinh")  return F(std::sinh(arg(0).asDouble()));
    if (name == "cosh")  return F(std::cosh(arg(0).asDouble()));
    if (name == "tanh")  return F(std::tanh(arg(0).asDouble()));
    if (name == "exp")   return F(std::exp(arg(0).asDouble()));
    if (name == "log")   return F(std::log(arg(0).asDouble()));
    if (name == "log10") return F(std::log10(arg(0).asDouble()));
    if (name == "log2")  return F(std::log2(arg(0).asDouble()));
    if (name == "floor") return F(std::floor(arg(0).asDouble()));
    if (name == "ceil")  return F(std::ceil(arg(0).asDouble()));
    if (name == "round") return F(std::round(arg(0).asDouble()));
    if (name == "trunc") return F(std::trunc(arg(0).asDouble()));
    if (name == "fmod")  return F(std::fmod(arg(0).asDouble(), arg(1).asDouble()));
    if (name == "fmin")  return F(std::fmin(arg(0).asDouble(), arg(1).asDouble()));
    if (name == "fmax")  return F(std::fmax(arg(0).asDouble(), arg(1).asDouble()));
    if (name == "hypot") return F(std::hypot(arg(0).asDouble(), arg(1).asDouble()));
    if (name == "rand") {
        // A fixed generator: the same program prints the same numbers every
        // run, which the optimized-vs-unoptimized comparison depends on.
        randState_ = randState_ * 1103515245ULL + 12345ULL;
        return I(static_cast<long long>((randState_ >> 16) & 0x7fff));
    }
    if (name == "srand") { randState_ = static_cast<unsigned long long>(asInt(arg(0))); return I(0); }
    if (name == "time" || name == "clock") return I(0);   // a deterministic clock
    if (name == "exit") throw Interp::ProgramExit{asInt(arg(0))};
    if (name == "assert") { if (!arg(0).truthy()) throw RuntimeError{"assertion failed"}; return I(0); }

    auto ctype = [&](int (*fn)(int)) { int ch = static_cast<int>(asInt(arg(0))); return I(ch >= 0 && ch < 256 && fn(ch) ? 1 : 0); };
    if (name == "isdigit")  return ctype(std::isdigit);
    if (name == "isalpha")  return ctype(std::isalpha);
    if (name == "isalnum")  return ctype(std::isalnum);
    if (name == "isspace")  return ctype(std::isspace);
    if (name == "isupper")  return ctype(std::isupper);
    if (name == "islower")  return ctype(std::islower);
    if (name == "ispunct")  return ctype(std::ispunct);
    if (name == "isxdigit") return ctype(std::isxdigit);
    if (name == "toupper") { int ch = static_cast<int>(asInt(arg(0))); return I(ch >= 0 && ch < 256 ? std::toupper(ch) : ch); }
    if (name == "tolower") { int ch = static_cast<int>(asInt(arg(0))); return I(ch >= 0 && ch < 256 ? std::tolower(ch) : ch); }

    throw RuntimeError{"call to undefined function '" + name + "'"};
}

// The slot a LOAD_INDEX/STORE_INDEX touches: the pointer's own offset plus
// the index, bounds-checked against the object the pointer points into.
static long long checkedSlot(const Value& base, const Value& idx) {
    if (!base.arr) {
        if (base.kind == Value::Kind::INT && base.ival == 0)
            throw Interp::RuntimeError{"null pointer dereference"};
        throw Interp::RuntimeError{"internal error: indexing a non-pointer value"};
    }
    long long at = base.off + idx.ival;
    if (at < 0 || static_cast<size_t>(at) >= base.arr->size())
        throw Interp::RuntimeError{"array index out of bounds"};
    return at;
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

            case Op::GOTO: {
                // labelMap.at() throws std::out_of_range on a miss, not
                // Interp::RuntimeError — a caller that only catches
                // RuntimeError (as this class's documented error surface
                // implies it should) would see that escape uncaught. A
                // dangling jump target can only come from an optimizer-
                // pass bug (a dropped/renamed LABEL with a stale jump
                // still pointing at the old name), so make it a clean,
                // catchable RuntimeError rather than a crash.
                auto it = labelMap.find(q.result.name);
                if (it == labelMap.end())
                    throw RuntimeError{"internal error: jump to undefined label '" + q.result.name + "'"};
                pc = it->second;
                continue;
            }

            case Op::IF_FALSE: {
                Value v = evalOperand(q.arg1, frame);
                if (!v.truthy()) {
                    auto it = labelMap.find(q.result.name);
                    if (it == labelMap.end())
                        throw RuntimeError{"internal error: jump to undefined label '" + q.result.name + "'"};
                    pc = it->second; continue;
                }
                break;
            }

            case Op::COPY:
                store(q.result, evalOperand(q.arg1, frame), frame);
                break;

            case Op::NEG: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result,
                      v.kind == Value::Kind::FLOAT ? Value::makeFloat(-v.fval)
                          : Value::makeInt(wrapInt(static_cast<long long>(0 - static_cast<unsigned long long>(v.ival)), q.ty)),
                      frame);
                break;
            }
            case Op::NOT: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result, Value::makeInt(v.truthy() ? 0 : 1), frame);
                break;
            }
            case Op::BITNOT: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result, Value::makeInt(wrapInt(~v.ival, q.ty)), frame);
                break;
            }
            case Op::TO_INT: {
                Value v = evalOperand(q.arg1, frame);
                store(q.result,
                      Value::makeInt(wrapInt(v.kind == Value::Kind::FLOAT ? static_cast<long long>(v.fval) : v.ival, q.ty)),
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
                long long at = checkedSlot(base, idx);
                store(q.result, (*base.arr)[static_cast<size_t>(at)], frame);
                break;
            }
            case Op::STORE_INDEX: {
                // result[arg1] = arg2 — result names the array (see ir.h).
                Value base = evalOperand(q.result, frame);
                Value idx  = evalOperand(q.arg1, frame);
                Value val  = evalOperand(q.arg2, frame);
                long long at = checkedSlot(base, idx);
                (*base.arr)[static_cast<size_t>(at)] = val;
                break;
            }

            case Op::COPY_BLOCK: {
                Value src = evalOperand(q.arg1, frame);
                Value dst = evalOperand(q.result, frame);
                long long n = q.arg2.ival;
                if (!dst.arr || !src.arr) {
                    bool nullish = (!dst.arr && dst.kind == Value::Kind::INT && dst.ival == 0)
                                || (!src.arr && src.kind == Value::Kind::INT && src.ival == 0);
                    throw RuntimeError{nullish ? "null pointer dereference"
                                               : "internal error: copying a non-pointer value"};
                }
                if (n > 0) {
                    // Bounds are checked at both ends before anything moves,
                    // so a bad copy never half-completes.
                    if (src.off < 0 || static_cast<size_t>(src.off + n) > src.arr->size() ||
                        dst.off < 0 || static_cast<size_t>(dst.off + n) > dst.arr->size())
                        throw RuntimeError{"array index out of bounds"};
                    // Through a temporary, so overlapping ranges (a struct
                    // assigned onto part of itself) behave like memmove.
                    std::vector<Value> tmp(src.arr->begin() + src.off, src.arr->begin() + src.off + n);
                    std::copy(tmp.begin(), tmp.end(), dst.arr->begin() + dst.off);
                }
                break;
            }

            case Op::PARAM:
                pendingParams_.push_back(evalOperand(q.arg1, frame));
                break;

            case Op::CALL: {
                const std::string& callee = q.arg1.name;
                long long argc = q.arg2.ival;
                // A mismatched PARAM/CALL count (only reachable through an
                // optimizer-pass bug — e.g. a PARAM quad deleted without
                // updating the CALL's stored argc) used to walk an
                // iterator before pendingParams_.begin(), which is
                // undefined behaviour, instead of the diagnosable error
                // this interpreter is supposed to be able to give for
                // exactly this class of pass bug.
                if (argc < 0 || static_cast<size_t>(argc) > pendingParams_.size())
                    throw RuntimeError{"internal error: call to '" + callee + "' has " +
                        std::to_string(argc) + " pending argument(s), only " +
                        std::to_string(pendingParams_.size()) + " available"};
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
                store(q.result, applyBinary(q.op, evalOperand(q.arg1, frame), evalOperand(q.arg2, frame), q.ty), frame);
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
    randState_ = 1;
    funcsByName_.clear();
    for (const auto& fn : program.functions) funcsByName_[fn.name] = &fn;
    globals_.clear();
    pendingParams_.clear();

    try {
        std::unordered_map<std::string, Value> initFrame;   // only temps ever land here; see file header
        execCode(program.globalInit, initFrame);

        auto it = funcsByName_.find(entry);
        if (it == funcsByName_.end()) throw RuntimeError{"no function named '" + entry + "'"};
        ExecResult r = execFunction(*it->second, {});
        return r.returned ? r.value : Value::makeInt(0);
    } catch (const ProgramExit& e) {
        return Value::makeInt(e.code);       // exit(n) ends the program with status n
    }
}

} // namespace minic
