#!/usr/bin/env python3
"""Print a ready-to-launch matching round, so the orchestrator does not pick targets by hand.

    tools/rounds/next_round.py                          # 6 lanes + 6 queued, recorded in the history log
    tools/rounds/next_round.py --dry-run --json         # just the args, record nothing
    tools/rounds/next_round.py --kind twin --lanes 4    # auto | twin | park | fresh | big | redo
    tools/rounds/next_round.py --stats                  # the pool by class, one screen

Output: the `clone.sh new` tag list and the args for
subagent({workflow: "tools/rounds/brief.js", async: true, args: ...}). One lane per unit
(.c file). `auto` fills lanes in this order, each lane's `extra` naming its class:

  1 park100   parks whose note records 100.00% (probably free: un-park, verify)
  2 twin      stubs with a matched twin >= 0.98 (PORT) then 0.80-0.98 (TEMPLATE), see twin_for.py
  3 cluster   smallest member of a stub-stub family (>= 0.90 alike), so one match solves them all
  4 park      parks >= 99%, then 80-99%
  5 fresh     never-written stubs under 350 instructions, smallest first, 2-5 per lane
  6 big       never-written stubs of 350+ instructions, one per 60-minute lane
  7 redo      parks under 80% (or with no percentage) and noted stubs: re-measure, then rewrite
              from m2c_draft.sh; 2-4 per 30-minute lane under 350 instructions, else one per 60

Never emitted: handwritten, finished units (slus_006.64/system/model, psyq libgte*/libapi*/libcard*),
slus_006.64/system/menu func_8001C76C, stack-switch stubs, convertible.py `insn after .size`
walls, jump tables in units that do not own them, <15-insn literal-address data words, and
atfree stubs of non-cdk-G0 units (except park100 and twin lanes).
Cooldown: a function emitted in the last 12 rounds is skipped; one emitted 3+ times in the same
class is skipped unless its park % has risen since. History: ~/xw/lane-history.tsv
(round, unix time, unit, funcs, class, park pcts), or $LANE_HISTORY / --history.
"""
import argparse
import collections
import glob
import importlib.util
import json
import math
import os
import re
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "scripts"))
import twin_for  # noqa: E402

# Round 535: the park class fell to 23 eligible entries of 281 while rounds 530-534
# showed a re-briefed improved park is the best target there is (round 533 turned three
# 40-round-stale parks into matches at retail's exact bytes). EVER still caps repeats,
# and a park only re-emits when its measured percentage rose.
COOLDOWN = 8
EVER = 3
BIG = 350
ABBREV = {"battle": "b", "battling": "bt", "field": "f", "menu": "m", "world_map": "wm", "lib_gear": "lg",
          "slus_006.64": "s", "gear_shop_menu": "gs", "shop_menu": "sh", "member_change_menu": "mc",
          "movie": "mv"}
CLASS_ORDER = ["park100", "twin", "cluster", "park", "fresh", "big", "redo"]
# `park` has had 297 eligible entries for many rounds and, standing fourth in
# CLASS_ORDER, it took every lane of rounds 417-420: 44 lanes, 7 matches (0.16 a
# lane) while `fresh`, `big` and `redo` got no seat at all. In `auto` a single
# class may therefore hold at most this share of the round's lanes, so the later
# classes always get lanes; an explicit --kind is not capped.
# Round 420 measured the mix: with park capped at 0.6 the fresh lanes that got seats
# matched 3 of 7 while park's 43 lanes of rounds 418-420 matched 5 (0.12 a lane). Park
# therefore now takes at most 0.4 of an auto round.
CLASS_SHARE = {"park": 0.35}
# The park class is at its floor as PROMPT.md defines it: rounds 418-421 briefed 50 park
# lanes for 4 matches (0.08 a lane) while big went 3 lanes/1 match and fresh 9/4. So park
# keeps one or two seats a round and `big` - 15 eligible never-written functions of 350+
# instructions, the largest untouched pool - takes the rest.
# Round 422 raised it to 0.35: eight big lanes parked brand-new bodies, five of them over
# 78% (one at 98.87), so the closest park is now worth more than another never-written one.
INSN = re.compile(r"^\s+/\*\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+[0-9A-Fa-f]{8}\s*\*/")


