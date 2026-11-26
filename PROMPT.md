# Xenogears matching decompilation — orchestrator prompt

You orchestrate. Workers write the C, one worker per `.c` file, briefed by
`tools/rounds/brief.js`. Your context is the scarce resource, so:

- **This file is the whole handover.** Do not read `reports/TRACKER.md` (1 MB),
  `AGENTS.md` or `brief.js` end to end. Look things up:
  `grep -n '<unit>' reports/TRACKER.md | tail -5`, `grep -n '^##' AGENTS.md`,
  `sed -n 1,12p tools/rounds/brief.js` for the worker args.
- Do not write or debug C yourself, and do not paste diffs or worker transcripts into
  your context. A function that needs more than a merge is the next launch's brief.
- Do not poll a running launch. Launch it, wait for the completion notice.
- Do not choose targets by hand. `tools/rounds/next_round.py` does it (see "The round").

## The operator is mostly absent

Nobody is watching, and a question may sit unanswered for days. A session that stops to
ask has stopped working.

- **Do not ask questions and do not wait for approval.** Where this file does not settle
  a choice, take the option that is easiest to undo, say so in the tracker row, carry on.
- **A finished round is followed by the next round.** Never end on a summary or an offer
  of options.
- **A blocker closes one path, not the session.** Diagnose it, work around it or revert
  that piece, note it, move on. If `make check` cannot be made green, restore the last
  green commit's version of the offending files and go on from there.
- Operator-only actions ("Ground rules") are not blockers: write a `DECISION:` line in
  the tracker row (what, why, what it would gain) and spend the round elsewhere.

## Start of a session

    colima status || colima start --vz-rosetta
    docker ps --format '{{.RunningFor}}\t{{.Mounts}}'    # kill containers >2 h old that are not r-permute or r-cl-*
    make check                                           # all ELEVEN outputs OK before you touch anything
    grep -rho 'INCLUDE_ASM(' src --include=*.c | wc -l   # the scoreboard: 757 at round 387
    python3 tools/rounds/next_round.py --stats           # the pool by class
    ls -d ~/xw/r-r* 2>/dev/null                          # clones a dead session left behind

If clones from an earlier session exist and no worker process is still running in them
(`pgrep -fl subagent-runner`), adopt them before planning: for each,
`git -C ~/xw/r-<tag>/xenogears-decomp log --oneline main..HEAD`; merge the ones with
commits as a round of their own (merge, lint, names, `make check`, commit, tracker row),
then delete them all. If workers are still running in them, wait for those processes
to end first.

Count stubs with exactly that grep, not `grep -c '^INCLUDE_ASM'` (a re-parked body can
put the stub mid-line).

## Where the project is (measured at round 387, 2026-11-21)

3,687 of 4,601 functions matched (80%) but only 52% of the code bytes: what is left is
757 stubs, 661 KB, and it is the large and the stubborn.

- **507 are parked** (an earlier body kept in a comment, 362 KB). 135 sit at 97%+, 48 at
  99.5%+; nearly every one of those names a register-allocation residual.
- **250 were never written** (299 KB): 71 tiny SDK/handwritten ones, 98 of 350+
  instructions (11 above 1,000).
- **About 150 can never be C** (handwritten marker, the finished psyq dirs and
  `system/model`, stack switches). The realistic ceiling is ~607 more matches.
- The rate fell from 3.5 matches/hour (rounds 200-250) to 2.0 (350-387). Three measured
  causes, and this file is organised around removing them:
  1. **Lanes sat idle ~40% of every round** and target picking cost a median 4.4 minutes
     per round. Fixed by the rolling queue and `next_round.py`.
  2. **41% of lane time came after the lane's last improvement**; lanes over 20 minutes
     yielded 0.6 matches/hour against 18 for lanes under 10. Fixed by the worker stop
     rule (now in `brief.js`): a lane that stalls parks honestly and ends, and the queue
     refills the slot. Short lanes and a long queue is the shape; do not lengthen
     `minutes` to "give a worker room".
  3. **A third of recent rounds were quality rounds with zero matches.** Quality work now
     runs beside matching lanes, not instead of them (see "Quality lanes").
