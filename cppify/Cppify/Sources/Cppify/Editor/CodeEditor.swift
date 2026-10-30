import SwiftUI
import AppKit

/// The editor is deliberately a hand-wrapped NSTextView rather than a
/// package: it keeps the app dependency-free like the compiler it fronts,
/// and MiniC's grammar is small enough that highlighting it is a modest
/// scanner rather than a project.
///
/// SwiftUI's TextEditor cannot do syntax highlighting at all, so wrapping
/// AppKit is not a preference here, it is the only route.
///
/// Layout: a line-number gutter beside the scroll view, both inside one
/// container. The gutter is a plain NSView, NOT an NSRulerView -- installing
/// a ruler on this scroll view stopped the text view from drawing its glyphs
/// (the content was present, nothing rendered).
///
/// Two deliberate choices make the gutter exact without touching any
/// TextKit layout API:
///   * every line has one fixed height (a paragraph style with equal minimum
///     and maximum), and
///   * long lines scroll horizontally instead of wrapping,
/// so line k always sits at inset + (k-1) * lineHeight and the gutter can be
/// drawn from the scroll offset alone.
struct CodeEditor: NSViewRepresentable {
    @Binding var text: String
    var errorLines: Set<Int>
    var highlightedLine: Int?

    static let lineHeight: CGFloat = 17
    static let gutterWidth: CGFloat = 50
    static let inset = NSSize(width: 8, height: 10)
    static let fontSize: CGFloat = 13

    static let paragraphStyle: NSParagraphStyle = {
        let p = NSMutableParagraphStyle()
        p.minimumLineHeight = lineHeight
        p.maximumLineHeight = lineHeight
        return p
    }()

    func makeCoordinator() -> Coordinator { Coordinator(self) }

    func makeNSView(context: Context) -> EditorContainer {
        let scroll = NSTextView.scrollableTextView()
        let textView = (scroll.documentView as? NSTextView) ?? NSTextView()

        textView.delegate = context.coordinator
        textView.isRichText = false
        textView.isAutomaticQuoteSubstitutionEnabled = false
        textView.isAutomaticDashSubstitutionEnabled = false
        textView.isAutomaticTextReplacementEnabled = false
        textView.isAutomaticSpellingCorrectionEnabled = false
        textView.allowsUndo = true
        textView.font = .monospacedSystemFont(ofSize: Self.fontSize, weight: .regular)
        // An AppKit view embedded in SwiftUI does not inherit preferredColorScheme,
        // so labelColor would resolve light-mode black on this dark background and
        // the text would be invisible. Pin both the appearance and the colour.
        textView.appearance = NSAppearance(named: .darkAqua)
        textView.textColor = NSColor(Palette.identifier)
        textView.backgroundColor = NSColor(Palette.editorBackground)
        textView.insertionPointColor = NSColor(Palette.type)
        textView.textContainerInset = Self.inset
        textView.drawsBackground = true
        textView.defaultParagraphStyle = Self.paragraphStyle
        textView.typingAttributes = [
            .font: NSFont.monospacedSystemFont(ofSize: Self.fontSize, weight: .regular),
            .foregroundColor: NSColor(Palette.identifier),
            .paragraphStyle: Self.paragraphStyle,
        ]

        // No wrapping: one logical line is one visual line, which is what lets
        // the gutter be computed rather than measured.
        textView.isHorizontallyResizable = true
        textView.isVerticallyResizable = true
        textView.maxSize = NSSize(width: CGFloat.greatestFiniteMagnitude,
                                  height: CGFloat.greatestFiniteMagnitude)
        textView.textContainer?.widthTracksTextView = false
        textView.textContainer?.containerSize = NSSize(width: CGFloat.greatestFiniteMagnitude,
                                                       height: CGFloat.greatestFiniteMagnitude)
        textView.string = text

        scroll.appearance = NSAppearance(named: .darkAqua)
        scroll.hasVerticalScroller = true
        scroll.hasHorizontalScroller = true
        scroll.drawsBackground = true
        scroll.backgroundColor = NSColor(Palette.editorBackground)

        let container = EditorContainer(scroll: scroll, textView: textView)
        context.coordinator.container = container
        DispatchQueue.main.async { context.coordinator.applyHighlighting(to: textView) }
        return container
    }

    func updateNSView(_ container: EditorContainer, context: Context) {
        guard let textView = container.scroll.documentView as? NSTextView else { return }
        context.coordinator.parent = self

        if textView.string != text {
            let selected = textView.selectedRange()
            textView.string = text
            textView.setSelectedRange(NSRange(location: min(selected.location, text.utf16.count), length: 0))
            context.coordinator.applyHighlighting(to: textView)
        }
        context.coordinator.markLines(in: textView, errors: errorLines, focus: highlightedLine)

        container.gutter.errorLines = errorLines
        container.gutter.focusLine = highlightedLine
        container.gutter.needsDisplay = true

        // Scroll only when the target CHANGES. This used to run on every
        // update, so after clicking a problem every later keystroke dragged
        // the view back to that line.
        if let line = highlightedLine, line != context.coordinator.lastScrolledLine {
            context.coordinator.scroll(container, toLine: line)
        }
        context.coordinator.lastScrolledLine = highlightedLine
    }

