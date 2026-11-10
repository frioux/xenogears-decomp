#!/usr/bin/env bash
# Pre-flight a unit list BEFORE briefing workers.
#
#   tools/scripts/preflight.sh world_map/main/misc7 battle/main/misc31 menu/main/misc4
#
# Prints, per unit: the maspsx flag the unit's retail object implies (the
# addiu-vs-ori count), how many stubs are linkable at all, how many are left, and
# the cheapest stubs that have never been attempted.
#
# Why this exists: briefing nine `battle` units as "Default preset" when their
# objects are addiu-dominant cost a whole round (reports/TRACKER.md, round 43).
# Two greps would have caught it; this script is those two greps plus the two
# other checks that keep being forgotten after a carve.
set -uo pipefail
[ $# -ge 1 ] || { sed -n '2,12p' "$0" >&2; exit 2; }
cd "$(dirname "$0")/../.."

for u in "$@"; do
    bin=${u%%/*}
    rel=${u#*/}                      # unit path without the binary prefix
    dir=asm/$bin/nonmatchings/$rel
    [ -d "$dir" ] || { echo "$u: no $dir (wrong unit path?)"; continue; }
    addiu=$(grep -rho 'addiu *\$[a-z0-9]*, *\$zero, *0x[0-9A-Fa-f]*' "$dir" | wc -l | tr -d ' ')
    ori=$(grep -rho 'ori *\$[a-z0-9]*, *\$zero, *0x[0-9A-Fa-f]*' "$dir" | wc -l | tr -d ' ')
    stubs=$(grep -rho '^INCLUDE_ASM' "src/$u.c" | wc -l | tr -d ' ')
    blocked=$(python3 tools/scripts/convertible.py "$u" 2>/dev/null | grep -c BLOCKED || true)
    if [ "$addiu" -gt "$ori" ]; then
        flag="ADDIU-dominant ($addiu vs $ori) -> needs --dont-expand-li (XenoWorldMap / XenoBattleTail style presets)"
    else
        flag="ori-dominant ($ori vs $addiu) -> Default preset is right"
    fi
    printf '%-26s stubs=%-3s blocked=%-3s %s\n' "$u" "$stubs" "$blocked" "$flag"
    # cheapest stubs by size, as a starting list. Whether each already has a parked
    # attempt has to be read in the file: the window check is not reliable, and a
    # wrong "unattempted" label sends a worker at a parked 98% function.
    for f in $(ls -S -r "$dir"/*.s 2>/dev/null | head -40); do
        echo "    cheap: $(basename "$f" .s)"
    done | head -5
done

# AT-FREE stubs: a stub whose original does a one-instruction load or store off an
# absolute symbol using a base register that is neither $at nor (for a load) the
# destination cannot be matched by any preset. gcc emits the one-instruction pseudo
# (`lw $d,D_x` / `sw $x,D_x`); our assembler expands it as `lui $at,%hi; op ..,%lo($at)`
# for stores and `lui $d,%hi; lw $d,%lo($d)` for loads, while the ASPSX that built
# battle/main/{misc6,misc26..misc35} picked an arbitrary free register. Across every
# other unit's originals `%lo(sym)($r)` uses $at 1657 times and a real register once,
# so it is an assembler-revision ceiling: no gcc version, -O level or -G setting moves
# it (TRACKER round 10 has the ten-shape experiment). `lui $r,%hi` + `addiu $r,$r,%lo`
# (la-style) and function calls are fine, which is why those stubs can still match.
for u in "$@"; do
    dir=asm/${u%%/*}/nonmatchings/${u#*/}
    [ -d "$dir" ] || continue
    python3 tools/scripts/atfree.py "$u" "$dir"
done