def _mod(rel):
    return twin_for._load_module(rel)


def history_path(args):
    return os.path.expanduser(args.history or os.environ.get("LANE_HISTORY") or "~/xw/lane-history.tsv")


def read_history(path):
    """[(round, unit, [funcs], class, [pcts])]"""
    out = []
    if os.path.isfile(path):
        for ln in open(path):
            p = ln.rstrip("\n").split("\t")
            if len(p) >= 5 and p[0].isdigit():
                pcts = p[5].split(",") if len(p) > 5 else []
                out.append((int(p[0]), p[2], p[3].split(","), p[4], pcts))
    return out


def tracker_round():
    best = 0
    try:
        for ln in open(os.path.join(ROOT, "reports", "TRACKER.md")):
            m = re.match(r"\| (\d+) \|", ln)
            if m:
                best = max(best, int(m.group(1)))
    except OSError:
        pass
    return best


def cdk0_units():
    """Units compiled cdk -G0, i.e. the only ones whose `jal free` tail can be emitted.

    A path key may carry the `{binary}` placeholder (the per-binary preset tables in
    gears.toml use it), so both spellings go in: the literal key and the placeholder
    form that in_cdk0() rebuilds for each unit."""
    out = set()
    text = open(os.path.join(ROOT, "gears.toml")).read()
    for blk in text.split("[[build_preset]]"):
        if "gcc-2.7.2-cdk-psx" in blk and re.search(r'gp_flag\s*=\s*"-G0"', blk):
            out.update(x.rstrip(".") for x in re.findall(r'"([\w./{}]+?\.)"', blk))
    return out


def in_cdk0(unit, cdk0):
    if unit in cdk0:
        return True
    stripped = unit.split("/", 1)[1] if "/" in unit else unit
    return "{binary}/" + stripped in cdk0


def preset_text(unit, table):
    binary = unit.split("/", 1)[0]
    stripped = unit.split("/", 1)[1] if unit.count("/") > 1 else unit
    hit = ("Default", "")
    for name, paths, variables in table:
        paths = [p.replace("{binary}", binary) for p in paths]
        if any((unit + ".").startswith(p) or (stripped + ".").startswith(p) for p in paths):
            hit = (name, variables)
    name, variables = hit
    pairs = dict(re.findall(r'(\w+)\s*=\s*"([^"]*)"', variables))
    if not pairs:
        return "Default (gcc-2.7.2-psx, -O2 -G8)"
    parts = [pairs.pop("gcc", "gcc-2.7.2-psx")]
    parts += ["%s %s" % (k[:-6] if k.endswith("_flags") else k, v) for k, v in pairs.items()]
    return "%s (%s)" % (name, ", ".join(parts))


def owns_tables(binary, unit):
    try:
        cfg = open(os.path.join(ROOT, "config", binary + ".yaml")).read()
    except OSError:
        return False
    return re.search(r"\.rodata, %s\]" % re.escape(unit), cfg) is not None


def is_finished(unit, func):
    tail = "/".join(unit.split("/")[-2:])
    if unit == "slus_006.64/system/model" or (tail.startswith("psyq/") and tail[5:] in (
            "libgte", "libgte_b", "libapi", "libapi_2", "libapi_3", "libcard", "libcard_b")):
        return True
    return unit == "slus_006.64/system/menu" and func == "func_8001C76C"


