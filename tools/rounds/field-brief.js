// Field round: give struct fields that are still unk<offset> real names, in the
// declaration and at every use. Field names are not in the object code, so nothing
// here can move a byte, but a rename that misses a user does not compile: each worker
// gets a clone (tools/rounds/clone.sh new) and proves the build.
// args: { workers: [{ tree, file, bins, structs?, hints? }] }
//   file:    the .h or .c that DECLARES the structs (one worker per declaring file)
//   bins:    space-separated outputs whose src/ uses those structs (check_bin.sh names)
//   structs: optional subset of struct names; default is every struct in the file
//   hints:   optional 'FIELD Struct.unkN newName - evidence' lines earlier workers reported
const HEADER = (w) => `You are naming struct fields in a PS1 matching decompilation (Xenogears, US). YOUR TREE: ${w.tree} - your own private copy of the repo; work only there. Read the 'Structs' and 'Names' paragraphs of AGENTS.md section 4 first.

YOU OWN THE STRUCT DECLARATIONS IN ${w.file}${w.structs ? " (only these structs: " + w.structs + ")" : ""}. You may edit that file and, ONLY to follow a renamed field, any .c or .h under src/ and include/ that uses it. Change nothing else: no function bodies beyond the field spelling, no INCLUDE_ASM lines, no function or global names.

THE JOB: a field declared \`unk<offset>\` (\`/* 0x4 */ u8 unk4[0x18];\`, \`s16 unk3D;\`) tells the reader nothing. Where the code shows what a field holds, give it the name a programmer would have typed.
1. Read the declarations, then every use of each unk field: grep -rnE '(\\.|->)unk4\\b' src include, keeping only the lines where the object really is this struct (several structs have an unk4).
2. Decide from the uses: what is stored in it, what it is compared with, which NAMED function it is passed to and as which argument, which table it indexes, what a loop does with it.
3. Rename it in the declaration and at every use, keeping the offset comment, the type and the size exactly as they are.
${w.hints ? "\nEARLIER WORKERS PROPOSED THESE. Each is a lead to check against the uses, not an instruction:\n" + w.hints + "\n" : ""}
RULES
- lowerCamelCase, in the style of the fields of this struct and its neighbours that already have names (\`pc\`, \`digits\`, \`x\`); a pointer field is prefixed p (\`pNext\`).
- Name what the uses DEMONSTRATE. \`flags\` for a field that is only ever masked and tested, \`count\` for one a loop runs up to, \`pNext\` for one that is followed to the same struct type are true names. \`hp\` because the struct looks like an actor is a guess: a wrong name is worse than unk, so leave unk<offset> where the uses do not settle it and say so in the report.
- A name that repeats the offset or the type (\`field4\`, \`byte3D\`, \`data18\`) is not a name. Leave it unk.
- An opaque span (\`u8 unk4[0x18]\`) that the code reaches into with casts (\`((u16 *)&p->unk4)[3]\`, \`*(s32 *)(p->unk4 + 8)\`) is several fields nobody declared. You own the declaration, so split it: one field per offset the code touches, typed by the width of the access (lhu/lh = 16-bit, lbu/lb = 8-bit, signedness from the load), \`unk<offset>\` or \`pad<offset>\` for the bytes nothing touches, the struct's total size unchanged, and every cast access rewritten as a plain field access. A span the code never reaches into, or uses whole as a buffer, gets one name or stays as it is.
- Padding stays \`pad<offset>\`. Do not rename a field that already has a real name.
- Do not touch a struct under include/psyq/ (those are SDK names) or one the file's comments say mirrors an SDK type.

PROOF, before you commit. Splitting a span can change code generation; a rename cannot, but a missed user does not compile.
- Run \`tools/scripts/check_bin.sh <bin>\` for each of: ${w.bins}. Each run rebuilds everything in matching mode (60-90 s) and must print OK. One command per tool call. If it dies with 'Error 137' or looks hung, wait 2 minutes and run it once more: that is memory contention on the build VM, not your code.
- A compile error names a use you missed: fix it and run again. A MISMATCH means a split changed an instruction: \`tools/scripts/culprit.sh <bin>\` names the functions, put THOSE accesses and the fields they need back the way they were and keep the rest.
- When every bin prints OK: git add -A && git commit -m "fields: ${w.file}". Only committed work is merged.

REPORT (the only thing I can see), one line per field you looked at and nothing else:
FIELD <Struct>.unk<offset> <newName> - evidence (the one use that justifies it: file, function, what it does with the field)
SPLIT <Struct>.unk<offset> -> <field>, <field>, ... - the accesses that showed the layout
FIELD_UNKNOWN <Struct>.unk<offset> - what the uses do show, so the next reader starts there
then the check_bin.sh result for each bin.`;

const KEY = (w, i) => "f" + i + "_" + w.file.replace(/^(src|include)\//, "").replace(/\.[ch]$/, "").replace(/\//g, "-");
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
