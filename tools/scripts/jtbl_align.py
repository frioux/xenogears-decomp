#!/usr/bin/env python3
"""Screen which of a unit's `switch` stubs can be converted without moving the overlay.

cc1 emits `.align 3` in front of EVERY jump table it generates, so a table retail
packed at a 4-byte-aligned offset (offset % 8 == 4, measured inside the unit's
`.rodata`) comes out 4 bytes later from C, the unit's `.rodata` grows by 4 bytes, and
every address after it in the overlay shifts - while objdiff still reads 100.00%.
So only the tables retail itself aligned to 8 inside the unit's `.rodata` range are
reproducible by a `switch`. That alignment is range-relative, not absolute: gas
`.align` is section-relative and linker/<bin>.ld places the unit's object with
SUBALIGN(4), so a table at an 8-aligned VRAM address can still be 4-mod-8 in the
range (and vice versa).

    tools/scripts/jtbl_align.py battle/main/misc11        # one unit
    tools/scripts/jtbl_align.py --report battle           # every unit of a binary
    tools/scripts/jtbl_align.py --all battle              # same listing (nothing to apply)

One line per stub that references a `jtbl_` label:

    OK      battle/main/misc12  func_80082D4C  jtbl_80070150 +0x60 8 words pad +0
    PAD+4   battle/main/misc11  func_80081318  jtbl_80070030 +0x1B4 8 words pad +4

The model: every other datum in the unit's `.rodata` range (the other stubs' tables,
already-converted C tables, named data) keeps retail's position, so the offset a C
table lands at is the running offset rounded up to 8, and `pad` is the signed
difference from where retail put it. The verdict is per stub - "convert this one and
leave the rest asm" - so it does not depend on whether the stub's neighbours convert.

Not modelled, judge those with tools/scripts/check_bin.sh:
  * a table retail padded with a trailing word C never emits (more `.word`s than
    `switch` case labels) reports OK and loses 4 bytes unless the datum after it is
    8-aligned, where cc1's own padding absorbs the word. Count words against cases.
  * a table whose C form is a different length for any other reason (a default case
    that is not the last one, a non-dense range) shifts everything after it.

Verdicts other than OK/PAD show the layout cannot be reasoned about from the config:
  UNOWNED  the unit has no `.rodata` subsegment in config/<bin>.yaml, so a C table is
           never placed at retail's address (a `[off, rodata]` blob owns the bytes;
           see own_rodata.py)
  NOBLOB   the unit owns its range but asm/<bin>/data/<unit>.rodata.s is missing
  NOLABEL  a referenced table is not a label inside the unit's range
Exit status is 0 always; it is a listing, not a gate.

The two cases that pin the model (re-check after any change to table_pads):
`battle/main/misc11` must be PAD+4 for every table after func_80080160's - round 128
built that unit's whole family and its .rodata ended at 0x278 instead of 0x274 - and
every table in `battle/main/misc12` must be OK (round 64 converted three of them).
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SUB = re.compile(r'^\s*- \[(0x[0-9A-Fa-f]+|\d+)(?:,\s*([^,\]]+))?(?:,\s*([^\]]+))?\]\s*$')
LABEL = re.compile(r'^(?:dlabel|glabel) (\w+)')
LINE = re.compile(r'^\s*/\* ([0-9A-F]+) ([0-9A-F]{8}) ')
JTBL = re.compile(r'jtbl_([0-9A-F]{8})')


def subsegments(text):
    """[(start, type, name)] for every subsegment line of a splat config."""
    out = []
    for l in text.splitlines():
        m = SUB.match(l)
        if m:
            out.append((int(m.group(1), 0), (m.group(2) or '').strip(), (m.group(3) or '').strip()))
    return out


def unit_range(text, unit):
    """(start, end) file offsets of the unit's own `.rodata`, or None."""
    ents = subsegments(text)
    for i, (st, t, name) in enumerate(ents):
        if t == '.rodata' and name == unit:
            j = i + 1
            while j < len(ents) and ents[j][1] == '.rodata' and ents[j][2] == unit:
                j += 1
            return st, (ents[j][0] if j < len(ents) else None)
    return None


