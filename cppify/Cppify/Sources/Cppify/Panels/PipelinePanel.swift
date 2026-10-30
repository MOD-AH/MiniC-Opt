import SwiftUI

/// Roadmap 5.1: choose the compiler, stage by stage.
///
/// The seven stages are chips, each showing the one implementation in use.
/// Lexer, parser and execution engine are chosen here; the optimizer is an
/// ordered list of pass groups; analyses are never chosen, they follow from
/// the passes (each pass's registry entry says what it requires), so a
/// prerequisite cannot be forgotten. Everything the compiler would reject is
/// said here before Run, in the compiler's own terms.
struct PipelinePanel: View {
    @Bindable var model: AppModel

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            StageChips(model: model)
            HStack(alignment: .top, spacing: 12) {
                PassEditor(model: model)
                    .frame(minWidth: 340, maxWidth: .infinity)
                AnalysisColumn(model: model)
                    .frame(minWidth: 240, idealWidth: 300, maxWidth: 360)
            }
        }
        .padding(12)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .background(Palette.consoleBackground)
    }
}

// MARK: - The seven stages

private struct StageChips: View {
    @Bindable var model: AppModel

    var body: some View {
        let p = model.pipeline
        let usesSSA = p.groups.contains { $0.passes.contains(where: PipelineSettings.isSSA) }
        let analyses = p.requiredAnalyses(model.techniques).count
        AdaptiveScroll(axes: .horizontal, showsIndicators: false) {
            HStack(spacing: 6) {
                chooser(1, "Lexer", selection: Binding(get: { model.pipeline.lexer }, set: { model.pipeline.lexer = $0 }),
                        options: [("re2c", "re2c DFA"), ("handwritten", "hand-written")])
                arrow
                chooser(2, "Parser", selection: Binding(get: { model.pipeline.parser }, set: { model.pipeline.parser = $0 }),
                        options: [("handwritten", "recursive descent"), ("bison", "Bison IELR(1)")])
                arrow
                chip(3, "Semantic", "typed AST")
                arrow
                chip(4, "IR", usesSSA && model.optimizationOn ? "TAC + SSA" : "TAC")
                arrow
                chip(5, "Analysis", "\(analyses) added automatically")
                arrow
                chip(6, "Optimizer", model.optimizationOn ? model.pipeline.preset : "off")
                arrow
                chooser(7, "Execution", selection: Binding(get: { model.pipeline.engine }, set: { model.pipeline.engine = $0 }),
                        options: [("quad", "quad interpreter"), ("vm", "bytecode VM"), ("ast", "AST interpreter")])
            }
            .padding(.vertical, 2)
        }
    }

    private var arrow: some View {
        Image(systemName: "chevron.right").font(.system(size: 9)).foregroundStyle(Palette.textDim)
    }

    private func header(_ stage: Int, _ name: String) -> some View {
        Text("\(stage) \(name.uppercased())")
            .font(.system(size: 8, weight: .bold)).tracking(0.6)
            .foregroundStyle(Palette.textDim)
    }

    /// The registry's one-line note for the technique a chip shows.
    private func note(_ stage: Int, _ key: String) -> String {
        let ids: [String: String] = ["re2c": "lexer.re2c", "handwritten-1": "lexer.handwritten",
                                     "handwritten-2": "parser.recursive-descent", "bison": "parser.bison",
                                     "quad": "engine.quad-interp", "vm": "engine.bytecode-vm", "ast": "engine.ast-interp"]
        let id = ids[key] ?? ids["\(key)-\(stage)"] ?? ""
        return model.techniques.first { $0.id == id }?.description ?? ""
    }

    private func chip(_ stage: Int, _ name: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: 3) {
            header(stage, name)
            Text(value).font(.system(size: 11, weight: .medium)).foregroundStyle(Palette.textSecondary)
        }
        .padding(.horizontal, 10).padding(.vertical, 6)
        .background(Color.white.opacity(0.04), in: RoundedRectangle(cornerRadius: 8))
    }

    private func chooser(_ stage: Int, _ name: String, selection: Binding<String>,
                         options: [(String, String)]) -> some View {
        VStack(alignment: .leading, spacing: 3) {
            header(stage, name)
            Menu {
                Picker("", selection: selection) {
                    ForEach(options, id: \.0) { Text($0.1).tag($0.0) }
                }
                .pickerStyle(.inline)
                .labelsHidden()
            } label: {
                Text(options.first { $0.0 == selection.wrappedValue }?.1 ?? selection.wrappedValue)
                    .font(.system(size: 11, weight: .semibold))
            }
            .menuStyle(.borderlessButton)
            .fixedSize()
        }
        .padding(.horizontal, 10).padding(.vertical, 6)
        .glassControl(tint: Palette.type, cornerRadius: 8)
        .help(note(stage, selection.wrappedValue))
    }
}

