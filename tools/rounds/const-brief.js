// Constant round: replace raw numbers in matched C with named constants (AGENTS.md
// section 4, 'Constants'). A #define of the same value hands the compiler the same
// token, so nothing here can move a byte, but each worker still gets a clone
// (tools/rounds/clone.sh new) and proves the build.
// args: { workers: [{ tree, file, bin, funcs?, hints? }] }
//   file:  the .c file the worker owns (one worker per file)
//   bin:   the output that file is linked into (a check_bin.sh name)
//   funcs: optional subset of functions, one name per line; default is every matched function
//   hints: optional 'CONST NAME value - evidence' lines earlier workers reported
const HEADER = (w) => `You are replacing raw numbers with named constants in a PS1 matching decompilation (Xenogears, US). YOUR TREE: ${w.tree} - your own private copy of the repo; work only there. Read the 'Constants' and 'Names' paragraphs of AGENTS.md section 4 first.

YOU OWN ${w.file}. Edit that file only: no header under include/, no other .c, no INCLUDE_ASM lines, no function, global or field names, and no change to any statement beyond spelling a number as a constant.

THE JOB: matched C that says \`if (pRoom->statValues[1] > 0x63) { pRoom->statValues[1] = 0x63; }\` matches and tells the reader nothing. The programmer typed a name: \`#define MENU_STAT_PERCENT_MAX 99\` and \`> MENU_STAT_PERCENT_MAX\`. ${w.funcs ? "Do these functions:\n" + w.funcs : "Do every function that is DEFINED IN C in the file."} Skip INCLUDE_ASM stubs and bodies parked in /* ... */ blocks.
1. Read the file once. For each number in live code ask what the use shows it to be: a value something is clamped to or compared against as a limit, a count a loop runs to, an id / state / mode / command that is compared, switched on or stored, a flag bit that is set or tested, a sentinel.
2. If a constant for it already exists, use it: grep -rnE '#define +[A-Z_0-9]+ +\\(?(0x63|99)\\b' include src (both spellings of the value), and read the header that declares the struct or table involved. Check the existing name MEANS the same thing, not just equals it.
3. Otherwise add a #define at the top of the file, below the includes and above the externs, related ones together under a one-line comment, and use it at every place in the file that means the same thing.
${w.hints ? "\nEARLIER WORKERS PROPOSED THESE. Each is a lead to check against the uses, not an instruction:\n" + w.hints + "\n" : ""}
RULES
- Upper snake case with the area prefix the file's functions use (MENU_, BATTLE_, FIELD_): MENU_STAT_PERCENT_MAX, BATTLE_ACTOR_COUNT, FIELD_SCRIPT_OP_JUMP. Unique: grep -rnw '<NAME>' src include | head -1.
- The #define carries the SAME value. Write it in the base a programmer would: decimal for quantities (99, 250, not 0x63, 0xFA), hex for bit masks and flag bits. A negative value is parenthesised: (-1).
- Name what the uses DEMONSTRATE. A value a stat is clamped to is that stat's maximum; a number a loop over g_BattleActors runs to is the actor count. A guess at what the game means (calling 7 'BOSS_ID' because it looks special) is worse than the number: leave it.
- A name that restates the value (VALUE_63, CONST_FA, NINETY_NINE, MASK_FF) is not a name. Leave the literal.
- One constant per MEANING. The same 99 used as two unrelated limits is two constants; do not sweep every 0x63 in the file into one name.
- A number that repeats a declared size is not a new constant: use sizeof(x) or the array's own dimension constant, and only where the code still matches.
- NOT constants, leave them: 0, 1 and -1 as plain arithmetic (TRUE / FALSE / NULL where that is what they are), shift counts and byte masks in arithmetic (>> 8, & 0xFF), fixed-point and GTE math, array indexes that are just positions, and offsets in raw pointer arithmetic (that is a missing struct, not a missing constant).
- Where the meaning is not shown, keep the literal and fix only its base (\`> 0x63\` -> \`> 99\` for a quantity). That is a complete, correct outcome.

PROOF, before you commit.
- Run \`tools/scripts/check_bin.sh ${w.bin}\`. It rebuilds everything in matching mode (60-90 s) and must print OK. One command per tool call. If it dies with 'Error 137' or looks hung, wait 2 minutes and run it once more: that is memory contention on the build VM, not your code.
- A MISMATCH means an edit changed more than a spelling (a different value, a sizeof with another type, a dropped cast): \`tools/scripts/culprit.sh ${w.bin}\` names the functions; put those lines back and keep the rest.
- When it prints OK: git add -A && git commit -m "constants: ${w.file}". Only committed work is merged.

REPORT (the only thing I can see), and nothing else:
CONST <NAME> <value> - evidence (the one use that justifies it: function, what it does with the value), with ' [existing]' after the name when you used a constant that was already defined
CONST_SHARED <NAME> <header> - for a constant you defined that describes a struct or table declared in that header, or that another file visibly needs, so it can be hoisted
then the number of literals you left because the code does not show their meaning, and the check_bin.sh result.`;

const KEY = (w, i) => "c" + i + "_" + w.file.replace(/^src\//, "").replace(/\.c$/, "").replace(/\//g, "-");
// Rolling launch. Every entry of args.workers is a lane, so workers.length is the concurrency
// and args.queue is the backlog: when a lane's worker ends (done, failed or timed out), the
// lane starts the next queue entry at once (same shape as a worker, its own clone and its own
// file), foreground and background lanes alike. The first `fg` lanes (default 2; the plan
// allows two foreground agents at a time) run on FG_MODEL, the rest on the subagent default,
// the -background model; `fg: 0` makes every lane background. A worker's optional `model`
// (e.g. "lunaroute/glm-5.3-background") overrides its lane's model. runs.run has no thinking
// option (the docs say `thinking` is ignored on dispatch); a level rides on the model id as a
// ":high" suffix, so put it in the worker's `model`. A failed or timed-out worker comes back as
// {key, ok: false, error}; its lane carries on.
const FG_MODEL = "lunaroute/deepseek-4.1-flash";
const queue = [...(args.queue || [])];
const first = [...args.workers];
const fg = args.fg ?? 2;
while (first.length < fg && queue.length) first.push(queue.shift());
let launched = 0;
const launch = (w, model) => {
  const key = KEY(w, launched++);
  const m = w.model || model;
  return runs.run(key, { agent: "worker", task: HEADER(w), cwd: w.tree, ...(w.minutes ? { maxRuntimeMs: w.minutes * 60000 } : {}), ...(m ? { model: m } : {}) })
    .then((r) => r, (e) => ({ key, ok: false, error: String((e && e.message) || e) }));
};
const lane = (w, model) => launch(w, model).then((r) => (queue.length ? lane(queue.shift(), model).then((rest) => [r, ...rest]) : [r]));
return Promise.all(first.map((w, i) => lane(w, i < fg ? FG_MODEL : undefined))).then((lanes) => lanes.flat());
