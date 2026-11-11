// Discovery round: un-park every parked body of a unit and score it under the
// alternative compilers. One worker = one clone = one unit file (or a few).
// args: { workers: [{ tree, file, unit, asm, binary, funcs?, notes? }] }
const HEADER = (w) => `You are a worker on a PS1 matching decompilation (Xenogears, US). YOUR TREE: ${w.tree} - your own private copy of the repo; work only there. Read AGENTS.md sections 3 and 4 and the "A unit carries ONE preset" section of section 5 in it before you start.

YOU OWN EXACTLY ONE FILE TO EDIT: ${w.file} (unit path for tooling: ${w.unit}). Do not edit any other .c file, do not edit gears.toml, config/*.yaml, or anything under tools/.

THE JOB (this is NOT a normal matching round - do not write new C)
Every stub in this unit that has a parked C body was parked under the unit's CURRENT compiler. Some of those bodies are probably byte-identical under a DIFFERENT compiler (gcc 2.7.2-cdk, or gcc 2.6.0), which is the single cheapest class of match left in this repo: the C is already written, only the compiler is wrong. Your job is to find out which ones, and to leave the tree in a state where the orchestrator can switch the unit's preset and harvest them.

STEPS
1. List the unit's parks: every \`/* ... */\` block that contains a C body, next to its \`INCLUDE_ASM\` line. ${w.notes || ""}
2. Un-park them ALL, one at a time, with
     python3 tools/scripts/park.py unpark ${w.file} ${w.asm} <Func>
   (it handles all four park shapes in this repo: block below the stub, block above it, \`//\` note lines in between, and a block closed on the body's own brace \`}*/\`). Some functions have the SAME body parked TWICE - un-park the one the tool finds, run the command again for that function if a second copy is still there, and keep exactly ONE definition.
   Keep the file's functions in ADDRESS ORDER. If a definition lands in the wrong place, move it.
3. The file must still compile under the unit's current preset. Check with
     tools/scripts/diff_func.sh ${w.unit} <Func> || true
   which prints MATCH/MISMATCH plus an \`objdiff: NN.NN%\` score. A few of these will already be 100% under the current preset - report those as \`CURRENT <Func> 100.00\`, they are free matches nobody noticed.
4. Score the whole file's C functions under the two alternative compilers, one compile each:
     tools/scripts/try_flags.sh ${w.unit} --all --gcc gcc-2.7.2-cdk-psx </dev/null
     tools/scripts/try_flags.sh ${w.unit} --all --gcc gcc-2.6.0-psx </dev/null
   Each prints one line per C function with its match percentage. They occasionally print \`compile-failed\` or \`absent\` for no good reason (container churn) - re-run the same command before believing it; if gcc-2.6.0 rejects the file (exit 33, mixed declarations) just say so and skip that compiler. If \`try_flags.sh\` says 'missing expected/asm/...', the tree is not in report mode: run \`make objdiff-config\` ONCE (\~35 s) and continue.
5. For every un-parked function:
   - 100.00 under \`gcc-2.7.2-cdk-psx\`  -> LEAVE IT UN-PARKED. It is a match once the orchestrator switches this unit to cdk. Report \`CDK <Func> 100.00\`.
   - 100.00 under \`gcc-2.6.0-psx\`     -> LEAVE IT UN-PARKED only if it is NOT 100 under cdk, and report \`V260 <Func> 100.00\`. (2.6.0 and cdk are different presets; if a function is 100 under both, cdk wins - the orchestrator decides, so just report both numbers.)
   - otherwise re-park it with
       python3 tools/scripts/park.py repark ${w.file} ${w.asm} <Func> --note "<one or two // comment lines: best %, under which compiler, and the one thing that differs>"
     and restore its address position. Do not lose the existing note text: fold it into the new note.
6. Verify the file is sane before you finish: every function you did not leave un-parked has its \`INCLUDE_ASM\` line AND its parked body, and \`python3 tools/scripts/lint_sources.py -q\` prints nothing (a function that is both INCLUDE_ASM'd and defined in C is the one error that hides from everything else).
7. \`git add -A && git commit -m "unpark screen ${w.unit}"\` and report.

DO NOT run \`make\` (any target except that one \`make objdiff-config\`), do not run \`check_bin.sh\`, and do not touch docker: the un-parked functions are compiled under the WRONG preset on purpose, so this tree will NOT link until the orchestrator changes the preset - that is expected, and it is why your report, not the linker, is the deliverable.

RULES
- No new C, no signature changes, no header edits (if a body needs a symbol declared, declare it in this file only).
- Never write \`*/\` or \`/*\` inside a parked body's text and never nest two blocks.
- Commit before you hit 25 minutes; the orchestrator merges commits only.

REPORT (the only thing I can see): one line per function - \`CDK <Func> <pct>\`, \`V260 <Func> <pct>\`, \`CURRENT <Func> 100.00\`, or \`PARKED <Func> <best pct under current preset> - <one-line reason>\` - then a one-paragraph summary of what the unit looks like under each compiler, and the exact list of functions you left un-parked.`;

return runs.all(args.workers.map((w, i) => ({ key: "u" + i + "_" + w.unit.replace(/\//g, "-"), agent: "worker", task: HEADER(w), cwd: w.tree })));
