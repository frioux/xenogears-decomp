#!/usr/bin/env python3
"""C functions that are still named func_<addr>, counted properly.

    tools/scripts/unnamed_c.py              # total + the files with the most
    tools/scripts/unnamed_c.py -l           # one line per function
    tools/scripts/unnamed_c.py <unit>       # just that file

A function is a definition only when a signature at brace depth 0 is followed by
`{` before any `;`, so prototypes, call sites and parked bodies (comments are
stripped first) do not count. A plain `grep func_` counts all three: the count it
gave in round 149 (277 -> 252 "unnamed definitions") was really prototypes and
calls, and the round-153 naming workers reported that several of the files they
were given define no `func_` function at all.
"""
import glob
import re
import sys

SIG = re.compile(r"^[A-Za-z_][\w \t\*]*?\b(\w+)\s*\(")


def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), s, flags=re.S)
    return re.sub(r"//[^\n]*", "", s)


def definitions(path):
    depth, pending = 0, None
    for line in strip_comments(open(path, errors="replace").read()).splitlines():
        t = line.strip()
        if depth == 0 and pending is None and t and not t.startswith(("extern", "typedef", "#")):
            m = SIG.match(t)
            if m and not t.endswith(";"):
                pending = m.group(1)
        if pending is not None and depth == 0:
            if "{" in t:
                yield pending
                pending = None
            elif ";" in t:
                pending = None
        depth += t.count("{") - t.count("}")
        if depth < 0:
            depth = 0


def main(argv):
    every = "-l" in argv
    files = argv[[i for i, a in enumerate(argv) if a != "-l"][0]:][:1] if [a for a in argv if a != "-l"] else []
    if files:
        paths = [files[0] if files[0].startswith("src/") else "src/%s.c" % files[0]]
    else:
        paths = glob.glob("src/**/*.c", recursive=True)
    rows = []
    for f in paths:
        names = [n for n in definitions(f) if re.fullmatch(r"func_[0-9A-F]{8}", n)]
        if names:
            rows.append((len(names), f, names))
            if every:
                for n in names:
                    print(n, f)
    rows.sort(reverse=True)
    print("total %d unnamed C definitions in %d files" % (sum(r[0] for r in rows), len(rows)))
    for n, f, _ in rows[:20]:
        print("%3d %s" % (n, f))


if __name__ == "__main__":
    main(sys.argv[1:])
