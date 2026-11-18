#!/usr/bin/env bash
# Compile a standalone C snippet with ONE unit's real compiler and flags and print
# the assembly, so a register-allocation question ("which register does this
# spelling put the %hi in?") is answered in ~10 s without editing the unit.
#
#   tools/scripts/cc1_probe.sh <unit> <snippet.c> [--gcc NAME] [--opt -O2] [--gp -G0]
#
# <unit> is a path under src/ without .c, e.g. battle/main/misc35. <snippet.c> is
# a self-contained file (it may `#include "common.h"` and declare externs); it is
# copied into a temp dir under build/ and never enters the ninja graph. cc1 flags
# and the gcc binary come out of build.ninja the way try_flags.sh reads them, so
# this needs a build to have run (`make objdiff-config`). Prints the assembly and
# then one `PROBE <sym> high=<reg> low=<reg>` line per symbol whose address the
# snippet takes (the function name is added when the snippet has several).
set -euo pipefail
[ $# -ge 2 ] || { sed -n '2,14p' "$0" >&2; exit 2; }
unit=$1 snippet=$2; shift 2
gcc=''; opt=''; gp=''
while [ $# -gt 0 ]; do
    case $1 in
        --gcc) gcc=$2; shift ;;
        --opt) opt=$2; shift ;;
        --gp) gp=$2; shift ;;
        *) echo "unknown argument '$1'" >&2; exit 2 ;;
    esac
    shift
done
snippet=$(cd "$(dirname "$snippet")" && pwd)/$(basename "$snippet")
cd "$(dirname "$0")/../.."
[ -f build.ninja ] || { echo "no build.ninja: run 'make objdiff-config' first" >&2; exit 1; }
[ -f "src/$unit.c" ] || { echo "no such unit: src/$unit.c" >&2; exit 1; }
[ -f "$snippet" ] || { echo "no such snippet: $snippet" >&2; exit 1; }

# Same lookups as try_flags.sh: a per-edge override on the cc edge wins over the
# global variable of the same name.
ninja_var() { awk -v v="^$1 = " '$0 ~ v {sub(v, ""); print; exit}' build.ninja; }
edge_var() { awk -v b="build build/src/$unit.c.$2: $3" -v v="^  $1 = " \
    '$0 ~ b {p = 1; next} p && /^build / {exit} p && $0 ~ v {sub(v, ""); print; exit}' build.ninja; }
cpp_flags=$(ninja_var cpp_flags) cc_flags=$(ninja_var cc_flags)
def_gcc=$(edge_var gcc s cc) def_opt=$(edge_var optimization_flag s cc) def_gp=$(edge_var gp_flag s cc)
def_gcc=${def_gcc:-$(ninja_var gcc)} def_opt=${def_opt:-$(ninja_var optimization_flag)}
def_gp=${def_gp:-$(ninja_var gp_flag)}
gcc=${gcc:-$def_gcc} opt=${opt:-$def_opt} gp=${gp:-$def_gp}
[ -n "$gcc" ] || { echo "could not read the compiler for this unit from build.ninja" >&2; exit 1; }

tmp=$(mktemp -d build/.cc1_probe.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
cp "$snippet" "$tmp/snippet.c"
echo "# probe: $unit -> $gcc $opt $gp" >&2
# No -S: cc1 2.6.0/2.7.2 reject it with "Invalid option"; cc1 only ever emits
# assembly, and the unit's own -fverbose-asm keeps the output readable.
# stdin </dev/null: maspsx and cc1 both read stdin to EOF when it is a pipe.
docker run --rm -i --platform=linux/amd64 -v"$PWD":/xenogears-decomp ethos:latest /bin/bash -c '
    set -e
    mips-linux-gnu-cpp -Iinclude -I build $1 -DSKIP_ASM -o "$5/snippet.i" "$5/snippet.c"
    "tools/$2/cc1" $3 $4 $6 -o "$5/snippet.s" "$5/snippet.i"' -- \
    "$cpp_flags" "$gcc" "$opt" "$gp" "$tmp" "$cc_flags" </dev/null \
    || { echo "compile failed (above); colima up? gcc '$gcc' present in tools/?" >&2; exit 1; }
[ -f "$tmp/snippet.s" ] || { echo "cc1 produced no assembly" >&2; exit 1; }
cat "$tmp/snippet.s"

# The summary: the register each %hi and its %lo landed in, one line per address
# the snippet takes. A %hi comes from `lui $r,%hi(sym)`; a %lo either as an
# addiu's destination (`addiu $r,$s,%lo`, $r then holds the address) or as a
# memory displacement's base (`sw $v0,%lo(sym)($r)`, the -G0 tie shape).
python3 - "$tmp/snippet.s" <<'PROBE_EOF'
import re, sys
# One entry per address the snippet takes: cc1 emits a %hi for each of them and
# the %lo(s) that follow reuse that high half, so the %hi starts a new entry.
occ, funcs, func = [], [], ""
def take(f, sym, reg):
    occ.append([f, sym, reg, []])
def low(f, sym, reg):
    for o in reversed(occ):
        if o[0] == f and o[1] == sym:
            if reg not in o[3]:
                o[3].append(reg)
            return
    occ.append([f, sym, "-", [reg]])
def sym(s):
    return re.split(r'[+-]', s.strip())[0]
def label(o):
    return "%s %s" % (o[0], o[1]) if len(funcs) > 1 else o[1]
for line in open(sys.argv[1]):
    m = re.match(r'\s*\.ent\s+(\S+)', line)
    if m:
        func = m.group(1)
        funcs.append(func)
        continue
    dest = re.match(r'\s*\w+\s+(\$\w+)', line)
    m = re.search(r'%hi\(([^)]*)\)', line)
    if m and dest:
        take(func, sym(m.group(1)), dest.group(1))
    # gcc 2.6.0 emits `la $r,sym` instead of %hi/%lo; gas then puts both halves
    # in $r, so that is the register to report.
    m = re.match(r'\s*la\s+(\$\w+)\s*,\s*(\w+)\s*$', line)
    if m:
        take(func, sym(m.group(2)), m.group(1))
        low(func, sym(m.group(2)), m.group(1))
    for m in re.finditer(r'%lo\(([^)]*)\)(?:\((\$\w+)\))?', line):
        reg = m.group(2)                     # displacement form: base holds the low half
        if not reg:
            reg = dest.group(1) if dest else "-"  # addiu $d,$s,%lo -> $d holds it
        low(func, sym(m.group(1)), reg)
for o in occ:
    print("PROBE %s high=%s low=%s" % (label(o), o[2], ",".join(o[3]) or "-"))
if not occ:
    sys.stderr.write("# no %hi/%lo/la in this output: the compiler emitted bare "
                     "`lw sym` pseudo-ops, so the assembler picks the registers\n")
PROBE_EOF
