#!/usr/bin/env python3
"""Live raw C lines: pointer arithmetic on a global, cast-and-offset field access.

    tools/scripts/raw_lines.py                  # total, then the worst files
    tools/scripts/raw_lines.py <unit or file>   # per-function counts in one file

Comments are stripped first, so a parked body (`/* ... */`) does not count and
never gets attributed to a function. PROMPT.md's plain grep counts parks too,
which is how round 151 briefed four workers on `INCLUDE_ASM` stubs: a park's raw
lines were read as that function's, and the "matched functions" list was parks.
"""
import glob
import re
import sys

RAW = re.compile(
    r"D_[0-9A-F]{8} \+ \w+ \* |\*\((const )?[us](8|16|32) ?\*+\)\(|\(\*\w+\)\["
    r"|\(\(?[us](8|16|32) ?\*\)\w+\)? \+ "
)


def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), s, flags=re.S)
    return re.sub(r"//[^\n]*", "", s)


def functions(path):
    """Yield (name, [raw lines]) per C definition, in file order."""
    cur, lines = None, []
    for line in strip_comments(open(path, errors="replace").read()).splitlines():
        if line and line[0] not in " \t/}#":
            m = re.match(r"^\w[\w \*]*?\b(\w+)\s*\(", line)
            if m and not line.rstrip().endswith(";"):
                if cur:
                    yield cur, lines
                cur, lines = m.group(1), []
        if RAW.search(line) and cur:
            lines.append(line.strip())
    if cur:
        yield cur, lines


def main(argv):
    if argv:
        f = argv[0] if argv[0].startswith("src/") else "src/%s.c" % argv[0]
        for name, lines in functions(f):
            print("%3d %s" % (len(lines), name))
            if "-v" in argv:
                for line in lines:
                    print("      " + line[:110])
        return
    totals = []
    for f in glob.glob("src/**/*.c", recursive=True):
        n = sum(len(lines) for _, lines in functions(f))
        if n:
            totals.append((n, f))
    totals.sort(reverse=True)
    print("total live raw %d in %d files" % (sum(n for n, _ in totals), len(totals)))
    for n, f in totals[:20]:
        print("%3d %s" % (n, f))


if __name__ == "__main__":
    main(sys.argv[1:])
