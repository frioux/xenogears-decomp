#!/usr/bin/env python3
"""Score every parked body of a unit under its own preset and under candidates.

    tools/rounds/park_sweep.py                      # every unit that has a park
    tools/rounds/park_sweep.py battle/main/misc18   # just these units

A park is a C body inside a `/* ... */` block next to its INCLUDE_ASM line (see
tools/scripts/park.py). `try_flags.sh --all` only scores functions that are live C,
so a parked body - exactly where the free matches are - is invisible to it. This
un-parks every body of a unit, compiles the unit once per candidate, prints one row
per parked function, then `git checkout`s the file.

Columns: own = the unit's own preset (a 100.00 there is a free match, the park is
stale), then one column per candidate (a 100.00 there means the unit wants that
preset - switch it, or split it if other functions in the unit break).

Requires report mode (`make objdiff-config`). Restores the tree; safe to run in a
clone while workers build elsewhere.
"""
import importlib.util
import json
import os
import re
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CANDIDATES = {
    "cdk": ["--gcc", "gcc-2.7.2-cdk-psx"],
    "260": ["--gcc", "gcc-2.6.0-psx"],
    "g0": ["--gp", "-G0"],
}


def load_park():
    spec = importlib.util.spec_from_file_location("park", os.path.join(ROOT, "tools/scripts/park.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def parked_funcs(src_path):
    """[(name, include_asm_line)] for every stub that owns a park block."""
    park = load_park()
    text = open(src_path).read()
    out = []
    for asm_dir, name in re.findall(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', text):
        line = 'INCLUDE_ASM("%s", %s);' % (asm_dir, name)
        found = False
        inc = text.find(line)
        for m in park.BLOCK.finditer(text):
            if not re.search(r"\b" + name + r"\s*\(", m.group(1)):
                continue
            if m.end() <= inc and park.FILLER.match(text[m.end():inc]):
                found = True
            elif m.start() >= inc + len(line) and park.FILLER.match(text[inc + len(line):m.start()]):
                found = True
            if found:
                break
        if found:
            out.append((name, line))
    return out


def unpark_all(src_path, funcs):
    park = load_park()
    for name, line in funcs:
        text = open(src_path).read()
        out = park.unpark(text, name, line)
        if out is not None:
            open(src_path, "w").write(out)


def score(unit, flags):
    # try_flags.sh reports a spurious compile-failed while the container is busy
    scores = {}
    for attempt in range(3):
        r = subprocess.run([os.path.join(ROOT, "tools/scripts/try_flags.sh"), unit, "--all"] + flags,
                           cwd=ROOT, capture_output=True, text=True)
        if "compile-failed" not in r.stdout:
            break
        time.sleep(5)
    for line in r.stdout.splitlines():
        m = re.match(r"\s*([\d.]+|--)\s+(\w+)$", line)
        if m and m.group(1) != "--":
            scores[m.group(2)] = float(m.group(1))
    return scores


def main():
    flags = [a for a in sys.argv[1:] if a.startswith("-")]
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    # --own-only: just the unit's own preset (the cheapest question - "is this park stale?")
    # --one: un-park one body at a time, for units where un-parking all of them does not compile
    cands = [] if "--own-only" in flags else list(CANDIDATES)
    one = "--one" in flags
    units = args or sorted(
        os.path.relpath(p, os.path.join(ROOT, "src"))[:-2]
        for d, _, fs in os.walk(os.path.join(ROOT, "src")) for p in [os.path.join(d, f) for f in fs]
        if p.endswith(".c"))
    print("unit\tfunc\town\t" + "\t".join(cands))
    for unit in units:
        src = os.path.join(ROOT, "src", unit + ".c")
        funcs = parked_funcs(src)
        if not funcs:
            continue
        if one:
            cols = {"own": {}}
            for name, line in funcs:
                park = load_park()
                out = park.unpark(open(src).read(), name, line)
                if out is None:
                    continue
                open(src, "w").write(out)
                try:
                    cols["own"].update(score(unit, []))
                finally:
                    subprocess.run(["git", "checkout", "--", src], cwd=ROOT)
            for c in cands:
                cols[c] = {}
        else:
            unpark_all(src, funcs)
            try:
                cols = {"own": score(unit, [])}
                for c in cands:
                    cols[c] = score(unit, CANDIDATES[c])
            finally:
                subprocess.run(["git", "checkout", "--", src], cwd=ROOT)
        for name, _ in funcs:
            row = [unit, name] + ["%.2f" % cols[c][name] if name in cols[c] else "failed"
                                  for c in ["own"] + cands]
            print("\t".join(row), flush=True)


if __name__ == "__main__":
    main()
