# MiniC-Opt — Test Plan

Owner: Member 4 (Implementation, Testing and Planning Coordinator)

Testing is not a phase that follows implementation. In this project it is the
mechanism by which optimization is made *safe*. The differential-execution
category below is the project's primary correctness guarantee.

| Category | Count | What is tested | Pass criterion |
|---|---|---|---|
| Positive / functional | 60 (14 written) | Valid MiniC programs covering every construct | Compiles; interpreter output matches `.expected` exactly |
| Negative | 30 (15 written) | One deliberate defect each: 10 lexical, 10 syntactic, 10 semantic | Correct diagnostic, correct line and column; never crashes, never silently accepts |
| Boundary | 20 | Empty function body, empty loop body, zero-iteration loop, `INT_MAX` arithmetic, deep nesting, recursion depth 1000, single-statement program | No crash, hang, or wrong result |
| Optimization micro-tests | 30+ (15 written: 3 each for P1-P3, 1 each for P4-P9) | Programs constructed so exactly one pass can fire | Named pass reports the exact expected transformation count; IR matches golden file |
| **Differential (semantic equivalence)** | All 60, every commit | Same program compiled with optimization off and on | **Byte-identical output. Any mismatch breaks the build.** |
| Regression (golden IR) | 1 per pass per representative program | Committed expected IR listings | Any diff fails the build |
| Performance / scalability | 10 | 200×200 matmul, 10k-element sort, 3000-line generated source | Compiles in < 5 s; solver converges; memory < 256 MB |
| Unit tests | 150+ target | DFA transitions, FIRST/FOLLOW, symbol-table scoping, backpatching, leader identification, gen/kill sets, meet operators, dominators | All green in CI |

## Status

- 12 of 60 positive benchmarks written, each with an `.expected` file; all
  12 also type-check clean under `--check` (lexer + parser + sema)
- 15 of 30 negative tests written (5 lexical + 5 syntactic + 5 semantic —
  one per category in the table above: undeclared identifier, type
  mismatch, missing return, call-arity mismatch, non-array subscripted) —
  all fifteen produce the correct diagnostic and terminate cleanly
  (verified with a timeout, no hangs)
