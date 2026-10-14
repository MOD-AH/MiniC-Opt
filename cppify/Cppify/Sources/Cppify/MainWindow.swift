import SwiftUI

/// Xcode's hierarchy, because that is what anyone opening a compiler IDE
/// expects: inspector right, editor centre, dock bottom.
///
/// Liquid Glass is confined to the toolbar, the inspector and the dock's
/// frame. The editor and every console surface are opaque — blurred colour
/// moving underneath syntax highlighting is exactly the thing that makes
/// code hard to read, and readability is the one thing this UI cannot trade.
struct MainWindow: View {
    @Bindable var model: AppModel

    var body: some View {
        ZStack {
            // Lowest layer: the fluid mesh, tinted by compiler state. It
            // lives behind everything and never under text.
            BackgroundMesh(state: model.state)
                .ignoresSafeArea()

            VStack(spacing: 10) {
                Toolbar(model: model)

                // VSplitView rather than a fixed-height VStack: the dock holds
                // the Processing panel's three columns, which need far more
                // room than the editor on a wide program. A native draggable
                // divider lets the dock take most of the window when reading a
                // trace and shrink back out of the way when writing code.
                VSplitView {
                    HSplitView {
                        VStack(spacing: 0) {
                            EditorHeader(model: model)
                            CodeEditor(text: $model.source,
                                       errorLines: model.errorLines,
                                       highlightedLine: model.highlightedLine)
                            InputPanel(model: model)
                        }
                        .opaqueSurface()
                        .frame(minWidth: 460)

                        Inspector(model: model)
                            .frame(minWidth: 250, idealWidth: 290, maxWidth: 360)
                    }
                    .frame(minHeight: 160)

                    Dock(model: model)
                        .frame(minHeight: 120, idealHeight: 300)
                }
            }
            .padding(12)
        }
        .background(Color.black)
        .background(TitlebarWordmark())
    }
}

// MARK: - Toolbar

private struct Toolbar: View {
    @Bindable var model: AppModel

    var body: some View {
        HStack(spacing: 12) {
            Button {
                Task { await model.compileAndRun() }
            } label: {
                Label("Run", systemImage: "play.fill")
                    .padding(.horizontal, 14).padding(.vertical, 7)
            }
            .buttonStyle(.plain)
            .glassControl(tint: Palette.insertGreen)
            .disabled(model.state == .compiling)
            .help("Compile, optimize and interpret (⌘R)")

            Button {
                model.optimizationOn.toggle()
            } label: {
                Label(model.optimizationOn ? "Optimization On" : "Optimization Off",
                      systemImage: model.optimizationOn ? "bolt.fill" : "bolt.slash")
                    .padding(.horizontal, 14).padding(.vertical, 7)
            }
            .buttonStyle(.plain)
            .glassControl(tint: model.optimizationOn ? Palette.type : nil)
            .help("Equivalent to --no-opt when off")

            if model.runStartedAt != nil {
                RunTimerPill(model: model)
                    .transition(.scale(scale: 0.6, anchor: .leading).combined(with: .opacity))
            }

            Spacer()

            if let m = model.result?.metrics, model.state == .succeeded {
                MetricPill(label: "static", before: m.staticBefore, after: m.staticAfter)
                MetricPill(label: "dynamic", before: m.dynamicBefore, after: m.dynamicAfter)
            }

            StatePill(state: model.state)
        }
        .padding(8)
        .liquidGlass(tint: model.state.tint)
        .animation(.easeInOut(duration: 0.4), value: model.state)
        .animation(.spring(response: 0.35, dampingFraction: 0.75), value: model.runStartedAt)
    }
}

/// Pops up beside the Optimization button on Run. While the program runs it
/// counts down the real time limit (CompilerService.runTimeLimit, the same
/// number that stops the process); afterwards it shows the measured times.
private struct RunTimerPill: View {
    @Bindable var model: AppModel
    @State private var showDetail = false

    private var limit: Double { CompilerService.runTimeLimit }

    var body: some View {
        Group {
            if model.state == .compiling, let start = model.runStartedAt {
                // Redrawn every 50 ms only while running; idle costs nothing.
                TimelineView(.periodic(from: start, by: 0.05)) { ctx in
                    let remaining = max(0, limit - ctx.date.timeIntervalSince(start))
                    countdown(remaining: remaining)
                }
            } else {
                finished
            }
        }
        .font(.system(size: 11, design: .monospaced))
        .padding(.horizontal, 10).padding(.vertical, 6)
        .glassControl(tint: tint)
        .contentShape(Rectangle())
        .onHover { showDetail = $0 }
        .popover(isPresented: $showDetail, arrowEdge: .bottom) { detail }
        .animation(.easeInOut(duration: 0.3), value: model.state)
    }

