import SwiftUI

/// Explains the MiniC subset, and — the part that actually matters when a
/// paste from the internet produces 26 unexpected-character errors — names
/// every construct in the current source that the language does not have.
struct LanguagePanel: View {
    @Bindable var model: AppModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                let issues = model.subsetIssues

                if issues.isEmpty {
                    HStack(spacing: 8) {
                        Image(systemName: "checkmark.seal.fill").foregroundStyle(Palette.insertGreen)
                        Text("This program stays inside the MiniC subset.")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundStyle(Palette.textPrimary)
                        Spacer()
                    }
                    .padding(10)
                    .background(Palette.insertGreen.opacity(0.10), in: RoundedRectangle(cornerRadius: 8))
                } else {
                    VStack(alignment: .leading, spacing: 8) {
                        HStack(spacing: 7) {
                            Image(systemName: "exclamationmark.triangle.fill")
                                .foregroundStyle(Palette.changeAmber)
                            VStack(alignment: .leading, spacing: 2) {
                                Text("\(issues.count) construct\(issues.count == 1 ? "" : "s") outside the MiniC subset")
                                    .font(.system(size: 12, weight: .semibold))
                                    .foregroundStyle(Palette.textPrimary)
                                Text("These are not compiler bugs. MiniC is a deliberately small subset of C, fixed by the project's grammar — the errors in Problems are what the lexer says when it meets them.")
                                    .font(.system(size: 11))
                                    .foregroundStyle(Palette.textSecondary)
                                    .fixedSize(horizontal: false, vertical: true)
                            }
                        }
                        .padding(10)
                        .background(Palette.changeAmber.opacity(0.10), in: RoundedRectangle(cornerRadius: 8))

                        ForEach(issues) { issue in
                            VStack(alignment: .leading, spacing: 4) {
                                HStack(spacing: 6) {
                                    Text(issue.feature)
                                        .font(.system(size: 12, weight: .semibold))
                                        .foregroundStyle(Palette.deleteRed)
                                    if let line = issue.line {
                                        Text("first seen line \(line)")
                                            .font(.system(size: 10, design: .monospaced))
                                            .foregroundStyle(Palette.textDim)
                                    }
                                    Spacer(minLength: 0)
                                }
                                Text(issue.explanation)
                                    .font(.system(size: 11))
                                    .foregroundStyle(Palette.textSecondary)
                                    .fixedSize(horizontal: false, vertical: true)
                                if let alt = issue.alternative {
                                    HStack(alignment: .top, spacing: 5) {
                                        Image(systemName: "arrow.turn.down.right")
                                            .font(.system(size: 9)).foregroundStyle(Palette.insertGreen)
                                        Text(alt)
                                            .font(.system(size: 11))
                                            .foregroundStyle(Palette.insertGreen)
                                            .fixedSize(horizontal: false, vertical: true)
                                    }
                                }
                            }
                            .padding(10)
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .background(Color.white.opacity(0.04), in: RoundedRectangle(cornerRadius: 7))
                        }
                    }
                }

                Divider().overlay(Palette.panelBorder)

                Text("WHAT MINIC SUPPORTS")
                    .font(.system(size: 9, weight: .semibold)).tracking(0.8)
                    .foregroundStyle(Palette.textDim)

                ForEach(SubsetAdvisor.supported, id: \.0) { row in
                    HStack(alignment: .top, spacing: 10) {
                        Text(row.0)
                            .font(.system(size: 11, weight: .semibold))
                            .foregroundStyle(Palette.type)
                            .frame(width: 80, alignment: .leading)
                        Text(row.1)
                            .font(.system(size: 11, design: .monospaced))
                            .foregroundStyle(Palette.textSecondary)
                            .fixedSize(horizontal: false, vertical: true)
                        Spacer(minLength: 0)
                    }
                }

                Text("This is the language the Review-1 grammar froze, and the compiler implements it completely. Supporting printf, structs or pointers would mean building a different compiler, not fixing this one.")
                    .font(.system(size: 10))
                    .foregroundStyle(Palette.textDim)
                    .fixedSize(horizontal: false, vertical: true)
                    .padding(.top, 2)
            }
            .padding(14)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Palette.consoleBackground)
    }
}

/// stdin for the interpreted program. Without this, every read_int() fails
/// with "end of input" — the process simply had no input attached.
struct InputPanel: View {
    @Bindable var model: AppModel

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(spacing: 6) {
                Image(systemName: "keyboard").font(.system(size: 10))
                    .foregroundStyle(model.needsInput ? Palette.changeAmber : Palette.textDim)
                Text("PROGRAM INPUT")
                    .font(.system(size: 9, weight: .semibold)).tracking(0.8)
                    .foregroundStyle(model.needsInput ? Palette.changeAmber : Palette.textDim)
                Spacer()
                Text(model.needsInput ? "this program calls read_int()" : "one value per line")
                    .font(.system(size: 9))
                    .foregroundStyle(Palette.textDim)
            }
            TextEditor(text: $model.programInput)
                .font(.system(size: 11, design: .monospaced))
                .scrollContentBackground(.hidden)
                .background(Palette.editorBackground)
                .frame(height: 54)
                .overlay(RoundedRectangle(cornerRadius: 6)
                    .strokeBorder(model.needsInput ? Palette.changeAmber.opacity(0.4) : Palette.panelBorder,
                                  lineWidth: 1))
        }
        .padding(10)
        .background(Palette.consoleBackground)
    }
}
