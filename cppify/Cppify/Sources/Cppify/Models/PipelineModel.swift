import Foundation

/// One entry of the compiler's technique registry (`minic --list-techniques`,
/// roadmap 1.3), the single list of what each stage can use.
struct Technique: Codable, Hashable, Identifiable {
    let id: String
    let name: String
    let stage: Int
    let kind: String
    let status: String
    let requires: [String]
    let conflictsWith: [String]
    let description: String
    let test: String

    var available: Bool { status == "Available" }

    static let stageNames = ["", "Lexer", "Parser", "Semantic", "IR", "Analysis", "Optimizer", "Execution"]
}

/// One group of the optimization pipeline: passes swept in order, to a fixed
/// point or a fixed number of times. A group of SSA passes is wrapped in SSA
/// construction and destruction by the compiler.
struct PassGroup: Codable, Hashable, Identifiable {
    var id = UUID()
    var passes: [String]
    var fixpoint = true
    var sweeps = 1

    enum CodingKeys: String, CodingKey { case passes, fixpoint, sweeps }
}

/// Everything the Pipeline tab chooses (roadmap 5.1): one implementation per
/// stage, and the ordered passes.
struct PipelineSettings: Hashable {
    var lexer = "re2c"            // the compiler's default (roadmap 4.3)
    var parser = "handwritten"
    var engine = "quad"           // whose output Output shows first; all engines run
    var preset = "classic"        // correctness, balanced, classic, optimized, custom
    var groups: [PassGroup] = PipelineSettings.groups(for: "classic")

    static let presets = ["correctness", "balanced", "classic", "optimized"]
    static let tacOrder = ["fold", "constprop", "copyprop", "cse", "dce", "unreachable", "strength", "licm", "peephole"]
    static let ssaOrder = ["sccp", "gvn", "licm-ssa", "ssa-dce"]

    /// What the compiler starts from when nothing is chosen: the original
    /// P1-P9 pipeline on the default front end and engine.
    static let safeDefaults = PipelineSettings()

    /// The same groups the compiler's presets resolve to (src/opt/pipeline.cpp).
    static func groups(for preset: String) -> [PassGroup] {
        switch preset {
        case "correctness": return []
        case "balanced":    return [PassGroup(passes: ["fold", "constprop", "copyprop", "dce", "unreachable", "peephole"])]
        case "optimized":   return [PassGroup(passes: ssaOrder), PassGroup(passes: tacOrder.filter { $0 != "strength" })]
        default:            return [PassGroup(passes: tacOrder)]
        }
    }

    static func isSSA(_ pass: String) -> Bool { ssaOrder.contains(pass) }

    mutating func apply(preset name: String) {
        preset = name
        groups = PipelineSettings.groups(for: name)
    }

    /// Any edit to the groups makes the pipeline custom.
    mutating func edited() { preset = "custom" }

    /// Problems the compiler would reject, said before Run.
    var problems: [String] {
        var out: [String] = []
        for (i, g) in groups.enumerated() {
            if g.passes.isEmpty { out.append("Group \(i + 1) has no passes.") }
            let ssa = g.passes.filter(PipelineSettings.isSSA), tac = g.passes.filter { !PipelineSettings.isSSA($0) }
            if !ssa.isEmpty && !tac.isEmpty {
                out.append("Group \(i + 1) mixes SSA passes (\(ssa[0])) with plain-TAC passes (\(tac[0])).")
            }
        }
        return out
    }

    /// The --config file for a custom pipeline (roadmap 1.4).
    func configJSON() -> String {
        let gs = groups.map { g -> String in
            let ps = g.passes.map { "\"\($0)\"" }.joined(separator: ", ")
            return "{\"passes\": [\(ps)], \"repeat\": " + (g.fixpoint ? "\"fixpoint\"" : "\(max(1, min(32, g.sweeps)))") + "}"
        }
        return "{\"groups\": [" + gs.joined(separator: ", ") + "]}"
    }

    /// Analyses the chosen passes need, with what needs them: the registry's
    /// `requires`, followed transitively (prerequisites are added, never chosen).
    func requiredAnalyses(_ registry: [Technique]) -> [(technique: Technique, neededBy: [String])] {
        let byID = Dictionary(registry.map { ($0.id, $0) }, uniquingKeysWith: { a, _ in a })
        var neededBy: [String: Set<String>] = [:]
        var work: [(String, String)] = []
        for g in groups { for p in g.passes { for r in byID[p]?.requires ?? [] { work.append((r, p)) } } }
        while let (req, by) = work.popLast() {
            if neededBy[req, default: []].insert(by).inserted {
                for r in byID[req]?.requires ?? [] { work.append((r, by)) }
            }
        }
        return neededBy.keys.sorted().compactMap { id in
            guard let t = byID[id], t.kind == "analysis" || t.kind == "ir" else { return nil }
            return (t, neededBy[id]!.sorted())
        }
    }
}
