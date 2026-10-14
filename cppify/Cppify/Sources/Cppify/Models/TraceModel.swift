import Foundation

// Swift mirror of the JSON that `minic-trace --trace-json` emits. The field
// names match the C++ writer in MiniCCore/src/trace/trace_json.cpp exactly,
// so Codable needs no CodingKeys; if the two ever drift, decoding fails
// loudly at the boundary rather than silently producing an empty panel.

struct Diagnostic: Codable, Identifiable, Hashable {
    var id: String { "\(stage):\(line):\(col):\(message)" }
    let stage: String        // "lexer" | "parser" | "sema"
    let line: Int
    let col: Int
    let severity: String
    let message: String
}

struct TraceEdit: Codable, Identifiable, Hashable {
    var id: String { "\(function)-\(quadIndex)-\(before)-\(after)" }
    let function: String
    let quadIndex: Int
    let before: String
    let after: String
    let kind: String
    let reason: String
    let sourceLine: Int

    /// Deleted instructions have no "after" text; the UI renders them struck through.
    var isDeletion: Bool { after.isEmpty }
}

struct TracePass: Codable, Identifiable, Hashable {
    var id: String { "\(pass)-\(instrBefore)-\(instrAfter)-\(transforms)-\(edits.count)" }
    let pass: String
    let title: String
    let description: String
    let precondition: String
    let transforms: Int
    let instrBefore: Int
    let instrAfter: Int
    let changed: Bool
    let irBefore: [String]
    let irAfter: [String]
    let edits: [TraceEdit]

    var instructionDelta: Int { instrAfter - instrBefore }
}

struct TraceSweep: Codable, Identifiable, Hashable {
    var id: Int { sweep }
    let sweep: Int
    let passes: [TracePass]

    /// A sweep in which nothing fired is the proof the fixed point was reached.
    var changedAnything: Bool { passes.contains { $0.changed } }
}

struct FunctionTrace: Codable, Identifiable, Hashable {
    var id: String { name }
    let name: String
    let sweepCount: Int
    let fixedPointReached: Bool
    let irInitial: [String]
    let irFinal: [String]
    let sweeps: [TraceSweep]
}

struct RunResult: Codable, Hashable {
    let exitCode: Int
    let output: String
}

struct RunReport: Codable, Hashable {
    let unoptimized: RunResult
    let optimized: RunResult
    let outputsMatch: Bool
}

struct Metrics: Codable, Hashable {
    let staticBefore: Int
    let staticAfter: Int
    let dynamicBefore: Int
    let dynamicAfter: Int
    let perPass: [String: Int]

    /// Guarded against division by zero so an empty program reports 0%, not NaN.
    var staticReduction: Double {
        staticBefore == 0 ? 0 : Double(staticBefore - staticAfter) / Double(staticBefore) * 100
    }
    var dynamicReduction: Double {
        dynamicBefore == 0 ? 0 : Double(dynamicBefore - dynamicAfter) / Double(dynamicBefore) * 100
    }
}

struct CompileResult: Codable {
    let ok: Bool
    let diagnostics: [Diagnostic]
    let source: String?
    let trace: [FunctionTrace]?
    let metrics: Metrics?
    let run: RunReport?
    /// Present only when the compiler could not open the file at all.
    let error: String?
}

/// What a production compiler does with the same source, measured by actually
/// invoking clang rather than quoting numbers. Absent when clang is missing or
/// the program is not valid standalone C.
struct ClangComparison: Hashable {
    let o0Instructions: Int
    let o2Instructions: Int
    /// Exit status of the clang-built binary, or -1 if it could not be run.
    /// Note this is truncated mod 256 by the OS, which the panel explains
    /// rather than hiding — a program returning 520 exits with 8.
    let exitCode: Int32
}