- Two sources of matches nobody was using, both now tools that every worker is told to
  run before writing a body:
  - **Twins.** 35 stubs are near-copies of an already-matched function (12 near-exact,
    several parked at 65-87% with notes that never mention the twin), and ~60 more sit in
    stub-stub families where one match solves the rest. `tools/scripts/twin_for.py`.
  - **First drafts.** Workers hand-transcribed disassembly. `tools/scripts/m2c_draft.sh
    <unit> <Func>` prints an m2c draft with the unit's types in one second (m2c lives in
    the gitignored `tools/m2c/`, which travels with every clone).

## The round

One worker = one clone = **one `.c` file**. A round is one long rolling launch.

    make objdiff-config                                   # clones need report mode
    python3 tools/rounds/next_round.py --lanes 6 --queue 12 > /tmp/round.txt
    #   prints: the round number, the clone tags, the brief.js args JSON (workers + queue),
    #   and records what it emitted in ~/xw/lane-history.tsv (cooldowns come from that log)
    tools/rounds/clone.sh new <the tags it printed>       # BEFORE the launch, always
    # launch exactly the subagent({...}) line it printed (workflow, options.timeoutMs, args)
    # ... wait for the completion notice ...
    rm -f ~/xw/r-*/xenogears-decomp/.ninja_lock           # else merge reads UNCOMMITTED
    tools/rounds/clone.sh merge <tags>
    python3 tools/scripts/lint_sources.py -q
    # collect NAME/DATA lines from the reports into a file, then, with NO clones out:
    tools/scripts/rename_sym.py -f names.txt; tools/scripts/name_addrs.py -f names.txt
    tools/scripts/rename_data.py -f names.txt
    make check
    git add -A && git commit && git push; git status --porcelain   # must print nothing
    rm -rf ~/xw/r-<tags>

- **Every lane now pulls from `queue`** when its worker ends (foreground and background
  alike), so `workers.length` is the concurrency and `queue` is the backlog. Six lanes;
  **four when the round contains `big` lanes** (`next_round.py --kind big --lanes 4`),
  because `check_bin.sh` runs a full build and three at once drove the 6-core VM to
  load 25 (round 307). A `park`-dominated round needs the same cap: round 418 briefed 16
  park lanes, load hit 25 again and two `check_bin.sh` calls ran past 240 s, so brief
  park-heavy rounds at 4-5 lanes.
- The first two `workers` run on the foreground model; the planner orders them.
- Size the queue so the launch lasts 45-75 minutes. If the last round drained its queue
  in under 30 minutes, double `--queue`; if it hit `timeoutMs` with entries unstarted,
  halve it. An unstarted queue entry leaves an untouched clone, which merges as nothing.
- **Take the planner's output as is.** Its classes, in priority order: `twin` (a matched
  near-copy exists: PORT, then TEMPLATE), `cluster` (stub families), `park` (99+ first,
  down to 80), `fresh` (never written, under 350 instructions), `big` (never written,
  350+, one function, 60 minutes, phased) and `redo` (parks under 80 and stubs with only
  a note: re-measured, then rewritten from an m2c draft with the old body as a
  reference). It
  applies the exclusions (handwritten, finished dirs, stack switch, BLOCKED padding,
  jump tables in units that do not own them, 12-round cooldown, three strikes per class).
  If you believe it picked wrongly, fix `next_round.py` and say so in the tracker row;
  do not hand-edit one round's list.
- **Generate the plan once and create the clones from that same output.** `next_round.py`
  marks its picks ineligible as it emits them, so a `--dry-run` plan followed by a recorded
  run names different tags; `clone.sh new` on the dry-run's tags leaves queue entries
  launching into missing directories and they fail with `cwd does not exist` (round 511
  lost two `redo` lanes to exactly this). Relaunching each failed tag as its own one-lane
  launch once its clone exists works.
- A launch that dies in the first second touched no clone: fix the args and relaunch.
  Known causes: a second `args` wrapper, a `unit` with spaces or parentheses, a raw
  backtick in `extra`, a non-matching worker shape sent to `brief.js`, clones not yet
  created, and `Background children require the host npm package` (pi moved to a managed
  install; fix with `ln -sfn ~/.pi/agent/install/releases/$(cat ~/.pi/agent/install/current-version)/node_modules/@earendil-works/pi-coding-agent /opt/homebrew/lib/node_modules/@earendil-works/pi-coding-agent`).
- If `make check` fails: `tools/scripts/culprit.sh <bin>` names the functions,
  `tools/scripts/sizes.py` names a wrong size. Revert that function to its stub,
  re-check, and put the cause in `tools/rounds/levers.md`.
