// MiniC-Opt — driver
//
// The full pipeline is implemented: lexer, parser, semantic analysis, TAC
// generation, reference interpreter, CFG and data-flow engine, the nine
// optimization passes P1-P9 behind a fixed-point pass manager, and the
// slot-addressed pseudo-assembly backend. Each stage has a corresponding
// --dump-*/--check/--run flag below; see usage() for the full list.
#include "lexer.h"
#include "lexing.h"
#include "bison_frontend.h"
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
#include "pipeline.h"
#include "ast_interp.h"
#include "vm.h"
#include "ssa.h"
#include "ssa/ssa_util.h"
#include <algorithm>
#include "verify.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>

using namespace minic;

// --lexer=re2c|handwritten (roadmap 4.1); the same for every mode. The re2c
// lexer is the default because it passed the roadmap 4.3 test: identical
// tokens and diagnostics on every test file and on 10,000 fuzzed inputs
// (scripts/lexfuzz.py), and faster (docs/front-ends.md).
static LexerKind g_lexer = LexerKind::RE2C;
// --parser=bison|handwritten (roadmap 4.2). The hand-written parser stays the
// default: Bison builds the identical AST, but its error recovery reports
// different follow-on diagnostics in some syntax-error tests, and it parses
// about 4x slower (docs/front-ends.md).
static bool g_bison = false;

// Parses with whichever parser was chosen; diagnostics go to `errors`.
static std::unique_ptr<Program> parseTokens(std::vector<Token> toks, const std::string& path,
                                            std::vector<Diagnostic>& errors) {
    if (g_bison) {
        BisonFrontEnd p(std::move(toks), path);
        auto prog = p.parseProgram();
        errors = p.errors();
        return prog;
    }
    Parser p(std::move(toks), path);
    auto prog = p.parseProgram();
    errors = p.errors();
    return prog;
}

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
"  --run-ast         run the typed AST directly with the reference interpreter\n"
"                    (no IR, no optimizer); output in --run's format\n"
"  --run-vm          like --run, on the register bytecode VM (takes --opt=/--preset=)\n"
"  --compile         compile and optimize, print only the instruction count\n"
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
"  --preset=<name>   optimization pipeline: correctness (none), balanced (~-O1),\n"
"                    classic (P1-P9 to a fixed point), optimized (~-O2: SSA\n"
"                    passes sccp,gvn,licm-ssa,ssa-dce, then P1-P9 minus P7)\n"
"  --config=<file>   pipeline from a JSON file: {\"preset\": \"...\"} or\n"
"                    {\"groups\": [{\"passes\": [...], \"repeat\": \"fixpoint\"|n}]}\n"
"  --print-pipeline  print the effective pass order and exit\n"
"  --verify-each     run the IR verifier before the first pass and after\n"
"                    every pass (also: MINIC_VERIFY=1 in the environment)\n"
"  --list-techniques print the technique registry as JSON\n"
"  --lexer=<which>   re2c (default; generated DFA, roadmap 4.1) or handwritten\n"
"  --parser=<which>  handwritten (default) or bison (IELR(1), roadmap 4.2)\n"
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

    LexResult lex = lexSource(g_lexer, ss.str(), path);
    std::vector<Token>& toks = lex.tokens;

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

    for (const Diagnostic& d : lex.errors)
        std::cerr << path << ":" << d.line << ":" << d.col
                  << ": error: " << d.message << "\n";

    std::cout << "\n" << toks.size() << " tokens, "
              << lex.errors.size() << " lexical error"
              << (lex.errors.size() == 1 ? "" : "s") << ".\n";

    return lex.errors.empty() ? 0 : 1;
}

