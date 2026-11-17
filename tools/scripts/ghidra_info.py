#!/usr/bin/env python3
"""Query the Ghidra facts database written by tools/ghidra/run.sh.

  ghidra_info.py <Func|addr> [--bin B] [--c] [--json]   one function's report
  ghidra_info.py --struct <global name|addr> [--bin B]  field-offset table for a global
  ghidra_info.py --struct-of <Func> <paramN>            field-offset table for a parameter
  ghidra_info.py --leaves [--bin B] [--limit N]         unmatched functions, leaf-first

The dump is keyed by address; names are re-resolved from this repo's current
config/symbol_addrs.*.txt at print time.  Dump location: $GHIDRA_OUT or
~/xw/ghidra-out.  Parameter numbers are 0-based (param0 = a0).  Field-access
"counts" are occurrences in Ghidra's decompiler pcode, not instruction counts.
"""
import argparse
import glob
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.environ.get("GHIDRA_OUT", os.path.expanduser("~/xw/ghidra-out"))
SLUS = "slus_006.64"
RAM = (0x80000000, 0x80200000)


def die(msg):
    sys.exit("ghidra_info: " + msg)


# ------------------------------------------------------------------ loading
_cache = {}


def load(path):
    if path not in _cache:
        with open(path) as f:
            _cache[path] = json.load(f)
    return _cache[path]


def programs():
    if not os.path.isdir(OUT):
        die("output dir %s missing; run tools/ghidra/run.sh (or set GHIDRA_OUT)" % OUT)
    progs = sorted(os.path.basename(os.path.dirname(p)) for p in glob.glob(OUT + "/*/_index.json"))
    if not progs:
        die("no <bin>/_index.json under %s; run tools/ghidra/run.sh" % OUT)
    return progs


def index(b):
    return load("%s/%s/_index.json" % (OUT, b))


def own_range(b):
    i = index(b)
    return int(i["ov_start"], 16), int(i["ov_end"], 16)


def func_json(b, addr):
    p = "%s/%s/%08X.json" % (OUT, b, addr)
    return load(p) if os.path.exists(p) else None


# -------------------------------------------------------------------- names
_SYM = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;(.*)$")
_names = {}


def symbols(b):
    """addr -> (name, size or None), plus sorted address list, from the config."""
    if b not in _names:
        d = {}
        path = os.path.join(REPO, "config", "symbol_addrs.%s.txt" % b)
        if os.path.exists(path):
            for line in open(path):
                m = _SYM.match(line)
                if m:
                    sz = re.search(r"size:(0x[0-9A-Fa-f]+|\d+)", m.group(3))
                    d[int(m.group(2), 16)] = (m.group(1), int(sz.group(1), 0) if sz else None)
        _names[b] = (d, sorted(d))
    return _names[b]


def owner(b, addr):
    """Which program's symbol file names this address when seen from program b."""
    if b != SLUS:
        lo, hi = own_range(b)
        if lo <= addr < hi:
            return b
    return SLUS


def name_of(b, addr, fallback=None):
    """Current name for an exact address (config first, then the dump's name)."""
    d, _ = symbols(owner(b, addr))
    if addr in d:
        return d[addr][0]
    if fallback:
        return fallback
    return "D_%08X" % addr


def addr_desc(b, addr):
    """name or name+0xN for any address."""
    import bisect
    o = owner(b, addr)
    d, keys = symbols(o)
    if addr in d:
        return d[addr][0]
    i = bisect.bisect_right(keys, addr) - 1
    if i >= 0 and addr - keys[i] < 0x2000:
        return "%s+0x%x" % (d[keys[i]][0], addr - keys[i])
    return "0x%08X" % addr


def val_desc(b, v):
    if isinstance(v, int) and RAM[0] <= v < RAM[1]:
        return addr_desc(b, v)
    if isinstance(v, int) and -0x8000 <= v < 0:
        return str(v)
    if isinstance(v, int) and v > 9:
        return "0x%X" % v
    return str(v)


