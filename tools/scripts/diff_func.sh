#!/usr/bin/env bash
# Diff one function: original disassembly (expected/asm/<unit>.s.o) vs what the
# compiler produces from our C (build/src/<unit>.c.o).
#
#   tools/scripts/diff_func.sh <unit> <function>
#   tools/scripts/diff_func.sh field/main/misc3 FieldSetupFadeTimer
#
# Prerequisite: `make objdiff-config` has been run once since the last `make build`
# or `make clean`. It builds everything in "report" mode and leaves the original
# objects in expected/asm/. This script then rebuilds just the one unit with ninja
# and shows a side-by-side diff. Exit 0 and "MATCH" when the disassembly is identical.
#
# If tools/objdiff/objdiff-cli (a native build of https://github.com/encounter/objdiff)
# is present, its fuzzy match percentage for the function is printed as well.
#
# Off linux/x86_64 the build and objdump steps run inside the build container.
set -euo pipefail

[ $# -eq 2 ] || { sed -n '2,13p' "$0" >&2; exit 2; }
unit=$1
func=$2
cd "$(dirname "$0")/../.."

target=expected/asm/$unit.s.o
base=build/src/$unit.c.o

# Print one function's instructions, stripped of absolute offsets so that the two
# objects compare even though the function sits at different addresses in each.
dump() {
    mips-linux-gnu-objdump -d -r --no-show-raw-insn "$1" \
        | awk -v f="<$2>:" '$0 ~ f {p=1; next} p && /^[0-9a-f]+ <.*>:$/ {exit} p' \
        | sed -E 's/^ *[0-9a-f]+:\t//; s/^\t+//; s/^[0-9a-f]+: (R_MIPS)/\1/; s/\b0 <[^>]+>$//; s/[[:space:]][0-9a-f]+ <([^>]+)>/ <\1>/g; s/,[0-9a-f]+ </,</g' || true
}

# Inside the container (or on linux/x86_64): build the unit and emit the diff.
if [ "$(uname -s)-$(uname -m)" = "Linux-x86_64" ]; then
    [ -f "$target" ] || { echo "missing $target: run 'make objdiff-config' first" >&2; exit 1; }
    grep -q -- '-DSKIP_ASM' build.ninja || { echo "build.ninja is not in report mode: run 'make objdiff-config' first" >&2; exit 1; }
    ninja "$base" >/dev/null

    left=$(dump "$target" "$func")
    right=$(dump "$base" "$func")
    [ -n "$left" ] || { echo "$func not found in $target" >&2; exit 1; }
    if [ -z "$right" ]; then
        echo "$func is not defined in $base (still INCLUDE_ASM?)" >&2
        printf '%s\n' "$left"
        exit 1
    fi
    if diff --side-by-side --width="${COLUMNS:-160}" <(printf '%s\n' "$left") <(printf '%s\n' "$right"); then
        echo "MATCH: $func ($(printf '%s\n' "$left" | grep -vc R_MIPS) instructions)"
        exit 0
    fi
    echo "MISMATCH: $func  (left = original, right = compiled)"
    exit 1
fi

# On the host: run the above in the container, then add objdiff's score if we have it.
status=0
docker run --rm -i --platform=linux/amd64 -v"$PWD":/xenogears-decomp -e COLUMNS="${COLUMNS:-160}" ethos:latest \
    /bin/bash -c ". /.venv/bin/activate && tools/scripts/diff_func.sh $(printf '%q ' "$@")" || status=$?

objdiff=tools/objdiff/objdiff-cli
if [ -x "$objdiff" ] && [ -f "$base" ]; then
    "$objdiff" diff -1 "$target" -2 "$base" -o - --format json "$func" 2>/dev/null \
        | python3 -c '
import json, sys
func = sys.argv[1]
d = json.load(sys.stdin)
for s in d.get("right", {}).get("symbols", []):
    if s.get("name") == func:
        print("objdiff: %s %.2f%% (%s bytes)" % (func, s.get("match_percent", 0.0), s.get("size", "?")))
        break
else:
    print("objdiff: %s not present in compiled object" % func)
' "$func" || true
fi
exit $status
