#!/usr/bin/env python3
"""Clang oracle: does Cppify agree with a real C compiler?

The differential gate compares Cppify with itself (unoptimized vs optimized
IR), so a bug shared by both runs, such as wrong C semantics in IRGen or the
interpreter, passes it. This script compares Cppify with clang instead.

For every positive test program it builds the file with clang, runs both on
the same stdin, and requires:
  * identical stdout bytes, and
  * Cppify's returned value mod 256 == clang's process exit status
    (`--run` prints main's full return value as a last line; a process exit
    status is truncated to 8 bits).

A program clang cannot build, or one listed in clang_oracle_exclusions.txt
(with the reason it cannot match, e.g. undefined behaviour), is reported as
EXCLUDED, never silently skipped.

Usage: scripts/clang_oracle.py [--verbose]
Each program is checked five times: Cppify unoptimized, with the classic
P1-P9 pipeline, with the optimized (SSA) preset (those two with the IR
verifier on), on the AST reference interpreter, which never sees IR, and on
the bytecode VM running optimized IR, and through the other front end (the
hand-written lexer and the Bison parser). All six must match clang.
Exit status: 0 when nothing mismatches, 1 otherwise.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "build", "minic")
DIRS = ["tests/programs", "tests/opt", "tests/boundary", "tests/ssa"]
EXCLUSIONS = os.path.join(ROOT, "tests", "clang_oracle_exclusions.txt")
TIMEOUT = 10


def load_exclusions():
    out = {}
    if os.path.exists(EXCLUSIONS):
        for line in open(EXCLUSIONS):
            line = line.strip()
            if line and not line.startswith("#"):
                name, _, reason = line.partition(" ")
                out[name] = reason.strip()
    return out


def run(cmd, stdin_path):
    env = dict(os.environ, MINIC_VERIFY="1")
    with open(stdin_path, "rb") as fin:
        p = subprocess.run(cmd, stdin=fin, capture_output=True, timeout=TIMEOUT, env=env)
    return p.returncode, p.stdout


def main():
    verbose = "--verbose" in sys.argv
    clang = shutil.which("clang")
    if not clang:
        print("clang not found; the oracle needs it")
        return 1
    excluded = load_exclusions()
    passed = failed = skipped = 0
    tmp = tempfile.mkdtemp(prefix="cppify-oracle-")
    for d in DIRS:
        for name in sorted(os.listdir(os.path.join(ROOT, d))):
            if not name.endswith(".c"):
                continue
            src = os.path.join(ROOT, d, name)
            label = f"{d.split('/')[-1]}/{name}"
            if name in excluded:
                print(f"  EXCLUDED  {label}  ({excluded[name]})")
                skipped += 1
                continue
            stdin = src[:-2] + ".in"
            if not os.path.exists(stdin):
                stdin = os.devnull
            exe = os.path.join(tmp, name[:-2])
            # Cppify knows the library functions without an #include, so a test
            # may omit it; the headers are force-included for clang to match.
            headers = [f for h in ("stdio.h", "stdlib.h", "string.h", "math.h", "ctype.h") for f in ("-include", h)]
            # A program that defines its own `bsearch` clashes with the forced
            # stdlib.h, so fall back to building it exactly as written.
            for extra in (headers, []):
                build = subprocess.run([clang, "-std=c17", "-w", "-O0", *extra, src, "-o", exe, "-lm"],
                                       capture_output=True, text=True)
                if build.returncode == 0:
                    break
            if build.returncode != 0:
                first = next((l for l in build.stderr.splitlines() if "error" in l), "build failed")
                print(f"  EXCLUDED  {label}  (not valid C for clang: {first.split('error: ')[-1]})")
                skipped += 1
                continue
            ref_rc, ref_out = run([exe], stdin)
            problems = []
            for mode_args in (["--run", "--no-opt"], ["--run", "--preset=classic"],
                              ["--run", "--preset=optimized"], ["--run-ast"],
                              ["--run-vm", "--preset=optimized"],
                              ["--run", "--lexer=handwritten", "--parser=bison", "--preset=optimized"]):
                rc, out = run([BIN] + mode_args + [src], stdin)
                mode = " ".join(a.lstrip("-") for a in mode_args[:1] + mode_args[1:])
                if rc != 0:
                    problems.append(f"{mode}: cppify exited {rc}")
                    continue
                tail = out[len(ref_out):] if out.startswith(ref_out) else None
                m = re.fullmatch(rb"(-?\d+)\n", tail) if tail is not None else None
                if m is None:
                    problems.append(f"{mode}: stdout differs")
                    if verbose:
                        problems.append(f"    clang:  {ref_out[-200:]!r}\n    cppify: {out[-200:]!r}")
                elif int(m.group(1)) % 256 != ref_rc:
                    problems.append(f"{mode}: returned {int(m.group(1))} (mod 256 = "
                                    f"{int(m.group(1)) % 256}), clang exited {ref_rc}")
            if problems:
                failed += 1
                print(f"  MISMATCH  {label}")
                for p in problems:
                    print(f"            {p}")
            else:
                passed += 1
                if verbose:
                    print(f"  MATCH     {label}")
    shutil.rmtree(tmp, ignore_errors=True)
    print(f"\nclang oracle: {passed} match, {failed} mismatch, {skipped} excluded")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
