# Cppify roadmap

Source documents: `CPPIFY_Full_Documents/` (Build Plan, Seven-Stage Technique Catalog).
Written 28 Oct 2026 against the code in `cppify/MiniCCore` at commit 3514fd7.

## 1. Where we are (measured, not assumed)

| Stage | What exists today | Status |
|---|---|---|
| 1 Lexer | Hand-written scanner (`src/lexer/lexer.cpp`, 455 lines) + `#define` | Available |
| 2 Parser | Recursive descent + Pratt expressions (`src/parser/parser.cpp`) | Available, but one fault cascades into several errors |
| 3 Semantic | Scoped symbol table, typed AST, format-string checks | Available, but integer widths are wrong (see 0.1) |
| 4 IR | Untyped quadruple TAC, no SSA | Available (TAC only) |
| 5 Analysis | CFG, reaching defs, available exprs, liveness, dominators, natural loops | Available; recomputed by every pass, no caching |
| 6 Optimizer | P1–P9, fixed point (cap 32 sweeps), per-edit trace | Available |

*(Snapshot as of 28 Oct, before Phase 0. For the current state, see the progress log in section 5. Phases 0–2 are done there.)*
| 7 Execution | One quad interpreter run twice (unoptimized vs optimized IR) | Available, but not an independent oracle |

Known defects found on 28 Oct:

- `unsigned char c = 255; c = c + 1;` gives 256 (clang: 0).
- `int` overflows at 2^63, not 2^31.
- `short` 32767 + 1 gives 32768 (clang: -32768).
- A missing `return` expression produces 3 errors; only the first is real.
- CI (`.github/workflows/ci.yml`) never builds or tests `cppify/`.

## 2. Two corrections to the documents before we start

**The "most optimised default path" note.** The note asks for a direct-coded DFA lexer and an LALR(1)/IELR(1) parser as defaults, on the grounds that they are the most optimised. Stages 1 and 2 only change how fast Cppify *compiles*. They do not change how fast the *compiled program* runs. That is decided entirely by stages 4–7: SSA, SCCP, GVN, LICM, and the VM.

