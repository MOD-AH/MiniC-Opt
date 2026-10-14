import Foundation

/// MiniC covers most of the C that a course uses, but not all of it. For the
/// few constructs still outside it, the compiler's own diagnostic is a bare
/// parse error, which tells a reader nothing useful.
///
/// This scans the source for constructs the subset does not have and explains
/// each one in plain language, with the MiniC equivalent where one exists.
/// It runs in the app rather than in the compiler on purpose: the compiler's
/// diagnostic text is pinned by its own negative tests and unit tests, and
/// this is a help layer, not a change to what the language accepts.
struct SubsetIssue: Identifiable, Hashable {
    var id: String { feature }
    let feature: String
    let explanation: String
    let alternative: String?
    let line: Int?
}

enum SubsetAdvisor {

    /// What MiniC does have — shown in the UI so the limits are discoverable
    /// before someone pastes a program rather than after.
    static let supported: [(String, String)] = [
        ("Types",      "int, char, float, void, pointers (any depth), arrays incl. multi-dimensional, struct, typedef, enum"),
        ("Statements", "if / else, while, do-while, for, switch / case / default, break, continue, goto / labels, return"),
        ("Operators",  "arithmetic, comparison, logical, bitwise, shifts, ?:, comma, sizeof, casts, & and *, . and ->, every compound assignment, ++ --"),
        ("Functions",  "definitions, recursion, struct by value, pointers and arrays by reference, prototypes"),
        ("Strings",    "string literals as char *, char arrays, and <string.h>: strlen strcpy strcat strcmp strchr strstr strcspn ..."),
        ("Input / output", "printf (width, precision, flags), scanf, sscanf, sprintf, getchar, putchar, puts, fgets(..., stdin)"),
        ("Library",    "<math.h>, <stdlib.h> (malloc calloc realloc free atoi abs rand exit), <ctype.h>"),
        ("Preprocessor", "#include is skipped (the library is always in scope); object-like #define is expanded"),
        ("Tolerated",  "const / static / extern ignored; long / short / unsigned -> int; double -> float"),
    ]

    /// Ordered most-likely-first so the top of the list is usually the thing
    /// that actually broke the paste.
    static func analyse(_ source: String) -> [SubsetIssue] {
        // Block comments are blanked first (newlines kept so line numbers stay
        // right). Without this, a comment that merely MENTIONS #include or
        // printf -- which the bundled samples do, explaining what had to
        // change -- gets flagged as if the code used it.
        let lines = blankStringLiterals(stripBlockComments(source)).components(separatedBy: "\n")
        var issues: [SubsetIssue] = []
        var seen = Set<String>()

        func flag(_ feature: String, _ explanation: String, _ alternative: String?, _ line: Int?) {
            guard !seen.contains(feature) else { return }
            seen.insert(feature)
            issues.append(SubsetIssue(feature: feature, explanation: explanation,
                                      alternative: alternative, line: line))
        }

        for (i, raw) in lines.enumerated() {
            let n = i + 1
            // Strip line comments so a construct mentioned in prose is not flagged.
            let line = raw.components(separatedBy: "//").first ?? raw

            if line.range(of: "\\bunion\\b", options: .regularExpression) != nil {
                flag("union",
                     "union is not supported: every field here has its own storage, so fields cannot overlap.",
                     "Use a struct (wasting a little space) or separate variables.", n)
            }
            // `int (*fp)(int)` -- a declarator in parentheses followed by a parameter list.
            if line.range(of: "\\(\\s*\\*\\s*[A-Za-z_][A-Za-z0-9_]*\\s*\\)\\s*\\(", options: .regularExpression) != nil {
                flag("Function pointers",
                     "A function cannot be stored in a variable or passed as an argument here.",
                     "Pass an int that selects the operation and switch on it.", n)
            }
            if line.range(of: "\\b(fopen|fclose|fread|fwrite|fseek|ftell|rewind)\\s*\\(", options: .regularExpression) != nil {
                flag("File I/O",
                     "There is no file system access: only standard input and standard output exist.",
                     "Read from stdin (scanf, fgets(buf, n, stdin)) and paste the file's contents into the Input box.", n)
            }
            if line.range(of: "^\\s*#\\s*(if|ifdef|ifndef|elif|else|endif)\\b", options: .regularExpression) != nil {
                flag("Conditional compilation",
                     "#if / #ifdef lines are skipped, not evaluated, so BOTH branches are compiled.",
                     "Delete the branch you do not want before compiling.", n)
            }
            if line.range(of: "^\\s*#\\s*define\\s+[A-Za-z_][A-Za-z0-9_]*\\(", options: .regularExpression) != nil {
                flag("Function-like macros",
                     "Only object-like macros (#define N 10) are expanded; a macro with parameters is not.",
                     "Write it as an ordinary function.", n)
            }
            if line.range(of: "\\.\\.\\.\\s*\\)", options: .regularExpression) != nil {
                flag("Variadic functions",
                     "A function of your own cannot take '...' arguments (printf and scanf themselves are fine).",
                     "Pass an array and a count instead.", n)
            }
        }

        return issues
    }