    private var tint: Color? {
        if model.timedOut { return Palette.errorRed }
        if model.state == .compiling { return CompilerState.compiling.tint }
        return nil
    }

    // Green while there is plenty of time, amber in the last 3 s, red in the last 1 s.
    private func urgency(_ remaining: Double) -> Color {
        remaining > 3 ? Palette.insertGreen : remaining > 1 ? Palette.changeAmber : Palette.errorRed
    }

    private func countdown(remaining: Double) -> some View {
        HStack(spacing: 7) {
            ZStack {
                Circle().stroke(Color.white.opacity(0.12), lineWidth: 2.5)
                Circle()
                    .trim(from: 0, to: remaining / limit)
                    .stroke(urgency(remaining), style: StrokeStyle(lineWidth: 2.5, lineCap: .round))
                    .rotationEffect(.degrees(-90))
            }
            .frame(width: 14, height: 14)
            Text(String(format: "%4.1fs", remaining))
                .foregroundStyle(urgency(remaining))
                .fontWeight(.semibold)
                .contentTransition(.numericText(countsDown: true))
            Text("left").foregroundStyle(Palette.textDim)
        }
    }

    @ViewBuilder
    private var finished: some View {
        HStack(spacing: 6) {
            if model.timedOut {
                Image(systemName: "hourglass.bottomhalf.filled").foregroundStyle(Palette.errorRed)
                Text("timed out at \(Int(limit))s").foregroundStyle(Palette.errorRed).fontWeight(.semibold)
            } else {
                Image(systemName: "timer").foregroundStyle(Palette.textDim)
                Text(Self.format(model.compileSeconds))
                    .foregroundStyle(Palette.insertGreen).fontWeight(.semibold)
                if model.comparing {
                    ProgressView().controlSize(.mini)
                    Text("clang").foregroundStyle(Palette.textDim)
                } else if let c = model.compareSeconds {
                    Text("·").foregroundStyle(Palette.textDim)
                    Text("clang \(Self.format(c))").foregroundStyle(Palette.textSecondary)
                }
            }
        }
    }

    private var detail: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Last run").font(.system(size: 12, weight: .semibold))
            row("MiniC: compile, optimize, run twice", model.compileSeconds)
            row("clang -O0 / -O2 + native run (in parallel)", model.compareSeconds)
            Divider()
            Text("A program is stopped after \(Int(limit)) s. The countdown shows how much of that is left while it runs.")
                .font(.system(size: 11))
                .foregroundStyle(Palette.textSecondary)
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(14)
        .frame(width: 300)
    }

    private func row(_ title: String, _ secs: Double?) -> some View {
        HStack {
            Text(title).font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
            Spacer()
            Text(secs.map(Self.format) ?? "—").font(.system(size: 11, design: .monospaced))
        }
    }

    /// Milliseconds below one second, seconds with two decimals above it.
    static func format(_ s: Double?) -> String {
        guard let s else { return "—" }
        return s < 1 ? String(format: "%.0f ms", s * 1000) : String(format: "%.2f s", s)
    }
}

private struct StatePill: View {
    let state: CompilerState
    var body: some View {
        HStack(spacing: 6) {
            Image(systemName: state.symbol)
                .foregroundStyle(state.tint)
                .symbolEffect(.pulse, isActive: state == .compiling)
            Text(state.label).font(.system(size: 12, weight: .medium))
        }
        .padding(.horizontal, 12).padding(.vertical, 7)
        .glassControl(tint: state.tint)
    }
}

private struct MetricPill: View {
    let label: String, before: Int, after: Int
    var body: some View {
        HStack(spacing: 5) {
            Text(label).foregroundStyle(Palette.textDim)
            Text("\(before)").foregroundStyle(Palette.textSecondary)
            Image(systemName: "arrow.right").font(.system(size: 8)).foregroundStyle(Palette.textDim)
            Text("\(after)").foregroundStyle(Palette.insertGreen).fontWeight(.semibold)
        }
        .font(.system(size: 11, design: .monospaced))
        .padding(.horizontal, 10).padding(.vertical, 6)
        .glassControl()
    }
}

// MARK: - Editor header

