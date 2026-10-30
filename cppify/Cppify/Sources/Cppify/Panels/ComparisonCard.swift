import SwiftUI

/// The terminal step of the Processing timeline: what did optimizing buy,
/// and what would a real C compiler have done with the same source?
///
/// The numbers come from actually invoking clang, never from a hardcoded
/// table, so they track whatever toolchain is installed. The verdict is
/// stated plainly even though it is unflattering: clang -O2 wins, usually
/// by a lot. Claiming otherwise is the one thing in a viva that would not
/// survive a single follow-up question.
struct ComparisonCard: View {
    @Bindable var model: AppModel
    let function: FunctionTrace

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            VStack(alignment: .leading, spacing: 3) {
                Text("FINAL STEP").font(.system(size: 9, weight: .semibold)).tracking(0.8)
                    .foregroundStyle(Palette.changeAmber)
                Text("Compared to a production compiler")
                    .font(.system(size: 14, weight: .semibold)).foregroundStyle(Palette.textPrimary)
            }

            if let m = model.result?.metrics {
                VStack(alignment: .leading, spacing: 7) {
                    Text("MEASURED, NOT ASSERTED")
                        .font(.system(size: 9, weight: .semibold)).tracking(0.8)
                        .foregroundStyle(Palette.textDim)

                    row("MiniC-Opt, no optimization", "\(m.staticBefore)", Palette.textSecondary)
                    row("MiniC-Opt, P1–P9", "\(m.staticAfter)", Palette.insertGreen, bold: true)
                    if let c = model.clang {
                        row("clang -O0", "\(c.o0Instructions)", Palette.textSecondary)
                        row("clang -O2", "\(c.o2Instructions)", Palette.changeAmber, bold: true)
                    }
                }
                .padding(10)
                .background(Color.white.opacity(0.04), in: RoundedRectangle(cornerRadius: 8))

                HStack(spacing: 14) {
                    stat("static", m.staticBefore, m.staticAfter, m.staticReduction)
                    stat("dynamic", m.dynamicBefore, m.dynamicAfter, m.dynamicReduction)
                }
            }

            if let c = model.clang {
                verdict(clang: c)
            } else {
                block("COMPARISON UNAVAILABLE",
                      "clang could not compile this program as standalone C, so there is "
                    + "nothing to compare against. Programs using the MiniC builtins "
                    + "(print_int, read_int) hit this: they are recognised by this compiler "
                    + "but are not declared C functions.",
                      tint: Palette.textDim)
            }

            block("WHAT A PRODUCTION COMPILER DOES THAT THIS ONE DOES NOT",
                  "Register allocation, instruction selection and scheduling, function "
                + "inlining, loop unrolling, vectorization, interprocedural and alias "
                + "analysis, profile-guided optimization, and emitting real machine code. "
                + "MiniC-Opt stops at slot-addressed pseudo-assembly and executes through "
                + "an interpreter — there is no native code generator here at all.",
                  tint: Palette.errorRed)

            block("WHAT THIS ONE DOES THAT CLANG DOES NOT",
                  "Explain itself. clang hands back a binary; at best -Rpass remarks note "
                + "that some transform occurred. It will not show you \(function.sweepCount) "
                + "fixed-point sweeps, name the reaching definition that licensed a "
                + "substitution, or tell you why dead-code elimination refused to delete a "
                + "division because it might trap. That trace is the entire point of this "
                + "project.",
                  tint: Palette.insertGreen)

            block("THE HONEST FRAMING",
                  "P1–P9 are the same classical passes production compilers implement. This "
                + "is a faithful, inspectable implementation of textbook optimization — "
                + "valuable because it is legible, not because it is faster.",
                  tint: Palette.type)
        }
    }

    @ViewBuilder
    private func verdict(clang c: ClangComparison) -> some View {
        let ours = model.result?.metrics?.staticAfter ?? 0
        let clangWins = c.o2Instructions < ours
        VStack(alignment: .leading, spacing: 5) {
            HStack(spacing: 6) {
                Image(systemName: clangWins ? "arrow.down.right.circle.fill" : "equal.circle.fill")
                    .foregroundStyle(clangWins ? Palette.changeAmber : Palette.insertGreen)
                Text(clangWins ? "clang -O2 wins, and by a wide margin"
                               : "comparable on this program")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundStyle(Palette.textPrimary)
            }
            Text(clangWins
                 ? "clang reduces this program to \(c.o2Instructions) instructions against our "
                 + "\(ours). On loops whose result is computable at compile time it simply "
                 + "evaluates the whole thing and returns the constant. This panel says so "
                 + "rather than hiding it."
                 : "Both compilers land in the same neighbourhood on this program, which "
                 + "says more about the program being small than about the optimizers.")
                .font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
                .fixedSize(horizontal: false, vertical: true)

            if c.exitCode >= 0, let run = model.result?.run {
                let interpreted = run.optimized.output.trimmed
                if let value = Int(interpreted), value % 256 == Int(c.exitCode), value > 255 {
                    Text("Note: the native binary exits with \(c.exitCode), not \(value). A process "
                       + "exit status is truncated mod 256 — \(value) mod 256 = \(c.exitCode). The "
                       + "interpreter reports the true value, so this is a difference in how a "
                       + "result is reported, not a miscompile.")
                        .font(.system(size: 10))
                        .foregroundStyle(Palette.changeAmber)
                        .fixedSize(horizontal: false, vertical: true)
                        .padding(.top, 3)
                }
            }
        }
        .padding(10)
        .background(Palette.changeAmber.opacity(0.10), in: RoundedRectangle(cornerRadius: 8))
    }

    private func row(_ label: String, _ value: String, _ color: Color, bold: Bool = false) -> some View {
        HStack {
            Text(label).font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
            Spacer()
            Text(value)
                .font(.system(size: 12, weight: bold ? .bold : .regular, design: .monospaced))
                .foregroundStyle(color)
        }
    }

    private func stat(_ label: String, _ before: Int, _ after: Int, _ pct: Double) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(label.uppercased()).font(.system(size: 9, weight: .semibold))
                .foregroundStyle(Palette.textDim)
            Text("\(before) → \(after)")
                .font(.system(size: 13, weight: .semibold, design: .monospaced))
                .foregroundStyle(Palette.textPrimary)
            Text(String(format: "%.1f%% fewer", pct))
                .font(.system(size: 10)).foregroundStyle(Palette.insertGreen)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(9)
        .background(Color.white.opacity(0.04), in: RoundedRectangle(cornerRadius: 8))
    }

    private func block(_ title: String, _ body: String, tint: Color) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(title).font(.system(size: 9, weight: .semibold)).tracking(0.8).foregroundStyle(tint)
            Text(body).font(.system(size: 11)).foregroundStyle(Palette.textSecondary)
                .fixedSize(horizontal: false, vertical: true)
        }
    }
}