    /// Line number of the first declaration that appears after a statement
    /// within the same block, or nil. Brace-depth tracked so a declaration
    /// opening a nested block is judged against that block, not the outer one.
    private static func firstMisplacedDeclaration(_ lines: [String]) -> Int? {
        let declPattern = "^\\s*(int|char|float|void)\\s+[A-Za-z_]"
        var sawStatementAtDepth: [Int: Bool] = [:]
        var depth = 0

        for (i, raw) in lines.enumerated() {
            let line = (raw.components(separatedBy: "//").first ?? raw)
                .trimmingCharacters(in: .whitespaces)
            if line.isEmpty { continue }

            let opens = line.filter { $0 == "{" }.count
            let closes = line.filter { $0 == "}" }.count
            let isDecl = line.range(of: declPattern, options: .regularExpression) != nil
                      && !line.contains("(")        // not a function definition
            let bodyOnly = line.replacingOccurrences(of: "{", with: "")
                               .replacingOccurrences(of: "}", with: "")
                               .trimmingCharacters(in: .whitespaces)

            if isDecl, sawStatementAtDepth[depth] == true {
                return i + 1
            }
            if !isDecl, !bodyOnly.isEmpty {
                sawStatementAtDepth[depth] = true
            }
            // Entering a block resets the rule for the new depth.
            if opens > closes {
                depth += opens - closes
                sawStatementAtDepth[depth] = false
            } else if closes > opens {
                depth = max(0, depth - (closes - opens))
            }
        }
        return nil
    }

    /// Replaces the CONTENTS of "..." with spaces, keeping the quotes. Text
    /// inside a format string is data, not code: without this, a printf whose
    /// format happens to contain "= , =" was reported as a comma operator.
    private static func blankStringLiterals(_ source: String) -> String {
        var out = ""
        var inStr = false
        var escaped = false
        for c in source {
            if inStr {
                if escaped { out.append(" "); escaped = false; continue }
                if c == "\\" { out.append(" "); escaped = true; continue }
                if c == "\"" { out.append(c); inStr = false; continue }
                out.append(c == "\n" ? c : " ")
                continue
            }
            if c == "\"" { out.append(c); inStr = true; continue }
            out.append(c)
        }
        return out
    }

    /// Replaces /* ... */ spans with spaces, preserving newlines so reported
    /// line numbers still match the original source.
    private static func stripBlockComments(_ source: String) -> String {
        var out = ""
        out.reserveCapacity(source.count)
        var inComment = false
        var i = source.startIndex
        while i < source.endIndex {
            let c = source[i]
            let next = source.index(after: i)
            if !inComment, c == "/", next < source.endIndex, source[next] == "*" {
                inComment = true
                out.append("  ")
                i = source.index(i, offsetBy: 2)
                continue
            }
            if inComment, c == "*", next < source.endIndex, source[next] == "/" {
                inComment = false
                out.append("  ")
                i = source.index(i, offsetBy: 2)
                continue
            }
            out.append(inComment ? (c == "\n" ? "\n" : " ") : c)
            i = next
        }
        return out
    }

    /// True when the program reads input, so the UI can point at the Input box.
    static func usesInput(_ source: String) -> Bool {
        stripBlockComments(source).range(of: "\\b(read_int|scanf|getchar|fgets|gets)\\s*\\(",
                                         options: .regularExpression) != nil
    }
}
