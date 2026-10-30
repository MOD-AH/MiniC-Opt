import SwiftUI

/// `Cppify --render-snapshots <trace.json> <out-dir>`: renders the dock's
/// panels offscreen, from a real compiler trace, to PNG files, then exits.
/// It exists so the panels can be checked by eye (and shown in the report)
/// without opening a window or driving the app.
@MainActor
enum SnapshotRenderer {
    static func runIfRequested() {
        let args = CommandLine.arguments
        guard let i = args.firstIndex(of: "--render-snapshots"), i + 2 < args.count else { return }
        let trace = URL(fileURLWithPath: args[i + 1]), out = URL(fileURLWithPath: args[i + 2])
        do {
            let data = try Data(contentsOf: trace)
            let result = try JSONDecoder().decode(CompileResult.self, from: data)
            let model = AppModel()
            model.result = result
            if let s = result.source { model.source = s }
            model.state = AppModel.outcome(of: result, subsetIssues: false)
            if args.contains("--optimized") { model.pipeline.apply(preset: "optimized") }
            model.techniques = (try? JSONDecoder().decode([Technique].self,
                from: Data(contentsOf: trace.deletingLastPathComponent().appendingPathComponent("registry.json")))) ?? []
            try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
            func save<V: View>(_ name: String, _ view: V, height: CGFloat = 430) {
                let r = ImageRenderer(content: view.frame(width: 1250, height: height)
                    .environment(\.colorScheme, .dark).environment(\.snapshotting, true))
                r.scale = 2
                guard let img = r.nsImage, let tiff = img.tiffRepresentation,
                      let png = NSBitmapImageRep(data: tiff)?.representation(using: .png, properties: [:]) else {
                    print("could not render \(name)"); return
                }
                try? png.write(to: out.appendingPathComponent("\(name).png"))
                print("wrote \(name).png")
            }
            save("pipeline", PipelinePanel(model: model))
            save("problems", ProblemsPanel(model: model), height: 240)
            save("output", OutputPanel(model: model), height: 240)
            for tab in ViewTab.allCases where tab != .benchmark {
                model.viewTab = tab
                save("view-" + tab.rawValue.lowercased().replacingOccurrences(of: " ", with: "-"), ViewsPanel(model: model), height: 520)
            }
            model.bench = BenchResult(runs: 9, compileMs: 0.23, quadMs: 63.7, vmMs: 16.6, astMs: 41.6, agree: true)
            model.viewTab = .benchmark
            save("view-benchmark", ViewsPanel(model: model), height: 260)
        } catch {
            print("snapshot failed: \(error)")
        }
        exit(0)
    }
}

private struct SnapshottingKey: EnvironmentKey { static let defaultValue = false }
extension EnvironmentValues {
    /// True while SnapshotRenderer draws a panel offscreen.
    var snapshotting: Bool {
        get { self[SnapshottingKey.self] }
        set { self[SnapshottingKey.self] = newValue }
    }
}

/// A ScrollView in the app; a plain stack in a snapshot, because
/// ImageRenderer cannot draw a scroll view's (AppKit-backed) contents.
struct AdaptiveScroll<Content: View>: View {
    @Environment(\.snapshotting) private var snapshotting
    var axes: Axis.Set = .vertical
    var showsIndicators = true
    @ViewBuilder var content: Content

    var body: some View {
        if snapshotting {
            VStack(alignment: .leading, spacing: 0) { content }
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
                .clipped()
        } else {
            ScrollView(axes, showsIndicators: showsIndicators) { content }
        }
    }
}
