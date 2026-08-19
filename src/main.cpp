// MiniC-Opt — driver
//
// Review 1 status: the lexical analyser is implemented and demonstrable.
// Later phases (parser, semantic analyser, IR, CFG, optimizer, interpreter)
// are stubbed and report their planned review milestone.
#include "lexer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace minic;

static void usage() {
    std::cout <<
"MiniC-Opt — an optimizing compiler for a subset of C\n"
"\n"
"usage: minic [options] <file.c>\n"
"\n"
"  --dump-tokens     print the token stream                   [Review 1  DONE]\n"
"  --dump-ast        print the abstract syntax tree           [Review 2]\n"
"  --dump-ir         print unoptimized three-address code     [Review 2]\n"
"  --dump-cfg        emit the control-flow graph as Graphviz  [Review 2]\n"
"  --opt=<list>      enable named optimization passes         [Review 2/3]\n"
"  --no-opt          disable all optimization (baseline)      [Review 2]\n"
"  --metrics         print the before/after instruction report[Review 3]\n"
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

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) { usage(); return 1; }

    std::string mode, file;
    for (const std::string& a : args) {
        if (a == "--help")    { usage(); return 0; }
        if (a == "--version") { std::cout << "MiniC-Opt 0.1.0 (Review 1 — lexer)\n"; return 0; }
        if (a.rfind("--", 0) == 0) mode = a; else file = a;
    }

    if (file.empty()) { std::cerr << "minic: no input file\n"; return 1; }

    if (mode == "--dump-tokens" || mode.empty()) return dumpTokens(file);

    std::cerr << "minic: '" << mode << "' is not implemented yet.\n"
              << "       Review 1 delivers the lexical analyser; run --help "
                 "to see the milestone for each stage.\n";
    return 3;
}