- **Objective O2 met:** unoptimized TAC for all 12 committed benchmarks
  executes through the reference interpreter (`--run`) and reproduces its
  `.expected` file byte-for-byte, including the two array-aliasing
  programs (`binary_search.c`, `linear_search.c`, where a callee mutates
  the caller's array through a parameter) and the one program exercising
  short-circuit `&&` inside a loop condition (`insertion_sort.c`)
- IR generation additionally spot-checked (uncommitted smoke test, not
  part of the tracked suite) on paths none of the 12 benchmarks exercise:
  `++`/`--` on both a scalar and an array element aliased through a call,
  implicit int/float conversion in both directions, and `&&`/`||` used as
  a plain value rather than a loop/if condition — all matched hand-worked
  expected output
- `scripts/run_all.sh` runs the positive suite through `--dump-tokens`,
  `--dump-ast`, `--check` and `--dump-ir`, the negative suite through all
  four (expecting a diagnostic — or a refusal to generate IR — from at
  least one stage), and then the full differential-oracle check: every
  benchmark's `--run` output against its `.expected` file
- **CFG + data-flow solver spot-checked, not just smoke-tested:** the
  report's own worked example (`docs/worked_example.md`) was hand-built as
  a test program and run through `--dump-cfg` — the resulting graph is
  exactly the four blocks the doc describes (entry, loop header, loop
  body, exit) with the single expected back edge from body to header, and
  `--run` on the same program reproduces the doc's own dynamic-instruction
  arithmetic (40 per iteration × 13 iterations = 520) exactly. Reaching
  definitions and live variables were also hand-solved on `gcd.c`'s three
  blocks and checked instruction-by-instruction against `--dump-dataflow`;
  both matched exactly. Available expressions gets the same automated
  convergence check as the other two (below) but was not separately
  hand-verified — outside this plan's explicit verify bullet for Step 5
- `scripts/run_all.sh` additionally runs the positive suite through
  `--dump-cfg` and `--dump-dataflow=reach|avail|live`, checking only that
  each converges and terminates (never hangs) — the hand-checks above are
  what establishes correctness, not this sweep
- **`PassManager` + P1–P3 implemented and wired** (`--opt=<list>`,
  `--no-opt`): P1 constant folding (syntactic), P2 constant propagation
  (licensed by reaching definitions), P3 copy propagation (reaching
  definitions on copies, plus an extra soundness condition — see below).
  9 micro-tests added under `tests/opt/`, three per pass, each checked
  against a hand-computed `.expected` value and matching Table 13's
  worked examples (`t1 = 4*5 → 20`, `x=3; y=x+2 → 5`); P3's tests use a
  function parameter and a loop variable as copy sources specifically so
  P2 cannot also fire, isolating what P3 alone is responsible for
- **Regression caught by the differential gate, not by inspection:** the
  first cut of copy propagation passed all 9 micro-tests but broke
  `gcd.c` (returned `0` instead of `21`) once run through
  `scripts/differential.sh`. The bug: propagating `x = y` into a later
  use is only sound if, in addition to `x`'s definition reaching that use
  uniquely (reaching definitions), `y` itself has not been reassigned in
  between — `gcd.c`'s loop body reassigns the copy's source between the
  copy and its use, and the naive version used the post-reassignment
  value. Fixed by restricting P3 to copies whose reaching definition is
  local to the same block being rewritten and by invalidating any
  tracked copy fact the moment its source variable is redefined
  (documented in `src/opt/propagation_util.h`). This is exactly the
  failure mode the differential-execution infrastructure exists to
  catch, and it caught it before the pass was ever trusted.
- **Differential-execution CI gate flipped on** (previously stubbed with
  `if: false`): every push now runs each of the 12 benchmarks and the 9
  optimizer micro-tests through `--run` unoptimized, `--run --no-opt`,
  `--run --opt=fold,constprop,copyprop`, and each pass individually, and
  fails the build on any disagreement or mismatch against `.expected`.
  Currently green: 21/21 across the full sweep.
- CI runs the same script on every push, under both g++ and clang++

## Status — Review 3 (P4-P9, backend, pass manager fixed point across all 9 passes)

- **P4-P9 implemented and wired** into the same `--opt=<list>`/`--no-opt`
  pass manager as P1-P3, under new names: `cse`, `dce`, `unreachable`,
  `strength`, `licm`, `peephole`. Each has a one-instruction-isolated
  micro-test under `tests/opt/` (function parameters or a loop-varying
  induction variable used specifically to keep P1-P3 from being able to
  touch the case, so the named pass is provably the one doing the work —
  the same isolation technique Step 6 used for `copyprop_param.c`/
  `copyprop_loop.c`).
- **Two conservative, documented scope limits**, in the same spirit as
  Step 5's direct-dominator deviation:
  - P7 (strength reduction) and P8 (LICM) only act on a natural loop
    whose header has exactly one predecessor outside the loop, and that
    predecessor's only successor is the header itself — i.e. a loop that
    already has a de-facto preheader. `cfg.h`'s frozen interface has no
    way to insert a brand-new block (`blocks_` is filled once, in the
    constructor, with no public mutator), so rather than extend that
    interface, a loop without this shape is simply left unoptimized
    (see `src/opt/loop_util.h`).
  - P8 additionally requires a candidate's own block to *dominate* the
    loop's tail block (guaranteeing it runs on every iteration, not just
    behind some inner conditional), and both P7 and P8 refuse to
    relocate anything that could trap (`DIV`/`MOD`, `LOAD_INDEX`) — the
    same trap-safety reasoning P5 already applies to dead-code deletion,
    now applied to *moving* code across the loop-entry boundary instead
    of deleting it.
