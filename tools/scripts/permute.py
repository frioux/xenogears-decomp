#!/usr/bin/env python3
"""Run decomp-permuter on one function, parked or live. Runs INSIDE the build container;
tools/scripts/permute.sh is the host wrapper and the thing to call.

    tools/scripts/permute.sh <unit> <Func> [--minutes N] [-j N] [--keep N]
    tools/scripts/permute.sh battle/main/misc8 func_8007CDD0 --minutes 10

The permuter makes thousands of small random edits to a function's C (new temporaries,
reordered declarations and statements, swapped operands, changed integer types) and
compiles each with the unit's real compiler, keeping whatever scores closer to the
original assembly. It is the tool for a body that is structurally right and a register
or a delay slot off, which is what most parks at 90%+ are.

What this does:
  1. Takes the function's C from src/<unit>.c: the live definition if there is one,
     otherwise the parked /* ... */ body next to its INCLUDE_ASM (the source file is
     not modified).
  2. Builds permuter/<Func>/: base.c (preprocessed, other bodies pruned), target.o
     (the original function alone, assembled from asm/), compile.sh (cc1 + maspsx with
     this unit's flags, read from build.ninja exactly as try_flags.sh does).
  3. Runs the permuter for --minutes (default 5) or until a candidate scores 0.
  4. Prints one RESULT line and the diff of the best candidate against the base.

A score of 0 is the permuter's "identical"; lower is closer. `PERMUTER_ZERO` means a
candidate scored 0: apply the printed diff to the real source by hand, then confirm
with diff_func.sh, which stays the judge. An improvement that is not zero is still a
lead: the diff shows which edit moved the allocator.

Needs report mode (`make objdiff-config`), like diff_func.sh.
"""
import argparse
import glob
import os
import re
import shutil
import signal
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOL = os.path.join(ROOT, "tools", "decomp-permuter")
WORK = os.path.join(ROOT, "permuter")


def die(msg):
    sys.exit("permute: " + msg)


def ninja_flags(unit):
    """The unit's compiler and flags, from build.ninja (global default + per-edge override)."""
    text = open(os.path.join(ROOT, "build.ninja")).read()
    if "-DSKIP_ASM" not in text:
        die("build.ninja is not in report mode: run 'make objdiff-config' first")

    def top(name):
        m = re.search(r"^%s = (.*)$" % name, text, re.M)
        return m.group(1) if m else ""

    def edge(name, ext, rule):
        m = re.search(r"^build build/src/%s\.c\.%s: %s .*\n((?:  .*\n)*)" % (re.escape(unit), ext, rule), text, re.M)
        if m:
            v = re.search(r"^  %s = (.*)$" % name, m.group(1), re.M)
            if v:
                return v.group(1)
        return top(name)

    f = {
        "cpp": top("cpp_flags"), "cc": top("cc_flags"), "endian": top("endianness"),
        "gcc": edge("gcc", "s", "cc"), "opt": edge("optimization_flag", "s", "cc"),
        "gp": edge("gp_flag", "s", "cc"), "msx": edge("maspsx_flags", "o", "maspsx"),
    }
    if not f["gcc"]:
        die("could not read the compiler for %s from build.ninja" % unit)
    return f


def function_source(unit, func):
    """The unit's source with `func` defined as live C, and the path of its original .s."""
    sys.path.insert(0, os.path.join(ROOT, "tools", "scripts"))
    import park

    path = os.path.join(ROOT, "src", unit + ".c")
    if not os.path.exists(path):
        die("no such unit: src/%s.c" % unit)
    text = open(path).read()
    binary = unit.split("/")[0]
    found = glob.glob(os.path.join(ROOT, "asm", binary, "*matchings", "**", func + ".s"), recursive=True)
    if not found:
        die("no asm/%s/*matchings/**/%s.s (wrong function name, or the tree was cleaned)" % (binary, func))

    live = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    if re.search(park.DEF % re.escape(func), live, re.M):
        return text, found[0]
    m = re.search(r'^INCLUDE_ASM\("asm/([^"]+)", %s\);' % re.escape(func), text, re.M)
    if not m:
        die("%s is neither defined nor INCLUDE_ASM'd in src/%s.c" % (func, unit))
    out = park.unpark(text, func, m.group(0))
    if out is None:
        die("%s is a stub with no parked C body next to it: the permuter needs C to start from" % func)
    # The declarations go directly above the body: they use types the includes define.
    decls = park_declarations(text, func)
    body = re.search(park.DEF % re.escape(func), out, re.M)
    return out[:body.start()] + decls + out[body.start():], found[0]


