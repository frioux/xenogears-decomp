#!/usr/bin/env bash
# Score ONE function under alternative compiler / flag combinations, without
# re-running gears. Answers "is this unit on the wrong preset?" in seconds per
# candidate instead of ~35 s.
#
#   tools/scripts/try_flags.sh <unit> <Func>                  # default preset
#   tools/scripts/try_flags.sh <unit> <Func> --sweep           # gcc x -mcpu x -O x -G
#   tools/scripts/try_flags.sh <unit> --all [--gcc ...]        # EVERY C function of
#                                                              # the unit, one compile
#   tools/scripts/try_flags.sh <unit> <Func> --cpu r8000
#   tools/scripts/try_flags.sh <unit> <Func> --gcc gcc-2.7.2-cdk-psx --opt -O3
#
# The default row is exactly what build.ninja does for that unit, because the
# flags are parsed out of build.ninja. Requires report mode
# (`make objdiff-config`) for the reference object in expected/asm/.
# Prints `<variant> <match %>` per candidate and exits 0 if one is 100.
set -euo pipefail
[ $# -ge 2 ] || { sed -n '2,16p' "$0" >&2; exit 2; }
unit=$1 func=$2; shift 2
all=0; [ "$func" = "--all" ] && { all=1; func=; }
gcc=''; opt=''; gp=''; cpu=''; maspsx=''; sweep=0
while [ $# -gt 0 ]; do
    case $1 in
        --sweep) sweep=1 ;;
        --all) all=1 ;;
        --gcc) gcc=$2; shift ;;
        --opt) opt=$2; shift ;;
        --gp) gp=$2; shift ;;
        --cpu) cpu=$2; shift ;;
        --maspsx) maspsx=$2; shift ;;
        *) echo "unknown argument '$1'" >&2; exit 2 ;;
    esac
    shift
done
cd "$(dirname "$0")/../.."
ref=expected/asm/$unit.s.o
[ -f "$ref" ] || { echo "missing $ref: run 'make objdiff-config' first" >&2; exit 1; }
[ -f "src/$unit.c" ] || { echo "no such unit: src/$unit.c" >&2; exit 1; }

# Defaults straight from the generated build.ninja.
ninja_var() { awk -v v="^$1 = " '$0 ~ v {sub(v, ""); print; exit}' build.ninja; }
# Per-edge overrides: the preset puts gcc/opt/gp on the cc edge and maspsx_flags on
# the maspsx edge, so a rule name is part of the lookup (looking for maspsx_flags on
# the cc edge silently falls back to Default's flags and mis-scores every
# --dont-expand-li unit, e.g. battle/misc28 and misc34).
edge_var() { awk -v b="build build/src/$unit.c.$2: $3" -v v="^  $1 = " \
    '$0 ~ b {p = 1; next} p && /^build / {exit} p && $0 ~ v {sub(v, ""); print; exit}' build.ninja; }
cpp_flags=$(ninja_var cpp_flags) cc_flags=$(ninja_var cc_flags) endian=$(ninja_var endianness)
def_gcc=$(edge_var gcc s cc) def_msx=$(edge_var maspsx_flags o maspsx)
def_gcc=${def_gcc:-$(ninja_var gcc)} def_msx=${def_msx:-$(ninja_var maspsx_flags)}
def_opt=$(edge_var optimization_flag s cc) def_opt=${def_opt:-$(ninja_var optimization_flag)}
def_gp=$(edge_var gp_flag s cc) def_gp=${def_gp:-$(ninja_var gp_flag)}
[ -n "$def_gcc" ] || { echo "could not read the compiler for this unit from build.ninja" >&2; exit 1; }

if [ $sweep -eq 1 ]; then
    gccs=($(cd tools && ls -d gcc-*-psx))
    opts=(-O1 -O2 -O3) gps=(-G4 -G8) cpus=(3000 r8000)
else
    gccs=(${gcc:-$def_gcc}) opts=(${opt:-$def_opt}) gps=(${gp:-$def_gp}) cpus=(${cpu:-3000})
fi
march() { case $1 in r*) echo $1 ;; *) echo "r$1" ;; esac; }

obj=build/.try_flags.o
compile() { # gcc opt gp cpu maspsx
    docker run --rm -i --platform=linux/amd64 -v"$PWD":/xenogears-decomp ethos:latest /bin/bash -c '
        . /.venv/bin/activate
        set -e
        mips-linux-gnu-cpp -Iinclude -I build -DSKIP_ASM $1 -o build/.try_flags.i "src/$2.c"
        "tools/$3/cc1" $4 $5 $(echo $6 | sed "s/-mcpu=[^ ]*//") -o build/.try_flags.s build/.try_flags.i
        python3 tools/maspsx/maspsx.py $7 $8 -Iinclude -I build $4 $5 -march=$9 -mtune=$9 \
            -no-pad-sections -o build/.try_flags.o build/.try_flags.s >/dev/null 2>&1' -- \
        "$cpp_flags" "$unit" "$1" "$2" "$3" "$cc_flags" "$5" "$endian" "$(march "$4")" \
        2>/dev/null || return 1
}
score() {   # $1 = json path
    if [ $all -eq 1 ]; then
        tools/objdiff/objdiff-cli diff -1 "$ref" -2 "$obj" -o - --format json \
            >"$1" 2>/dev/null || true
        python3 -c 'import json,sys
d = json.load(open(sys.argv[1]))
for s in d.get("right", {}).get("symbols", []):
    if not s.get("name"):
        continue
    if "match_percent" in s and s.get("size"):
        print("%7.2f %s" % (s["match_percent"], s["name"]))
    else:
        print("     -- %s" % s["name"])' "$1"
        return
    fi
    tools/objdiff/objdiff-cli diff -1 "$ref" -2 "$obj" -o - --format json "$func" \
        >"$1" 2>/dev/null || true
    python3 -c 'import json,sys
try:
    d = json.load(open(sys.argv[1]))
    syms = [s for s in d.get("right", {}).get("symbols", []) if s.get("name") == sys.argv[2]]
    # "no C body" shows up as a right-side symbol with no size/match_percent
    if not syms or "match_percent" not in syms[0] or not syms[0].get("size"):
        print("not-in-C (still INCLUDE_ASM?)")
    else:
        print(round(syms[0]["match_percent"], 2))
except Exception:
    print("absent")' "$1" "$func"
}

status=1
for g in "${gccs[@]}"; do for o in "${opts[@]}"; do for p in "${gps[@]}"; do for c in "${cpus[@]}"; do
    rm -f "$obj"
    if compile "$g" "$o" "$p" "$c" "${maspsx:-$def_msx}" && [ -f "$obj" ]; then pct=$(score build/.try_flags.json)
    else pct="compile-failed"; fi
    printf '%-30s %s\n' "${g%-psx} $o $p -mcpu=$c" "$pct"
    [ "$pct" = "100.0" ] && status=0
    [ $all -eq 1 ] && status=0
done; done; done; done
rm -f build/.try_flags.i build/.try_flags.s build/.try_flags.o build/.try_flags.json
exit $status
