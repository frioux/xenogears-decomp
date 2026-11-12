#!/usr/bin/env python3
"""Move whole functions of a unit into their own TU so they can carry another preset.

    tools/scripts/split_unit.py battle/main/misc17 \\
        --to 0x8009413C --preset XenoBattleTailCdk --name misc17_psx

Presets are keyed per file in `gears.toml`, so a unit whose functions need two
different compilers has to become several files. `--to` names the functions that
need the OTHER preset (the one the new file will get); they must be *contiguous*
runs of the unit's functions, because a splat `c` range is a byte range. Each run
of same-preset functions becomes a file:

  - the first run stays in `src/<bin>/<unit>.c` and keeps the unit's current preset
  - every later run gets `<unit>_r<N>.c` and a new `[[build_preset]]` block

Everything is written here: the yaml `c` ranges (computed from `symbol_addrs`),
the new `.c` files (chunks copied verbatim, `INCLUDE_ASM` paths rewritten to the
new run's asm directory), and the `gears.toml` entry. What the tool cannot do is
the C inside: after the split set the new file's preset in gears.toml and run
`make check` - the split itself must be byte-identical.

Then un-park the bodies the new preset makes exact with `tools/scripts/park.py`.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INCLUDE = re.compile(r'^INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', re.M)
DEF_HEAD = re.compile(r'^[A-Za-z_][\w \t*]*?\b(\w+)\s*\([^;]*\)\s*\{', re.M)


def unit_range(binary, unit):
    """(file offset of the unit's first byte, its yaml match object)."""
    yml = (ROOT / f"config/{binary}.yaml").read_text()
    m = re.search(r'^\s*- \[(0x[0-9A-Fa-f]+), c, %s\]\s*$' % re.escape(unit), yml, re.M)
    if not m:
        sys.exit(f"config/{binary}.yaml has no `c` range for {unit}")
    return int(m.group(1), 16), m


def symtab(binary):
    return {m.group(1) for m in re.finditer(r'^(\w+)\s*=\s*0x',
                                            (ROOT / f"config/symbol_addrs.{binary}.txt").read_text(), re.M)}


def addresses(unit):
    """{function: offset inside the unit} straight from the expected object.

    Using the object rather than symbol_addrs means a function that is only
    referenced (or newly converted and not yet named) still gets a correct split
    point.
    """
    obj = ROOT / f"expected/asm/{unit}.s.o"
    if not obj.is_file():
        sys.exit("need report mode (make objdiff-config) for expected/asm/%s.s.o" % unit)
    out = subprocess.run(["nm", str(obj)], capture_output=True, text=True).stdout
    addr = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "Tt":
            addr[parts[2]] = int(parts[0], 16)
    return addr


def shared_decls(src):
    """Typedef blocks and #defines, which any run split off mid-file may still need.

    A unit's mid-file `typedef struct {...} X;` is not in the text before the first
    function, so a run that starts after it would not compile without it. Repeating
    an identical typedef in one TU is legal C89, so every new file gets all of them.
    Data definitions are NOT shareable (two files defining D_x would not link), so
    only typedefs and #defines are collected here; they stay where they were too.
    """
    lines = src.splitlines(True)
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("#define"):
            out.append(line)
            i += 1
            continue
        if line.startswith("typedef"):
            depth = line.count("{") - line.count("}")
            block = [line]
            i += 1
            while i < len(lines) and not (depth <= 0 and block[-1].rstrip().endswith(";")):
                depth += lines[i].count("{") - lines[i].count("}")
                block.append(lines[i])
                i += 1
            out.append("".join(block))
            continue
        i += 1
    return "".join(dict.fromkeys(out))


def chunks(src, names):
    """Split the source into (function name, text) chunks in file order."""
    marks = []
    for m in INCLUDE.finditer(src):
        marks.append((m.start(), m.group(2)))
    for m in DEF_HEAD.finditer(src):
        if m.group(1) in names:
            marks.append((m.start(), m.group(1)))
    marks.sort()
    # a chunk runs from its first mark to the next mark that belongs to another function
    out = []
    for i, (pos, name) in enumerate(marks):
        nxt = len(src)
        for pos2, name2 in marks[i + 1:]:
            if name2 != name:
                nxt = pos2
                break
        out.append((pos, name))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit", help="e.g. battle/main/misc17")
    ap.add_argument("--to", required=True,
                    help="comma-separated functions that need the NEW preset")
    ap.add_argument("--preset", required=True,
                    help="name of the [[build_preset]] the new files get (must exist in gears.toml)")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()
    binary, unit = a.unit.split("/", 1)
    want_other = {n.strip() for n in a.to.split(",") if n.strip()}

    src_path = ROOT / f"src/{binary}/{unit}.c"
    src = src_path.read_text()
    header_end = INCLUDE.search(src).start() if INCLUDE.search(src) else 0
    header = src[:header_end]

    addr = addresses(a.unit)
    start_off, _ = unit_range(binary, unit)
    off = lambda name: start_off + addr[name]

    marks = chunks(src, set(addr))
    order = []
    seen = set()
    for pos, name in marks:
        if name not in seen:
            seen.add(name)
            order.append((pos, name))
    if not order:
        sys.exit("no functions found")

    # group consecutive functions by which preset they need
    groups = []
    for pos, name in order:
        flag = name in want_other
        if groups and groups[-1][0] == flag:
            groups[-1][1].append((pos, name))
        else:
            groups.append((flag, [(pos, name)]))
    # the unit's own file must keep the first group; if the first group needs the
    # new preset, flip every flag so the first group stays put
    if groups[0][0]:
        groups = [(not f, g) for f, g in groups]
        if groups[0][0]:
            sys.exit("the whole unit needs the new preset - switch the preset instead of splitting")

    addr = addresses(a.unit)
    start_off, _ = unit_range(binary, unit)
    off = lambda name: start_off + addr[name]

    print(f"{a.unit}: {len(order)} functions in {len(groups)} run(s)")
    edits = []
    pending = ""
    for i, (needs_new, members) in enumerate(groups):
        names = [n for _, n in members]
        fname = unit if i == 0 else f"{unit}_r{i}"
        asm_dir = f"asm/{binary}/nonmatchings/{fname}"
        print(f"  run {i}: {'NEW PRESET' if needs_new else 'unit preset'} "
              f"{len(names)} fn off 0x{off(names[0]):X}  src/{binary}/{fname}.c  {names[0]}..{names[-1]}")
        body_start = members[0][0]
        body_end = groups[i + 1][1][0][0] if i + 1 < len(groups) else len(src)
        body = pending + src[body_start:body_end]
        pending = ""
        # a function's leading comment block must travel with the function, not stay
        # at the end of the previous chunk (an unterminated /* does not compile)
        open_at = body.rfind("/*")
        if open_at >= 0 and body.find("*/", open_at) < 0:
            pending, body = body[open_at:], body[:open_at]
        if i > 0:
            body = shared_decls(src) + "\n" + body
        body = re.sub(r'(INCLUDE_ASM\(\s*")asm/[^"]*(")', lambda m: m.group(1) + asm_dir + m.group(2), body)
        edits.append((fname, header + body))
        edits[-1] = (fname, edits[-1][1], names, needs_new, off(names[0]))

    if a.dry_run:
        return

    for fname, text, _, _, _ in edits:
        path = ROOT / f"src/{binary}/{fname}.c"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        print(f"wrote {path.relative_to(ROOT)}")

    # yaml: replace the single range with one range per run
    yml_path = ROOT / f"config/{binary}.yaml"
    yml = yml_path.read_text()
    old = re.search(r'^(\s*)- \[0x[0-9A-Fa-f]+, c, %s\]\s*$\n' % re.escape(unit), yml, re.M)
    indent = old.group(1)
    lines = []
    for fname, _, _, _, offset in edits:
        lines.append(f"{indent}- [0x{offset:X}, c, {fname}]\n")
    yml = yml[:old.start()] + "".join(lines) + yml[old.end():]
    yml_path.write_text(yml)
    print(f"rewrote {yml_path.relative_to(ROOT)}: {len(edits)} c range(s)")

    # gears.toml: the new files become path keys of the requested preset
    g_path = ROOT / "gears.toml"
    g = g_path.read_text()
    block = re.search(r'\[\[build_preset\]\]\nname = "%s"\n(?:.*?\n)*?paths = \[(.*?)\]' % re.escape(a.preset),
                      g, re.S)
    if not block:
        sys.exit(f"gears.toml has no preset with a paths = [...] list named {a.preset!r}")
    before, inside, after = g[:block.start(1)], block.group(1), g[block.end(1):]
    if inside.strip() and not inside.rstrip().endswith(","):
        inside = inside.rstrip() + ",\n"
    keys = "".join('    "%s/%s.",\n' % (binary, fname) for fname, _, _, needs_new, _ in edits if needs_new)
    g = before + inside + keys + after
    g_path.write_text(g)
    print(f"added {len(keys.splitlines())} path key(s) to preset {a.preset!r} in gears.toml")

if __name__ == "__main__":
    main()