def park_declarations(text, func):
    """Declarations a park carries inside its comment block, above the body.

    Workers park the externs, typedefs and structs a body needs in the same block, and
    park.unpark() takes only the function. Prose notes share the block, so keep just the
    chunks that start like a declaration and run to a `;` at brace depth 0.
    """
    sys.path.insert(0, os.path.join(ROOT, "tools", "scripts"))
    import park

    for m in park.BLOCK.finditer(text):
        body = re.search(park.DEF % re.escape(func), m.group(1), re.M)
        if not body:
            continue
        head, keep, pos = m.group(1)[:body.start()], [], 0
        start = re.compile(r"^(?:typedef|extern|struct|union|enum|static|#define)\b", re.M)
        while True:
            s = start.search(head, pos)
            if not s:
                break
            if head.startswith("#define", s.start()):
                end = head.find("\n", s.start())
                end = len(head) if end < 0 else end
            else:
                depth, end = 0, s.start()
                while end < len(head):
                    c = head[end]
                    depth += (c == "{") - (c == "}")
                    if c == ";" and depth == 0:
                        break
                    end += 1
                end += 1
            keep.append(head[s.start():end])
            pos = end
        return "\n".join(keep) + "\n" if keep else ""
    return ""


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, **kw)


def setup(unit, func):
    f = ninja_flags(unit)
    source, asm = function_source(unit, func)
    d = os.path.join(WORK, func)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)

    raw = os.path.join(d, "unit.c")
    open(raw, "w").write(source)
    cpp = ["mips-linux-gnu-cpp", "-P", "-Iinclude", "-I", "build", "-I", os.path.dirname(os.path.join("src", unit + ".c"))]
    cpp += f["cpp"].split() + ["-DNON_MATCHING", "-DSKIP_ASM", "-DPERMUTER", raw]
    pre = run(cpp, capture_output=True, text=True)
    if pre.returncode:
        die("preprocessing failed:\n" + pre.stderr[-2000:])
    os.remove(raw)
    for dep in ("unit.d", os.path.join(d, "unit.d")):    # cpp_flags carries -MD
        if os.path.exists(os.path.join(ROOT, dep)):
            os.remove(os.path.join(ROOT, dep))

    sys.path.insert(0, TOOL)
    from strip_other_fns import strip_other_fns
    open(os.path.join(d, "base.c"), "w").write(strip_other_fns(pre.stdout, func))

    # The permuter's own cpp pass is a no-op on preprocessed text, but it calls plain `cpp`.
    shim = os.path.join(d, "bin")
    os.makedirs(shim)
    open(os.path.join(shim, "cpp"), "w").write('#!/bin/sh\nexec mips-linux-gnu-cpp "$@"\n')
    os.chmod(os.path.join(shim, "cpp"), 0o755)

    compile_sh = os.path.join(d, "compile.sh")
    open(compile_sh, "w").write("""#!/bin/bash
# permuter calls: compile.sh <in.c> -o <out.o>
# maspsx reads stdin to EOF; without </dev/null it hangs whenever the caller's stdin is an open pipe.
set -e
cd %(root)s
tools/%(gcc)s/cc1 %(opt)s %(gp)s %(cc)s -quiet -o "$3.s" "$1"
python3 tools/maspsx/maspsx.py %(msx)s %(endian)s -Iinclude -I build %(opt)s %(gp)s \\
    -march=r3000 -mtune=r3000 -no-pad-sections -o "$3" "$3.s" </dev/null 2>/dev/null
rm -f "$3.s"
""" % dict(f, root=ROOT))
    os.chmod(compile_sh, 0o755)

    wrap = os.path.join(d, "target.s")
    open(wrap, "w").write('.include "macro.inc"\n.set noat\n.set noreorder\n.text\n.align 2\n.include "%s"\n'
                          % os.path.relpath(asm, ROOT))
    asm_cmd = ["mips-linux-gnu-as", f["endian"], "-Iinclude", "-I", "build", f["opt"], f["gp"],
               "-march=r3000", "-mtune=r3000", "-no-pad-sections", "-o", os.path.join(d, "target.o"), wrap]
    a = run([x for x in asm_cmd if x], capture_output=True, text=True)
    if a.returncode:
        die("assembling the original failed:\n" + a.stderr[-2000:])

    import json
    json.dump(f, open(os.path.join(d, "flags.json"), "w"))
    open(os.path.join(d, "settings.toml"), "w").write('func_name = "%s"\ncompiler_type = "gcc"\n' % func)
    return d


