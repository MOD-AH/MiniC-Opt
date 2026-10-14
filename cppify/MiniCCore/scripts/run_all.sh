#!/usr/bin/env bash
# MiniC-Opt — build and run the full test suite.
# Owner: Member 4
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# macOS ships no GNU `timeout`; fall back to running the command untimed
# rather than failing every check with "command not found" (exit 127).
if command -v timeout >/dev/null 2>&1; then
  timeout() { command timeout "$@"; }
elif command -v gtimeout >/dev/null 2>&1; then
  timeout() { command gtimeout "$@"; }
else
  timeout() { shift; "$@"; }
fi

BIN="build/minic"
pass=0; fail=0

hdr() { printf '\n\033[1m%s\033[0m\n' "$1"; }
ok()  { printf '  \033[32mPASS\033[0m  %s\n' "$1"; pass=$((pass+1)); }
no()  { printf '  \033[31mFAIL\033[0m  %s\n' "$1"; fail=$((fail+1)); }

hdr "Building"
# CMake is the project's build system and what CI uses. It is not installed
# everywhere, though (stock macOS has no cmake), and the test suite is far
# more useful than the build system it was configured with — so fall back to
# compiling the sources directly rather than refusing to run at all. The
# source list is globbed, not hardcoded, so a newly added pass needs no edit
# here: every .cpp under src/ is exactly the minic target (verified against
# CMakeLists.txt).
if command -v cmake > /dev/null 2>&1; then
  cmake -B build -DCMAKE_BUILD_TYPE=Release > /dev/null || { echo "cmake configure failed"; exit 1; }
  cmake --build build --parallel           > /dev/null || { echo "build failed"; exit 1; }
  echo "  build/minic ready (cmake)"
