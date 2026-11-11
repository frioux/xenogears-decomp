#!/usr/bin/env bash
# Score every C function of a unit under one alternative preset.
#
#   tools/scripts/screen_unit.sh battle/main/misc28 --gcc gcc-2.7.2-cdk-psx
#
# A unit carries exactly one preset (gears.toml keys it by path), so the answer
# this buys is which functions of a *function-mixed* unit need their own file -
# the split recipe is the misc27 note in gears.toml. Wraps try_flags.sh (one
# compile per function) so the two tools cannot disagree about flags.
set -uo pipefail
[ $# -ge 1 ] || { sed -n '2,8p' "$0" >&2; exit 2; }
unit=$1; shift
cd "$(dirname "$0")/../.."
[ -f "src/$unit.c" ] || { echo "no such unit: src/$unit.c" >&2; exit 1; }
funcs=$(python3 - "src/$unit.c" <<'PY'
import re, sys
src = open(sys.argv[1]).read()
stubs = set(re.findall(r'INCLUDE_ASM\(\s*"[^"]+"\s*,\s*(\w+)\s*\)', src))
seen = set()
for m in re.finditer(r'^[A-Za-z_][\w \t*]*?\b(\w+)\s*\([^;]*\)\s*\{', src, re.M):
    name = m.group(1)
    if name not in stubs and name not in seen:
        seen.add(name)
        print(name)
PY
)
[ -n "$funcs" ] || { echo "no C functions in src/$unit.c" >&2; exit 1; }
printf '%s\n' "$funcs" | while read -r f; do
    for try in 1 2 3; do
        pct=$(tools/scripts/try_flags.sh "$unit" "$f" "$@" </dev/null 2>/dev/null | tail -1 | awk '{print $NF}') || true
        [ "$pct" = compile-failed ] || break
        sleep 3   # try_flags fails transiently under container churn; re-run the row
    done
    printf '%-34s %s\n' "$f" "${pct:-compile-failed}"
done
