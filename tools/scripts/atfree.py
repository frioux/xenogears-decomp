"""Report how many of a unit's stubs contain an unmatchable symbol access.

A one-instruction load or store off an absolute symbol expands differently in our
assembler than in the ASPSX that built some units: we get `lui $at,%hi` + `...%lo($at)`
for stores and `lui $d,%hi` + `lw $d,%lo($d)` for loads, while those units picked an
arbitrary free register. No gcc version, -O level or -G setting changes that, so a stub
containing such an access can never reach objdiff 100%. See TRACKER round 10.

    python3 tools/scripts/atfree.py battle/main/misc30

Exit status is 0 when every stub is reachable, 1 when at least one is not.
"""
import glob
import re
import sys

PAT = re.compile(r"(lw|lbu|lh|lhu|lb|sw|sb|sh) +\$(\w+), *%lo\([^)]*\)\$?\(?(\$\w+)\)?")
LOADS = {"lw", "lbu", "lh", "lhu", "lb"}


def unmatchable(path):
    for line in open(path, errors="ignore"):
        m = PAT.search(line)
        if not m:
            continue
        op, dst, base = m.group(1), m.group(2), m.group(3).lstrip("$")
        if base == "at":
            continue
        if op in LOADS and base == dst:
            continue  # gcc's own `lw $d,sym` expansion: destination doubles as base
        return m.group(0)
    return None


def main(argv):
    unit = argv[1]
    files = sorted(glob.glob(argv[2] + "/*.s"))
    bad = [(f, unmatchable(f)) for f in files]
    bad = [(f, why) for f, why in bad if why]
    for f, why in bad:
        print(f"{unit} {f.rsplit('/', 1)[1][:-2]}: {why}")
    print(f"{unit}: {len(bad)} of {len(files)} stubs unmatchable")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
