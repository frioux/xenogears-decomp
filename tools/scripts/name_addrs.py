#!/usr/bin/env python3
"""Apply the names `rename_sym.py` skipped for having no symbol_addrs entry.

    tools/scripts/rename_sym.py -f names.txt      # first: renames what it can
    tools/scripts/name_addrs.py -f names.txt      # then: the rest, + the entries
    tools/scripts/name_addrs.py -f names.txt -n   # dry run

A sed alone is NOT enough: splat names a binary's own relocations (calls, `.data`
function tables) from `symbol_addrs`, so an overlay whose asm refers to the address
still emits `func_<addr>` and the link fails. This adds `NewName = 0xADDR;` (the
address is the old name's suffix) to the symbol_addrs file of every binary whose
`src/` or `asm/` mentions the new name.

Two rules keep splat happy, both learned the hard way in round 36:

- an entry must not land in a file whose yaml already loads another file with that
  name or vram (`slus_006.64` is loaded by every overlay, `gear_shop_menu` also
  loads `lib_gear`), or splat aborts with "Duplicate symbol detected" and nothing
  links;
- an overlay address can be a different function than the same address in another
  overlay, so a pair whose old name is defined in 2+ binaries is skipped and
  reported instead of renamed in all of them.

Exit status is 1 if anything was skipped.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def symbol_files():
    return {p.name[len("symbol_addrs."):-len(".txt")]: p
            for p in (ROOT / "config").glob("symbol_addrs.*.txt")}


def entries(text):
    return re.findall(r"(?m)^(\w+) = (0x[0-9A-Fa-f]+);", text)


def loaded_by(binary):
    """symbol_addrs names a binary's yaml loads, read as text (no yaml dep)."""
    cfg = ROOT / "config" / f"{binary}.yaml"
    if not cfg.is_file():
        return set()
    text = cfg.read_text()
    m = re.search(r"symbol_addrs_path:(.*?)\n  \w", text, re.S)
    if not m:
        return set()
    return {p.rsplit("symbol_addrs.", 1)[1][:-len(".txt")]
            for p in re.findall(r"symbol_addrs\.\S+?\.txt", m.group(1))}


def main():
    argv = sys.argv[1:]
    dry = "-n" in argv
    if "-f" not in argv:
        print(__doc__, file=sys.stderr)
        return 2
    names = Path(argv[argv.index("-f") + 1]).read_text()
    pairs = re.findall(r"(?m)^NAME\s+(\w+)\s+(\w+)\s+-", names)

    src = {p: p.read_text() for p in (ROOT / "src").rglob("*.c")}
    hdr = {p: p.read_text() for p in (ROOT / "include").rglob("*.h")}
    files = symbol_files()
    addrs = {b: entries(p.read_text()) for b, p in files.items()}
    src_bins = {b: "".join(t for p, t in src.items() if p.parts[-3:][0] == b)
                for b in files}
    asm_bins = {b: "".join(p.read_text() for p in (ROOT / "asm" / b).rglob("*.s"))
                for b in files if (ROOT / "asm" / b).is_dir()}

    rename, skipped = [], []
    for old, new in pairs:
        blob = "".join(src.values()) + "".join(hdr.values())
        if re.search(r"INCLUDE_ASM\(\s*\"[^\"]*\"\s*,\s*" + old + r"\b", blob):
            skipped.append((old, new, "still an INCLUDE_ASM stub somewhere"))
        elif not re.search(r"\b" + old + r"\s*\([^;]*?\)\s*\{", blob, re.S):
            skipped.append((old, new, "no C definition of the old name"))
        elif any(re.search(r"\b" + new + r"\b", t) for t in list(src.values()) + list(hdr.values())) \
                and re.search(r"\b" + new + r"\b", blob):
            skipped.append((old, new, "new name already used"))
        else:
            bins = {p.parts[-3] for p in src if re.search(r"\b" + old + r"\b", src[p])}
            defs = [p for p in src if re.search(r"\b" + old + r"\s*\([^;]*?\)\s*\{", src[p], re.S)]
            if len(defs) > 1:
                skipped.append((old, new, f"address twin, defined in {[str(p) for p in defs]}"))
            elif len(bins) != 1:
                skipped.append((old, new, f"no single owner binary: {sorted(bins)}"))
            elif sum(1 for b in files if re.search(r"\b" + new + r"\b", src_bins[b])) > 1:
                skipped.append((old, new, "new name would cover 2+ binaries"))
            else:
                rename.append((old, new, bins.pop()))

    added = {}
    for old, new, owner in rename:
        for b, p in files.items():
            text = p.read_text()
            if re.search(r"(?m)^" + new + r" = ", text):
                continue
            if not re.search(r"\b" + new + r"\b", src_bins.get(b, "") + asm_bins.get(b, "")) \
                    and b != owner:
                continue
            clash = {(n, a) for o in loaded_by(b) if o != b for n, a in addrs[o]}
            if (new, "0x" + old[5:]) in clash or any(a == "0x" + old[5:] for _, a in clash):
                continue
            if any(a == "0x" + old[5:] for _, a in addrs[b]):
                continue
            addrs[b].append((new, "0x" + old[5:]))
            added.setdefault(p, []).append(f"{new} = 0x{old[5:]};")

    for old, new, _ in rename:
        for p in list(src) + list(hdr):
            t = re.sub(r"\b" + old + r"\b", new, src.get(p) or hdr.get(p))
            if p in src:
                src[p] = t
            else:
                hdr[p] = t
    if not dry:
        for p, t in list(src.items()) + list(hdr.items()):
            p.write_text(t)
        for p, lines in added.items():
            p.write_text(p.read_text().rstrip("\n") + "\n" + "\n".join(lines) + "\n")

    print(f"renamed {len(rename)} pairs, added {sum(len(v) for v in added.values())} "
          f"symbol_addrs entries in {len(added)} files{' (dry run)' if dry else ''}")
    for p, lines in added.items():
        print(f"  {p.name}: {len(lines)}")
    for old, new, why in skipped:
        print(f"SKIP {old} -> {new}: {why}")
    return 1 if skipped else 0


if __name__ == "__main__":
    sys.exit(main())
