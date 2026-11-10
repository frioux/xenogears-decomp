#!/usr/bin/env bash
# Parallel-round plumbing for agent sessions. One worker = one clone = one .c file.
#
#   tools/rounds/clone.sh new   <tag>...   # cp -ac this tree into ~/xw/r-<tag> on a new branch
#   tools/rounds/clone.sh reset <tag>...   # put existing clones back on origin/main
#   tools/rounds/clone.sh merge <tag>...   # commit stragglers, merge each clone into this tree
#   tools/rounds/clone.sh clean            # delete every clone
#
# `cp -ac` is a copy-on-write duplicate on APFS, which is why a dozen clones of a
# 1.5 GB tree costs nothing. Do not use git worktrees: the build writes to fixed
# paths (build/, asm/, expected/) and gears finds the project by directory name.
#
# A worker needs the tree to already be in report mode (expected/asm/, objdiff.json,
# and a build.ninja with -DSKIP_ASM), so run `make objdiff-config` HERE before `new`.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
POOL=${POOL:-$HOME/xw}
BRANCH_PREFIX=round-

usage() { sed -n '2,16p' "$0" >&2; exit 2; }
[ $# -ge 1 ] || usage

case $1 in
new)
    shift
    [ $# -ge 1 ] || usage
    grep -q -- '-DSKIP_ASM' "$ROOT/build.ninja" 2>/dev/null ||
        { echo "run 'make objdiff-config' here first: clones need report mode" >&2; exit 1; }
    for tag in "$@"; do
        dst=$POOL/r-$tag
        rm -rf "$dst"
        mkdir -p "$dst"
        cp -ac "$ROOT" "$dst/xenogears-decomp"
        # cp -ac is NOT CoW across this volume in practice: measured ~2.6 GB per
        # clone, of which 1.3 GB is disc/disc{1,2}, the raw disc images that only
        # `make extract` reads. A dozen clones filled the disk and killed a whole
        # round with ENOSPC. disc/<bin>.bin stay: culprit.sh diffs against them.
        rm -rf "$dst/xenogears-decomp/disc/disc1" "$dst/xenogears-decomp/disc/disc2"
        cd "$dst/xenogears-decomp"
        git checkout -q -b "$BRANCH_PREFIX$tag"
        git config user.email "worker@local"
        git config user.name "worker"
        echo "$dst/xenogears-decomp"
    done
    ;;
reset)
    shift
    for tag in "$@"; do
        d=$POOL/r-$tag/xenogears-decomp
        [ -d "$d" ] || { echo "?? no clone r-$tag" >&2; continue; }
        cd "$d"
        git checkout -q -- . 2>/dev/null || true
        git clean -fdq -e disc 2>/dev/null || true
        git fetch -q origin
        git checkout -q -B "$BRANCH_PREFIX$tag" origin/main
        echo "r-$tag -> $(git log --oneline -1 --format=%h)"
    done
    ;;
merge)
    shift
    cd "$ROOT"
    for tag in "$@"; do
        d=$POOL/r-$tag/xenogears-decomp
        [ -d "$d" ] || { echo "?? no clone r-$tag" >&2; continue; }
        if [ -n "$(git -C "$d" status --porcelain)" ]; then
            git -C "$d" add -A && git -C "$d" commit -q -m "wip leftovers from r-$tag"
        fi
        branch=$(git -C "$d" branch --show-current)
        git remote add -f "r$tag" "$d" >/dev/null 2>&1 || git fetch -q "r$tag"
        if git merge --no-edit -q "r$tag/$branch" 2>/dev/null; then
            echo "merged r-$tag"
        else
            echo "CONFLICT r-$tag (resolve or 'git merge --abort')"
            git merge --abort 2>/dev/null || true
        fi
        git remote remove "r$tag" 2>/dev/null || true
    done
    ;;
clean)
    rm -rf "$POOL"/r-*
    echo "removed $POOL/r-*"
    ;;
*)
    usage
    ;;
esac
