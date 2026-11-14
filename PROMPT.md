# Xenogears matching decompilation — orchestrator prompt

You orchestrate. Workers write the C, one worker per `.c` file, briefed by
`tools/rounds/brief.js`. Your context is the scarce resource, so:

- **This file is the whole handover.** Do not read `reports/TRACKER.md` (160 KB of
  history), `AGENTS.md` or `brief.js` end to end. Look up what you need:
  `grep -n '<unit>' reports/TRACKER.md | tail -20` before briefing a unit,
  `grep -n '^##' AGENTS.md` for a section, `sed -n 1,3p tools/rounds/brief.js` for
  the worker args.
- Do not write or debug C yourself, and do not paste diffs or worker transcripts into
  your context. A function that needs more than a merge is next round's brief.
- Do not poll a running round. Launch it, wait for the completion notice.

Start:

    colima status || colima start --vz-rosetta
    make check              # all ELEVEN outputs OK before you touch anything
    grep -rc '^INCLUDE_ASM' src | awk -F'[/:]' '{n[$2]+=$NF} END{for(b in n) print n[b], b}' | sort -rn

That last line is the scoreboard (stubs per binary; 4568 functions in total). Three
numbers move in this job: **functions matched** and **matched functions with a real
name** go up, and **raw lines** (matched C that no programmer would have written; see
"Realistic C") goes down.

## The round loop

One worker = one clone = **one `.c` file**.

    make objdiff-config                          # clones need report mode
    tools/rounds/clone.sh new a b c ...          # cp -ac clones in ~/xw/r-<tag>
    # subagent({workflow: "tools/rounds/brief.js", async: true, args: {workers: [
    #   {tree, file, unit, asm, binary, overlay, preset, symbols, funcs, extra}, ...]}})
    # ... ~30 min, workers are killed at 30 ...
    rm -f ~/xw/r-*/xenogears-decomp/.ninja_lock  # else merge sees UNCOMMITTED and drops the clone
    tools/rounds/clone.sh merge a b c ...
    python3 tools/scripts/lint_sources.py -q
    # save the reports' NAME lines to a file, then:
    tools/scripts/rename_sym.py -f <names file>  # see "Names"
    make check
    git commit, git push, one row in reports/TRACKER.md
    rm -rf ~/xw/r-<tags>                         # fresh `new` clones next round

If `make check` fails: `tools/scripts/culprit.sh <bin>` names the differing functions
and `tools/scripts/sizes.py` names a function whose size is wrong. Revert that
function to its stub, re-check, and put the cause in the next brief. `git checkout --
src include config` drops the uncommitted renames if you need to rule them out (they
cannot move bytes). Run `make progress-publish` every third round.

## Names

**A function that is 100% C must not stay `func_<addr>`.** Naming is half the value
of a match, and it is cheap.

- Workers end their report with `NAME func_800XXXXX NewName - evidence` for every
  function they matched (the rules are in `brief.js`). They never rename in their
  clone; you apply the names after the merge with `tools/scripts/rename_sym.py` then
  `tools/scripts/name_addrs.py`, which rewrite `symbol_addrs`, `src/` and `include/`
  together and skip, with a reason, any name that is taken, any function that is still
  a stub, and any address that two files claim.
- Accept a name when its evidence cites something in the body (a callee, global,
  field, string). Drop it when the evidence is a guess at story purpose. A plain true
  name (`MenuCopyRect16`) is better than none; a wrong name is worse than `func_`. Do
  not re-read the C to adjudicate; a bad name is one `rename_sym.py Old=New` to fix.
- **Rename only while no clones are out.** A worker whose new C calls the old name
  breaks the merge. A naming round is two commands, in this order:
  `tools/scripts/rename_sym.py -f names.txt` (renames the symbols that already have an
  entry) then `tools/scripts/name_addrs.py -f names.txt` (renames the rest and **adds**
  `NewName = 0xADDR;` to the `symbol_addrs` file of every binary whose `src/` or `asm/`
  refers to the address). The added entry is the point: splat names a binary's own calls
  and `.data` function tables from `symbol_addrs`, so renaming only in `src/` gives
  `undefined reference to func_<addr>` from every stub and table that points at it
  (round 36: 449 of 502 names, an hour of link errors). Both tools exit 1 on a SKIP;
  read the SKIP block. `name_addrs.py` owns a symbol by the binary whose C *defines* it (so a
  main-exe function that five overlays call names fine), leaves a name alone when its address
  is a *different* function in another overlay (2+ `.c` files define it) and never writes an
  entry into a file its yaml already loads ("Duplicate symbol detected" from
  splat stops the whole build), so ~4-8 names a round stay `func_`; leave them.
- **The names file must keep the three-column form**, `NAME func_ADDR NewName - evidence`. A file
  stripped to two columns still renames (rename_sym.py parses it) but makes `name_addrs.py` a silent
  no-op, so every function whose address has no `symbol_addrs` entry - all of `member_change_menu` -
  stays `func_` with no error. Round 41 lost nothing only because it noticed the "renamed 0 pairs"
  line; keep the evidence text in the file you feed both tools.
