import SwiftUI

/// The compiler's operational state. Drives the Liquid Glass tint, so the
/// window's colour means something rather than being decoration.
enum CompilerState: Equatable {
    case idle, compiling, failed, succeeded

    var tint: Color {
        switch self {
        case .idle:      return Color(red: 0.16, green: 0.42, blue: 0.85)   // cool blue
        case .compiling: return Color(red: 0.20, green: 0.55, blue: 0.95)
        case .failed:    return Color(red: 0.75, green: 0.35, blue: 0.12)   // muted amber/red
        case .succeeded: return Color(red: 0.18, green: 0.70, blue: 0.40)   // calm green
        }
    }

    var label: String {
        switch self {
        case .idle:      return "Ready"
        case .compiling: return "Compiling…"
        case .failed:    return "Build failed"
        case .succeeded: return "Build succeeded"
        }
    }

    var symbol: String {
        switch self {
        case .idle:      return "circle"
        case .compiling: return "circle.dotted"
        case .failed:    return "xmark.circle.fill"
        case .succeeded: return "checkmark.circle.fill"
        }
    }
}

/// Opaque surfaces for anything carrying text that must stay readable.
/// Liquid Glass never goes behind code or console output — blurred colour
/// bleeding underneath syntax highlighting destroys legibility, which is
/// the one thing a compiler UI cannot trade away.
enum Palette {
    static let editorBackground  = Color(red: 0.059, green: 0.059, blue: 0.078)  // #0F0F14
    static let consoleBackground = Color(red: 0.102, green: 0.102, blue: 0.141)  // #1A1A24
    static let panelBorder       = Color.white.opacity(0.08)

    static let textPrimary   = Color(white: 0.94)
    static let textSecondary = Color(white: 0.62)
    static let textDim       = Color(white: 0.42)

    // Syntax colours, chosen for contrast against editorBackground rather
    // than to match the glass tint.
    static let keyword    = Color(red: 0.98, green: 0.47, blue: 0.64)
    static let type       = Color(red: 0.51, green: 0.78, blue: 1.00)
    static let number     = Color(red: 0.84, green: 0.73, blue: 1.00)
    static let charLit    = Color(red: 0.99, green: 0.79, blue: 0.49)
    static let comment    = Color(white: 0.45)
    static let identifier = Color(white: 0.90)
    static let op         = Color(red: 0.60, green: 0.86, blue: 0.78)

    // Functional colours: these mean something and must not be reused decoratively.
    static let errorRed    = Color(red: 1.00, green: 0.35, blue: 0.35)
    static let deleteRed   = Color(red: 0.95, green: 0.38, blue: 0.38)
    static let insertGreen = Color(red: 0.38, green: 0.85, blue: 0.52)
    static let changeAmber = Color(red: 0.98, green: 0.76, blue: 0.33)
}

extension View {
    /// Liquid Glass with a graceful fallback. The APIs are macOS 26+; the
    /// availability check keeps the app building and running on anything
    /// older rather than refusing to launch.
    @ViewBuilder
    func liquidGlass(tint: Color? = nil, in shape: some Shape = RoundedRectangle(cornerRadius: 14)) -> some View {
        if #available(macOS 26.0, *) {
            self.glassEffect(tint.map { Glass.regular.tint($0.opacity(0.22)) } ?? .regular, in: shape)
        } else {
            self.background(.regularMaterial, in: shape)
        }
    }

    /// For controls: adds the crisp 1px edge that tells a developer this is
    /// clickable rather than decorative, which borderless glass otherwise hides.
    @ViewBuilder
    func glassControl(tint: Color? = nil, cornerRadius: CGFloat = 8) -> some View {
        let shape = RoundedRectangle(cornerRadius: cornerRadius)
        if #available(macOS 26.0, *) {
            self.glassEffect(Glass.regular.tint(tint?.opacity(0.3)).interactive(), in: shape)
                .overlay(shape.strokeBorder(Color.white.opacity(0.18), lineWidth: 1))
        } else {
            self.background(.regularMaterial, in: shape)
                .overlay(shape.strokeBorder(Color.white.opacity(0.18), lineWidth: 1))
        }
    }

    /// Opaque surface for code and console text.
    func opaqueSurface(_ color: Color = Palette.editorBackground, cornerRadius: CGFloat = 10) -> some View {
        self.background(color, in: RoundedRectangle(cornerRadius: cornerRadius))
            .overlay(RoundedRectangle(cornerRadius: cornerRadius)
                .strokeBorder(Palette.panelBorder, lineWidth: 1))
    }
}
