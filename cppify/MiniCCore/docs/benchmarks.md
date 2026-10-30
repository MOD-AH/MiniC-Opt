# Benchmarks: classic (P1–P9) vs optimized (SSA)

Produced by `scripts/bench.py --runs 5`. The full per-program data, together with the configuration needed to reproduce it, is in `bench/results.json`.

## How it is measured

- **Programs:** all 75 programs in `tests/programs`. Each is fed its `.in` file if it has one.
- **Runs:** each program and preset is run 5 times through `minic --trace-json --preset=<p>`, and the median is reported.
- **Instruction counts:**
  - *Static* is the quads left after optimization.
  - *Dynamic* is the quads the interpreter executed, with LABEL not counted.
  - Both are deterministic, so they are the headline numbers.
- **Times:**
  - *Compile* is lex + parse + sema + irgen + optimize.
  - *Run* is the optimized program's execution.
  - Each is a median, measured separately.
  - At about 1 ms per program these are noise-level; read them as orders of magnitude only.
- **Correctness:** every run also checks that the optimized output equals the unoptimized output.
- **Saved with the results:** git commit, binary version, each preset's resolved pipeline, N, platform and date.

## Result (28 Oct 2026, macOS 27.0.1 arm64, commit 3514fd7 plus uncommitted work)

| preset | static after | dynamic after | outputs match |
|---|---:|---:|---:|
| unoptimized | 4,397 | 32,449 | |
| classic: P1–P9 to a fixed point | 3,795 | 30,850 | 75/75 |
| optimized: SSA group, then P1–P9 except P7 | **3,576** | **28,460** | 75/75 |

Compared with classic, optimized executes 7.7% fewer instructions and leaves 5.8% fewer in the code. It is fewer on 54 programs, equal on 21, and more on none.

The gains are in loop-heavy programs:

| program | unoptimized | classic | optimized |
|---|---:|---:|---:|
| big_sieve_100.c | 2,218 | 2,215 | 1,797 |
| collatz.c | 1,228 | 1,226 | 1,004 |
| sieve.c | 1,095 | 1,093 | 886 |
| count_primes.c | 1,497 | 1,445 | 1,284 |
| sample_matrix_2d.c | 1,583 | 1,236 | 1,112 |

## What does the work

Rewrites summed over the 75 programs under the optimized preset:

| pass | rewrites |
|---|---:|
| GVN | 921 |
| SCCP | 403 |
| LICM on SSA | 29 |
| mark-sweep DCE | 0 |

After the SSA group, P1–P9 find little left: 13 copy propagations, 9 dead stores, 6 CSEs.

Two of the new passes add nothing on this suite, though each does on its own test (`tests/ssa`):

- **Mark-sweep DCE:** GVN's copy folding leaves no dead SSA values here.
- **LICM on SSA:** it moves 29 instructions but no loads. Its load rule is conservative: a constant in-bounds index, or the first thing in the loop header.

## Why P7 is not in the optimized preset

The interpreter counts every quad as 1. Strength reduction replaces `t = i * K` with a copy of a mirror variable, then adds one instruction per iteration to update that mirror. Under this cost model it can only add instructions.

Measured the same way as the table above, totals with P7 versus without:

| preset | with P7 (static / dynamic) | without P7 (static / dynamic) |
|---|---:|---:|
| optimized | 3,596 / 28,521 | 3,576 / 28,460 |
| classic | 3,795 / 30,850 | 3,777 / 30,800 |

On real hardware a multiply costs more than an add, and P7 would pay off there. The classic preset keeps it so that it stays the original compiler's pipeline.

## Execution engines (roadmap 3.2)

The same `scripts/bench.py` run times the three engines on the longer programs in `bench/programs`.

- **Same IR:** the quad interpreter (`--run`) and the bytecode VM (`--run-vm`) both run the optimized IR.
- **No IR:** the AST reference interpreter (`--run-ast`) has no IR, so all of its time counts as run time.
- **Timing:** each figure is the median of 15 runs, timed as a whole process (5 proved too few: one slow compile made a run time come out negative).
- **Compile time:** `--compile` stops after the optimizer and is timed separately; it is subtracted to leave run time.
- **Correctness:** all three engines must print the same output, and they do.

Times are in milliseconds.

| program | compile | quad run | VM run | AST run | VM speed-up | agree |
|---|---:|---:|---:|---:|---:|:-:|
| bubble_800.c | 4.5 | 112.4 | 22.4 | 132.6 | 5.0x | yes |
| collatz_30k.c | 4.6 | 698.9 | 174.9 | 625.1 | 4.0x | yes |
| fib_rec_24.c | 4.4 | 66.8 | 17.3 | 47.5 | 3.9x | yes |
| matmul_48.c | 6.7 | 83.0 | 18.2 | 81.9 | 4.6x | yes |
| sieve_200k.c | 36.2 | 140.8 | 41.0 | 128.0 | 3.4x | yes |

**Why the VM is faster.** The quad interpreter looks every operand up by name in a per-call hash map, and finds every jump target by searching a label table. The VM does that work once, before running. Each name becomes a register index and each label an instruction index. Execution is then a single `switch` over a flat instruction array.

**Why the AST interpreter is slow.** It is no faster than the quad interpreter, and that is expected: it exists to be a second, independent answer, not a fast one.

**A finding from adding `--compile`.** `sieve_200k.c` takes much longer to compile than the others (see the compile column). IRGen zero-fills a global array one element at a time: 200,000 stores for `int flags[200000]`. Not fixed yet.
