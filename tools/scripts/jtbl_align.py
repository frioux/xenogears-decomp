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

One line per stub that references a `jtbl_` label. Converted functions are listed too
(marked `(converted)`, and not counted): they are known to link, so a verdict flip on
one of them means the model broke.

    OK       battle/main/misc12  func_80082D4C  jtbl_80070150 +0x60 8 words 8 cases pad +0
    PAD+4    battle/main/misc11  func_80081318  jtbl_80070030 +0x1B4 8 words 8 cases pad +4
    PADWORD  battle/main/misc23  func_800A6884  jtbl_80070554 +0x0 6 words 5 cases pad +0 PADWORD

Two independent checks, both needed:

1. ALIGNMENT. Every other datum in the unit's `.rodata` range (the other stubs'
tables, already-converted C tables, named data) keeps retail's position, so the offset
a C table lands at is the running offset rounded up to 8, and `pad` is the signed
difference from where retail put it. The verdict is per stub - "convert this one and
leave the rest asm" - so it does not depend on whether the stub's neighbours convert.

2. TABLE LENGTH. A `switch` emits one word per case, so the table's word count must
equal the case count read out of the stub's own dispatch: `sltiu reg,IDX,0xN` (often
after `addiu IDX,IDX,-M`, which the bound is applied to, so the cases run M..M+N-1).
Either way a C switch over that range emits exactly N words, and retail's extra word
is a pad.
  PADWORD    retail's table is longer than N words at the END of the range: the pad
             word is retail's, no C emits it, the unit's `.rodata` ends short and the
             overlay shifts (round 130: menu/misc10 func_801E3ECC, 8 words for
             `sltiu ...,0x7`; battle/misc23 func_800A6884, 6 words for `sltiu ...,0x5`).
             When a jump table follows the pad at the aligned end, that word is cc1's
             own `.align 3` and the length is fine (world_map/misc11 func_8008A72C:
             `sltiu ...,0x41`, 66 words whose last is that padding).
  SHORTWORD  retail's table is SHORTER than N words: the source has cases with no
             body of their own, and the C switch has to be extended with empty
             `case` labels until the table is N entries long (round 130 left these
             out of world_map/misc11 and got a 6-entry table where 13 were needed).
  NOCASE     the dispatch's bound could not be read out of the .s

Verdicts other than OK/PAD*/PADWORD/SHORTWORD/NOCASE show the layout cannot be reasoned
about from the config:
  UNOWNED  the unit has no `.rodata` subsegment in config/<bin>.yaml, so a C table is
           never placed at retail's address (a `[off, rodata]` blob owns the bytes;
           see own_rodata.py)
  NOBLOB   the unit owns its range but asm/<bin>/data/<unit>.rodata.s is missing
  NOLABEL  a referenced table is not a label inside the unit's range
Exit status is 0 always; it is a listing, not a gate. `--report` ends with a tally of
the verdicts, so the rejected stubs are the ones after the `of`.

Still not modelled, judge those with tools/scripts/check_bin.sh: a table whose C form
is a different length for another reason (a default case that is not the last one, a
different jump-table alignment inside the table).

The cases that pin the model (re-check after any change to table_pads/dispatch_counts):
`battle/main/misc11` must be PAD+4 for every table after func_80080160's - round 128
built that unit's whole family and its .rodata ended at 0x278 instead of 0x274 - and
every table in `battle/main/misc12` must be OK (round 64 converted three of them).
Length: func_801E3ECC and func_800A6884 must be PADWORD; world_map/misc11's
func_8008A72C (65 and 5 cases) and func_8008BB40 (13) OK; battle/misc18's
BattleRollSlotConditionAction (6) and BattleSetSlotConditionActionValue (8) OK.
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
INSN = re.compile(r'^[ \t]*/\* [0-9A-F]+ [0-9A-F]{8} [0-9A-F]{8} \*/[ \t]*(\S+)[ \t]*(.*)$', re.M)
OPERANDS = re.compile(r'(\$\w+)\s*,\s*(\$\w+)\s*,\s*(-?(?:0x[0-9A-Fa-f]+|\d+))')
RANK = {"OK": 0, "SHORTWORD": 1, "NOCASE": 2, "PADWORD": 3}


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
    """{stub: (asm text, [table vram addresses], already converted)}."""
    out = {}
    for sub, converted in (("nonmatchings", False), ("matchings", True)):
        for f in sorted((ROOT / f"asm/{binary}/{sub}/{unit}").glob("*.s")):
            text = f.read_text()
            tabs = sorted({int(m.group(1), 16) for m in JTBL.finditer(text)})
            if tabs:
                out[f.stem] = (text, tabs, converted)
    return out