// MARK: - Passes

private struct PassEditor: View {
    @Bindable var model: AppModel

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack(spacing: 8) {
                Text("PASSES").font(.system(size: 10, weight: .semibold)).tracking(0.8).foregroundStyle(Palette.textDim)
                Picker("Preset", selection: Binding(get: { model.pipeline.preset },
                                                    set: { model.pipeline.apply(preset: $0) })) {
                    ForEach(PipelineSettings.presets, id: \.self) { Text($0.capitalized).tag($0) }
                    Text("Custom").tag("custom")
                }
                .frame(width: 190)
                Spacer()
                Button {
                    model.pipeline = .safeDefaults
                    model.optimizationOn = true
                } label: {
                    Label("Reset to Safe Defaults", systemImage: "arrow.counterclockwise")
                }
                .help("Classic P1–P9 pipeline, re2c lexer, hand-written parser, quad interpreter")
            }
            .font(.system(size: 11))

            if !model.optimizationOn {
                Text("Optimization is off (toolbar), so no pass runs.")
                    .font(.system(size: 11)).foregroundStyle(Palette.changeAmber)
            }
            ForEach(model.pipeline.problems, id: \.self) { p in
                Label(p, systemImage: "exclamationmark.triangle.fill")
                    .font(.system(size: 11)).foregroundStyle(Palette.errorRed)
            }

            AdaptiveScroll {
                VStack(alignment: .leading, spacing: 10) {
                    if model.pipeline.groups.isEmpty {
                        Text("No passes: the IR runs exactly as generated.")
                            .font(.system(size: 11)).foregroundStyle(Palette.textDim)
                    }
                    ForEach(model.pipeline.groups) { g in
                        GroupCard(model: model, id: g.id)
                    }
                    HStack {
                        Button("Add SSA group") { add(["sccp"]) }
                        Button("Add TAC group") { add(["fold"]) }
                    }
                    .font(.system(size: 11))
                }
            }
        }
        .padding(10)
        .opaqueSurface(Palette.editorBackground)
    }

    private func add(_ passes: [String]) {
        model.pipeline.groups.append(PassGroup(passes: passes))
        model.pipeline.edited()
    }
}

/// One group: its passes in order, how often it sweeps, and controls to
/// reorder, add and remove. Only passes of the group's kind can be added,
/// since SSA and TAC passes cannot share a group.
private struct GroupCard: View {
    @Bindable var model: AppModel
    let id: UUID

    // Looked up by identity, not position: SwiftUI may still evaluate a card
    // whose group was just removed, and a stale index would crash.
    private var index: Int { model.pipeline.groups.firstIndex { $0.id == id } ?? 0 }
    private var group: PassGroup { model.pipeline.groups.first { $0.id == id } ?? PassGroup(passes: []) }
    private var isSSA: Bool { group.passes.contains(where: PipelineSettings.isSSA) }

    var body: some View {
        if model.pipeline.groups.contains(where: { $0.id == id }) { card }
    }

