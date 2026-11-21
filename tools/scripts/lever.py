#!/usr/bin/env python3
"""Look up measured C idioms and blockers in tools/rounds/levers.md.

  lever.py <keyword>...   levers matching ALL keywords (heading + keywords + text,
                          case-insensitive); if none match, levers ranked by how many
                          keywords they contain
  lever.py --index        headings with lever counts and ids
  lever.py --symptoms     symptom -> keywords table
"""
import os
import re
import sys

PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "rounds", "levers.md")


def load():
    sections = []  # {title, symptoms, try, levers: [{id, keywords, text, heading}]}
    cur = lever = None
    for line in open(PATH, encoding="utf-8").read().split("\n"):
        if line.startswith("## "):
            cur = {"title": line[3:].strip(), "symptoms": "", "try": "", "levers": []}
            sections.append(cur)
            lever = None
        elif cur is None:
            continue
        elif line.startswith("symptoms:"):
            cur["symptoms"] = line[9:].strip()
        elif line.startswith("try:"):
            cur["try"] = line[4:].strip()
        elif line.startswith("- id:"):
            m = re.match(r"- id:\s*(\S+)\s*\|\s*keywords:\s*(.*)$", line)
            if not m:
                continue
            lever = {"id": m.group(1), "keywords": m.group(2).strip(), "lines": [], "heading": cur["title"]}
            cur["levers"].append(lever)
        elif lever is not None:
            lever["lines"].append(line)
    for s in sections:
        for lv in s["levers"]:
            lv["text"] = "\n".join(lv["lines"]).rstrip()
    return sections


def show(lv):
    print("[%s] (%s) keywords: %s" % (lv["id"], lv["heading"], lv["keywords"]))
    print(lv["text"])
    print()


def main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__)
        return 0 if argv else 2
    sections = load()
    if argv[0] == "--index":
        for s in sections:
            print("%s (%d)" % (s["title"], len(s["levers"])))
            for lv in s["levers"]:
                print("  " + lv["id"])
        return 0
    if argv[0] == "--symptoms":
        for s in sections:
            print("%s -> %s" % (s["symptoms"] or s["title"], s["try"]))
        return 0
    words = [w.lower() for w in argv]
    scored = []
    for s in sections:
        for lv in s["levers"]:
            hay = (s["title"] + "\n" + lv["keywords"] + "\n" + lv["text"]).lower()
            hits = sum(1 for w in words if w in hay)
            scored.append((hits, lv))
    exact = [lv for h, lv in scored if h == len(words)]
    if exact:
        found, note = exact, ""
    else:
        found = [lv for h, lv in sorted(scored, key=lambda x: -x[0]) if h > 0]
        note = "no lever has all of: %s; showing partial matches ranked by hits\n\n" % " ".join(words)
    if not found:
        print("no lever matches: %s (try --symptoms or --index)" % " ".join(words))
        return 1
    sys.stdout.write(note)
    for lv in found:
        show(lv)
    print("%d lever(s)" % len(found))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
