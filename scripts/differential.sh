#!/usr/bin/env bash
# MiniC-Opt — the differential-execution gate.
# Owner: Member 4
#
# This is the safety net the report's methodology is built around: every
# committed benchmark and every optimizer micro-test must produce
# BYTE-IDENTICAL output whether run unoptimized or through the full P1-P9
# pipeline, AND with each pass enabled completely alone (isolating a bug
# to one specific pass rather than only ever seeing it in combination). A
# mismatch here means some pass made an unsafe rewrite — see
# propagation_util.h's file header for a real example this exact check
# caught during development (P3 copy propagation, before the same-source-
# not-redefined condition was added).
#
# Run standalone:   ./scripts/differential.sh
# Wired into CI via scripts/run_all.sh and .github/workflows/ci.yml.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BIN="build/minic"

# macOS ships no GNU `timeout`; fall back to running the command untimed
# rather than failing every check with "command not found" (exit 127).
if command -v timeout >/dev/null 2>&1; then
  timeout() { command timeout "$@"; }
elif command -v gtimeout >/dev/null 2>&1; then
  timeout() { command gtimeout "$@"; }
else
  timeout() { shift; "$@"; }
fi

pass=0; fail=0
hdr() { printf '\n\033[1m%s\033[0m\n' "$1"; }
ok()  { printf '  \033[32mPASS\033[0m  %s\n' "$1"; pass=$((pass+1)); }
no()  { printf '  \033[31mFAIL\033[0m  %s\n' "$1"; fail=$((fail+1)); }

ALL_PASSES="fold,constprop,copyprop,cse,dce,unreachable,strength,licm,peephole"

hdr "Differential-execution gate — unoptimized vs. --opt=$ALL_PASSES"
for f in tests/programs/*.c tests/opt/*.c; do
  name="$(basename "$f")"
  base="${f%.c}"
  unopt="$(mktemp)"; opt="$(mktemp)"
  timeout 5 "$BIN" --run "$f"                    > "$unopt" 2>/dev/null
  ru=$?
  timeout 5 "$BIN" --run --opt="$ALL_PASSES" "$f" > "$opt"   2>/dev/null
  ro=$?
  if [ "$ru" -ne 0 ] || [ "$ro" -ne 0 ]; then
    no "$name  (interpreter exited unopt=$ru opt=$ro)"
  elif ! cmp -s "$unopt" "$opt"; then
    no "$name  (unoptimized and optimized output differ)"
  elif [ -f "${base}.expected" ] && ! cmp -s "$unopt" "${base}.expected"; then
    no "$name  (both agree with each other but not with .expected)"
  else
    ok "$name"
  fi
  rm -f "$unopt" "$opt"
done

hdr "Differential-execution gate — each pass enabled alone"
for passname in $(echo "$ALL_PASSES" | tr ',' ' '); do
  for f in tests/programs/*.c tests/opt/*.c; do
    name="$(basename "$f") [--opt=$passname]"
    unopt="$(mktemp)"; opt="$(mktemp)"
    timeout 5 "$BIN" --run "$f"                   > "$unopt" 2>/dev/null
    ru=$?
    timeout 5 "$BIN" --run --opt="$passname" "$f" > "$opt"   2>/dev/null
    ro=$?
    if [ "$ru" -ne 0 ] || [ "$ro" -ne 0 ]; then
      no "$name  (interpreter exited unopt=$ru opt=$ro)"
    elif ! cmp -s "$unopt" "$opt"; then
      no "$name  (unoptimized and single-pass output differ)"
    else
      ok "$name"
    fi
    rm -f "$unopt" "$opt"
  done
done

hdr "Summary"
printf "  %d passed, %d failed\n\n" "$pass" "$fail"
[ "$fail" -eq 0 ]