def blob_labels(binary, unit):
    """[(file offset, vram, name)] for every datum splat found in the unit's `.rodata`."""
    path = ROOT / f"asm/{binary}/data/{unit}.rodata.s"
    if not path.is_file():
        return None
    lines = path.read_text().splitlines()
    out = []
    for i, l in enumerate(lines):
        m = LABEL.match(l)
        if m and i + 1 < len(lines):
            n = LINE.match(lines[i + 1])
            if n:
                out.append((int(n.group(1), 16), int(n.group(2), 16), m.group(1)))
    return out


def stub_tables(binary, unit):
    """{stub: [table vram addresses]} for the unit's still-INCLUDE_ASM'd stubs."""
    d = ROOT / f"asm/{binary}/nonmatchings/{unit}"
    out = {}
    for f in sorted(d.glob("*.s")):
        tabs = sorted({int(m.group(1), 16) for m in JTBL.finditer(f.read_text())})
        if tabs:
            out[f.stem] = tabs
    return out


def table_pads(labels, lo, hi):
    """{table vram: (range offset, words, pad)}; pad is what cc1's `.align 3` costs."""
    out = {}
    cursor = lo
    for k, (off, vram, name) in enumerate(labels):
        end = labels[k + 1][0] if k + 1 < len(labels) else hi
        length = (end - off) if end is not None else 0
        if off > cursor:
            cursor = off  # unnamed bytes keep retail's position
        if name.startswith("jtbl_"):
            pad = -(cursor - lo) % 8
            out[vram] = (off - lo, length // 4, cursor + pad - off)
        cursor = off + length
    return out


def report_unit(binary, unit, ytext):
    """Print one line per jtbl stub of the unit; return (convertible, stubs) counts."""
    stubs = stub_tables(binary, unit)
    if not stubs:
        return 0, 0
    rng = unit_range(ytext, unit)
    pads = None
    if rng:
        labels = blob_labels(binary, unit)
        if labels is not None:
            pads = table_pads(labels, rng[0], rng[1])
    ok = 0
    for name in sorted(stubs):
        tabs = stubs[name]
        names = "  ".join(f"jtbl_{t:08X}" for t in tabs)
        if rng is None:
            print(f"{'UNOWNED':8} {binary}/{unit}  {name}  {names}  "
                  "(unit owns no .rodata; a C table would not be placed)")
            continue
        if pads is None:
            print(f"{'NOBLOB':8} {binary}/{unit}  {name}  {names}  "
                  f"(no asm/{binary}/data/{unit}.rodata.s)")
            continue
        got = [pads.get(t) for t in tabs]
        if None in got:
            print(f"{'NOLABEL':8} {binary}/{unit}  {name}  {names}  "
                  "(table is not a label in the unit's .rodata range)")
            continue
        worst = max(g[2] for g in got)
        if worst == 0:
            ok += 1
        verdict = "OK" if worst == 0 else f"PAD{worst:+d}"
        cells = "  ".join(f"jtbl_{t:08X} +0x{r:X} {w} words pad {d:+d}"
                          for t, (r, w, d) in zip(tabs, got))
        print(f"{verdict:8} {binary}/{unit}  {name}  {cells}")
    return ok, len(stubs)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("target", help="<bin>/<unit>, or <bin> with --report")
    ap.add_argument("--report", "--all", dest="report", action="store_true",
                    help="one line per jtbl stub for every unit of <bin> (nothing to apply)")
    a = ap.parse_args()
    binary, _, unit = a.target.partition('/')
    if a.report and unit:
        sys.exit("--report/--all take a binary name, not a unit")
    if not (ROOT / f"asm/{binary}/nonmatchings").is_dir():
        sys.exit("asm/ not generated: run `make objdiff-config` (or any build) first")
    ytext = (ROOT / f"config/{binary}.yaml").read_text()
    if a.report:
        units = [name for _, t, name in subsegments(ytext) if t == 'c']
    else:
        units = [unit]
    ok = total = 0
    for u in units:
        if not (ROOT / f"asm/{binary}/nonmatchings/{u}").is_dir():
            continue
        o, n = report_unit(binary, u, ytext)
        ok += o
        total += n
    if a.report:
        print(f"{binary}: {ok} convertible of {total} jtbl stubs")
    elif total == 0:
        print(f"{binary}/{units[0]}: no stub references a jtbl_ label")
    sys.exit(0)


if __name__ == "__main__":
    main()
