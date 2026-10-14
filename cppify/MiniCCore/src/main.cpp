// MiniC-Opt — driver
//
// The full pipeline is implemented: lexer, parser, semantic analysis, TAC
// generation, reference interpreter, CFG and data-flow engine, the nine
// optimization passes P1-P9 behind a fixed-point pass manager, and the
// slot-addressed pseudo-assembly backend. Each stage has a corresponding
// --dump-*/--check/--run flag below; see usage() for the full list.
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
#include "trace_json.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

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
// unchanged: it has no control flow for a CFG-based pass to act on.
//
// The names written by globalInit are exactly the program's globals (every
// global is initialized there, explicitly or to zero), and they are handed
// to the pass manager because P5 cannot otherwise tell a global apart from
// a dead local — see makeDeadCodeEliminationPass in opt_passes.h.
static IRProgram applyOptimization(const IRProgram& ir, const OptConfig& opt,
                                    std::vector<PassManager::PassReport>* reportOut,
                                    TraceRecorder* recorder = nullptr) {
    IRProgram out;
    out.globalInit = ir.globalInit;

    std::set<std::string> globals;
    for (const Quad& q : ir.globalInit)
        if (q.result.kind == OperandKind::VARIABLE) globals.insert(q.result.name);

    for (const IRFunction& fn : ir.functions) {
        CFG cfg(fn);
        if (!opt.noOpt) {
            PassManager pm;
            registerAllPasses(pm, globals);
            if (!opt.only.empty()) pm.enableOnly(opt.only);
            pm.run(cfg, 32, recorder, fn.name);
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

// Cppify fork only. Emits one JSON document describing everything the app's
// Processing panel needs: diagnostics, the unoptimized IR, every sweep and
// pass with its individual edits, the metrics, and the result of running the
// program both ways. Diagnostics are reported as data rather than printed,
// so the app can place them in the editor gutter instead of parsing stderr.
static int traceJson(const std::string& path, const OptConfig& optIn) {
    std::ifstream in(path);
    if (!in) {
        std::cout << "{\"ok\":false,\"error\":\"cannot open file\"}\n";
        return 2;
    }
    std::stringstream ss; ss << in.rdbuf();
    const std::string source = ss.str();

    // Front end, collecting diagnostics instead of printing them.
    Lexer lex(source, path);
    std::vector<Token> toks = lex.tokenize();
    Parser parser(toks, path);
    std::unique_ptr<Program> prog = parser.parseProgram();
    Sema sema;
    bool semaOk = parser.errors().empty() && prog && sema.analyze(*prog);

    std::ostringstream os;
    os << "{\"ok\":" << (semaOk ? "true" : "false");
    os << ",\"diagnostics\":[";
    bool wroteAny = false;
    if (!lex.errors().empty()) { writeDiagnostics(os, lex.errors(), "lexer"); wroteAny = true; }
    if (!parser.errors().empty()) {
        if (wroteAny) os << ',';
        writeDiagnostics(os, parser.errors(), "parser"); wroteAny = true;
    }
    if (parser.errors().empty() && !sema.errors().empty()) {
        if (wroteAny) os << ',';
        writeDiagnostics(os, sema.errors(), "sema");
    }
    os << ']';

    if (!semaOk) {
        os << '}';
        std::cout << os.str() << "\n";
        return 1;
    }

    IRGen gen;
    IRProgram unopt = gen.generate(*prog);

    OptConfig opt = optIn;
    opt.requested = true;
    if (opt.only.empty() && !opt.noOpt) {
        opt.only = {"fold","constprop","copyprop","cse","dce",
                    "unreachable","strength","licm","peephole"};
    }

    TraceRecorder recorder;
    std::vector<PassManager::PassReport> report;
    IRProgram optimized = applyOptimization(unopt, opt, &report, &recorder);

    // The program is executed TWICE (once unoptimized as the reference, once
    // optimized), and both runs may call read_int(). stdin can only be
    // consumed once, so it is slurped here and replayed into each run from a
    // fresh stream -- otherwise the second run always hit end-of-input and
    // the differential check reported a spurious mismatch.
    std::string stdinData;
    {
        std::ostringstream all;
        all << std::cin.rdbuf();
        stdinData = all.str();
    }

    // Run both ways. A program that traps (division by zero, out-of-bounds)
    // is a legitimate outcome to report, not a reason to fail the trace.
    auto execute = [&stdinData](const IRProgram& p, std::string& out, long long& dyn) -> int {
        std::ostringstream captured;
        std::istringstream feed(stdinData);
        std::streambuf* old = std::cout.rdbuf(captured.rdbuf());
        std::streambuf* oldIn = std::cin.rdbuf(feed.rdbuf());
        std::cin.clear();
        int rc = 0;
        long long value = 0;
        try {
            Interp interp;
            value = interp.run(p).ival;
            dyn = interp.instructionsExecuted;
        } catch (const Interp::RuntimeError& e) {
            std::cout.rdbuf(old);
            std::cin.rdbuf(oldIn);
            out = "runtime error: " + e.message;
            return 1;
        }
        std::cout.rdbuf(old);
        std::cin.rdbuf(oldIn);
        out = captured.str();
        if (!out.empty() && out.back() != '\n') out += "\n";
        out += std::to_string(value) + "\n";
        return rc;
    };

    std::string unoptOut, optOut;
    long long unoptDyn = 0, optDyn = 0;
    int unoptRc = execute(unopt, unoptOut, unoptDyn);
    int optRc   = execute(optimized, optOut, optDyn);

    auto staticCount = [](const IRProgram& p) {
        int n = 0;
        for (const IRFunction& f : p.functions) n += static_cast<int>(f.code.size());
        return n;
    };

    os << ",\"source\":";  writeJsonString(os, source);
    os << ",\"trace\":";   writeTrace(os, recorder);
    os << ",\"metrics\":{";
    os << "\"staticBefore\":"  << staticCount(unopt);
    os << ",\"staticAfter\":"  << staticCount(optimized);
    os << ",\"dynamicBefore\":" << unoptDyn;
    os << ",\"dynamicAfter\":"  << optDyn;
    os << ",\"perPass\":{";
    {
        std::vector<std::string> order;
        std::unordered_map<std::string,int> totals;
        for (const auto& r : report) {
            if (!totals.count(r.pass)) order.push_back(r.pass);
            totals[r.pass] += r.transforms;
        }
        for (size_t i = 0; i < order.size(); ++i) {
            if (i) os << ',';
            writeJsonString(os, order[i]);
            os << ':' << totals[order[i]];
        }
    }
    os << "}}";
    os << ",\"run\":{";
    os << "\"unoptimized\":{\"exitCode\":" << unoptRc << ",\"output\":";
    writeJsonString(os, unoptOut);
    os << "},\"optimized\":{\"exitCode\":" << optRc << ",\"output\":";
    writeJsonString(os, optOut);
    os << "},\"outputsMatch\":" << ((unoptOut == optOut && unoptRc == optRc) ? "true" : "false");
    os << '}';
    os << '}';

    std::cout << os.str() << "\n";
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
        // Version literal kept in step with project(... VERSION) in CMakeLists.txt.
        if (a == "--version") { std::cout << "MiniC-Opt 0.1.0 (full pipeline: lexer, parser, sema, IR, CFG, P1-P9, backend)\n"; return 0; }
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
    if (mode == "--trace-json") return traceJson(file, opt);

    std::cerr << "minic: unknown option '" << mode << "'.\n"
              << "       Run --help to see the available options.\n";
    return 3;
}
