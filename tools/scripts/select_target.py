#!/usr/bin/env python3
"""Pick the next function to decompile: lists INCLUDE_ASM stubs with stats."""
import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BINARIES = ["slus_006.64", "field", "battling", "world_map", "member_change_menu",
            "shop_menu", "gear_shop_menu", "lib_gear"]
STUB_RE = re.compile(r'^\s*INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)')
INSN_RE = re.compile(r'^\s+/\*\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+[0-9A-Fa-f]{8}\s*\*/\s*(\S+)')
HI_RE = re.compile(r'%hi\(([A-Za-z_]\w*)')
JAL_RE = re.compile(r'\bjal\s+([A-Za-z_]\w*)')


def has_attempt(lines, i):
    """A commented-out C attempt parked directly under the stub at lines[i].

    It is a comment block whose text contains a function body. Scanning a fixed
    window instead made 112 of 569 stubs look like they had an attempt, because
    the next real function's brace falls inside the window.
    """
    body = []
    for ln in lines[i + 1:]:
        s = ln.strip()
        if not s:
            continue
        if s.startswith("INCLUDE_ASM"):
            return False
        if s.startswith(("/*", "*", "//", "#if 0")):
            body.append(s)
            continue
        break
    text = "\n".join(body)
    return "#if 0" in text or "{" in text


def analyze_asm(path):
    """Return (insns, jals, unnamed_data, named_refs) or None if .s absent."""
    if not path.is_file():
        return None
    insns = jals = unnamed = named = 0
    for ln in path.read_text(errors="replace").splitlines():
        if not INSN_RE.match(ln):
            continue
        insns += 1
        for m in JAL_RE.finditer(ln):
            jals += 1
            if not m.group(1).startswith("func_"):
                named += 1
        for m in HI_RE.finditer(ln):  # %hi only, so hi/lo pairs count once
            sym = m.group(1)
            if sym.startswith("D_"):
                unnamed += 1
            elif not sym.startswith("func_"):
                named += 1
    return insns, jals, unnamed, named


def collect():
    rows = []
    for binary in BINARIES:
        for src in sorted((ROOT / "src" / binary).rglob("*.c")):
            lines = src.read_text(errors="replace").splitlines()
            for i, ln in enumerate(lines):
                m = STUB_RE.match(ln)
                if not m:
                    continue
                asm_dir, name = m.groups()
                stats = analyze_asm(ROOT / asm_dir / (name + ".s"))
                insns, jals, unnamed, named = stats if stats else (None, None, None, None)
                rows.append({
                    "binary": binary,
                    "file": str(src.relative_to(ROOT)),
                    "function": name,
                    "insns": insns,
                    "jals": jals,
                    "unnamed_data": unnamed,
                    "named_refs": named,
                    "is_named": not name.startswith("func_"),
                    "attempt": has_attempt(lines, i),
                })
    return rows


def score(r):
    """Lower is better: small, named, few unnamed data refs."""
    size = r["insns"] if r["insns"] is not None else 500
    s = size + 8 * (r["unnamed_data"] or 0) + 2 * (r["jals"] or 0)
    if not r["is_named"]:
        s += 40
    if r["insns"] is None:
        s += 1000
    return s


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--binary", choices=BINARIES)
    ap.add_argument("--file", help="substring of source path")
    ap.add_argument("--max-insns", type=int)
    ap.add_argument("--min-insns", type=int)
    ap.add_argument("--named-only", action="store_true",
                    help="only functions with real (non func_) names")
    ap.add_argument("--limit", type=int, default=30)
    ap.add_argument("--sort", choices=["size", "named", "file"], default=None)
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args()

    rows = collect()
    totals = Counter(r["binary"] for r in rows)
    if a.binary:
        rows = [r for r in rows if r["binary"] == a.binary]
    if a.file:
        rows = [r for r in rows if a.file in r["file"]]
    if a.named_only:
        rows = [r for r in rows if r["is_named"]]
    if a.max_insns is not None:
        rows = [r for r in rows if r["insns"] is not None and r["insns"] <= a.max_insns]
    if a.min_insns is not None:
        rows = [r for r in rows if r["insns"] is not None and r["insns"] >= a.min_insns]

    if a.sort == "size":
        rows.sort(key=lambda r: (r["insns"] is None, r["insns"] or 0))
    elif a.sort == "named":
        rows.sort(key=lambda r: (r["unnamed_data"] or 0, -(r["named_refs"] or 0), score(r)))
    elif a.sort == "file":
        rows.sort(key=lambda r: (r["file"], r["function"]))
    else:
        rows.sort(key=score)
    matched = len(rows)
    rows = rows[:a.limit]

    if a.json:
        print(json.dumps(rows, indent=2))
        return 0

    hdr = f'{"insns":>5} {"jal":>3} {"D_":>3} {"named":>5} {"C?":>2}  {"binary":<18} {"function":<36} file'
    print(hdr)
    for r in rows:
        f = lambda v: "?" if v is None else str(v)
        print(f'{f(r["insns"]):>5} {f(r["jals"]):>3} {f(r["unnamed_data"]):>3} '
              f'{f(r["named_refs"]):>5} {"y" if r["attempt"] else "-":>2}  '
              f'{r["binary"]:<18} {r["function"]:<36} {r["file"]}')
    tot = ", ".join(f"{b}={totals[b]}" for b in BINARIES if totals[b])
    print(f"-- showing {len(rows)} of {matched} matching; remaining stubs: "
          f"{sum(totals.values())} total ({tot})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
