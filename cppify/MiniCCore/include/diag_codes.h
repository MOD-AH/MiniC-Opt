// Cppify — stable diagnostic codes (roadmap 5.4)
//
// The app groups, filters and links diagnostics by code, so a code must not
// change when a message's wording does. Codes are assigned here, from the
// message text, by an ordered list of patterns; docs/diagnostics.md lists
// them. A message no pattern recognises gets its stage's -000 code, and
// run_all fails if any negative test produces one.
#pragma once
#include <string>

namespace minic {

struct DiagInfo {
    std::string code;        // e.g. "CPPIFY-PARSE-002"
    std::string title;       // short category name, e.g. "Expected an expression"
    bool unsupported = false;  // C that Cppify deliberately does not implement
};

// stage: "lexer", "parser" or "sema", as in --trace-json.
DiagInfo classifyDiagnostic(const std::string& stage, const std::string& message);

}  // namespace minic
