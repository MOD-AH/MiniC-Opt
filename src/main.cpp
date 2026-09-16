// MiniC-Opt — driver
//
// Review 2 status: the lexer and parser are implemented and demonstrable
// (--dump-tokens, --dump-ast). Later phases (semantic analysis, IR, CFG,
// optimizer, interpreter) are stubbed and report their planned milestone.
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "sema.h"
#include "irgen.h"
#include "interp.h"
#include "cfg.h"
#include "dataflow_analyses.h"
#include "opt_passes.h"
#include "ir_utils.h"
#include "backend.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>

using namespace minic;

static void usage() {
    std::cout <<
"MiniC-Opt — an optimizing compiler for a subset of C\n"
"\n"
"usage: minic [options] <file.c>\n"
"\n"
"  --dump-tokens     print the token stream                   [Review 1  DONE]\n"
"  --dump-ast        print the abstract syntax tree           [Review 2  DONE]\n"
"  --check           run semantic analysis, report diagnostics[Review 2  DONE]\n"
"  --dump-ir         print unoptimized three-address code     [Review 2  DONE]\n"
"  --run             lex+parse+check+generate IR, then interpret it,\n"
"                    printing main()'s return value (matches tests/programs/*.expected)\n"
"                                                                [Review 2  DONE]\n"
"  --dump-cfg        emit the control-flow graph as Graphviz  [Review 2  DONE]\n"
"  --dump-dataflow=<reach|avail|live>\n"
"                    print per-block IN/OUT sets for one data-flow\n"
"                    analysis, spot-check / viva tool             [Review 2  DONE]\n"
"  --opt=<list>      enable named optimization passes: fold,constprop,copyprop,\n"
"                    cse,dce,unreachable,strength,licm,peephole\n"
"                    (may be combined with --dump-ir/--run/--dump-cfg/--metrics)\n"
"                                                                [Review 2  DONE]\n"
"  --no-opt          run the optimizer with every pass disabled (baseline);\n"
"                    combined with --dump-ir/--run/--dump-cfg the same way\n"
"                    --opt= is                                  [Review 2  DONE]\n"
"  --metrics         print the per-pass before/after static instruction\n"
"                    report plus the dynamic instruction count, run with\n"
"                    and without optimization (defaults to every pass\n"
"                    enabled unless combined with --opt=/--no-opt)\n"
"                                                                [Review 3  DONE]\n"
"  --emit-asm        print slot-addressed pseudo-assembly (linearisation +\n"
"                    stack slots) for the optimized IR — NOT covered by the\n"
"                    differential-execution gate; see docs/test_plan.md\n"
"                                                                [Review 3  DONE]\n"
"  --version         print version information\n"
"  --help            print this message\n";
}

static int dumpTokens(const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::cerr << "minic: cannot open '" << path << "'\n"; return 2; }
    std::stringstream ss; ss << in.rdbuf();

    Lexer lex(ss.str(), path);
    std::vector<Token> toks = lex.tokenize();

    std::cout << "\n"
              << std::setw(4)  << "#"
              << std::setw(6)  << "LINE"
              << std::setw(5)  << "COL"
              << "   " << std::left << std::setw(16) << "TOKEN TYPE"
              << "LEXEME" << std::right << "\n";

    int n = 0;
    for (const Token& t : toks) {
        std::cout << std::setw(4)  << ++n
                  << std::setw(6)  << t.line
                  << std::setw(5)  << t.col
                  << "   " << std::left << std::setw(16) << tokName(t.type)
                  << t.lexeme << std::right << "\n";
    }

    for (const Diagnostic& d : lex.errors())
        std::cerr << path << ":" << d.line << ":" << d.col
                  << ": error: " << d.message << "\n";

    std::cout << "\n" << toks.size() << " tokens, "
              << lex.errors().size() << " lexical error"
              << (lex.errors().size() == 1 ? "" : "s") << ".\n";

    return lex.errors().empty() ? 0 : 1;
}

