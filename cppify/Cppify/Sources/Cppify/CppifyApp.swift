import SwiftUI

@main
struct CppifyApp: App {
    @State private var model = AppModel()

    var body: some Scene {
        WindowGroup("Cppify — MiniC Optimizing Compiler") {
            MainWindow(model: model)
                .frame(minWidth: 1100, minHeight: 700)
                .preferredColorScheme(.dark)
        }
        .windowStyle(.hiddenTitleBar)
        .commands {
            CommandGroup(replacing: .newItem) {
                Button("New") { model.newDocument() }.keyboardShortcut("n")
                Button("Open…") { model.openDocument() }.keyboardShortcut("o")
                Button("Save") { model.saveDocument() }.keyboardShortcut("s")
            }
            CommandMenu("Samples") {
                ForEach(model.samples) { sample in
                    Button(sample.name) { model.load(sample) }
                }
            }
            CommandMenu("Build") {
                Button("Run") { Task { await model.compileAndRun() } }
                    .keyboardShortcut("r")
                    .disabled(model.state == .compiling)
            }
        }
    }
}

@Observable
@MainActor
final class AppModel {
    var source: String = AppModel.starterProgram
    /// Supplied to the program's stdin, one value per line, for read_int().
    var programInput: String = ""
    var documentURL: URL?
    var state: CompilerState = .idle

    var result: CompileResult?
    var clang: ClangComparison?
    var serviceError: String?

    // Timing of the latest Run, shown by the toolbar timer. Measured, never
    // estimated: each is a wall-clock interval around the real process.
    var runStartedAt: Date?
    var compileSeconds: Double?          // MiniC: compile + optimize + both interpreted runs
    var compareSeconds: Double?          // clang -O0/-O2 and the native run
    var comparing = false
    var timedOut = false

    var enabledPasses: Set<String> = Set(Pass.all.map(\.id))
    var optimizationOn = true

    var selectedTab: DockTab = .processing
    /// Which (sweep, pass) the Processing panel is showing.
    var selection: PassSelection?
    /// Line the editor should scroll to and mark, set by clicking an edit.
    var highlightedLine: Int?

    private let service = CompilerService()

    var diagnostics: [Diagnostic] { result?.diagnostics ?? [] }

    /// Constructs in the current source that MiniC does not have. Computed
    /// from the text, so it is available before a compile and explains the
    /// cryptic lexer errors after one.
    var subsetIssues: [SubsetIssue] { SubsetAdvisor.analyse(source) }
    var needsInput: Bool { SubsetAdvisor.usesInput(source) }
    var errorLines: Set<Int> { Set(diagnostics.map(\.line)) }

    /// The function the Processing panel shows. MiniC programs in this
    /// project are small, so the first function with any optimization
    /// activity is the useful default rather than making the user pick.
    var focusedFunction: FunctionTrace? {
        guard let trace = result?.trace else { return nil }
        return trace.first { $0.sweeps.contains { $0.changedAnything } } ?? trace.first
    }

    func compileAndRun() async {
        state = .compiling
        serviceError = nil
        clang = nil
        selection = nil
        highlightedLine = nil
        compileSeconds = nil
        compareSeconds = nil
        timedOut = false
        let started = Date()
        runStartedAt = started

        // The clang comparison does not depend on MiniC's result, so it starts
        // now and overlaps the compile instead of waiting behind it.
        let snapshot = source
        comparing = true
        let comparison = Task { () -> (ClangComparison?, Double) in
            let t0 = Date()
            let c = await service.compareWithClang(source: snapshot)
            return (c, Date().timeIntervalSince(t0))
        }

        do {
            let r = try await service.compile(source: source,
                                              stdin: programInput,
                                              passes: enabledPasses,
                                              optimize: optimizationOn)
            compileSeconds = Date().timeIntervalSince(started)
            result = r
            state = r.ok ? .succeeded : .failed
            if !r.ok { selectedTab = .problems }
            if r.ok {
                // Pre-select the first pass that actually did something, so
                // the panel opens on content rather than an empty state.
                if let fn = focusedFunction {
                    outer: for sweep in fn.sweeps {
                        for pass in sweep.passes where pass.changed {
                            selection = PassSelection(sweep: sweep.sweep, passID: pass.id)
                            break outer
                        }
                    }
                }
                let (c, secs) = await comparison.value
                // Only keep it if no newer Run has started meanwhile.
                if runStartedAt == started { clang = c; compareSeconds = secs }
            } else {
                comparison.cancel()
            }
        } catch {
            compileSeconds = Date().timeIntervalSince(started)
            if case CompilerService.ServiceError.timedOut = error { timedOut = true }
            serviceError = error.localizedDescription
            state = .failed
            selectedTab = .problems
            comparison.cancel()
        }
        if runStartedAt == started { comparing = false }
    }

