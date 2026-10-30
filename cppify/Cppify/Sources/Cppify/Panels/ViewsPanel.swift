import SwiftUI

/// Roadmap 5.2: what each compiler stage produced for this program.
struct ViewsPanel: View {
    @Bindable var model: AppModel

    var body: some View {
        VStack(spacing: 0) {
            Picker("", selection: $model.viewTab) {
                ForEach(ViewTab.allCases) { Text($0.rawValue).tag($0) }
            }
            .pickerStyle(.segmented)
            .labelsHidden()
            .padding(8)

            Group {
                switch model.viewTab {
                case .tokens:    TokensView(model: model)
                case .ast:       TextView(text: model.result?.ast,
                                          empty: "The AST appears once the program parses.")
                case .cfg:       CFGView(model: model)
                case .ssa:       SSAView(model: model)
                case .compare:   CompareView(model: model)
                case .benchmark: BenchmarkView(model: model)
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .background(Palette.consoleBackground)
    }
}

/// Shared empty state.
private func placeholder(_ text: String) -> some View {
    Text(text).font(.system(size: 11)).foregroundStyle(Palette.textDim)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
}

// MARK: - Tokens

private struct TokensView: View {
    @Bindable var model: AppModel

    var body: some View {
        if let toks = model.result?.tokens, !toks.isEmpty {
            AdaptiveScroll {
                LazyVStack(alignment: .leading, spacing: 0) {
                    ForEach(Array(toks.enumerated()), id: \.offset) { i, t in
                        Button { model.highlightedLine = t.line } label: {
                            HStack(spacing: 10) {
                                Text("\(i + 1)").frame(width: 44, alignment: .trailing).foregroundStyle(Palette.textDim)
                                Text("\(t.line):\(t.col)").frame(width: 64, alignment: .leading).foregroundStyle(Palette.textDim)
                                Text(t.type).frame(width: 150, alignment: .leading)
                                    .foregroundStyle(t.type == "ERROR" ? Palette.errorRed
                                                     : t.type.hasPrefix("KW_") ? Palette.keyword
                                                     : t.type.hasPrefix("OP_") ? Palette.op : Palette.type)
                                Text(t.lexeme).foregroundStyle(Palette.textPrimary).lineLimit(1)
                                Spacer(minLength: 0)
                            }
                            .font(.system(size: 11, design: .monospaced))
                            .padding(.horizontal, 12).padding(.vertical, 2)
                            .contentShape(Rectangle())
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.vertical, 6)
            }
        } else {
            placeholder("Press Run: the token stream appears here, even when the build fails.")
        }
    }
}

private struct TextView: View {
    let text: String?
    let empty: String

    var body: some View {
        if let text, !text.isEmpty {
            AdaptiveScroll(axes: [.vertical, .horizontal]) {
                Text(text)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(Palette.textPrimary)
                    .textSelection(.enabled)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(12)
            }
        } else {
            placeholder(empty)
        }
    }
}

// MARK: - CFG

private struct CFGView: View {
    @Bindable var model: AppModel
    @State private var functionName = ""
    @State private var optimized = true

    var body: some View {
        if let set = model.result?.cfg {
            let fns = optimized ? set.optimized : set.unoptimized
            let fn = fns.first { $0.name == functionName } ?? fns.first { $0.name == "main" } ?? fns.first
            VStack(spacing: 0) {
                HStack {
                    Picker("Function", selection: Binding(get: { fn?.name ?? "" }, set: { functionName = $0 })) {
                        ForEach(fns) { Text($0.name).tag($0.name) }
                    }
                    .frame(width: 220)
                    Picker("", selection: $optimized) {
                        Text("Unoptimized").tag(false)
                        Text("Optimized").tag(true)
                    }
                    .pickerStyle(.segmented)
                    .labelsHidden()
                    .frame(width: 220)
                    Spacer()
                    legend
                }
                .font(.system(size: 11))
                .padding(.horizontal, 10)
                if let fn {
                    AdaptiveScroll(axes: [.horizontal, .vertical]) { CFGGraph(function: fn).padding(16) }
                }
            }
        } else {
            placeholder("The control-flow graph appears once the program compiles.")
        }
    }

    private var legend: some View {
        HStack(spacing: 10) {
            swatch(Palette.insertGreen, "condition true")
            swatch(Palette.deleteRed, "condition false")
            swatch(Palette.textDim, "jump / fall through")
            swatch(Palette.changeAmber, "back edge (loop)")
        }
        .font(.system(size: 10))
        .foregroundStyle(Palette.textSecondary)
    }

    private func swatch(_ c: Color, _ t: String) -> some View {
        HStack(spacing: 4) { RoundedRectangle(cornerRadius: 1).fill(c).frame(width: 12, height: 3); Text(t) }
    }
}

/// A layered drawing: a block's row is its breadth-first distance from the
/// entry, so every forward edge points down and only a loop's back edge
/// points up (drawn round the right-hand side).
private struct CFGGraph: View {
    let function: FunctionCFG

    private static let width: CGFloat = 250
    private static let lineHeight: CGFloat = 15
    private static let hGap: CGFloat = 40, vGap: CGFloat = 46

    private struct Layout {
        var frames: [Int: CGRect] = [:]
        var rank: [Int: Int] = [:]
        var size: CGSize = .zero
    }

    private var layout: Layout {
        var l = Layout()
        let blocks = function.blocks
        let byID = Dictionary(blocks.map { ($0.id, $0) }, uniquingKeysWith: { a, _ in a })
        var queue = blocks.first.map { [$0.id] } ?? []
        if let f = queue.first { l.rank[f] = 0 }
        while !queue.isEmpty {
            let b = queue.removeFirst()
            for s in byID[b]?.succs ?? [] where l.rank[s] == nil {
                l.rank[s] = l.rank[b]! + 1
                queue.append(s)
            }
        }
        let maxRank = l.rank.values.max() ?? 0
        for b in blocks where l.rank[b.id] == nil { l.rank[b.id] = maxRank + 1 }   // unreachable: last row
        let rows = Dictionary(grouping: blocks, by: { l.rank[$0.id]! }).sorted { $0.key < $1.key }
        var y: CGFloat = 0
        var widest: CGFloat = 0
        for (_, row) in rows {
            let h = row.map { height($0) }.max() ?? 30
            for (i, b) in row.sorted(by: { $0.id < $1.id }).enumerated() {
                l.frames[b.id] = CGRect(x: CGFloat(i) * (Self.width + Self.hGap), y: y, width: Self.width, height: height(b))
            }
            widest = max(widest, CGFloat(row.count) * (Self.width + Self.hGap))
            y += h + Self.vGap
        }
        l.size = CGSize(width: widest + 80, height: y)
        return l
    }

    private func height(_ b: BlockInfo) -> CGFloat { 24 + CGFloat(max(1, b.lines.count)) * Self.lineHeight }

    var body: some View {
        let l = layout
        ZStack(alignment: .topLeading) {
            Canvas { ctx, _ in
                for b in function.blocks {
                    guard let from = l.frames[b.id] else { continue }
                    let lastLine = b.lines.last ?? ""
                    let isBranch = lastLine.hasPrefix("ifFalse")
                    for (k, s) in b.succs.enumerated() {
                        guard let to = l.frames[s] else { continue }
                        // succs of a branch: [target if false, fall-through if true]
                        let color: Color = isBranch ? (k == 0 ? Palette.deleteRed : Palette.insertGreen) : Palette.textDim
                        var path = Path()
                        let back = (l.rank[s] ?? 0) <= (l.rank[b.id] ?? 0)
                        let end: CGPoint
                        if back {
                            let start = CGPoint(x: from.maxX, y: from.midY)
                            end = CGPoint(x: to.maxX, y: to.minY + 12)
                            path.move(to: start)
                            path.addCurve(to: end, control1: CGPoint(x: start.x + 70, y: start.y),
                                          control2: CGPoint(x: end.x + 70, y: end.y))
                            ctx.stroke(path, with: .color(Palette.changeAmber), style: StrokeStyle(lineWidth: 1.5, dash: [5, 3]))
                            arrow(ctx, at: end, pointing: CGVector(dx: -1, dy: 0), color: Palette.changeAmber)
                        } else {
                            let start = CGPoint(x: from.midX + (isBranch ? (k == 0 ? 30 : -30) : 0), y: from.maxY)
                            end = CGPoint(x: to.midX, y: to.minY)
                            path.move(to: start)
                            path.addCurve(to: end, control1: CGPoint(x: start.x, y: start.y + 26),
                                          control2: CGPoint(x: end.x, y: end.y - 26))
                            ctx.stroke(path, with: .color(color), lineWidth: 1.5)
                            arrow(ctx, at: end, pointing: CGVector(dx: 0, dy: 1), color: color)
                        }
                    }
                }
            }
            .frame(width: l.size.width, height: l.size.height)

            ForEach(function.blocks) { b in
                if let f = l.frames[b.id] {
                    VStack(alignment: .leading, spacing: 0) {
                        Text("B\(b.id)\(b.id == 0 ? "  entry" : "")")
                            .font(.system(size: 9, weight: .bold))
                            .foregroundStyle(Palette.textDim)
                            .padding(.bottom, 3)
                        ForEach(Array(b.lines.enumerated()), id: \.offset) { _, line in
                            Text(line).font(.system(size: 10, design: .monospaced))
                                .foregroundStyle(line.hasSuffix(":") ? Palette.type : Palette.textPrimary)
                                .lineLimit(1)
                                .frame(height: Self.lineHeight, alignment: .leading)
                        }
                    }
                    .padding(6)
                    .frame(width: f.width, height: f.height, alignment: .topLeading)
                    .background(Palette.editorBackground, in: RoundedRectangle(cornerRadius: 6))
                    .overlay(RoundedRectangle(cornerRadius: 6).strokeBorder(Palette.panelBorder))
                    .offset(x: f.minX, y: f.minY)
                }
            }
        }
        .frame(width: l.size.width, height: l.size.height, alignment: .topLeading)
    }

    private func arrow(_ ctx: GraphicsContext, at p: CGPoint, pointing d: CGVector, color: Color) {
        var tri = Path()
        let n = CGVector(dx: -d.dy, dy: d.dx)
        tri.move(to: p)
        tri.addLine(to: CGPoint(x: p.x - d.dx * 7 + n.dx * 4, y: p.y - d.dy * 7 + n.dy * 4))
        tri.addLine(to: CGPoint(x: p.x - d.dx * 7 - n.dx * 4, y: p.y - d.dy * 7 - n.dy * 4))
        tri.closeSubpath()
        ctx.fill(tri, with: .color(color))
    }
}

// MARK: - SSA

private struct SSAView: View {
    @Bindable var model: AppModel
    @State private var functionName = ""

    var body: some View {
        if let fns = model.result?.ssa, !fns.isEmpty {
            let fn = fns.first { $0.name == functionName } ?? fns.first { $0.name == "main" } ?? fns[0]
            VStack(alignment: .leading, spacing: 6) {
                HStack {
                    Picker("Function", selection: Binding(get: { fn.name }, set: { functionName = $0 })) {
                        ForEach(fns) { Text($0.name).tag($0.name) }
                    }
                    .frame(width: 220)
                    Text("Each value has one definition; a phi picks the definition that matches the edge taken.")
                        .font(.system(size: 10)).foregroundStyle(Palette.textSecondary)
                    Spacer()
                }
                .font(.system(size: 11))
                .padding(.horizontal, 10)
                AdaptiveScroll(axes: [.vertical, .horizontal]) {
                    VStack(alignment: .leading, spacing: 1) {
                        ForEach(Array(fn.lines.enumerated()), id: \.offset) { _, line in
                            Text(line)
                                .font(.system(size: 11, design: .monospaced))
                                .foregroundStyle(line.contains("= phi(") ? Palette.changeAmber
                                                 : line.hasSuffix(":") ? Palette.type : Palette.textPrimary)
                        }
                    }
                    .textSelection(.enabled)
                    .padding(12)
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
            }
        } else {
            placeholder("The SSA form appears once the program compiles.")
        }
    }
}

// MARK: - Compare

/// Every engine on the same program: the quad interpreter on unoptimized and
/// optimized IR, the bytecode VM on optimized IR, and the AST interpreter on
/// no IR at all. They must print the same thing.
private struct CompareView: View {
    @Bindable var model: AppModel

    var body: some View {
        if let r = model.result, let run = r.run, let e = r.engines {
            let rows: [(String, String, Int, Double?)] = [
                ("Quad interpreter, unoptimized IR", run.unoptimized.output, run.unoptimized.exitCode, r.timingsMs?["runUnoptimized"]),
                ("Quad interpreter, optimized IR", run.optimized.output, run.optimized.exitCode, r.timingsMs?["runOptimized"]),
                ("Bytecode VM, optimized IR", e.vm.output, e.vm.exitCode, e.vm.ms),
                ("AST interpreter, no IR", e.reference.output, e.reference.exitCode, e.reference.ms),
            ]
            AdaptiveScroll {
                VStack(alignment: .leading, spacing: 10) {
                    HStack(spacing: 8) {
                        Image(systemName: e.allAgree ? "checkmark.seal.fill" : "exclamationmark.triangle.fill")
                            .foregroundStyle(e.allAgree ? Palette.insertGreen : Palette.errorRed)
                        Text(e.allAgree ? "All four agree." : "The engines disagree: one of them, or IRGen, has a bug.")
                            .font(.system(size: 12, weight: .semibold))
                            .foregroundStyle(e.allAgree ? Palette.insertGreen : Palette.errorRed)
                    }
                    Text("The AST interpreter never sees IR, so it is the check that an IRGen bug cannot pass by agreeing with itself.")
                        .font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
                    HStack(alignment: .top, spacing: 10) {
                        ForEach(rows, id: \.0) { row in
                            VStack(alignment: .leading, spacing: 4) {
                                Text(row.0.uppercased()).font(.system(size: 9, weight: .semibold)).tracking(0.5)
                                    .foregroundStyle(row.1 == run.unoptimized.output ? Palette.textDim : Palette.errorRed)
                                Text("exit \(row.2)" + (row.3.map { String(format: "  ·  %.2f ms", $0) } ?? ""))
                                    .font(.system(size: 10, design: .monospaced)).foregroundStyle(Palette.textDim)
                                Text(row.1.isEmpty ? "(no output)" : row.1)
                                    .font(.system(size: 11, design: .monospaced))
                                    .foregroundStyle(Palette.textPrimary)
                                    .textSelection(.enabled)
                                    .frame(maxWidth: .infinity, alignment: .topLeading)
                            }
                            .padding(8)
                            .frame(maxWidth: .infinity, alignment: .topLeading)
                            .background(Palette.editorBackground, in: RoundedRectangle(cornerRadius: 6))
                        }
                    }
                }
                .padding(12)
            }
        } else {
            placeholder("Press Run: every engine runs the program and their outputs appear side by side.")
        }
    }
}

// MARK: - Benchmark

private struct BenchmarkView: View {
    @Bindable var model: AppModel

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack(spacing: 10) {
                Stepper("Runs per engine: \(model.benchRuns)", value: $model.benchRuns, in: 1...51, step: 2)
                    .frame(width: 210)
                Button {
                    Task { await model.runBenchmark() }
                } label: {
                    Label(model.benchRunning ? "Measuring…" : "Run benchmark", systemImage: "stopwatch")
                }
                .disabled(model.benchRunning)
                Text("Medians of in-process runs, with this pipeline. Small programs run in microseconds, so their times are mostly noise.")
                    .font(.system(size: 10)).foregroundStyle(Palette.textSecondary)
                Spacer()
            }
            .font(.system(size: 11))
            if let err = model.benchError {
                Text(err).font(.system(size: 11)).foregroundStyle(Palette.errorRed)
            }
            if let b = model.bench {
                let rows = [("Quad interpreter", b.quadMs), ("Bytecode VM", b.vmMs), ("AST interpreter", b.astMs)]
                let maxMs = max(rows.map(\.1).max() ?? 1, 0.001)
                VStack(alignment: .leading, spacing: 8) {
                    Text(String(format: "Compile and optimize: %.2f ms", b.compileMs))
                        .font(.system(size: 11, design: .monospaced)).foregroundStyle(Palette.textSecondary)
                    ForEach(rows, id: \.0) { name, ms in
                        HStack(spacing: 8) {
                            Text(name).font(.system(size: 11)).frame(width: 130, alignment: .leading)
                            GeometryReader { geo in
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(name == "Bytecode VM" ? Palette.insertGreen : Palette.type)
                                    .frame(width: max(2, geo.size.width * ms / maxMs))
                            }
                            .frame(height: 12)
                            Text(String(format: "%.3f ms", ms)).font(.system(size: 11, design: .monospaced)).frame(width: 90, alignment: .trailing)
                            Text(b.vmMs > 0 ? String(format: "%.1f× the VM", ms / b.vmMs) : "")
                                .font(.system(size: 10)).foregroundStyle(Palette.textDim).frame(width: 80, alignment: .leading)
                        }
                    }
                    Label(b.agree ? "All three printed the same output in every run." : "The engines printed different output.",
                          systemImage: b.agree ? "checkmark.circle" : "xmark.octagon")
                        .font(.system(size: 11))
                        .foregroundStyle(b.agree ? Palette.insertGreen : Palette.errorRed)
                }
            } else if !model.benchRunning {
                placeholder("Times the quad interpreter, the bytecode VM and the AST interpreter on the current program.")
            }
            Spacer(minLength: 0)
        }
        .padding(12)
    }
}