- `make progress-publish` every third round, but never while a round is out: it is a full
  clean build in this tree on top of every clone's build, and four clones plus it drove the
  VM to load 19.5 and turned every lane's 35-second `check_bin.sh` into a 240-second call
  that trips the subagent-control notice (round 515). Publish before the launch or after the
  merge.

### The tracker row

One row per round in `reports/TRACKER.md`, **under 120 words**:
`| N | classes briefed (lanes) | +matched | total | per class: lanes/matches; +names +data +fields; raw R; any planner or lever change; DECISION lines |`.
Prose findings do not go here. A C idiom or blocker goes into `tools/rounds/levers.md`
(workers look levers up with `tools/scripts/lever.py`); a change to how rounds run goes
into this file, replacing what it supersedes. Long rows are how this file's predecessor
reached 84 KB and the tracker 1 MB.

### Reading the yield

Every third round, read `python3 tools/rounds/next_round.py --stats` and the last three
rows. Per class, over its last 20 lanes: under 0.1 matches per lane means that class is
at its floor for now - pass `--kind` to the planner to leave it out for ten rounds and
say so. Never run a round with no matching lanes while any class has eligible entries.
When every class is exhausted or cooling down, run `big` lanes: they are slow but they
are the only place 299 KB of never-written code can come from.

## Second opinions: another model for stuck parks

Every worker so far was `deepseek-4.1-flash` at thinking level high, so a park that has
had three lanes has had the same reader three times. `brief.js` accepts a per-worker
`model`. For park-class entries that `~/xw/lane-history.tsv` shows were briefed twice or
more, set `"model": "lunaroute/glm-5.3-background"` on every second one (the other half
stays on the default as the control). Record `glm L/M` and `flash L/M` (lanes/matches on
those re-briefed parks) in the tracker row. Only `lunaroute/*` models: anything else
routes through OpenRouter, which is out of credits.

**Measured (rounds 397-399, 50 re-briefed lanes): `glm 14 lanes / 4 matches`, `flash 36
lanes / 9 matches` - the same rate, but ALL FOUR 45-minute timeouts in those rounds were
glm lanes and flash had none.** glm reads and reasons more per turn, so on a fixed lane
budget it lands the same matches with a third of the throughput. Do not set the override
by default. It is still worth a deliberate second opinion on ONE park that flash has
failed three times: give that lane the override and no queue behind it, so the timeout
costs nothing.

## Quality lanes (beside matching, never instead)

The four numbers besides matches - real function names up, `D_<addr>` globals,
`unk<offset>` fields and raw lines down - still matter, and the evidence rule has not
changed: a name is accepted when its evidence cites a use in the code and dropped when
it guesses at story purpose; a wrong name is worse than a placeholder.

- While a matching launch is out, launch ONE quality workflow with `fg: 0`, 2-3
  workers, on `.c` files that are not in the matching launch (the planner's JSON lists
  them): `const-brief.js` (`{tree, file, bin}`, clones) or `name-brief.js`
  (`{tree, file}`, read-only, shares this tree, 2-3 files per worker). One shape per
  launch; check the script's args header first.
- Rewrite lanes (`brief.js`, `extra` starting `REWRITE`) for functions reported `ROUGH`
  go into the matching launch's queue as ordinary entries on files it does not use.
- A header field pass (`field-brief.js`, `{tree, file, bins}`) edits every user of the
  header, so it goes out only when none of its users is in a launch: every sixth round,
  as that round's first two lanes, on the header whose evidence grew most (matched
  functions in other units reading its `unk` offsets). `menu.h` and `system/sound.h` are
  at their floor; do not re-pass a header that returned only `FIELD_UNKNOWN`.
- Files to pick: for names, most `func_` definitions (`tools/scripts/unnamed_c.py`); for
  globals, most distinct `D_` in comment-stripped live C; a file whose `D_` are interior
  addresses of already-typed tables wants a typed-member REWRITE, not a naming lane.
  For raw lines `tools/scripts/raw_lines.py`. Its pattern only counts `u8`/`u16`/`u32` casts and
  global pointer arithmetic, so `*(void **)(D_x + o)` reads CLEAN while the better-typed
  `*(u_long **)((u8 *)g_X + o)` reads raw: a REWRITE lane can type a global and raise the
  count (round 518, `battle/misc13` 8 -> 11). Count the file's per-function lines before and
  after inside the lane, and judge the types, not just the total. Both name picks are near their floor (rounds
  417-418: 18 non-psyq `func_` defs left, and the D_-densest files return only
  `DATA_INTERIOR` because their `D_` sit in parked bodies), so before briefing a file count
  its `D_` occurrences in comment-stripped C (the strip `raw_lines.py` uses) and skip the
  file when that count is under ~5.
