#!/usr/bin/env python3
"""Score build/.try_flags.o against the expected object for one symbol."""
import json
import subprocess
import sys

unit, func, obj = sys.argv[1], sys.argv[2], sys.argv[3]
out = subprocess.run(
    ["tools/objdiff/objdiff-cli", "diff", "-1", "expected/asm/%s.s.o" % unit,
     "-2", obj, "-o", "-", "--format", "json", func],
    capture_output=True, text=True).stdout
d = json.loads(out)
syms = [s for s in d.get("right", {}).get("symbols", []) if s.get("name") == func]
if not syms or "match_percent" not in syms[0]:
    print("absent")
else:
    print("%.2f%% size=%s" % (syms[0]["match_percent"], syms[0].get("size")))
