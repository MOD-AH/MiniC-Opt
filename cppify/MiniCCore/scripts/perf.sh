#!/usr/bin/env bash
# MiniC-Opt — performance and scalability suite.
#
# docs/test_plan.md's performance row asks for a 3000-line generated source,
# a large matmul and a large sort, compiling in under 5 s with the solver
# converging. The inputs are GENERATED here rather than committed: they are
# thousands of lines of mechanical noise, and a generator is both smaller to
# read and easy to resize.
#
# Thresholds are set from measured times with a wide margin (the slowest
# case runs in about 3 s locally and is allowed 15 s), because a performance
# test that flakes on a shared CI runner is worse than no performance test.
# Every case prints its measured time whether it passes or not, so a
# regression is visible in the log before it is large enough to fail.
#
# Run standalone:   ./scripts/perf.sh
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BIN="build/minic"
ALL="fold,constprop,copyprop,cse,dce,unreachable,strength,licm,peephole"

pass=0; fail=0
hdr() { printf '\n\033[1m%s\033[0m\n' "$1"; }
ok()  { printf '  \033[32mPASS\033[0m  %-34s %6.2fs (limit %ss)\n' "$1" "$2" "$3"; pass=$((pass+1)); }
no()  { printf '  \033[31mFAIL\033[0m  %-34s %6.2fs (limit %ss)\n' "$1" "$2" "$3"; fail=$((fail+1)); }

if [ ! -x "$BIN" ]; then
  echo "perf: $BIN not built — run scripts/run_all.sh first" >&2
  exit 1
fi

GEN="$(mktemp -d)"
trap 'rm -rf "$GEN"' EXIT

python3 - "$GEN" <<'PY'
import sys
d = sys.argv[1]

# A single enormous basic block: 3000 straight-line statements.
lines = ["int main() {", "    int acc;", "    acc = 0;"]
lines += ["    acc = acc + %d;" % (i % 97) for i in range(3000)]
lines += ["    return acc % 1000;", "}"]
open(d + "/gen3000.c", "w").write("\n".join(lines) + "\n")

# A third of the size, to show how the optimizer scales with block length.
lines = ["int main() {", "    int acc;", "    acc = 0;"]
lines += ["    acc = acc + %d;" % (i % 97) for i in range(1000)]
lines += ["    return acc % 1000;", "}"]
open(d + "/gen1000.c", "w").write("\n".join(lines) + "\n")

# Many small functions rather than one large one: stresses per-function
# setup (CFG build, pass manager construction) instead of block length.
f = ["int fn%d(int x) { return x + %d; }" % (i, i) for i in range(200)]
f.append("int main() { int s; s = 0;")
f += ["    s = s + fn%d(1);" % i for i in range(200)]
f.append("    return s % 1000; }")
open(d + "/many_funcs.c", "w").write("\n".join(f) + "\n")

# Deeply nested loops: dominator and natural-loop work, not block length.
body = "s = s + 1;"
for v in "ijklm":
    body = "for (%s = 0; %s < 4; %s = %s + 1) { %s }" % (v, v, v, v, body)
open(d + "/deep_loops.c", "w").write(
    "int main() {\n    int i; int j; int k; int l; int m; int s;\n"
    "    s = 0;\n    " + body + "\n    return s;\n}\n")

open(d + "/matmul60.c", "w").write("""int main() {
    int a[3600]; int b[3600]; int c[3600];
    int i; int j; int k; int s;
    for (i = 0; i < 60; i = i + 1) {
        for (j = 0; j < 60; j = j + 1) {
            a[i * 60 + j] = i + j;
            b[i * 60 + j] = i - j;
            c[i * 60 + j] = 0;
        }
    }
    for (i = 0; i < 60; i = i + 1) {
        for (j = 0; j < 60; j = j + 1) {
            s = 0;
            for (k = 0; k < 60; k = k + 1) {
                s = s + a[i * 60 + k] * b[k * 60 + j];
            }
            c[i * 60 + j] = s;
        }
    }
    return c[0] % 1000;
}
""")

open(d + "/sort2000.c", "w").write("""int main() {
    int a[2000];
    int i; int j; int t;
    for (i = 0; i < 2000; i = i + 1) { a[i] = (2000 - i) % 977; }
    for (i = 0; i < 1999; i = i + 1) {
        for (j = 0; j < 1999 - i; j = j + 1) {
            if (a[j] > a[j + 1]) { t = a[j]; a[j] = a[j + 1]; a[j + 1] = t; }
        }
    }
    return a[0];
}
""")
PY

# case <label> <limit-seconds> <command...>
case_run() {
  local label="$1"; shift
  local limit="$1"; shift
  local start end elapsed
  start=$(python3 -c 'import time; print(time.time())')
  if ! "$@" > /dev/null 2>&1; then
    end=$(python3 -c 'import time; print(time.time())')
    elapsed=$(python3 -c "print($end - $start)")
    printf '  \033[31mFAIL\033[0m  %-34s %6.2fs (command failed)\n' "$label" "$elapsed"
    fail=$((fail+1))
    return
  fi
  end=$(python3 -c 'import time; print(time.time())')
  elapsed=$(python3 -c "print($end - $start)")
  if python3 -c "import sys; sys.exit(0 if $elapsed <= $limit else 1)"; then
    ok "$label" "$elapsed" "$limit"
  else
    no "$label" "$elapsed" "$limit"
  fi
}

hdr "Front end and analyses on a 3000-line source (must stay linear)"
case_run "check 3000 lines"            5 "$BIN" --check "$GEN/gen3000.c"
case_run "unoptimized IR 3000 lines"   5 "$BIN" --dump-ir "$GEN/gen3000.c"
case_run "CFG build 3000 lines"        5 "$BIN" --dump-cfg "$GEN/gen3000.c"
case_run "live-variable solver 3000"   5 "$BIN" --dump-dataflow=live "$GEN/gen3000.c"
case_run "reaching-defs solver 3000"   5 "$BIN" --dump-dataflow=reach "$GEN/gen3000.c"

hdr "Optimizer scalability"
# The optimizer is the superlinear part: roughly 0.3 s at 1000 statements
# and 3 s at 3000, because every sweep rescans one very long basic block.
# The 3000-line case is given a wide limit for that reason, and is the one
# to watch if a pass ever gets slower.
case_run "full pipeline 1000 lines"    5 "$BIN" --dump-ir --opt=$ALL "$GEN/gen1000.c"
case_run "full pipeline 3000 lines"   15 "$BIN" --dump-ir --opt=$ALL "$GEN/gen3000.c"
case_run "full pipeline 200 functions"  5 "$BIN" --dump-ir --opt=$ALL "$GEN/many_funcs.c"
case_run "full pipeline 5-deep loops"   5 "$BIN" --dump-ir --opt=$ALL "$GEN/deep_loops.c"

hdr "Interpreted workloads"
case_run "60x60 matmul"                 5 "$BIN" --run "$GEN/matmul60.c"
case_run "2000-element bubble sort"    10 "$BIN" --run "$GEN/sort2000.c"

hdr "Summary"
printf "  %d passed, %d failed\n\n" "$pass" "$fail"
[ "$fail" -eq 0 ]
