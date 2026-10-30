<p align="center"><img src="branding/preview-icon.png" width="160" alt="Cppify icon"></p>

# Cppify — a native macOS front end for MiniC-Opt

A Mac app that behaves like CodeBlocks or Geany for the MiniC subset — write
code, compile, see errors, run — plus one thing no other IDE has: a
**Processing** panel that explains, pass by pass and edit by edit, exactly
what the optimizer did and why it was allowed to.

## Running it

```bash
cd cppify
./build-app.sh release     # builds the compiler, the app, and Cppify.app
open Cppify.app
```

Requires macOS 26+ (the Liquid Glass APIs) and Xcode's command-line tools.
`Cppify/Package.swift` also opens directly in Xcode as a project.

## Layout

```
cppify/
├── MiniCCore/      fork of the MiniC-Opt compiler + the trace facility
│   ├── src/ include/        started as a byte-copy of ../src, then extended
│   ├── src/trace/           --trace-json, which feeds the Processing panel
│   ├── tests/ scripts/      the original's harnesses, run against the fork
│   └── build/minic-trace    the binary the app bundles and spawns
├── Cppify/         the SwiftUI app (SwiftPM package)
├── branding/       icon + title-bar wordmark: source art and the generator
└── build-app.sh    builds both and assembles Cppify.app
```

## The original compiler is untouched

The compiler at the root of this repository (`../src`, `../include`) is the
graded, CI-green MiniC-Opt, and nothing in `cppify/` edits it. Cppify works
from a fork (`MiniCCore/`), which has since grown well beyond the original
MiniC subset; a second CI job (`cppify-core`) is meant to test it but has not
yet run on GitHub. To test the fork:

```bash
cd cppify/MiniCCore
bash scripts/run_all.sh && bash scripts/differential.sh && bash scripts/golden.sh
```

## What the language now covers

| Area | Supported |
|---|---|
| Types | int, char, float, void; pointers of any depth; arrays incl. multi-dimensional; `struct`, `typedef`, `enum` |
| Statements | if/else, while, do-while, for (with declarations), switch/case/default, break, continue, goto/labels, return |
| Operators | arithmetic, comparison, logical, bitwise, shifts, `?:`, comma, `sizeof`, casts, `&` `*` `.` `->`, all compound assignments |
| Library | printf/scanf/sscanf/sprintf (width, precision, flags), getchar/putchar/puts/fgets, `<string.h>`, `<math.h>`, `<ctype.h>`, malloc/calloc/realloc/free, atoi, rand, exit |
| Preprocessor | `#include` skipped (library always in scope); object-like `#define`, `#undef`, `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#endif` |

Still outside it: `union`, function pointers, file I/O, function-like
macros and user-defined variadic functions. The app's advisor names each of these
when a pasted program uses one.

How the new features map onto the existing IR:
a pointer is (array, offset) in the interpreter, so `*p` and `p[i]` are ordinary
`LOAD_INDEX`/`STORE_INDEX`. A scalar whose address is taken lives in a one-slot
array. A struct is a flat run of slots with fixed field offsets. The only new
opcode is `COPY_BLOCK`, for struct assignment.

| Check (vendored copy, current) | Result |
|---|---|
| Full suite (incl. 285 unit tests and a 40-seed fuzz run) | 798 / 0 |
| Differential-execution gate (every pass and preset alone, IR verifier on) | 2040 / 0 |
| Clang oracle (`scripts/clang_oracle.py`), unoptimized + classic + optimized | 120 match / 0 mismatch / 0 excluded |
| Fuzzing against clang (`scripts/fuzz.py`), last large run | 740 programs × 7 configurations, 0 mismatches |
| Golden-IR regression | 0 mismatched |

The differential gate compares Cppify with itself; the clang oracle compares it
with a real C compiler, building every positive test with clang and requiring
identical output and exit status. What the subset covers, and where it knowingly
differs from clang, is in [`MiniCCore/docs/supported-c-subset.md`](MiniCCore/docs/supported-c-subset.md).
What comes next is in [`ROADMAP.md`](ROADMAP.md).

## The optimizer

`minic --preset=<name>` picks a pipeline (`--print-pipeline` shows the order):

