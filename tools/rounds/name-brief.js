// Naming round: propose real names for functions that are already 100% C but are
// still called func_<addr>, and for the D_<addr> globals their C uses. Read-only, so every worker can share the main tree and
// the round can run while a matching round is out.
// args: { workers: [{ tree, file, funcs? }] }   funcs: optional subset, one name per line
const HEADER = (w) => `You are naming functions and globals in a PS1 matching decompilation (Xenogears, US). The repo is ${w.tree}. THIS IS A READ-ONLY JOB: do not edit, build, diff, commit or run make/docker. Your report is the deliverable.

FILE: ${w.file}. ${w.funcs ? "Name these functions:\n" + w.funcs : "Name every function that is DEFINED IN C in this file and is still called func_<addr>."} Skip anything that is still an INCLUDE_ASM stub, including a body parked in a /* ... */ block under one. Then name every global that the C of this file uses and that is still called D_<addr>.

HOW
1. Read the file once, top to bottom, and the 'Names' paragraph of AGENTS.md section 4.
2. For each function, look at what it calls, which globals and struct fields it touches, and its constants and strings. If a callee or global is itself unnamed, one grep for its other users is allowed (grep -rn 'func_800XXXXX' src include | head); do not go further than that.
3. Propose a name.
4. For each D_<addr> global the file's live C uses (not one that only a parked body or a stub mentions), look at every use in this file and, with one grep, its users elsewhere (grep -rnw 'D_800XXXXX' src include | head): what is stored in it, what indexes it, which named functions read and write it, and for initialised data what the contents are.

RULES
- PascalCase with the area prefix the already-named functions of this file and its directory use.
- Name what the code demonstrably DOES (BattleClearActorSlots, MenuDrawGoldDigits, WorldMapSetCameraTarget), not a guess at what the game uses it for. A dull true name beats a vivid wrong one.
- If the purpose is opaque, name the mechanism (MenuCopyRect16, BattleSetActorFlag40) rather than leaving func_.
- Names must be unique: check each with grep -rnw '<NewName>' src include config | head -1, and do not hand two functions the same name.
- Under psyq/ only a real SDK name counts (tools/scripts/psyq_names.py); otherwise write NAME_UNKNOWN.
- Do not rename functions or globals that already have a real name.
- Globals are g_ + PascalCase with the same area prefix (g_BattleActorSlots, g_MenuCursorRow, g_FieldScriptPc). Name what the global HOLDS as the uses show it: the table a named function indexes by actor, the counter a loop runs to, the string's text (g_StrClearOtag). A name that only repeats the type or the address (g_BattleByteTable, g_Data800D2CB0) says nothing more than D_ does: write DATA_UNKNOWN instead.
- A D_ that is the INSIDE of a table (D_800D343F used as 'D_800D343F + i * 0x40' is field 0x3F of the 0x40-byte table at D_800D3400) is not a global of its own and must not get a name: write DATA_INTERIOR. Name the base if the file uses it.
- Under psyq/ a global gets a name only if it is a real SDK one; otherwise DATA_UNKNOWN.

REPORT (the only thing I can see), one line per function and per global and nothing else:
NAME func_800XXXXX NewName - evidence (the one callee, global, field, string or constant that justifies it)
NAME_UNKNOWN func_800XXXXX - why
DATA D_800XXXXX g_NewName - evidence (the one use that justifies it: function, what it does with the global)
DATA_UNKNOWN D_800XXXXX - what the uses do show
DATA_INTERIOR D_800XXXXX - base D_800YYYYY, element size, field offset`;

// file may be a comma-separated list of files (one worker, several small files), so the
// run key is sanitised: anything outside [A-Za-z0-9._-] becomes '-', and it is truncated.
const KEY = (w, i) => "n" + i + "_" + w.file.replace(/^src\//, "").replace(/\.c\b/g, "").replace(/[^A-Za-z0-9._-]+/g, "-").slice(0, 100);
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