def stub_reason(path, binary, unit):
    """Why a stub .s must never be briefed, or None."""
    text = open(path, errors="replace").read()
    if "Handwritten function" in text:
        return "handwritten"
    if re.search(r"\b(sw|sd)\s+\$sp\b", text):
        return "stack-switch"
    body = text.splitlines()
    name = os.path.basename(path)[:-2]
    size_at = next((i for i, ln in enumerate(body) if re.match(r"\.size\s+%s\b" % re.escape(name), ln)), None)
    if size_at is not None and any(INSN.match(ln) for ln in body[size_at + 1:]):
        return "insn-after-size"
    if ("jtbl" in text or "jlabel" in text) and not owns_tables(binary, unit.split("/", 1)[1]):
        return "jtbl-not-owned"
    insns = [ln for ln in body if INSN.match(ln)]
    # A bare `j <callee>` (+ nop) is a hand-written sibling-call thunk: no compiler in
    # tools/ emits MIPS sibcalls (round 424 measured the mips.md define_expands in 2.6.0,
    # 2.7.2 and 2.8.1), so such a stub can never be C.
    if 0 < len(insns) <= 3:
        ops = [re.sub(r"^\s+/\*[^*]*\*/\s*", "", ln).split()[0] for ln in insns]
        if ops[0] in ("j", "jal") and all(o in ("nop", "jr", "ra") for o in ops[1:]):
            return "sibling-call"
    if 0 < len(insns) < 15:
        m = re.match(r"\s+/\*\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+([0-9A-Fa-f]{8})", insns[0])
        word = int(m.group(1), 16)
        word = int.from_bytes(word.to_bytes(4, "big"), "little")  # the comment is the raw bytes, little-endian
        if 0x80000000 <= word < 0x80200000:
            return "literal-address"
    return None


