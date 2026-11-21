#!/usr/bin/env python3
"""Find the matched functions (twins) that an unmatched stub is a copy of.

    tools/scripts/twin_for.py func_8009F1C4              # best twins + stub siblings
    tools/scripts/twin_for.py func_800822C4 --bin battle # overlays reuse addresses
    tools/scripts/twin_for.py --queue [--min 0.8]        # every stub with a twin, TSV
    tools/scripts/twin_for.py --rebuild                  # drop the cache first

A stub is every function under asm/<bin>/nonmatchings, a twin is a function under
asm/<bin>/matchings. Each instruction is tokenised as mnemonic + register list
(strict) or mnemonic only (loose); similarity is difflib's ratio over the token
lists, candidates come from 4-gram shingle overlap (not from length). Verdicts:
PORT >= 0.98 (copy the twin, fix callees), TEMPLATE 0.80-0.98, LOOSE 0.60-0.80.

Needs the tree in report mode (`make objdiff-config`): that is what writes
asm/*/matchings. The token index and the park table are cached in
build/twin_index.pkl and rebuilt when an asm or src directory changes.
"""
import argparse
import collections
import difflib
import importlib.util
import os
import pickle
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CACHE = os.path.join(ROOT, "build", "twin_index.pkl")
LINE = re.compile(r"/\*\s*\w+\s+\w+\s+(\w{8})\s*\*/\s+(\w[\w.]*)\s*(.*)")
K = 4
MIN_TWIN = 0.60
MIN_INSNS = 6


def _stamp():
    """(path, mtime_ns) of every asm directory that holds .s files."""
    out = []
    asm = os.path.join(ROOT, "asm")
    for b in sorted(os.listdir(asm)):
        for kind in ("matchings", "nonmatchings"):
            for d, _, _ in os.walk(os.path.join(asm, b, kind)):
                out.append((d, os.stat(d).st_mtime_ns))
    return out


def _src_stamp():
    out = []
    for d, _, fs in os.walk(os.path.join(ROOT, "src")):
        for f in fs:
            if f.endswith(".c"):
                p = os.path.join(d, f)
                out.append((p, os.stat(p).st_mtime_ns))
    return out


def has_matchings():
    asm = os.path.join(ROOT, "asm")
    return any(os.path.isdir(os.path.join(asm, b, "matchings")) for b in os.listdir(asm))


def _parse_asm(path):
    """{func: (strict tokens, mnemonics, raw words)} for one .s file."""
    funcs = {}
    cur = None
    for ln in open(path, errors="replace"):
        if ln.startswith("glabel"):
            cur = ln.split()[1]
            funcs[cur] = ([], [], [])
            continue
        m = LINE.match(ln.strip())
        if m and cur:
            mn = m.group(2)
            regs = re.findall(r"\$\w+", m.group(3).split("#")[0])
            funcs[cur][0].append(mn + " " + ",".join(regs))
            funcs[cur][1].append(mn)
            funcs[cur][2].append(m.group(1))
    return funcs


def _build_items():
    items = []
    asm = os.path.join(ROOT, "asm")
    for b in sorted(os.listdir(asm)):
        for kind in ("matchings", "nonmatchings"):
            top = os.path.join(asm, b, kind)
            for d, _, fs in os.walk(top):
                rel = os.path.relpath(d, top)
                unit = b if rel == "." else b + "/" + rel
                for f in sorted(fs):
                    if not f.endswith(".s"):
                        continue
                    for name, (s, l, w) in _parse_asm(os.path.join(d, f)).items():
                        if s:
                            items.append(dict(name=name, bin=b, unit=unit, stub=kind == "nonmatchings",
                                              s=s, l=l, w=w))
    return items


def _shingles(t):
    return {tuple(t[i:i + K]) for i in range(max(1, len(t) - K + 1))}


def _save(c):
    os.makedirs(os.path.dirname(CACHE), exist_ok=True)
    tmp = CACHE + ".%d.tmp" % os.getpid()
    with open(tmp, "wb") as f:
        pickle.dump({k: v for k, v in c.items() if k not in ("inv", "sh")}, f, protocol=4)
    os.replace(tmp, CACHE)


def _load():
    """The cache dict: items, key, plus lazily 'all' (twins) and 'parks'."""
    key = (_stamp(), 2)
    if os.path.isfile(CACHE) and "--rebuild" not in sys.argv:
        try:
            with open(CACHE, "rb") as f:
                c = pickle.load(f)
            if c.get("key") == key:
                return c
        except Exception:
            pass
    if not has_matchings():
        sys.exit("asm/*/matchings is missing: run `make objdiff-config` (report mode) first, "
                 "otherwise there are no matched functions to compare against.")
    c = {"key": key, "items": _build_items()}
    _save(c)
    return c


def _ready():
    c = _load()
    if "inv" not in c:
        inv = collections.defaultdict(list)
        for i, it in enumerate(c["items"]):
            it["sh"] = _shingles(it["s"])
            for g in it["sh"]:
                inv[g].append(i)
        c["inv"] = inv
    return c


def _ratio(a, b):
    return difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()