def resolve_target(arg, b_hint):
    """-> list of (bin, addr) candidates for a name or address."""
    progs = programs()
    m = re.fullmatch(r"(?:0x|func_|D_|FUN_)?([0-9A-Fa-f]{8})", arg)
    cands = []
    if m:
        a = int(m.group(1), 16)
        for b in progs:
            if b_hint and b != b_hint:
                continue
            if "%08X" % a in index(b)["funcs"]:
                cands.append((b, a))
    # by current name
    for b in progs:
        if b_hint and b != b_hint:
            continue
        d, _ = symbols(b)
        for a, (n, _s) in d.items():
            if n == arg and "%08X" % a in index(b)["funcs"] and owner(b, a) == b:
                cands.append((b, a))
    for b in progs:
        if b_hint and b != b_hint:
            continue
        for a, f in index(b)["funcs"].items():
            if f["name"] == arg and (b, int(a, 16)) not in cands:
                cands.append((b, int(a, 16)))
    return sorted(set(cands))


def pick(arg, b_hint):
    c = resolve_target(arg, b_hint)
    if not c:
        die("function %r not found in %s (is it a function symbol? try an address, or --bin)" % (arg, OUT))
    if len(c) > 1:
        die("%r is ambiguous: %s; use --bin" % (arg, ", ".join("%s:%08X" % x for x in c)))
    return c[0]


# ------------------------------------------------------------- formatting
def proto(b, addr, fj):
    """Prototype with the current name."""
    sig = fj.get("signature")
    if not sig:
        return "? %s(...)  [no decompile]" % name_of(b, addr, fj["name"])
    p = sig["prototype"]
    return p.replace(fj["name"] + "(", name_of(b, addr, fj["name"]) + "(", 1)


def fmt_args(b, ca, ab, nargs):
    parts = []
    for i in range(nargs or 0):
        k = str(i)
        if k in ca:
            parts.append("%d=%s" % (i, val_desc(b, ca[k])))
        elif ab and k in ab:
            parts.append("%d=<%s>" % (i, base_desc(b, ab[k])))
    return ", ".join(parts)


def base_desc(b, base):
    """Human form of a base key (param:0, global:ADDR, deref:ADDR, ret:ADDR[+0x..])."""
    m = re.fullmatch(r"(param|global|deref|ret):([0-9A-F]{8}|\d+)(\+0x[0-9a-f]+)?", base)
    if not m:
        m2 = re.fullmatch(r"deref:(param:\d+)\+0x([0-9a-f]+)", base)
        if m2:
            return "param%s->[+0x%s]" % (m2.group(1)[6:], m2.group(2))
        return base
    kind, v, off = m.groups()
    off = off or ""
    if kind == "param":
        return "param%s%s" % (v, off)
    a = int(v, 16)
    if kind == "global":
        return "%s%s" % (addr_desc(b, a), off)
    if kind == "deref":
        return "*%s%s" % (addr_desc(b, a), off)
    return "ret of %s()%s" % (name_of(b, a), off)


def field_rows(b, fields):
    """Group raw field_accesses -> {base: {(offset, stride): [sizes set, r, w]}}."""
    g = {}
    for f in fields:
        row = g.setdefault(f["base"], {}).setdefault((f["offset"], f.get("stride", 0)), [set(), 0, 0])
        row[0].add(f["size"])
        row[1 if f["rw"] == "R" else 2] += f["count"]
    return g


def stride_s(s):
    return "" if not s else (" [idx stride 0x%x]" % s if s > 0 else " [idx ?]")


def fmt_off(o):
    return "-0x%x" % -o if o < 0 else "+0x%x" % o


# ------------------------------------------------------------ callers merge
def merged_callers(b, addr):
    """List of caller records (each with 'bin'), own program + slus_xrefs of overlays."""
    fj = func_json(b, addr)
    out = list(fj["callers"]) if fj else []
    if b == SLUS:
        for ob in programs():
            if ob == SLUS:
                continue
            p = "%s/%s/slus_xrefs.json" % (OUT, ob)
            if os.path.exists(p):
                out += [x for x in load(p).get("%08X" % addr, []) if x["kind"] in ("call", "tail")]
    return out


def data_refs(b, addr):
    out = []
    if b == SLUS:
        for ob in programs():
            p = "%s/%s/slus_xrefs.json" % (OUT, ob)
            if ob != SLUS and os.path.exists(p):
                out += [x for x in load(p).get("%08X" % addr, []) if x["kind"] in ("data", "funcptr")]
    return out


