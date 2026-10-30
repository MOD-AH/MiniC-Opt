// Cppify — choose a lexer (roadmap 4.1, 4.3).
#pragma once
#include "diagnostic.h"
#include "token.h"
#include <string>
#include <vector>

namespace minic {

enum class LexerKind { HANDWRITTEN, RE2C };

struct LexResult {
    std::vector<Token> tokens;
    std::vector<Diagnostic> errors;
};

LexResult lexSource(LexerKind kind, const std::string& source, const std::string& filename);

}  // namespace minic
