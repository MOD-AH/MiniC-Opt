import SwiftUI
import AppKit

/// Puts the CPPIFY wordmark in the title bar, just right of the traffic lights.
///
/// It is added to the SAME AppKit view that holds the close/minimise/zoom
/// buttons and positioned from the zoom button's real frame, so it shares
/// their coordinate space: it lines up exactly, and stays lined up if the
/// system moves the buttons (a different macOS version, a window resize).
/// Positioning it from SwiftUI instead would mean guessing the title bar's
/// geometry.
struct TitlebarWordmark: NSViewRepresentable {
    func makeNSView(context: Context) -> NSView { Installer() }
    func updateNSView(_ nsView: NSView, context: Context) {}

    private final class Installer: NSView {
        private weak var logo: NSImageView?

        override func viewDidMoveToWindow() {
            super.viewDidMoveToWindow()
            install()
        }

        override func layout() {
            super.layout()
            position()
        }

        private func install() {
            guard logo == nil,
                  let zoom = window?.standardWindowButton(.zoomButton),
                  let bar = zoom.superview,
                  // 1x and 2x in one file: AppKit draws the pixel-exact one
                  // for the display, so it is never resampled on screen.
                  let image = Bundle.module.image(forResource: "wordmark")
            else { return }
            image.size = NSSize(width: 51, height: 15)          // points; the TIFF's 1x rep is exactly this
            let view = DragThroughImageView(image: image)
            view.imageScaling = .scaleNone
            view.setAccessibilityLabel("Cppify")
            bar.addSubview(view)
            logo = view
            position()
            // The title bar re-lays out the buttons on resize and full-screen.
            NotificationCenter.default.addObserver(self, selector: #selector(windowChanged),
                                                   name: NSWindow.didResizeNotification, object: window)
        }

        @objc private func windowChanged() { position() }

        private func position() {
            guard let logo, let window,
                  let zoom = window.standardWindowButton(.zoomButton),
                  let mini = window.standardWindowButton(.miniaturizeButton),
                  let bar = logo.superview
            else { return }
            let size = logo.image?.size ?? .zero
            // The same gap the system leaves between its own buttons, and
            // centred on the buttons' midline. Snapped to device pixels, not
            // whole points: on a Retina display that keeps the exact half-point
            // centre instead of sitting half a point low.
            let gap = zoom.frame.minX - mini.frame.maxX
            let rect = NSRect(x: zoom.frame.maxX + gap, y: zoom.frame.midY - size.height / 2,
                              width: size.width, height: size.height)
            logo.frame = bar.backingAlignedRect(rect, options: .alignAllEdgesNearest)
        }
    }

    /// A title bar drag that starts on the logo should still move the window.
    private final class DragThroughImageView: NSImageView {
        override var mouseDownCanMoveWindow: Bool { true }
        override func hitTest(_ point: NSPoint) -> NSView? { nil }
    }
}
