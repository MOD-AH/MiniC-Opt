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
    // Roadmap 5.4. Optional so an older compiler's output still decodes.
    let code: String?        // stable, e.g. "CPPIFY-PARSE-002"
    let title: String?       // the code's category
    let unsupported: Bool?   // C that Cppify deliberately does not implement
    let primary: Bool?       // false: may only be a consequence of an earlier error

    var isPrimary: Bool { primary ?? true }
}

// ---- views (roadmap 5.2) ----------------------------------------------------

struct TokenInfo: Codable, Hashable {
    let type: String
    let lexeme: String
    let line: Int
    let col: Int
}

struct BlockInfo: Codable, Hashable, Identifiable {
    let id: Int
    let lines: [String]
    let succs: [Int]
}

struct FunctionCFG: Codable, Hashable, Identifiable {
    var id: String { name }
    let name: String
    let blocks: [BlockInfo]
}

struct CFGSet: Codable, Hashable {
    let unoptimized: [FunctionCFG]
    let optimized: [FunctionCFG]
}

struct FunctionLines: Codable, Hashable, Identifiable {
    var id: String { name }
    let name: String
    let lines: [String]
}

struct EngineResult: Codable, Hashable {
    let exitCode: Int
    let output: String
    let ms: Double
}

/// The AST reference interpreter and the bytecode VM (roadmap 3.1, 3.2).
struct EngineReport: Codable, Hashable {
    let reference: EngineResult
    let vm: EngineResult
    let allAgree: Bool
}

struct FrontEndInfo: Codable, Hashable {
    let lexer: String
    let parser: String
}

/// `minic --bench-engines=N`: medians of N in-process runs.
struct BenchResult: Codable, Hashable {
    let runs: Int
    let compileMs: Double
    let quadMs: Double
    let vmMs: Double
    let astMs: Double
    let agree: Bool
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

/// The pipeline that actually ran, as the compiler resolved it.
struct PipelineGroupInfo: Codable, Hashable {
    let passes: [String]
    let fixpoint: Bool
    let sweeps: Int
    let ssa: Bool
    let order: String        // e.g. "ssa -> [sccp, gvn] to a fixed point ... -> out-of-ssa"
}

struct PipelineInfo: Codable, Hashable {
    let preset: String
    let groups: [PipelineGroupInfo]
}

/// How often an analysis was computed, and how often the cache served it.
struct AnalysisCount: Codable, Hashable {
    let computed: Int
    let reused: Int
}

struct CompileResult: Codable {
    let ok: Bool
    let diagnostics: [Diagnostic]
    let source: String?
    let trace: [FunctionTrace]?
    let metrics: Metrics?
    let run: RunReport?
    /// Absent on a failed compile, and from compilers older than roadmap 1.5.
    let pipeline: PipelineInfo?
    // Roadmap 5.2 views. Tokens are present even when the build fails; the
    // AST once the program parses; the rest once it compiles.
    let tokens: [TokenInfo]?
    let ast: String?
    let frontEnd: FrontEndInfo?
    let engines: EngineReport?
    let cfg: CFGSet?
    let ssa: [FunctionLines]?
    let timingsMs: [String: Double]?
    let analyses: [String: AnalysisCount]?
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
