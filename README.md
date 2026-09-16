# MiniC-Opt

**An optimizing compiler for a subset of C — built from first principles, without a parser generator.**

BCSE307L Compiler Design · Course Project · Team ____

---

## What this is

MiniC-Opt compiles **MiniC** — a deliberately chosen subset of ANSI C — through a
classical pipeline, and then does the part most student compilers skip: it
**optimizes** the intermediate code using real data-flow analysis, **measures**
how much work it removed, and **proves** on every build that the program's
observable behaviour did not change.

```
source.c → tokens → AST → symbol table → three-address code
         → basic blocks → control-flow graph → data-flow facts
         → 8 optimization passes → optimized code → execute → measure
```

The design goal is not raw compiler performance. It is **legibility with proof**:
a compiler small enough that any team member can explain any part of it, and
instrumented enough that the effect of a single optimization pass is a number you
can point at.

---

## Current status — full pipeline implemented (Review 1-3 core: P1-P9, backend, interpreter)

| Stage | Module | Status |
|---|---|---|
| Lexical analysis | `src/lexer` | **Implemented and demonstrable** |
| Syntax analysis | `src/parser` | **Implemented and demonstrable** |
| Semantic analysis | `src/sema` | **Implemented and demonstrable** |
| IR generation (TAC) | `src/ir` | **Implemented and demonstrable** |
| CFG + data-flow engine | `src/cfg` | **Implemented and demonstrable** |
| Optimization passes | `src/opt` | **P1-P9 all implemented and demonstrable** |
| Code emitter + peephole | `src/backend` | **Implemented** (linearisation + stack slots; P9 peephole itself runs on the CFG, under the differential gate — see below) |
| Reference interpreter | `src/interp` | **Implemented and demonstrable** |

Pass manager (`--opt=<list>`, `--no-opt`) is implemented per the frozen
`include/pass.h` contract, with fixed-point sweeping to convergence, now
running all nine passes: `fold`, `constprop`, `copyprop` (Review 2), and
`cse`, `dce`, `unreachable`, `strength`, `licm`, `peephole` (Review 3).
The differential-execution CI gate — optimized vs. unoptimized output
through the interpreter on every push — checks the full 9-pass pipeline
AND every pass enabled completely alone, and is green (290/290 on the
current suite). `--metrics` reports per-pass transform counts plus
static/dynamic instruction counts; `--emit-asm` prints the backend's
slot-addressed pseudo-assembly (explicitly outside the differential gate
— there is no second execution oracle for it; see `docs/test_plan.md`).

Two loop passes (P7 strength reduction, P8 LICM) are deliberately scoped
to loops that already have a de-facto single preheader block, since
`cfg.h`'s frozen interface has no way to insert a new one — a documented
conservative limit, not a correctness gap; see `docs/test_plan.md` and
`src/opt/loop_util.h`. The full 60/30 benchmark and negative-test suite
from the original objectives table remains a representative subset
(14/60 positive, 15/30 negative written) rather than the complete count
— consistent with how Review 2 was finished without chasing that number
either.

---

## Build

Requires a C++17 compiler (g++ 11+ or clang++ 14+) and CMake 3.16+.

```bash
git clone https://github.com/<org>/MiniC-Opt.git
cd MiniC-Opt
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
./build/minic --dump-tokens tests/programs/gcd.c
./build/minic --help
```

Example output:

```
   #  LINE  COL   TOKEN TYPE      LEXEME
   1     1    1   KW_INT          int
   2     1    5   IDENT           gcd
   3     1    8   LPAREN          (
   ...
  52    13    1   EOF             <end of file>

52 tokens, 0 lexical errors.
```

Error reporting carries line and column on every diagnostic:

```bash
$ ./build/minic --dump-tokens tests/negative/lex01_bad_char.c
tests/negative/lex01_bad_char.c:2:15: error: unexpected character '@'
```

## Test

```bash
./scripts/run_all.sh
```

Runs the lexer over every positive benchmark (must produce zero errors) and every
negative test (must produce exactly the expected diagnostic).

---

## The MiniC language