# ------------------------------------------------------------ one function
def report(b, addr, show_c):
    fj = func_json(b, addr)
    if fj is None:
        die("no dump for %s:%08X" % (b, addr))
    nm = name_of(b, addr, fj["name"])
    L = []
    L.append("%s  [%s %08X]  size 0x%x  stack frame 0x%x%s" % (
        nm, b, addr, fj["size"], fj["stack_frame"], "" if fj["decompile_ok"] else "  (DECOMPILE FAILED)"))
    sig = fj.get("signature")
    L.append("signature: " + proto(b, addr, fj) + ("   [BIOS/syscall thunk: real params unknown]" if fj.get("thunk") else ""))
    if sig:
        L.append("  params: " + (", ".join("%d:%s(%s)" % (p["index"], p["storage"], p["type"]) for p in sig["params"]) or "none")
                 + "   returns value: " + ("yes" if sig["returns_value"] else "no"))
    callers = merged_callers(b, addr)
    L.append("callers (%d):" % len(callers))
    for c in sorted(callers, key=lambda c: (c["bin"], c["from_func_addr"], c["at"])):
        ca = fmt_args(c["bin"], c.get("const_args") or {}, c.get("arg_bases"), c.get("nargs"))
        L.append("  %s [%s] @%s %s%s%s" % (
            name_of(c["bin"], int(c["from_func_addr"], 16), c["from_name"]), c["bin"], c["at"],
            "(tail) " if c["kind"] == "tail" else "", "args: " + ca if ca else "",
            "" if c.get("ret_used") is not True else "  [uses result]"))
    dr = data_refs(b, addr)
    if dr:
        L.append("referenced as data/function pointer from (%d):" % len(dr))
        for c in dr:
            L.append("  %s [%s] %s" % (name_of(c["bin"], int(c["from_func_addr"], 16), c["from_name"]), c["bin"], c["kind"]))
    L.append("callees (%d):" % len(fj["callees"]))
    for c in fj["callees"]:
        ca_addr = int(c["addr"], 16)
        cb = c["bin"]
        cj = func_json(cb, ca_addr)
        cn = name_of(cb, ca_addr, c.get("name") or "func_%08X" % ca_addr)
        ca = fmt_args(b, c.get("const_args") or {}, c.get("arg_bases"), c.get("nargs"))
        L.append("  @%s %s%s  nargs=%s ret_used=%s%s" % (
            c["at"], "(tail) " if c["kind"] == "tail" else "", cn,
            c["nargs"] if c["nargs"] is not None else "?", "?" if c["ret_used"] is None else ("yes" if c["ret_used"] else "no"),
            ("  args: " + ca) if ca else ""))
        if cj:
            L.append("       " + proto(cb, ca_addr, cj) + ("   [thunk: params unknown]" if cj.get("thunk") else ""))
    if fj["indirect_calls"]:
        L.append("indirect calls (%d):" % len(fj["indirect_calls"]))
        for c in fj["indirect_calls"]:
            v = c["via"]
            ca = fmt_args(b, c.get("const_args") or {}, c.get("arg_bases"), c.get("nargs"))
            L.append("  @%s via %s%s%s  nargs=%d ret_used=%s%s" % (
                c["at"], base_desc(b, v["base"]), fmt_off(v["offset"]) if v["offset"] else "",
                stride_s(v.get("stride", 0)), c["nargs"], "yes" if c["ret_used"] else "no",
                ("  args: " + ca) if ca else ""))
    if fj["func_refs"]:
        L.append("function addresses taken: " + ", ".join(
            "%s@%s" % (name_of(b, int(x["addr"], 16)), x["at"]) for x in fj["func_refs"]))
    L.append("globals (%d):" % len(fj["globals"]))
    for g in fj["globals"]:
        a = int(g["addr"], 16)
        L.append("  %-32s %08X %-10s sizes=%s%s" % (
            addr_desc(b, a), a, g["access"], ",".join(map(str, g["sizes"])) or "-",
            "  (address taken)" if g["addr_taken"] and g["access"] != "addr-taken" else ""))
    if fj["strings"]:
        L.append("strings:")
        for s in fj["strings"]:
            L.append("  %s %r" % (s["addr"], s["text"]))
    if fj["jump_tables"]:
        L.append("jump tables: " + ", ".join("@%s %d cases incl. default? (%d distinct targets)" % (
            j["at"], j["ncases"], j["ntargets"])
                                             for j in fj["jump_tables"]))
    L.append("field accesses (by base; counts are pcode occurrences):")
    grouped = field_rows(b, fj["field_accesses"])
    for base in sorted(grouped, key=lambda k: (k.startswith("other"), k)):
        L.append("  %s:" % base_desc(b, base))
        for (off, stride), (sizes, r, w) in sorted(grouped[base].items()):
            L.append("    %-8s size %-5s R%-3d W%-3d%s" % (
                fmt_off(off), "/".join(map(str, sorted(sizes))), r, w, stride_s(stride)))
    if show_c:
        L.append("\n--- ghidra C (reference only) ---")
        L.append(open("%s/%s/%08X.c" % (OUT, b, addr)).read())
    return "\n".join(L)