def _best(c, i, want_stub, limit=6):
    """Top `limit` (strict, loose, j) for item i among stubs or matched functions."""
    items, inv = c["items"], c["inv"]
    it = items[i]
    cnt = collections.Counter()
    for g in it["sh"]:
        lst = inv[g]
        if len(lst) > 600:
            continue
        for j in lst:
            cnt[j] += 1
    cands = []
    n = len(it["s"])
    for j, hits in cnt.items():
        o = items[j]
        if j == i or o["stub"] != want_stub or len(o["s"]) < MIN_INSNS:
            continue
        m = len(o["s"])
        if 2.0 * min(n, m) / (n + m) < MIN_TWIN:  # the ratio cannot reach MIN_TWIN
            continue
        cands.append((hits / (len(it["sh"]) + len(o["sh"]) - hits), j))
    cands.sort(reverse=True)
    out = [(_ratio(it["s"], items[j]["s"]), _ratio(it["l"], items[j]["l"]), j) for _, j in cands[:limit]]
    out.sort(reverse=True)
    return out


def _info(c, i):
    return {"twins": [t for t in _best(c, i, False) if t[0] >= MIN_TWIN][:3],
            "sibs": [(t[0], t[2]) for t in _best(c, i, True) if t[0] >= 0.90]}


def all_twins(c):
    """{stub index: _info}, for every stub of MIN_INSNS or more; cached."""
    if "all" not in c:
        c["all"] = {i: _info(c, i) for i, it in enumerate(c["items"]) if it["stub"] and len(it["s"]) >= MIN_INSNS}
        _save(c)
    return c["all"]


def defs_in_src():
    """{name: [(src path relative to the tree, line)]} for depth-0 `Name(...) {` definitions."""
    out = collections.defaultdict(list)
    pat = re.compile(r"^[A-Za-z_][^;{}()\n]*?\b(\w+)\s*\([^;{}]*?\)\s*\{", re.M)
    for d, _, fs in os.walk(os.path.join(ROOT, "src")):
        for f in fs:
            if not f.endswith(".c"):
                continue
            p = os.path.join(d, f)
            text = open(p, errors="replace").read()
            text = re.sub(r"/\*.*?\*/", lambda m: re.sub(r"[^\n]", " ", m.group(0)), text, flags=re.S)
            text = re.sub(r"//[^\n]*", "", text)
            for m in pat.finditer(text):
                if m.group(1) in ("if", "while", "for", "switch", "return", "sizeof"):
                    continue
                out[m.group(1)].append((os.path.relpath(p, ROOT), text.count("\n", 0, m.start(1)) + 1))
    return out


def definition(defs, name, binary):
    """Best (path, line) for a twin, preferring its own binary's src dir."""
    hits = defs.get(name, [])
    own = [h for h in hits if h[0].startswith("src/%s/" % binary)]
    return (own or hits or [("?", 0)])[0]


def verdict(strict):
    if strict >= 0.98:
        return "PORT"
    if strict >= 0.80:
        return "TEMPLATE"
    return "LOOSE"


def same_words(a, b):
    return sum(1 for x, y in zip(a["w"], b["w"]) if x == y)


def _load_module(rel):
    spec = importlib.util.spec_from_file_location(os.path.basename(rel)[:-3], os.path.join(ROOT, rel))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def park_table():
    """{(unit, func): (pct or None, note text)} for every stub that owns a park, cached."""
    c = _load()
    key = (_src_stamp(), 2)
    if c.get("parks_key") == key and "parks" in c:
        return c["parks"]
    park = _load_module("tools/scripts/park.py")
    ps = _load_module("tools/rounds/park_sweep.py")
    trail = re.compile(r"(?:\s|//[^\n]*\n|/\*(?:[^*]|\*(?!/))*\*/)*\Z")
    table = {}
    for d, _, fs in os.walk(os.path.join(ROOT, "src")):
        for f in fs:
            if not f.endswith(".c"):
                continue
            sp = os.path.join(d, f)
            unit = os.path.relpath(sp, os.path.join(ROOT, "src"))[:-2]
            parked = dict(ps.parked_funcs(sp))
            if not parked:
                continue
            text = open(sp, errors="replace").read()
            for name, line in parked.items():
                inc = text.find(line)
                region = ""
                for m in park.BLOCK.finditer(text):
                    if not re.search(r"\b" + name + r"\s*\(", m.group(1)) or not park.body_in(m.group(1), name):
                        continue
                    if m.end() <= inc and park.filler_only(text[m.end():inc]):
                        a, b = m.start(), inc + len(line)
                    elif m.start() >= inc + len(line) and park.filler_only(text[inc + len(line):m.start()]):
                        a, b = inc, m.end()
                    else:
                        continue
                    mm = trail.search(text[:a], max(0, a - 3000))
                    region = text[(mm.start() if mm else a):b]
                    break
                # the headline figure: after PARKED/Best/Attempt, else the first percentage written
                # (a max over all of them picks up "100.00% under another preset" asides)
                pm = (re.search(r"(?:PARKED|BEST|Best|best|Attempt)[^0-9%]{0,15}(\d{1,3}(?:\.\d+)?)%", region)
                      or re.search(r"(\d{1,3}\.\d+)%", region) or re.search(r"(\d{1,3})%", region))
                pct = float(pm.group(1)) if pm and float(pm.group(1)) <= 100 else None
                table[(unit, name)] = (pct, " ".join(region.split())[:2000])
    c["parks"], c["parks_key"] = table, key
    _save(c)
    return table


