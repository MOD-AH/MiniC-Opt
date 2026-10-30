import SwiftUI

/// Console output. Opaque surface, monospaced, no glass behind the text.
struct OutputPanel: View {
    @Bindable var model: AppModel

    var body: some View {
        AdaptiveScroll {
            VStack(alignment: .leading, spacing: 12) {
                if let err = model.serviceError {
                    section("Compiler could not run", tint: Palette.errorRed) {
                        Text(err).foregroundStyle(Palette.errorRed)
                    }
                } else if let run = model.result?.run {
                    // The engine chosen in the Pipeline tab; every engine ran.
                    let engines = model.result?.engines
                    let (engineName, shown, exit): (String, String, Int) = {
                        switch model.pipeline.engine {
                        case "vm":  return ("bytecode VM", engines?.vm.output ?? run.optimized.output, engines?.vm.exitCode ?? run.optimized.exitCode)
                        case "ast": return ("AST interpreter", engines?.reference.output ?? run.optimized.output, engines?.reference.exitCode ?? run.optimized.exitCode)
                        default:    return ("quad interpreter", run.optimized.output, run.optimized.exitCode)
                        }
                    }()
                    section("Program output  ·  \(engineName)" + (exit != 0 ? "  ·  stopped with a runtime error" : ""),
                            tint: exit != 0 ? Palette.errorRed : Palette.insertGreen) {
                        Text(shown.isEmpty ? "(no output)" : shown)
                            .foregroundStyle(exit != 0 ? Palette.errorRed : Palette.textPrimary)
                    }
                    if let e = engines, !e.allAgree {
                        Text("The engines disagree. See Views → Compare.")
                            .foregroundStyle(Palette.errorRed)
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
                AdaptiveScroll {
                    VStack(alignment: .leading, spacing: 0) {
                        ForEach(model.diagnostics) { d in
                            Button {
                                model.highlightedLine = d.line
                            } label: {
                                HStack(alignment: .top, spacing: 9) {
                                    Image(systemName: d.unsupported == true ? "puzzlepiece.extension.fill"
                                                      : d.isPrimary ? "xmark.octagon.fill" : "arrow.turn.down.right")
                                        .font(.system(size: 11))
                                        .foregroundStyle(d.isPrimary ? Palette.errorRed : Palette.textDim)
                                    VStack(alignment: .leading, spacing: 3) {
                                        HStack(spacing: 6) {
                                            if let code = d.code {
                                                Text(code)
                                                    .font(.system(size: 9, weight: .semibold, design: .monospaced))
                                                    .foregroundStyle(d.isPrimary ? Palette.changeAmber : Palette.textDim)
                                                    .padding(.horizontal, 5).padding(.vertical, 1)
                                                    .background(Color.white.opacity(0.06), in: Capsule())
                                            }
                                            if let title = d.title {
                                                Text(title).font(.system(size: 10, weight: .medium)).foregroundStyle(Palette.textSecondary)
                                            }
                                            if d.unsupported == true {
                                                Text("not supported by Cppify").font(.system(size: 10)).foregroundStyle(Palette.changeAmber)
                                            }
                                        }
                                        Text(d.message)
                                            .font(.system(size: 12))
                                            .foregroundStyle(d.isPrimary ? Palette.textPrimary : Palette.textSecondary)
                                            .multilineTextAlignment(.leading)
                                        Text("line \(d.line), column \(d.col)  ·  caught by \(d.stage)"
                                             + (d.isPrimary ? "" : "  ·  may only follow from the error above"))
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
