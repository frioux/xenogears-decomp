// Naming round: propose real names for functions that are already 100% C but are
// still called func_<addr>. Read-only, so every worker can share the main tree and
// the round can run while a matching round is out.
// args: { workers: [{ tree, file, funcs? }] }   funcs: optional subset, one name per line
const HEADER = (w) => `You are naming functions in a PS1 matching decompilation (Xenogears, US). The repo is ${w.tree}. THIS IS A READ-ONLY JOB: do not edit, build, diff, commit or run make/docker. Your report is the deliverable.

FILE: ${w.file}. ${w.funcs ? "Name these functions:\n" + w.funcs : "Name every function that is DEFINED IN C in this file and is still called func_<addr>."} Skip anything that is still an INCLUDE_ASM stub, including a body parked in a /* ... */ block under one.

HOW
1. Read the file once, top to bottom, and the 'Names' paragraph of AGENTS.md section 4.
2. For each function, look at what it calls, which globals and struct fields it touches, and its constants and strings. If a callee or global is itself unnamed, one grep for its other users is allowed (grep -rn 'func_800XXXXX' src include | head); do not go further than that.
3. Propose a name.

RULES
- PascalCase with the area prefix the already-named functions of this file and its directory use.
- Name what the code demonstrably DOES (BattleClearActorSlots, MenuDrawGoldDigits, WorldMapSetCameraTarget), not a guess at what the game uses it for. A dull true name beats a vivid wrong one.
- If the purpose is opaque, name the mechanism (MenuCopyRect16, BattleSetActorFlag40) rather than leaving func_.
- Names must be unique: check each with grep -rnw '<NewName>' src include config | head -1, and do not hand two functions the same name.
- Under psyq/ only a real SDK name counts (tools/scripts/psyq_names.py); otherwise write NAME_UNKNOWN.
- Do not rename functions that already have a real name, and do not propose names for data.

REPORT (the only thing I can see), one line per function and nothing else:
NAME func_800XXXXX NewName - evidence (the one callee, global, field, string or constant that justifies it)
NAME_UNKNOWN func_800XXXXX - why`;

return runs.all(args.workers.map((w, i) => ({ key: "n" + i + "_" + w.file.replace(/^src\//, "").replace(/\.c$/, "").replace(/\//g, "-"), agent: "worker", task: HEADER(w), cwd: w.tree })));
