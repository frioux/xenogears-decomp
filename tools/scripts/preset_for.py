#!/usr/bin/env python3
"""Print the gears.toml preset a unit actually builds under.

    tools/scripts/preset_for.py battle/main/misc13 menu/main/misc9

The lookup is the one `gears` does - last preset whose `paths` lists the unit
wins, because later presets overwrite earlier ones - and the `paths` array is
read in full, multi-line included. Getting this wrong by hand cost two rounds:
round 171 told a worker `battle/misc19` was XenoBattle260 (it is Default) and
round 172 nearly repeated it for `battle/misc13`.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def presets():
    text = (ROOT / "gears.toml").read_text()
    for block in re.split(r"\[\[build_preset\]\]", text)[1:]:
        name = re.search(r'name\s*=\s*"([^"]+)"', block).group(1)
        at = block.find("paths")
        if at < 0:
            paths = []
        else:
            start = block.find("[", at)
            paths = re.findall(r'"([^"]+)"', block[start:block.find("]", start) + 1])
        body = re.search(r"build_variables\s*=\s*\{(.*?)\}", block, re.S)
        variables = " ".join(body.group(1).split()) if body else ""
        yield name, paths, variables


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    table = list(presets())
    for unit in sys.argv[1:]:
        # slus_006.64 presets are keyed by the path under the binary
        # (`system/sound.`), overlays by the whole thing (`battle/main/misc13.`).
        stripped = unit.split("/", 1)[1] if unit.count("/") > 1 else unit
        binary = unit.split("/", 1)[0]
        hit = (None, "")
        for name, paths, variables in table:
            # a path may be written with a `{binary}` placeholder, and it is a
            # prefix ending at a '.' - `battle/main/misc1.` must not claim misc13
            paths = [p.replace("{binary}", binary) for p in paths]
            if any((unit + ".").startswith(p) or (stripped + ".").startswith(p) for p in paths):
                hit = (name, variables)
        name, variables = hit if hit[0] else ("Default", "")
        print(f"{unit}: {name}\n    {variables or 'inherits Default (gcc-2.7.2-psx, -O2 -G8)'}")


if __name__ == "__main__":
    main()
