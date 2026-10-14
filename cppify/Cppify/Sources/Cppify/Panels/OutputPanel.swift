import SwiftUI

/// Console output. Opaque surface, monospaced, no glass behind the text.
struct OutputPanel: View {
    @Bindable var model: AppModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 12) {
                if let err = model.serviceError {
                    section("Compiler could not run", tint: Palette.errorRed) {
                        Text(err).foregroundStyle(Palette.errorRed)
                    }
                } else if let run = model.result?.run {
                    section("Program output", tint: Palette.insertGreen) {
                        Text(run.optimized.output.isEmpty ? "(no output)" : run.optimized.output)
                            .foregroundStyle(Palette.textPrimary)
                    }

                    // The differential check, surfaced. This is the project's
                    // central safety property: optimizing must not change what
                    // the program prints.
                    HStack(spacing: 8) {
                        Image(systemName: run.outputsMatch ? "checkmark.seal.fill" : "exclamationmark.triangle.fill")
                            .foregroundStyle(run.outputsMatch ? Palette.insertGreen : Palette.errorRed)
                        VStack(alignment: .leading, spacing: 2) {
                            Text(run.outputsMatch
                                 ? "Optimized and unoptimized output are identical"
                                 : "MISMATCH — optimization changed the program's behaviour")
                                .font(.system(size: 12, weight: .semibold))
                                .foregroundStyle(run.outputsMatch ? Palette.insertGreen : Palette.errorRed)
                            Text(run.outputsMatch
                                 ? "The optimizer changed the code, not what it computes."
                                 : "This is a real bug: unoptimized produced \(run.unoptimized.output.trimmed), optimized produced \(run.optimized.output.trimmed).")
                                .font(.system(size: 11))
                                .foregroundStyle(Palette.textSecondary)
                        }
                        Spacer()
                    }
                    .padding(10)
                    .background((run.outputsMatch ? Palette.insertGreen : Palette.errorRed).opacity(0.10),
                                in: RoundedRectangle(cornerRadius: 8))

                    if !run.unoptimized.output.isEmpty {
                        section("Unoptimized run (reference)", tint: Palette.textDim) {
                            Text(run.unoptimized.output).foregroundStyle(Palette.textSecondary)
                        }
                    }
                } else if !model.diagnostics.isEmpty {
                    Text("Build failed — see the Problems tab.")
                        .foregroundStyle(Palette.errorRed)
                } else {
                    Text("Press Run to compile and execute.")
                        .foregroundStyle(Palette.textDim)
                }
            }
            .font(.system(size: 12, design: .monospaced))
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(14)
        }
        .background(Palette.consoleBackground)
    }

    @ViewBuilder
    private func section(_ title: String, tint: Color, @ViewBuilder content: () -> some View) -> some View {
        VStack(alignment: .leading, spacing: 5) {
            Text(title.uppercased())
                .font(.system(size: 9, weight: .semibold)).tracking(0.7)
                .foregroundStyle(tint)
            content()
        }
    }
}

/// Diagnostics, in the compiler's own file:line:col form, each one clickable.
struct ProblemsPanel: View {
    @Bindable var model: AppModel

    var body: some View {
        Group {
            if let err = model.serviceError {
                centered(icon: "bolt.trianglebadge.exclamationmark", tint: Palette.errorRed,
                         title: "The compiler could not run", detail: err)
            } else if model.diagnostics.isEmpty {
                centered(icon: "checkmark.circle", tint: Palette.insertGreen,
                         title: "No problems",
                         detail: model.result == nil ? "Press Run to compile."
                                                     : "The program compiled with no diagnostics.")
            } else {
                ScrollView {
                    VStack(alignment: .leading, spacing: 0) {
                        ForEach(model.diagnostics) { d in
                            Button {
                                model.highlightedLine = d.line
                            } label: {
                                HStack(alignment: .top, spacing: 9) {
                                    Image(systemName: "xmark.octagon.fill")
                                        .font(.system(size: 11))
                                        .foregroundStyle(Palette.errorRed)
                                    VStack(alignment: .leading, spacing: 2) {
                                        Text(d.message)
                                            .font(.system(size: 12))
                                            .foregroundStyle(Palette.textPrimary)
                                            .multilineTextAlignment(.leading)
                                        Text("line \(d.line), column \(d.col)  ·  caught by \(d.stage)")
                                            .font(.system(size: 10, design: .monospaced))
                                            .foregroundStyle(Palette.textDim)
                                    }
                                    Spacer(minLength: 0)
                                }
                                .padding(.horizontal, 14).padding(.vertical, 9)
                                .contentShape(Rectangle())
                            }
                            .buttonStyle(.plain)
                            Divider().overlay(Palette.panelBorder)
                        }
                    }
                }
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Palette.consoleBackground)
    }

    private func centered(icon: String, tint: Color, title: String, detail: String) -> some View {
        VStack(spacing: 8) {
            Image(systemName: icon).font(.system(size: 26)).foregroundStyle(tint)
            Text(title).font(.system(size: 13, weight: .medium)).foregroundStyle(Palette.textPrimary)
            Text(detail).font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .padding(20)
    }
}

extension String {
    var trimmed: String { trimmingCharacters(in: .whitespacesAndNewlines) }
}
