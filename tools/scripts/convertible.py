#!/usr/bin/env python3
"""List the stubs of a unit that are SAFE to convert to C.

    tools/scripts/convertible.py slus_006.64/psyq/libgte
    tools/scripts/convertible.py field/main/misc8 --all

A stub's `.s` sometimes holds more than the function: instructions or data after
`.size`, or a `dlabel` block before it. In matching mode `INCLUDE_ASM` pulls all
of it into the object; a C definition reproduces only the function body, so those
extra bytes vanish, the unit's `.text`/`.rodata` shrinks, and every later symbol
shifts -- `make check` then fails while `objdiff` still says 100%. This is why
"convert everything in the unit" does not work: in psyq/libgte most stubs carry
1-3 padding nops that `as` inserted, and about 8 of 10 candidates are unlinkable.

Prints one line per stub: OK / BLOCKED, with the reason and the instruction count.
Exit status is 0 always; it is a listing, not a gate.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INSN = re.compile(r"^\s+/\*\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+[0-9A-Fa-f]{8}\s*\*/")
STUB = re.compile(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)')


def main():
    binary, unit = sys.argv[1].split("/", 1)
    src = ROOT / "src" / binary / (unit + ".c")
    text = src.read_text()
    asm_dir = STUB.search(text).group(1)
    rows = []
    for _, name in STUB.findall(text):
        path = ROOT / asm_dir / (name + ".s")
        if not path.is_file():
            rows.append(("?", name, 0, "no .s"))
            continue
        body = path.read_text(errors="replace").splitlines()
        n_insn = sum(1 for ln in body if INSN.match(ln))
        size_at = next((i for i, ln in enumerate(body) if ln.startswith(".size")), None)
        if size_at is None:
            rows.append(("OK", name, n_insn, "no .size"))
            continue
        after = [ln for ln in body[size_at + 1:] if INSN.match(ln)]
        labels = [ln for ln in body if ln.startswith("dlabel")]
        problems = []
        if after:
            problems.append(f"{len(after)} insn after .size")
        if labels:
            problems.append("owns " + ", ".join(l.split()[1] for l in labels))
        rows.append(("BLOCKED" if problems else "OK", name, n_insn, "; ".join(problems)))
    ok = [r for r in rows if r[0] == "OK"]
    print(f"{unit}: {len(ok)} convertible of {len(rows)} stubs")
    for state, name, n, why in sorted(rows, key=lambda r: (r[0] != "OK", r[2])):
        if state == "OK" and "--all" not in sys.argv:
            print(f"  {state:8} {name:32} {n:4} insns")
        elif state != "OK":
            print(f"  {state:8} {name:32} {n:4} insns  {why}")


if __name__ == "__main__":
    main()