def patch_scoring():
    """Make the permuter compare D_ symbols the way the linker does.

    `D_800D3400[i].unk20` assembles to reloc D_800D3400 with addend 0x20 where the
    original names D_800D3420; the linked bytes are the same (diff_func.sh folds this
    too). Without the fold every typed global is a permanent penalty and nothing
    reaches 0.
    """
    from src import objdump

    orig = objdump.process_mips_reloc

    def fold(reloc_row, prev, repl, imm):
        out = orig(reloc_row, prev, repl, imm)
        m = re.match(r"%(hi|lo)\(D_([0-9A-F]{8})([+-](?:0x)?[0-9a-f]+)?\)$", out)
        if not m:
            return out
        if m.group(1) == "hi":
            return "%hi(D_*)"
        addr = (int(m.group(2), 16) + int(m.group(3) or "0", 0)) & 0xFFFFFFFF
        return "%%lo(D_%08X)" % addr

    objdump.process_mips_reloc = fold


def patch_compiler(d):
    """Compile candidates without a shell or a second Python per candidate.

    compile.sh costs ~0.55 s a candidate, of which ~0.25 s is bash plus maspsx's
    interpreter start-up. Running cc1 directly and maspsx's own main() in this process
    gives the same object (the matched-function check in AGENTS.md is the proof) at
    roughly twice the candidates per second. compile.sh stays as the readable reference.
    """
    import importlib.util
    import io
    import json
    import tempfile
    from src import compiler

    f = json.load(open(os.path.join(d, "flags.json")))
    sys.path.insert(0, os.path.join(ROOT, "tools", "maspsx"))
    spec = importlib.util.spec_from_file_location("maspsx_cli", os.path.join(ROOT, "tools", "maspsx", "maspsx.py"))
    cli = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cli)
    cc1 = [os.path.join("tools", f["gcc"], "cc1")] + f["opt"].split() + f["gp"].split() + f["cc"].split() + ["-quiet"]
    msx = (["maspsx.py"] + f["msx"].split() + [f["endian"], "-Iinclude", "-I", "build"] + f["opt"].split()
           + f["gp"].split() + ["-march=r3000", "-mtune=r3000", "-no-pad-sections"])

    def compile(self, source, *, show_errors=False):
        show = show_errors or self.show_errors or self.debug_mode
        with tempfile.NamedTemporaryFile(prefix="permuter", suffix=".c", mode="w", delete=False) as c:
            c.write(source)
        s_name, o_name = c.name[:-2] + ".s", c.name[:-2] + ".o"
        saved = sys.argv, sys.stdin, sys.stderr
        try:
            err = None if show else subprocess.DEVNULL
            if subprocess.call(cc1 + ["-o", s_name, c.name], cwd=ROOT, stdin=subprocess.DEVNULL, stdout=err, stderr=err):
                return None
            sys.argv, sys.stdin, sys.stderr = msx + ["-o", o_name], io.StringIO(open(s_name).read()), io.StringIO()
            try:
                cli.main()
            except SystemExit as e:
                if e.code:
                    return None
            if not os.path.exists(o_name) or not os.path.getsize(o_name):
                return None
            return o_name
        finally:
            sys.argv, sys.stdin, sys.stderr = saved
            for name in (c.name, s_name):
                try:
                    os.remove(name)
                except OSError:
                    pass

    compiler.Compiler.compile = compile


def permuter_main(argv):
    sys.path.insert(0, TOOL)
    sys.path.insert(0, os.path.join(TOOL, ".deps"))
    os.chdir(ROOT)
    patch_scoring()
    patch_compiler(argv[0])
    from src.main import main
    sys.argv = ["permuter.py"] + argv
    main()


