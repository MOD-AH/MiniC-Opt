// Cppify — the Bison-generated parser (roadmap 4.2), behind the same
// interface as the hand-written Parser. Grammar: src/parser/parser_bison.y.
#pragma once
#include "ast.h"
#include "diagnostic.h"
#include "token.h"
#include <memory>
#include <string>
#include <vector>

namespace minic {

class BisonFrontEnd {
public:
    BisonFrontEnd(std::vector<Token> tokens, std::string filename);
    // Never returns null; syntax errors are in errors().
    std::unique_ptr<Program> parseProgram();
    const std::vector<Diagnostic>& errors() const { return sink_.all(); }

private:
    std::vector<Token> toks_;
    std::string file_;
    DiagnosticSink sink_;
};

}  // namespace minic