- **correctness:** no passes.
- **balanced:** P1–P3, P5, P6 and P9.
- **classic:** the original P1–P9 to a fixed point.
- **optimized:** an SSA group (SCCP, GVN, LICM with alias analysis, mark-sweep DCE) followed by P1–P9 without P7.

`--config=file.json` takes any ordered grouping of passes, and `--verify-each` checks the IR after every pass.

On the 75 test programs, optimized executes 7.7% fewer instructions than classic. It wins on 54 programs, loses on none, and every output is identical. See [`MiniCCore/docs/benchmarks.md`](MiniCCore/docs/benchmarks.md).

The new IR verifier and fuzzing against clang (`scripts/fuzz.py`) found five miscompiles in P2, P3, P7 and P8. They are fixed in the fork, but **they are still present in the root compiler**, which this work does not modify. Each one is listed in the ROADMAP progress log with its regression test.

## Execution engines and front ends

The same program can run on three engines:

- **`--run`:** the quad interpreter.
- **`--run-vm`:** a register bytecode VM compiled from the IR, 3.4–5.0× faster than the quad interpreter on `bench/programs`.
- **`--run-ast`:** a reference interpreter that walks the typed AST with no IR at all. It catches IRGen bugs that unoptimized-vs-optimized comparison cannot: both of those runs start from the same IR.

The front end also has two implementations of each stage:

- **Lexer:** an re2c-generated DFA, now the default, and the hand-written scanner.
- **Parser:** the hand-written recursive-descent parser, still the default, and a Bison IELR(1) parser.

Both pairs are checked token for token and AST for AST on every run. See [`MiniCCore/docs/front-ends.md`](MiniCCore/docs/front-ends.md).

## Branding

`branding/source/` holds the original artwork. `branding/make-assets.swift`
turns it into an Icon Composer document (`branding/AppIcon.icon`, so macOS 26
draws the icon with its own mask, glass and Default / Dark / Clear / Tinted
styles) and a title-bar wordmark resampled at exact 1x and 2x pixel sizes;
`branding/package-assets.sh` compiles both into `Cppify/Resources/`.

## Why the compiler needed changing at all

`PassReport` records `{pass, transforms, instrBefore, instrAfter}` — counts,
with no record of *which* instruction changed or why. That is enough for
`--metrics` and not nearly enough to explain an optimization to a person.

So the vendored copy gains `--trace-json`, which emits one JSON document
containing every sweep, every pass, the IR before and after, and one entry
per individual rewrite carrying a reason string **written by the pass that
performed it**. Constant propagation can say *"`n#1` has exactly one reaching
definition and it assigns a constant"* because that is literally the condition
it tested. A UI diffing before/after IR could only guess, and a guess dressed
up as an explanation is worse than no explanation.

Recording is opt-in. With no recorder attached the passes behave exactly as
before, which is what keeps the numbers above valid.

## Design notes

**Liquid Glass is confined to chrome.** Toolbar, inspector and the dock frame
only. The editor and every console surface are opaque `#0F0F14`. Blurred
colour moving underneath syntax highlighting is precisely what makes code hard
to read, and readability is the one thing a compiler UI cannot trade away.

**The glass tint means something.** Cool blue idle, pulsing while compiling,
muted amber-red on failure, green on success — the window's colour is a state
readout, not decoration.

**No Metal shaders under the text view.** Live fragment shaders beneath a
scrolling text view are the documented cause of typing lag. The background is
a few blurred gradients that move only when compiler state changes.

**Line-number gutter as a sibling view.** An `NSRulerView` was tried first and
removed: installing one stopped the text view from drawing glyphs at all. The
gutter is now a plain `NSView` beside the text view, with a fixed line height and
no wrapping, so line N is always at the same y in both.

**Run means interpret.** MiniC-Opt has no native code generator; `--emit-asm`
produces slot-addressed pseudo-assembly and `--run` executes quads in the
reference interpreter. The app does not and cannot produce a `.exe`.

## The final Processing step

The timeline ends with **"vs. production compiler"**, which compiles the same
source with `clang -O0` and `-O2` and reports the real numbers. On the starter
program that is clang 38 → **3** instructions against MiniC-Opt's 27 → **13**.

The panel says clang wins, plainly. The defensible claim is not that this
compiler is better but that it is *legible*: clang hands back a binary, while
this one shows four fixed-point sweeps, names the reaching definition behind
each substitution, and explains why dead-code elimination refused to delete a
division that might trap.