- **Backlog:** about 2,100 already-matched functions are still `func_`. While a
  matching round is out, run a naming round over them with
  `tools/rounds/name-brief.js` (`{workers: [{tree: <this repo>, file}, ...]}`). It is
  read-only, so workers share this tree and need no clones. Pick the files with the
  most unnamed definitions that are **not** in the current matching round, hold the
  NAME lines, and apply them after the matching round has merged, with the two commands
  above. Count definitions with `/* ... */` blocks stripped: a bare `grep func_` counts
  `extern` prototypes and parked bodies, and round 35 spent two duds on files with zero
  unnamed C functions. Ten naming workers yield ~500 names, so this is the cheaper of
  the two numbers to move.
- Tracker row: `+N matched, +M named, raw R`.

## Realistic C

**A match is step one. The product is C that reads like the source Square compiled.**
These two lines match and are not acceptable as the final form:

    *(u8 *)(D_800CCE3D + slot * 0x170) = (*p)[1];
    u8 *p = D_800D343F + index * 0x40;

Nobody typed those. They say "there is an array of 0x170-byte structs based a little
below `D_800CCE3D`, and `p` points at something with a cursor in it", and the finished
function says so in its types: `g_Slots[slot].unk3D = pScript->pc[1];`. Raw pointer
arithmetic on a global, cast-and-offset field access (`*(s16 *)(pNode + 6)`), `(*p)[n]`,
m2c temporaries and locals that exist only to carry a cast are all the same defect: a
missing struct. The rules a worker follows are in `brief.js` ("REALISTIC C").

- **Starting raw is allowed, staying raw is not.** A worker gets the match however it
  can, commits, then rewrites toward the typed form and keeps whichever version still
  matches. What it could not clean it reports as `ROUGH <name> - reason`. A ROUGH
  function counts as matched and is still owed work: it goes into a later rewrite
  round, and again after that, until it is clean or the tracker records the specific
  construct that only the raw spelling matches.
- **The backlog** (2,663 lines in 103 files when this was written, parked bodies
  included):

      grep -rcE 'D_[0-9A-F]{8} \+ \w+ \* |\*\((const )?[us](8|16|32) ?\*+\)\(|\(\*\w+\)\[|\(\(?[us](8|16|32) ?\*\)\w+\)? \+ ' src | awk -F: '$2 {n+=$2; print} END {print "TOTAL:" n}' | sort -t: -k2 -rn | head -21

  The first line is the total, the rest are the worst files. Put the total in the tracker row (`raw N`). It must not rise across a round: a
  matching round that adds raw lines owes a rewrite round on the same files.
- **Rewrite rounds.** Same `brief.js`, same clones, one worker per file, run alongside
  matching rounds on files that are not in them. `funcs` lists matched functions that
  the grep hits, and `extra` starts with the word `REWRITE` followed by the struct the
  file needs (table base, element size, the header that should own it). Group by
  table, not by file count: the functions that index one table want one struct, so
  give them to one worker, and when several files share a table have the first round
  land the struct in `include/` before other files are briefed against it. Two workers
  declaring the same table differently is the "re-splitting a shared header" break
  below.
- **objdiff reads 99.x% for a typed global and that is fine.** `D_800D3400[i].unk20`
  assembles to the relocation `D_800D3400` plus an addend where the original object
  names `D_800D3420`; the linked bytes are identical (`make check` confirmed it on
  `battle/misc6_r1`). `diff_func.sh` folds the addend and prints `MATCH`; the worker
  reports `MATCH <name> RELOC` and confirms with `check_bin.sh`. So overlays are not
  an excuse for the exact-address spelling, and `select_target.py` or a report that
  ranks by objdiff percentage will show these a little under 100.
- Do not accept invented field names to make a line look better. `unk3D` in a declared
  struct is realistic; `hp` on a guess is a wrong name, and the Names rule applies.
- Fix the cause when a round's reports show the same raw form coming back: one line in
  `brief.js` with the typed spelling that matched.

## Choosing what to brief

Measured over ten sessions; the brief decides the yield (3 per round with units picked
by stub count, 10 to 16 with the method below).

1. Rank units by **instructions per fresh stub**, smallest first:
   `python3 tools/scripts/select_target.py --json` (ignore its `attempt` flag). Stubs
   under ~120 instructions are where the matches are.
   **A stub is fresh only if no percentage sits within ~25 lines of its `INCLUDE_ASM`.**
   Parks are written `// Best: 84.76%` far more often than as `/* ... */`, so `grep
   '^/\*'` over-reports badly: round 36 briefed 11 units picked that way and 9 of them
   were 100% parked (7 matches from 11 workers). Measured round 37 with that test: 1338
   stubs in the tree, **425 with no percentage within 25 lines**; after the reachability
   filter in item 2 the best 13 units held **77 reachable stubs and closed 4**. "No park
   note" does not mean "achievable" - a stub can be fresh and still a wall - so budget
   10-16 workers per round and expect single digits; the naming round is the reliable
   half (+252 names from 10 workers, same round).