**Included:** `int`, `char`, `float`, `void`; one-dimensional arrays; the full
arithmetic, relational and logical operator set with C precedence and
short-circuit evaluation; `if`/`else`, `while`, `for`, `do-while`, `break`,
`continue`, `return`; user-defined functions with parameters, return values and
recursion.

**Excluded, deliberately:** pointers, `struct`, `union`, `typedef`, the
preprocessor, multi-dimensional arrays, `goto`, `switch`, standard-library
linking, and native machine-code emission.

The exclusions are a contract, not an oversight — see `docs/grammar.ebnf` for the
authoritative definition of what the compiler accepts.

---

## The optimization catalogue

Every pass is stated as **a precondition established by an analysis** ⇒ **the
rewrite that precondition licenses**. No transformation runs without a proof
behind it.

| # | Pass | Analysis required | Example |
|---|---|---|---|
| P1 | Constant folding | none (syntactic) | `t = 4 * 5` → `t = 20` |
| P2 | Constant propagation | reaching definitions | `x = 3; y = x + 2` → `y = 5` |
| P3 | Copy propagation | reaching definitions | `a = b; c = a + 1` → `c = b + 1` |
| P4 | Common subexpression elimination | available expressions | `t2 = a + b` → `t2 = t1` |
| P5 | Dead-code elimination | live variables (backward) | unused `x = y * z` → deleted |
| P6 | Unreachable-code elimination | CFG reachability | body of `if(0)` → deleted |
| P7 | Strength reduction / algebra | induction variables | `i * 8` → incremented by 8 |
| P8 | Loop-invariant code motion | dominators, back edges, natural loops | `t = a*b` hoisted to pre-header |
| P9 | Peephole | pattern matching post-linearisation | `goto L1; L1: goto L2` → `goto L2` |

---

## How correctness is guaranteed

Because MiniC-Opt does not emit native code, the host CPU cannot serve as an
oracle. Instead:

1. A **reference interpreter** defines the operational semantics of the IR.
2. Every benchmark is executed **twice** — once from unoptimized IR, once from
   optimized IR — on identical inputs.
3. The two outputs must be **byte-identical**. Any divergence fails the build.

This runs in CI on every push, so "the optimizer is correct" is a continuously
re-verified claim rather than a one-time assertion.

---

## Repository layout

```
include/            public headers (token, lexer, ir, cfg, pass interfaces)
src/lexer/          hand-written DFA scanner                    [Member 1]
src/parser/         recursive descent + Pratt expression parser [Member 1]
src/sema/           scoped symbol table, type checking          [Member 2]
src/ir/             three-address code generation               [Member 3]
src/cfg/            basic blocks, CFG, iterative data-flow      [Member 3]
src/opt/            pass manager and the optimization passes    [Member 4 + all]
src/backend/        linearisation, stack slots, peephole        [Member 4]
src/interp/         reference interpreter                       [Member 4]
tests/programs/     positive benchmark suite (+ .expected)
tests/negative/     programs with one deliberate defect each
tests/unit/         GoogleTest unit tests
docs/               grammar, test plan, worked example
scripts/            build and test automation
```

---

## Team

| Member | Role | Owns | Passes |
|---|---|---|---|
| 1 | Project Lead / Problem Analyst | lexer, parser, AST, grammar | P1, P7 |
| 2 | Background & Requirements Analyst | semantics, symbol table, errors, metrics | P5, P6 |
| 3 | System Designer / Core Algorithms | IR generator, CFG, data-flow engine | P4, P8 |
| 4 | Implementation, Testing & Planning | pass manager, emitter, interpreter, CI | P2, P3, P9 |

Every module has a designated secondary owner so that no single absence can
stall the project.

---

## References

Aho, Lam, Sethi & Ullman, *Compilers: Principles, Techniques and Tools*, 2nd ed. ·
Cooper & Torczon, *Engineering a Compiler*, 2nd ed. ·
Muchnick, *Advanced Compiler Design and Implementation* ·
Kildall (1973), *A unified approach to global program optimization*, POPL.

## License

MIT — see `LICENSE`.
