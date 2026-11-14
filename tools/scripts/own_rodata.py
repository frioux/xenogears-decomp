#!/usr/bin/env python3
"""Make a unit's C own its switch jump tables (overlay `.rodata` migration).

    tools/scripts/own_rodata.py battle/main/misc12          # edit config/battle.yaml
    tools/scripts/own_rodata.py --report battle             # dry run over every unit of a binary
    tools/scripts/own_rodata.py --all battle                # apply to every unit that has no problems

Overlays keep all their `.rodata` in one asm blob (`[0x0, rodata]` in the yaml), so a
C `switch` makes cc1 emit a second copy of its table into the unit's object and the
overlay shifts. This tool carves the byte range holding the unit's `jtbl_` tables out of
that blob and gives it to the unit (`[off, .rodata, <unit>]`, the shape field.yaml uses).
splat then moves each table into the `.s` of the stub that references it
(`.section .rodata` at the top of the stub), the unit's `.o` owns it, and:

  * with no C changes the output is byte-identical (the tables come back through
    INCLUDE_ASM in the same order), and
  * a stub converted to a `switch` has its table emitted by cc1 at the same position,
    because gcc 2.x writes a function's table right after the function and the unit
    object is laid out in file (= address) order.

Refused (reported, nothing written) when the range cannot be owned cleanly:
  ORDER    table addresses are not increasing in function-address order (a C file is
           laid out in address order, so the tables could not be reproduced)
  FOREIGN  a label that is not one of the unit's tables sits inside the range (strings or
           other data of someone else would be pulled into the unit)
  NOBLOB   no `rodata` subsegment covers the range (already owned, or not an overlay)
After running it do `make check`, then convert stubs as usual and judge with
`tools/scripts/check_bin.sh <bin>`.
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SUB = re.compile(r'^(\s*)- \[(0x[0-9A-Fa-f]+|\d+)(?:,\s*([^,\]]+))?(?:,\s*([^\]]+))?\]\s*$')
INSN = re.compile(r'^\s*/\* ([0-9A-F]+) ([0-9A-F]{8}) [0-9A-F]{8} \*/')
JTBL = re.compile(r'jtbl_([0-9A-F]{8})')


def entries(lines):
    """[(line index, indent, start, type, name)] for every subsegment line."""
    out = []
    for i, l in enumerate(lines):
        m = SUB.match(l)
        if m:
            out.append((i, m.group(1), int(m.group(2), 0), (m.group(3) or '').strip(), (m.group(4) or '').strip()))
    return out


def vram_base(text):
    return int(re.search(r'^\s*vram:\s*(0x[0-9A-Fa-f]+)', text, re.M).group(1), 16)


def blob_labels(binary):
    """Sorted [(vram, name)] for every dlabel in the overlay's asm rodata blobs and unit-owned stubs."""
    labels = {}
    files = list((ROOT / f"asm/{binary}/data").glob("*.rodata.s"))
    for f in files:
        lines = f.read_text().splitlines()
        for i, l in enumerate(lines):
            m = re.match(r'dlabel (\w+)', l)
            if m and i + 1 < len(lines):
                n = re.match(r'\s*/\* [0-9A-F]+ ([0-9A-F]{8})', lines[i + 1])
                if n:
                    labels[int(n.group(1), 16)] = m.group(1)
    return sorted(labels.items())


def unit_stubs(binary, unit):
    """{func: (address, insn count, [table addresses in first-use order])} for the unit's stubs."""
    d = ROOT / f"asm/{binary}/nonmatchings/{unit}"
    out = {}
    for f in sorted(d.glob("*.s")):
        text = f.read_text()
        addr = None
        # address of the first instruction of the function itself (after `.section .text`)
        body = text.split('.section .text', 1)[-1]
        for l in body.splitlines():
            m = INSN.match(l)
            if m:
                addr = int(l.split()[2], 16)
                break
        n = sum(1 for l in body.splitlines() if INSN.match(l))
        tabs = []
        for m in JTBL.finditer(text):
            a = int(m.group(1), 16)
            if a not in tabs:
                tabs.append(a)
        out[f.stem] = (addr, n, tabs)
    return out


