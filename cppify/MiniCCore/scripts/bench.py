#!/usr/bin/env python3
"""Benchmark protocol (roadmap 2.7): compare optimization presets.

For every program in tests/programs, and every preset asked for, this runs
`minic --trace-json --preset=<p>` N times and records:

  * static instructions after optimization, and dynamic instructions the
    interpreter executed (deterministic: they come out the same every run);
  * compile time (lex + parse + sema + irgen + optimize) and run time of the
    optimized program, each the MEDIAN of the N runs, measured separately;
  * whether the optimized run printed exactly what the unoptimized one did.

Instruction counts are the headline numbers: they do not depend on the
machine or on what else it is doing. Times are reported, but on programs this
small they are dominated by noise; read them as orders of magnitude.

It then compares the execution engines (roadmap 3.2) on the longer-running
programs in bench/programs: the quad interpreter (--run), the register
bytecode VM (--run-vm) and the AST reference interpreter (--run-ast), all on
the same optimized IR where IR is involved. Each is timed end to end as a
process, at least 15 times, median; compile time (`--compile`, which stops after the
optimizer and prints only a count) is measured the same way and subtracted, leaving run time. Every
engine's answer must equal the others'.

Everything needed to reproduce a result is saved with it: git commit, the
binary's version, the resolved pipeline of each preset, N, the platform and
the date. Usage:

  scripts/bench.py [--runs N] [--presets classic,optimized] [--out FILE]
"""
import argparse
import datetime
import json
import os
import platform
import statistics
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "build", "minic")
COMPILE_STAGES = ("lex", "parse", "sema", "irgen", "optimize")


def trace(src, preset):
    stdin = src[:-2] + ".in"
    with open(stdin if os.path.exists(stdin) else os.devnull, "rb") as fin:
        p = subprocess.run([BIN, "--trace-json", f"--preset={preset}", src],
                           stdin=fin, capture_output=True, timeout=60)
    if p.returncode != 0:
        raise RuntimeError(f"{src} [{preset}]: exit {p.returncode}: {p.stderr.decode()[:200]}")
    return json.loads(p.stdout)


def measure(src, preset, runs):
    samples = [trace(src, preset) for _ in range(runs)]
    first = samples[0]
    compile_ms = [sum(s["timingsMs"][k] for k in COMPILE_STAGES) for s in samples]
    run_ms = [s["timingsMs"]["runOptimized"] for s in samples]
    return {
        "staticBefore": first["metrics"]["staticBefore"],
        "staticAfter": first["metrics"]["staticAfter"],
        "dynamicBefore": first["metrics"]["dynamicBefore"],
        "dynamicAfter": first["metrics"]["dynamicAfter"],
        "compileMsMedian": statistics.median(compile_ms),
        "runMsMedian": statistics.median(run_ms),
        "outputsMatch": first["run"]["outputsMatch"],
        "pipeline": first["pipeline"],
    }


def wall_ms(cmd, runs):
    """Median wall time of `cmd` in ms, and its stdout (identical every run)."""
    times, out = [], None
    for _ in range(runs):
        t0 = time.perf_counter()
        p = subprocess.run(cmd, capture_output=True, timeout=300)
        times.append((time.perf_counter() - t0) * 1000)
        if p.returncode != 0:
            raise RuntimeError(f"{cmd}: exit {p.returncode}: {p.stderr.decode()[:200]}")
        out = p.stdout
    return statistics.median(times), out


ENGINES = [("quad", ["--run", "--preset=optimized"]), ("vm", ["--run-vm", "--preset=optimized"]),
           ("ast", ["--run-ast"])]


def engines(runs):
    rows = {}
    folder = os.path.join(ROOT, "bench", "programs")
    for name in sorted(f for f in os.listdir(folder) if f.endswith(".c")):
        src = os.path.join(folder, name)
        compile_ms, _ = wall_ms([BIN, "--compile", "--preset=optimized", src], runs)
        row = {"compileMsMedian": compile_ms}
        outs = set()
        for eng, flags in ENGINES:
            total, out = wall_ms([BIN] + flags + [src], runs)
            outs.add(out)
            row[eng] = {"totalMsMedian": total, "runMs": max(total - (compile_ms if eng != "ast" else 0), 0.0)}
        row["agree"] = len(outs) == 1
        rows[name] = row
    return rows


