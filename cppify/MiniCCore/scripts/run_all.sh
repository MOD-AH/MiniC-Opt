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
      tests/unit/test_symbol_cfg.cpp tests/unit/test_dataflow_ir.cpp tests/unit/test_pipeline.cpp \
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

hdr "Error recovery — one fault must give exactly one diagnostic (syn_rec*.c; --check)"
# Each syn_rec file holds a single syntax error placed where the old
# panic-mode recovery cascaded into several. Counting the "error:" lines
# pins the recovery, not just the presence of an error.
for f in tests/negative/syn_rec*.c; do
  n=$("$BIN" --check "$f" 2>&1 | grep -c ': error: ')
  if [ "$n" -eq 1 ]; then ok "$(basename "$f")  →  1 error"; else no "$(basename "$f")  ($n errors, expected 1)"; fi
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
  "--opt=ssa" "--opt=sccp" "--opt=gvn" "--opt=licm-ssa" "--opt=ssa-dce"
  "--preset=balanced" "--preset=optimized"
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

hdr "Generated front ends (roadmap 4.1, 4.2) — committed code matches its spec; both lexers and both parsers agree"
# The generated .cpp files are committed so a build needs neither tool. When
# the tools are here, regenerate in a scratch copy of the same layout (both
# tools write paths into their output) and require byte-identical files.
BISON="$(command -v /opt/homebrew/opt/bison/bin/bison || command -v bison)"
if command -v re2c > /dev/null 2>&1; then
  tmp="$(mktemp -d)"; mkdir -p "$tmp/src/lexer"; cp src/lexer/lexer_re2c.re "$tmp/src/lexer/"
  ( cd "$tmp" && re2c -W --no-generation-date -o src/lexer/lexer_re2c.cpp src/lexer/lexer_re2c.re 2>/dev/null )
  if cmp -s "$tmp/src/lexer/lexer_re2c.cpp" src/lexer/lexer_re2c.cpp; then
    ok "src/lexer/lexer_re2c.cpp is what re2c generates from lexer_re2c.re"
  else no "src/lexer/lexer_re2c.cpp is stale: regenerate it from lexer_re2c.re (docs/front-ends.md)"; fi
  rm -rf "$tmp"
else echo "  (re2c not installed: generated lexer not re-checked)"; fi
if [ -n "$BISON" ] && "$BISON" --version | head -1 | grep -q ' 3\.8'; then
  tmp="$(mktemp -d)"; mkdir -p "$tmp/src/parser"; cp src/parser/parser_bison.y "$tmp/src/parser/"
  ( cd "$tmp" && "$BISON" -o src/parser/parser_bison.cpp --header=src/parser/parser_bison.hpp src/parser/parser_bison.y 2>/dev/null )
  if cmp -s "$tmp/src/parser/parser_bison.cpp" src/parser/parser_bison.cpp && cmp -s "$tmp/src/parser/parser_bison.hpp" src/parser/parser_bison.hpp; then
    ok "src/parser/parser_bison.{cpp,hpp} are what Bison 3.8 generates from parser_bison.y"
  else no "src/parser/parser_bison.{cpp,hpp} are stale: regenerate them from parser_bison.y (docs/front-ends.md)"; fi
  rm -rf "$tmp"
else echo "  (Bison 3.8 not installed: generated parser not re-checked)"; fi
lexdiff=0; astdiff=0; posdiff=0; files=0
for f in $(find tests -name '*.c' | sort); do
  files=$((files+1))
  "$BIN" --dump-tokens --lexer=handwritten "$f" > /tmp/minic_t1_$$ 2>&1
  "$BIN" --dump-tokens --lexer=re2c "$f" > /tmp/minic_t2_$$ 2>&1
  cmp -s /tmp/minic_t1_$$ /tmp/minic_t2_$$ || { lexdiff=$((lexdiff+1)); echo "    tokens differ: $f"; }
  "$BIN" --check "$f" > /dev/null 2> /tmp/minic_c1_$$; "$BIN" --check --parser=bison "$f" > /dev/null 2> /tmp/minic_c2_$$
  case "$f" in
    tests/negative/syn*|tests/negative/lex*)
      # Error recovery differs by design; the first error must be at the same place.
      [ "$(head -1 /tmp/minic_c1_$$ | cut -d: -f2,3)" = "$(head -1 /tmp/minic_c2_$$ | cut -d: -f2,3)" ] \
        || { posdiff=$((posdiff+1)); echo "    first diagnostic moved: $f"; } ;;
    *)
      "$BIN" --dump-ast "$f" > /tmp/minic_a1_$$ 2>/dev/null; "$BIN" --dump-ast --parser=bison "$f" > /tmp/minic_a2_$$ 2>/dev/null
      { cmp -s /tmp/minic_a1_$$ /tmp/minic_a2_$$ && cmp -s /tmp/minic_c1_$$ /tmp/minic_c2_$$; } \
        || { astdiff=$((astdiff+1)); echo "    AST or diagnostics differ: $f"; } ;;
  esac