static int dumpAst(const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::cerr << "minic: cannot open '" << path << "'\n"; return 2; }
    std::stringstream ss; ss << in.rdbuf();

    Lexer lex(ss.str(), path);
    std::vector<Token> toks = lex.tokenize();

    Parser parser(toks, path);
    std::unique_ptr<Program> prog = parser.parseProgram();

    AstPrinter printer(std::cout);
    printer.print(*prog);

    for (const Diagnostic& d : lex.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : parser.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";

    size_t total = lex.errors().size() + parser.errors().size();
    std::cerr << total << " error" << (total == 1 ? "" : "s") << ".\n";
    return total == 0 ? 0 : 1;
}

// Shared by --dump-ir and --run: lexes, parses, and semantically checks
// `path`, printing any diagnostics exactly as --check does. Returns nullptr
// on any error at any of the three stages — IR generation assumes a clean,
// fully-annotated tree (every Expr::resolvedType set, every implicit
// conversion already spliced in by Sema) and is not meant to run on a
// program that didn't get there.
static std::unique_ptr<Program> checkedParse(const std::string& path, bool& ok) {
    ok = false;
    std::ifstream in(path);
    if (!in) { std::cerr << "minic: cannot open '" << path << "'\n"; return nullptr; }
    std::stringstream ss; ss << in.rdbuf();

    Lexer lex(ss.str(), path);
    std::vector<Token> toks = lex.tokenize();

    Parser parser(toks, path);
    std::unique_ptr<Program> prog = parser.parseProgram();

    Sema sema;
    bool clean = sema.analyze(*prog);

    for (const Diagnostic& d : lex.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : parser.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : sema.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";

    size_t total = lex.errors().size() + parser.errors().size() + sema.errors().size();
    if (total != 0 || !clean) {
        std::cerr << total << " error" << (total == 1 ? "" : "s") << "; not generating IR.\n";
        return prog;
    }
    ok = true;
    return prog;
}

struct OptConfig {
    bool requested = false;      // true iff --opt=... or --no-opt was given at all
    bool noOpt = false;          // --no-opt: run the pipeline with zero passes enabled
    std::vector<std::string> only;   // --opt=name,name,...  (empty + requested-without-noOpt = all passes)
};

// Runs the Step 6 pipeline over every function's CFG and flattens each
// back to a linear IRFunction — see ir_utils.h's flatten() for why that
// round trip is lossless for P1-P3. globalInit is carried through
// unchanged: it has no control flow for a CFG-based pass to act on, and
// no committed benchmark declares a global to begin with.
static IRProgram applyOptimization(const IRProgram& ir, const OptConfig& opt,
                                    std::vector<PassManager::PassReport>* reportOut) {
    IRProgram out;
    out.globalInit = ir.globalInit;
    for (const IRFunction& fn : ir.functions) {
        CFG cfg(fn);
        if (!opt.noOpt) {
            PassManager pm;
            registerAllPasses(pm);
            if (!opt.only.empty()) pm.enableOnly(opt.only);
            pm.run(cfg);
            if (reportOut) {
                const auto& r = pm.report();
                reportOut->insert(reportOut->end(), r.begin(), r.end());
            }
        }
        IRFunction outFn;
        outFn.name = fn.name;
        outFn.params = fn.params;
        outFn.tempCounter = fn.tempCounter;
        outFn.labelCounter = fn.labelCounter;
        outFn.code = flatten(cfg);
        out.functions.push_back(std::move(outFn));
    }
    return out;
}

static void printIr(const IRProgram& ir, std::ostream& os) {
    if (!ir.globalInit.empty()) {
        os << "; -- global init --\n";
        for (const Quad& q : ir.globalInit) os << "    " << q.toString() << "\n";
    }
    for (const IRFunction& fn : ir.functions) {
        os << "\n" << fn.name << "(";
        for (size_t i = 0; i < fn.params.size(); ++i) {
            if (i) os << ", ";
            os << fn.params[i];
        }
        os << "):\n";
        for (const Quad& q : fn.code) {
            if (q.op == Op::LABEL) os << "  " << q.toString() << "\n";
            else                   os << "    " << q.toString() << "\n";
        }
    }
}

static int dumpIr(const std::string& path, const OptConfig& opt) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    if (opt.requested) {
        std::vector<PassManager::PassReport> report;
        ir = applyOptimization(ir, opt, &report);
        for (const auto& r : report)
            std::cerr << "[opt] " << r.pass << ": " << r.transforms << " transform"
                       << (r.transforms == 1 ? "" : "s") << "  (instrs " << r.instrBefore
                       << " -> " << r.instrAfter << ")\n";
    }
    printIr(ir, std::cout);
    return 0;
}

