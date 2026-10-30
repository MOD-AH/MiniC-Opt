// swift-tools-version: 6.0
import PackageDescription

// Cppify — a native macOS front end for the MiniC-Opt optimizing compiler.
//
// Built as a SwiftPM package rather than a hand-written .xcodeproj: Xcode
// opens Package.swift directly as a project, and the same tree also builds
// from the command line, which is what lets the build be verified rather
// than assumed. `make app` (see build-app.sh) wraps the product into a
// double-clickable .app bundle with the compiler binary inside it.
let package = Package(
    name: "Cppify",
    platforms: [
        // glassEffect / GlassEffectContainer are macOS 26+. Verified against
        // the installed SDK rather than taken from documentation.
        .macOS("26.0")
    ],
    targets: [
        .executableTarget(
            name: "Cppify",
            path: "Sources/Cppify",
            resources: [.copy("../../Resources/minic-trace"),
                        .copy("../../Resources/samples"),
                        .copy("../../Resources/wordmark.tiff")]
        )
    ]
)