def plan(binary, unit, ytext, labels):
    """(status, details, (lo, hi) or None) for one unit."""
    stubs = {k: v for k, v in unit_stubs(binary, unit).items() if v[2]}
    if not stubs:
        return "NOJTBL", "no stub references a jtbl_", None
    base = vram_base(ytext)
    lines = ytext.splitlines()
    for _, _, _, t, name in entries(lines):
        if t == '.rodata' and name == unit:
            return "DONE", "unit already has a .rodata subsegment", None
    tabs = sorted({a for v in stubs.values() for a in v[2]})
    # order: functions by address, their tables must be increasing
    seq = []
    for f, (addr, _, ts) in sorted(stubs.items(), key=lambda kv: kv[1][0]):
        seq += [(addr, t, f) for t in ts]
    firsts = {}
    for addr, t, f in seq:
        firsts.setdefault(t, (addr, f))
    byfunc = sorted(firsts.items(), key=lambda kv: kv[1][0])
    for (t1, (a1, f1)), (t2, (a2, f2)) in zip(byfunc, byfunc[1:]):
        if t2 < t1:
            return "ORDER", f"{f2} (table {t2:08X}) follows {f1} (table {t1:08X})", None
    lo = tabs[0]
    after = [a for a, _ in labels if a > tabs[-1]]
    ents = entries(lines)
    blob = None
    for k, (i, ind, st, t, name) in enumerate(ents):
        if t == 'rodata' and base + st <= lo and k + 1 < len(ents):
            nxt = base + ents[k + 1][2]
            if lo < nxt:
                blob = (st, nxt)
    if not blob:
        return "NOBLOB", f"no `rodata` subsegment covers {lo:08X}", None
    hi = min([a for a in after] + [blob[1]])
    if hi > blob[1]:
        hi = blob[1]
    foreign = [(a, n) for a, n in labels if lo <= a < hi and a not in tabs]
    if foreign:
        return "FOREIGN", "labels inside range: " + ", ".join(f"{n}" for _, n in foreign[:4]), None
    return "OK", f"{len(tabs)} tables {lo:08X}..{hi:08X}, {len(stubs)} stubs", (lo, hi)


def apply(ytext, unit, lo, hi):
    base = vram_base(ytext)
    lines = ytext.split('\n')
    ents = entries(lines)
    for k, (i, ind, st, t, name) in enumerate(ents):
        nxt = base + ents[k + 1][2] if k + 1 < len(ents) else None
        if t == 'rodata' and nxt and base + st <= lo and hi <= nxt:
            new = []
            if base + st < lo:
                new.append(f"{ind}- [0x{st:X}, rodata]")
            new.append(f"{ind}- [0x{lo - base:X}, .rodata, {unit}]")
            if hi < nxt:
                new.append(f"{ind}- [0x{hi - base:X}, rodata]")
            lines[i:i + 1] = new
            return '\n'.join(lines)
    raise SystemExit("internal: no blob for range")


def units_of(binary, ytext):
    return [name for _, _, _, t, name in entries(ytext.splitlines()) if t == 'c']


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("target", help="<bin>/<unit>, or <bin> with --all/--report")
    ap.add_argument("--all", action="store_true", help="apply to every unit of <bin> that plans OK")
    ap.add_argument("--report", action="store_true", help="dry run over every unit of <bin>")
    a = ap.parse_args()
    if a.all or a.report:
        binary, units = a.target, None
    else:
        binary, _, unit = a.target.partition('/')
        units = [unit]
    ypath = ROOT / f"config/{binary}.yaml"
    ytext = ypath.read_text()
    if units is None:
        units = units_of(binary, ytext)
    if not (ROOT / f"asm/{binary}/nonmatchings").is_dir():
        sys.exit("asm/ not generated: run `make objdiff-config` (or any build) first")
    labels = blob_labels(binary)
    bad = 0
    for u in units:
        if not (ROOT / f"asm/{binary}/nonmatchings/{u}").is_dir():
            continue
        status, detail, rng = plan(binary, u, ytext, labels)
        if status in ("NOJTBL", "DONE") and (a.all or a.report):
            continue
        stubs = {k: v for k, v in unit_stubs(binary, u).items() if v[2]}
        insns = sum(v[1] for v in stubs.values())
        print(f"{status:8} {binary}/{u}: {detail} [{len(stubs)} jtbl stubs, {insns} insns]")
        if status != "OK":
            bad += 1
            continue
        if not a.report:
            ytext = apply(ytext, u, *rng)
            # later units look for a blob again; labels are unchanged (addresses are stable)
    if not a.report:
        ypath.write_text(ytext)
        print("wrote", ypath.relative_to(ROOT), "- now run `make check` (must stay all OK with no C changes)")
    sys.exit(1 if (bad and not (a.all or a.report)) else 0)


if __name__ == "__main__":
    main()
