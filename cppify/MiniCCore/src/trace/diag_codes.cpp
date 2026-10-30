// Cppify — stable diagnostic codes (see include/diag_codes.h)
#include "diag_codes.h"
#include <vector>

namespace minic {
namespace {

struct Rule {
    const char* stage;     // "lexer" also covers the preprocessor
    const char* needle;    // the message contains this
    const char* code;
    const char* title;
    bool unsupported;
};

// Order matters: the first rule that matches wins, so narrower patterns come
// before broader ones of the same stage. Append new rules; never renumber.
const std::vector<Rule>& rules() {
    static const std::vector<Rule> r = {
        // preprocessor (reported by the lexer)
        {"lexer", "#if condition",               "CPPIFY-PP-001", "Unsupported #if condition", true},
        {"lexer", "#elif condition",             "CPPIFY-PP-001", "Unsupported #if condition", true},
        {"lexer", "without a matching #if",      "CPPIFY-PP-002", "Unmatched #endif, #else or #elif", false},
        {"lexer", "unterminated #if",            "CPPIFY-PP-003", "Unterminated #if", false},
        {"lexer", "function-like macro",         "CPPIFY-PP-004", "Function-like macro", true},
        {"lexer", "after #else",                 "CPPIFY-PP-005", "Directive after #else", false},
        // lexer
        {"lexer", "unexpected character",        "CPPIFY-LEX-001", "Unexpected character", false},
        {"lexer", "unterminated block comment",  "CPPIFY-LEX-002", "Unterminated comment", false},
        {"lexer", "malformed numeric literal",   "CPPIFY-LEX-003", "Malformed number", false},
        {"lexer", "unterminated character",      "CPPIFY-LEX-004", "Unterminated character literal", false},
        {"lexer", "unterminated string",         "CPPIFY-LEX-005", "Unterminated string", false},
        // parser
        {"parser", "'union'",                    "CPPIFY-PARSE-017", "union (not supported)", true},
        {"parser", "function pointers",          "CPPIFY-PARSE-018", "Function pointer (not supported)", true},
        {"parser", "expected ';'",               "CPPIFY-PARSE-001", "Missing ';'", false},
        {"parser", "expected an expression",     "CPPIFY-PARSE-002", "Expected an expression", false},
        {"parser", "expected a type",            "CPPIFY-PARSE-003", "Expected a type", false},
        {"parser", "expected a declaration",     "CPPIFY-PARSE-004", "Expected a declaration", false},
        {"parser", "left-hand side of assignment", "CPPIFY-PARSE-007", "Not assignable", false},
        {"parser", "nests too deeply",           "CPPIFY-PARSE-008", "Expression too deeply nested", false},
        {"parser", "has no size",                "CPPIFY-PARSE-009", "Array without a size", false},
        {"parser", "array size",                 "CPPIFY-PARSE-010", "Bad array size", false},
        {"parser", "array dimension",            "CPPIFY-PARSE-010", "Bad array size", false},
        {"parser", "constant integer expression", "CPPIFY-PARSE-011", "Not a constant", false},
        {"parser", "initializer",                "CPPIFY-PARSE-012", "Bad initializer", false},
        {"parser", "struct",                     "CPPIFY-PARSE-013", "Bad struct definition", false},
        {"parser", "field",                      "CPPIFY-PARSE-013", "Bad struct definition", false},
        {"parser", "not a function name",        "CPPIFY-PARSE-014", "Call of a non-function", false},
        {"parser", "before the first 'case'",    "CPPIFY-PARSE-015", "Statement outside a case", false},
        {"parser", "parameter name",             "CPPIFY-PARSE-016", "Missing parameter name", false},
        {"parser", "expected ",                  "CPPIFY-PARSE-005", "Unexpected token", false},
        {"parser", "unexpected ",                "CPPIFY-PARSE-006", "Unexpected token", false},
        // semantic analysis
        {"sema", "function pointers",            "CPPIFY-SEMA-018", "Function pointer (not supported)", true},
        {"sema", "undeclared function 'fopen'",  "CPPIFY-SEMA-019", "File I/O (not supported)", true},
        {"sema", "undeclared function 'fclose'", "CPPIFY-SEMA-019", "File I/O (not supported)", true},
        {"sema", "undeclared function 'fread'",  "CPPIFY-SEMA-019", "File I/O (not supported)", true},
        {"sema", "undeclared function 'fwrite'", "CPPIFY-SEMA-019", "File I/O (not supported)", true},
        {"sema", "undeclared function",          "CPPIFY-SEMA-002", "Undeclared function", false},
        {"sema", "undeclared identifier",        "CPPIFY-SEMA-001", "Undeclared identifier", false},
        {"sema", "expects ",                     "CPPIFY-SEMA-003", "Wrong number of arguments", false},
        {"sema", "missing return",               "CPPIFY-SEMA-005", "Missing return", false},
        {"sema", "'return",                      "CPPIFY-SEMA-006", "Return value mismatch", false},
        {"sema", "redeclaration",                "CPPIFY-SEMA-007", "Redeclaration", false},
        {"sema", "defined twice",                "CPPIFY-SEMA-007", "Redeclaration", false},
        {"sema", "shadows a built-in",           "CPPIFY-SEMA-007", "Redeclaration", false},
        {"sema", "division by zero",             "CPPIFY-SEMA-008", "Division by zero", false},
        {"sema", "duplicate case",               "CPPIFY-SEMA-014", "Bad switch", false},
        {"sema", "duplicate 'default'",          "CPPIFY-SEMA-014", "Bad switch", false},
        {"sema", "switch",                       "CPPIFY-SEMA-014", "Bad switch", false},
        {"sema", "label '",                      "CPPIFY-SEMA-013", "Undefined label", false},
        {"sema", "address",                      "CPPIFY-SEMA-017", "Library call misuse", false},
        {"sema", "format",                       "CPPIFY-SEMA-017", "Library call misuse", false},
        {"sema", "initializer",                  "CPPIFY-SEMA-012", "Bad initializer", false},
        {"sema", "type mismatch",                "CPPIFY-SEMA-004", "Type mismatch", false},
        {"sema", "wrong type",                   "CPPIFY-SEMA-004", "Type mismatch", false},
        {"sema", "does not match",               "CPPIFY-SEMA-004", "Type mismatch", false},
        {"sema", "incompatible",                 "CPPIFY-SEMA-004", "Type mismatch", false},
        {"sema", "assign to an array",           "CPPIFY-SEMA-015", "Not assignable", false},
        {"sema", "increment/decrement",          "CPPIFY-SEMA-015", "Not assignable", false},
        {"sema", "operand of '&'",               "CPPIFY-SEMA-015", "Not assignable", false},
        {"sema", "struct",                       "CPPIFY-SEMA-011", "Struct misuse", false},
        {"sema", "field",                        "CPPIFY-SEMA-011", "Struct misuse", false},
        {"sema", "'.'",                          "CPPIFY-SEMA-011", "Struct misuse", false},
        {"sema", "'->'",                         "CPPIFY-SEMA-011", "Struct misuse", false},
        {"sema", "pointer",                      "CPPIFY-SEMA-010", "Pointer misuse", false},
        {"sema", "subscript",                    "CPPIFY-SEMA-010", "Pointer misuse", false},
        {"sema", "void",                         "CPPIFY-SEMA-016", "Void misuse", false},
        {"sema", "must be integral",             "CPPIFY-SEMA-009", "Wrong operand type", false},
        {"sema", "must be numeric",              "CPPIFY-SEMA-009", "Wrong operand type", false},
        {"sema", "must be a scalar",             "CPPIFY-SEMA-009", "Wrong operand type", false},
        {"sema", "integral expression",          "CPPIFY-SEMA-009", "Wrong operand type", false},
        {"sema", "invalid operand",              "CPPIFY-SEMA-009", "Wrong operand type", false},
        {"sema", "invalid operator",             "CPPIFY-SEMA-009", "Wrong operand type", false},
        {"sema", "cannot cast",                  "CPPIFY-SEMA-009", "Wrong operand type", false},
        {"sema", "too many initializers",        "CPPIFY-SEMA-012", "Bad initializer", false},
    };
    return r;
}

}  // namespace

DiagInfo classifyDiagnostic(const std::string& stage, const std::string& message) {
    for (const Rule& r : rules())
        if (stage == r.stage && message.find(r.needle) != std::string::npos)
            return DiagInfo{r.code, r.title, r.unsupported};
    std::string s = stage == "lexer" ? "LEX" : stage == "parser" ? "PARSE" : "SEMA";
    return DiagInfo{"CPPIFY-" + s + "-000", "Other", false};
}

}  // namespace minic