    private var card: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(spacing: 8) {
                Text("Group \(index + 1)").font(.system(size: 12, weight: .semibold)).foregroundStyle(Palette.textPrimary)
                Text(isSSA ? "SSA: built before, destroyed after" : "plain TAC")
                    .font(.system(size: 10)).foregroundStyle(isSSA ? Palette.type : Palette.textDim)
                Spacer()
                Picker("", selection: Binding(
                    get: { group.fixpoint ? 0 : group.sweeps },
                    set: { v in
                        model.pipeline.groups[index].fixpoint = (v == 0)
                        model.pipeline.groups[index].sweeps = max(1, v)
                        model.pipeline.edited()
                    })) {
                    Text("to a fixed point").tag(0)
                    ForEach([1, 2, 3, 4], id: \.self) { Text("\($0) sweep\($0 == 1 ? "" : "s")").tag($0) }
                }
                .labelsHidden()
                .frame(width: 140)
                Button(role: .destructive) {
                    model.pipeline.groups.remove(at: index)
                    model.pipeline.edited()
                } label: { Image(systemName: "trash") }
                .buttonStyle(.borderless)
                .help("Remove this group")
            }
            ForEach(Array(group.passes.enumerated()), id: \.offset) { pi, id in
                HStack(spacing: 6) {
                    Text("\(pi + 1).").font(.system(size: 10, design: .monospaced)).foregroundStyle(Palette.textDim)
                        .frame(width: 22, alignment: .trailing)
                    Text(Pass.named(id)?.title ?? id).font(.system(size: 12)).foregroundStyle(Palette.textPrimary)
                    Text(id).font(.system(size: 10, design: .monospaced)).foregroundStyle(Palette.textDim)
                    Spacer()
                    Button { move(pi, by: -1) } label: { Image(systemName: "chevron.up") }
                        .disabled(pi == 0)
                    Button { move(pi, by: 1) } label: { Image(systemName: "chevron.down") }
                        .disabled(pi == group.passes.count - 1)
                    Button {
                        model.pipeline.groups[index].passes.remove(at: pi)
                        model.pipeline.edited()
                    } label: { Image(systemName: "minus.circle") }
                }
                .buttonStyle(.borderless)
                .help(Pass.named(id)?.what ?? id)
            }
            Menu("Add pass") {
                ForEach(addable, id: \.self) { id in
                    Button("\(Pass.named(id)?.title ?? id)  (\(id))") {
                        // A pass may repeat: `constprop, fold, constprop` is a valid order.
                        model.pipeline.groups[index].passes.append(id)
                        model.pipeline.edited()
                    }
                }
            }
            .menuStyle(.borderlessButton)
            .fixedSize()
            .font(.system(size: 11))
        }
        .padding(10)
        .background(Color.white.opacity(0.04), in: RoundedRectangle(cornerRadius: 8))
    }

    private var addable: [String] { isSSA ? PipelineSettings.ssaOrder : PipelineSettings.tacOrder }

    private func move(_ i: Int, by d: Int) {
        model.pipeline.groups[index].passes.swapAt(i, i + d)
        model.pipeline.edited()
    }
}

// MARK: - Analyses, derived

private struct AnalysisColumn: View {
    @Bindable var model: AppModel

    var body: some View {
        let needed = model.pipeline.requiredAnalyses(model.techniques)
        VStack(alignment: .leading, spacing: 8) {
            Text("ANALYSES (ADDED AUTOMATICALLY)")
                .font(.system(size: 10, weight: .semibold)).tracking(0.8).foregroundStyle(Palette.textDim)
            Text("Not chosen: every pass's registry entry names what it needs, and those (and what they need) are listed here.")
                .font(.system(size: 10)).foregroundStyle(Palette.textSecondary)
                .fixedSize(horizontal: false, vertical: true)
            if model.techniques.isEmpty {
                Text("The registry could not be read from the compiler.")
                    .font(.system(size: 11)).foregroundStyle(Palette.changeAmber)
            }
            AdaptiveScroll {
                VStack(alignment: .leading, spacing: 8) {
                    ForEach(needed, id: \.technique.id) { item in
                        VStack(alignment: .leading, spacing: 2) {
                            Text(item.technique.name).font(.system(size: 12, weight: .medium)).foregroundStyle(Palette.textPrimary)
                            Text("needed by " + item.neededBy.joined(separator: ", "))
                                .font(.system(size: 10, design: .monospaced)).foregroundStyle(Palette.type)
                            Text(item.technique.description)
                                .font(.system(size: 10)).foregroundStyle(Palette.textSecondary)
                                .fixedSize(horizontal: false, vertical: true)
                        }
                    }
                    if needed.isEmpty && !model.techniques.isEmpty {
                        Text("None for these passes.").font(.system(size: 11)).foregroundStyle(Palette.textDim)
                    }
                }
            }
        }
        .padding(10)
        .opaqueSurface(Palette.editorBackground)
    }
}
