// MiniC-Opt (Cppify fork) — JSON serialisation for the optimization trace.
#pragma once
#include "diagnostic.h"
#include "trace.h"

#include <ostream>
#include <string>
#include <vector>

namespace minic {

// Writes `s` as a quoted, escaped JSON string.
void writeJsonString(std::ostream& os, const std::string& s);

// Writes diagnostics as the ELEMENTS of a JSON array (no enclosing
// brackets), so several stages can be concatenated into one array. Each
// carries its stable code (diag_codes.h) and whether it is primary: a likely
// root cause. Parser diagnostics after the first, and any parser diagnostic
// after a lexical error (`afterEarlierErrors`), may only be consequences of
// it, so they are secondary.
void writeDiagnostics(std::ostream& os, const std::vector<Diagnostic>& diags,
                      const std::string& stage, bool afterEarlierErrors = false);

// Writes the whole recording as a JSON array of per-function objects.
void writeTrace(std::ostream& os, const TraceRecorder& rec);

}  // namespace minic
