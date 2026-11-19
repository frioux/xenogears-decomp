#!/usr/bin/env python3
"""List the stubs that are worth briefing, per unit, from the unit's OWN stub lines.

    python3 tools/scripts/fresh_census.py                 # units ranked by clean fresh stubs
    python3 tools/scripts/fresh_census.py battle/misc23   # one unit, stub by stub
    python3 tools/scripts/fresh_census.py --max-insns 150 # narrower size cap (default 400)

`fresh_stubs.py` under-reports badly: at round 285 it said 3 stubs while this census
found 61 units holding non-parked stubs of 150 instructions or fewer, and the rounds
briefed from the census landed 4-11 matches each. A stub is "clean fresh" when its own
`.s` has no jump table, no stack switch (`sw $sp`, matched with `+` so splat's column
alignment does not hide it) and no `/* Handwritten function */` marker, and no parked
body sits next to its INCLUDE_ASM line.

Caveat that cost a round: build the list from each unit's OWN `INCLUDE_ASM` lines, as
this script does. Attributing a stub to a unit by grepping the function name across
`src/` gives the wrong unit whenever the address is a twin in another overlay, and
round 291 briefed four function names that exist nowhere in the unit it named.
"""
import importlib.util
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def load_park():
    spec = importlib.util.spec_from_file_location("park", os.path.join(ROOT, "tools/scripts/park.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def stub_info():
    """{'<bin>/nonmatchings/<dir>/<func>': (insns, has_jtbl, has_stack_switch, handwritten)}"""
    info = {}
    base = os.path.join(ROOT, "asm")
    for dirpath, _, files in os.walk(base):
        if "nonmatchings" not in dirpath:
            continue
        for f in files:
            if not f.endswith(".s"):
                continue
            path = os.path.join(dirpath, f)
            text = open(path, errors="ignore").read()
            key = os.path.relpath(path, base)[:-2]
            info[key] = (
                len(re.findall(r"^\s+/\* [0-9A-F]+ ", text, re.M)),
                bool(re.search(r"\bjtbl_", text)),
                bool(re.search(r"sw +\$sp", text)),
                "Handwritten function" in text,
            )
    return info


def parked_funcs(text):
    park = load_park()
    out = set()
    for asm, name in re.findall(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', text):
        line = 'INCLUDE_ASM("%s", %s);' % (asm, name)
        inc = text.find(line)
        for m in park.BLOCK.finditer(text):
            if not re.search(r"\b" + name + r"\s*\(", m.group(1)):
                continue
            if m.end() <= inc and park.FILLER.match(text[m.end():inc]):
                out.add(name)
                break
            if m.start() >= inc + len(line) and park.FILLER.match(text[inc + len(line):m.start()]):
                out.add(name)
                break
    return out


def unit_stubs(info, max_insns):
    """{unit: [(insns, func)]} for the clean fresh stubs of every unit."""
    rows = {}
    for dirpath, _, files in os.walk(os.path.join(ROOT, "src")):
        for f in files:
            if not f.endswith(".c"):
                continue
            path = os.path.join(dirpath, f)
            unit = os.path.relpath(path, os.path.join(ROOT, "src"))[:-2]
            text = open(path).read()
            parked = parked_funcs(text)
            for asm, name in re.findall(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', text):
                got = info.get(asm[len("asm/"):] + "/" + name)
                if not got:
                    continue
                insns, jtbl, stack_switch, handwritten = got
                if jtbl or stack_switch or handwritten or insns > max_insns or name in parked:
                    continue
                rows.setdefault(unit, []).append((insns, name))
    return rows


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    max_insns = 400
    for i, a in enumerate(sys.argv):
        if a == "--max-insns":
            max_insns = int(sys.argv[i + 1])
            args = [x for x in args if x != sys.argv[i + 1]]
    rows = unit_stubs(stub_info(), max_insns)
    if args:
        for unit in args:
            print("%s (%d clean fresh stubs):" % (unit, len(rows.get(unit, []))))
            for insns, name in sorted(rows.get(unit, [])):
                print("  - %s %d insns" % (name, insns))
        return
    ranked = sorted(rows.items(), key=lambda kv: (-len(kv[1]), sum(x[0] for x in kv[1])))
    print("%d clean fresh stubs in %d units (<=%d insns)" % (
        sum(len(v) for v in rows.values()), len(rows), max_insns))
    for unit, stubs in ranked[:25]:
        print("%3d stubs %6d insns  %s" % (len(stubs), sum(x[0] for x in stubs), unit))


if __name__ == "__main__":
    main()
