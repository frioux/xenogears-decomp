// Apply round: port decomp-permuter results onto parked bodies. One worker = one clone = one .c file.
// args: { workers: [{ tree, file, unit, binary, overlay, funcs, extra? }] }
// `tools/rounds/permute_sweep.py --report --briefs` prints this shape with everything but `tree` filled in.
const HEADER = (w) => `You are a worker on a PS1 matching decompilation (Xenogears, US). YOUR TREE: ${w.tree} - your own private copy of the repo; work only there. Read AGENTS.md sections 3 and 4 plus CLAUDE.md 'Known sharp edges' in it before you start.

YOU OWN EXACTLY ONE FILE: ${w.file} (unit path for tooling: ${w.unit}). Do not edit any other .c file, gears.toml, config/ or tools/.

THE JOB (not a normal matching round - do not start stubs that are not on your list)
Each function below is an INCLUDE_ASM stub with a parked C body next to it (a /* ... */ block, with a note saying how close it got). A search tool, decomp-permuter, has already made thousands of small random edits to each of those bodies, compiling every one with this unit's real compiler, and kept the edit that came closest to the original assembly. Your job is to port that edit into the source and gate it.

${w.funcs}

Each line is: function, ZERO or BETTER, the permuter's score for the parked body (base) and for its best candidate (best; 0 = identical, lower is closer), and the path of a unified diff.
- The diff is between the PREPROCESSED parked body and the PREPROCESSED candidate, so macros and constants appear expanded ('((void *) 0)' for NULL, numbers for enum names) and the line numbers are meaningless. Read it for WHAT changed: usually a new temporary ('new_var'), a reordered declaration or statement, a changed integer type, a swapped operand, a split or merged expression. The candidate's whole preprocessed file is next to it with the same name and a .c suffix; search it for the function name if the diff alone is unclear.
- Port the SAME change to the parked body by hand. Do not paste preprocessed text into the source: keep the unit's macros, names and struct types.

STEPS, per function
1. Un-park it: python3 tools/scripts/park.py unpark ${w.file} <asm dir> <Func>   (the asm dir is the path inside that function's INCLUDE_ASM("asm/<...>", ...) line with the leading asm/ removed). Keep the file's functions in address order.
2. Apply the edit. Run: tools/scripts/diff_func.sh ${w.unit} <Func> || true   (one command per tool call; ~5 s; left = original, right = yours; the trailing || true matters).
3. ZERO: you should now see a line starting MATCH: and 'objdiff: 100.00%'. That is a match. (If MATCH: is printed, the byte count equals the original's and objdiff reads 99.x% only because a D_ global is reached as base+offset, report it as MATCH <name> RELOC; check_bin.sh in the final gate is the proof.) If it does not match, you ported the edit inexactly: compare against the candidate .c again before trying anything else.
4. Once it matches, COMMIT (git add -A && git commit -m "match <Func> via permuter"), then make it read like source: give 'new_var' a real name and type, and try removing each permuter edit that looks gratuitous ONE AT A TIME, re-running the diff after each; keep an edit only if the match needs it. Commit again. Never trade the match for looks.
5. BETTER: port the edit and check that the objdiff percentage went up. Then let the permuter continue from your improved body - it reads the function straight from your file, parked or live:
     tools/scripts/permute.sh ${w.unit} <Func> --minutes 4 -j 2
   It prints 'RESULT PERMUTER_ZERO|PERMUTER_BETTER|PERMUTER_NONE ... base=N best=M' and the diff of its best candidate. Port, re-diff, repeat at most three times per function while the score keeps falling (gains compound: each run starts from the last improvement). Between runs, look at what the remaining diff IS and fix by hand whatever is plainly a wrong type, a missing local or a wrong operand order; the permuter is good at allocator ties and bad at structure.
- **The permuter's score is not the judge: objdiff is.** A candidate that scored 20 (down from 25) once measured 13 points WORSE under objdiff, because the edit moved a lui/addiu pair above a load. After every port, run diff_func.sh before keeping it, and keep the change only if the percentage went up. A parked body that is one instruction short with a folded address (gcc ships an lw at a 16 displacement off the hi register where retail materialises the base) is a structural wall, not an allocator tie: stop permuting it and record the shape in the note.
6. Not at 100.00% when you stop: re-park the best version -
     python3 tools/scripts/park.py repark ${w.file} <asm dir> <Func> --note "<// comment lines: best %, the permuter score, the one thing that still differs, what was tried>"
   keeping what the old note said. The INCLUDE_ASM line must be back and the block must end with a line that is exactly */ .

RULES
- NEVER run docker kill / docker rm or otherwise touch containers: the build container image is shared with every other worker on this host. If a build wedges, wait and re-run.
- Never run make (any target) except one 'make objdiff-config' if diff_func.sh says the tree is not in report mode. Never run bare ninja.
- No inline assembly, no #pragma, no per-function flags. char is unsigned.
- Do not change the signature of a function that is already matched, and do not re-split an opaque field in a shared header under include/ (index it in your .c instead).
- Never write */ or /* inside a parked block, and never nest two blocks.
- A unit whose stubs reference a jtbl_ symbol (grep -l jtbl asm/${w.binary}/nonmatchings/<unit dir>/*.s): a C switch there can show 100% and still break the link. Only check_bin.sh decides; if it fails after you converted such a function, re-park it and say JTBL.
- You are killed at 30 minutes. Commit after every match; plan to be in the final gate by minute 22.
${w.extra || ""}
MANDATORY FINAL GATE, before your report - run both in your tree and paste their output:
1. python3 tools/scripts/lint_sources.py -q        (must print nothing)
2. tools/scripts/check_bin.sh ${w.overlay}          (must print OK; ~35 s, rebuilds in matching mode. 'Error 137' or a hang is memory contention from other workers: wait 2 minutes and run it once more. If it prints a mismatch, tools/scripts/culprit.sh ${w.overlay} names the functions whose bytes differ - re-park the ones you touched.)

NAMES: for every function you brought to 100.00%, propose a real name in the report and do NOT rename anything in your tree: 'NAME func_800XXXXX NewName - evidence', PascalCase with the area prefix the unit's named functions use, naming what the body demonstrably does (a callee, a global, a field), never a guess.

FINAL REPORT (the only thing I can see): one line per function - 'MATCH <name> <pct>', 'IMPROVED <name> <old pct> -> <new pct> (permuter score <n>)' or 'FAIL <name> - one-line reason' - then the NAME lines, the two gate outputs, and anything a later agent should know. No diffs, no code.`;

