#!/usr/bin/env python3
"""Move one function between "parked" and "converted" inside a unit.

    tools/scripts/park.py unpark src/battle/main/misc28.c misc28 func_800B63F0
    tools/scripts/park.py repark src/battle/main/misc28.c misc28 func_800B63F0 \\
        --note "best 95.76% under cdk: register swap in the loop guard.\\n"

A *park* is a C body inside a `/* ... */` block next to its `INCLUDE_ASM` line. The
repo stores parks in four shapes and this tool handles all of them:

  - block below the stub                        INCLUDE_ASM(...); /* body */
  - block above the stub                        /* body */ INCLUDE_ASM(...);
  - note lines between the stub and the block   INCLUDE_ASM(...); // note /* body */
  - a block closed on the body's own brace      /* body }*/

The same body is often stored twice (once before and once after the stub); `unpark`
removes only the block it uses, so run it once per duplicate only if the second copy
is still there - `repark` then re-creates a single one.
"""
import argparse
import re
import sys

DEF = r"^[A-Za-z_][\w \t*]*?\b%s\s*\([^;]*\)\s*\{\n(?:.*?\n)*?\}\n"
BLOCK = re.compile(r"^/\*((?:[^*]|\*(?!/))*?)\*/\n", re.M)
FILLER = re.compile(r"^(?:[ \t]*(?://.*)?\n)*$")


def unpark(text, name, asm_line):
    inc = text.find(asm_line)
    if inc < 0:
        return None
    for m in BLOCK.finditer(text):
        if not re.search(r"\b" + name + r"\s*\(", m.group(1)):
            continue
        body = re.search(DEF % name, m.group(1), re.M)
        if not body:
            continue
        if m.end() <= inc and FILLER.match(text[m.end():inc]):
            return text[:m.start()] + body.group(0) + text[inc + len(asm_line) + 1:]
        if m.start() >= inc + len(asm_line) and FILLER.match(text[inc + len(asm_line):m.start()]):
            return text[:inc] + body.group(0) + text[m.end():]
    return None


def repark(text, name, asm_line, note=""):
    m = re.search(DEF % name, text, re.M)
    if not m:
        return None
    return text[:m.start()] + note + "/*\n" + m.group(0) + "*/\n" + asm_line + "\n" + text[m.end():]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("action", choices=["unpark", "repark"])
    ap.add_argument("source", help="src/<bin>/<unit>.c")
    ap.add_argument("asm_dir", help="the unit directory under asm/<bin>/nonmatchings/main/")
    ap.add_argument("function")
    ap.add_argument("--note", default="", help="comment lines to put above the re-parked body")
    a = ap.parse_args()

    asm_line = 'INCLUDE_ASM("asm/%s", %s);' % (a.asm_dir, a.function)
    text = open(a.source).read()
    if a.action == "unpark":
        out = unpark(text, a.function, asm_line)
    else:
        out = repark(text, a.function, asm_line, a.note)
    if out is None:
        sys.exit("%s: no %s state found for %s (asm path %r)" % (a.source, a.action, a.function, a.asm_dir))
    open(a.source, "w").write(out)


if __name__ == "__main__":
    main()