static int dumpAst(const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::cerr << "minic: cannot open '" << path << "'\n"; return 2; }
    std::stringstream ss; ss << in.rdbuf();

    LexResult lex = lexSource(g_lexer, ss.str(), path);
    std::vector<Token>& toks = lex.tokens;

    std::vector<Diagnostic> parseErrors;
    std::unique_ptr<Program> prog = parseTokens(toks, path, parseErrors);

    AstPrinter printer(std::cout);
    printer.print(*prog);

    for (const Diagnostic& d : lex.errors)
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : parseErrors)
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";

    size_t total = lex.errors.size() + parseErrors.size();
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

    LexResult lex = lexSource(g_lexer, ss.str(), path);
    std::vector<Token>& toks = lex.tokens;

    std::vector<Diagnostic> parseErrors;
    std::unique_ptr<Program> prog = parseTokens(toks, path, parseErrors);

    // After a syntax error the AST is only what recovery salvaged; whatever
    // Sema would infer from it ("missing return", "undeclared x") is an echo
    // of that error, not a new one. So semantic analysis runs only on a clean
    // parse, as --trace-json (and so the app) already did.
    Sema sema;
    bool clean = lex.errors.empty() && parseErrors.empty() && sema.analyze(*prog);

    for (const Diagnostic& d : lex.errors)
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : parseErrors)
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : sema.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";

    size_t total = lex.errors.size() + parseErrors.size() + sema.errors().size();
    if (total != 0 || !clean) {
        std::cerr << total << " error" << (total == 1 ? "" : "s") << "; not generating IR.\n";
        return prog;
    }
    ok = true;
    return prog;
}

struct OptConfig {
    bool requested = false;      // true iff --opt=, --no-opt, --preset= or --config= was given
    bool noOpt = false;          // --no-opt: run the pipeline with zero passes enabled
    std::vector<std::string> only;   // --opt=name,name,...  (empty + requested-without-noOpt = all passes)
    Pipeline pipeline;           // what actually runs; resolved in main()
    bool verify = false;         // --verify-each / MINIC_VERIFY=1
};

