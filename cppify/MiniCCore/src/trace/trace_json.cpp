// MiniC-Opt (Cppify fork) — serialises a TraceRecorder to JSON.
//
// Hand-written rather than pulling in a JSON library, for the same reason
// the lexer is hand-written: this fork inherits the original's zero-
// dependency rule, and the document shape here is small and fully known.
#include "trace_json.h"

#include <sstream>

namespace minic {
namespace {

void escape(std::ostream& os, const std::string& s) {
    os << '"';
    for (char c : s) {
        switch (c) {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\n': os << "\\n";  break;
            case '\r': os << "\\r";  break;
            case '\t': os << "\\t";  break;
            default:
                // Control characters must be escaped to stay valid JSON.
                if (static_cast<unsigned char>(c) < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    os << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
                } else {
                    os << c;
                }
        }
    }
    os << '"';
}

void writeStringArray(std::ostream& os, const std::vector<std::string>& v) {
    os << '[';
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) os << ',';
        escape(os, v[i]);
    }
    os << ']';
}

void writeEdit(std::ostream& os, const TraceEdit& e) {
    os << '{';
    os << "\"function\":";   escape(os, e.function);
    os << ",\"quadIndex\":"  << e.quadIndex;
    os << ",\"before\":";    escape(os, e.before);
    os << ",\"after\":";     escape(os, e.after);
    os << ",\"kind\":";      escape(os, e.kind);
    os << ",\"reason\":";    escape(os, e.reason);
    os << ",\"sourceLine\":" << e.sourceLine;
    os << '}';
}

// Human-facing title for each pass id. The ids themselves are what --opt=
// accepts, so they stay terse; the panel wants something readable.
std::string titleFor(const std::string& pass) {
    if (pass == "fold")        return "Constant Folding";
    if (pass == "constprop")   return "Constant Propagation";
    if (pass == "copyprop")    return "Copy Propagation";
    if (pass == "cse")         return "Common Subexpression Elimination";
    if (pass == "dce")         return "Dead Code Elimination";
    if (pass == "unreachable") return "Unreachable Code Elimination";
    if (pass == "strength")    return "Strength Reduction";
    if (pass == "licm")        return "Loop-Invariant Code Motion";
    if (pass == "peephole")    return "Peephole Optimization";
    return pass;
}

void writePass(std::ostream& os, const TracePass& p) {
    os << '{';
    os << "\"pass\":";          escape(os, p.pass);
    os << ",\"title\":";        escape(os, titleFor(p.pass));
    os << ",\"description\":";  escape(os, p.description);
    os << ",\"precondition\":"; escape(os, p.precondition);
    os << ",\"transforms\":"    << p.transforms;
    os << ",\"instrBefore\":"   << p.instrBefore;
    os << ",\"instrAfter\":"    << p.instrAfter;
    os << ",\"changed\":"       << (p.changed ? "true" : "false");
    os << ",\"irBefore\":";     writeStringArray(os, p.irBefore);
    os << ",\"irAfter\":";      writeStringArray(os, p.irAfter);
    os << ",\"edits\":[";
    for (size_t i = 0; i < p.edits.size(); ++i) {
        if (i) os << ',';
        writeEdit(os, p.edits[i]);
    }
    os << "]}";
}

void writeFunction(std::ostream& os, const FunctionTrace& f) {
    os << '{';
    os << "\"name\":";               escape(os, f.name);
    os << ",\"sweepCount\":"         << f.sweepCount;
    os << ",\"fixedPointReached\":"  << (f.fixedPointReached ? "true" : "false");
    os << ",\"irInitial\":";         writeStringArray(os, f.irInitial);
    os << ",\"irFinal\":";           writeStringArray(os, f.irFinal);
    os << ",\"sweeps\":[";
    for (size_t i = 0; i < f.sweeps.size(); ++i) {
        if (i) os << ',';
        os << "{\"sweep\":" << f.sweeps[i].sweep << ",\"passes\":[";
        for (size_t j = 0; j < f.sweeps[i].passes.size(); ++j) {
            if (j) os << ',';
            writePass(os, f.sweeps[i].passes[j]);
        }
        os << "]}";
    }
    os << "]}";
}

}  // namespace

void writeJsonString(std::ostream& os, const std::string& s) { escape(os, s); }

void writeDiagnostics(std::ostream& os, const std::vector<Diagnostic>& diags,
                      const std::string& stage) {
    for (size_t i = 0; i < diags.size(); ++i) {
        if (i) os << ',';
        os << "{\"stage\":"; escape(os, stage);
        os << ",\"line\":"   << diags[i].line;
        os << ",\"col\":"    << diags[i].col;
        os << ",\"severity\":\"error\",\"message\":";
        escape(os, diags[i].message);
        os << '}';
    }
}

void writeTrace(std::ostream& os, const TraceRecorder& rec) {
    os << '[';
    const std::vector<FunctionTrace>& fns = rec.functions();
    for (size_t i = 0; i < fns.size(); ++i) {
        if (i) os << ',';
        writeFunction(os, fns[i]);
    }
    os << ']';
}

}  // namespace minic