static int runProgram(const std::string& path, const OptConfig& opt) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    if (opt.requested) ir = applyOptimization(ir, opt, nullptr);
    Interp interp;
    try {
        Value result = interp.run(ir, "main");
        if (result.kind == Value::Kind::FLOAT) std::cout << result.fval << "\n";
        else                                    std::cout << result.ival << "\n";
    } catch (const Interp::RuntimeError& e) {
        std::cerr << "minic: runtime error: " << e.message << "\n";
        return 1;
    }
    return 0;
}

static int dumpCfg(const std::string& path, const OptConfig& opt) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    if (opt.requested) ir = applyOptimization(ir, opt, nullptr);
    for (const IRFunction& fn : ir.functions) {
        CFG cfg(fn);
        std::cout << cfg.toDot();
    }
    return 0;
}

static int dumpDataflow(const std::string& path, const std::string& kind) {
    if (kind != "reach" && kind != "avail" && kind != "live") {
        std::cerr << "minic: --dump-dataflow expects reach, avail, or live (got '" << kind << "')\n";
        return 2;
    }
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    for (const IRFunction& fn : ir.functions) {
        CFG cfg(fn);
        DataFlowResult r = (kind == "reach") ? computeReachingDefs(cfg, fn)
                          : (kind == "avail") ? computeAvailableExpressions(cfg, fn)
                          :                     computeLiveVariables(cfg, fn);
        std::cout << "\n" << fn.name << ":\n";
        for (size_t bi = 0; bi < cfg.blocks().size(); ++bi) {
            std::cout << "  B" << bi << ":\n    IN:  {";
            bool first = true;
            for (const auto& s : r.in[bi])  { if (!first) std::cout << ", "; std::cout << s; first = false; }
            std::cout << "}\n    OUT: {";
            first = true;
            for (const auto& s : r.out[bi]) { if (!first) std::cout << ", "; std::cout << s; first = false; }
            std::cout << "}\n";
        }
    }
    return 0;
}

static int dumpAsm(const std::string& path, const OptConfig& opt) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    if (opt.requested) ir = applyOptimization(ir, opt, nullptr);
    std::cout << emitProgram(ir);
    return 0;
}