def git_commit():
    try:
        out = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, capture_output=True, text=True)
        dirty = subprocess.run(["git", "status", "--porcelain", "."], cwd=ROOT, capture_output=True, text=True)
        return out.stdout.strip() + ("-dirty" if dirty.stdout.strip() else "")
    except OSError:
        return "unknown"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--presets", default="classic,optimized")
    ap.add_argument("--out", default=os.path.join(ROOT, "bench", "results.json"))
    args = ap.parse_args()
    presets = args.presets.split(",")

    progs = sorted(f for f in os.listdir(os.path.join(ROOT, "tests", "programs")) if f.endswith(".c"))
    results = {}
    for name in progs:
        src = os.path.join(ROOT, "tests", "programs", name)
        results[name] = {p: measure(src, p, args.runs) for p in presets}

    # Engine timing subtracts two medians, so it needs more samples than the
    # instruction counts do: with 5, one slow compile made a run time negative.
    engine_rows = engines(max(args.runs, 15))
    version = subprocess.run([BIN, "--version"], capture_output=True, text=True).stdout.strip()
    doc = {
        "config": {
            "commit": git_commit(),
            "binary": version,
            "runs": args.runs,
            "statistic": "median",
            "presets": {p: results[progs[0]][p]["pipeline"] for p in presets},
            "platform": platform.platform(),
            "machine": platform.machine(),
            "date": datetime.datetime.now().isoformat(timespec="seconds"),
        },
        "programs": {n: {p: {k: v for k, v in r.items() if k != "pipeline"} for p, r in pr.items()}
                     for n, pr in results.items()},
        "engines": engine_rows,
    }
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w") as f:
        json.dump(doc, f, indent=1)

    # Summary: totals per preset, and per-program comparison of the last
    # preset against the first.
    print(f"{len(progs)} programs, {args.runs} runs each, medians; commit {doc['config']['commit']}\n")
    print(f"| preset | static after | dynamic after | compile ms (sum of medians) | run ms (sum of medians) | outputs match |")
    print(f"|---|---:|---:|---:|---:|---:|")
    base_static = sum(results[n][presets[0]]["staticBefore"] for n in progs)
    base_dyn = sum(results[n][presets[0]]["dynamicBefore"] for n in progs)
    print(f"| (unoptimized) | {base_static} | {base_dyn} | | | |")
    for p in presets:
        rs = [results[n][p] for n in progs]
        print(f"| {p} | {sum(r['staticAfter'] for r in rs)} | {sum(r['dynamicAfter'] for r in rs)} | "
              f"{sum(r['compileMsMedian'] for r in rs):.1f} | {sum(r['runMsMedian'] for r in rs):.1f} | "
              f"{sum(r['outputsMatch'] for r in rs)}/{len(rs)} |")
    if len(presets) >= 2:
        a, b = presets[0], presets[-1]
        better = [n for n in progs if results[n][b]["dynamicAfter"] < results[n][a]["dynamicAfter"]]
        worse = [n for n in progs if results[n][b]["dynamicAfter"] > results[n][a]["dynamicAfter"]]
        print(f"\ndynamic instructions, {b} vs {a}: fewer on {len(better)}, more on {len(worse)}, "
              f"equal on {len(progs) - len(better) - len(worse)}")
        for n in worse:
            print(f"  more: {n}  {results[n][a]['dynamicAfter']} -> {results[n][b]['dynamicAfter']}")
    print("\nengines on bench/programs (median ms; run = total - compile; the AST")
    print("interpreter has no IR, so its whole time counts as run):\n")
    print("| program | compile | quad run | VM run | AST run | VM speed-up | agree |")
    print("|---|---:|---:|---:|---:|---:|:-:|")
    for n, r in engine_rows.items():
        q, v, a = r["quad"]["runMs"], r["vm"]["runMs"], r["ast"]["runMs"]
        print(f"| {n} | {r['compileMsMedian']:.1f} | {q:.1f} | {v:.1f} | {a:.1f} | "
              f"{(q / v if v > 0 else float('inf')):.1f}x | {'yes' if r['agree'] else 'NO'} |")
    print(f"\nwrote {os.path.relpath(args.out, ROOT)}")
    mismatched = [n for n in progs for p in presets if not results[n][p]["outputsMatch"]]
    mismatched += [n for n, r in engine_rows.items() if not r["agree"]]
    return 1 if mismatched else 0


if __name__ == "__main__":
    sys.exit(main())