def best_output(d):
    best = None
    for p in glob.glob(os.path.join(d, "output-*")):
        try:
            score = int(open(os.path.join(p, "score.txt")).read())
        except (OSError, ValueError):
            continue
        if best is None or score < best[0]:
            best = (score, p)
    return best


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--permuter-main":
        return permuter_main(sys.argv[2:])

    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit", help="path under src/ without .c, e.g. battle/main/misc8")
    ap.add_argument("function")
    ap.add_argument("--minutes", type=float, default=5, help="search time; 0 = only score the body as written")
    ap.add_argument("-j", type=int, default=4, help="parallel compiles (the build VM has 6 cores)")
    ap.add_argument("--keep", type=int, default=3, help="how many of the best output dirs to keep")
    a = ap.parse_args()

    if not os.path.isdir(os.path.join(TOOL, "src")):
        die("tools/decomp-permuter is missing: call tools/scripts/permute.sh, which fetches it")
    d = setup(a.unit, a.function)
    log = os.path.join(d, "log.txt")
    env = dict(os.environ, PATH=os.path.join(d, "bin") + os.pathsep + os.environ["PATH"])
    # -u, not PYTHONUNBUFFERED: the variable would reach maspsx through compile.sh, which hangs it.
    cmd = [sys.executable, "-u", os.path.abspath(__file__), "--permuter-main", d, "-j", str(a.j),
           "--stop-on-zero", "--best-only", "--algorithm", "difflib",
           # Branch targets count: a back edge that lands one nop off is a real mismatch
           # (archive_b func_80037878), and the permuter ignores targets by default.
           "--no-ignore-branch-targets"]
    start = time.time()
    with open(log, "w") as out:
        p = subprocess.Popen(cmd, cwd=ROOT, env=env, stdout=out, stderr=subprocess.STDOUT, start_new_session=True)
        # The clock starts when the base has compiled and scored, not at launch: under a
        # loaded build VM that alone can take a minute. --minutes 0 stops right there.
        deadline = None
        while p.poll() is None:
            time.sleep(0.5)
            now = time.time()
            if deadline is None:
                if "base score = " in open(log, errors="replace").read():
                    deadline = now + a.minutes * 60
                elif now - start > 300:
                    deadline = now
            if deadline is not None and now >= deadline:
                os.killpg(p.pid, signal.SIGINT)
                try:
                    p.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(p.pid, signal.SIGKILL)
                    p.wait()
    elapsed = time.time() - start

    text = open(log, errors="replace").read()
    m = re.search(r"base score = (\d+)", text)
    rel = os.path.relpath(d, ROOT)
    if not m:
        print(text[-3000:])
        die("the base did not compile or parse (log above, %s/log.txt). GTE/inline-asm bodies and "
            "gcc extensions the permuter's C parser rejects cannot be permuted." % rel)
    base = int(m.group(1))
    iters = re.findall(r"iteration (\d+)", text)
    best = best_output(d)
    score = min(base, best[0]) if best else base

    outs = sorted(glob.glob(os.path.join(d, "output-*")),
                  key=lambda p: (int(open(os.path.join(p, "score.txt")).read()), p))
    for p in outs[a.keep:]:
        shutil.rmtree(p, ignore_errors=True)

    tag = "PERMUTER_ZERO" if score == 0 else ("PERMUTER_BETTER" if score < base else "PERMUTER_NONE")
    print("RESULT %s %s %s base=%d best=%d iterations=%s seconds=%d dir=%s" % (
        tag, a.unit, a.function, base, score, iters[-1] if iters else "?", elapsed, rel))
    if best and best[0] < base:
        diff = open(os.path.join(best[1], "diff.txt"), errors="replace").read().splitlines()
        print("--- best candidate vs base (%s/diff.txt; full source next to it) ---" % os.path.relpath(best[1], ROOT))
        print("\n".join(diff[:150]))
        if len(diff) > 150:
            print("... %d more lines" % (len(diff) - 150))
    if base == 0:
        print("The body as written already scores 0: confirm with diff_func.sh, it may simply match.")
    sys.exit(0)


if __name__ == "__main__":
    main()