// Wall-clock milliseconds per front-end and back-end stage, for --trace-json.
struct StageTimes {
    std::vector<std::pair<std::string, double>> ms;
    std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
    void lap(const char* stage) {
        auto now = std::chrono::steady_clock::now();
        ms.emplace_back(stage, std::chrono::duration<double, std::milli>(now - t).count());
        t = now;
    }
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
                                    TraceRecorder* recorder = nullptr,
                                    std::map<std::string, AnalysisManager::Counts>* analysesOut = nullptr) {
    IRProgram out;
    out.globalInit = ir.globalInit;

    std::set<std::string> globals;
    for (const Quad& q : ir.globalInit)
        if (q.result.kind == OperandKind::VARIABLE) globals.insert(q.result.name);

    for (const IRFunction& fn : ir.functions) {
        CFG cfg(fn);
        PipelineRun run;
        runPipeline(cfg, opt.pipeline, globals, fn.name, opt.verify, recorder, run);
        if (reportOut) reportOut->insert(reportOut->end(), run.report.begin(), run.report.end());
        if (analysesOut)
            for (const auto& kv : run.analyses) {
                (*analysesOut)[kv.first].computed += kv.second.computed;
                (*analysesOut)[kv.first].reused += kv.second.reused;
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

// Roadmap 3.1: runs the typed AST directly, with no IR at all, and prints
// exactly what --run prints. Comparing the two catches an IRGen bug, which
// unoptimized-vs-optimized never can (both start from the same IR).
static int runAst(const std::string& path) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    AstInterp interp;
    try {
        Value result = interp.run(*prog);
        if (result.kind == Value::Kind::FLOAT) std::cout << result.fval << "\n";
        else                                    std::cout << result.ival << "\n";
    } catch (const Interp::RuntimeError& e) {
        std::cerr << "minic: runtime error: " << e.message << "\n";
        return 1;
    }
    return 0;
}

// Front end, IRGen and the optimizer, printing only the size of the result:
// what scripts/bench.py times as "compile" (--dump-ir would time printing).
static int compileOnly(const std::string& path, const OptConfig& opt) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    if (opt.requested) ir = applyOptimization(ir, opt, nullptr);
    size_t n = ir.globalInit.size();
    for (const auto& f : ir.functions) n += f.code.size();
    std::cout << n << " instructions\n";
    return 0;
}

// Roadmap 3.2: the same as --run, on the register bytecode VM.
static int runVm(const std::string& path, const OptConfig& opt) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    if (opt.requested) ir = applyOptimization(ir, opt, nullptr);
    VM vm;
    try {
        Value result = vm.run(ir);
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
    std::map<std::string, AnalysisManager::Counts> analyses;
    IRProgram optimized = opt.noOpt ? baseline : applyOptimization(baseline, opt, &report, nullptr, &analyses);

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

    if (!analyses.empty()) {
        std::cout << "\nanalyses (computed / served from cache):\n";
        for (const auto& kv : analyses)
            std::cout << "  " << kv.first << ": " << kv.second.computed << " / " << kv.second.reused << "\n";
    }

    std::cout << "\nstatic instructions:  " << beforeStatic << " -> " << afterStatic;
    if (beforeStatic > 0)
        std::cout << "  (" << (100.0 * static_cast<double>(beforeStatic - afterStatic) / static_cast<double>(beforeStatic)) << "% fewer)";
    std::cout << "\n";

    // Both runs must see the same input. Reading std::cin directly, the
    // optimized run found it already consumed by the first and took the
    // end-of-input path, so a program using scanf reported a large, false
    // reduction (stdin_scanf_fgets.c: 223 -> 42 instead of 223 -> 214).
    // Same remedy as --trace-json: read stdin once, replay it to each run.
    std::string input;
    {
        std::ostringstream all;
        all << std::cin.rdbuf();
        input = all.str();
    }
    auto countRun = [&input](const IRProgram& p) {
        std::istringstream feed(input);
        std::streambuf* old = std::cin.rdbuf(feed.rdbuf());
        std::cin.clear();
        Interp interp;
        try {
            interp.run(p, "main");
        } catch (...) {
            std::cin.rdbuf(old);
            throw;
        }
        std::cin.rdbuf(old);
        return interp.instructionsExecuted;
    };
    long long beforeDynamic = -1, afterDynamic = -1;
    try {
        beforeDynamic = countRun(baseline);
        afterDynamic = countRun(optimized);
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

// Runs one engine with stdin replayed from `input` and stdout captured, and
// renders the result as --run would: the program's output, then main's value.
// A runtime error is an outcome to report (exit code 1), not a failure.
template <class Body>
static int captureRun(const std::string& input, Body body, std::string& out, double& ms) {
    std::ostringstream captured;
    std::istringstream feed(input);
    std::streambuf* oldOut = std::cout.rdbuf(captured.rdbuf());
    std::streambuf* oldIn = std::cin.rdbuf(feed.rdbuf());
    std::cin.clear();
    auto t0 = std::chrono::steady_clock::now();
    int rc = 0;
    try {
        Value v = body();
        out = captured.str();
        if (!out.empty() && out.back() != '\n') out += "\n";
        out += (v.kind == Value::Kind::FLOAT ? std::to_string(v.fval) : std::to_string(v.ival)) + "\n";
    } catch (const Interp::RuntimeError& e) {
        // What the program printed before it stopped is often the best clue.
        out = captured.str();
        if (!out.empty() && out.back() != '\n') out += "\n";
        out += "runtime error: " + e.message;
        rc = 1;
    }
    ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::cout.rdbuf(oldOut);
    std::cin.rdbuf(oldIn);
    return rc;
}

static std::string slurpStdin() {
    std::ostringstream all;
    all << std::cin.rdbuf();
    return all.str();
}

// The CFG of every function, for the app's graph view: blocks with their
// code and successor edges.
static void writeCfgs(std::ostream& os, const IRProgram& ir) {
    os << '[';
    for (size_t fi = 0; fi < ir.functions.size(); ++fi) {
        if (fi) os << ',';
        CFG cfg(ir.functions[fi]);
        os << "{\"name\":"; writeJsonString(os, ir.functions[fi].name);
        os << ",\"blocks\":[";
        for (size_t bi = 0; bi < cfg.blocks().size(); ++bi) {
            const BasicBlock& b = cfg.blocks()[bi];
            if (bi) os << ',';
            os << "{\"id\":" << b.id << ",\"lines\":[";
            for (size_t qi = 0; qi < b.code.size(); ++qi) {
                if (qi) os << ',';
                writeJsonString(os, b.code[qi].toString());
            }
            os << "],\"succs\":[";
            for (size_t si = 0; si < b.succs.size(); ++si) os << (si ? "," : "") << b.succs[si];
            os << "]}";
        }
        os << "]}";
    }
    os << ']';
}

// Each function of `ir` in SSA form (phis included), whatever the pipeline,
// for the app's SSA view.
static void writeSsa(std::ostream& os, const IRProgram& ir) {
    os << '[';
    for (size_t fi = 0; fi < ir.functions.size(); ++fi) {
        if (fi) os << ',';
        CFG cfg(ir.functions[fi]);
        makeSSAConstructPass()->run(cfg);
        os << "{\"name\":"; writeJsonString(os, ir.functions[fi].name);
        os << ",\"lines\":[";
        bool first = true;
        auto line = [&](const std::string& t) { if (!first) os << ','; first = false; writeJsonString(os, t); };
        for (const auto& b : cfg.blocks()) {
            size_t start = bodyStart(b);
            for (size_t i = 0; i < start; ++i) line(b.code[i].toString());
            for (const Phi& phi : b.phis) line("    " + phiText(phi));
            for (size_t i = start; i < b.code.size(); ++i) line("    " + b.code[i].toString());
        }
        os << "]}";
    }
    os << ']';
}

// --bench-engines=N: compile once, then run each engine N times in-process
// (stdin replayed each time) and report median milliseconds. For the app's
// Benchmark view; scripts/bench.py measures whole processes instead.
static int benchEngines(const std::string& path, const OptConfig& opt, int runs) {
    bool ok = false;
    auto prog = checkedParse(path, ok);
    if (!ok) return 1;
    auto t0 = std::chrono::steady_clock::now();
    IRGen gen;
    IRProgram ir = gen.generate(*prog);
    IRProgram optimized = applyOptimization(ir, opt, nullptr);
    double compileMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::string input = slurpStdin();
    auto median = [&](auto body, std::string& out) {
        std::vector<double> t;
        for (int i = 0; i < runs; ++i) { double ms = 0; captureRun(input, body, out, ms); t.push_back(ms); }
        std::sort(t.begin(), t.end());
        return t[t.size() / 2];
    };
    std::string q, v, a;
    double quadMs = median([&] { Interp in; return in.run(optimized); }, q);
    double vmMs   = median([&] { VM vm; return vm.run(optimized); }, v);
    double astMs  = median([&] { AstInterp ai; return ai.run(*prog); }, a);
    std::cout << "{\"runs\":" << runs << ",\"compileMs\":" << compileMs << ",\"quadMs\":" << quadMs
              << ",\"vmMs\":" << vmMs << ",\"astMs\":" << astMs << ",\"agree\":"
              << (q == v && q == a ? "true" : "false") << "}\n";
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
    StageTimes times;
    LexResult lex = lexSource(g_lexer, source, path);
    std::vector<Token>& toks = lex.tokens;
    times.lap("lex");
    std::vector<Diagnostic> parseErrors;
    std::unique_ptr<Program> prog = parseTokens(toks, path, parseErrors);
    times.lap("parse");
    Sema sema;
    bool semaOk = lex.errors.empty() && parseErrors.empty() && prog && sema.analyze(*prog);
    times.lap("sema");

    std::ostringstream os;
    os << "{\"ok\":" << (semaOk ? "true" : "false");
    os << ",\"diagnostics\":[";
    bool wroteAny = false;
    if (!lex.errors.empty()) { writeDiagnostics(os, lex.errors, "lexer"); wroteAny = true; }
    if (!parseErrors.empty()) {
        if (wroteAny) os << ',';
        writeDiagnostics(os, parseErrors, "parser", !lex.errors.empty()); wroteAny = true;
    }
    if (parseErrors.empty() && !sema.errors().empty()) {
        if (wroteAny) os << ',';
        writeDiagnostics(os, sema.errors(), "sema");
    }
    os << ']';

    // For the Tokens and AST views, whether or not the program compiles.
    os << ",\"tokens\":[";
    for (size_t i = 0; i < toks.size() && i < 20000; ++i) {
        if (i) os << ',';
        os << "{\"type\":"; writeJsonString(os, tokName(toks[i].type));
        os << ",\"lexeme\":"; writeJsonString(os, toks[i].lexeme);
        os << ",\"line\":" << toks[i].line << ",\"col\":" << toks[i].col << '}';
    }
    os << ']';
    if (prog && parseErrors.empty()) {
        std::ostringstream ast;
        AstPrinter printer(ast);
        printer.print(*prog);
        os << ",\"ast\":"; writeJsonString(os, ast.str());
    }
    os << ",\"frontEnd\":{\"lexer\":\"" << (g_lexer == LexerKind::RE2C ? "re2c" : "handwritten")
       << "\",\"parser\":\"" << (g_bison ? "bison" : "handwritten") << "\"}";

    if (!semaOk) {
        os << '}';
        std::cout << os.str() << "\n";
        return 1;
    }

    IRGen gen;
    IRProgram unopt = gen.generate(*prog);

    times.lap("irgen");
    const OptConfig& opt = optIn;

    TraceRecorder recorder;
    std::vector<PassManager::PassReport> report;
    std::map<std::string, AnalysisManager::Counts> analyses;
    IRProgram optimized = applyOptimization(unopt, opt, &report, &recorder, &analyses);
    times.lap("optimize");

    // The program is executed several times (unoptimized and optimized on the
    // quad interpreter, then on the AST interpreter and the VM), and each run
    // may read stdin. stdin can only be consumed once, so it is slurped here
    // and replayed into each run -- otherwise the second run always hit
    // end-of-input and the differential check reported a spurious mismatch.
    std::string stdinData = slurpStdin();
    std::string unoptOut, optOut, astOut, vmOut;
    long long unoptDyn = 0, optDyn = 0;
    double ms = 0, astMs = 0, vmMs = 0;
    int unoptRc = captureRun(stdinData, [&] {
        Interp interp; Value v = interp.run(unopt); unoptDyn = interp.instructionsExecuted; return v; }, unoptOut, ms);
    times.lap("runUnoptimized");
    int optRc = captureRun(stdinData, [&] {
        Interp interp; Value v = interp.run(optimized); optDyn = interp.instructionsExecuted; return v; }, optOut, ms);
    times.lap("runOptimized");
    // The two independent engines (roadmap 3.1, 3.2), for the Compare view.
    int astRc = captureRun(stdinData, [&] { AstInterp a; return a.run(*prog); }, astOut, astMs);
    int vmRc = captureRun(stdinData, [&] { VM vm; return vm.run(optimized); }, vmOut, vmMs);
    times.lap("runEngines");

    auto staticCount = [](const IRProgram& p) {
        int n = 0;
        for (const IRFunction& f : p.functions) n += static_cast<int>(f.code.size());
        return n;
    };

    os << ",\"pipeline\":" << opt.pipeline.toJson();
    os << ",\"timingsMs\":{";
    for (size_t i = 0; i < times.ms.size(); ++i) {
        if (i) os << ',';
        writeJsonString(os, times.ms[i].first);
        os << ':' << times.ms[i].second;
    }
    os << "},\"analyses\":{";
    {
        bool first = true;
        for (const auto& kv : analyses) {
            if (!first) os << ',';
            first = false;
            writeJsonString(os, kv.first);
            os << ":{\"computed\":" << kv.second.computed << ",\"reused\":" << kv.second.reused << '}';
        }
    }
    os << '}';
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
    bool agree = astOut == unoptOut && astRc == unoptRc && vmOut == optOut && vmRc == optRc;
    os << ",\"engines\":{\"reference\":{\"exitCode\":" << astRc << ",\"output\":";
    writeJsonString(os, astOut);
    os << ",\"ms\":" << astMs << "},\"vm\":{\"exitCode\":" << vmRc << ",\"output\":";
    writeJsonString(os, vmOut);
    os << ",\"ms\":" << vmMs << "},\"allAgree\":" << (agree && unoptOut == optOut && unoptRc == optRc ? "true" : "false") << '}';
    os << ",\"cfg\":{\"unoptimized\":";
    writeCfgs(os, unopt);
    os << ",\"optimized\":";
    writeCfgs(os, optimized);
    os << "},\"ssa\":";
    writeSsa(os, unopt);
    os << '}';

    std::cout << os.str() << "\n";
    return 0;
}

static int dumpCheck(const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::cerr << "minic: cannot open '" << path << "'\n"; return 2; }
    std::stringstream ss; ss << in.rdbuf();

    LexResult lex = lexSource(g_lexer, ss.str(), path);
    std::vector<Token>& toks = lex.tokens;

    std::vector<Diagnostic> parseErrors;
    std::unique_ptr<Program> prog = parseTokens(toks, path, parseErrors);

    // After a syntax error the AST is only what recovery salvaged; whatever
    // Sema would infer from it ("missing return", "undeclared x") is an echo
    // of that error, not a new one. So semantic analysis runs only on a clean
    // parse, as --trace-json (and so the app) already did.
    Sema sema;
    bool clean = lex.errors.empty() && parseErrors.empty() && sema.analyze(*prog);

    for (const Diagnostic& d : lex.errors)
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : parseErrors)
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";
    for (const Diagnostic& d : sema.errors())
        std::cerr << path << ":" << d.line << ":" << d.col << ": error: " << d.message << "\n";

    size_t total = lex.errors.size() + parseErrors.size() + sema.errors().size();
    std::cerr << total << " error" << (total == 1 ? "" : "s") << ".\n";
    return (total == 0 && clean) ? 0 : 1;
}

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) { usage(); return 1; }

    std::string mode, file, preset, configPath;
    bool optList = false;   // --opt= given, even empty (the app sends that for "no passes")
    OptConfig opt;
    const char* envVerify = std::getenv("MINIC_VERIFY");
    opt.verify = envVerify && std::string(envVerify) == "1";
    for (const std::string& a : args) {
        if (a == "--help")    { usage(); return 0; }
        // Version literal kept in step with project(... VERSION) in CMakeLists.txt.
        if (a == "--version") { std::cout << "MiniC-Opt 0.1.0 (full pipeline: lexer, parser, sema, IR, CFG, P1-P9, backend)\n"; return 0; }
        if (a == "--list-techniques") { std::cout << registryJson(); return 0; }
        if (a == "--no-opt")  { opt.requested = true; opt.noOpt = true; continue; }
        if (a == "--verify-each") { opt.verify = true; continue; }
        if (a == "--lexer=re2c") { g_lexer = LexerKind::RE2C; continue; }
        if (a == "--lexer=handwritten") { g_lexer = LexerKind::HANDWRITTEN; continue; }
        if (a == "--parser=bison") { g_bison = true; continue; }
        if (a == "--parser=handwritten") { g_bison = false; continue; }
        if (a.rfind("--preset=", 0) == 0) { opt.requested = true; preset = a.substr(9); continue; }
        if (a.rfind("--config=", 0) == 0) { opt.requested = true; configPath = a.substr(9); continue; }
        if (a.rfind("--opt=", 0) == 0) {
            opt.requested = true;
            optList = true;
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

    // Exactly one way of choosing the pipeline; with none, P1-P9 (what
    // --metrics and --trace-json have always shown).
    int sources = (opt.noOpt ? 1 : 0) + (optList ? 1 : 0) + (!preset.empty() ? 1 : 0) +
                  (!configPath.empty() ? 1 : 0);
    if (sources > 1) {
        std::cerr << "minic: choose one of --no-opt, --opt=, --preset= and --config=\n";
        return 3;
    }
    std::string err;
    bool ok = true;
    if (opt.noOpt || (optList && opt.only.empty())) ok = presetPipeline("correctness", opt.pipeline, err);
    else if (optList)              ok = pipelineFromPassList(opt.only, opt.pipeline, err);
    else if (!preset.empty())      ok = presetPipeline(preset, opt.pipeline, err);
    else if (!configPath.empty()) {
        std::ifstream cf(configPath);
        if (!cf) { std::cerr << "minic: cannot open config '" << configPath << "'\n"; return 2; }
        std::stringstream cs; cs << cf.rdbuf();
        ok = parsePipelineConfig(cs.str(), opt.pipeline, err);
    } else                         ok = presetPipeline("classic", opt.pipeline, err);
    if (!ok) { std::cerr << "minic: invalid pipeline: " << err << "\n"; return 3; }
    if (opt.noOpt) opt.pipeline.preset = "correctness";
    else if (optList) opt.pipeline.preset = opt.only.empty() ? "correctness" : "custom (--opt)";

    if (mode == "--print-pipeline") { std::cout << opt.pipeline.describe(); return 0; }
    if (file.empty()) { std::cerr << "minic: no input file\n"; return 1; }

    try {
        if (mode == "--dump-tokens" || mode.empty()) return dumpTokens(file);
        if (mode == "--dump-ast") return dumpAst(file);
        if (mode == "--check") return dumpCheck(file);
        if (mode == "--dump-ir") return dumpIr(file, opt);
        if (mode == "--run") return runProgram(file, opt);
        if (mode == "--run-ast") return runAst(file);
        if (mode == "--run-vm") return runVm(file, opt);
        if (mode == "--compile") return compileOnly(file, opt);
        if (mode.rfind("--bench-engines=", 0) == 0) {
            int n = std::atoi(mode.c_str() + 16);
            return benchEngines(file, opt, n < 1 ? 1 : (n > 101 ? 101 : n));
        }
        if (mode == "--dump-cfg") return dumpCfg(file, opt);
        if (mode.rfind("--dump-dataflow=", 0) == 0) return dumpDataflow(file, mode.substr(16));
        if (mode == "--emit-asm") return dumpAsm(file, opt);
        if (mode == "--metrics") return printMetrics(file, opt);
        if (mode == "--trace-json") return traceJson(file, opt);
    } catch (const VerifyError& e) {
        std::cerr << "minic: " << e.message << "\n";
        return 4;
    }

    std::cerr << "minic: unknown option '" << mode << "'.\n"
              << "       Run --help to see the available options.\n";
    return 3;
}