else
  mkdir -p build
  # shellcheck disable=SC2046
  ${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
      $(find src -name '*.cpp' | sort) -o "$BIN" \
      || { echo "direct build failed"; exit 1; }
  echo "  build/minic ready (cmake not found — compiled directly)"
fi

hdr "Unit tests (tests/unit) — framework-level assertions over each stage"
# Built directly rather than through the cmake target so this section works
# on the no-cmake path too. One ok/no for the suite as a whole; the runner
# prints per-case detail itself when something fails.
UNIT_BIN="build/unit_tests"
if ${CXX:-c++} -std=c++17 -O1 -Iinclude -Itests/unit \
      tests/unit/unit_main.cpp tests/unit/test_lexer.cpp \
      tests/unit/test_symbol_cfg.cpp tests/unit/test_dataflow_ir.cpp \
      $(find src -name '*.cpp' ! -name 'main.cpp' | sort) \
      -o "$UNIT_BIN" > /dev/null 2>&1; then
  unit_out="$("$UNIT_BIN" 2>&1)"
  if [ $? -eq 0 ]; then
    ok "unit suite  →  $(echo "$unit_out" | tail -1 | sed 's/^ *//')"
  else
    no "unit suite"
    echo "$unit_out" | sed 's/^/    /'
  fi
else
  no "unit suite (failed to build)"
fi

hdr "Positive suite — lexer must report zero errors"
for f in tests/programs/*.c; do
  if "$BIN" --dump-tokens "$f" > /dev/null 2>&1; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

# The negative suite is organised by the stage that is SUPPOSED to catch each
# file: lex*.c are lexically malformed, syn*.c parse-malformed, sem*.c
# type-malformed. Each sweep below therefore only covers the files whose defect
# is visible by that stage — a sem*.c file is lexically and syntactically valid
# by construction, so demanding a lexer diagnostic for it would be a bug in the
# harness, not in the compiler. The --check sweep further down is the one that
# must catch every file, whatever its stage.
hdr "Negative suite — lexer must report at least one diagnostic (lex*.c)"
for f in tests/negative/lex*.c; do
  if "$BIN" --dump-tokens "$f" > /dev/null 2>&1; then
    no "$(basename "$f")  (expected an error, got none)"
  else
    ok "$(basename "$f")  →  $("$BIN" --dump-tokens "$f" 2>&1 >/dev/null | head -1 | cut -d' ' -f2-)"
  fi
done

hdr "Positive suite — parser must report zero errors (--dump-ast)"
for f in tests/programs/*.c; do
  if timeout 5 "$BIN" --dump-ast "$f" > /dev/null 2>&1; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "Negative suite — parser must report at least one diagnostic (lex*.c, syn*.c; --dump-ast), and never hang"
for f in tests/negative/lex*.c tests/negative/syn*.c; do
  timeout 5 "$BIN" --dump-ast "$f" > /dev/null 2>&1
  rc=$?
  if [ "$rc" -eq 124 ]; then
    no "$(basename "$f")  (timed out — parser hung)"
  elif [ "$rc" -eq 0 ]; then
    no "$(basename "$f")  (expected an error, got none)"
  else
    ok "$(basename "$f")  →  $(timeout 5 "$BIN" --dump-ast "$f" 2>&1 >/dev/null | tail -2 | head -1 | cut -d' ' -f2-)"
  fi
done

hdr "Positive suite — sema must report zero diagnostics (--check)"
for f in tests/programs/*.c; do
  if timeout 5 "$BIN" --check "$f" > /dev/null 2>&1; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "Negative suite — at least one stage must report a diagnostic (--check), and never hang"
for f in tests/negative/*.c; do
  timeout 5 "$BIN" --check "$f" > /dev/null 2>&1
  rc=$?
  if [ "$rc" -eq 124 ]; then
    no "$(basename "$f")  (timed out — hung)"
  elif [ "$rc" -eq 0 ]; then
    no "$(basename "$f")  (expected an error, got none)"
  else
    ok "$(basename "$f")  →  $(timeout 5 "$BIN" --check "$f" 2>&1 >/dev/null | tail -2 | head -1 | cut -d' ' -f2-)"
  fi
done

hdr "Positive suite — IR generation must succeed (--dump-ir), and never hang"
for f in tests/programs/*.c; do
  if timeout 5 "$BIN" --dump-ir "$f" > /dev/null 2>&1; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "Negative suite — IR generation must refuse (--dump-ir), and never hang"
for f in tests/negative/*.c; do
  timeout 5 "$BIN" --dump-ir "$f" > /dev/null 2>&1
  rc=$?
  if [ "$rc" -eq 124 ]; then
    no "$(basename "$f")  (timed out — hung)"
  elif [ "$rc" -eq 0 ]; then
    no "$(basename "$f")  (expected refusal, IR was generated)"
  else
    ok "$(basename "$f")"
  fi
done

hdr "Objective O2 — interpreted IR must match every benchmark's .expected byte-for-byte"
for f in tests/programs/*.c; do
  base="${f%.c}"
  exp="$base.expected"
  name="$(basename "$f")"
  got="$(mktemp)"
  # A program that reads input has its stdin in <name>.in; the rest read nothing.
  in="$base.in"; [ -f "$in" ] || in=/dev/null
  timeout 5 "$BIN" --run "$f" < "$in" > "$got" 2>/dev/null
  rc=$?
  if [ "$rc" -ne 0 ]; then
    no "$name  (interpreter exited $rc)"
  elif cmp -s "$got" "$exp"; then
    ok "$name"
  else
    no "$name  (output != $(basename "$exp"))"
  fi
  rm -f "$got"
done

hdr "Boundary suite — degenerate but legal programs must not crash, hang or mis-evaluate"
# Every .expected here was computed by hand before the compiler was asked,
# so a disagreement means the compiler is wrong rather than the file being
# stale. Covers empty bodies, zero-trip loops, INT_MAX/INT_MIN literals,
# deep statement nesting, recursion depth 1000, truncation of negative
# division and modulo, and an expression exactly at the parser's depth limit.
for f in tests/boundary/*.c; do
  base="${f%.c}"
  exp="$base.expected"
  name="$(basename "$f")"
  got="$(mktemp)"
  timeout 5 "$BIN" --run "$f" > "$got" 2>/dev/null
  rc=$?
  if [ "$rc" -eq 124 ]; then
    no "$name  (timed out — hung)"
  elif [ "$rc" -ne 0 ]; then
    no "$name  (interpreter exited $rc)"
  elif cmp -s "$got" "$exp"; then
    ok "$name"
  else
    no "$name  (output != $(basename "$exp"))"
  fi
  rm -f "$got"
done

hdr "Positive suite — CFG construction must succeed (--dump-cfg), and never hang"
for f in tests/programs/*.c; do
  if timeout 5 "$BIN" --dump-cfg "$f" > /dev/null 2>&1; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "Positive suite — data-flow solver must converge on all three analyses (--dump-dataflow), and never hang"
for f in tests/programs/*.c; do
  good=1
  for kind in reach avail live; do
    timeout 5 "$BIN" --dump-dataflow=$kind "$f" > /dev/null 2>&1 || good=0
  done
  if [ "$good" -eq 1 ]; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "Optimizer micro-tests (tests/opt) — each pass, run alone or combined, must reproduce .expected"
OPT_FLAGS_SWEEP=(
  "" "--no-opt"
  "--opt=fold" "--opt=constprop" "--opt=copyprop" "--opt=cse" "--opt=dce"
  "--opt=unreachable" "--opt=strength" "--opt=licm" "--opt=peephole"
  "--opt=fold,constprop,copyprop,cse,dce,unreachable,strength,licm,peephole"
)
for f in tests/opt/*.c; do
  base="${f%.c}"; exp="$base.expected"
  name="$(basename "$f")"
  good=1
  for optflag in "${OPT_FLAGS_SWEEP[@]}"; do
    got="$(mktemp)"
    timeout 5 "$BIN" --run $optflag "$f" > "$got" 2>/dev/null || good=0
    cmp -s "$got" "$exp" || good=0
    rm -f "$got"
  done
  if [ "$good" -eq 1 ]; then ok "$name"; else no "$name"; fi
done

hdr "Backend emitter and --metrics smoke test (not covered by the differential gate — see docs/test_plan.md)"
for f in tests/programs/*.c; do
  good=1
  timeout 5 "$BIN" --emit-asm --opt=fold,constprop,copyprop,cse,dce,unreachable,strength,licm,peephole "$f" > /dev/null 2>&1 || good=0
  timeout 5 "$BIN" --metrics "$f" > /dev/null 2>&1 || good=0
  if [ "$good" -eq 1 ]; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "Golden-IR regression (see scripts/golden.sh for detail)"
if ./scripts/golden.sh > /tmp/minic_golden_$$.log 2>&1; then
  ok "committed IR listings match for all 9 passes plus the full pipeline"
else
  no "golden IR mismatch — see below"
  cat /tmp/minic_golden_$$.log
fi
rm -f /tmp/minic_golden_$$.log

hdr "Performance and scalability (see scripts/perf.sh for detail)"
if ./scripts/perf.sh > /tmp/minic_perf_$$.log 2>&1; then
  ok "$(grep -c PASS /tmp/minic_perf_$$.log) timing cases within budget"
else
  no "performance budget exceeded — see below"
  cat /tmp/minic_perf_$$.log
fi
rm -f /tmp/minic_perf_$$.log

hdr "Differential-execution gate (see scripts/differential.sh for detail)"
if ./scripts/differential.sh > /tmp/minic_differential_$$.log 2>&1; then
  ok "unoptimized == full P1-P9 pipeline == each pass alone, on every benchmark and micro-test"
else
  no "differential mismatch — see below"
  cat /tmp/minic_differential_$$.log
fi
rm -f /tmp/minic_differential_$$.log


hdr "Summary"
printf "  %d passed, %d failed\n\n" "$pass" "$fail"
[ "$fail" -eq 0 ]
