#!/usr/bin/env bash
# Run decomp-permuter on one function (a parked body or a live near-miss).
#
#   tools/scripts/permute.sh <unit> <Func> [--minutes N] [-j N] [--keep N]
#   tools/scripts/permute.sh battle/main/misc8 func_8007CDD0 --minutes 10
#
# Prints `RESULT PERMUTER_ZERO|PERMUTER_BETTER|PERMUTER_NONE ... base=<score> best=<score>`
# and the diff of the best candidate. Work files land in permuter/<Func>/ (gitignored).
# Needs report mode (`make objdiff-config`). See tools/scripts/permute.py for the details.
#
# The permuter itself is fetched once into tools/decomp-permuter (gitignored) at a pinned
# commit; clones made with tools/rounds/clone.sh inherit it.
set -euo pipefail
[ $# -ge 2 ] || { sed -n '2,12p' "$0" >&2; exit 2; }
cd "$(dirname "$0")/../.."

PIN=860b6d9
tool=tools/decomp-permuter
if [ ! -d "$tool/src" ]; then
    git clone -q https://github.com/simonlindholm/decomp-permuter "$tool"
    git -C "$tool" checkout -q "$PIN" 2>/dev/null || echo "permute: pinned commit $PIN not found, using HEAD" >&2
fi

if [ "$(uname -s)-$(uname -m)" = "Linux-x86_64" ]; then
    . /.venv/bin/activate 2>/dev/null || true
    [ -d "$tool/.deps/toml" ] || python3 -m pip install -q --target "$tool/.deps" toml
    exec python3 tools/scripts/permute.py "$@"
fi
exec docker run --rm -i --platform=linux/amd64 -v"$PWD":/xenogears-decomp ethos:latest \
    /bin/bash -c "tools/scripts/permute.sh $(printf '%q ' "$@")"