    @MainActor
    final class Coordinator: NSObject, NSTextViewDelegate {
        var parent: CodeEditor
        weak var container: EditorContainer?
        var lastScrolledLine: Int?
        /// Re-highlighting on every keystroke is wasteful on a long file;
        /// coalescing keeps typing responsive.
        private var pending: DispatchWorkItem?

        init(_ parent: CodeEditor) { self.parent = parent }

        func textDidChange(_ notification: Notification) {
            guard let textView = notification.object as? NSTextView else { return }
            parent.text = textView.string
            container?.gutter.needsDisplay = true

            pending?.cancel()
            let work = DispatchWorkItem { [weak self, weak textView] in
                MainActor.assumeIsolated {
                    guard let self, let textView else { return }
                    self.applyHighlighting(to: textView)
                }
            }
            pending = work
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.12, execute: work)
        }

        /// Exact because every line is the same height.
        func scroll(_ container: EditorContainer, toLine line: Int) {
            guard line >= 1 else { return }
            let y = CodeEditor.inset.height + CGFloat(line - 1) * CodeEditor.lineHeight
            (container.scroll.documentView as? NSTextView)?
                .scrollToVisible(NSRect(x: 0, y: y - 2 * CodeEditor.lineHeight,
                                        width: 1, height: 5 * CodeEditor.lineHeight))
        }

        /// Highlighting mirrors the compiler's own token categories, so what
        /// the editor colours is what the lexer actually recognises.
        func applyHighlighting(to textView: NSTextView) {
            guard let storage = textView.textStorage else { return }
            let source = textView.string
            let full = NSRange(location: 0, length: (source as NSString).length)

            storage.beginEditing()
            storage.setAttributes([
                .foregroundColor: NSColor(Palette.identifier),
                .font: NSFont.monospacedSystemFont(ofSize: CodeEditor.fontSize, weight: .regular),
                // Must be re-applied here: setAttributes replaces every
                // attribute, and losing the fixed line height would put the
                // gutter out of step with the text.
                .paragraphStyle: CodeEditor.paragraphStyle,
            ], range: full)

            for (pattern, color, options) in MiniCSyntax.rules {
                guard let regex = try? NSRegularExpression(pattern: pattern, options: options) else { continue }
                regex.enumerateMatches(in: source, range: full) { match, _, _ in
                    guard let r = match?.range else { return }
                    storage.addAttribute(.foregroundColor, value: NSColor(color), range: r)
                }
            }
            storage.endEditing()
            markLines(in: textView, errors: parent.errorLines, focus: parent.highlightedLine)
        }

        /// Tints the background of lines carrying a diagnostic, and of the
        /// line the user jumped to from the Problems list or an optimization
        /// edit.
        func markLines(in textView: NSTextView, errors: Set<Int>, focus: Int?) {
            guard let storage = textView.textStorage else { return }
            let ns = textView.string as NSString
            let full = NSRange(location: 0, length: ns.length)
            storage.beginEditing()
            storage.removeAttribute(.backgroundColor, range: full)
            var line = 1
            var index = 0
            while index < ns.length {
                let lineRange = ns.lineRange(for: NSRange(location: index, length: 0))
                if errors.contains(line) {
                    storage.addAttribute(.backgroundColor,
                                         value: NSColor(Palette.errorRed).withAlphaComponent(0.22),
                                         range: lineRange)
                } else if focus == line {
                    storage.addAttribute(.backgroundColor,
                                         value: NSColor(Palette.changeAmber).withAlphaComponent(0.20),
                                         range: lineRange)
                }
                line += 1
                index = NSMaxRange(lineRange)
                if lineRange.length == 0 { break }
            }
            storage.endEditing()
        }
    }
}

/// Gutter + scroll view, laid out side by side.
final class EditorContainer: NSView {
    let scroll: NSScrollView
    let gutter: GutterView

    init(scroll: NSScrollView, textView: NSTextView) {
        self.scroll = scroll
        self.gutter = GutterView(scroll: scroll, textView: textView)
        super.init(frame: .zero)
        addSubview(gutter)
        addSubview(scroll)

        // The gutter has no way to know the text scrolled unless told.
        scroll.contentView.postsBoundsChangedNotifications = true
        NotificationCenter.default.addObserver(
            forName: NSView.boundsDidChangeNotification,
            object: scroll.contentView, queue: .main
        ) { [weak self] _ in
            MainActor.assumeIsolated { self?.gutter.needsDisplay = true }
        }
    }

    required init?(coder: NSCoder) { fatalError("not used") }