    /// The project's own benchmark programs, bundled so there is always a
    /// working MiniC program to hand. Hand-translating C from the internet is
    /// the slow path; this is the fast one.
    struct Sample: Identifiable, Hashable {
        var id: String { name }
        let name: String
        let url: URL
    }

    var samples: [Sample] {
        guard let dir = Bundle.module.url(forResource: "samples", withExtension: nil),
              let files = try? FileManager.default.contentsOfDirectory(at: dir,
                                                                       includingPropertiesForKeys: nil)
        else { return [] }
        return files.filter { $0.pathExtension == "c" }
            .map { Sample(name: $0.deletingPathExtension().lastPathComponent, url: $0) }
            .sorted { $0.name < $1.name }
    }

    func load(_ sample: Sample) {
        guard let text = try? String(contentsOf: sample.url, encoding: .utf8) else { return }
        source = text
        documentURL = nil
        result = nil; clang = nil; state = .idle
        // Interactive samples need input; seed something sensible so Run works
        // immediately rather than failing on end-of-input.
        programInput = SubsetAdvisor.usesInput(text) ? "7\n1\n3\n5\n7\n9\n11\n13\n11\n" : ""
    }

    func newDocument() {
        source = AppModel.starterProgram
        documentURL = nil
        result = nil; clang = nil; state = .idle
    }

    func openDocument() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.cSource]
        panel.allowsMultipleSelection = false
        guard panel.runModal() == .OK, let url = panel.url,
              let text = try? String(contentsOf: url, encoding: .utf8) else { return }
        source = text
        documentURL = url
        result = nil; clang = nil; state = .idle
    }

    func saveDocument() {
        if let url = documentURL {
            try? source.write(to: url, atomically: true, encoding: .utf8)
            return
        }
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.cSource]
        panel.nameFieldStringValue = "program.c"
        guard panel.runModal() == .OK, let url = panel.url else { return }
        try? source.write(to: url, atomically: true, encoding: .utf8)
        documentURL = url
    }

    static let starterProgram = """
    /* The report's worked optimization example.
       Every pass has something to do here: constants to propagate and fold,
       a loop-invariant multiply, a redundant repeat of it, and dead stores
       left behind once the constants are resolved. Press Run. */
    int main() {
        int i, n, s, t, a, b;
        a = 4;  b = 5;  n = 100;  s = 0;
        i = 0;
        while (i < n) {
            t = a * b;
            s = s + t;
            s = s + a * b;
            i = i + 1 * 8;
        }
        return s;
    }
    """
}

struct PassSelection: Equatable, Hashable {
    let sweep: Int
    let passID: String
}

enum DockTab: String, CaseIterable, Identifiable {
    case output = "Output"
    case problems = "Problems"
    case processing = "Processing"
    case language = "Language"
    var id: String { rawValue }

    var symbol: String {
        switch self {
        case .output:     return "terminal"
        case .problems:   return "exclamationmark.triangle"
        case .processing: return "wand.and.rays"
        case .language:   return "character.book.closed"
        }
    }
}

import AppKit