def merged_json(b, addr):
    fj = dict(func_json(b, addr))
    fj["name_current"] = name_of(b, addr, fj["name"])
    fj["callers"] = merged_callers(b, addr)
    fj["data_refs"] = data_refs(b, addr)
    return fj


# ---------------------------------------------------------------- struct view
def struct_table(b0, title, entries, note=None):
    """entries: list of (bin, funcaddr, field dict). Print an offset table."""
    rows = {}
    for b, fa, f in entries:
        r = rows.setdefault((f["offset"], f.get("stride", 0)), {"sizes": set(), "r": 0, "w": 0, "funcs": {}})
        r["sizes"].add(f["size"])
        r["r" if f["rw"] == "R" else "w"] += f["count"]
        r["funcs"][(b, fa)] = 1
    print(title)
    if note:
        print("  " + note)
    if not rows:
        print("  (no accesses)")
        return
    print("  %-9s %-9s %5s %5s %6s  %s" % ("offset", "size(s)", "reads", "writes", "funcs", "examples"))
    for (off, stride), r in sorted(rows.items()):
        ex = []
        for (b, fa) in list(r["funcs"])[:3]:
            ex.append(name_of(b, fa, index(b)["funcs"]["%08X" % fa]["name"]))
        print("  %-9s %-9s %5d %5d %6d  %s%s" % (
            fmt_off(off), "/".join(map(str, sorted(r["sizes"]))), r["r"], r["w"], len(r["funcs"]),
            ", ".join(ex), stride_s(stride)))


def sees(b, a):
    if b == SLUS:
        lo, hi = own_range(b)
        return lo <= a < hi
    lo, hi = own_range(b)
    return lo <= a < hi or a < lo


def struct_global(arg, b_hint):
    m = re.fullmatch(r"(?:0x|D_)?([0-9A-Fa-f]{8})", arg)
    cands = {}  # addr -> display name
    for b in programs():
        d, _ = symbols(b)
        for a, (n, _s) in d.items():
            if n == arg or (m and a == int(m.group(1), 16)):
                cands.setdefault(a, n)
    if m:
        cands.setdefault(int(m.group(1), 16), "0x" + m.group(1).upper())
    if not cands:
        die("global %r not found in config/symbol_addrs.*.txt" % arg)
    for a in sorted(cands):
        print("== %s  (0x%08X) ==" % (cands[a], a))
        for kind, title in (("global", "the global itself is the struct/array (name->field):"),
                            ("deref", "pointer stored in the global (*name->field):")):
            ents = []
            for b in programs():
                if b_hint and b != b_hint or not sees(b, a):
                    continue
                for fa in index(b)["field_bases"].get("%s:%08X" % (kind, a), []):
                    fj = func_json(b, int(fa, 16))
                    for f in fj["field_accesses"]:
                        if f["base"] == "%s:%08X" % (kind, a):
                            ents.append((b, int(fa, 16), f))
            struct_table(b_hint, title, ents)
    print("(note: a base is the nearest preceding data symbol at export time, so accesses to unnamed "
          "neighbours within 0x2000 can be folded in; offsets past the symbol's size are suspect)")


