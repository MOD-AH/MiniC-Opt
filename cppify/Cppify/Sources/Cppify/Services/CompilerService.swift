import Foundation

/// Drives the bundled `minic-trace` binary and, for the comparison step,
/// the system clang. Everything runs out-of-process: a compiler crash or a
/// runaway interpreted program cannot take the UI down with it.
@MainActor
final class CompilerService {

    /// How long the user's program may run before it is stopped. The toolbar
    /// countdown reads this same constant, so the timer and the kill agree.
    nonisolated static let runTimeLimit: TimeInterval = 10

    enum ServiceError: LocalizedError {
        case compilerMissing
        case timedOut
        case badOutput(String)

        var errorDescription: String? {
            switch self {
            case .compilerMissing:
                return "The bundled minic-trace binary could not be found."
            case .timedOut:
                return "The program ran longer than \(Int(CompilerService.runTimeLimit)) seconds and was stopped. "
                     + "Usually a loop that never ends -- often one waiting for input the Input box does not contain "
                     + "(scanf and getchar return EOF forever once the input runs out)."
            case .badOutput(let detail):
                return "The compiler produced output the app could not read: \(detail)"
            }
        }
    }

    /// Located in the app bundle first; falls back to the development build
    /// path so the app is runnable straight from `swift run`.
    private var compilerURL: URL? {
        if let bundled = Bundle.module.url(forResource: "minic-trace", withExtension: nil) {
            return bundled
        }
        let dev = URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent()   // Services
            .deletingLastPathComponent()   // Cppify
            .deletingLastPathComponent()   // Sources
            .deletingLastPathComponent()   // Cppify (package root)
            .appendingPathComponent("MiniCCore/build/minic-trace")
        return FileManager.default.isExecutableFile(atPath: dev.path) ? dev : nil
    }

    // MARK: - Compiling

    /// Writes `source` to a scratch file and runs the full trace over it.
    /// The temp file is removed even when the run throws.
    func compile(source: String, stdin: String, passes: Set<String>, optimize: Bool) async throws -> CompileResult {
        guard let compiler = compilerURL else { throw ServiceError.compilerMissing }

        let scratch = FileManager.default.temporaryDirectory
            .appendingPathComponent("cppify-\(UUID().uuidString).c")
        try source.write(to: scratch, atomically: true, encoding: .utf8)
        defer { try? FileManager.default.removeItem(at: scratch) }

        var args = ["--trace-json"]
        if !optimize {
            args.append("--no-opt")
        } else if passes.count < Pass.all.count {
            args.append("--opt=" + Pass.all.map(\.id).filter(passes.contains).joined(separator: ","))
        }
        args.append(scratch.path)

        let out = try await run(compiler, args: args, timeout: Self.runTimeLimit, stdin: stdin)
        guard let data = out.stdout.data(using: .utf8), !data.isEmpty else {
            throw ServiceError.badOutput("empty response")
        }
        do {
            return try JSONDecoder().decode(CompileResult.self, from: data)
        } catch {
            throw ServiceError.badOutput(String(describing: error))
        }
    }

    // MARK: - The production-compiler comparison

