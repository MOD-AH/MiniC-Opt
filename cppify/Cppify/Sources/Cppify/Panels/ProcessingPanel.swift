import SwiftUI

/// The reason this app exists. Three columns: which pass ran when, what it
/// did to the code, and why that was legal.
struct ProcessingPanel: View {
    @Bindable var model: AppModel

    var body: some View {
        Group {
            if model.result?.ok != true {
                empty("wand.and.rays", "Nothing to show yet",
                      model.diagnostics.isEmpty ? "Press Run to compile and optimize."
                                                : "Fix the errors in Problems, then Run again.")
            } else if let fn = model.focusedFunction {
                HSplitView {
                    Timeline(model: model, function: fn)
                        .frame(minWidth: 210, idealWidth: 250, maxWidth: 320)
                    DiffColumn(model: model, function: fn)
                        .frame(minWidth: 300)
                    ExplanationColumn(model: model, function: fn)
                        .frame(minWidth: 300, idealWidth: 380)
                }
            } else {
                empty("wand.and.rays", "No functions traced", "The program produced no IR.")
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Palette.consoleBackground)
    }

    private func empty(_ icon: String, _ title: String, _ detail: String) -> some View {
        VStack(spacing: 8) {
            Image(systemName: icon).font(.system(size: 26)).foregroundStyle(Palette.textDim)
            Text(title).font(.system(size: 13, weight: .medium)).foregroundStyle(Palette.textPrimary)
            Text(detail).font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

// MARK: - Column 1: the sweep timeline

private struct Timeline: View {
    @Bindable var model: AppModel
    let function: FunctionTrace

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 10) {
                Text("FUNCTION \(function.name.uppercased())")
                    .font(.system(size: 9, weight: .semibold)).tracking(0.8)
                    .foregroundStyle(Palette.textDim)

                ForEach(function.sweeps) { sweep in
                    VStack(alignment: .leading, spacing: 3) {
                        HStack(spacing: 5) {
                            Text("Sweep \(sweep.sweep)")
                                .font(.system(size: 11, weight: .bold))
                                .foregroundStyle(Palette.textSecondary)
                            if !sweep.changedAnything {
                                Text("fixed point")
                                    .font(.system(size: 9, weight: .semibold))
                                    .foregroundStyle(Palette.insertGreen)
                                    .padding(.horizontal, 5).padding(.vertical, 1)
                                    .background(Palette.insertGreen.opacity(0.15), in: Capsule())
                            }
                        }
                        ForEach(sweep.passes) { pass in
                            PassRow(pass: pass,
                                    selected: model.selection == PassSelection(sweep: sweep.sweep, passID: pass.id))
                                .onTapGesture {
                                    model.selection = PassSelection(sweep: sweep.sweep, passID: pass.id)
                                    model.highlightedLine = pass.edits.first?.sourceLine
                                }
                        }
                    }
                }

                // The terminal step: not a pass, but the question a reviewer
                // will actually ask.
                Divider().overlay(Palette.panelBorder).padding(.vertical, 4)
                ComparisonRow(selected: model.selection == Self.comparisonSelection)
                    .onTapGesture { model.selection = Self.comparisonSelection }
            }
            .padding(12)
        }
    }

    static let comparisonSelection = PassSelection(sweep: -1, passID: "__comparison__")
}

private struct PassRow: View {
    let pass: TracePass
    let selected: Bool

