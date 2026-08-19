#!/usr/bin/env bash
# MiniC-Opt — build and run the full test suite.
# Owner: Member 4
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BIN="build/minic"
pass=0; fail=0

hdr() { printf '\n\033[1m%s\033[0m\n' "$1"; }
ok()  { printf '  \033[32mPASS\033[0m  %s\n' "$1"; pass=$((pass+1)); }
no()  { printf '  \033[31mFAIL\033[0m  %s\n' "$1"; fail=$((fail+1)); }

hdr "Building"
cmake -B build -DCMAKE_BUILD_TYPE=Release > /dev/null || { echo "cmake failed"; exit 1; }
cmake --build build --parallel > /dev/null   || { echo "build failed"; exit 1; }
echo "  build/minic ready"

hdr "Positive suite — lexer must report zero errors"
for f in tests/programs/*.c; do
  if "$BIN" --dump-tokens "$f" > /dev/null 2>&1; then ok "$(basename "$f")"; else no "$(basename "$f")"; fi
done

hdr "Negative suite — lexer must report at least one diagnostic"
for f in tests/negative/*.c; do
  if "$BIN" --dump-tokens "$f" > /dev/null 2>&1; then
    no "$(basename "$f")  (expected an error, got none)"
  else
    ok "$(basename "$f")  →  $("$BIN" --dump-tokens "$f" 2>&1 >/dev/null | head -1 | cut -d' ' -f2-)"
  fi
done

hdr "Not yet implemented (planned)"
echo "  Review 2 : parser, semantic analyser, TAC generation, CFG, interpreter, P1-P3"
echo "  Review 3 : P4-P9, code emitter, full benchmark measurement"

hdr "Summary"
printf "  %d passed, %d failed\n\n" "$pass" "$fail"
[ "$fail" -eq 0 ]
