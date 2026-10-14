import SwiftUI

/// The lowest visual layer: a slow, soft colour field that the glass panels
/// refract. Deliberately NOT a live Metal shader — running fragment shaders
/// under a fast-scrolling text view is the documented cause of typing lag,
/// and a compiler UI has to stay responsive during a build. This is a few
/// blurred gradient blobs whose positions shift only when compiler state
/// changes, so there is nothing animating while you type.
struct BackgroundMesh: View {
    let state: CompilerState
    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    @State private var drift = false

    var body: some View {
        GeometryReader { geo in
            let w = geo.size.width, h = geo.size.height
            ZStack {
                Color.black

                blob(state.tint.opacity(0.55), size: w * 0.85)
                    .offset(x: drift ? -w * 0.22 : -w * 0.28,
                            y: drift ? -h * 0.18 : -h * 0.24)

                blob(state.tint.opacity(0.32), size: w * 0.70)
                    .offset(x: drift ? w * 0.30 : w * 0.24, y: h * 0.10)

                blob(accent.opacity(0.28), size: w * 0.55)
                    .offset(x: drift ? w * 0.05 : w * 0.12, y: h * 0.34)
            }
            .animation(.easeInOut(duration: 0.8), value: state)
            .onAppear {
                // A single very slow drift, skipped entirely when the system
                // asks for reduced motion.
                guard !reduceMotion else { return }
                withAnimation(.easeInOut(duration: 18).repeatForever(autoreverses: true)) {
                    drift = true
                }
            }
        }
    }

    /// A second hue so the field reads as a mesh rather than one flat wash.
    private var accent: Color {
        switch state {
        case .idle, .compiling: return Color(red: 0.45, green: 0.30, blue: 0.85)
        case .failed:           return Color(red: 0.85, green: 0.25, blue: 0.30)
        case .succeeded:        return Color(red: 0.20, green: 0.60, blue: 0.75)
        }
    }

    private func blob(_ color: Color, size: CGFloat) -> some View {
        Circle()
            .fill(RadialGradient(colors: [color, color.opacity(0)],
                                 center: .center, startRadius: 0, endRadius: size / 2))
            .frame(width: size, height: size)
            .blur(radius: 60)
    }
}