const KEY = (w, i) => "p" + i + "_" + w.unit.replace(/\//g, "-");
// Rolling launch. The plan allows two foreground (not -background) agents at a time, so the
// first `fg` workers (default 2) run on FG_MODEL and the rest on the subagent default, the
// -background model. When a foreground worker ends, its lane starts the next entry of
// args.queue at once (same shape as a worker, its own clone and its own file), so the
// foreground slots stay busy instead of idling until the slowest background worker ends.
// A failed or timed-out worker comes back as {key, ok: false, error}; its lane carries on.
const FG_MODEL = "lunaroute/deepseek-4.1-flash";
const queue = [...(args.queue || [])];
const first = [...args.workers];
const fg = args.fg ?? 2;
while (first.length < fg && queue.length) first.push(queue.shift());
let launched = 0;
const launch = (w, model) => {
  const key = KEY(w, launched++);
  return runs.run(key, { agent: "worker", task: HEADER(w), cwd: w.tree, ...(w.minutes ? { maxRuntimeMs: w.minutes * 60000 } : {}), ...(model ? { model } : {}) })
    .then((r) => r, (e) => ({ key, ok: false, error: String((e && e.message) || e) }));
};
const lane = (w) => launch(w, FG_MODEL).then((r) => (queue.length ? lane(queue.shift()).then((rest) => [r, ...rest]) : [r]));
return Promise.all(first.map((w, i) => (i < fg ? lane(w) : launch(w).then((r) => [r])))).then((lanes) => lanes.flat());
