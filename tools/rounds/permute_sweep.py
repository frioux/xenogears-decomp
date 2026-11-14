#!/usr/bin/env python3
"""Run decomp-permuter over every parked body, closest first. CPU only, no model tokens.

    nohup tools/rounds/permute_sweep.py --minutes 3 >> ~/xw/permute-sweep.log 2>&1 &
    tools/rounds/permute_sweep.py --report            # what it has found so far
    tools/rounds/permute_sweep.py --report --briefs   # the same, as permute-brief.js workers

The sweep works in its own clone (~/xw/r-permute, refreshed from this tree and put in
report mode on every start), so `make check` here does not disturb it and it can run
alongside a round. Results live OUTSIDE the clone and survive it:

    ~/xw/permuter-results.tsv        one line per (function, parked body) tried
    ~/xw/permuter-out/<Func>.diff    best candidate's diff against the parked body
    ~/xw/permuter-out/<Func>.c       best candidate's full preprocessed source

A function is skipped when its current parked body (by hash) already has a result, so
restarting is cheap and a re-parked body is tried again. Only one sweep runs at a time
(~/xw/permute-sweep.pid).

SCORED only the body's score as parked is known (pass 1); the search has not run yet.
ERROR  the body does not compile or parse on its own (stale park, GTE macro, inline asm).
ZERO   a candidate is byte-identical by the permuter's measure: hand it to an apply
       worker (tools/rounds/permute-brief.js), which ports the diff and gates it.
BETTER closer but not identical: the diff is a lead for the same worker.
NONE   nothing closer in the time given; raise --minutes for the ones worth it.
"""
import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
POOL = os.environ.get("POOL", os.path.expanduser("~/xw"))
CLONE = os.path.join(POOL, "r-permute", "xenogears-decomp")
RESULTS = os.path.join(POOL, "permuter-results.tsv")
OUT = os.path.join(POOL, "permuter-out")
PID = os.path.join(POOL, "permute-sweep.pid")
FIELDS = ["func", "unit", "pct", "tag", "base", "best", "iterations", "seconds", "body", "date"]

sys.path.insert(0, os.path.join(ROOT, "tools", "scripts"))
import park  # noqa: E402

STUB = re.compile(r'^INCLUDE_ASM\("asm/([^"]+)", (\w+)\);', re.M)
PCT = re.compile(r"(?<![\d.])(\d{2}(?:\.\d+)?)\s?%")


def parks(tree):
    """Every stub with a parked C body: (pct, unit, func, body hash)."""
    found = []
    src = os.path.join(tree, "src")
    for dirpath, _, files in os.walk(src):
        for name in files:
            if not name.endswith(".c"):
                continue
            path = os.path.join(dirpath, name)
            text = open(path, errors="replace").read()
            unit = os.path.relpath(path, src)[:-2]
            for m in STUB.finditer(text):
                func = m.group(2)
                out = park.unpark(text, func, m.group(0))
                if out is None:
                    continue
                body = re.search(park.DEF % re.escape(func), out, re.M)
                if not body:
                    continue
                # The note sits above the stub or above the block below it; 80 lines
                # either side is enough to see it and only orders the queue.
                lines_before = text[:m.start()].splitlines()[-80:]
                lines_after = text[m.end():].splitlines()[:80]
                pcts = [float(p) for p in PCT.findall("\n".join(lines_before + lines_after)) if float(p) < 100]
                digest = hashlib.sha1(body.group(0).encode()).hexdigest()[:10]
                found.append((max(pcts) if pcts else 0.0, unit, func, digest))
    return sorted(found, reverse=True)


def results():
    rows = []
    if os.path.exists(RESULTS):
        for line in open(RESULTS):
            parts = line.rstrip("\n").split("\t")
            if len(parts) == len(FIELDS):
                rows.append(dict(zip(FIELDS, parts)))
    return rows


