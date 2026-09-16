// MiniC-Opt — shared diagnostic type
// Owner: Member 2 (Error Reporting / M10)
//
// One shape for every front-end stage's error messages, so the driver can
// report lexer, parser and (from Step 3) semantic-analysis diagnostics
// through the same formatting code instead of each stage inventing its own.
#pragma once
#include <string>
#include <vector>

namespace minic {

struct Diagnostic {
    int         line;
    int         col;
    std::string message;
};

// A small append-only collector. Stages that only ever accumulate into a
// flat vector (the lexer) are free to keep doing that directly — this
// exists for stages (the parser, later sema) that want the collection
// behaviour named rather than reimplemented.
class DiagnosticSink {
public:
    void add(int line, int col, std::string message) {
        diags_.push_back(Diagnostic{line, col, std::move(message)});
    }
    bool empty() const { return diags_.empty(); }
    size_t size() const { return diags_.size(); }
    const std::vector<Diagnostic>& all() const { return diags_; }

private:
    std::vector<Diagnostic> diags_;
};

} // namespace minic
