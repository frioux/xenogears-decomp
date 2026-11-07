#!/usr/bin/env bash
# Publish a rendered progress site to the gh-pages branch of origin.
# Usage: tools/progress/publish.sh [report.json]
set -euo pipefail

report="${1:-build/progress.json}"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
report="$(cd "$(dirname "$report")" && pwd)/$(basename "$report")"
url="$(git remote get-url origin)"
sha="$(git rev-parse HEAD)"
short="${sha:0:7}"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

if git clone --quiet --branch gh-pages --single-branch "$url" "$tmp"; then
    :
else
    rm -rf "$tmp" && mkdir -p "$tmp"
    git init --quiet "$tmp"
    git -C "$tmp" checkout --quiet --orphan gh-pages
    git -C "$tmp" remote add origin "$url"
fi

python3 "$here/render.py" --report "$report" --out "$tmp" --history "$tmp/history" --commit "$sha"
touch "$tmp/.nojekyll"

pct="$(python3 -c 'import json,sys; print("%.1f%%" % float(json.load(open(sys.argv[1]))["measures"]["matched_code_percent"]))' "$report")"

git -C "$tmp" add -A
git -C "$tmp" commit --quiet -m "progress: $short $pct"
git -C "$tmp" push origin gh-pages

echo "Published: https://frioux.github.io/xenogears-decomp/"
