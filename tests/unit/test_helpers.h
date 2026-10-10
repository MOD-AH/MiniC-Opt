// MiniC-Opt — shared fixtures for the unit tests.
//
// Most tests need a compiler stage driven from a string of source rather
// than a file, so these wrap the front end down to whichever stage the test
// cares about. Each helper stops where it is asked to, so a parser test
// never silently depends on semantic analysis succeeding.
#pragma once
#include "cfg.h"
#include "dataflow_analyses.h"
#include "interp.h"
#include "irgen.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "test_framework.h"

#include <memory>
#include <string>
#include <vector>

namespace th {

inline std::vector<minic::Token> lexOk(const std::string& src) {
    minic::Lexer lex(src, "<test>");
    return lex.tokenize();
}

inline std::vector<minic::Diagnostic> lexErrors(const std::string& src) {
    minic::Lexer lex(src, "<test>");
    lex.tokenize();
    return lex.errors();
}

// Tokens with END_OF_FILE stripped, so a test can index positionally
// without accounting for the terminator.
inline std::vector<minic::Token> lexBody(const std::string& src) {
    std::vector<minic::Token> ts = lexOk(src);
    if (!ts.empty() && ts.back().type == minic::Tok::END_OF_FILE) ts.pop_back();
    return ts;
}

struct ParseResult {
    std::unique_ptr<minic::Program> program;
    std::vector<minic::Diagnostic>  errors;
};

inline ParseResult parse(const std::string& src) {
    minic::Lexer lex(src, "<test>");
    std::vector<minic::Token> toks = lex.tokenize();
    minic::Parser p(std::move(toks), "<test>");
    ParseResult r;
    r.program = p.parseProgram();
    r.errors = p.errors();
    return r;
}

// Runs the front end through semantic analysis. `ok` reports whether sema
// produced zero diagnostics; parser errors are surfaced in `errors` too, so
// a test can tell the two apart by checking `ok` against `errors.empty()`.
struct CheckResult {
    bool ok = false;
    std::vector<minic::Diagnostic> errors;
};

inline CheckResult check(const std::string& src) {
    minic::Lexer lex(src, "<test>");
    std::vector<minic::Token> toks = lex.tokenize();
    minic::Parser p(std::move(toks), "<test>");
    std::unique_ptr<minic::Program> prog = p.parseProgram();
    CheckResult r;
    if (!p.errors().empty()) {
        r.ok = false;
        r.errors = p.errors();
        return r;
    }
    minic::Sema sema;
    r.ok = sema.analyze(*prog);
    r.errors = sema.errors();
    return r;
}

// True iff some diagnostic's message contains `needle`.
inline bool hasError(const std::vector<minic::Diagnostic>& errs, const std::string& needle) {
    for (const minic::Diagnostic& d : errs)
        if (d.message.find(needle) != std::string::npos) return true;
    return false;
}

// Source must be sema-clean; the IR for the whole program is returned.
inline minic::IRProgram irOf(const std::string& src) {
    minic::Lexer lex(src, "<test>");
    std::vector<minic::Token> toks = lex.tokenize();
    minic::Parser p(std::move(toks), "<test>");
    std::unique_ptr<minic::Program> prog = p.parseProgram();
    minic::Sema sema;
    sema.analyze(*prog);
    minic::IRGen gen;
    return gen.generate(*prog);
}

// The named function's IR, or an empty IRFunction if there is no such name.
inline minic::IRFunction funcOf(const std::string& src, const std::string& name = "main") {
    minic::IRProgram prog = irOf(src);
    for (const minic::IRFunction& f : prog.functions)
        if (f.name == name) return f;
    return minic::IRFunction{};
}

// Counts quads with the given opcode across one function.
inline int countOp(const minic::IRFunction& fn, minic::Op op) {
    int n = 0;
    for (const minic::Quad& q : fn.code)
        if (q.op == op) ++n;
    return n;
}

inline int countOp(const minic::CFG& cfg, minic::Op op) {
    int n = 0;
    for (const minic::BasicBlock& b : cfg.blocks())
        for (const minic::Quad& q : b.code)
            if (q.op == op) ++n;
    return n;
}

inline long long runToValue(const std::string& src) {
    minic::IRProgram prog = irOf(src);
    minic::Interp interp;
    return interp.run(prog).ival;
}

}  // namespace th
