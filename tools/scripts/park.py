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
import os
import re
import sys

DEF = r"^[A-Za-z_][\w \t*]*?\b%s\s*\([^;]*\)\s*\{\n(?:.*?\n)*?\}[ \t]*(?=\n|$)"
# The same definition with its signature left on the comment's opening line
# ("/* int func_x(int index) {"), which is how a park written by appending a body
# to a note block looks. DEF alone cannot see those, so the census counted them
# as unwritten stubs and rounds 331 and 333 briefed parked bodies (batting/misc2,
# field/misc12, world_map/misc6, battle/misc23).
DEF_STAR = r"^[ \t]+[A-Za-z_][\w \t*]*?\b%s\s*\([^;]*\)\s*\{\n(?:.*?\n)*?\}[ \t]*(?=\n|$)"


def body_in(block, name):
    """The parked body of `name` inside one /* */ block's text, or None."""
    m = re.search(DEF % name, block, re.M)
    if m:
        return m.group(0)
    m = re.search(DEF_STAR % name, block, re.M)
    if m:
        return m.group(0).lstrip(" \t")
    return None
# A park's note and its body are often two blocks glued on one line ("*//*"), so a
# block may also start right after a previous one closes; requiring line-start-only
# made fresh_census call three round-433 parks "never written".
BLOCK = re.compile(r"(?:^|(?<=\*/))[ \t]*/\*((?:[^*]|\*(?!/))*?)\*/[ \t]*(?=\n)", re.M)
FILLER = re.compile(r"^(?:[ \t]*(?://.*)?\n)*$")
# A park's prose note is often its own /* ... */ block between the stub and the
# body block ("PARKED 99.75%" above the /* body */), so a comment block counts as
# filler. Without this, parked_funcs missed those parks entirely and the census
# counted them as unwritten bodies (round 306: seven of nine lanes hit parks).
NOTE_BLOCK = re.compile(r"/\*((?:[^*]|\*(?!/))*?)\*/", re.S)


def nl(s):
    """Guarantee a line break before an inserted body (a park may start right
    after a note block's `*/` on the same line)."""
    return s if not s or s.endswith("\n") else s + "\n"


def filler_only(segment):
    """True when the segment holds only blanks, // lines and /* */ notes."""
    return FILLER.match(NOTE_BLOCK.sub("", segment)) is not None


def unpark(text, name, asm_line):
    inc = text.find(asm_line)
    if inc < 0:
        return None
    end = inc + len(asm_line)
    if text[end:end + 1] == "\n":
        end += 1
    for m in BLOCK.finditer(text):
        if not re.search(r"\b" + name + r"\s*\(", m.group(1)):
            continue
        body = body_in(m.group(1), name)
        if not body:
            continue
        body = body.rstrip(" \t") + "\n"
        if m.end() <= inc and filler_only(text[m.end():inc]):
            return nl(text[:m.start()]) + body + text[end:]
        if m.start() >= end and filler_only(text[end:m.start()]):
            return nl(text[:inc]) + body + text[m.end():]
    return None


def comment(note):
    """A note written as free text must not land in the file as live C."""
    lines = [l if l.lstrip().startswith(("//", "/*")) else "// " + l for l in note.splitlines()]
    return "".join(l + "\n" for l in lines)


def existing_asm_line(text, name):
    """The INCLUDE_ASM line this file already holds for `name`."""
    m = re.search(r'INCLUDE_ASM\("[^"]*",\s*' + name + r"\s*\);", text)
    return m.group(0) if m else None


def asm_line_for(text, name, unit_dir, source=None):
    """The stub line to write, taking the asm directory from the file itself: any
    sibling stub in the same unit names it, so the unit argument (`misc16_r2` is
    not `misc16`) cannot invent a build-breaking path. With no sibling stub to copy,
    the path is derived from the source path (src/<bin>/<rest>.c ->
    asm/<bin>/nonmatchings/<rest>), because `asm/` + the unit slug omits both the
    binary and `nonmatchings/` and breaks the build."""
    line = existing_asm_line(text, name)
    if line:
        return line
    m = re.search(r'INCLUDE_ASM\("([^"]*/[^/"]*)",', text)
    if m:
        return 'INCLUDE_ASM("%s", %s);' % (m.group(1), name)
    rel = (source or "").replace(os.sep, "/")
    if rel.startswith("src/") and rel.endswith(".c"):
        rest = rel[len("src/"):-len(".c")]
        binary, _, tail = rest.partition("/")
        if tail:
            return 'INCLUDE_ASM("asm/%s/nonmatchings/%s", %s);' % (binary, tail, name)
    return 'INCLUDE_ASM("%s", %s);' % ("asm/" + unit_dir, name)


def repark(text, name, asm_line, note=""):
    # A stale INCLUDE_ASM for the function we are parking would leave it both
    # stubbed and defined, which lint_sources.py rejects.
    stale = existing_asm_line(text, name)
    if stale:
        text = text.replace(stale + "\n", "")
    m = re.search(DEF % name, text, re.M)
    if not m:
        return None
    out = text[:m.start()] + comment(note) + "/*\n" + m.group(0) + "*/\n" + asm_line + "\n" + text[m.end():]
    # A second copy of the body that closed on its own brace ("}*/") leaves a
    # dangling "*/" right after the stub; it is invisible to objdiff and to
    # lint_sources.py and only cc1 rejects it (round 523, sound.c).
    out = re.sub(r"(%s[ \t]*\n)\n*[ \t]*\*/[ \t]*\n" % re.escape(asm_line), r"\1\n", out, count=1)
    assert out.count("*/") <= out.count("/*"), "repark left an unmatched */"
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("action", choices=["unpark", "repark"])
    ap.add_argument("source", help="src/<bin>/<unit>.c")
    ap.add_argument("asm_dir", help="unused when the file already names an asm directory; otherwise the unit slug")
    ap.add_argument("function")
    ap.add_argument("--note", default="", help="comment lines to put above the re-parked body")
    a = ap.parse_args()

    text = open(a.source).read()
    asm_line = asm_line_for(text, a.function, a.asm_dir, a.source)
    if a.action == "unpark":
        out = unpark(text, a.function, asm_line)
    else:
        out = repark(text, a.function, asm_line, a.note)
    if out is None:
        sys.exit("%s: no %s state found for %s (asm path %r)" % (a.source, a.action, a.function, a.asm_dir))
    open(a.source, "w").write(out)


if __name__ == "__main__":
    main()