    /// Compiles the same source with clang at -O0 and -O2 and counts emitted
    /// instructions. Returns nil when clang is unavailable or the program is
    /// not valid standalone C (for example it calls print_int, which is a
    /// MiniC builtin and not a declared C function) — the panel then shows
    /// the MiniC-Opt columns only and says why.
    func compareWithClang(source: String) async -> ClangComparison? {
        let clang = URL(fileURLWithPath: "/usr/bin/clang")
        guard FileManager.default.isExecutableFile(atPath: clang.path) else { return nil }

        let dir = FileManager.default.temporaryDirectory
            .appendingPathComponent("cppify-clang-\(UUID().uuidString)")
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: dir) }

        let src = dir.appendingPathComponent("prog.c")
        guard (try? source.write(to: src, atomically: true, encoding: .utf8)) != nil else { return nil }

        func instructionCount(level: String) async -> Int? {
            let asm = dir.appendingPathComponent("out\(level).s")
            guard let r = try? await run(clang,
                                         args: ["-std=c17", "-w", level, "-S", src.path, "-o", asm.path],
                                         timeout: 20),
                  r.exitCode == 0,
                  let text = try? String(contentsOf: asm, encoding: .utf8)
            else { return nil }
            // Count instruction lines: indented mnemonics, skipping directives
            // (which start with '.') and labels (which end with ':').
            return text.split(separator: "\n").filter { line in
                guard line.hasPrefix("\t") || line.hasPrefix("    ") else { return false }
                let t = line.trimmingCharacters(in: .whitespaces)
                return !t.isEmpty && !t.hasPrefix(".") && !t.hasSuffix(":") && !t.hasPrefix("#")
            }.count
        }

        // Build and run the -O2 binary so the panel can show what a real
        // executable returns, including the mod-256 truncation.
        func nativeExitCode() async -> Int32 {
            let exe = dir.appendingPathComponent("prog")
            guard let b = try? await run(clang, args: ["-std=c17", "-w", "-O2", src.path, "-o", exe.path], timeout: 20),
                  b.exitCode == 0,
                  let r = try? await run(exe, args: [], timeout: Self.runTimeLimit)
            else { return -1 }
            return r.exitCode
        }

        // The three clang jobs are independent, so they run at once: the step
        // costs as long as the slowest of them, not the sum of all three.
        async let o0 = instructionCount(level: "-O0")
        async let o2 = instructionCount(level: "-O2")
        async let exitCode = nativeExitCode()
        guard let i0 = await o0, let i2 = await o2 else { _ = await exitCode; return nil }
        return ClangComparison(o0Instructions: i0, o2Instructions: i2, exitCode: await exitCode)
    }

    // MARK: - Process plumbing

    private struct ProcessOutput { let stdout: String; let stderr: String; let exitCode: Int32 }

    /// Runs a process with a hard timeout. stdout and stderr are drained on
    /// background queues: a program that fills the pipe buffer while we wait
    /// on termination would otherwise deadlock.
    private func run(_ url: URL, args: [String], timeout: TimeInterval,
                     stdin: String = "") async throws -> ProcessOutput {
        try await withCheckedThrowingContinuation { continuation in
            let process = Process()
            process.executableURL = url
            process.arguments = args

            let outPipe = Pipe(), errPipe = Pipe(), inPipe = Pipe()
            process.standardOutput = outPipe
            process.standardError = errPipe
            // Without this the interpreted program sees no stdin at all and
            // every read_int() fails with "end of input". The pipe is always
            // attached and always closed, so a program that reads more than
            // was supplied gets a clean EOF rather than hanging forever.
            process.standardInput = inPipe

            let box = OutputBox()
            outPipe.fileHandleForReading.readabilityHandler = { box.appendOut($0.availableData) }
            errPipe.fileHandleForReading.readabilityHandler = { box.appendErr($0.availableData) }

            let resumed = ResumeGuard()
            process.terminationHandler = { proc in
                outPipe.fileHandleForReading.readabilityHandler = nil
                errPipe.fileHandleForReading.readabilityHandler = nil
                box.appendOut(outPipe.fileHandleForReading.availableData)
                box.appendErr(errPipe.fileHandleForReading.availableData)
                guard resumed.claim() else { return }
                continuation.resume(returning: ProcessOutput(stdout: box.outString,
                                                             stderr: box.errString,
                                                             exitCode: proc.terminationStatus))
            }

            do {
                try process.run()
                let data = Data(stdin.utf8)
                if !data.isEmpty { inPipe.fileHandleForWriting.write(data) }
                try? inPipe.fileHandleForWriting.close()
            } catch {
                guard resumed.claim() else { return }
                continuation.resume(throwing: error)
                return
            }

            DispatchQueue.global().asyncAfter(deadline: .now() + timeout) {
                guard process.isRunning else { return }
                process.terminate()
                // SIGTERM may be ignored by a tight interpreter loop.
                DispatchQueue.global().asyncAfter(deadline: .now() + 1) {
                    if process.isRunning { kill(process.processIdentifier, SIGKILL) }
                }
                guard resumed.claim() else { return }
                continuation.resume(throwing: ServiceError.timedOut)
            }
        }
    }
}

/// Collects pipe data from background queues under a lock.
private final class OutputBox: @unchecked Sendable {
    private let lock = NSLock()
    private var out = Data(), err = Data()
    func appendOut(_ d: Data) { guard !d.isEmpty else { return }; lock.lock(); out.append(d); lock.unlock() }
    func appendErr(_ d: Data) { guard !d.isEmpty else { return }; lock.lock(); err.append(d); lock.unlock() }
    var outString: String { lock.lock(); defer { lock.unlock() }; return String(decoding: out, as: UTF8.self) }
    var errString: String { lock.lock(); defer { lock.unlock() }; return String(decoding: err, as: UTF8.self) }
}

/// A continuation must be resumed exactly once. Both the termination handler
/// and the timeout can fire; whichever gets here first wins.
private final class ResumeGuard: @unchecked Sendable {
    private let lock = NSLock()
    private var done = false
    func claim() -> Bool {
        lock.lock(); defer { lock.unlock() }
        if done { return false }
        done = true
        return true
    }
}