def dispatch_counts(text):
    """{table vram: words a C `switch` over the same cases emits}.

    A dispatch starts at `lui v,%hi(jtbl_X)`: either the `lw v,%lo(jtbl_X)($at)` form
    with the `sll v,IDX,2` fed into `$at` just before it, or `addiu v,v,%lo(jtbl_X)`
    with the sll just after. IDX is bounded by `sltiu v,IDX,0xN` - after retail's own
    `addiu IDX,IDX,-M` when the cases do not start at 0, which is why the bound is
    already the case count and not max+1."""
    out = {}
    insns = [(m.group(1), m.group(2)) for m in INSN.finditer(text)]
    for i, (op, args) in enumerate(insns):
        if op != 'lui' or not (t := JTBL.search(args)):
            continue
        near = sorted(range(max(i - 4, 0), min(i + 5, len(insns))), key=lambda j: abs(j - i))
        for j in near:
            m = OPERANDS.search(insns[j][1])
            if insns[j][0] != 'sll' or not m or int(m.group(3), 0) != 2:
                continue
            for k in range(j - 1, max(j - 10, -1), -1):
                b = OPERANDS.search(insns[k][1])
                if insns[k][0] in ('sltiu', 'slti') and b and b.group(2) == m.group(2):
                    out[int(t.group(1), 16)] = int(b.group(3), 0)
                    break
            if int(t.group(1), 16) in out:
                break
    return out


def table_pads(labels, lo, hi):
    """{table vram: (range offset, words, pad, end, next label is a jump table)}.

    `pad` is what cc1's `.align 3` costs; `words`/`end` are retail's extent of the
    table (to the next label, or to the end of the range)."""
    out = {}
    cursor = lo
    for k, (off, vram, name) in enumerate(labels):
        nxt = labels[k + 1] if k + 1 < len(labels) else None
        end = (nxt[0] if nxt else hi)
        length = (end - off) if end is not None else 0
        if off > cursor:
            cursor = off  # unnamed bytes keep retail's position
        if name.startswith("jtbl_"):
            pad = -(cursor - lo) % 8
            out[vram] = (off - lo, length // 4, cursor + pad - off,
                         end - lo if end is not None else off - lo,
                         bool(nxt) and nxt[2].startswith("jtbl_"))
        cursor = off + length
    return out


def table_verdict(rel, words, pad, end, next_table, n):
    """'OK' when a C table of `n` words lands at retail's offset and is as long."""
    if pad:
        return f"PAD{pad:+d}"
    if n is None:
        return "NOCASE"
    cend = rel + 4 * n
    if cend == end or (next_table and cend + (-cend) % 8 == end):
        return "OK"
    return "PADWORD" if end > cend else "SHORTWORD"


def report_unit(binary, unit, ytext):
    """Print one line per jtbl stub of the unit; return {verdict: count}."""
    stubs = stub_tables(binary, unit)
    tally = {}

    def note(verdict, converted):
        if not converted:
            tally[verdict] = tally.get(verdict, 0) + 1

    rng = unit_range(ytext, unit)
    pads = None
    if rng:
        labels = blob_labels(binary, unit)
        if labels is not None:
            pads = table_pads(labels, rng[0], rng[1])
    for name in sorted(stubs):
        text, tabs, converted = stubs[name]
        names = "  ".join(f"jtbl_{t:08X}" for t in tabs)
        tag = "  (converted)" if converted else ""
        if rng is None:
            print(f"{'UNOWNED':8} {binary}/{unit}  {name}  {names}  "
                  f"(unit owns no .rodata; a C table would not be placed){tag}")
            note("UNOWNED", converted)
            continue
        if pads is None:
            print(f"{'NOBLOB':8} {binary}/{unit}  {name}  {names}  "
                  f"(no asm/{binary}/data/{unit}.rodata.s){tag}")
            note("NOBLOB", converted)
            continue
        got = [pads.get(t) for t in tabs]
        if None in got:
            print(f"{'NOLABEL':8} {binary}/{unit}  {name}  {names}  "
                  f"(table is not a label in the unit's .rodata range){tag}")
            note("NOLABEL", converted)
            continue
        counts = dispatch_counts(text)
        cells = []
        verdicts = []
        for t, (r, w, d, end, next_table) in zip(tabs, got):
            n = counts.get(t)
            v = table_verdict(r, w, d, end, next_table, n)
            verdicts.append(v)
            cells.append(f"jtbl_{t:08X} +0x{r:X} {w} words "
                         f"{n if n is not None else '?'} cases pad {d:+d}"
                         + ("" if v == "OK" else f" {v}"))
        verdict = max(verdicts, key=lambda v: RANK.get(v, 4))  # 4: the PAD+n misses
        note(verdict, converted)
        print(f"{verdict:8} {binary}/{unit}  {name}  {'  '.join(cells)}{tag}")
    return tally


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
    tally = {}
    for u in units:
        if not (ROOT / f"asm/{binary}/nonmatchings/{u}").is_dir():
            continue
        for verdict, n in report_unit(binary, u, ytext).items():
            tally[verdict] = tally.get(verdict, 0) + n
    total = sum(tally.values())
    if a.report:
        ok = tally.get("OK", 0)
        rest = " ".join(f"{v} {n}" for v, n in sorted(tally.items()) if v != "OK")
        print(f"{binary}: {ok} convertible of {total} jtbl stubs" + (f" ({rest})" if rest else ""))
    elif total == 0:
        print(f"{binary}/{units[0]}: no stub references a jtbl_ label")
    sys.exit(0)


if __name__ == "__main__":
    main()