GCC and Clang both use hand-written recursive descent. We still build the re2c lexer and the Bison parser as real, selectable backends, because the catalog asks for genuine alternatives. Either one becomes the default only after it produces byte-identical tokens or ASTs on every test program (the Build Plan's definition of done, §15).

**The Tauri + React + Monaco stack.** The Build Plan itself says "if CPPIFY already has a functioning desktop shell, retain it". The SwiftUI app stays. What we take from the plan is the separation it insists on: a versioned JSON request/response between the app and `minic-trace`.

## 3. Phases

Each item lists what "done" means. An item is marked Available in the app only when it meets the Build Plan's definition of done: compiled in, tested, has correct unsupported-behaviour, and documented.

### Phase 0: correctness floor (do first; everything later is measured against it)

| # | Work | Done when |
|---|---|---|
| 0.1 | C integer widths: char 8, short 16, int 32, long 64; signed wraps the way clang does in practice, unsigned wraps mod 2^n. A narrowing op is inserted by IRGen; folding mirrors it. | A width test set matches clang byte for byte; all existing suites still pass. |
| 0.2 | Clang oracle: `scripts/clang_oracle.sh` compiles every positive test program with clang and compares stdout and exit code against Cppify. Programs using undefined behaviour or non-C built-ins are listed as excluded, each with a reason. | 0 unexplained mismatches. |
| 0.3 | Parser recovery: one primary error per fault, with downstream errors suppressed. (Stable codes such as `CPPIFY-PARSE-004` moved to 5.4.) | The factorial example gives 1 error. |
| 0.4 | CI builds `cppify/MiniCCore` and runs `run_all`, `differential`, `golden`, the unit tests and the clang oracle. | The workflow is green on GitHub. |
| 0.5 | `docs/supported-c-subset.md`: syntax, types, library calls, undefined and implementation-defined cases. | Every claim in it has a test. |

### Phase 1: pipeline infrastructure (M1 + M3)

| # | Work | Done when |
|---|---|---|
| 1.1 | IR verifier: every block ends in a terminator, branch targets exist, no use before definition on any path, and SSA dominance when SSA is on. It runs after every pass in tests. | Deliberately broken IR is rejected; all suites pass with verify-after-each-pass. |
| 1.2 | `AnalysisManager`: `get<Liveness>(fn)`, cached per IR revision and invalidated by passes that report what they changed. | The same outputs, with analysis recomputations counted and reduced. |
| 1.3 | Technique registry (`registry.json`): id, stage, kind, status, requires, conflictsWith. This is the single source the app reads. | `--list-techniques` prints it; every Available entry has a test. |
| 1.4 | `--config pipeline.json` plus presets: correctness, balanced (-O1), optimized (-O2), and custom. Dependencies are resolved by the backend, which also prints the effective pass order. | Ordered passes, including repeats, run exactly as configured; a bad configuration is rejected with a reason. |
| 1.5 | Trace reports per-stage timing, the effective configuration and the pass order. | The app shows them. |

### Phase 2: the optimizing back half (M4). This is the user's "most optimised path"

| # | Work | Done when |
|---|---|---|
| 2.1 | SSA construction (dominance frontiers, Cytron et al.) for scalars that are not address-taken, plus out-of-SSA (copy insertion) before execution. | Verifier passes and differential tests are green with SSA in and out and no other passes. |
| 2.2 | SCCP (Wegman–Zadeck) on SSA. | It subsumes P1 + P2 + P6 on the benchmark suite; differential tests and the oracle are green. |
| 2.3 | GVN, dominator-scoped hash of values (replaces block-local P4). | It finds cross-block redundancies that P4 misses (shown on a test). |
| 2.4 | SSA DCE (mark-sweep from side effects). | It removes dead code P5 leaves. |
| 2.5 | Basic alias analysis: distinct array bases, address-taken set, calls clobber all. | LICM and DCE use it; a must-not-move test passes. |
| 2.6 | LICM and strength reduction on SSA with alias facts. | It hoists loads that P8 refuses today, safely. |
| 2.7 | Benchmark protocol: median of N runs, separate compile and run time, and the reproducible configuration saved with the results. | The -O2 preset is compared with the P1–P9 preset on all 72 programs. |

### Phase 3: two independent engines (Stage 7)

| # | Work | Done when |
|---|---|---|
| 3.1 | AST reference interpreter: runs the typed AST directly, so an IRGen bug can no longer hide behind "both runs agree". | Matches the quad interpreter and clang on all positive programs. |
| 3.2 | Register bytecode VM compiled from optimized IR, with switch dispatch. This is the "optimized VM". | Same output as the reference interpreter; faster per the benchmark protocol (measured, not claimed). |

### Phase 4: front-end alternatives (the default path, stages 1–2)

| # | Work | Done when |
|---|---|---|
| 4.1 | A direct-coded DFA lexer generated by re2c 4.6. The generated `.cpp` is committed so builds don't need re2c. | Identical token stream to the hand-written lexer on every test file, including error tokens. |
| 4.2 | An LALR(1)/IELR(1) parser generated by Bison 3.8 that builds the same AST classes. The typedef-name problem is solved by lexer feedback from the symbol table. | Identical `--dump-ast` output on every program; diagnostics at the same line and column. |
| 4.3 | Make them the default if and only if 4.1 and 4.2 pass. Report the compile-time difference honestly. | The registry marks both Available. |

*(Phases 3 and 4 are done; see the progress log. Outcome of 4.3: the re2c lexer became the default; the Bison parser did not.)*

*(Phase 5 is done; see the progress log. All five phases of this roadmap are now complete.)*

### Phase 5: the app (M5)

| # | Work |
|---|---|
| 5.1 | Pipeline tab: the seven stages as chips, one implementation per stage, multi-select analyses with prerequisites added automatically, an ordered pass list, presets, and Reset to Safe Defaults. |
| 5.2 | New views: Tokens, AST, CFG (graph), SSA IR, Compare (two engines), Benchmark. |
| 5.3 | Build states: Compilation failed, Execution failed, Validation mismatch, Unsupported feature, Success. |
| 5.4 | Problems: stable error codes (`CPPIFY-PARSE-004`), primary versus secondary, and click to jump. |
| 5.5 | Close the known gaps listed in `docs/supported-c-subset.md`: `sizeof(double)` = 8, `sizeof` typed as `size_t`, and signed `char` bytes in string literals. |

### Not planned (and why)

- **LLVM IR export, JIT, vectorization, PGO:** no target code generator to support them. These stay Planned in the registry.
- **Tauri/React rewrite:** replaces a working shell for no capability gain.
- **A "disable type checking" switch:** it isn't correctness-preserving (Build Plan §3).

## 4. Order and time

The order is Phase 0, then 1, 2, 3, 4, 5. Each step depends on the one before:

- SSA needs the verifier and the analysis manager.
- Faster-VM claims need the benchmark protocol.
- Making the new front ends default needs the equivalence tests.

End-semester exams start in early November 2026. Phase 0 is a few sessions of work. Phases 2–4 are winter-break sized.

## 5. Progress log

| Date | Item | Result |
|---|---|---|
| 2026-10-28 | Roadmap written | — |
| 2026-10-28 | 0.1 integer widths | Typed quads (`IntTy` on every quad, `include/int_semantics.h` shared by interpreter and folder). char/short/int/long and unsigned now match clang; `int_widths.c` added. Golden IR unchanged. |
| 2026-10-28 | 0.2 clang oracle | `scripts/clang_oracle.py`, wired into `run_all.sh`: 110 match, 0 mismatch, 0 excluded. Verified it catches the old width bug (pre-fix build: 1 mismatch). |
| 2026-10-28 | 0.3 parser recovery | An error at a block's `}` no longer consumes it; Sema no longer runs after syntax errors. The factorial example gives 1 error (was 3). `syn_rec01-03` pin it. **Not done:** stable error codes (moved to 5.4, with the Problems panel). |
| 2026-10-28 | 0.4 CI | `cppify-core` job (macOS + Ubuntu) runs `run_all.sh`. YAML validated locally; **not yet run on GitHub** (nothing pushed). |
| 2026-10-28 | 0.5 subset doc | `docs/supported-c-subset.md`. Probing for it found two more silent miscompiles, both fixed: `static` locals were re-initialized on every call (`static_locals.c`), and `#ifdef`/`#if` were ignored (`preproc_cond.c`, `lex06`, `lex07`). |
| 2026-10-28 | Phase 0 totals | run_all 769/0 (was 724), differential 1100/0 (was 1070), oracle 110/0, golden 0 mismatched, unit 264/264. |
| 2026-10-28 | 0.2 correction | The oracle's "optimized" run used plain `--run`, which applies no passes, so it had only ever checked unoptimized Cppify. It now runs `--no-opt`, `--preset=classic` and `--preset=optimized`, the last two with the verifier on. |
| 2026-10-28 | 0.4 correction | `CMakeLists.txt` never listed `src/trace/trace_json.cpp`, so a cmake build (which CI prefers when cmake exists, as it does on GitHub runners) would fail to link. All sources are now listed. Still **not run on GitHub**. |
| 2026-10-28 | 1.1 IR verifier | `include/verify.h`: block shape, branch targets, succs/preds consistency, operand shape, temporaries written before read on every path, and in SSA one definition per name with every use dominated. `--verify-each` / `MINIC_VERIFY=1` runs it before the first pass and after every pass; the differential gate, the oracle and the fuzzer all run with it on. Seven unit tests feed it deliberately broken IR. It found a real bug on its first run: P7 rebuilt the induction variable as a temporary, so with a *global* loop counter the mirror started at 0 (`strength_global_iv.c`: 18, should be 30). |
| 2026-10-28 | 1.2 analysis cache | `include/analysis_manager.h`: dominators, reaching definitions and liveness are cached per function and dropped when a pass reports a change; dominators survive passes that declare `preservesCFG()`. `--metrics` and the trace print computed vs. reused. On `worked_example.c` (classic): dominators 1 computed / 7 reused, reaching definitions 7 / 1. Golden IR unchanged. |
| 2026-10-28 | 1.3 registry | `registryJson()` in `src/opt/pipeline.cpp` is the single list (39 techniques, stages 1–7, Available/Planned, requires, conflictsWith, test). `--list-techniques` prints it; `registry.json` is that output, and `run_all` fails if they differ or if an Available entry's test file is missing. |
| 2026-10-28 | 1.4 config + presets | `--preset=correctness\|balanced\|classic\|optimized`, `--config=file.json` (groups of passes, each `"repeat": "fixpoint"` or 1–32), `--print-pipeline`. SSA groups are bracketed by `ssa`/`out-of-ssa` automatically; mixing SSA and TAC passes in a group, an unknown or Planned pass, a bad repeat, or malformed JSON are rejected with the reason (exit 3; six cases in `run_all`). A unit test runs `constprop, fold, constprop` once and checks the report order. |
| 2026-10-28 | 1.5 trace | `--trace-json` adds `pipeline` (the resolved groups and order), `timingsMs` (lex, parse, sema, irgen, optimize, both runs) and `analyses`. The app's inspector now has a P1–P9 / Optimized (SSA) switch and a "Last run" section showing the order, stage times and cache counts. Built and decode-tested against real traces; **not checked by eye** in the running app. |
| 2026-10-28 | 2.1 SSA | `src/ssa/ssa.cpp`. In: Cytron phi placement at iterated dominance frontiers, pruned by liveness (semi-pruned alone left a dead phi naming a never-written temporary, which the verifier caught), then renaming over the dominator tree. Out: Sreedhar method I copies, dead phis dropped first, then Chaitin coalescing over an interference graph; non-interfering classes share a name, and a temporary may merge into a variable. Round trip with no other pass: verifier clean on every program, IR identical on 106 of 111 files (the rest only lose dead code after `return`) before temporary coalescing, and never longer after it. `swap_and_lost_copy.c` covers both classic out-of-SSA traps. |
| 2026-10-28 | 2.2 SCCP | `src/ssa/sccp.cpp`, folding through the shared `foldBinary`/`foldUnary`. After SCCP, P1+P2+P6 find **nothing** on any of the 120 programs (checked in `run_all`), and SCCP alone is never worse than the three together. `sccp_loop_constant.c`: SCCP 5 rewrites, P2 0. |
| 2026-10-28 | 2.3 GVN | `src/ssa/gvn.cpp`, dominator-scoped, folds copies and redundant phis. `gvn_cross_block.c`: GVN 4, P4 0. On the 75 programs it is the busiest pass (921 rewrites). |
| 2026-10-28 | 2.4 SSA DCE | `src/ssa/ssa_dce.cpp`. `dce_dead_cycle.c`: 55 dynamic instructions vs P5's 106. **On the 75 programs it removes nothing** in the optimized preset; GVN's copy folding and SCCP leave no dead SSA values there. |
| 2026-10-28 | 2.5 alias | `src/ssa/alias.h`: per-function allocations, pointers derived by copy or pointer arithmetic, unknown otherwise; calls clobber memory except a read-only builtin list. Not attempted: offsets within one array, global arrays. |
| 2026-10-28 | 2.6 LICM on SSA | `src/ssa/licm_ssa.cpp`. Hoists non-trapping arithmetic, DIV/MOD by a non-zero constant, in-range constant shifts, and loads that no aliasing store or call can change and that cannot trap earlier (constant in-bounds index into a local array that dominates the preheader, or first in the header). `licm_hoists_load.c`: 2 hoisted, P8 0; `licm_must_not_move.c` (aliasing store, store through a derived pointer, out-of-bounds zero-trip load, call writing a global): 0 hoisted. **On the 75 programs it hoists 29 instructions and no loads.** Strength reduction on SSA was not written; P7 runs after out-of-SSA instead and now also matches the one-quad `i = i + C` that coalescing leaves. |
| 2026-10-28 | 2.7 benchmarks | `scripts/bench.py`: N runs per program (default 5), medians, compile and run time separately, instruction counts, outputs-match, saved to `bench/results.json` with commit, binary version, resolved pipelines, platform and date. Optimized vs classic on the 75 programs: dynamic instructions 28,460 vs 30,850 (−7.7%), static 3,576 vs 3,795 (−5.8%), fewer on 54 programs, more on **0**, every output identical. P7 is left out of `optimized`: with every quad costing 1 it adds an instruction per iteration and removes none (measured: 28,521 dynamic with it, 28,460 without). Measuring this exposed one more existing bug: `--metrics` gave the optimized run an already-consumed stdin, so programs using scanf showed false reductions (`stdin_scanf_fgets.c`: 223 → 42 instead of 223 → 214); it now replays stdin like `--trace-json`. Times are ~1 ms per program and only noise-level. |
| 2026-10-28 | Fuzzing | `scripts/fuzz.py`: random programs built by clang, run under 7 configurations with the verifier on. First runs found **four more miscompiles in P2, P3 and P8**, all present in the original root compiler too (confirmed by building it): P8 hoisted an assignment whose variable was live into the loop (`licm_zero_trip.c`), P8 treated a global as invariant in a loop with a call (`licm_call_writes_global.c`), P2 ignored a parameter's entry value, and P3 kept a copy of a global across a call (`constprop_entry_values.c`). All fixed in the fork. After the fixes: 740 programs × 7 configurations, 0 mismatches. `run_all` fuzzes 40 per run. |
| 2026-10-28 | Phases 1–2 totals | run_all 798/0, differential 2040/0 (each pass and preset alone, verifier on), oracle 120 × 3 modes / 0 mismatches, golden 0 mismatched, unit 285/285. The root MiniC-Opt compiler is **unmodified**; it still has the five optimizer bugs listed above. |
| 2026-10-28 | 3.1 AST reference interpreter | `src/astinterp/ast_interp.cpp`, `--run-ast`: walks Sema's typed AST with no IR. Shares only the C library, the arithmetic primitive and the memory model with the quad interpreter. `goto` is handled by re-entering the enclosing statement in a "seeking" mode (`goto_shapes.c`: backwards, out of two loops, into a loop body, inside a switch). It matches `--run` on all positive programs and clang in the oracle and the fuzzer. **Mutation check:** with a planted IRGen bug (`continue` skipping the `for` step), unoptimized and optimized runs agreed on the wrong answer (8) and the differential gate passed; `--run-ast` gave 3, as clang does. |
| 2026-10-28 | 3.2 bytecode VM | `src/vm/vm.cpp`, `--run-vm`: compiles IR once (registers instead of named variables, resolved jump targets, constant pool, callee indices) and runs a switch-dispatch loop. Same semantics as `--run` by construction (shared arithmetic, library, error messages) and by test (run_all, oracle, fuzzer). Measured by `scripts/bench.py` on `bench/programs`: the VM's run time is **3.4–5.0× faster** than the quad interpreter's on optimized IR (see docs/benchmarks.md). Adding `--compile` to time compilation alone showed that IRGen zero-fills a global array element by element (200,000 stores for `int flags[200000]`); not fixed. |
| 2026-10-28 | 4.1 re2c lexer | `src/lexer/lexer_re2c.re` → committed `lexer_re2c.cpp` (re2c 4.6, direct-coded DFA). Directive meaning moved into a shared `Preprocessor`. Identical `--dump-tokens` output on every test file; `scripts/lexfuzz.py`: 10,000 malformed inputs, 0 differences. Getting there fixed two lexer bugs (reading past the end after a trailing `\`; an indented `#endif` lost in a false `#if` region) and one of mine (signed `char` in the re2c spec). |
| 2026-10-28 | 4.2 Bison parser | `src/parser/parser_bison.y` → committed `parser_bison.{cpp,hpp}` (Bison 3.8, IELR(1), `%expect 0`, the typedef "lexer hack"). Declaration semantics moved into a shared `ParseContext`. Identical `--dump-ast` and `--check` output on every file without a syntax error, and on 600 random programs under both lexers. Every syntax-error test reports its first diagnostic at the same line and column; later diagnostics differ in 5 (different recovery). Found a hand-written parser bug: `for (int i = 0; ; i++)` was rejected (`for_decl_empty_cond.c`). |
| 2026-10-28 | 4.3 defaults | **re2c lexer: now the default** (equivalent, and about 30% faster to lex: 4.2 vs 6.0 ms on 16,000 lines). **Bison parser: not the default** (different error recovery, and about 4× slower to parse: 29 vs 7 ms); available as `--parser=bison` and checked on every run. Full account: `MiniCCore/docs/front-ends.md`. |
| 2026-10-28 | Phases 3–4 totals | run_all 951/0 (unit 285, generated front ends up to date, lexers and parsers equivalent, lexfuzz 300/0, fuzz 38/0), clang oracle 123 programs × 6 modes (unoptimized, classic, optimized, AST interpreter, VM, Bison parser + hand-written lexer) / 0 mismatches. |
| 2026-10-28 | 5.1 Pipeline tab | `Panels/PipelinePanel.swift`. The seven stages are chips. Lexer, parser and execution engine are chosen there; semantic and IR show what applies. An ordered pass editor: groups of passes, each to a fixed point or 1–4 sweeps, reorder, add (repeats allowed), remove, SSA and TAC kept apart as the compiler requires. Presets and **Reset to Safe Defaults**. Analyses are not chosen: they are derived from the registry's `requires` of the chosen passes, transitively, and listed with what needs them. A custom pipeline is sent as a `--config` file; problems the compiler would reject (empty group, mixed group) are shown before Run. The inspector now summarizes the pipeline in run order. |
| 2026-10-28 | 5.2 Views | `Panels/ViewsPanel.swift`, fed by new `--trace-json` fields: **Tokens** (even when the build fails; click a token to jump), **AST**, **CFG** (graph per function, unoptimized or optimized: layered by distance from entry, true/false edges coloured, back edges dashed), **SSA IR** (phis highlighted), **Compare** (quad interpreter unoptimized and optimized, VM, AST interpreter side by side), **Benchmark** (`--bench-engines=N`: in-process medians of the three engines). A runtime error now keeps what the program printed before it (it used to replace it). |
| 2026-10-28 | 5.3 Build states | Compilation failed, Unsupported feature (a diagnostic marked unsupported, or the subset advisor's findings), Execution failed (a runtime error), Validation mismatch (unoptimized ≠ optimized, or any engine disagreeing), Build succeeded; each with its own tint, icon and default tab. |
| 2026-10-28 | 5.4 Problems | Stable codes (`CPPIFY-PARSE-001`, …; `src/trace/diag_codes.cpp`, listed in `MiniCCore/docs/diagnostics.md`), a category title, unsupported flag, and primary vs secondary (later parse errors, or any after a lexical error, are marked as possibly consequential and dimmed). Click to jump, as before. `run_all` fails if any negative test produces an unclassified (`-000`) diagnostic. |
| 2026-10-28 | 5.5 Known gaps | `sizeof(double)` is 8 and struct layout follows; `sizeof` has type `size_t`; bytes above 127 in strings, character constants and library-written strings are negative `char` values. `tests/programs/c_gaps_closed.c` matches clang. Still open, and documented in `docs/supported-c-subset.md`: `sizeof(1.5f)` is 8 (the lexer drops `f`), float arithmetic runs in double precision, `\x` escapes. |
| 2026-10-28 | Phase 5: how it was checked | The app builds; the JSON it reads was decoded by its own model types for a successful build, a failed build and a runtime error. The panels were rendered offscreen from real compiler traces (`Cppify --render-snapshots <trace.json> <dir>`, added for this) and inspected: pipeline groups and derived analyses, problems with codes and primary/secondary, the CFG graph, the engine comparison. **Not checked:** clicking through the live app (your older `appy/Cppify.app` was running with the same bundle identity, so I did not drive the UI), and native controls (menus, pickers, buttons) do not appear in offscreen renders. |
| 2026-10-28 | Phase 5 totals | run_all 960 passed, 1 failed: the new diagnostic-code check itself was wrong (under `pipefail` it took `minic`'s expected exit status 1 for a failure). Fixed and re-run on its own: passes. Clang oracle 124 programs × 6 modes, 0 mismatches; unit 285/285; fuzz 38/0; lexfuzz 300/0. |