def census(history_rounds=None):
    """Every stub with its insns, park, flags: a list of dicts."""
    c = twin_for._ready()
    insns = {(it["unit"], it["name"]): len(it["s"]) for it in c["items"] if it["stub"]}
    parks = twin_for.park_table()
    atfree = _mod("tools/scripts/atfree.py")
    cdk0 = cdk0_units()
    out, seen = [], set()
    for sp in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True)):
        unit = os.path.relpath(sp, os.path.join(ROOT, "src"))[:-2]
        text = open(sp, errors="replace").read()
        lines = text.split("\n")
        binary = unit.split("/")[0]
        stubs = [(m.group(1), m.group(2)) for m in re.finditer(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', text)]
        for n, (asm_dir, name) in enumerate(stubs):
            if (unit, name) in seen or (unit, name) not in insns:
                continue
            seen.add((unit, name))
            spath = os.path.join(ROOT, asm_dir, name + ".s")
            if not os.path.isfile(spath):
                continue
            pk = parks.get((unit, name))
            reason = "finished" if is_finished(unit, name) else stub_reason(spath, binary, unit)
            if reason is None and not in_cdk0(unit, cdk0) and atfree.unmatchable(spath):
                reason = "atfree"
            noted = False
            if pk is None:  # a percentage near the stub with no parked body is still an attempt
                idx = next((i for i, l in enumerate(lines) if "INCLUDE_ASM" in l and
                            re.search(r"[,\s]%s\s*\)" % re.escape(name), l)), None)
                if idx is not None:
                    nxt = next((j for j in range(idx + 1, len(lines)) if "INCLUDE_ASM" in lines[j]), len(lines))
                    noted = any(re.search(r"\d%", l) for l in lines[idx + 1:nxt])
            out.append(dict(unit=unit, bin=binary, func=name, insns=insns[(unit, name)], file="src/%s.c" % unit,
                            asm=asm_dir, parked=pk is not None, pct=pk[0] if pk else None,
                            note=pk[1] if pk else "", reason=reason, noted=noted))
    return out


def classify(stubs, twin_rows, clusters, items):
    """Set s['cls'] on each stub (highest class it qualifies for) and attach twin/family."""
    by = {(s["unit"], s["func"]): s for s in stubs}
    for s in stubs:
        s["cls"] = None
        s["twin"] = None
        s["family"] = None
    for r in twin_rows:
        s = by.get((r["unit"], r["stub"]))
        if s and (s["bin"] == r["bin"]):
            s["twin"] = r
    for g in clusters:
        names = [(items[i]["unit"], items[i]["name"]) for i in g]
        for k in names:
            if k in by:
                by[k]["family"] = [n for n in names if n != k]
    for s in stubs:
        p = s["pct"]
        if s["parked"] and p is not None and p >= 99.995:
            s["cls"] = "park100"
        elif s["twin"]:
            s["cls"] = "twin"
        elif s["family"] and not s["parked"]:
            s["cls"] = "cluster"
        elif s["parked"]:
            s["cls"] = "park" if p is not None and p >= 80 else "redo"
        elif s["noted"]:
            s["cls"] = "redo"
        else:
            s["cls"] = "big" if s["insns"] >= BIG else "fresh"
    return by


def cooled(s, cls, history, round_no):
    """True when the stub may be emitted again in class `cls`."""
    name = s["func"]
    ever = []
    for r, unit, funcs, c, pcts in history:
        if name not in funcs or r == round_no:
            continue
        if r >= round_no - COOLDOWN:
            return False
        if c == cls:
            i = funcs.index(name)
            ever.append(float(pcts[i]) if i < len(pcts) and pcts[i] not in ("", "-") else None)
    if len(ever) >= EVER:
        old = [p for p in ever if p is not None]
        if s["pct"] is None or not old or s["pct"] <= max(old):
            return False
    return True


def eligible_stubs(stubs, history, round_no):
    out = {}
    for s in stubs:
        cls = s["cls"]
        if s["reason"] and not (s["reason"] == "atfree" and cls in ("park100", "twin")):
            continue
        if cls not in CLASS_ORDER:
            continue
        if cooled(s, cls, history, round_no):
            out[(s["unit"], s["func"])] = s
    return out


def clean(text):
    return text.replace("`", "'").replace("${", "$ {")


def note_excerpt(s, n=320):
    note = clean(re.sub(r"\s+", " ", re.sub(r"INCLUDE_ASM\([^)]*\);", "", s["note"]))).strip()
    return note[:n] + ("..." if len(note) > n else "")


def matching_asm(twin, name):
    rest = twin["twin_unit"].split("/", 1)
    p = os.path.join("asm", rest[0], "matchings", rest[1] if len(rest) > 1 else "", name + ".s")
    return p if os.path.isfile(os.path.join(ROOT, p)) else None


CLS = {"park100": "park", "twin": "twin", "cluster": "cluster", "park": "park", "fresh": "fresh", "big": "big",
       "redo": "redo"}


def lane_for(unit, funcs, cls, stubs_by, parks_hint=None):
    """Build (minutes, extra) for a lane of eligible stubs `funcs` (stub dicts) in one unit.

    `extra` carries only lane-specific evidence; each class's fixed procedure text is CLASS_NOTE in brief.js."""
    total = sum(f["insns"] for f in funcs)
    names = ", ".join("%s (%d insns)" % (f["func"], f["insns"]) for f in funcs)
    if cls == "park100":
        minutes = 30 if total < BIG else 60
        extra = "Notes say 100.00 percent (probably stale): " + " | ".join(
            "%s (%d insns): %s" % (f["func"], f["insns"], note_excerpt(f, 160)) for f in funcs)
    elif cls == "twin":
        minutes = 30 if total < BIG else 60
        bits = []
        for f in funcs:
            t = f["twin"]
            tasm = matching_asm(t, t["twin"])
            bits.append("%s (%d insns) %s %s at %s, strict %.3f loose %.3f, %d of %d words identical%s%s" % (
                f["func"], f["insns"], "PORT of" if t["strict"] >= 0.98 else "TEMPLATE from", t["twin"], t["twin_file"],
                t["strict"], t["loose"], t["same"], f["insns"], "; twin asm " + tasm if tasm else "",
                "; own park at %s percent" % ("%.2f" % f["pct"] if f["pct"] is not None else "?") if f["parked"] else ""))
        extra = " | ".join(bits)
    elif cls == "cluster":
        f = funcs[0]
        minutes = 30 if f["insns"] < BIG else 60
        fam = [("%s (%s)" % (n, u)) for u, n in f["family"]]
        more = " and %d more" % (len(fam) - 10) if len(fam) > 10 else ""
        extra = "%s (%d insns), siblings: %s%s" % (f["func"], f["insns"], ", ".join(fam[:10]), more)
    elif cls == "park":
        # Rounds 527-529 lost fourteen park lanes to the wall clock: a lane spends the whole
        # budget in permuter runs and diff attempts and never reaches its re-park, so
        # nothing is committed. Short lanes, a long queue, and brief.js's CHECKPOINT
        # (commit the park by budget-8 whatever happened) are the shape. A 99%+ park is the
        # one case worth a longer lane: its residual is a register or slot, the permuter
        # searches it, and round 529 cut a 99.96 park at 30 minutes.
        near = any(f["pct"] is not None and f["pct"] >= 99 for f in funcs)
        minutes = 45 if near else 30
        extra = " | ".join("%s at %.2f percent, %d insns, note: %s" % (f["func"], f["pct"], f["insns"], note_excerpt(f, 160))
                           for f in funcs)
    elif cls == "fresh":
        # Never-written stubs are a from-scratch transcription: round 401 capped three of
        # twelve lanes at 30 minutes and all three were over 140 instructions.
        minutes = 45
        extra = "Never written, no park or note: " + names
    elif cls == "redo":
        # A redo lane rewrites the body from an m2c draft, which is slower than re-spelling a
        # park: round 398 ran five of 18 lanes into the 30-minute cap, so 45 minutes and at
        # most two functions per lane.
        minutes = 45 if all(f["insns"] < BIG for f in funcs) else 60
        extra = " | ".join("%s (%d insns) %s%s" % (
            f["func"], f["insns"], "scored %.2f percent" % f["pct"] if f["pct"] is not None else "no usable percentage",
            ", note: " + note_excerpt(f, 100) if f["note"] else "") for f in funcs)
    else:  # big
        f = funcs[0]
        minutes = 60
        extra = "%s (%d instructions), never written" % (f["func"], f["insns"])
    return minutes, extra


def pick(funcs, key, limit, budget):
    """Up to `limit` funcs in `key` order whose insn total stays within budget (always at least one)."""
    out, total = [], 0
    for f in sorted(funcs, key=key):
        if out and (len(out) >= limit or total + f["insns"] > budget):
            continue
        out.append(f)
        total += f["insns"]
    return out


def candidate_lanes(cls, elig):
    """Lanes of one class, best first: [(sort key, unit, [stub dicts])]."""
    per = collections.defaultdict(list)
    for s in elig.values():
        if s["cls"] == cls:
            per[s["unit"]].append(s)
    lanes = []
    for unit, fs in per.items():
        if cls == "park100":
            chosen = pick(fs, lambda f: f["insns"], 3, 450)
            lanes.append(((0, sum(f["insns"] for f in chosen)), unit, chosen))
        elif cls == "twin":
            chosen = pick(fs, lambda f: (-f["twin"]["strict"], f["insns"]), 3, 450)
            lanes.append(((-chosen[0]["twin"]["strict"], sum(f["insns"] for f in chosen)), unit, chosen))
        elif cls == "cluster":
            chosen = pick(fs, lambda f: f["insns"], 1, 0)
            lanes.append(((-len(chosen[0]["family"]), chosen[0]["insns"]), unit, chosen))
        elif cls == "park":
            hi = [f for f in fs if f["pct"] >= 99]
            chosen = pick(hi or fs, lambda f: (-f["pct"], f["insns"]), 2, 450)
            lanes.append(((0 if hi else 1, -chosen[0]["pct"], chosen[0]["insns"]), unit, chosen))
        elif cls == "fresh":
            chosen = pick(fs, lambda f: f["insns"], 5, 450)
            lanes.append(((sum(f["insns"] for f in chosen) / len(chosen),), unit, chosen))
        elif cls == "redo":
            small = [f for f in fs if f["insns"] < BIG]
            chosen = pick(small, lambda f: f["insns"], 2, 450) if small else pick(fs, lambda f: f["insns"], 1, 0)
            lanes.append(((sum(f["insns"] for f in chosen) / len(chosen),), unit, chosen))
        else:
            chosen = pick(fs, lambda f: f["insns"], 1, 0)
            lanes.append(((chosen[0]["insns"],), unit, chosen))
    lanes.sort(key=lambda x: x[0])
    return lanes


def make_tag(unit, round_no, used):
    parts = unit.split("/")
    last = parts[-1]
    m = re.match(r"misc(\d.*)$", last)
    tail = re.sub(r"[^a-z0-9]", "", (m.group(1) if m else last).lower())[:8]
    base = "r%d-%s%s" % (round_no, ABBREV.get(parts[0], parts[0][:2]), tail)
    tag, n = base, 0
    while tag in used:
        n += 1
        tag = base + "abcdefgh"[n % 8]
    used.add(tag)
    return tag


def worker(unit, tag, funcs, cls, table):
    binary = unit.split("/")[0]
    minutes, extra = lane_for(unit, funcs, cls, None)
    sym = "config/symbol_addrs.%s.txt" % binary
    if binary != "slus_006.64":
        sym += " plus config/symbol_addrs.slus_006.64.txt"
    return {
        "tree": os.path.expanduser("~/xw/r-%s/xenogears-decomp" % tag),
        "file": funcs[0]["file"],
        "unit": unit,
        "asm": funcs[0]["asm"],
        "binary": binary,
        "overlay": binary,
        "preset": preset_text(unit, table),
        "symbols": sym,
        "funcs": [f["func"] for f in funcs],
        "minutes": minutes,
        "cls": CLS[cls],
        "extra": extra,
    }


def plan(args, elig, round_no):
    kinds = {"auto": CLASS_ORDER, "twin": ["twin", "cluster"], "park": ["park100", "park"],
             "fresh": ["fresh"], "big": ["big"], "redo": ["redo"]}[args.kind]
    table = list(_mod("tools/scripts/preset_for.py").presets())
    want = args.lanes + args.queue
    taken, used, tags, out = set(), set(), set(), []
    for cls in kinds:
        cap = want if args.kind != "auto" or cls not in CLASS_SHARE else max(1, int(want * CLASS_SHARE[cls]))
        for _, unit, funcs in candidate_lanes(cls, elig):
            if len(out) >= want or sum(1 for o in out if o[2] == cls) >= cap:
                break
            if unit in taken:
                continue
            # Round 408: a header field pass edits every user of the header, so the
            # round's matching lanes must stay out of that header's binaries.
            if args.exclude_binaries and unit.split("/", 1)[0] in args.exclude_binaries:
                continue
            taken.add(unit)
            tag = make_tag(unit, round_no, tags)
            out.append((worker(unit, tag, funcs, cls, table), tag, cls, funcs))
    return out


def record(path, round_no, lanes):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    now = int(time.time())
    with open(path, "a") as f:
        for w, tag, cls, funcs in lanes:
            pcts = ",".join("%.2f" % x["pct"] if x["pct"] is not None else "-" for x in funcs)
            f.write("%d\t%d\t%s\t%s\t%s\t%s\n" % (round_no, now, w["unit"], ",".join(x["func"] for x in funcs), cls, pcts))


def stats(stubs, elig):
    rows = collections.OrderedDict((c, [0, 0, 0, 0]) for c in CLASS_ORDER)
    excl = collections.Counter()
    for s in stubs:
        if s["reason"]:
            excl[s["reason"]] += 1
        r = rows[s["cls"]]
        r[0] += 1
        r[1] += s["insns"] * 4
        if (s["unit"], s["func"]) in elig:
            r[2] += 1
            r[3] += s["insns"] * 4
    lines = ["pool: %d stubs, %d bytes, %d units; excluded outright: %s"
             % (len(stubs), sum(s["insns"] * 4 for s in stubs), len({s["unit"] for s in stubs}),
                ", ".join("%s %d" % kv for kv in excl.most_common()) or "none"),
             "%-11s %6s %9s %9s %10s" % ("class", "stubs", "bytes", "eligible", "elig bytes")]
    for c, (n, b, e, eb) in rows.items():
        lines.append("%-11s %6d %9d %9d %10d" % (c, n, b, e, eb))
    lines.append("eligible = not excluded and past the cooldown; %d stubs are excluded outright (reasons above)"
                 % sum(1 for s in stubs if s["reason"]))
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exclude-binary", dest="exclude_binaries", action="append", default=[],
                    help="skip units in this binary/overlay (repeatable) - use when a header "
                         "field pass is editing every user of that binary's headers")
    ap.add_argument("--lanes", type=int, default=6)
    ap.add_argument("--queue", type=int, default=6)
    ap.add_argument("--round", type=int)
    ap.add_argument("--kind", choices=["auto", "twin", "park", "fresh", "big", "redo"], default="auto")
    ap.add_argument("--max-bytes", type=int, default=16000,
                    help="drop queue entries from the end until the args JSON fits (default 16000)")
    ap.add_argument("--json", action="store_true", help="print only the JSON object")
    ap.add_argument("--stats", action="store_true")
    ap.add_argument("--dry-run", action="store_true", help="do not record in the history log")
    ap.add_argument("--record", action="store_true", help="record even with --stats/--json (default when not --dry-run)")
    ap.add_argument("--history", help="history log path (default ~/xw/lane-history.tsv or $LANE_HISTORY)")
    args = ap.parse_args()

    hpath = history_path(args)
    history = read_history(hpath)
    round_no = args.round or 1 + max([tracker_round()] + [h[0] for h in history])
    stubs = census()
    rows, clusters, c = twin_for.queue_rows(0.8)
    classify(stubs, rows, clusters, c["items"])
    elig = eligible_stubs(stubs, history, round_no)
    if args.stats:
        print(stats(stubs, elig))
        return
    lanes = plan(args, elig, round_no)
    if not lanes:
        sys.exit("nothing eligible for --kind %s" % args.kind)
    workers = [l[0] for l in lanes[:args.lanes]]
    queue = [l[0] for l in lanes[args.lanes:]]
    obj = {"round": "r%d" % round_no, "workers": workers}
    if queue:
        obj["queue"] = queue
    while queue and len(json.dumps(obj, separators=(",", ":"))) > args.max_bytes:
        queue.pop()
        lanes.pop()
        if not queue:
            del obj["queue"]
        print("# args over --max-bytes %d: dropped a queue entry, %d queued" % (args.max_bytes, len(queue)), file=sys.stderr)
    tags = [l[1] for l in lanes]
    deep = max(w["minutes"] for w in workers)
    # Every lane drains the queue, so the round's deadline is the worst case of the whole
    # backlog spread over the lanes, plus one longest lane of slack.
    total = sum(l[0]["minutes"] for l in lanes)
    options = {"timeoutMs": int((math.ceil(total / len(workers)) + deep) * 60000)}
    if args.json:
        print(json.dumps({"clone_tags": tags, "options": options, "args": obj}, separators=(",", ":")))
    else:
        print("# round %d: %d workers + %d queued%s" % (round_no, len(workers), len(queue),
                                                      "  (dry run, not recorded)" if args.dry_run else ""))
        print("# lanes: " + ", ".join("%s %s %s" % (l[1], l[2], ",".join(f["func"] for f in l[3])) for l in lanes))
        print("# files: " + " ".join(dict.fromkeys(w["file"] for w in workers + queue)))
        print("tools/rounds/clone.sh new " + " ".join(tags))
        if len([w for w in workers if w["minutes"] > 30]) > 4:
            print("# WARNING: more than 4 deep (60+ minute) workers; PROMPT.md caps a deep round at 4 workers")
        print("subagent({workflow: \"tools/rounds/brief.js\", async: true%s, args: <below>})"
              % (", options: %s" % json.dumps(options) if options else ""))
        print(json.dumps(obj, separators=(",", ":")))
    if not args.dry_run and (args.record or not args.stats):
        record(hpath, round_no, lanes)


if __name__ == "__main__":
    main()
