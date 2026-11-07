#!/usr/bin/env python3
"""Check that every file in disc/ referenced by a splat config matches the
sha1 recorded in that config. Exit status is non-zero on any mismatch."""

import glob
import hashlib
import os
import sys

import yaml

SKIP = {"overlay-template.yaml"}


def main():
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    bad = 0
    for cfg in sorted(glob.glob(os.path.join(root, "config", "*.yaml"))):
        if os.path.basename(cfg) in SKIP:
            continue
        with open(cfg) as fh:
            spec = yaml.safe_load(fh)
        target = os.path.join(root, spec["options"]["target_path"])
        want = spec.get("sha1")
        if not os.path.exists(target):
            print(f"MISSING  {spec['options']['target_path']}  (from {os.path.basename(cfg)})")
            bad += 1
            continue
        with open(target, "rb") as fh:
            got = hashlib.sha1(fh.read()).hexdigest()
        status = "OK      " if got == want else "MISMATCH"
        bad += got != want
        print(f"{status} {spec['options']['target_path']}  {got}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