    override func layout() {
        super.layout()
        let g = CodeEditor.gutterWidth
        gutter.frame = NSRect(x: 0, y: 0, width: g, height: bounds.height)
        scroll.frame = NSRect(x: g, y: 0, width: max(0, bounds.width - g), height: bounds.height)
    }
}

/// Line numbers, drawn from the scroll offset and the fixed line height.
final class GutterView: NSView {
    private weak var scroll: NSScrollView?
    private weak var textView: NSTextView?
    var errorLines: Set<Int> = []
    var focusLine: Int?

    init(scroll: NSScrollView, textView: NSTextView) {
        self.scroll = scroll
        self.textView = textView
        super.init(frame: .zero)
    }

    required init?(coder: NSCoder) { fatalError("not used") }

    /// Same orientation as the text view, so y grows downward in both.
    override var isFlipped: Bool { true }

    override func draw(_ dirtyRect: NSRect) {
        NSColor(Palette.consoleBackground).setFill()
        bounds.fill()
        NSColor(Palette.panelBorder).setFill()
        NSRect(x: bounds.maxX - 1, y: 0, width: 1, height: bounds.height).fill()

        guard let scroll, let textView else { return }
        let h = CodeEditor.lineHeight
        let top = CodeEditor.inset.height
        let offset = scroll.contentView.bounds.origin.y
        let total = textView.string.utf8.reduce(1) { $0 + ($1 == 10 ? 1 : 0) }

        let first = max(1, Int(floor((offset - top) / h)) + 1)
        let last = min(total, Int(ceil((offset + bounds.height - top) / h)) + 1)
        guard first <= last else { return }

        let font = NSFont.monospacedSystemFont(ofSize: 10, weight: .regular)
        for line in first...last {
            let y = top + CGFloat(line - 1) * h - offset
            let isError = errorLines.contains(line)
            let isFocus = focusLine == line

            if isError || isFocus {
                (isError ? NSColor(Palette.errorRed) : NSColor(Palette.changeAmber))
                    .withAlphaComponent(0.22).setFill()
                NSRect(x: 0, y: y, width: bounds.width - 1, height: h).fill()
            }
            let color = isError ? NSColor(Palette.errorRed)
                                : (isFocus ? NSColor(Palette.changeAmber) : NSColor(Palette.textDim))
            let attrs: [NSAttributedString.Key: Any] = [.font: font, .foregroundColor: color]
            let label = "\(line)" as NSString
            let size = label.size(withAttributes: attrs)
            label.draw(at: NSPoint(x: bounds.width - size.width - 10, y: y + (h - size.height) / 2),
                       withAttributes: attrs)
        }
    }
}

/// The MiniC language, as the lexer now defines it. Order matters: later
/// rules overwrite earlier ones, so strings come after the keywords they may
/// contain and comments come last of all.
enum MiniCSyntax {
    static let keywords = ["if", "else", "while", "for", "do", "return", "break", "continue",
                           "struct", "typedef", "enum", "sizeof", "switch", "case", "default", "goto"]
    static let types    = ["int", "char", "float", "void", "long", "short", "unsigned", "signed",
                           "double", "const", "static", "extern", "bool", "size_t", "FILE"]
    static let builtins = ["printf", "scanf", "print_int", "print_float", "print_char", "read_int",
                           "malloc", "calloc", "free", "strlen", "strcpy", "strcmp", "strcat",
                           "strcspn", "strncpy", "strchr", "strstr", "fgets", "gets", "puts", "putchar", "getchar",
                           "sprintf", "sscanf", "fprintf", "realloc", "memset", "memcpy", "atoi", "atof",
                           "abs", "sqrt", "pow", "floor", "ceil", "rand", "srand", "exit", "toupper", "tolower",
                           "isdigit", "isalpha", "isspace", "NULL", "stdin", "stdout", "stderr", "EOF"]

    static var rules: [(String, Color, NSRegularExpression.Options)] {
        [
            ("\\b(\(keywords.joined(separator: "|")))\\b", Palette.keyword, []),
            ("\\b(\(types.joined(separator: "|")))\\b", Palette.type, []),
            ("\\b(\(builtins.joined(separator: "|")))\\b", Palette.op, []),
            ("\\b0[xX][0-9a-fA-F]+[uUlL]*\\b", Palette.number, []),
            ("\\b\\d+\\.\\d+[fF]?\\b", Palette.number, []),
            ("\\b\\d+[uUlL]*\\b", Palette.number, []),
            ("[+\\-*/%=<>!&|^~]+", Palette.op, []),
            ("\"(\\\\.|[^\"\\\\\\n])*\"", Palette.charLit, []),
            ("'(\\\\.|[^'])'", Palette.charLit, []),
            ("^[ \\t]*#[^\\n]*", Palette.comment, [.anchorsMatchLines]),
            ("//[^\\n]*", Palette.comment, []),
            ("/\\*[\\s\\S]*?\\*/", Palette.comment, []),
        ]
    }
}
