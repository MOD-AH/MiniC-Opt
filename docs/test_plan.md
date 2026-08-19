# MiniC-Opt — Test Plan

Owner: Member 4 (Implementation, Testing and Planning Coordinator)

Testing is not a phase that follows implementation. In this project it is the
mechanism by which optimization is made *safe*. The differential-execution
category below is the project's primary correctness guarantee.

| Category | Count | What is tested | Pass criterion |
|---|---|---|---|
| Positive / functional | 60 (12 written) | Valid MiniC programs covering every construct | Compiles; interpreter output matches `.expected` exactly |
| Negative | 30 (5 written) | One deliberate defect each: 10 lexical, 10 syntactic, 10 semantic | Correct diagnostic, correct line and column; never crashes, never silently accepts |
| Boundary | 20 | Empty function body, empty loop body, zero-iteration loop, `INT_MAX` arithmetic, deep nesting, recursion depth 1000, single-statement program | No crash, hang, or wrong result |
| Optimization micro-tests | 30+ (3 per pass) | Programs constructed so exactly one pass can fire | Named pass reports the exact expected transformation count; IR matches golden file |
| **Differential (semantic equivalence)** | All 60, every commit | Same program compiled with optimization off and on | **Byte-identical output. Any mismatch breaks the build.** |
| Regression (golden IR) | 1 per pass per representative program | Committed expected IR listings | Any diff fails the build |
| Performance / scalability | 10 | 200×200 matmul, 10k-element sort, 3000-line generated source | Compiles in < 5 s; solver converges; memory < 256 MB |
| Unit tests | 150+ target | DFA transitions, FIRST/FOLLOW, symbol-table scoping, backpatching, leader identification, gen/kill sets, meet operators, dominators | All green in CI |

## Review 1 status

- 12 of 60 positive benchmarks written, each with an `.expected` file
- 5 of 30 negative tests written — all five produce the correct diagnostic
- `scripts/run_all.sh` runs both suites through the lexer
- CI runs the same script on every push, under both g++ and clang++

## Automation

```bash
./scripts/run_all.sh          # everything
```

A red build cannot be merged. Coverage is measured with gcov/lcov, targeting
≥ 85% line coverage on the optimization passes specifically — those are the
components where a defect is silent rather than loud.
