#!/usr/bin/env python3
"""Differential fuzzing against clang.

Generates random programs in the supported subset (nested loops with break
and continue, if/else, ternaries, calls that write globals, arrays, every
integer operator, division and shifts by safe constants), builds each with
clang, and requires Cppify to print the same bytes and return the same
status under every optimization configuration (IR verifier on), on the AST
reference interpreter and on the bytecode VM.

The fixed test suite only exercises the shapes someone thought to write. On
its first runs this found four miscompiles in P2, P3 and P8 that every test
had missed (see tests/opt/constprop_entry_values.c, licm_zero_trip.c and
licm_call_writes_global.c).

Programs are generated from a seed, so a failure is reproducible: the
mismatching seed is printed and its source kept in the work directory.

Usage: scripts/fuzz.py [--count N] [--seed S] [--keep DIR]
Exit status: 0 when nothing mismatches.
"""
import argparse
import os
import random
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "build", "minic")
CONFIGS = (["--run", "--no-opt"], ["--run", "--preset=classic"], ["--run", "--preset=optimized"],
           ["--run", "--opt=sccp"], ["--run", "--opt=gvn"], ["--run", "--opt=licm-ssa"],
           ["--run", "--opt=ssa-dce"], ["--run-ast"], ["--run-vm", "--preset=optimized"],
           ["--run", "--lexer=handwritten", "--parser=bison", "--preset=optimized"])


def gen(rng):
    lines = ["#include <stdio.h>", "int g1 = 3; int g2;", "int arr_g[8];"]
    lines.append("int helper(int a, int b) { if (a > b) return a - b; return b + a * 2; }")
    lines.append("void bump(void) { g2 = g2 + 1; arr_g[g2 & 7] = g2; }")
    vars_ = ["a", "b", "c", "d", "e"]
    def expr(depth=0):
        r = rng.random()
        if depth > 2 or r < 0.3:
            return rng.choice(vars_ + [str(rng.randint(-5, 20)), "g1", "g2", "arr[%d]" % rng.randint(0, 5), "i"])
        if r < 0.4: return "helper(%s, %s)" % (expr(depth+1), expr(depth+1))
        if r < 0.45: return "(%s ? %s : %s)" % (expr(depth+1), expr(depth+1), expr(depth+1))
        op = rng.choice(["+", "-", "*", "/", "%", "<", ">", "==", "!=", "&", "|", "^", "&&", "||", "<<", ">>"])
        if op in ("/", "%"): return "(%s %s %d)" % (expr(depth+1), op, rng.choice([1, 2, 3, 7, -3]))
        if op in ("<<", ">>"): return "(%s %s %d)" % (expr(depth+1), op, rng.randint(0, 4))
        return "(%s %s %s)" % (expr(depth+1), op, expr(depth+1))
    def stmt(depth):
        r = rng.random()
        v = rng.choice(vars_)
        if depth > 2 or r < 0.45: return ["%s = %s;" % (v, expr())]
        if r < 0.55: return ["arr[%d] = %s;" % (rng.randint(0, 5), expr())]
        if r < 0.6: return ["bump();"]
        if r < 0.62: return ["g1 = %s;" % expr()]
        if r < 0.75:
            out = ["if (%s) {" % expr()] + sum([stmt(depth+1) for _ in range(rng.randint(1, 3))], []) + ["} else {"] + sum([stmt(depth+1) for _ in range(rng.randint(0, 2))], []) + ["}"]
            return out
        if r < 0.9:
            k = rng.choice(["i", "j"])
            body = sum([stmt(depth+1) for _ in range(rng.randint(1, 4))], [])
            if rng.random() < 0.2: body.append("if (%s) break;" % expr())
            if rng.random() < 0.2: body.insert(0, "if (%s) continue;" % expr())
            return ["for (%s = 0; %s < %d; %s = %s + 1) {" % (k, k, rng.randint(0, 6), k, k)] + body + ["}"]
        return ["%s = %s + 1;" % (v, v)]
    body = ["int main() {", "int a = %d, b = %d, c = %d, d, e = 0, i = 0, j = 0;" % (rng.randint(-9, 9), rng.randint(-9, 9), rng.randint(0, 9)),
            "int arr[6]; arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5; arr[5] = 6; d = 0;"]
    for _ in range(rng.randint(4, 10)): body += stmt(0)
    body += ['printf("%d %d %d %d %d %d %d\\n", a, b, c, d, e, g1, g2);', 'printf("%d %d %d\\n", arr[0], arr[3], arr_g[1]);', "return (a ^ b ^ c) & 255;", "}"]
    return "\n".join(lines + body) + "\n"


def matches(out, rc, ref_out, ref_rc):
    if rc != 0 or not out.startswith(ref_out):
        return False
    tail = out[len(ref_out):].strip()
    return tail.lstrip(b"-").isdigit() and int(tail) % 256 == ref_rc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--count", type=int, default=100)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--keep", help="directory to keep generated programs in")
    args = ap.parse_args()
    work = args.keep or tempfile.mkdtemp(prefix="cppify-fuzz-")
    os.makedirs(work, exist_ok=True)
    env = dict(os.environ, MINIC_VERIFY="1")
    ran = failed = 0
    for seed in range(args.seed, args.seed + args.count):
        path = os.path.join(work, f"fuzz{seed}.c")
        with open(path, "w") as f:
            f.write(gen(random.Random(seed)))
        exe = os.path.join(work, "ref")
        if subprocess.run(["clang", "-std=c17", "-w", "-O0", "-fwrapv", path, "-o", exe],
                          capture_output=True).returncode:
            continue
        try:
            ref = subprocess.run([exe], capture_output=True, timeout=5)
        except subprocess.TimeoutExpired:
            continue
        ran += 1
        bad = []
        for flags in CONFIGS:
            try:
                p = subprocess.run([BIN] + flags + [path], capture_output=True, timeout=20, env=env)
                if not matches(p.stdout, p.returncode, ref.stdout, ref.returncode):
                    bad.append(flags[-1] + (": " + p.stderr.decode()[:160] if p.stderr else ""))
            except subprocess.TimeoutExpired:
                bad.append(flags[-1] + ": timed out")
        if bad:
            failed += 1
            print(f"MISMATCH seed {seed} ({path})")
            for b in bad:
                print(f"    {b}")
    if not args.keep and not failed:
        shutil.rmtree(work, ignore_errors=True)
    print(f"fuzz: {ran} programs, {failed} mismatched (seeds {args.seed}..{args.seed + args.count - 1})")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