def struct_of(fname, pname, b_hint):
    b, a = pick(fname, b_hint)
    m = re.fullmatch(r"(?:param|a)?_?(\d+)", pname)
    if not m:
        die("parameter must look like param0, a0 or 0 (0-based)")
    n = int(m.group(1))
    fj = func_json(b, a)
    base = "param:%d" % n
    ents = [(b, a, f) for f in fj["field_accesses"] if f["base"] == base]
    struct_table(b, "%s %s: accesses inside this function only (Ghidra does not propagate through callees)" % (
        name_of(b, a, fj["name"]), "param%d" % n), ents)
    callers = merged_callers(b, a)
    srcs = {}
    for c in callers:
        k = str(n)
        if k in (c.get("const_args") or {}):
            v = c["const_args"][k]
            s = "const " + val_desc(c["bin"], v)
        elif k in (c.get("arg_bases") or {}):
            s = base_desc(c["bin"], c["arg_bases"][k])
            if s.startswith("param"):
                s += " of " + name_of(c["bin"], int(c["from_func_addr"], 16), c["from_name"])
        else:
            s = "(not determinable)"
        srcs.setdefault(s, []).append(name_of(c["bin"], int(c["from_func_addr"], 16), c["from_name"]))
    print("\ncallers pass in slot %d (%d call sites):" % (n, len(callers)))
    for s, who in sorted(srcs.items(), key=lambda kv: -len(kv[1])):
        print("  %-40s x%-3d %s" % (s, len(who), ", ".join(sorted(set(who))[:3])))
    print("(for a global source, run --struct <global> for the cross-function view)")


# --------------------------------------------------------------------- leaves
def include_asm_names():
    """{bin: {name: unit path}} from INCLUDE_ASM lines under src/."""
    r = {}
    pat = re.compile(r"^\s*INCLUDE_ASM\(\s*[^,]+,\s*(\w+)\s*\)")
    for path in glob.glob(os.path.join(REPO, "src", "**", "*.c"), recursive=True):
        rel = os.path.relpath(path, os.path.join(REPO, "src"))
        b = rel.split(os.sep)[0]
        for line in open(path, errors="replace"):
            m = pat.match(line)
            if m:
                r.setdefault(b, {})[m.group(1)] = rel
    return r


def leaves(b_hint, limit):
    inc = include_asm_names()
    progs = programs()
    # current names + callers count
    ncallers = {}
    for b in progs:
        for a, f in index(b)["funcs"].items():
            for c in f["callees"]:
                cb, ca = c.split(":")
                ncallers.setdefault((cb, ca), set()).add((b, a))
    unmatched = {}  # (bin, ADDR) -> (name, unit)
    for b in progs:
        for a, f in index(b)["funcs"].items():
            n = name_of(b, int(a, 16), f["name"])
            if n in inc.get(b, {}):
                unmatched[(b, a)] = (n, inc[b][n])
    rows = []
    for (b, a), (n, unit) in unmatched.items():
        if b_hint and b != b_hint:
            continue
        f = index(b)["funcs"][a]
        pend = set()
        for c in f["callees"]:
            cb, ca = c.split(":")
            if (cb, ca) in unmatched and (cb, ca) != (b, a):
                pend.add((cb, ca))
        rows.append((len(pend), f["size"], b, a, n, unit, len(ncallers.get((b, a), ()))))
    rows.sort()
    print("# unmatched = still INCLUDE_ASM'd; deps = unmatched direct callees (leaf-first, then by size)")
    print("%-8s %-19s %6s %7s %4s  %s  %s" % ("addr", "bin", "size", "callers", "deps", "name", "unit"))
    for deps, size, b, a, n, unit, nc in rows[:limit]:
        print("%s %-19s %6d %7d %4d  %s  %s" % (a, b, size, nc, deps, n, unit))
    print("# %d unmatched functions, %d shown" % (len(rows), min(limit, len(rows))))


# ----------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("func", nargs="?")
    ap.add_argument("param", nargs="?")
    ap.add_argument("--bin")
    ap.add_argument("--c", action="store_true", help="append Ghidra's decompiled C")
    ap.add_argument("--json", action="store_true", help="emit the merged record")
    ap.add_argument("--struct", metavar="GLOBAL")
    ap.add_argument("--struct-of", action="store_true", help="--struct-of <Func> <paramN>")
    ap.add_argument("--leaves", action="store_true")
    ap.add_argument("--limit", type=int, default=60)
    a = ap.parse_args()
    if a.leaves:
        leaves(a.bin, a.limit)
    elif a.struct:
        struct_global(a.struct, a.bin)
    elif a.struct_of:
        if not (a.func and a.param):
            die("--struct-of needs <Func> <paramN>")
        struct_of(a.func, a.param, a.bin)
    elif a.func:
        b, addr = pick(a.func, a.bin)
        if a.json:
            print(json.dumps(merged_json(b, addr), indent=1))
        else:
            print(report(b, addr, a.c))
    else:
        ap.print_help()


if __name__ == "__main__":
    main()
