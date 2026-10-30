#!/usr/bin/env python3
"""Lexer equivalence fuzzing (roadmap 4.1).

The test files are well-formed C, but lexers differ on malformed input:
unterminated strings and comments, stray backslashes, '#' mid-line, numbers
with odd suffixes, CR, tabs, bytes above 127, directives inside skipped
#if regions. This glues random fragments of exactly that kind together and
requires `minic --dump-tokens` (tokens, positions and diagnostics) to print
the same bytes with the hand-written lexer and with the re2c one.

Usage: scripts/lexfuzz.py [--count N] [--seed S]
"""
import argparse
import os
import random
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "build", "minic")

FRAGMENTS = [
    "int", "intx", "unsigned", "_a1", "x", "  ", "\t", "\n", "\r\n", "\r",
    "0", "007", "12", "3.5", "3.", ".5", "1e5", "1e", "1e+", "1.5e-3", "2.0f", "2fx", "10u", "10UL",
    "7lu", "0x1F", "0X", "0xffu", "0x1g", "1.5x", "9_",
    "'a'", "'\\n'", "'\\''", "''", "'ab'", "'", "'\\", "'\n'",
    '"hi"', '"a\\"b"', '"\\t\\0\\q"', '"unterminated', '"a\\\nb"', '"',
    "/* c */", "/* **/", "/***/", "/* unterminated", "// line", "//", "/", "*/",
    "#define N 3\n", "#define F(x) x\n", "#undef N\n", "#if 0\n", "#if 1\n", "#ifdef N\n", "#ifndef N\n",
    "#else\n", "#elif 1\n", "#endif\n", "  #endif\n", "#if defined(N)\n", "#if x+1\n", "#include <stdio.h>\n",
    "#define LONG 1 \\\n + 2\n", "#define C /* c */ 4\n", "a # b", "N", "LONG", "C", "NULL", "EOF",
    "+", "++", "+=", "-", "->", "--", "-=", "*", "*=", "/=", "%", "%=", "=", "==", "!", "!=",
    "<", "<=", "<<", "<<=", ">", ">=", ">>", ">>=", "&", "&&", "&=", "|", "||", "|=", "^", "^=", "~",
    "(", ")", "{", "}", "[", "]", ",", ".", "?", ":", ";", "@", "$", "`", "\\", "\x7f", "\xe2\x82\xac", "\f",
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--count", type=int, default=500)
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()
    work = tempfile.mkdtemp(prefix="cppify-lexfuzz-")
    bad = 0
    for seed in range(args.seed, args.seed + args.count):
        rng = random.Random(seed)
        text = "".join(rng.choice(FRAGMENTS) + rng.choice(["", " ", "", "\n"]) for _ in range(rng.randint(1, 40)))
        path = os.path.join(work, f"lex{seed}.c")
        with open(path, "wb") as f:
            f.write(text.encode("latin-1", errors="replace") if False else text.encode("utf-8"))
        outs = [subprocess.run([BIN, "--dump-tokens", f"--lexer={k}", path], capture_output=True, timeout=10)
                for k in ("handwritten", "re2c")]
        if (outs[0].stdout, outs[0].stderr, outs[0].returncode) != (outs[1].stdout, outs[1].stderr, outs[1].returncode):
            bad += 1
            print(f"MISMATCH seed {seed}: {path}")
    if not bad:
        for f in os.listdir(work):
            os.remove(os.path.join(work, f))
        os.rmdir(work)
    print(f"lexfuzz: {args.count} inputs, {bad} differ (seeds {args.seed}..{args.seed + args.count - 1})")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
