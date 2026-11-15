#!/usr/bin/env python3
"""Catch the source mistakes that only show up as a confusing link error.

    tools/scripts/lint_sources.py            # scan src/
    tools/scripts/lint_sources.py -q         # only failures

Three checks, all of which have cost real time in this repo:

1. A function that is both `INCLUDE_ASM`'d and defined in C. maspsx then fails
   with "symbol `X' is already defined", but `build.ninja` sends maspsx's stderr
   to /dev/null, so the only visible symptom is `ld: cannot find
   build/src/<unit>.c.o`.
2. A block comment that closes early, usually a parked attempt containing `*/`
   (`Normal*/Color`) or two attempt blocks nested, leaving code (often a stray
   `}`) outside the comment.
3. A stub swallowed by a comment that does close, just too late: a parked body
   above it missing its `*/` eats the next `INCLUDE_ASM` line, and the only
   symptom is `ld: undefined reference to func_X` from an asm data table.

Exit status is 1 if anything is found, so it can gate a commit.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEF_RE = re.compile(
    r"^\s*(?:void|int|u_int|s_int|s32|u8|s8|u8|u_char|s_char|s16|s_short|u16|"
    r"u_short|short|long|u_long|float|double|char|u32|undefined8|undefined16|"
    r"undefined32|mem_addr)\s*\**\s*(\w+)\s*\([^;{]*\)\s*\{",
    re.M,
)
STUB_RE = re.compile(r'INCLUDE_ASM\(\s*"[^"]+"\s*,\s*(\w+)\s*\)')


def strip_comments(text):
    """Return text with comments blanked, plus a flag if a comment never closes."""
    out = []
    i, n = 0, len(text)
    unterminated = False
    while i < n:
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            if j < 0:
                unterminated = True
                out.append(" " * (n - i))
                break
            out.append(" " * (j + 2 - i))
            i = j + 2
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        else:
            out.append(text[i])
            i += 1
    return "".join(out), unterminated


def comment_blocks(text, min_size=0):
    """Yield (start_line, end_line, body) for each /* */ block at least min_size bytes."""
    i, n, line = 0, len(text), 1
    while i < n:
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            if j < 0:
                return
            start = line
            line += text[i:j + 2].count("\n")
            if j + 2 - i >= min_size:
                yield start, line, text[i:j + 2]
            i = j + 2
            continue
        if text[i] == "\n":
            line += 1
        i += 1


def main():
    quiet = "-q" in sys.argv
    problems = 0
    for src in sorted((ROOT / "src").rglob("*.c")):
        text = src.read_text(errors="replace")
        code, unterminated = strip_comments(text)
        rel = src.relative_to(ROOT)
        if unterminated:
            problems += 1
            print(f"{rel}: a /* block comment never closes")
        stubs = set(STUB_RE.findall(code))
        for name in sorted(set(STUB_RE.findall(text)) - stubs):
            problems += 1
            print(f"{rel}: {name}'s INCLUDE_ASM is inside a comment "
                  f"(a parked body above it never closed)")
        for name in sorted(stubs & set(DEF_RE.findall(code))):
            problems += 1
            print(f"{rel}: {name} is both INCLUDE_ASM'd and defined in C "
                  f"(maspsx: 'symbol already defined'; the linker reports a missing .o)")
        # A park is one function's attempt. A big /* */ block holding two or more
        # definitions has swallowed the function that follows it: its callers then
        # link with "undefined reference", and every other check above still passes
        # because strip_comments() blanks the swallowed definition (round 75:
        # menu/main/misc10's func_801E36D4 park ate MenuComputeCharacterStats).
        for start, end, body in comment_blocks(text, min_size=200):
            defs = DEF_RE.findall(body)
            if len(set(defs)) > 1:   # the same body stored twice is a duplicate, not a swallow
                problems += 1
                print(f"{rel}: the /* */ block at line {start} holds {len(defs)} "
                      f"function definitions ({', '.join(defs)}) - a park that "
                      f"swallowed the function after it; close it before "
                      f"line {start}")
        # a `*/` sitting where no comment was open shows up as an unbalanced count
        if not quiet and code.count("/*") != code.count("*/"):
            pass
    if problems:
        print(f"-- {problems} problem(s)")
        return 1
    if not quiet:
        print("src/: clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
