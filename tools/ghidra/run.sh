#!/bin/sh
# Headless Ghidra analysis + facts export for the Xenogears binaries.
#
#   tools/ghidra/run.sh [bin ...]     (default: all eleven)
#
# Needs build/out/<bin>.elf (a normal build).  Output goes to
# ${GHIDRA_OUT:-$HOME/xw/ghidra-out}/<bin>/ (see tools/scripts/ghidra_info.py).
# Env: GHIDRA_HOME (default /opt/homebrew/opt/ghidra/libexec), JAVA_HOME,
# GHIDRA_JOBS (parallel binaries, default 4).
set -eu
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
OUT=${GHIDRA_OUT:-$HOME/xw/ghidra-out}
GHIDRA_HOME=${GHIDRA_HOME:-/opt/homebrew/opt/ghidra/libexec}
ALL="slus_006.64 field battle battling world_map movie menu member_change_menu shop_menu gear_shop_menu lib_gear"

if [ "${1:-}" = "--one" ]; then
    bin=$2
    proj="$OUT/_proj/$bin"
    log="$OUT/_logs/$bin.log"
    t0=$(date +%s)
    rm -rf "$OUT/$bin" "$proj"
    mkdir -p "$proj" "$OUT/_logs"
    printf 'xg\n' > "$proj/dummy.bin"
    slus="$repo/build/out/slus_006.64.elf"
    [ "$bin" = slus_006.64 ] && slus=-
    "$GHIDRA_HOME/support/analyzeHeadless" "$proj" xg \
        -import "$proj/dummy.bin" -loader BinaryLoader -processor MIPS:LE:32:default \
        -scriptPath "$here" \
        -preScript XgSetup.java "$bin" "$repo/build/out/$bin.elf" "$slus" \
        -postScript XgExport.java "$OUT" \
        -deleteProject > "$log" 2>&1 || { echo "FAILED $bin (see $log)"; tail -20 "$log"; exit 1; }
    grep -h '^INFO  XgSetup\|XGSUMMARY' "$log" | sed 's/^INFO  //; s/ (GhidraScript)//' > "$OUT/_logs/$bin.summary"
    grep -q XGSUMMARY "$OUT/_logs/$bin.summary" || { echo "FAILED $bin: no summary (see $log)"; tail -20 "$log"; exit 1; }
    echo "$(sed -n 's/.*XGSUMMARY /XGSUMMARY /p' "$OUT/_logs/$bin.summary") total_seconds=$(( $(date +%s) - t0 ))"
    exit 0
fi

if [ -z "${JAVA_HOME:-}" ] && ! command -v java >/dev/null 2>&1 || ! java -version >/dev/null 2>&1; then
    for j in /opt/homebrew/opt/openjdk@25 /opt/homebrew/opt/openjdk@21 /opt/homebrew/opt/openjdk; do
        [ -x "$j/bin/java" ] && { JAVA_HOME=$j; export JAVA_HOME; break; }
    done
fi
[ -x "$GHIDRA_HOME/support/analyzeHeadless" ] || { echo "no Ghidra at $GHIDRA_HOME (set GHIDRA_HOME)" >&2; exit 1; }
bins=${*:-$ALL}
for b in $bins; do
    [ -f "$repo/build/out/$b.elf" ] || { echo "missing $repo/build/out/$b.elf (build first)" >&2; exit 1; }
done
mkdir -p "$OUT"
export OUT GHIDRA_HOME JAVA_HOME="${JAVA_HOME:-}"
# slus first so the biggest job starts early; every program is independent
printf '%s\n' $bins | xargs -P "${GHIDRA_JOBS:-4}" -I{} sh "$here/run.sh" --one {}
