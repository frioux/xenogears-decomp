#!/usr/bin/env python3
"""Score parked bodies under every compiler in tools/gcc-*, to test "was this unit built
with a compiler nobody has tried?".

    tools/rounds/cc_sweep.py                       # parks recorded >= 99.0, all tools/gcc-*
    tools/rounds/cc_sweep.py --min 98 --gcc gcc-2.8.1-psx
    tools/rounds/cc_sweep.py --unit battle/main/misc9 --retry
    tools/rounds/cc_sweep.py --report              # summarize the TSV, no compiling
    tools/rounds/cc_sweep.py --check-unit slus_006.64/system/sound --gcc gcc-2.8.1-psx

Per unit, the selected parked bodies are un-parked (park_sweep), the unit is compiled with
try_flags.sh --all under its own preset (the "base" column, re-measured, not the note's
number) and again with `--gcc <dir>` for each compiler, and the source is restored. Every
other flag stays the unit's own. try_flags runs share build/.try_flags.*, so everything
here is serial within one tree; run it in a private clone in report mode
(`make objdiff-config`). Results go to ~/xw/cc-sweep.tsv (override with --out) and the run
is resumable: a (park, compiler) cell that is already filled is skipped unless --retry.
A cell reads "failed" when the unit did not compile or the function was absent.

--check-unit scores every live C function of a unit under one compiler: the discriminating
test (a compiler counts only if everything already matched stays at 100.00).
"""
import argparse
import importlib.util
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import park_sweep as ps  # noqa: E402

park = ps.load_park()
FIXED = ["unit", "func", "preset", "note", "base"]
TRAIL = re.compile(r"(?:\s|//[^\n]*\n|/\*(?:[^*]|\*(?!/))*\*/)*\Z")


