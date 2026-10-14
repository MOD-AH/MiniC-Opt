#!/usr/bin/env bash
# MiniC-Opt — golden-IR regression tests.
#
# The differential gate proves an optimized program still BEHAVES the same.
# It cannot notice a pass that quietly stops firing, or starts emitting a
# different shape that happens to compute the same answer. These tests pin
# the actual IR: one case per pass over the micro-test built for it, plus
# the full pipeline over two real benchmarks.
#
# That makes them deliberately brittle — any genuine improvement to a pass
# will change its golden file, and that is the point: the diff has to be
# read and accepted rather than passing silently. Regenerate with
#
#     ./scripts/golden.sh --update
#
# and commit the diff alongside the change that caused it.
#
# Only stdout is captured; the per-sweep "[opt]" log goes to stderr and is
# deliberately excluded, since transform counts shift for reasons that are
# not IR-shape regressions.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BIN="build/minic"
DIR="tests/golden"
ALL="fold,constprop,copyprop,cse,dce,unreachable,strength,licm,peephole"

UPDATE=0
[ "${1:-}" = "--update" ] && UPDATE=1

pass=0; fail=0
hdr() { printf '\n\033[1m%s\033[0m\n' "$1"; }
ok()  { printf '  \033[32mPASS\033[0m  %s\n' "$1"; pass=$((pass+1)); }
no()  { printf '  \033[31mFAIL\033[0m  %s\n' "$1"; fail=$((fail+1)); }

if [ ! -x "$BIN" ]; then
  echo "golden: $BIN not built — run scripts/run_all.sh first" >&2
  exit 1
fi
mkdir -p "$DIR"

# name | source | --opt argument
CASES="
fold|tests/opt/fold_add.c|fold
constprop|tests/opt/constprop_basic.c|constprop
copyprop|tests/opt/copyprop_param.c|copyprop
cse|tests/opt/cse_basic.c|cse
dce|tests/opt/dce_basic.c|dce
unreachable|tests/opt/unreachable_basic.c|unreachable
strength|tests/opt/strength_basic.c|strength
licm|tests/opt/licm_basic.c|licm
peephole|tests/opt/peephole_basic.c|peephole
pipeline_worked_example|tests/programs/worked_example.c|$ALL
pipeline_gcd|tests/programs/gcd.c|$ALL
unoptimized_gcd|tests/programs/gcd.c|
"

if [ "$UPDATE" -eq 1 ]; then
  hdr "Regenerating golden IR listings"
else
  hdr "Golden-IR regression — committed IR must match byte-for-byte"
fi

echo "$CASES" | while IFS='|' read -r name src opts; do
  [ -z "$name" ] && continue
  golden="$DIR/$name.ir"
  got="$(mktemp)"
  if [ -z "$opts" ]; then
    "$BIN" --dump-ir "$src" > "$got" 2>/dev/null
  else
    "$BIN" --dump-ir --opt="$opts" "$src" > "$got" 2>/dev/null
  fi
  rc=$?
  if [ "$rc" -ne 0 ]; then
    no "$name  (minic exited $rc)"
  elif [ "$UPDATE" -eq 1 ]; then
    cp "$got" "$golden"
    printf '  updated  %s\n' "$name"
  elif [ ! -f "$golden" ]; then
    no "$name  (no golden file — run ./scripts/golden.sh --update)"
  elif cmp -s "$got" "$golden"; then
    ok "$name"
  else
    no "$name  (IR differs from $golden)"
    diff -u "$golden" "$got" | head -25 | sed 's/^/      /'
  fi
  rm -f "$got"
done

# The while loop runs in a subshell, so recount from the filesystem rather
# than relying on counters that did not survive the pipeline.
if [ "$UPDATE" -eq 1 ]; then
  printf '\n  %d golden files written to %s\n\n' "$(ls "$DIR"/*.ir 2>/dev/null | wc -l | tr -d ' ')" "$DIR"
  exit 0
fi

mismatch=0
echo "$CASES" | { while IFS='|' read -r name src opts; do
  [ -z "$name" ] && continue
  golden="$DIR/$name.ir"
  got="$(mktemp)"
  if [ -z "$opts" ]; then
    "$BIN" --dump-ir "$src" > "$got" 2>/dev/null
  else
    "$BIN" --dump-ir --opt="$opts" "$src" > "$got" 2>/dev/null
  fi
  if [ ! -f "$golden" ] || ! cmp -s "$got" "$golden"; then
    mismatch=$((mismatch+1))
  fi
  rm -f "$got"
done
printf '\n\033[1mSummary\033[0m\n  %d mismatched\n\n' "$mismatch"
exit $([ "$mismatch" -eq 0 ] && echo 0 || echo 1); }
