#!/usr/bin/env bash
# Give already-existing clones the orchestrator's generated state, so a reset
# clone can diff immediately instead of rebuilding report mode itself.
#
#   tools/rounds/sync-report.sh <tag>...
#
# Copies build.ninja, objdiff.json, asm/, expected/ and build/ from this tree
# (which must be in report mode, i.e. after `make objdiff-config`) into
# ~/xw/r-<tag>/xenogears-decomp with `cp -ac` (APFS copy-on-write, so this is
# nearly free) after `clone.sh reset`. Ninja paths in build.ninja are relative,
# so a copied tree is valid; only mtimes matter and they are preserved by -c
# ordering (the copy is newer than nothing, so the first diff rebuilds the one
# unit it needs).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
POOL=${POOL:-$HOME/xw}
[ $# -ge 1 ] || { sed -n '2,12p' "$0" >&2; exit 2; }
grep -q -- '-DSKIP_ASM' "$ROOT/build.ninja" 2>/dev/null ||
    { echo "run 'make objdiff-config' here first: clones need report mode" >&2; exit 1; }
for tag in "$@"; do
    d=$POOL/r-$tag/xenogears-decomp
    [ -d "$d" ] || { echo "?? no clone r-$tag" >&2; continue; }
    rm -rf "$d/build" "$d/expected" "$d/asm" "$d/build.ninja" "$d/objdiff.json"
    cp -ac "$ROOT/build" "$ROOT/expected" "$ROOT/asm" "$ROOT/build.ninja" "$ROOT/objdiff.json" "$d/"
    echo "r-$tag synced"
done