def refresh_clone():
    shutil.rmtree(os.path.dirname(CLONE), ignore_errors=True)
    os.makedirs(os.path.dirname(CLONE))
    subprocess.check_call(["cp", "-ac", ROOT, CLONE])
    for d in ("disc/disc1", "disc/disc2", "permuter"):
        shutil.rmtree(os.path.join(CLONE, d), ignore_errors=True)
    print("sweep: building the clone in report mode", flush=True)
    # The Makefile mounts $(PWD) into the container, and cwd= alone leaves the parent's PWD in place.
    subprocess.check_call(["make", "objdiff-config"], cwd=CLONE, env=dict(os.environ, PWD=CLONE),
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def run_one(unit, func, minutes, j):
    """One permute.sh run in the clone -> (tag, base, best, iterations, seconds)."""
    p = subprocess.run(["tools/scripts/permute.sh", unit, func, "--minutes", str(minutes), "-j", str(j), "--keep", "1"],
                       cwd=CLONE, env=dict(os.environ, PWD=CLONE), stdin=subprocess.DEVNULL, capture_output=True, text=True)
    m = re.search(r"^RESULT PERMUTER_(\w+) \S+ \S+ base=(\d+) best=(\d+) iterations=(\S+) seconds=(\d+) dir=(\S+)", p.stdout, re.M)
    if m:
        tag, base, best, iters, secs, d = m.groups()
        outs = [x for x in os.listdir(os.path.join(CLONE, d)) if x.startswith("output-")]
        if outs and int(best) < int(base):
            shutil.copy(os.path.join(CLONE, d, outs[0], "diff.txt"), os.path.join(OUT, func + ".diff"))
            shutil.copy(os.path.join(CLONE, d, outs[0], "source.c"), os.path.join(OUT, func + ".c"))
        result = (tag, base, best, iters, secs)
    else:
        # Did not parse or compile: GTE macros, inline asm, a body that no longer builds.
        open(os.path.join(OUT, func + ".error"), "w").write((p.stdout + p.stderr)[-4000:])
        result = ("ERROR", "-", "-", "-", "0")
    shutil.rmtree(os.path.join(CLONE, "permuter", func), ignore_errors=True)
    return result


def record(row):
    row = row + [datetime.date.today().isoformat()]
    with open(RESULTS, "a") as f:
        f.write("\t".join(row) + "\n")
    print("\t".join(row), flush=True)


def sweep(a):
    os.makedirs(OUT, exist_ok=True)
    if os.path.exists(PID):
        try:
            os.kill(int(open(PID).read()), 0)
            sys.exit("sweep: another sweep is running (pid %s); wait for it or kill it" % open(PID).read().strip())
        except (OSError, ValueError):
            pass
    open(PID, "w").write(str(os.getpid()))
    try:
        refresh_clone()
        done = {(r["func"], r["body"]) for r in results() if r["tag"] != "SCORED"}
        queue = [p for p in parks(CLONE) if p[0] >= a.min_pct and (a.retry or (p[2], p[3]) not in done)]
        if a.unit:
            queue = [p for p in queue if any(p[1].startswith(u) for u in a.unit)]

        # Pass 1: the permuter's own score for each body as parked (a few seconds each).
        # A note's percentage is unreliable for ordering (it is often a neighbour's), and
        # the score is what the search minimises: ~40 is one register off, 800 is far.
        scored = {(r["func"], r["body"]): r["base"] for r in results() if r["base"].isdigit()}
        todo = [p for p in queue if (p[2], p[3]) not in scored]
        print("sweep: scoring %d parked bodies" % len(todo), flush=True)
        for pct, unit, func, digest in todo:
            tag, base = run_one(unit, func, 0, a.j)[:2]
            record([func, unit, "%.2f" % pct, "SCORED" if tag != "ERROR" else tag, base, base, "-", "0", digest])
            scored[(func, digest)] = base
        queue = sorted((p for p in queue if scored[(p[2], p[3])].isdigit() and 0 < int(scored[(p[2], p[3])]) <= a.max_base),
                       key=lambda p: int(scored[(p[2], p[3])]))
        queue = queue[:a.limit] if a.limit else queue
        print("sweep: %d parked bodies with base score <= %d to search, %.1f min each" % (len(queue), a.max_base, a.minutes), flush=True)
        for pct, unit, func, digest in queue:
            tag, base, best, iters, secs = run_one(unit, func, a.minutes, a.j)
            record([func, unit, "%.2f" % pct, tag, base, best, iters, secs, digest])
    finally:
        os.remove(PID)


def report(a):
    live = {(func, digest): (pct, unit) for pct, unit, func, digest in parks(ROOT)}
    latest = {}
    for r in results():
        if r["tag"] != "SCORED" or r["func"] not in latest or latest[r["func"]]["body"] != r["body"]:
            latest[r["func"]] = r
    rows = [r for r in latest.values() if (r["func"], r["body"]) in live]
    stale = len(latest) - len(rows)
    count = {}
    for r in rows:
        count[r["tag"]] = count.get(r["tag"], 0) + 1
    useful = sorted((r for r in rows if r["tag"] in ("ZERO", "BETTER")),
                    key=lambda r: (r["tag"] != "ZERO", int(r["best"]) / max(int(r["base"]), 1)))
    if a.briefs:
        by_unit = {}
        for r in useful:
            by_unit.setdefault(r["unit"], []).append(r)
        workers = []
        for unit, rs in sorted(by_unit.items(), key=lambda kv: -sum(r["tag"] == "ZERO" for r in kv[1])):
            binary = unit.split("/")[0]
            workers.append({
                "tree": "<clone path>", "file": "src/%s.c" % unit, "unit": unit, "binary": binary,
                "overlay": binary,
                "funcs": "\n".join("- %s %s base=%s best=%s diff=%s" % (
                    r["func"], r["tag"], r["base"], r["best"], os.path.join(OUT, r["func"] + ".diff")) for r in rs),
            })
        print(json.dumps({"workers": workers}, indent=1))
        return
    print("permuter results for bodies still parked as tried: %s  (%d results are for bodies that have since changed or matched)"
          % (", ".join("%s %d" % kv for kv in sorted(count.items())) or "none", stale))
    for r in useful:
        print("%-6s %-34s %-28s park %6s%%  score %s -> %s" % (r["tag"], r["unit"], r["func"], r["pct"], r["base"], r["best"]))
    untried = sum(1 for k in live if k[0] not in latest or latest[k[0]]["body"] != k[1] or latest[k[0]]["tag"] == "SCORED")
    print("parked bodies not yet tried in their current form: %d" % untried)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--report", action="store_true", help="print results instead of running")
    ap.add_argument("--briefs", action="store_true", help="with --report: emit permute-brief.js worker args")
    ap.add_argument("--min-pct", type=float, default=0, help="only parks whose note claims at least this percentage")
    ap.add_argument("--max-base", type=int, default=300, help="only bodies whose permuter score as parked is at most this")
    ap.add_argument("--minutes", type=float, default=3)
    ap.add_argument("-j", type=int, default=4)
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--unit", action="append", help="only units starting with this (repeatable)")
    ap.add_argument("--retry", action="store_true", help="re-run bodies that already have a result")
    a = ap.parse_args()
    report(a) if a.report else sweep(a)


if __name__ == "__main__":
    main()