    var body: some View {
        HStack(spacing: 6) {
            Circle()
                .fill(pass.changed ? Palette.insertGreen : Palette.textDim.opacity(0.35))
                .frame(width: 5, height: 5)
            Text(pass.pass)
                .font(.system(size: 11, design: .monospaced))
                .foregroundStyle(pass.changed ? Palette.textPrimary : Palette.textDim)
            Spacer(minLength: 4)
            if pass.transforms > 0 {
                Text("\(pass.transforms)")
                    .font(.system(size: 10, weight: .semibold, design: .monospaced))
                    .foregroundStyle(Palette.insertGreen)
            }
            if pass.instructionDelta != 0 {
                Text("\(pass.instructionDelta)")
                    .font(.system(size: 10, design: .monospaced))
                    .foregroundStyle(Palette.changeAmber)
            }
        }
        .padding(.horizontal, 8).padding(.vertical, 5)
        .background(selected ? Palette.type.opacity(0.18) : .clear,
                    in: RoundedRectangle(cornerRadius: 6))
        .contentShape(Rectangle())
    }
}

private struct ComparisonRow: View {
    let selected: Bool
    var body: some View {
        HStack(spacing: 6) {
            Image(systemName: "scalemass").font(.system(size: 10)).foregroundStyle(Palette.changeAmber)
            Text("vs. production compiler")
                .font(.system(size: 11, weight: .medium))
                .foregroundStyle(Palette.textPrimary)
            Spacer(minLength: 0)
        }
        .padding(.horizontal, 8).padding(.vertical, 6)
        .background(selected ? Palette.changeAmber.opacity(0.18) : .clear,
                    in: RoundedRectangle(cornerRadius: 6))
        .contentShape(Rectangle())
    }
}

// MARK: - Column 2: before / after IR

private struct DiffColumn: View {
    @Bindable var model: AppModel
    let function: FunctionTrace

    private var selectedPass: TracePass? {
        guard let sel = model.selection else { return nil }
        return function.sweeps.first { $0.sweep == sel.sweep }?
                      .passes.first { $0.id == sel.passID }
    }

    var body: some View {
        ScrollView {
            if model.selection == Timeline.comparisonSelection {
                FinalIRView(function: function)
            } else if let pass = selectedPass {
                if pass.changed {
                    VStack(alignment: .leading, spacing: 10) {
                        header("BEFORE → AFTER", pass: pass)
                        DiffView(before: pass.irBefore, after: pass.irAfter)
                    }
                    .padding(12)
                } else {
                    VStack(alignment: .leading, spacing: 8) {
                        header("NO CHANGE", pass: pass)
                        Text("This pass found nothing to do on this program. That is a result, not a failure — a pass firing zero times is how the fixed point is reached.")
                            .font(.system(size: 11))
                            .foregroundStyle(Palette.textSecondary)
                    }
                    .padding(12)
                }
            } else {
                Text("Select a pass.").foregroundStyle(Palette.textDim).padding(20)
            }
        }
    }

    private func header(_ label: String, pass: TracePass) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(label).font(.system(size: 9, weight: .semibold)).tracking(0.8)
                .foregroundStyle(Palette.textDim)
            Text(pass.title).font(.system(size: 13, weight: .semibold))
                .foregroundStyle(Palette.textPrimary)
        }
    }
}

/// Line-level diff. A longest-common-subsequence would be overkill for lists
/// this short; marking lines that exist on only one side identifies every
/// insertion and deletion correctly, which is what the reader needs.
private struct DiffView: View {
    let before: [String], after: [String]

    var body: some View {
        let beforeSet = Set(before), afterSet = Set(after)
        HStack(alignment: .top, spacing: 10) {
            column(before, title: "before") { !afterSet.contains($0) ? Palette.deleteRed : nil }
            column(after,  title: "after")  { !beforeSet.contains($0) ? Palette.insertGreen : nil }
        }
    }

    private func column(_ lines: [String], title: String,
                        tint: @escaping (String) -> Color?) -> some View {
        VStack(alignment: .leading, spacing: 1) {
            Text(title).font(.system(size: 9, weight: .semibold))
                .foregroundStyle(Palette.textDim).padding(.bottom, 3)
            ForEach(Array(lines.enumerated()), id: \.offset) { _, line in
                let c = tint(line)
                Text(line)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(c ?? Palette.textSecondary)
                    .strikethrough(c == Palette.deleteRed)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.horizontal, 5).padding(.vertical, 1)
                    .background((c ?? .clear).opacity(c == nil ? 0 : 0.12))
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}

private struct FinalIRView: View {
    let function: FunctionTrace
    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("START → FINISH").font(.system(size: 9, weight: .semibold)).tracking(0.8)
                .foregroundStyle(Palette.textDim)
            Text("Everything the optimizer did to \(function.name), end to end")
                .font(.system(size: 13, weight: .semibold)).foregroundStyle(Palette.textPrimary)
            DiffView(before: function.irInitial, after: function.irFinal)
        }
        .padding(12)
    }
}

