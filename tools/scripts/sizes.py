#!/usr/bin/env python3
"""Flag every C function whose compiled size differs from its span in symbol_addrs.

    make objdiff-config            # report mode objects in build/src (a clean build also works)
    tools/scripts/sizes.py

The gate that catches the worst failure mode in this repo: a converted function - or a
conversion that lets gcc shorten an already-matched *caller* - changes a function's size,
so every later symbol shifts and the overlay stops being byte-identical while objdiff
still reports 100.00% for every function involved. This names the function instead of
making you bisect a 334-symbol `culprit.sh` diff.

A retail span includes any padding before the next symbol, so a *smaller* size is the
suspicious direction; a larger one usually means a boundary the symbol table does not
describe (data after the last function of a unit) - check those by hand.
"""
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def retail_sizes():
    sizes = {}
    for path in glob.glob(os.path.join(ROOT, "config/symbol_addrs.*.txt")):
        binary = path.split(".")[-2]
        addrs = {}
        for line in open(path):
            m = re.match(r"(\w+)\s*=\s*0x([0-9A-Fa-f]+);", line)
            if m:
                addrs[m.group(1)] = int(m.group(2), 16)
        ordered = sorted((a, n) for n, a in addrs.items() if n.startswith("func_"))
        sizes[binary] = {ordered[i][1]: ordered[i + 1][0] - ordered[i][0]
                         for i in range(len(ordered) - 1)}
    return sizes


def main():
    sizes = retail_sizes()
    flagged = 0
    for binary, table in sizes.items():
        pattern = os.path.join(ROOT, "build/src", binary, "**", "*.c.o")
        for obj in glob.glob(pattern, recursive=True):
            out = subprocess.run(["nm", "-S", obj], capture_output=True, text=True).stdout
            for line in out.splitlines():
                parts = line.split()
                if len(parts) != 4 or parts[2] != "T" or parts[3] not in table:
                    continue
                ours = int(parts[1], 16)
                if ours != table[parts[3]]:
                    unit = os.path.relpath(obj, os.path.join(ROOT, "build/src"))[:-4]
                    print("%s %s retail %d ours %d (%+d)" % (unit, parts[3], table[parts[3]], ours,
                                                             ours - table[parts[3]]))
                    flagged += 1
    print("flagged: %d" % flagged)
    return 1 if flagged else 0


if __name__ == "__main__":
    sys.exit(main())
