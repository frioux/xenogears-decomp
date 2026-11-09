#!/usr/bin/env python3
"""Name `func_`/stub symbols in src/slus_006.64/psyq/ by matching them against the
real Psy-Q SDK library objects.

The SDK trees are public (github.com/sozud/psy-q: versions 3.3, 3.5, 3.6, 4.0,
`<ver>/PSX/LIB/*.LIB`), and their objects keep their symbol tables. `psy-k`
(https://github.com/ttkb-oss/psy-k) reads them. Input here is a "sig dump": one
line per exported function, `ARCHIVE/MODULE.OBJ <TAB> NAME <TAB> <hex words>`,
where a word is 8 hex chars and a word the linker relocates is 0xDEADBEEF.

Producing that dump (see also reports/TRACKER.md "Naming"):

    cargo install --path psy-k-bin          # or build it in a checkout of psy-k
    psyk extract LIBGPU.LIB                 # inside a dir holding the .LIB
    # a ~40-line Rust bin over the psy-k API walks obj.sections(), tracking
    # SectionSwitch to concatenate each section's Code blocks, marks every
    # Section::Patch offset as 0xDEADBEEF, and prints each XDEF
    # (section, offset, name) as `name <TAB> words...` sliced to the next XDEF.

Then:

    tools/scripts/psyq_names.py sig33.txt sig35.txt sig36.txt sig40.txt

It prints, per function in this repo, every library version that matched and the
name it matched. A rename is only trustworthy when the name is the same in ALL
versions given (that is what separates NormalColorCol from a guess) and at least
half of the matched words are unpatched -- functions whose retail form differs
from the library's only in relocated words (the libetc interrupt callbacks, most
Cd* stubs) look identical to several siblings and must be skipped.

Nothing is renamed automatically; read the output and edit symbol_addrs by hand.
"""
import collections
import glob
import os
import re
import sys

# which SDK archive each src/slus_006.64/psyq/<dir> unit can have come from
FAMILY = {
    'libgte': 'LIBGTE', 'libgpu': 'LIBGPU', 'libapi': 'LIBAPI', 'libcard': 'LIBCARD',
    'libspu': 'LIBSPU', 'libc': 'LIBC', 'libcd': 'LIBCD', 'libetc': 'LIBETC',
    'libsn': 'LIBSN', 'libc2': 'LIBC2', 'libmcrd': 'LIBMCRD', 'libpress': 'LIBPRESS',
    'libds': 'LIBDS', 'libsio': 'LIBSIO', 'libtap': 'LIBTAP', 'libgun': 'LIBGUN',
    'libcomb': 'LIBCOMB', 'libmath': 'LIBMATH',
    # units split out of raw asm segments in session 4
    'libgpu_b': 'LIBGPU', 'libgpu_c': 'LIBGPU', 'libgpu_tex': 'LIBGPU',
    'libgte_c': 'LIBGTE', 'libapi_b': 'LIBAPI', 'libcard_b': 'LIBCARD',
}
PATCHED = 0xDEADBEEF


def load(path):
    lib = collections.defaultdict(list)
    for line in open(path):
        obj, name, hexs = line.rstrip('\n').split('\t')
        words = [int(hexs[i:i + 8], 16) for i in range(0, len(hexs), 8)]
        if words:
            lib[len(words)].append((obj, name, words))
    return lib


def parse_s(path):
    """Instruction words of a splat .s, byte-swapped to host order.

    The .s prints the raw bytes in memory order, so the 8 hex characters are the
    little-endian encoding of the word.
    """
    out = []
    for line in open(path):
        m = re.search(r'/\*\s*[0-9A-F]+\s+[0-9A-F]+\s+([0-9A-F]{8})\s*\*/', line)
        if m:
            out.append(int.from_bytes(int(m.group(1), 16).to_bytes(4, 'big'), 'little'))
    return out


def main(argv):
    if not argv:
        sys.exit(__doc__)
    libs = {os.path.basename(p): load(p) for p in argv}
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    for path in sorted(glob.glob(root + '/asm/slus_006.64/*/psyq/**/*.s', recursive=True)):
        func = os.path.basename(path)[:-2]
        words = parse_s(path)
        if not words or func.startswith('D_'):
            continue
        want = FAMILY.get(path.split('/psyq/')[1].split('/')[0])
        found = []
        for ver, lib in libs.items():
            best = []
            for obj, name, sig in lib.get(len(words), []):
                if want and not obj.startswith(want + '/'):
                    continue
                real = sum(1 for w in sig if w != PATCHED)
                if real < 2:
                    continue
                if all((a == PATCHED) or (b == PATCHED) or (a == b) for a, b in zip(words, sig)):
                    best.append((real, obj, name))
            if not best:
                continue
            best.sort(reverse=True)
            top = best[0][0]
            ties = [b[1:] for b in best if b[0] == top]
            name = ties[0][1] if len(ties) == 1 else '?' + '|'.join(t[1] for t in ties)
            found.append('%s:%s:%s:%d' % (ver, name, ties[0][0], top))
        if found:
            print('%s\t%d\t%s' % (func, len(words), ';'.join(found)))


if __name__ == '__main__':
    main(sys.argv[1:])
