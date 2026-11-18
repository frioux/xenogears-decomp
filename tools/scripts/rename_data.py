#!/usr/bin/env python3
"""Give data symbols real names: D_<addr> -> g_Name, everywhere at once.

    tools/scripts/rename_data.py -f names.txt      # "DATA D_800D2CB0 g_Name - evidence" lines
    tools/scripts/rename_data.py -f names.txt -n   # dry run

The data counterpart of rename_sym.py + name_addrs.py. For each pair it whole-word
replaces the old name in src/ and include/ and makes sure the owning binary's
symbol_addrs file has `g_Name = 0xADDR;`, because the asm of every stub that still
refers to the address is generated from symbol_addrs: a rename in src/ alone links
as `undefined reference to g_Name`.

The owner is decided by the address. Below the lowest overlay vram it is a main-exe
symbol: one entry in symbol_addrs.slus_006.64.txt, which every overlay loads. Above
it, overlays share the address range, so the same D_<addr> in two binaries is two
different things: it is renamed only when exactly one binary's src/ uses it.

A pair is skipped, and the rest still applied, when:
  - the old name is not D_<8 hex digits> or the new one is not g_<Name>,
  - no C under src/ uses the old name (nothing to justify a name from),
  - the new name is already used anywhere,
  - an overlay address is used from 2+ binaries (address twin),
  - the address already has a different real name in a symbol_addrs file the owner loads.
Exit status is 1 if anything was skipped. Run `make check` afterwards.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAIN = "slus_006.64"


def loaded_by(binary):
    """Binaries whose symbol_addrs file this binary's yaml loads, itself included."""
    text = (ROOT / "config" / f"{binary}.yaml").read_text()
    m = re.search(r"symbol_addrs_path:(.*?)\n  \w", text, re.S)
    return set(re.findall(r"symbol_addrs\.(\S+?)\.txt", m.group(1))) if m else {binary}


def overlay_base():
    """Lowest vram any overlay is linked at; everything below belongs to the main exe."""
    vrams = [int(v, 16)
             for p in (ROOT / "config").glob("*.yaml") if p.stem != MAIN
             for v in re.findall(r"(?m)^\s+vram:\s*(0x[0-9A-Fa-f]+)", p.read_text())]
    return min(vrams)


def main():
    argv = sys.argv[1:]
    dry = "-n" in argv
    if "-f" not in argv:
        sys.exit(__doc__)
    pairs = re.findall(r"(?m)^DATA\s+(\w+)\s+(\w+)\s+[-\u2014]",
                       Path(argv[argv.index("-f") + 1]).read_text())
    if not pairs:
        sys.exit("no 'DATA D_<addr> g_Name - evidence' lines in the file")

    code = {p: p.read_text() for d in ("src", "include") for p in (ROOT / d).rglob("*.[ch]")}
    syms = {p.name[len("symbol_addrs."):-len(".txt")]: p
            for p in (ROOT / "config").glob("symbol_addrs.*.txt")}
    sym_text = {b: p.read_text() for b, p in syms.items()}
    words = set(re.findall(r"\w+", "".join(code.values()) + "".join(sym_text.values())))
    base = overlay_base()

    good, skipped = {}, []          # old -> (new, owner)
    for old, new in pairs:
        users = {p.relative_to(ROOT).parts[1] for p, t in code.items()
                 if p.suffix == ".c" and re.search(r"\b%s\b" % old, t)}
        why = None
        if not re.fullmatch(r"D_[0-9A-F]{8}", old):
            why = "old name is not D_<addr>"
        elif not re.fullmatch(r"g_[A-Z]\w*", new):
            why = "'%s' is not a g_Name" % new
        elif not users:
            why = "no C uses it"
        elif new in words or new in (n for n, _ in good.values()):
            why = "'%s' is already used" % new
        elif old in good:
            why = "listed twice"
        elif int(old[2:], 16) >= base and len(users) > 1:
            why = "address twin, used from %s" % sorted(users)
        else:
            owner = MAIN if int(old[2:], 16) < base else users.pop()
            taken = [n for b in loaded_by(owner)
                     for n, a in re.findall(r"(?m)^(\w+) = (0x[0-9A-Fa-f]+);", sym_text[b])
                     if int(a, 16) == int(old[2:], 16) and n != old]
            if taken:
                why = "address is already named %s" % taken[0]
            else:
                good[old] = (new, owner)
        if why:
            skipped.append("SKIP %s -> %s: %s" % (old, new, why))

    added = 0
    if good:
        pattern = re.compile(r"\b(%s)\b" % "|".join(good))
        swap = lambda m: good[m.group(1)][0]
        for p, t in code.items():
            if pattern.search(t) and not dry:
                p.write_text(pattern.sub(swap, t))
        for old, (new, owner) in good.items():
            if re.search(r"(?m)^%s = " % old, sym_text[owner]):
                sym_text[owner] = re.sub(r"(?m)^%s\b" % old, new, sym_text[owner])
            else:
                sym_text[owner] = "%s\n%s = 0x%s;\n" % (sym_text[owner].rstrip("\n"), new, old[2:])
                added += 1
        if not dry:
            for b in {owner for _, owner in good.values()}:
                syms[b].write_text(sym_text[b])
    print("%s %d names, added %d symbol_addrs entries"
          % ("would rename" if dry else "renamed", len(good), added))
    print("\n".join(skipped), end="\n" if skipped else "")
    sys.exit(1 if skipped else 0)


if __name__ == "__main__":
    main()
