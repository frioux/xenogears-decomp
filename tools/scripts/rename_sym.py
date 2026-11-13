#!/usr/bin/env python3
"""Give matched functions real names, everywhere at once.

    tools/scripts/rename_sym.py func_80012345=BattleInitActors ...
    tools/scripts/rename_sym.py -f names.txt     # one "[NAME] <old> <new> [- evidence]" per line
    tools/scripts/rename_sym.py -n ...           # dry run

Whole-word replaces <old> in config/symbol_addrs.*.txt, src/ and include/. The asm
under asm/ and expected/ is regenerated from symbol_addrs, so run
`make objdiff-config` (or `make check`) afterwards.

A pair is skipped, and the rest still applied, when:
  - <old> has no entry in any symbol_addrs file,
  - <old> is still an INCLUDE_ASM stub or has no C definition (only functions that
    are 100% C get names),
  - <new> is not a fresh identifier (already used anywhere, or func_/D_ shaped).
Exit status is 1 if anything was skipped.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
IDENT = re.compile(r"[A-Za-z_]\w*$")


def parse(argv):
    dry = "-n" in argv
    argv = [a for a in argv if a != "-n"]
    pairs = []
    if argv[:1] == ["-f"]:
        for line in Path(argv[1]).read_text().splitlines():
            words = line.split()
            if words[:1] == ["NAME"]:
                words = words[1:]
            if len(words) >= 2:
                pairs.append((words[0], words[1]))
    else:
        pairs = [tuple(a.split("=", 1)) for a in argv if "=" in a]
    if not pairs:
        sys.exit(__doc__)
    return dry, pairs


def main():
    dry, pairs = parse(sys.argv[1:])
    files = [
        p
        for pattern in ("config/symbol_addrs.*.txt", "src/**/*.[ch]", "include/**/*.h")
        for p in ROOT.glob(pattern)
    ]
    text = {p: p.read_text() for p in files}
    src = "\n".join(t for p, t in text.items() if p.suffix == ".c")
    words = set(re.findall(r"\w+", "\n".join(text.values())))
    symbols = set(
        re.findall(
            r"^(\w+)\s*=",
            "\n".join(t for p, t in text.items() if p.suffix == ".txt"),
            re.M,
        )
    )

    good, skipped = {}, 0
    for old, new in pairs:
        why = None
        if old not in symbols:
            why = "not in any symbol_addrs file"
        elif re.search(r"INCLUDE_ASM\([^)]*,\s*%s\s*\)" % re.escape(old), src):
            why = "still an INCLUDE_ASM stub"
        elif not re.search(r"\b%s\s*\([^;{}]*\)\s*\{" % re.escape(old), src):
            why = "no C definition"
        elif not IDENT.match(new) or re.match(r"(func|D)_", new):
            why = "'%s' is not a real name" % new
        elif new in words or new in good.values():
            why = "'%s' is already used" % new
        elif old in good:
            why = "listed twice"
        if why:
            print("SKIP %s -> %s: %s" % (old, new, why))
            skipped += 1
        else:
            good[old] = new

    if good:
        pattern = re.compile(r"\b(%s)\b" % "|".join(map(re.escape, good)))
        touched = 0
        for p, t in text.items():
            renamed = pattern.sub(lambda m: good[m.group(1)], t)
            if renamed != t:
                touched += 1
                if not dry:
                    p.write_text(renamed)
        print(
            "%s %d names in %d files"
            % ("would rename" if dry else "renamed", len(good), touched)
        )
    sys.exit(1 if skipped else 0)


if __name__ == "__main__":
    main()