private struct EditorHeader: View {
    @Bindable var model: AppModel
    var body: some View {
        HStack(spacing: 8) {
            Image(systemName: "doc.text").font(.system(size: 11)).foregroundStyle(Palette.textDim)
            Text(model.documentURL?.lastPathComponent ?? "untitled.c")
                .font(.system(size: 12, weight: .medium))
                .foregroundStyle(Palette.textSecondary)
            if !model.diagnostics.isEmpty {
                Text("\(model.diagnostics.count) error\(model.diagnostics.count == 1 ? "" : "s")")
                    .font(.system(size: 11))
                    .foregroundStyle(Palette.errorRed)
                    .padding(.horizontal, 7).padding(.vertical, 2)
                    .background(Palette.errorRed.opacity(0.14), in: Capsule())
            }
            // The single most useful thing to show someone whose paste just
            // produced 26 unexpected-character errors.
            let issues = model.subsetIssues
            if !issues.isEmpty {
                Button { model.selectedTab = .language } label: {
                    HStack(spacing: 4) {
                        Image(systemName: "exclamationmark.triangle.fill").font(.system(size: 9))
                        Text("\(issues.count) outside MiniC — see Language")
                            .font(.system(size: 11, weight: .medium))
                    }
                    .foregroundStyle(Palette.changeAmber)
                    .padding(.horizontal, 7).padding(.vertical, 2)
                    .background(Palette.changeAmber.opacity(0.16), in: Capsule())
                }
                .buttonStyle(.plain)
            }
            Spacer()
        }
        .padding(.horizontal, 12).padding(.vertical, 8)
        .background(Palette.consoleBackground)
    }
}

// MARK: - Inspector

private struct Inspector: View {
    @Bindable var model: AppModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 14) {
                Text("OPTIMIZATION PASSES")
                    .font(.system(size: 10, weight: .semibold)).tracking(0.8)
                    .foregroundStyle(Palette.textDim)

                ForEach(Pass.all) { pass in
                    PassToggle(pass: pass,
                               isOn: Binding(
                                get: { model.enabledPasses.contains(pass.id) },
                                set: { on in
                                    if on { model.enabledPasses.insert(pass.id) }
                                    else { model.enabledPasses.remove(pass.id) }
                                }),
                               transforms: model.result?.metrics?.perPass[pass.id],
                               enabled: model.optimizationOn)
                }

                Divider().overlay(Palette.panelBorder)

                HStack {
                    Button("All") { model.enabledPasses = Set(Pass.all.map(\.id)) }
                    Button("None") { model.enabledPasses = [] }
                }
                .buttonStyle(.link)
                .font(.system(size: 11))
            }
            .padding(14)
        }
        .liquidGlass(tint: model.state.tint)
    }
}

private struct PassToggle: View {
    let pass: Pass
    @Binding var isOn: Bool
    let transforms: Int?
    let enabled: Bool

    var body: some View {
        HStack(alignment: .top, spacing: 8) {
            Toggle("", isOn: $isOn).labelsHidden().controlSize(.mini).disabled(!enabled)
            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: 5) {
                    Text(pass.number)
                        .font(.system(size: 9, weight: .bold, design: .monospaced))
                        .foregroundStyle(Palette.textDim)
                    Text(pass.title)
                        .font(.system(size: 12, weight: .medium))
                        .foregroundStyle(enabled && isOn ? Palette.textPrimary : Palette.textDim)
                    if let t = transforms, t > 0 {
                        Text("\(t)")
                            .font(.system(size: 10, weight: .semibold, design: .monospaced))
                            .foregroundStyle(Palette.insertGreen)
                            .padding(.horizontal, 5).padding(.vertical, 1)
                            .background(Palette.insertGreen.opacity(0.15), in: Capsule())
                    }
                }
                Text(pass.id)
                    .font(.system(size: 10, design: .monospaced))
                    .foregroundStyle(Palette.textDim)
            }
            Spacer(minLength: 0)
        }
        .help(pass.what)
    }
}

// MARK: - Dock

private struct Dock: View {
    @Bindable var model: AppModel

    var body: some View {
        VStack(spacing: 0) {
            HStack(spacing: 4) {
                ForEach(DockTab.allCases) { tab in
                    Button {
                        model.selectedTab = tab
                    } label: {
                        HStack(spacing: 5) {
                            Image(systemName: tab.symbol).font(.system(size: 10))
                            Text(tab.rawValue).font(.system(size: 12, weight: .medium))
                            if tab == .problems, !model.diagnostics.isEmpty {
                                Text("\(model.diagnostics.count)")
                                    .font(.system(size: 10, weight: .bold))
                                    .padding(.horizontal, 5).padding(.vertical, 1)
                                    .background(Palette.errorRed.opacity(0.25), in: Capsule())
                            }
                        }
                        .padding(.horizontal, 12).padding(.vertical, 6)
                    }
                    .buttonStyle(.plain)
                    .glassControl(tint: model.selectedTab == tab ? model.state.tint : nil,
                                  cornerRadius: 7)
                    .foregroundStyle(model.selectedTab == tab ? Palette.textPrimary : Palette.textSecondary)
                }
                Spacer()
            }
            .padding(8)

            Group {
                switch model.selectedTab {
                case .output:     OutputPanel(model: model)
                case .problems:   ProblemsPanel(model: model)
                case .processing: ProcessingPanel(model: model)
                case .language:   LanguagePanel(model: model)
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .liquidGlass(tint: model.state.tint)
    }
}
