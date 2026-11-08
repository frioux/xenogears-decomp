#!/usr/bin/env bash
# When `make check` fails, say WHICH functions are wrong.
#
#   tools/scripts/culprit.sh field
#
# `cmp`s the built output against the extracted original, turns the differing
# byte offsets into addresses using the load address in linker/<bin>.ld, and
# prints how many differing bytes fall in each function (from
# config/symbol_addrs.<bin>.txt). "make check failed" without this means
# bisecting six binaries by hand.
set -euo pipefail
[ $# -eq 1 ] || { sed -n '2,9p' "$0" >&2; exit 2; }
bin=$1
cd "$(dirname "$0")/../.."
built=build/out/$bin.bin out=disc/$bin.bin
if [ "$bin" = slus_006.64 ]; then built=build/out/slus_006.64 out=disc/slus_006.64; fi
[ -f "$built" ] || { echo "missing $built: run a build first" >&2; exit 1; }
[ -f "$out" ] || { echo "missing $out: run 'make extract' first" >&2; exit 1; }

base=$(awk '/^[[:space:]]*\.[A-Za-z_]+[[:space:]]+0x8[0-9A-Fa-f]+/ {print $2; exit}' linker/$bin.ld)
[ -n "$base" ] || { echo "could not read the load address from linker/$bin.ld" >&2; exit 1; }

cmp -l "$built" "$out" 2>/dev/null | awk '{print $1 - 1}' |
    python3 -c '
import sys

base = int(sys.argv[1], 16)
syms = []
for line in open("config/symbol_addrs." + sys.argv[2] + ".txt"):
    if "=" not in line:
        continue
    name, _, rest = line.partition("=")
    try:
        addr = int(rest.split(";")[0].strip().rstrip(","), 16)
    except ValueError:
        continue
    if 0x80000000 <= addr < 0x80200000:
        syms.append((addr, name.strip()))
syms.sort()
addrs = [a for a, _ in syms]


def owner(addr):
    """Name of the last symbol at or before addr."""
    if not addrs or addr < addrs[0]:
        return "(before first symbol)"
    lo, hi = 0, len(addrs) - 1
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if addrs[mid] <= addr:
            lo = mid
        else:
            hi = mid - 1
    return syms[lo][1]


runs = []
for line in sys.stdin:
    off = int(line)
    if runs and off == runs[-1][1] + 1:
        runs[-1][1] = off
    else:
        runs.append([off, off])

per = {}
for a, b in runs:
    name = owner(base + a)
    n, lo, hi = per.get(name, (0, a, b))
    per[name] = (n + b - a + 1, min(lo, a), max(hi, b))

total = sum(b - a + 1 for a, b in runs)
for name, (n, lo, hi) in sorted(per.items(), key=lambda kv: -kv[1][0]):
    print("%-34s %7d bytes  0x%08X-0x%08X" % (name, n, base + lo, base + hi))
print("-- %d differing byte(s) in %d symbol(s), %d run(s)" % (total, len(per), len(runs)))
' "$base" "$bin"
