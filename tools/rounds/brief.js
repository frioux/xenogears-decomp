// Fan a round of decompilation workers out, one unit per worker.
// args: { round, workers: [{ tree, file, unit, asm, binary, overlay, preset,
//                            presetNote?, symbols, funcs, more?, extra? }] }
const HEADER = (w) => `You are a worker on a PS1 matching decompilation (Xenogears, US). YOUR TREE: ${w.tree} - this is your own private copy of the repo; work only there. Read AGENTS.md sections 3 and 4 plus CLAUDE.md 'Known sharp edges' in it before you start.

YOU OWN EXACTLY ONE FILE: ${w.file} (unit path for diffing: ${w.unit}). ${w.ownNote || "Do not edit any other .c file."}

INNER LOOP, from the root of your tree: run \`tools/scripts/diff_func.sh ${w.unit} <Func> || true\`. Note the unit path always carries the binary prefix for main-exe units (\`slus_006.64/system/sound\`, not \`system/sound\`); getting that wrong costs a round, so if the script says 'missing expected/asm/...' your path is wrong, not your tree. The trailing \`|| true\` matters - the script exits 1 on a mismatch and that is not a failure. One command per tool call, and never paste diff output back into bash. It compiles only your unit inside the build container (~5 s) and prints MATCH or a side-by-side diff (left = original, right = yours) plus an \`objdiff: NN.NN%\` score.

A function counts as matched when the output has BOTH a line starting \`MATCH:\` AND \`objdiff: 100.00%\` for that function. MATCH alone is not enough: functions have looked matched while their C was a different size or used different registers, and that breaks the link.

ONE EXCEPTION, and only for a jump table: if the diff is otherwise perfect but the only difference is the switch's table operand (the original names a \`jtbl_\` symbol, yours says \`.rodata\` or \`.L...\`), objdiff can never say 100% because the linker resolves both to the same address. In that case run \`tools/scripts/check_bin.sh ${w.overlay}\` (~35 s). If it prints \`OK\`, your function really does match - commit it and say CHECK_BIN_OK in your report. NOTE this leaves your tree in matching mode, so run \`make objdiff-config\` before you can diff again (~35 s); do it at most once, at the end of your batch.

RULES
- Never run \`make\` (any target) except that one \`make objdiff-config\`: it does a full clean rebuild and takes 30 s. Use the diff script. Never run bare ninja outside it.
- Keep the file's functions in address order. Leave stubs you do not match exactly as they are.
- No inline assembly, no #pragma, no per-function compiler flags. \`char\` is unsigned (-funsigned-char).
- If a function is not at 100.00% after about 8 honest attempts, restore its INCLUDE_ASM line and leave your best attempt in a /* ... */ block directly below it with the best percentage in a comment. Do not delete it when you move on.
- You may append names to ${w.symbols} and add declarations to headers under include/ if a function needs them (your tree is yours; I will merge it). If you edit a header, delete build/src/${w.unit}.c.{i,s,o} afterwards, because ninja does not notice header changes.
- Do not define new globals or data unless the original object really has that data in this unit.

JOB CONTROL: you are killed at 30 minutes. Run \`git add -A && git commit -m "match N in ${w.unit}"\` after every 2-3 matches instead of saving it for the end, and plan to be done writing by minute 22.

Compiler preset for this unit: ${w.preset}${w.presetNote ? " - " + w.presetNote : ""}.
\`tools/scripts/try_flags.sh ${w.unit} <Func> [--gcc gcc-2.7.2-cdk-psx] [--opt -O3] [--gp -G0]\` scores one function under a different preset in about a second, if you suspect the preset is wrong. Do not change gears.toml; report what you find instead.

ASSIGNED FUNCTIONS, smallest first:
${w.funcs}

${w.more || "If you get through the list, continue with the next smallest stubs in the same file."}

The assembly for each function is ${w.asm}/<Func>.s - read it before writing C. Matched functions already in your unit are the reference for struct layouts and idiom; read a few neighbours first.
${w.extra || ""}
TECHNIQUES THAT PAY OFF
- Signed vs unsigned: \`sll/sra\` pairs vs \`andi 0xff\` mean a parameter or field has the wrong type.
- \`lhu\`/\`lbu\` in the original vs \`lw\` in yours tells you the true field width.
- Register allocation misses usually come from local declaration order, split/merged temporaries, or a repeated field load turned into a local (or the reverse).
- Try \`++i\` vs \`i++\` and swapping operands of commutative ops when only instruction order differs.
- The original keeps \`li a0,N\` in a \`jal\`'s delay slot while yours hoists it: gcc has no scheduler here, so the \`li\` is stolen into the slot by reorg only when it is the last insn before the call and no register conflict blocks it. Move other argument setup earlier, or make the constant argument the last one evaluated.

KNOWN BLOCKERS ALREADY FOUND IN THIS REPO - do not rediscover them
- Only difference is a hoisted \`li a0,N\` where the original keeps it in the jal delay slot: put the argument in an \`int\` local before the call, or pass a variable instead of a literal.
- A constant the original loads ONCE into a call-clobbered register before a switch ('li v1,0xA0', 'li a2,0x70): assign it to a source local before the switch; a literal gives a per-block li.
- A divisor/argument the original passes with a plain move/div and no andi or sll/sra pair wants the narrow SIGNED type (s16), not u8/s32.
- Whole-word script-flag read-modify-write: use the union scalar member (actor->scriptFlags.flags), never the bitfields.
- addu operand order follows C order: (i + p->array)->field emits index-first addu, p->array[i].field does not.
- Translate wide vectors as long tmp[3] cast to (VECTOR*) to get 32-bit sw/lw; a VECTOR local gives sh.
- Never write */ or /* inside a parked /* ... */ attempt (e.g. Normal*/Color), and never nest two attempt blocks: it has broken the build three times this session.
- \`%gp_rel(sym)($gp)\` vs \`%hi/%lo\`: maspsx only emits gp-relative when the unit itself defines the symbol in .sdata/.sbss. A plain \`extern\` gives %hi/%lo.
- Original has an unused stack slot and yours does not: declare an unused local (e.g. \`RECT rect;\`) - it reproduces the frame exactly.
- gcc merges two address computations the original keeps separate: force a pointer local (or the reverse) rather than rewriting the expression.
- A \`switch\` becomes a jump table in .rodata; \`if/else\` chains do not. If the original has a \`jtbl_\` reference, use a switch, and vice versa.
- Original has \`addiu $d,$s,-K\` off a base register where yours re-forms the address with lui/addiu: gcc only keeps the single addiu when the base is a RUNTIME value - a parameter, a pointer loaded from a global, a loop-carried pointer that advances, or a struct-member address reached through a runtime pointer. A compile-time symbol base always folds into a fresh lui/addiu pair.
- Do NOT add, move or resize data definitions in a unit whose .rodata/.data is still supplied by splat (check config/<binary>.yaml: if the unit has a .rodata range there, the bytes come from asm, not from your C). Adding them double-counts and shifts the whole overlay - that is what broke misc2 twice.
- Converting the LAST function of a unit whose data still comes from asm deletes that data. Check \'grep -c dlabel asm/<binary>/matchings/<unit>/<Func>.s\' before you convert: if the stub's .s defines rodata/data labels, converting it drops those bytes, the overlay shifts and \'make check\' fails even though objdiff said 100%. Leave it as INCLUDE_ASM and report it instead.
- In \`psyq/\`, objdiff 100% is not proof: several stubs own trailing nops after \`.size\` that no C definition emits. If the stub's .s has instructions after \`.size\`, do not convert it.
- Never change the signature or return type of a function that is already matched; it is someone else's signed-off 100% and it usually costs bytes at link time.
- The GTE macros in include/psyq/inline_c.h are now the RIGHT SDK revision (3.x: one nop plus the canonical COP2 word; read-backs into $2/$8 via gte_*_gpr and gte_*_val). NormalColor, ColorCol, LocalLight, Square*, SetRot/Light/ColorMatrix, AverageSZ3/Z4 all match. Still blocked, do not retry blind: the *3 family (NormalColor3, ColorCol3, DpqColor3) where retail lazily loads incoming stack arguments after the GTE command, and functions whose .s is bare mtc2/ctc2 with no macro equivalent.
- A macro whose scratch registers are declared as hard-coded clobbers ("$2","$3") beats "=r" outputs: that is what makes gcc copy a1/a2 into $v0 at the end instead of storing through $v0. If a GTE store/read-back is one register off, try the clobber form.
- psx 100%-in-objdiff-but-wrong: run \'python3 tools/scripts/convertible.py ${w.binary}/${w.unit}\' to list which stubs of your unit are linkable at all. BLOCKED = instructions after .size or the stub owns dlabel data; those cannot be converted without moving the unit\'s data to C as one address-ordered job, so leave them and report them.
- If the stub already has a parked \'/* ... */\' attempt under it, READ IT FIRST and use it as the starting point (its percentage is in the comment). Rewriting such a function from the assembly throws away the answer: those are 95-100% cases whose only difference is a register or a delay slot.
- \`-mcpu=r8000\` is NOT this project's compiler (it breaks slus_006.64 and field at link time). Do not chase it; the delay-slot shapes are source-level.

WHERE THE VALUE IS (measured over four sessions, do not re-derive)
- The 95-99.6% class is DEAD. Nine workers spent a full round on the closest remaining stubs and moved several up (96.40 to 96.81, 91.88 to 95.23) without closing one: the instruction multiset is already identical and only an allocator tie-break in gcc's global.c differs (func_801E6578 has allocnos with identical def/use counts and live ranges; 20+ source shapes leave the tie untouched; func_800A0C94 has 24 declaration orders byte-identical; func_80023804's sibling compiles the identical expression the other way round). If a parked note says the function is 95% or more and lists >=6 things already tried, do NOT spend more than 10 minutes on it. Report it and move to the next stub, and prefer a stub with no parked note at all.
- The live pools are the ones with stubs that have NO parked attempt: 'psyq/libgpu_b' (25, the Gs library - func_800477D0 is GsSortObject4), 'psyq/libgpu_c' (18), 'system/controller_b' (15), 'system/controller_c' (6), 'psyq/libapi_b' (8), 'psyq/libcard_b' (6), 'system/archive_b' (5), 'psyq/libgpu_tex' (3), 'psyq/libgte_c' (3). They were raw 'asm' yaml segments until this session, so nothing has ever been attempted in them.
- 'python3 tools/scripts/convertible.py <binary>/<unit>' lists which stubs are linkable at all. A BLOCKED stub (instructions after '.size', or the stub's .s owns dlabel data) cannot be converted and objdiff cannot see that. Check it, report BLOCKED entries, and never spend budget on them.

MANDATORY FINAL GATE, before you write your report: run these two in your tree and paste their output into the report.
1. 'python3 tools/scripts/lint_sources.py -q' - catches a function that is both INCLUDE_ASM'd and defined in C (maspsx rejects it, and the build system hides the error, so the only symptom is a missing .o).
2. 'tools/scripts/check_bin.sh ${w.overlay}' - this is YOUR binary; do not guess another. It rebuilds in MATCHING mode and byte-compares. It MUST print OK. If it does not, your change moved data or changed a function's size: 'tools/scripts/culprit.sh ${w.overlay}' names the functions whose bytes differ - revert the ones you touched and keep the rest. This single step is worth 20 minutes of the orchestrator's time per round, and skipping it has thrown away whole rounds of work. (It leaves the tree in matching mode; that is fine at this point.)

FINAL REPORT (the only thing I can see): one line per function you touched - \`MATCH <name> <pct>\` or \`FAIL <name> - one-line reason (best %)\` - plus any symbols/headers you added and anything a later agent should know about this unit. No diffs, no code.`;

return runs.all(args.workers.map((w, i) => ({ key: "w" + i + "_" + w.unit.replace(/\//g, "-"), agent: "worker", task: HEADER(w), cwd: w.tree })));