def describe(c, i, defs, parks):
    items = c["items"]
    it = items[i]
    info = c["all"][i] if "all" in c and i in c["all"] else _info(c, i)
    lines = ["%s  %s  %d insns" % (it["name"], it["unit"], len(it["s"]))]
    if not info["twins"]:
        lines.append("  no twin")
    for strict, loose, j in info["twins"]:
        o = items[j]
        path, ln = definition(defs, o["name"], o["bin"])
        lines.append("  %-8s %s  %s:%d  strict %.3f loose %.3f  same-words %d/%d  (%s)" % (
            verdict(strict), o["name"], path, ln, strict, loose, same_words(it, o), len(it["w"]), o["unit"]))
    for ratio, j in info["sibs"]:
        o = items[j]
        pk = parks.get((o["unit"], o["name"]))
        note = "no park" if not pk else "park " + ("%.2f%%" % pk[0] if pk[0] is not None else "(no pct)")
        lines.append("  sibling  %s  %.3f  %d insns  %s  %s" % (o["name"], ratio, len(o["s"]), o["unit"], note))
    return lines


def queue_rows(minimum):
    """(rows, clusters, c): rows best first; clusters of stubs >= 0.90 alike, smallest first."""
    c = _ready()
    items = c["items"]
    info = all_twins(c)
    defs = defs_in_src()
    parks = park_table()
    rows = []
    for i, d in info.items():
        if not d["twins"] or d["twins"][0][0] < minimum:
            continue
        strict, loose, j = d["twins"][0]
        it, o = items[i], items[j]
        pk = parks.get((it["unit"], it["name"]))
        path, ln = definition(defs, o["name"], o["bin"])
        rows.append(dict(stub=it["name"], bin=it["bin"], unit=it["unit"], insns=len(it["s"]),
                         park=pk[0] if pk else None, parked=bool(pk), twin=o["name"],
                         twin_file="%s:%d" % (path, ln), twin_unit=o["unit"],
                         strict=strict, loose=loose, same=same_words(it, o)))
    rows.sort(key=lambda r: (-r["strict"], r["insns"]))
    parent = {}

    def find(x):
        while parent.setdefault(x, x) != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for i, d in info.items():
        for _, j in d["sibs"]:
            parent[find(i)] = find(j)
    groups = collections.defaultdict(list)
    for x in list(parent):
        groups[find(x)].append(x)
    clusters = []
    for g in groups.values():
        g = [i for i in g if len(items[i]["s"]) >= 12]
        if len(g) > 1:
            clusters.append(sorted(g, key=lambda i: len(items[i]["s"])))
    clusters.sort(key=lambda g: len(items[g[0]]["s"]))
    return rows, clusters, c


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("func", nargs="?")
    ap.add_argument("--bin")
    ap.add_argument("--queue", action="store_true")
    ap.add_argument("--min", type=float, default=0.8)
    ap.add_argument("--rebuild", action="store_true")
    a = ap.parse_args()
    if a.queue:
        rows, clusters, c = queue_rows(a.min)
        items = c["items"]
        parks = park_table()
        print("stub\tbin\tunit\tinsns\tparked%\ttwin\ttwin_file\tstrict\tloose\tsame_words")
        for r in rows:
            pk = "-" if not r["parked"] else ("%.2f" % r["park"] if r["park"] is not None else "?")
            print("\t".join(map(str, [r["stub"], r["bin"], r["unit"], r["insns"], pk, r["twin"], r["twin_file"],
                                      "%.3f" % r["strict"], "%.3f" % r["loose"], r["same"]])))
        print("\n# stub-stub clusters (>= 0.90, members >= 12 insns), smallest first")
        for g in clusters:
            cells = []
            for i in g:
                it = items[i]
                pk = parks.get((it["unit"], it["name"]))
                park = "" if not pk else ",park " + ("%.2f" % pk[0] if pk[0] is not None else "?")
                cells.append("%s(%s,%d%s)" % (it["name"], it["unit"], len(it["s"]), park))
            print("cluster\t" + "\t".join(cells))
        return
    if not a.func:
        ap.error("give a function name or --queue")
    c = _ready()
    found = [i for i, it in enumerate(c["items"])
             if it["stub"] and it["name"] == a.func and (a.bin is None or it["bin"] == a.bin)]
    if not found:
        sys.exit("%s is not an unmatched stub (asm/*/nonmatchings)%s" % (a.func, " in " + a.bin if a.bin else ""))
    defs = defs_in_src()
    parks = park_table()
    for i in found:
        print("\n".join(describe(c, i, defs, parks)))


if __name__ == "__main__":
    main()