// Runs the same program once unoptimized and once through whatever
// optimization config the command line asked for (defaulting to every
// pass enabled if --metrics was given with neither --opt= nor --no-opt),
// and reports both the static instruction counts (per pass, and overall)
// and the dynamic instruction counts the reference interpreter actually
// executes for each — the same two numbers docs/worked_example.md hand-
// computes for its own worked case (15 -> 10 static, 125 -> 84 dynamic).
static int printMetrics(const std::string& path, const OptConfig& optIn) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram baseline = gen.generate(*prog);

    OptConfig opt = optIn;
    if (!opt.requested) opt.requested = true;   // --metrics alone means "show me the optimizer's effect"

    std::vector<PassManager::PassReport> report;
    IRProgram optimized = opt.noOpt ? baseline : applyOptimization(baseline, opt, &report);

    auto staticCount = [](const IRProgram& p) {
        long long n = static_cast<long long>(p.globalInit.size());
        for (const auto& fn : p.functions) n += static_cast<long long>(fn.code.size());
        return n;
    };
    long long beforeStatic = staticCount(baseline);
    long long afterStatic  = staticCount(optimized);

    std::unordered_map<std::string, int> totalsByPass;
    std::vector<std::string> passOrder;
    for (const auto& r : report) {
        if (totalsByPass.find(r.pass) == totalsByPass.end()) passOrder.push_back(r.pass);
        totalsByPass[r.pass] += r.transforms;
    }

    std::cout << "per-pass transform counts (summed over every fixed-point sweep):\n";
    if (passOrder.empty()) {
        std::cout << "  (no optimization ran)\n";
    } else {
        for (const auto& name : passOrder)
            std::cout << "  " << name << ": " << totalsByPass[name] << "\n";
    }

    std::cout << "\nstatic instructions:  " << beforeStatic << " -> " << afterStatic;
    if (beforeStatic > 0)
        std::cout << "  (" << (100.0 * static_cast<double>(beforeStatic - afterStatic) / static_cast<double>(beforeStatic)) << "% fewer)";
    std::cout << "\n";

    Interp beforeInterp, afterInterp;
    long long beforeDynamic = -1, afterDynamic = -1;
    try {
        beforeInterp.run(baseline, "main");
        beforeDynamic = beforeInterp.instructionsExecuted;
        afterInterp.run(optimized, "main");
        afterDynamic = afterInterp.instructionsExecuted;
    } catch (const Interp::RuntimeError& e) {
        std::cerr << "minic: runtime error while measuring dynamic instructions: " << e.message << "\n";
        return 1;
    }
    std::cout << "dynamic instructions: " << beforeDynamic << " -> " << afterDynamic;
    if (beforeDynamic > 0)
        std::cout << "  (" << (100.0 * static_cast<double>(beforeDynamic - afterDynamic) / static_cast<double>(beforeDynamic)) << "% fewer)";
    std::cout << "\n";
    return 0;
}
static int dumpCheck(const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::cerr << "minic: cannot open '" << path << "'\n"; return 2; }
    std::stringstream ss; ss << in.rdbuf();

    Lexer lex(ss.str(), path);
    std::vector<Token> toks = lex.tokenize();

    Parser parser(toks, path);
    std::unique_ptr<Program> prog = parser.parseProgram();

    Sema sema;
    bool clean = sema.analyze(*prog);

    for (const Diagnostic& d : lex.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : parser.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : sema.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";

    size_t total = lex.errors().size() + parser.errors().size() + sema.errors().size();
    std::cerr << total << " error" << (total == 1 ? "" : "s") << ".\n";
    return (total == 0 && clean) ? 0 : 1;
}

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) { usage(); return 1; }

    std::string mode, file;
    OptConfig opt;
    for (const std::string& a : args) {
        if (a == "--help")    { usage(); return 0; }
        if (a == "--version") { std::cout << "MiniC-Opt 0.1.0 (Review 1 — lexer)\n"; return 0; }
        if (a == "--no-opt")  { opt.requested = true; opt.noOpt = true; continue; }
        if (a.rfind("--opt=", 0) == 0) {
            opt.requested = true;
            std::string list = a.substr(6);
            std::string cur;
            for (char c : list) {
                if (c == ',') { if (!cur.empty()) opt.only.push_back(cur); cur.clear(); }
                else cur += c;
            }
            if (!cur.empty()) opt.only.push_back(cur);
            continue;
        }
        if (a.rfind("--", 0) == 0) mode = a; else file = a;
    }

    if (file.empty()) { std::cerr << "minic: no input file\n"; return 1; }

    if (mode == "--dump-tokens" || mode.empty()) return dumpTokens(file);
    if (mode == "--dump-ast") return dumpAst(file);
    if (mode == "--check") return dumpCheck(file);
    if (mode == "--dump-ir") return dumpIr(file, opt);
    if (mode == "--run") return runProgram(file, opt);
    if (mode == "--dump-cfg") return dumpCfg(file, opt);
    if (mode.rfind("--dump-dataflow=", 0) == 0) return dumpDataflow(file, mode.substr(16));
    if (mode == "--emit-asm") return dumpAsm(file, opt);
    if (mode == "--metrics") return printMetrics(file, opt);

    std::cerr << "minic: '" << mode << "' is not implemented yet.\n"
              << "       Review 2 delivers the lexer and parser; run --help "
                 "to see the milestone for each stage.\n";
    return 3;
}