- **The worked example (`docs/worked_example.md`) is now a committed
  benchmark** (`tests/programs/worked_example.c`, expected `520`) — every
  pass in the pipeline actually runs on it end-to-end. Its optimized IR
  does not byte-for-byte reproduce the report's own hand-derived
  intermediate form: because `a` and `b` are literal constants from the
  start, P2 constant-propagates `t`'s value (20) directly into both
  additions before CSE or LICM ever see a shared multiply to reuse or
  hoist, reaching an equally correct (and very slightly smaller) result
  by a different combination of the same five passes. Documented here
  rather than silently "fixed" to match — the doc is the specification
  for the *mechanism*, not a byte-exact acceptance test, and forcing a
  match would mean picking apart a correct optimization.
- **`tests/programs/nested_loop_break.c`** (expected `3`) is a second new
  committed benchmark, chosen specifically because a `break` out of an
  inner loop that is the last statement of an outer loop's body is the
  one common source-level shape that produces a genuine jump-to-a-jump
  chain (`goto L_inner_exit; ...; L_inner_exit: goto L_outer_header`) for
  P9 to collapse — see `tests/opt/peephole_basic.c`'s header comment for
  the full derivation.
- **The differential-execution gate now runs three ways**, not one
  (`scripts/differential.sh`): unoptimized vs. the full 9-pass pipeline,
  AND unoptimized vs. each of the 9 passes enabled completely alone —
  210 checks on the pre-Review-3 suite, 290 once the two new benchmarks
  and six new micro-tests are included, all green. Isolating each pass
  matters because a bug specific to one pass can otherwise hide behind
  another pass quietly cleaning up after it (exactly the shape of the
  Step 6 copy-propagation bug, which a combined-only check might have
  taken longer to localize).
- **Backend emitter added** (`src/backend/emit.cpp`, `--emit-asm`):
  linearizes the optimized IR into slot-addressed pseudo-assembly (each
  variable/temporary replaced by a numbered stack slot). **This is
  explicitly NOT covered by the differential-execution gate** — the
  reference interpreter is the project's only execution oracle, and it
  runs the TAC quadruples directly, not this pseudo-assembly. The
  emitter's correctness rests on being a faithful, mechanical, no-new-
  decisions transcription of already-verified IR, not on a second
  execution check. `scripts/run_all.sh` only smoke-tests that emission
  completes without error on every benchmark.
- **`--metrics` implemented**: prints per-pass transform-count totals
  (summed across every fixed-point sweep) plus static and dynamic
  instruction counts before/after, the same two numbers
  `docs/worked_example.md` hand-computes for its own case. Dynamic counts
  come from a new `Interp::instructionsExecuted` counter (every executed
  quad except `LABEL`, matching the convention the worked example's own
  hand count already used — confirmed by reproducing its 125 exactly
  before this addition existed).
- **Known pre-existing rough edge, not introduced by Review 3:** the
  earliest "negative suite" check in `scripts/run_all.sh` (lexer-level,
  `--dump-tokens`) runs against every file in `tests/negative/`, including
  the `sem0*`/`syn0*` files that are only supposed to be caught at the
  sema/parser stage — those files are lexically and (for `sem0*`)
  syntactically valid, so this specific stage-scoped check reports them
  as failures even though the later, correct stage-scoped checks
  (`--dump-ast`, `--check`) all pass for every file. Left as-is here
  since it predates this step and touching Steps 2-3's negative-suite
  harness is outside Review 3's scope.
- `scripts/run_all.sh` and `.github/workflows/ci.yml` need no changes to
  pick up the new passes automatically for `--opt=`/`--no-opt` (the pass
  manager is registered generically), but `run_all.sh`'s optimizer
  micro-test sweep and `scripts/differential.sh`'s pass list were both
  updated to name all 9 passes explicitly rather than just P1-P3.

## Automation

```bash
./scripts/run_all.sh          # everything
```

A red build cannot be merged. Coverage is measured with gcov/lcov, targeting
≥ 85% line coverage on the optimization passes specifically — those are the
components where a defect is silent rather than loud.