2. **Give the worker the reachable subset, not the unit.** Leave out of `funcs`:
   - `python3 tools/scripts/atfree.py <bin>/<unit> asm/<bin>/nonmatchings/<unit>`
     (assembler ceiling, no preset matches them; most of `battle/main/misc26..35`);
   - `grep -l jtbl asm/<bin>/nonmatchings/<unit>/*.s` (a C `switch` there shifts the
     overlay while objdiff says 100);
   - `python3 tools/scripts/convertible.py <bin>/<unit>` BLOCKED entries;
   - stubs with `sw $sp`/`lw $sp` (stack switch; the alloca bodies it catches are
     `grep -l 'sw \$sp, 0x0' asm/*/nonmatchings/`, a dozen in `system/` and the battle tail);
   - stubs of <15 insns that start by loading a literal address: often a data word, not a
     function (`battling/misc12` `func_80088BFC` is a 4-byte pointer datum, code starts at +4);
   - parks at 95%+ with a list of things tried. **Every park has been swept under every
     preset; a park that is not 100 is dead.** Only new C closes it, and it is worth ~0.
3. Put what the tracker says about the unit (one or two lines) in `extra`. **No raw
   backtick in `extra`** — `brief.js` is a template literal.
4. Finished, never brief: `slus_006.64/system/model`, `psyq/{libgte,libgte_b,libapi,
   libapi_2,libapi_3,libcard,libcard_b}`.
5. Two consecutive rounds of ~0 in a unit: stop briefing it and say why in the tracker.

## Presets (settled; do not re-screen)

`gears.toml` is the truth, keyed per file; a unit not listed there is Default (gcc
2.7.2, `-O2 -G8`). Read the unit's preset from it and pass it to the worker verbatim.
Workers never edit `gears.toml`; they report `PRESET_260` / `PRESET_G0` /
`PRESET_CDK` with both percentages and the scores of every already-matched function,
and you decide.

- All maspsx axes and all compiler axes (2.6.0, cdk, `-G0`) have been swept at unit
  and park level. Do not run a repo-wide sweep again. After a round,
  `tools/rounds/park_sweep.py <unit>...` on the units that gained parks is enough.
- Every matched function stays 100.0 under the variant and a park reaches 100.0:
  switch the unit. Some break: split it with `tools/scripts/split_unit.py` (AGENTS.md,
  "A unit carries ONE preset"). A unit with `.rodata`/`.data`/`.bss` subsegments in
  `config/<bin>.yaml` cannot be split (`field/main/misc`, `system/sound`). The one
  measured candidate left, `battle/misc19`, costs five files for two functions.
- Never a directory key plus a per-file key for the same tree, never one file under
  two presets. After any preset edit, two consecutive `make objdiff-config` runs must
  give the same per-unit flags; use `clone.sh new`, not `reset`; and re-diff every park
  in the unit, because a percentage is only valid under the preset that produced it.
- Not this project's compilers: `-mcpu=r8000`, gcc 2.8.1. No `-G0` in `world_map`.

## What objdiff cannot see

`objdiff 100.00%` + `MATCH` is not proof; only `make check` is. The ways a round has
broken an overlay with every function at 100:

- a body a few bytes off in size, or functions out of address order;
- a C `switch` in a unit whose tables are asm-owned `jtbl_` symbols;
- converting a callee, which let gcc shorten an already-matched **caller** by 8 bytes
  (keep the stub and the old `extern void f();`);
- park surgery that swallowed the next function or the next `INCLUDE_ASM` line (the
  symptom is `undefined reference` at link);
- a worker re-splitting an opaque field of a shared header.

## Harness facts

- Workers hit the 30-minute kill constantly. Fine if they committed; `merge` refuses
  uncommitted work. A lost body is still in `worker_transcript.jsonl` under
  `subagent-artifacts/`.
- pi-subagents "needs attention / no activity for 300s" notices are noise. A real
  failure reads `error:Subagent timed out`.
- `try_flags.sh` and `park_sweep.py` return `compile-failed` under load; re-run a
  surprising row before believing it.
- After editing a brief:
  `node -e "new Function('args','runs',require('fs').readFileSync('tools/rounds/brief.js','utf8'))"`.

## Where lessons go

So that the next session also starts from this file alone:

- a C idiom or a blocker a worker should know: one line in `brief.js`;
- something that changes what to brief or how to merge: edit this file, and delete
  what it supersedes;
- `reports/TRACKER.md` gets the round's row and at most five lines of per-unit detail.

## Ground rules

- `make check` green on all eleven is the only definition of done. Never edit
  `config/checksum.sha`, `expected/` or the extracted binaries. Never commit `asm/`,
  `build/`, `expected/`, `linker/`, `objdiff.json`, `build.ninja`, `disc/` or notes.
- No inline assembly in game code, no `#pragma`, no per-function flags. A function
  matches as C or stays `INCLUDE_ASM`. (`field/main/misc2.c`'s `$sp` switch and the
  `psyq/` GTE macros are the existing exceptions.)
- Ask the operator before anything that deletes committed work, changes a preset that
  matched functions depend on, or adds more than three files for one split.
- Report to the operator only: functions matched, functions named, the raw-line
  count, the tracker row, and any decision you need.