// MARK: - Column 3: explanation

private struct ExplanationColumn: View {
    @Bindable var model: AppModel
    let function: FunctionTrace

    private var selectedPass: TracePass? {
        guard let sel = model.selection else { return nil }
        return function.sweeps.first { $0.sweep == sel.sweep }?
                      .passes.first { $0.id == sel.passID }
    }

    var body: some View {
        ScrollView {
            if model.selection == Timeline.comparisonSelection {
                ComparisonCard(model: model, function: function)
                    .padding(12)
            } else if let pass = selectedPass, let meta = Pass.named(pass.pass) {
                VStack(alignment: .leading, spacing: 14) {
                    block("WHAT IT DOES", meta.what)
                    block("WHAT MAKES IT LEGAL", meta.licensedBy, tint: Palette.type)
                    if let refuses = meta.refuses {
                        block("WHAT IT REFUSES TO DO", refuses, tint: Palette.changeAmber)
                    }

                    if pass.edits.isEmpty {
                        block("THIS RUN", "No rewrites on this program.", tint: Palette.textDim)
                    } else {
                        VStack(alignment: .leading, spacing: 7) {
                            Text("EVERY REWRITE THIS RUN (\(pass.edits.count))")
                                .font(.system(size: 9, weight: .semibold)).tracking(0.8)
                                .foregroundStyle(Palette.insertGreen)
                            ForEach(pass.edits) { edit in
                                EditCard(edit: edit)
                                    .onTapGesture { model.highlightedLine = edit.sourceLine }
                            }
                        }
                    }
                }
                .padding(12)
            } else {
                Text("Select a pass to see why it did what it did.")
                    .font(.system(size: 11)).foregroundStyle(Palette.textDim).padding(20)
            }
        }
    }

    private func block(_ title: String, _ body: String, tint: Color = Palette.textDim) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(title).font(.system(size: 9, weight: .semibold)).tracking(0.8).foregroundStyle(tint)
            Text(body).font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
                .fixedSize(horizontal: false, vertical: true)
        }
    }
}

private struct EditCard: View {
    let edit: TraceEdit

    var body: some View {
        VStack(alignment: .leading, spacing: 5) {
            HStack(spacing: 6) {
                Text(edit.kind)
                    .font(.system(size: 9, weight: .semibold))
                    .foregroundStyle(Palette.changeAmber)
                if edit.sourceLine > 0 {
                    Text("line \(edit.sourceLine)")
                        .font(.system(size: 9, design: .monospaced))
                        .foregroundStyle(Palette.textDim)
                }
                Spacer(minLength: 0)
            }
            Text(edit.before)
                .font(.system(size: 11, design: .monospaced))
                .foregroundStyle(Palette.deleteRed)
                .strikethrough(edit.isDeletion)
            if !edit.isDeletion {
                HStack(spacing: 4) {
                    Image(systemName: "arrow.down").font(.system(size: 8)).foregroundStyle(Palette.textDim)
                    Text(edit.after)
                        .font(.system(size: 11, design: .monospaced))
                        .foregroundStyle(Palette.insertGreen)
                }
            }
            Text(edit.reason)
                .font(.system(size: 10))
                .foregroundStyle(Palette.textSecondary)
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(9)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color.white.opacity(0.04), in: RoundedRectangle(cornerRadius: 7))
        .contentShape(Rectangle())
    }
}
