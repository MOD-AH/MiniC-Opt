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
graded, CI-green MiniC-Opt, and nothing in `cppify/` edits it; CI builds and
tests only that root compiler. Cppify works from a fork (`MiniCCore/`), which
has since grown well beyond the original MiniC subset. To test the fork:

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
| Preprocessor | `#include` skipped (library always in scope); object-like `#define` expanded |

Still outside it: `union`, function pointers, file I/O, `#if`/`#ifdef`, function-like
macros and user-defined variadic functions. The app's advisor names each of these
when a pasted program uses one.

How the new features map onto the existing IR, with no change to the nine passes:
a pointer is (array, offset) in the interpreter, so `*p` and `p[i]` are ordinary
`LOAD_INDEX`/`STORE_INDEX`. A scalar whose address is taken lives in a one-slot
array. A struct is a flat run of slots with fixed field offsets. The only new
opcode is `COPY_BLOCK`, for struct assignment.

| Check (vendored copy, current) | Result |
|---|---|
| Full suite (incl. 264 unit tests) | 724 / 0 |
| Differential-execution gate | 1070 / 0 |
| Golden-IR regression | 0 mismatched |

The library-heavy tests and the new samples were checked against the output of
the same source compiled natively with clang, so they don't rely on this
compiler's own idea of what C means.

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