def preset_table():
    spec = importlib.util.spec_from_file_location(
        "preset_for", os.path.join(ROOT, "tools/scripts/preset_for.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return list(mod.presets())


def preset_of(table, unit):
    stripped = unit.split("/", 1)[1] if unit.count("/") > 1 else unit
    binary = unit.split("/", 1)[0]
    hit = "Default"
    for name, paths, _ in table:
        paths = [p.replace("{binary}", binary) for p in paths]
        if any((unit + ".").startswith(p) or (stripped + ".").startswith(p) for p in paths):
            hit = name
    return hit


def note_pct(text, name, line):
    """The percentage a park's note records (the census heuristic), or None."""
    inc = text.find(line)
    for m in park.BLOCK.finditer(text):
        if not re.search(r"\b" + name + r"\s*\(", m.group(1)) or not park.body_in(m.group(1), name):
            continue
        if m.end() <= inc and park.filler_only(text[m.end():inc]):
            a, b = m.start(), inc + len(line)
        elif m.start() >= inc + len(line) and park.filler_only(text[inc + len(line):m.start()]):
            a, b = inc, m.end()
        else:
            continue
        mm = TRAIL.search(text[:a], max(0, a - 3000))
        region = text[(mm.start() if mm else a):b]
        pm = re.search(r"(?:PARKED|Best:?|best)[^0-9%]{0,15}(\d{1,3}(?:\.\d+)?)%", region)
        if pm:
            return float(pm.group(1))
        found = [float(x) for x in re.findall(r"(\d{1,3}(?:\.\d+)?)%", region) if float(x) <= 100]
        return max(found) if found else None
    return None


def read_tsv(path):
    rows, cols = {}, list(FIXED)
    if os.path.exists(path):
        lines = open(path).read().splitlines()
        if lines:
            cols = lines[0].split("\t")
            for l in lines[1:]:
                r = dict(zip(cols, l.split("\t")))
                rows[(r["unit"], r["func"])] = r
    return rows, cols


def write_tsv(path, rows, cols):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        f.write("\t".join(cols) + "\n")
        for k in sorted(rows):
            f.write("\t".join(rows[k].get(c, "") for c in cols) + "\n")
    os.replace(tmp, path)


def discover_gcc():
    tools = os.path.join(ROOT, "tools")
    return sorted(d for d in os.listdir(tools)
                  if d.startswith("gcc-") and os.path.isdir(os.path.join(tools, d)))


def fmt(scores, name):
    return "%.2f" % scores[name] if name in scores else "failed"


def score_parks(src, original, unit, selected, flags):
    """Un-park `selected` bodies, score the unit, restore the file. A body that stops the
    unit compiling when un-parked with the others is retried alone. Returns (scores, runs)."""
    runs = 0
    todo = [list(selected)]
    scores = {}
    while todo:
        group = todo.pop()
        ps.unpark_all(src, [(n, l) for n, l, _ in group])
        try:
            sc = ps.score(unit, flags)
            runs += 1
        finally:
            open(src, "w").write(original)
        scores.update({n: sc[n] for n, _, _ in group if n in sc})
        missing = [p for p in group if p[0] not in sc]
        if len(group) > 1:
            todo.extend([p] for p in missing)
    return scores, runs


def sweep(args):
    gccs = args.gcc.split(",") if args.gcc else discover_gcc()
    for g in gccs:
        if not os.path.isdir(os.path.join(ROOT, "tools", g)):
            sys.exit("no tools/%s" % g)
    table = preset_table()
    rows, cols = read_tsv(args.out)
    cols += [g for g in gccs if g not in cols]

    todo = {}
    for d, _, fs in os.walk(os.path.join(ROOT, "src")):
        for f in fs:
            if not f.endswith(".c"):
                continue
            sp = os.path.join(d, f)
            unit = os.path.relpath(sp, os.path.join(ROOT, "src"))[:-2]
            if args.unit and unit not in args.unit:
                continue
            text = open(sp).read()
            for name, line in ps.parked_funcs(sp):
                pct = note_pct(text, name, line)
                if pct is not None and pct >= args.min and pct != 100:
                    todo.setdefault(unit, []).append((name, line, pct))
    print("selected %d parks in %d units; compilers: %s"
          % (sum(len(v) for v in todo.values()), len(todo), ", ".join(gccs)), flush=True)

    for unit in sorted(todo):
        src = os.path.join(ROOT, "src", unit + ".c")
        funcs = todo[unit]
        for name, _, pct in funcs:
            rows.setdefault((unit, name), {}).update(
                unit=unit, func=name, preset=preset_of(table, unit), note="%g" % pct)
        pending = {"base": [f for f in funcs if args.retry or not rows[(unit, f[0])].get("base")]}
        for g in gccs:
            pending[g] = [f for f in funcs if args.retry or not rows[(unit, f[0])].get(g)]
        if not any(pending.values()):
            continue
        original = open(src).read()
        t0, runs = time.time(), 0
        try:
            for col, sel in pending.items():
                if sel:
                    sc, n = score_parks(src, original, unit, sel, [] if col == "base" else ["--gcc", col])
                    runs += n
                    for name, _, _ in sel:
                        rows[(unit, name)][col] = fmt(sc, name)
        finally:
            open(src, "w").write(original)
        write_tsv(args.out, rows, cols)
        print("%s: %d parks, %d compiles, %.0fs" % (unit, len(funcs), runs, time.time() - t0), flush=True)
    write_tsv(args.out, rows, cols)
    report(args)


def num(s):
    try:
        return float(s)
    except (TypeError, ValueError):
        return None


def report(args):
    rows, cols = read_tsv(args.out)
    gccs = [c for c in cols if c not in FIXED]
    print("%d parks in %s" % (len(rows), args.out))
    hits = []
    for g in gccs:
        n100 = better = worse = 0
        for r in rows.values():
            b, v = num(r.get("base")), num(r.get(g))
            if v is None:
                continue
            if v >= 100:
                n100 += 1
                hits.append((r["unit"], r["func"], g))
            if b is not None and v > b + 0.005:
                better += 1
            elif b is not None and v < b - 0.005:
                worse += 1
        print("%-22s 100.00: %d  improved: %d  regressed: %d" % (g, n100, better, worse))
    stale = [(r["unit"], r["func"], "base") for r in rows.values() if (num(r.get("base")) or 0) >= 100]
    print("at 100.00 under own preset (stale park): %d" % len(stale))
    for h in stale + hits:
        print("  %s %s %s" % h)


def check_unit(args):
    if not args.gcc or "," in args.gcc:
        sys.exit("--check-unit needs exactly one --gcc <dir>")
    unit = args.check_unit
    own = ps.score(unit, [])
    alt = ps.score(unit, ["--gcc", args.gcc])
    if not alt:
        sys.exit("%s under %s: nothing scored (compile failed?)" % (unit, args.gcc))
    # the test is on functions that already match: they must all stay 100.00
    lost = [(n, v) for n, v in sorted(alt.items()) if own.get(n, 0) >= 100.0 and v < 100.0]
    for n, v in lost:
        print("  %-40s own %.2f  %s %.2f" % (n, own[n], args.gcc, v))
    held = sum(1 for n, v in alt.items() if own.get(n, 0) >= 100.0 and v >= 100.0)
    print("%s under %s: %d functions scored; %d of %d already-matched stay 100.00: %s"
          % (unit, args.gcc, len(alt), held, held + len(lost),
             "ALL STAY 100.00" if not lost else "REGRESSES %d" % len(lost)))
    return 1 if lost else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--min", type=float, default=99.0, help="minimum recorded note percentage")
    ap.add_argument("--gcc", help="comma-separated tools/ dirs (default: every tools/gcc-*)")
    ap.add_argument("--unit", action="append", help="restrict to this unit (repeatable)")
    ap.add_argument("--out", default=os.path.expanduser("~/xw/cc-sweep.tsv"))
    ap.add_argument("--retry", action="store_true", help="re-score cells that are already filled")
    ap.add_argument("--report", action="store_true", help="summarize the TSV and exit")
    ap.add_argument("--check-unit", help="score every live function of this unit under --gcc")
    args = ap.parse_args()
    if args.report:
        return report(args)
    if args.check_unit:
        sys.exit(check_unit(args))
    sweep(args)


if __name__ == "__main__":
    main()
