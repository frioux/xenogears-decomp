#!/usr/bin/env bash
# Byte-check ONE output against config/checksum.sha.
#
#   tools/scripts/check_bin.sh field
#
# Why: objdiff compares object files, so it cannot report 100% for a function
# whose only difference is a relocation the linker resolves identically -- in
# this repo that is a `switch` compiled to a section-relative .rodata jump table
# where the original names a `jtbl_` symbol. Those functions cap at ~99.7%
# forever. The linked bytes are the real gate (that is what `make check` uses),
# so this rebuilds in matching mode and compares the one output you care about.
#
# Cost: a full clean build of all six outputs (~60-90 s). That is the price of
# certainty; use it at the end of a batch, not in the inner loop.
#
# It leaves the tree in MATCHING mode, so `diff_func.sh` stops working until you
# run `make objdiff-config` again. Prints "OK" or the first differing byte.
set -euo pipefail
[ $# -eq 1 ] || { sed -n '2,18p' "$0" >&2; exit 2; }
name=$1
cd "$(dirname "$0")/../.."

case $name in
    slus_006.64) out=build/out/slus_006.64 ;;
    field | battling | world_map | movie | member_change_menu | shop_menu | gear_shop_menu | lib_gear)
        out=build/out/$name.bin ;;
    *)
        echo "unknown output '$name' (want slus_006.64, field, battling, world_map, movie, member_change_menu, shop_menu, gear_shop_menu, lib_gear)" >&2
        exit 2
        ;;
esac

make build >/dev/null
want=$(awk -v o="$out" '$2 == o {print $1}' config/checksum.sha)
got=$({ sha256sum "$out" 2>/dev/null || shasum -a 256 "$out"; } | cut -d' ' -f1)
if [ "$want" = "$got" ]; then
    echo "$out: OK"
else
    echo "$out: MISMATCH"
    echo "  want $want"
    echo "  got  $got"
    exit 1
fi