done
rm -f /tmp/minic_[tca][12]_$$
if [ "$lexdiff" -eq 0 ]; then ok "re2c and hand-written lexers: identical tokens and diagnostics on all $files test files"
else no "lexers differ on $lexdiff file(s)"; fi
if [ "$astdiff" -eq 0 ]; then ok "Bison and hand-written parsers: identical AST and diagnostics on every file without a syntax error"
else no "parsers differ on $astdiff file(s)"; fi
if [ "$posdiff" -eq 0 ]; then ok "Bison and hand-written parsers: first syntax error at the same line and column in every syntax-error test"
else no "first syntax error moved in $posdiff file(s)"; fi
if lf="$(python3 scripts/lexfuzz.py --count 300 2>&1)"; then ok "$(echo "$lf" | tail -1)"
else no "lexer fuzzing found a difference — see below"; echo "$lf"; fi

hdr "Diagnostic codes (roadmap 5.4) — every negative test's diagnostics classify to a specific stable code"
uncoded=0
for f in tests/negative/*.c; do
  # minic exits 1 on these (they fail to compile), so its JSON is captured
  # first rather than piped: under pipefail its status would decide the test.
  json="$("$BIN" --trace-json "$f" < /dev/null)"
  if printf '%s' "$json" | python3 -c '
import json, sys
d = json.load(sys.stdin)
sys.exit(1 if any(x.get("code", "").endswith("-000") or "code" not in x for x in d["diagnostics"]) else 0)'; then :
  else uncoded=$((uncoded+1)); echo "    uncoded diagnostic in $f"; fi
done
if [ "$uncoded" -eq 0 ]; then ok "no negative test produces a fallback (-000) or missing code"
else no "$uncoded negative test(s) produce an unclassified diagnostic"; fi

hdr "Execution engines — AST reference interpreter and bytecode VM must print exactly what --run prints"
for f in tests/programs/*.c tests/opt/*.c tests/boundary/*.c tests/ssa/*.c; do
  in="${f%.c}.in"; [ -f "$in" ] || in=/dev/null
  ref="$(mktemp)"; got="$(mktemp)"
  timeout 5 "$BIN" --run "$f" < "$in" > "$ref" 2>/dev/null; rr=$?
  good=1
  for engine in "--run-ast" "--run-vm --no-opt" "--run-vm --preset=optimized"; do
    timeout 5 "$BIN" $engine "$f" < "$in" > "$got" 2>/dev/null; rg=$?
    { [ "$rg" -eq "$rr" ] && cmp -s "$ref" "$got"; } || good=0
  done
  rm -f "$ref" "$got"
  if [ "$good" -eq 1 ]; then ok "$(basename "$f")"; else no "$(basename "$f") (engines disagree)"; fi
done

hdr "SSA micro-tests (tests/ssa) — every preset and SSA pass must reproduce .expected, verifier on"
for f in tests/ssa/*.c; do
  base="${f%.c}"; exp="$base.expected"
  good=1
  for optflag in "${OPT_FLAGS_SWEEP[@]}"; do
    got="$(mktemp)"
    MINIC_VERIFY=1 timeout 5 "$BIN" --run $optflag "$f" > "$got" 2>/dev/null || good=0
    cmp -s "$got" "$exp" || good=0
    rm -f "$got"
  done
  if [ "$good" -eq 1 ]; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "SSA passes do what their classic counterparts cannot (tests/ssa)"
# transforms <file> <--opt list> <pass>: that pass's total in --metrics.
transforms() { "$BIN" --metrics --opt="$2" "tests/ssa/$1.c" 2>/dev/null | awk -v p="  $3:" 'index($0, p) == 1 {print $2}'; }
claim() {   # claim <file> <new pass> <old pass> <what>
  local new old
  new="$(transforms "$1" "$2" "$2")"; old="$(transforms "$1" "$3" "$3")"
  if [ "${new:-0}" -gt 0 ] && [ "${old:-1}" -eq 0 ]; then ok "$4 ($2: $new, $3: $old)"
  else no "$4 ($2: ${new:-?}, $3: ${old:-?})"; fi
}
claim sccp_loop_constant sccp constprop "SCCP proves a loop-carried constant P2 cannot"
claim gvn_cross_block gvn cse "GVN removes a cross-block redundancy P4 cannot"
dyn() { "$BIN" --metrics --opt="$2" "tests/ssa/$1.c" 2>/dev/null | awk '/^dynamic/ {print $5}'; }
# P5 does delete something here (the dead initial store), so compare what runs.
d_new="$(dyn dce_dead_cycle ssa-dce)"; d_old="$(dyn dce_dead_cycle dce)"
if [ "${d_new:-0}" -gt 0 ] && [ "$d_new" -lt "${d_old:-0}" ]; then
  ok "mark-sweep DCE removes a dead cycle P5 keeps (dynamic instructions: ssa-dce $d_new, dce $d_old)"
else no "mark-sweep DCE vs P5 on dce_dead_cycle.c (ssa-dce ${d_new:-?}, dce ${d_old:-?})"; fi
claim licm_hoists_load licm-ssa licm "licm-ssa hoists a load and a constant division P8 refuses"
moved="$(transforms licm_must_not_move licm-ssa licm-ssa)"
if [ "${moved:-1}" -eq 0 ]; then ok "licm-ssa hoists nothing from licm_must_not_move.c"
else no "licm-ssa moved ${moved:-?} instructions in licm_must_not_move.c"; fi
cat > /tmp/minic_sub_$$.json <<'JSON'
{"groups": [{"passes": ["sccp"]}, {"passes": ["fold", "constprop", "unreachable"]}]}
JSON
left=0
for f in tests/programs/*.c tests/opt/*.c tests/ssa/*.c; do
  in="${f%.c}.in"; [ -f "$in" ] || in=/dev/null
  n="$(timeout 5 "$BIN" --metrics --config=/tmp/minic_sub_$$.json "$f" < "$in" 2>/dev/null \
        | awk '/^  (fold|constprop|unreachable):/ {s += $2} END {print s + 0}')"
  [ "$n" -eq 0 ] || { left=$((left+1)); echo "    after sccp, P1/P2/P6 still changed $f ($n)"; }
done
rm -f /tmp/minic_sub_$$.json
if [ "$left" -eq 0 ]; then ok "SCCP subsumes P1+P2+P6: after it, they find nothing on any program"
else no "SCCP left work for P1/P2/P6 on $left program(s)"; fi

hdr "Pipeline configuration and technique registry"
reject() {   # reject <config json> <expected message fragment>
  printf '%s' "$1" > /tmp/minic_cfg_$$.json
  local msg rc
  msg="$("$BIN" --config=/tmp/minic_cfg_$$.json --print-pipeline 2>&1)"; rc=$?
  if [ "$rc" -eq 3 ] && echo "$msg" | grep -q "$2"; then ok "rejects $1"
  else no "accepted or wrong reason for $1: $msg"; fi
}
reject '{"groups":[{"passes":["bogus"]}]}' "unknown pass 'bogus'"
reject '{"groups":[{"passes":["sccp","dce"]}]}' "mixes SSA passes"
reject '{"groups":[{"passes":["inline"]}]}' "is Planned"
reject '{"groups":[{"passes":["fold"],"repeat":99}]}' "repeat"
reject '{"preset":"fastest"}' "unknown preset"
reject '{"groups":[' "not valid JSON"
rm -f /tmp/minic_cfg_$$.json
for p in correctness balanced classic optimized; do
  if "$BIN" --preset=$p --print-pipeline > /dev/null 2>&1; then ok "preset $p resolves"; else no "preset $p"; fi
done
if "$BIN" --list-techniques | cmp -s - registry.json; then ok "registry.json matches --list-techniques"
else no "registry.json is stale: regenerate with build/minic --list-techniques > registry.json"; fi
missing="$(python3 -c '
import json, os
for t in json.load(open("registry.json")):
    if t["status"] == "Available" and not os.path.exists(t["test"]): print(t["id"], t["test"])
')"
if [ -z "$missing" ]; then ok "every Available technique names a test that exists"
else no "Available techniques without their test: $missing"; fi

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

hdr "Clang oracle (see scripts/clang_oracle.py for detail)"
# The gate above compares Cppify with itself; this compares it with clang,
# which is the only check that catches a C-semantics bug both runs share.
if ! command -v clang > /dev/null 2>&1 || ! command -v python3 > /dev/null 2>&1; then
  no "clang oracle needs clang and python3"
elif oracle_out="$(python3 scripts/clang_oracle.py 2>&1)"; then
  ok "$(echo "$oracle_out" | tail -1)"
else
  no "clang oracle mismatch — see below"
  echo "$oracle_out" | grep -v '^  MATCH'
fi


hdr "Differential fuzzing against clang (see scripts/fuzz.py; a larger run is --count 400)"
if ! command -v clang > /dev/null 2>&1; then
  no "fuzzing needs clang"
elif fuzz_out="$(python3 scripts/fuzz.py --count 40 2>&1)"; then
  ok "$(echo "$fuzz_out" | tail -1)"
else
  no "fuzz mismatch — see below"
  echo "$fuzz_out"
fi

hdr "Summary"
printf "  %d passed, %d failed\n\n" "$pass" "$fail"
[ "$fail" -eq 0 ]