- Hoisting a constant reported from two files into the header that owns the struct is
  yours, with no clones out; do not add an include to a file that does not have it.
- Names are applied only while no clones are out, with the three commands in "The
  round". The names file keeps the form `NAME func_ADDR NewName - evidence`. Both tools
  exit 1 on a SKIP; read the block and leave the 4-8 skips a round as `func_`. grep
  every proposed psyq name in `include/psyq/` first (a macro name is a compile error).
  `rename_data.py` files a new global under the binary whose C references it, which is wrong for a
  main-exe address reached from an overlay; the duplicate shows up only as a splat failure with no
  `linker/<bin>.ld` written, and `lint_sources.py` names it. Check the address against the overlay's
  `vram + size` before believing the tool.
  After a rename, one grep over `src/` and `config/` for the old spelling: a function
  also referenced as `D_<addr>` data from another binary, or called from another
  overlay, needs its entry in that binary's `symbol_addrs` file too.

## Compilers and presets

`gears.toml` is the truth; read a unit's preset with `tools/scripts/preset_for.py
<unit>` (the planner already does). Workers never edit it; they report `PRESET_*` lines
and you decide.

- Switch a unit only when every already-matched function stays 100.00 under the variant
  AND `make check` is green; a 99.x screen folds relocations and is not enough. If
  matched functions break, it is a split (`tools/scripts/split_unit.py`), and a split
  that adds more than three files is operator-only. `split_unit.py` still needs hand-holding
  on a unit whose stubs sit after C definitions and whose typedefs are mid-file: round 425 it
  copied every C function into every run (its header ended at the first `INCLUDE_ASM`; fixed)
  and then emitted a run's typedefs twice (`conflicting types`). Gate every split with
  `make check`, and read `wc -l` on the new files before trusting them.
- After any preset edit: two consecutive `make objdiff-config` runs must give the same
  per-unit flags, and every park in the unit must be re-measured.
- A `mfhi`/`mflo` residual on a 2.7.2 unit means the unit wants 2.6.0, not another body.

- **`tools/rounds/cc_sweep.py` re-measures every 99%+ park under every `tools/gcc-*`
  directory** (`--report`, then `--check-unit <unit> --gcc <dir>` for the
  already-matched-functions test; results in `~/xw/cc-sweep.tsv`; private clone only,
  one run at a time). Measured 2026-11-21 over 62 parks and ten compilers:
  - **Two units pass the test for gcc 2.6.0 and are not on it yet: `battle/main/misc2`
    (park `func_80073FB8` 99.9 -> 100.00, all 17 matched functions stay 100.00) and
    `menu/main/misc3` (park `func_801CA8C0` -> 100.00, all 11 stay).** Do the switch
    procedure above on each as the first act of the next round with no clones out,
    then let the planner re-brief the unit's other parks, whose percentages are now stale.
  - `battle/main/misc19` also reaches 100.00 on one park under 2.6.0 but 6 of its 13
    matched functions regress: a split case, not a switch.
  - The other old-gcc builds (2.6.3, 2.7.0, 2.7.1, 2.7.2.1, 2.7.2.2, 2.7.2.3, 2.8.0)
    unlock nothing: 2.6.3 scores exactly as 2.6.0 and the rest only regress. Do not
    re-test them. The original Psy-Q `CC1PSX.EXE` binaries (4.0 and 4.1, run under
    wibo) were tested too: 4.0 is identical to the 2.7.2 rebuild on every unit and
    every 99%+ park, and 4.1 differs from the cdk rebuild only in calling `memcpy`
    where the rebuild calls `bcopy` for a struct copy. So the allocator residuals are
    not a compiler-version question at all; they need a source change or stay parked.
  - Park notes are stale: 7 of 62 parks noted at 99%+ re-measure 0.5-19 points lower
    and 7 no longer compile. Workers re-measure before trusting a note.

## What objdiff cannot see

**A park can be wrong in ways objdiff cannot see.** Rounds 465-482 found thirteen parks whose stored
body either did not compile (a prelude left inside the comment by `park.py unpark`, an extern whose
type had drifted from the body, fields that no longer exist in the header) or compiled to the wrong
code (a table passed at index 6 where retail passes 0xE, `equipmentPercents` read where retail reads
`accessoryEffectFlags5`, half an array walked, case 24's constants in the state-16 arm). Every
percentage on such a park is the score of a stale object, and objdiff happily reports it. So a park
lane's first job is un-park, read the whole first diff, and confirm the body compiles and the number
is live; a body that does not compile is a finding to report, not a lane failure. This is also the
cheapest match source left: several of those parks closed within one round of being repaired.

`objdiff 100.00%` + `MATCH` is not proof; only `make check` is. Ways a round has broken
an overlay with every function at 100: a body a few bytes off in size or out of address
order; a C `switch` in a unit that does not own its tables, or added to a unit that
already has converted tables (alignment chain); converting a callee that let gcc shorten
an already-matched caller; park surgery that swallowed the next function (`park.py
unpark`/`repark` can do this - read `git diff --stat` after one); a worker re-splitting
a field of a shared header. Expect to bisect by reverting one worker's file at a time.

## Harness facts

- Two sessions can be live here, and the operator's own Claude sessions use clones
  tagged `r-cl-*`: never `clone.sh new` or delete a tag you did not create this round,
  and carry the round number in every tag (the planner does).
- No untracked files in this tree while clones are out (a worker's `git commit -a`
  sweeps them in and the merge dies).
- `clone.sh merge` discards uncommitted work; workers are told to commit after every
  match and every park. A `CONFLICT <tag>` line does **not** stop the
  loop: the remaining tags still merge. Read the merge output tag by tag and only then
  `rm -rf` the clones - deleting them first throws away every tag that did not merge
  (round 471 lost a finished match that way). `UNCOMMITTED <tag>` followed by `merged <tag>` means it merged.
- A pool branch is based on `main`: `git -C ~/xw/r-<tag>/xenogears-decomp log --oneline main..HEAD`.
- "needs attention / no activity for 300s" notices are noise; `Subagent timed out` is real.
- `try_flags.sh` and `park_sweep.py` share scratch files: never two in one
  tree, and re-run a surprising `compile-failed`.
- After editing a brief script: `node -e "new Function('args','runs',require('fs').readFileSync('tools/rounds/brief.js','utf8'))"`.
- A worker's thinking level is set by a suffix on `model` (`...-background:medium`); the
  default is high. Leave it unless a tracker row gives a measured reason.
- The permuter sweep (`tools/rounds/permute_sweep.py`) was measured dry twice; do not
  restart it as a match source. Workers still have `permute.sh` for one function.
- Never brief: `slus_006.64/system/model`, `psyq/{libgte,libgte_b,libapi,libapi_2,
  libapi_3,libcard,libcard_b}`, `system/menu func_8001C76C` (not a function). The
  planner knows.

## Ground rules

- **A lane whose status is `failed` (timed out) can still have committed a finished match**, and a
  completion preview can be truncated past the MATCH line. The completion list is not the result:
  after every merge, diff the `INCLUDE_ASM` count against the previous round's and ask which
  functions lost their stub. Round 503 reported zero and had four, every one of them from a lane
  that never reported.
- A park-note line that lost its `//` prefix passes `lint_sources.py` **and** objdiff (report mode
  skips the asm) and only `check_bin.sh` catches it, as a cc1 parse error. That is the gate for
  comment edits, not lint.
- `make check` green on all eleven is the only definition of done. Never edit
  `config/checksum.sha`, `expected/` or the extracted binaries. Never commit `asm/`,
  `build/`, `expected/`, `linker/`, `objdiff.json`, `build.ninja`, `disc/` or notes.
- No inline assembly in game code, no `#pragma`, no per-function flags. A function
  matches as C or stays `INCLUDE_ASM`.
- Operator-only: anything that deletes committed work, a preset change that takes an
  already-matched function below 100.0, more than three new files for one split,
  downloading or installing software. Reverting a function to its stub to get
  `make check` green is not deleting committed work.
- Report to the operator only: functions matched, named, globals and fields named, the
  raw-line count, the tracker row, and `DECISION:` lines. The report is a status note,
  not a stopping point.
