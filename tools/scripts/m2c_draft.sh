#!/usr/bin/env bash
# Print a first-draft C translation of one INCLUDE_ASM stub, made by m2c
# (https://github.com/matt-kempster/m2c). The draft is a starting point for
# hand-matching, not matching code.
#
#   tools/scripts/m2c_draft.sh <unit> <function> [extra m2c args]
#   tools/scripts/m2c_draft.sh battle/main/misc23 func_800B1F6C
#   tools/scripts/m2c_draft.sh slus_006.64/system/sound func_80041234 --no-casts
#
# The first output line is "// context: unit" or "// context: none".
# Exit 2: bad usage or m2c not installed (the script never downloads anything).
# Exit 1: no draft could be produced. Exit 0: draft printed on stdout.
set -euo pipefail

[ $# -ge 2 ] || { sed -n '2,10p' "$0" >&2; exit 2; }
unit=$1
func=$2
shift 2
cd "$(dirname "$0")/../.."

m2c=tools/m2c/m2c.py
if [ ! -f "$m2c" ]; then
    echo "m2c not found at $m2c. Operator: install it with" >&2
    echo "  git clone https://github.com/matt-kempster/m2c tools/m2c && python3 -m pip install --target tools/m2c/_deps pycparser" >&2
    exit 2
fi

bin=${unit%%/*}
rest=${unit#*/}
stub=asm/$bin/nonmatchings/$rest/$func.s
src=src/$unit.c
[ -f "$stub" ] || { echo "no stub $stub (already converted?)" >&2; exit 1; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# Strip what pycparser cannot parse from preprocessed C, keeping only types,
# globals and prototypes. Reads stdin, writes stdout.
filter_context() {
    python3 -c '
import re, sys

text = sys.stdin.read()
# line markers / leftover directives
text = "\n".join(l for l in text.splitlines() if not l.lstrip().startswith("#"))

def skip_str(s, i):
    q = s[i]
    i += 1
    while i < len(s) and s[i] != q:
        i += 2 if s[i] == "\\" else 1
    return i + 1

def skip_parens(s, i):
    # s[i] == "(": return index just past the matching ")"
    d = 0
    while i < len(s):
        c = s[i]
        if c in "\"\x27":
            i = skip_str(s, i)
            continue
        if c == "(":
            d += 1
        elif c == ")":
            d -= 1
            if d == 0:
                return i + 1
        i += 1
    return i

def drop_calls(s, pat):
    # remove NAME ( balanced ) for every match of regex pat
    out, pos = [], 0
    rx = re.compile(pat + r"\s*\(")
    while True:
        m = rx.search(s, pos)
        if not m:
            out.append(s[pos:])
            return "".join(out)
        out.append(s[pos:m.start()])
        pos = skip_parens(s, m.end() - 1)

text = drop_calls(text, r"\b__attribute__")
text = drop_calls(text, r"\b(?:__asm__|__asm|asm)(?:\s+(?:volatile|__volatile__))?")

out, stack, i, n = [], 0, 0, len(text)
while i < n:
    c = text[i]
    if c in "\"\x27":
        j = skip_str(text, i)
        out.append(text[i:j])
        i = j
    elif c == "{" and stack == 0:
        prev = "".join(out).rstrip()[-1:]
        if prev in (")", "="):
            # function body or initializer: skip the balanced block
            d = 0
            while i < n:
                if text[i] in "\"\x27":
                    i = skip_str(text, i)
                    continue
                if text[i] == "{":
                    d += 1
                elif text[i] == "}":
                    d -= 1
                    if d == 0:
                        i += 1
                        break
                i += 1
            if prev == "=":
                body = "".join(out).rstrip()
                out = [body[:-1].rstrip()]
            else:
                # a prototype needs a terminating ;
                out.append(";")
                # swallow a stray ; after the block
                k = i
                while k < n and text[k].isspace():
                    k += 1
                if k < n and text[k] == ";":
                    i = k + 1
        else:
            stack += 1
            out.append(c)
            i += 1
    else:
        if c == "{":
            stack += 1
        elif c == "}":
            stack -= 1
        out.append(c)
        i += 1

text = "".join(out)
# INCLUDE_ASM stubs, the remaining bare asm statements, and empty statements
text = re.sub(r"\bvoid\s+__maspsx_include_asm_hack_\w+\s*\(\s*\)\s*;", "", text)
text = re.sub(r"(?m)^\s*;\s*$", "", text)
text = re.sub(r"\n{3,}", "\n\n", text)
sys.stdout.write(text.strip() + "\n")
'
}

# Build $1 = the filtered context for the unit; return 1 when there is none.
build_context() {
    local out=$1 pre=$tmp/pre.c
    if [ -f "build/src/$unit.c.i" ]; then
        cp "build/src/$unit.c.i" "$pre"
    elif [ -f "$src" ]; then
        # same include paths and defines as the cpp rule in build.ninja; SKIP_ASM
        # turns INCLUDE_ASM into nothing
        ${CC:-cc} -E -x c -Iinclude -Ibuild -D_LANGUAGE_C -DUSE_INCLUDE_ASM -DNON_MATCHING \
            -DSKIP_ASM -DM2CTX -P -undef -nostdinc -w "$src" -o "$pre" 2>"$tmp/cpp.err" || return 1
    else
        return 1
    fi
    filter_context <"$pre" >"$out" || return 1
    [ -s "$out" ]
}

# Files defining jump tables the stub references but does not define itself.
inputs=("$stub")
while read -r label; do
    [ -n "$label" ] || continue
    grep -qE "^(d|g)?label $label\b|^$label:" "$stub" && continue
    def=$(grep -rlE "^(d|g)label $label\b|^$label:" "asm/$bin" 2>/dev/null | head -n 1 || true)
    if [ -n "$def" ]; then
        inputs+=("$def")
    else
        echo "warning: $label is not defined under asm/$bin; switch may not be recovered" >&2
    fi
done < <(grep -oE '\bjtbl_[0-9A-Fa-f]+' "$stub" | sort -u)

run_m2c() {
    local pp=
    [ -d tools/m2c/_deps ] && pp=tools/m2c/_deps${PYTHONPATH:+:$PYTHONPATH}
    PYTHONPATH=${pp:-${PYTHONPATH:-}} python3 "$m2c" -t mipsel-gcc-c "$@"
}

ctx=$tmp/context.c
if build_context "$ctx" && run_m2c --context "$ctx" "$@" "${inputs[@]}" >"$tmp/out.c" 2>"$tmp/err.txt"; then
    echo "// context: unit"
    cat "$tmp/out.c"
    exit 0
fi
[ -s "$tmp/err.txt" ] && { echo "m2c failed with context, retrying without:" >&2; head -n 5 "$tmp/err.txt" >&2; }

if run_m2c "$@" "${inputs[@]}" >"$tmp/out.c" 2>"$tmp/err.txt"; then
    echo "// context: none"
    cat "$tmp/out.c"
    exit 0
fi
cat "$tmp/err.txt" >&2
exit 1
