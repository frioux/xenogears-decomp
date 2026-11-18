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

## The operator is mostly absent

Nobody is watching this session, and a question may sit unanswered for days. A session
that stops to ask has stopped working. So:

- **Do not ask questions and do not wait for approval.** Where this file, `brief.js` or
  the tracker does not settle a choice, take the option that is easiest to undo, say
  what you chose and why in the tracker row, and carry on.
- **A finished round is followed by the next round.** Do not end on a summary or an
  offer of options. Keep going until there is nothing left to brief under "Choosing
  what to brief", and then run naming and rewrite rounds over the backlogs.
- **A blocker closes one path, not the session.** A broken unit, a failing worker, a
  tool error: diagnose it, work around it or revert that piece, note it, and move to
  the work that does not depend on it. If `make check` cannot be made green, revert to
  the last green commit's state for the offending files and go on from there.
- **The three operator-only actions under "Ground rules" are not blockers either.** Do
  not take them and do not wait for a yes: write a `DECISION:` line in the tracker row
  (what, why, what it would gain) and spend the round on something else. The operator
  reads those lines on return.

Start:

    colima status || colima start --vz-rosetta
    make check              # all ELEVEN outputs OK before you touch anything
    grep -rc '^INCLUDE_ASM' src | awk -F'[/:]' '{n[$2]+=$NF} END{for(b in n) print n[b], b}' | sort -rn

That last line is the scoreboard (stubs per binary; 4568 functions in total). Four
numbers move in this job: **functions matched** and **matched functions with a real
name** go up, and **raw lines** (matched C that no programmer would have written; see
"Realistic C") and **placeholder names** (`D_<addr>` globals and `unk<offset>` fields
in matched C; see "Names for data and fields") go down.

## Order of work

Rounds 45-51 spent about 90 matching workers on 19 matches. The stubs a 30-minute worker
can write from nothing are used up, and re-briefing the same units does not bring them
back. Every large gain in this project came from removing a blocker for many functions
at once (the round-34 splits, the gcc 2.6.0 and cdk presets), so that is the work now.
Take the first item below that has work; 1-3 are finite queues that three tools opened
on 2026-11-14, and each is described under its own heading further down.

1. **Fresh-stub deep round** ("Choosing what to brief", "The round loop"). One worker per
   unit on the 350-1200 instruction stubs `fresh_stubs.py` lists, 60 minutes each. This
   is where the matches have come from since round 216: a 480-instruction body matched,
   and a 431+441 pair came in from one worker. Four workers a round, not eight.
   Both preset harvests under this heading are **closed** as of round 220, do not re-run them:
   the cdk -G0 queue (`battle/misc26/29/31` switched in round 55) is exhausted -
   `misc27` has one park at 90.55 under both -G8 and -G0, `misc27_b` has no park at all
   (5 stubs: 2 never written, 3 the round-148 `sw $sp` coroutine saves); and the permuter
   queue is dry (see item 2).
2. **Permuter harvest** ("The permuter"). A CPU-only search over every parked body runs
   in the background; when its report lists ZERO or BETTER results, an apply round ports
   them. **The sweep is dry as of round 217, do not restart it as a match source:** its
   8 results bought 0 matches (one ZERO was wrong code - the candidate read
   `extraWeaponIds[3]` where retail reads `[0]` - one had already been ported, two were
   on the never-brief list, and four improved a park without matching).
3. **Jump-table stubs** ("Jump tables"). About 110 stubs that were excluded because a C
   `switch` broke the link are ordinary stubs now. They are larger than average, so
   brief them in the deep shape ("The round loop").
4. **Naming and rewrite rounds** ("Names", "Names for data and fields", "Realistic
   C"), alongside any of the above. These are the reliable product: run one with every
   matching round.
5. **Fresh-stub matching rounds** ("Choosing what to brief"), on what `fresh_stubs.py`
   still lists.

**Stop rule for matching rounds.** A matching round that lands fewer than 3 functions
is a signal, not bad luck. After two in a row from the same kind of brief, do not launch
a third: move down or up this list, and if 1-3 are empty and 5 is yielding nothing, run
only naming and rewrite rounds and say so in the tracker row. Tokens spent re-attempting
a saturated pool are the one thing this file exists to prevent.

## The round loop

One worker = one clone = **one `.c` file**.

    make objdiff-config                          # clones need report mode
    tools/rounds/clone.sh new a b c ...          # cp -ac clones in ~/xw/r-<tag>
    # subagent({workflow: "tools/rounds/brief.js", async: true, args: {workers: [
    #   {tree, file, unit, asm, binary, overlay, preset, symbols, funcs, extra}, ...],
    #   queue: [2-4 more of the same shape]}})   # see "Foreground lanes"
    # ... ~30 min, workers are killed at 30 ...
    rm -f ~/xw/r-*/xenogears-decomp/.ninja_lock  # else merge sees UNCOMMITTED and drops the clone
    tools/rounds/clone.sh merge a b c ...
    python3 tools/scripts/lint_sources.py -q
    # save the reports' NAME lines to a file, then:
    tools/scripts/rename_sym.py -f <names file>  # see "Names"
    tools/scripts/rename_data.py -f <names file> # see "Names for data and fields"
    make check
    git commit, git push, one row in reports/TRACKER.md
    rm -rf ~/xw/r-<tags>                         # fresh `new` clones next round

**Two worker shapes.** The default is the 30-minute worker with a list of small stubs.
For functions over ~150 instructions (most of what is left, and nearly every jump-table
stub) use the **deep shape**: 1-3 functions per worker, `minutes: 60` in the worker's
args and the same limit on the launch -
`subagent({workflow: "tools/rounds/brief.js", async: true, options: {timeoutMs: 3600000}, args: {workers: [...]}})` -
and at most 8 workers, because each one also runs the permuter (`-j 2`) on the 6-core
build VM. The brief tells a worker to commit after every match, so a kill at 60 loses
little. Judge the deep shape by matched instructions, not function count. **A port round
(`permute-brief.js`) is the same constraint:** each worker runs the permuter for real, so
7 port workers plus the sweep drove the load to 22 on 6 cores and several workers spent
4+ minutes in one compile. Cap a port round at 5 workers, or pause the sweep for it.

**Foreground lanes.** The plan allows two foreground (not `-background`) agents at a
time, and an idle foreground slot is wasted capacity, so every round script under
`tools/rounds/` keeps both busy: the FIRST TWO entries of `workers` run on
`lunaroute/deepseek-4.1-flash`, the rest on the `-background` default, and each time a
foreground worker ends its lane starts the next entry of `args.queue` immediately. So:

- Put the two workers you most want finished first in `workers`, and pass a `queue` of
  2-4 more (same shape, each with **its own clone and its own `.c` file**, distinct from
  every other worker in the round). `clone.sh new` them with the rest and merge them
  with the rest; a queued worker that never started leaves an untouched clone, which
  merges as nothing.
- The queue does not raise the load: only `workers.length` run at once, so the 8-worker
  and 5-worker caps above count `workers`, not `workers` + `queue`.
- Size the queue from the last round's call trace. If both lanes drained the queue well
  before the background workers ended, queue more next time; if the round ended on a
  queued worker long after the background ones, queue fewer.
- `options.timeoutMs` on the launch is the deadline of the whole round, and the script
  already kills each worker at its own `minutes`. With a queue on a deep round, set it to
  cover the chain: `minutes * 60000 * (1 + ceil(queue.length / 2))`. On a default
  30-minute round leave it out, as before.
- A naming round (`name-brief.js`) launched while a matching round is out would take the
  same two slots: pass `fg: 0` to the one that matters less.

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
  **That tool had three bugs, fixed in round 221 after they cost three rounds:** its owner
  binary came from `parts[1]` of an ABSOLUTE path (always `Users`), so `b == owner` was
  never true and it added NO entry for any rename; its entry regex required a single
  space, so the 84 column-aligned entries were invisible to its duplicate guards; and an
  existing `func_<addr> = 0x<addr>;` line blocked the new name instead of being
  rewritten. `tools/scripts/lint_sources.py` now also fails on a non-static C definition
  with no entry in its own binary's file, and on an entry that duplicates one
  `symbol_addrs.slus_006.64.txt` already has - **that second one makes splat abort ONE
  binary ("Duplicate symbol detected"), gears ignores the exit code, and the build dies
  with `ld: cannot open linker script file linker/<bin>.ld`**, naming neither the symbol
  nor the file. Four matched-but-invisible functions were repaired this way.
- **The names file must keep the three-column form**, `NAME func_ADDR NewName - evidence`. A file
  stripped to two columns still renames (rename_sym.py parses it) but makes `name_addrs.py` a silent
  no-op, so every function whose address has no `symbol_addrs` entry - all of `member_change_menu` -
  stays `func_` with no error. Round 41 lost nothing only because it noticed the "renamed 0 pairs"
  line; keep the evidence text in the file you feed both tools.
- **Backlog: use `tools/scripts/unnamed_c.py`** - it counts a `func_` as a definition
  only when a depth-0 signature is followed by `{` before any `;`, so prototypes, call
  sites and parked bodies do not count. On 2026-11-16 that is **49 in 23 files**, 29 of
  them under `psyq/`, i.e. the non-SDK function-naming backlog is about 20 functions:
  the function half of naming is done, and the `D_` global and `unk` field half is
  where the work is. (The old recipe - comment strip plus dropping `extern` lines -
  counted calls and prototypes as definitions and reported 277; round 153 handed
  workers files with "20 unnamed definitions" that define none, and they said so.)
  While a
  matching round is out, run a naming round over them with
  `tools/rounds/name-brief.js` (`{workers: [{tree: <this repo>, file}, ...]}`). It is
  read-only, so workers share this tree and need no clones. **Give a worker 2-3 files,
  not six:** round 138 handed seven workers six files each and six of them timed out at
  30 minutes with nothing in their output file, only the template (the one that finished
  reported 46 NAME/DATA lines). `file` may be a comma-separated list - the run key is
  sanitised - so a naming round is many small workers, not a few big ones. Pick the files with the
  most unnamed definitions that are **not** in the current matching round, hold the
  NAME lines, and apply them after the matching round has merged, with the two commands
  above. A naming worker yields ~10 names on a well-picked file and 0
  on a dud, so budget for a few dozen, not a few hundred; a worker given a file whose
  `D_` globals its live C never touches comes back empty.
- **Feed the tools the union of every NAME line ever reported.** Both tools self-skip
  (already applied, name taken, still a stub, address twin), so `grep -h '^NAME func_'`
  over every `*_worker_output.md` in `~/.pi/agent/sessions/<project>/subagent-artifacts/`
  is safe, and it collects the proposals earlier rounds never landed: round 49 applied 177
  names from a 1753-line union, ~94 of them from earlier rounds. Keep duplicates out by
  one line per address (a coin toss between two evidence-backed names is fine; the loser
  is one `rename_sym.py Old=New`).
- **A rename leaves `D_<addr>` behind.** When the named function is also referenced from
  another binary as a *data* symbol (`extern u8 D_80025A88[]` for a main-exe callback in
  `battle/main/misc28`), neither tool touches that C: the overlay's asm relocations get the
  new name, the old spelling has no definition, and `make check` fails with `undefined
  reference to D_80025A88`. One grep per applied name over `src/` **and `config/`** for both
  spellings - `name_addrs.py` can rename the C and leave the old spelling in place as an
  existing `symbol_addrs` entry (`battle` `func_800AA934`, round 139: the entry still said
  `func_800AA934 = 0x800AA934;`, so splat kept naming the stubs' calls with it and the link
  failed until the entry itself was edited). The overlay-to-overlay twin is the same
  failure from the other side: round 141 renamed a `lib_gear` function that
  `field/main/misc6` calls, `rename_data`/`name_addrs` put the new name only in
  `symbol_addrs.lib_gear.txt`, and `field` linked with `undefined reference to
  LibGearFreeModel` - the caller's symbol file needs the entry too
  (`LibGearFreeModel = 0x801E8030;` in `symbol_addrs.field.txt`, beside the
  `LibGearSetModelAnimation` entry already there).
- Tracker row: `+N matched, +M named, +D data, +F fields, raw R`.

## Names for data and fields

**`D_800D2CB0` and `u8 unk4[0x18]` are placeholders, not names.** A function called
`BattleCopyRowTiles` that reads `D_800D2CB0[i]` into `pRow->unk4` is half named. The
same rule as for functions applies to what they touch: once the code shows what a
global or a field holds, it gets the name a programmer would have typed
(`g_BattleRowTiles[i]`, `pRow->tiles`), and it gets it in a follow-up pass, not during
the match.

The evidence rule does not relax. A name is accepted when its evidence cites a use in
the code (what is stored, what indexes it, which named function takes it as which
argument, a string's text) and dropped when it is a guess at what the game means. `hp`
on a guess is still worse than `unk3D`. What changes is that `unk3D` with the evidence
sitting in three matched functions is now a debt, like `func_`. A name that restates
the type or the address (`g_BattleByteTable`, `field4`) is not a name; leave the
placeholder.

**Globals: `D_<addr>` -> `g_Name`.** Proposed by workers, applied by you, exactly like
function names.

- Matching and rewrite workers add `DATA D_800XXXXX g_NewName - evidence` lines to
  their report (rules in `brief.js`), and `name-brief.js` workers propose them for every
  `D_` the live C of their file uses. Nobody renames a global in a clone: the expected
  objects still carry the old name.
- Apply with `tools/scripts/rename_data.py -f names.txt`, after `rename_sym.py` and
  `name_addrs.py`, under the same condition: **no clones out**. It replaces the name in
  `src/` and `include/` and adds `g_Name = 0xADDR;` to the owner's `symbol_addrs` file,
  which is what makes the asm of every stub that still loads the address say the new
  name (a sed over `src/` alone is `undefined reference to g_Name`). Below the overlay
  base (`0x8006FAF0`) the owner is the main exe and the name applies in every binary;
  above it the address means something different in each overlay, so a `D_` used from
  two binaries' `src/` is skipped as a twin. It exits 1 on a SKIP; read the block. The
  same file can carry `NAME` and `DATA` lines, each tool reads its own.
- `DATA_INTERIOR` lines are not names. `D_800D343F` used as `D_800D343F + i * 0x40` is
  field 0x3F of the table at `D_800D3400`; naming it would freeze the raw form. Those go
  to a rewrite round ("Realistic C") as the struct that file needs, and the *base* gets
  the name.
- A typed table keeps working after its base is renamed: the asm still names the
  interior addresses `D_800D3420` and the C says `g_Slots[i].unk20`, which is the 99.x%
  objdiff reading "Realistic C" already describes.
- Do not name under `psyq/` except with a real SDK name.
- **The Psy-Q sig dump exists now, so psyq naming is a tool run, not a reading job.**
  `~/xw/psyq-sigs/` holds `sig33|35|36|37|40|43.txt` (one line per exported SDK
  function across six Psy-Q versions, every relocated word marked 0xDEADBEEF) and
  `sigdump.rs`, the ~60-line psy-k bin that produced them. Run
  `tools/scripts/psyq_names.py ~/xw/psyq-sigs/sig*.txt` and it prints, per repo
  function, every version that matched and the name it matched. Round 205 landed 7
  names this way (unnamed psyq C definitions 29 -> 22) and left the rest: a name is
  only trustworthy when it is identical in every version that matched, and the
  remaining exports are hand-patched (the libgte/libapi BIOS region, Sony's
  `vector.s`) or version-ambiguous (`PAD_init` 3.3/3.5 vs `PAD_init2` 3.6+). The
  three rejected and three STUB_NAME proposals are in that round's outputs.
  To rebuild the dumps if they are lost: clone `sozud/psy-q` (3.3/3.5/3.6/3.7/4.0/4.3)
  and `ttkb-oss/psy-k`, add `XDEF::section/offset` + `Patch::offset` accessors (the
  fields are private), and mark patch offsets **relative to the current Code block**,
  not the section - section-relative masks up to 93 of 97 words of `ResetGraph` and
  makes the matcher name functions wrongly.
- Two `name_addrs.py` bugs this exposed, both fixed in round 205: "new name already
  used" now means another function is *defined* under it (a bare word search refused
  every SDK name, because `include/psyq/*.h` already declares the prototype of the
  function being named), and the owner binary is `parts[1]` (it was `parts[-3]`, which
  is `"psyq"` for every `src/slus_006.64/psyq/*.c`, so psyq renames silently wrote no
  `symbol_addrs` entry - the round-36 link-failure class).

**Fields: `unk<offset>` -> a name.** Applied by a worker in a clone, because a field
rename has to follow the struct's type through every user and only the compiler can
check that.

- `tools/rounds/field-brief.js`, `{workers: [{tree, file, bins, structs?, hints?}]}`:
  one worker per **declaring file** (the `.h` or `.c` that holds the struct), `bins` the
  outputs whose `src/` uses it. The worker renames in the declaration and at every use,
  splits an opaque span (`u8 unk4[0x18]` reached into with casts) into the fields the
  code touches, proves it with `check_bin.sh` per bin, commits, and reports `FIELD`,
  `SPLIT` and `FIELD_UNKNOWN` lines. Merge with `clone.sh merge` like any round. Field
  names are not in the object code, so only a split can move a byte and `make check`
  catches it.
- Matching workers rename fields of structs declared in their own `.c` as they go, and
  report `FIELD Struct.unkN name - evidence` for structs in shared headers without
  touching them (the shared-header rule in `brief.js` stands). Pass those lines to the
  field worker as `hints`.
- **A field worker owns its declaring file and edits its users**, so it must not share
  a round with a matching or rewrite worker on any `.c` that includes that header. A
  struct declared in a `.c` is that file's worker alone and can go out whenever the
  file is free; a header under `include/` goes out between matching rounds, or in a
  round whose other workers are all on other binaries. Two field workers whose headers
  share a user merge cleanly unless they touched the same line; a CONFLICT clone is
  re-briefed next round.
- **A `.c` that declares no structs of its own is not a field worker.** Round 179 gave
  one `src/menu/main/misc1.c`, which only allocates header types, so it worked
  `include/system/menu.h` - the same header the round's header worker was renaming -
  and the merge hit three CONFLICTs. Check the declaring file first (`grep -c '"i.e. typedef struct"'`)
  and if it has none, brief it with `bins` on a header no other worker has.
- Four passes over the three big headers (`actor.h`, `menu.h`, `game.h`) took `unk`
  1966 -> 1692 in one session, so this is the paying queue when matching stalls. The
  evidence that works: a field named from a change in TWO independent compilations of one
  layout (GameCharacter vs GameGear's accessory switch, actor.h vs libgear's model),
  from a load/store WIDTH in a stub's asm (which forces a SPLIT), and from a matched
  function in ANOTHER file reading the same offset.
- `FIELD_UNKNOWN` is a fine outcome and is not re-briefed until new matched functions
  use the struct.

**Backlog and what to pick** (when this was written: 2,574 distinct `D_` names in
`src/` and `include/`, about 2,600 `unk` field declarations):

    grep -rhoE '\bD_[0-9A-F]{8}\b' src include | sort -u | wc -l
    grep -rcE '/\* 0x[0-9A-Fa-f]+ \*/.*\bunk' src include | awk -F: '$2 {n+=$2; print $2, $1} END {print n, "TOTAL"}' | sort -rn | head -21

Pick by evidence, not by count: a global or struct used by several *named, matched*
functions names itself, and one touched only by stubs or `func_` bodies yields
`UNKNOWN`. So run the function naming round over a file first and the data and field
rounds over it afterwards. Put both totals in the tracker row every third round; like
the raw count, they must not rise except by the placeholders a newly matched function
brings with it.

## Named constants

**`if (stat > 0x63) stat = 0x63;` is a placeholder too.** A number with a meaning is
written as a named constant (`MENU_STAT_PERCENT_MAX`), in decimal when it is a
quantity; the rule and its limits are in AGENTS.md section 4, "Constants". The
evidence rule is the one for names: the use has to show what the number is, a name
that restates the value is rejected, and a literal whose meaning nobody can show
stays a literal.

- Matching and rewrite workers define constants at the top of their own `.c` as they
  go and report `CONST NAME value - evidence` (rules in `brief.js`). They never add
  one to a header under `include/`.
- The backlog goes out as its own round: `tools/rounds/const-brief.js`,
  `{workers: [{tree, file, bin, funcs?, hints?}]}`, one worker per `.c` file in a
  clone, proved with `check_bin.sh <bin>` and merged with `clone.sh merge`. A worker
  owns its `.c` only, so the round can share clones' time with field rounds on other
  binaries but not with a matching or rewrite worker on the same file.
- **Hoisting is yours.** A `CONST_SHARED` line, or the same `CONST` reported from two
  files, means the `#define` belongs in the header beside the struct or table it
  describes. Move it there with **no clones out**, delete the per-file copies, and let
  `make check` prove it.
- Pick files the way names are picked: ones whose functions are matched *and named*,
  because a constant's meaning comes from the named things around it. This lists
  candidates (about 4,400 lines when this was written; it also counts parked bodies,
  so it is a picker, not a tracker number):

      grep -rcE '(==|!=|[<>]=?|case|[^=!<>]=) 0x[0-9A-Fa-f]+\b' src | awk -F: '$2 {print $2, $1}' | sort -rn | head -20

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
- **The backlog: use `tools/scripts/raw_lines.py`, not a grep** (**284 live lines in 68 files** on 2026-11-16).

      tools/scripts/raw_lines.py                      # total, then the worst files
      tools/scripts/raw_lines.py battle/main/misc5    # per-function counts

  The tool strips `/* ... */` first, so parked bodies do not count and are never
  attributed to a function - which the plain grep below does, and that is how round
  151 briefed four rewrite workers on `INCLUDE_ASM` stubs whose "raw lines" and
  "matched functions" were parks (one of them, `world_map/misc10`, has **zero** live
  raw lines). The old grep, for the historical numbers only:

      grep -rcE 'D_[0-9A-F]{8} \+ \w+ \* |\*\((const )?[us](8|16|32) ?\*+\)\(|\(\*\w+\)\[|\(\(?[us](8|16|32) ?\*\)\w+\)? \+ ' src

  Put the tool's total in the tracker row (`raw N`). It must not rise across a round: a
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
  A field whose uses do show what it is gets its name in a field round ("Names for data
  and fields"), which is also where an opaque `u8 unk4[0x18]` span is split.
- Fix the cause when a round's reports show the same raw form coming back: one line in
  `brief.js` with the typed spelling that matched.

## The permuter

`tools/scripts/permute.sh <unit> <Func> [--minutes N] [-j N]` runs decomp-permuter on one
function: thousands of small random edits to its C (new temporaries, reordered
declarations and statements, swapped operands, changed integer types), each compiled
with the unit's real compiler and scored against the original. It reads the body from
the source, parked or live, and needs report mode. A score of 0 is identical; a body one
register off scores about 40. This is the tool for the parks this file used to call dead:
the reason they were dead is that hand-written shapes do not move an allocator tie, and
a search that tries several shapes a second does. Workers have it in every brief.

What is measured so far (2026-11-14, five bodies, on a loaded VM at 3-8 candidates a
second): `battle/misc20` `func_8009E5C8`, parked at 99.91% with "8 shapes tried", went
to 0 in 26 seconds and is a real match (`MATCH`, objdiff 100.00%; the edit was one
hoisted `rowOffset = i * 0x170` local). `battle/misc8` `func_8007CDD0` went 40 -> 30 in
a minute. `battle/misc16` `func_8008DC34` (score 10) found nothing in 3 minutes. So it
closes some parks quickly and not others; the yield over the whole set is not known
yet. **Keep the count** (ZERO results per pass, and how many of them a worker turned
into a gated match) in the tracker: if a full pass yields under ~10 matches, stop
re-running it and say so here; if it yields dozens, a longer second pass is the best
use of idle CPU there is.

**The sweep** runs it over every parked body (494 when this was written), CPU only:

    pgrep -f permute_sweep.py || nohup tools/rounds/permute_sweep.py --minutes 3 >> ~/xw/permute-sweep.log 2>&1 </dev/null &
    tools/rounds/permute_sweep.py --report             # ZERO / BETTER results for bodies still parked
    tools/rounds/permute_sweep.py --report --briefs    # the same as permute-brief.js worker args

- It works in its own clone (`~/xw/r-permute`, rebuilt from this tree at each start; never
  `clone.sh clean` it away mid-sweep, and never use the tag `permute`), scores every body
  first, then searches the closest first (`--max-base 300`). Results are in
  `~/xw/permuter-results.tsv` and `~/xw/permuter-out/<Func>.diff`, keyed by a hash of the
  parked body, so a restart skips what is done and a re-parked body is tried again.
- Start it at session start and after any round that adds or changes parks. It may run
  during a round (`-j 4` of 6 cores; drop to `-j 2` while a deep round is out).
- **Apply round:** when `--report` lists results, `clone.sh new` one clone per unit, fill
  `tree` into the `--briefs` JSON and launch `tools/rounds/permute-brief.js` with it.
  ZERO results are a port-and-gate job; BETTER results are leads the worker continues
  from with its own permuter runs. Merge and gate exactly like a matching round.
- `ERROR` rows are bodies that no longer compile on their own, or that use GTE macros or
  inline asm the permuter's C parser rejects; `~/xw/permuter-out/<Func>.error` says which.
  A stale body is worth one line in a normal brief ("the park does not compile"); the
  rest are not permuter work. **Most ERROR rows are stale, so they are not a repair
  queue** - round 185 spent 7 workers on the 117 worst ones and every park but
  `battle/misc8`'s three already built; only a worker that un-parks the body and runs
  `diff_func.sh` can tell (budget them as re-measure jobs, not matches). A park that
  reports `Syntax error in base.c` here is usually one `park.py unpark` cannot read at
  all: a signature left on the `/*` line (`/* int func_x(int index) {`) or a note block
  with no closing `*/` of its own, so note+body is one comment. Un-park those by hand.
- **A ZERO is not proof of a match: the permuter does not score stack offsets.** A park
  can read `best=0` with the instruction stream and size exact and still differ in the
  frame layout (`battling/misc3` `func_8007661C` has been parked at 99.91% on exactly
  that, and three separate bodies in `permuter-results.tsv` carry base 0 today). Port it,
  then `diff_func.sh` and `check_bin.sh` decide, as always.
- A jump-table function never reaches 0 (the original names `jtbl_X`, the candidate
  `.rodata`); a low BETTER score there is the result, and `check_bin.sh` is the judge.
- After a sweep has covered everything at 3 minutes, a second pass with `--minutes 10
  --retry --max-base 120` on what stayed NONE is worth one night of CPU. Record in the
  tracker how many ZERO results a pass produced, so the next session knows whether a
  longer pass paid.

## Jump tables

`tools/scripts/own_rodata.py` gives a unit its own switch tables (AGENTS.md, "Switch jump
tables in overlays"), and it has been applied to 49 units of `battle`, `battling`,
`world_map` and `menu` with `make check` green. In those units a C `switch` links; the
brief explains to the worker how a jump-table match reads in `diff_func.sh` and that
`check_bin.sh` decides. `fresh_stubs.py` now lists those stubs.

- Not covered: `battling/main/misc2`, `menu/main/misc1`, `movie/main/misc1` (a string
  sits inside the table range; `own_rodata.py --report <bin>` says FOREIGN). Their
  jump-table stubs stay out of `funcs`. `field` and `slus_006.64` units already had
  per-unit `.rodata`.

Round 56 retired most of this queue, and the reason is **layout, not C**: five workers
hit it and none converted a switch. Two pre-conversion checks decide a jtbl stub, and
neither is visible in objdiff, so put them in `funcs` filters rather than in a brief:
count the stub's `.s` table words against the case count (retail pads with a trailing
0 word - `battle/misc23` `func_800A6884` is six words for five cases, so the overlay
loses 4 bytes and `convertible.py`'s BLOCKED is right), and check the table's alignment
**relative to the unit's `.rodata` range, not its vram address**: cc1's `.align 3` aligns
against the section start, so a table is producible only when `(table address - unit
.rodata start) % 8 == 0`. Round 171 screened by vram address, took `battle/misc11`'s five
98-99.9% parks (its range starts at 0x8006FE7C, 4 mod 8, and every table sits at +0x194 +
n*0x20, so all five are 4 mod 8 in section space), converted func_800816F8 to an
instruction-identical 420 bytes and got `check_bin battle` MISMATCH, 343940 vs 343936: one
pad word each, unfixable from C. `world_map/misc12`'s `func_80090C68`/`90E14` (99.91%
instruction-identical, +4 bytes) are the same measurement made the hard way. A unit's
tables are a chain - the first table's size sets the parity of every one after it - so
either the whole switch family converts in address order, first table at +0 (**round
171: `world_map/misc15`, three tables at retail's +0/+0x18/+0x48, three matches, one
100.00% sibling`), or none of them does.
- Run `own_rodata.py` on a unit BEFORE any of its jump-table stubs is converted; it
  finds the tables through the stubs' `.s`. For a unit split after today, run it on the
  new unit, then `make check` with no C change.
- Untested corner: a converted function that also has its own string literals. If
  `check_bin.sh` fails on one, revert it and note the unit.

## Choosing what to brief

Measured over ten sessions; the brief decides the yield (3 per round with units picked
by stub count, 10 to 16 with the method below).

1. Rank units by **instructions per fresh stub**, smallest first:
   `python3 tools/scripts/select_target.py --json` (ignore its `attempt` flag). Stubs
   under ~120 instructions are where the matches are.
   `python3 tools/scripts/ghidra_info.py --leaves --bin <bin>` orders the same stubs
   leaf-first (those whose callees are all matched already), and workers have
   `ghidra_info.py <Func>` for prototypes, caller constants and struct offsets. The
   database is `~/xw/ghidra-out`; regenerate it with `tools/ghidra/run.sh` from a tree
   in matching mode only after a new overlay is carved or function boundaries move.
   **A stub is fresh only if no percentage sits within ~60 lines above its `INCLUDE_ASM`
   AND the function's own name does not appear in those 60 lines.** A park note belongs to
   the stub *above* the parked body, so it sits as far above as the note is long plus the
   body: 25 lines caught only half the parks and round 45 briefed a dozen parked stubs
   (`battle/misc16`'s `func_8008F8F4` had its note 47 lines up and a `/* body */` in
   between). Measured with 60 lines + the name test: **400 fresh stubs in 108 units**
   (`tools/scripts/fresh_stubs.py`; it applies the atfree/jtbl filters below too, and
   since 2026-11-14 keeps jump-table stubs of units that own their tables and atfree
   stubs of cdk -G0 units: 455 fresh stubs in 111 units).
   Round 45 with this test: 14 workers, 3 committed matches before the kill.
   **A parked body, however, is written directly BELOW its stub, not above it**, so the
   above-window test alone missed every park whose note is the stub's own: on 2026-11-14
   the tool reported 433 stubs in 111 units and only 186 in 76 were really fresh (`sound`,
   `battle/misc4` and `battling/misc6` bottomed out entirely). `fresh_stubs.py` now skips
   the window between a stub and the next one too; run it before trusting any count above.
   Parks are written `// Best: 84.76%` far more often than as `/* ... */`, so `grep
   '^/\*'` over-reports badly: round 36 briefed 11 units picked that way and 9 of them
   were 100% parked (7 matches from 11 workers). Measured round 37 with that test: 1338
   stubs in the tree, **425 with no percentage within 25 lines**; after the reachability
   filter in item 2 the best 13 units held **77 reachable stubs and closed 4**. "No park
   note" does not mean "achievable" - a stub can be fresh and still a wall - so budget
   10-16 workers per round and expect single digits; the naming round is the reliable
   half (+252 names from 10 workers, same round).
2. **Give the worker the reachable subset, not the unit.** Leave out of `funcs`:
   - `python3 tools/scripts/atfree.py <bin>/<unit> asm/<bin>/nonmatchings/<unit>` hits in
     a unit that is NOT on a cdk -G0 preset. They are not a wall (see "Presets"): they
     are evidence the unit wants that preset, so screen the unit instead of briefing them;
   - `grep -l jtbl asm/<bin>/nonmatchings/<unit>/*.s` only in a unit that does not own
     its tables ("Jump tables");
   - `python3 tools/scripts/convertible.py <bin>/<unit>` BLOCKED entries — **run this
     on every unit before writing `funcs`.** `fresh_stubs.py` does not apply it, so a
     unit whose small stubs are all jump-table owners looks like 2-3 easy fresh stubs and
     is really a wall (`battle/misc30`, round 72: both stubs BLOCKED, 0 matches; its real
     pool was two other stubs, 47 and 78 insns). A unit's tables are often a prefix chain
     (`func_A` -> `B` -> `C`), so only a prefix of the jtbl stubs can ever be converted;
     `convertible.py`'s verdict already accounts for that;
   - **read the BLOCKED reason, they are not the same thing** (fixed 2026-11-16, when the
     script matched the first `.size` in the file instead of the function's own, which
     called every data-owning stub blocked). Measured across all 141 units: **869 OK, 113
     jump-table owners, 8 rodata owners, 52 with real post-`.size` padding.** `owns rodata
     D_x` **is convertible** — write the `const` definitions between the same two
     functions and un-park in the same commit (rounds 165-166: `field/main/misc2`
     `func_8007520C` and `func_800759E4` at 100.00%, no yaml edit). `owns jtbl_` needs
     `own_rodata.py` for the unit FIRST (see "Jump tables"), then the family converts in
     address order. Only `N insn after .size` is a wall;
   - stubs with `sw $sp`/`lw $sp` (stack switch; the alloca bodies it catches are
     `grep -l 'sw \$sp, 0x0' asm/*/nonmatchings/`, a dozen in `system/` and the battle tail).
     **This filter is not optional and neither tool applies it:** `fresh_stubs.py` and
     `convertible.py` both call these stubs fresh and linkable, because a stack switch has
     no post-`.size` instructions and no `dlabel` data. Round 148 spent two of six workers
     on them (`slus_006.64/system/rendering_pause_text` `func_8001FB30`,
     `battling/misc15` `func_8008BC04` is a coroutine register save), and the worker's own
     report found three more in `battle/main/misc27_b` (`func_800B626C/B61F8/B62C8`). Grep
     each assigned `.s` for `sw \$sp` before putting it in `funcs`;
   - stubs of <15 insns that start by loading a literal address: often a data word, not a
     function (`battling/misc12` `func_80088BFC` is a 4-byte pointer datum, code starts at +4);
   - parks, from a worker's hand-writing list. Every park has been swept under every
     preset except cdk -G0, and hand iteration on one is worth ~0. A park is permuter
     work ("The permuter"): the sweep and the apply round own it.
3. Put what the tracker says about the unit (one or two lines) in `extra`. **No raw
   backtick in `extra`** — `brief.js` is a template literal.
4. Finished, never brief: `slus_006.64/system/model`, `psyq/{libgte,libgte_b,libapi,
   libapi_2,libapi_3,libcard,libcard_b}`, and `slus_006.64/system/menu`
   `func_8001C76C`, which is not a function: 92 `.word` lines with no `jr $ra`, the last
   368 bytes of the unit's `c` range, unreferenced, and the linker puts that unit's
   `.text` immediately before the next unit's so it can never be C (round 176 proved it;
   `fresh_stubs.py` lists it because `convertible.py` counts words).
5. Two consecutive rounds of ~0 in a unit: stop briefing it and say why in the tracker.

## Presets (settled; do not re-screen)

`gears.toml` is the truth, keyed per file; a unit not listed there is Default (gcc
2.7.2, `-O2 -G8`). Read the unit's preset from it and pass it to the worker verbatim,
with `tools/scripts/preset_for.py <unit>` (it agrees with `build.ninja` on all 204 units).
Reading it by eye cost two rounds: `paths` arrays are multi-line, the last listing preset
wins, `{binary}` is a placeholder, and `battle/main/misc1.` is a prefix of misc13/misc19
unless the match stops at the dot - round 171 briefed `battle/misc19` as XenoBattle260
when it is Default, and round 172 nearly did the same to `battle/misc13`.
Workers never edit `gears.toml`; they report `PRESET_260` / `PRESET_G0` /
`PRESET_CDK` with both percentages and the scores of every already-matched function,
and you decide.

- **cdk with -G0 is the `battle` tail's compiler, and it was never swept.** gcc
  2.7.2-cdk's cc1 splits an absolute address itself under `-G0` (`lui $v0,%hi(sym)` /
  `sw $zero,%lo(sym)($v0)`, base register chosen by the allocator); under `-G8`, and
  in plain 2.7.2 and 2.6.0 at any `-G`, it emits the `sw $zero,sym` macro and the
  assembler goes through `$at`. That is the whole "atfree / assembler ceiling" class
  (verified on `battle/misc32`: four parks 45% -> 100.0, the four matched functions
  unchanged, `make check` green, unit now on `XenoBattleTailCdk0`). The queue:
  - **Switch, zero regressions measured** (every already-matched function stays 100.0
    under `--gcc gcc-2.7.2-cdk-psx --gp -G0`): `battle/main/misc26`, `misc31` (both on
    `XenoBattleTail`), `misc29`, `misc27`, `misc27_b` (on `XenoBattleTailCdk`). Run an
    `unpark-brief.js` round on these five (it now scores cdk -G0 and reports `CDK0` and
    `KEEP` lines), move each unit whose `KEEP` lines are all 100.00 into
    `XenoBattleTailCdk0`'s `paths` (and out of its old entry; `misc29` has its own
    entry near the end of `gears.toml`), then the usual two identical `make
    objdiff-config` runs, `make check`, and re-diff the unit's parks. Parks there that
    say ATFREE were written against the wrong compiler and several park blocks carry
    externs that conflict once un-parked (dedupe them).
  - **Already on cdk -G0, parks mis-diagnosed:** `misc28`, `misc30`, `misc32`, `misc33`,
    `misc34`. Their "ATFREE" stubs are ordinary stubs; `fresh_stubs.py` lists them.
  - **Mixed:** `misc35` (its one matched function regresses): a `split_unit.py` case.
  - **Not cdk -G0** (matched functions break): `battle/misc25`, `system/animation_scripts`,
    `controller_b`, `rendering`, `temp1`, `temp2`. Their few atfree hits need another
    explanation; leave them.
  - `try_flags.sh` runs share `build/.try_flags.*`: never two at once in one tree.
- All maspsx axes and the other compiler axes (2.6.0, cdk at `-G8`, `-G0` on plain
  2.7.2) have been swept at unit and park level. Do not run a repo-wide sweep again. After a round,
  `tools/rounds/park_sweep.py <unit>...` on the units that gained parks is enough.
- Every matched function stays 100.0 under the variant and a park reaches 100.0:
  switch the unit. **A screen at 99.5-99.9 on every function is NOT enough** - those
  percentages fold relocations, so a unit can look instruction-identical under the new
  compiler and still fail `make check` on the linked bytes (round 200: `battle/main/misc9`
  screened clean under 2.6.0 as an instruction-stream diff and broke `battle.bin`; the
  switch was reverted). Run `make check` as part of the switch, and if it fails, revert
  and treat the unit as a per-range split case instead. Some break: split it with `tools/scripts/split_unit.py` (AGENTS.md,
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
- a C `switch` in a unit that does not own its tables ("Jump tables");
- converting a callee, which let gcc shorten an already-matched **caller** by 8 bytes
  (keep the stub and the old `extern void f();`);
- park surgery that swallowed the next function or the next `INCLUDE_ASM` line (the
  symptom is `undefined reference` at link);
- a worker re-splitting an opaque field of a shared header.
- **three jtbl conversions in one unit whose OWN `check_bin` passed** (round 64,
  `battle/misc12`): the unit already had four converted switch tables from an earlier
  round and three more changed the alignment chain for every table after them. A worker
  cannot see this - only `make check` on the merged tree can - so any round that converts
  a switch in a unit with existing converted switches must expect a bisect (file at each
  of the worker's commits, then fully reverted) and the bodies stay in git for a future
  whole-family job.

One `.c` per round, even for two workers who would touch different functions: round 63
briefed `slus_006.64/system/sound` to a permuter port and a matching worker in the same
round and the merge conflicted. When the conflict is one function, the real match wins
over the BETTER park.

## Harness facts

- **A stuck build container eats a core.** `docker ps` showed a `make objdiff-config`
  for `~/xw/r-b17` still at 100% CPU two days after its round (2026-11-14). Workers are
  forbidden to touch docker; you are not. At session start, `docker ps --format
  '{{.RunningFor}}\t{{.Mounts}}'` and `docker kill` any container older than two hours
  whose clone is not in your current round or `r-permute`.
- **`maspsx` reads stdin to EOF.** Any script that runs it (`try_flags.sh`, `permute.sh`)
  hangs when stdin is an open pipe; they redirect `</dev/null` themselves now, and a new
  one must too.

- **Two orchestrator sessions can be live in this repo.** `tools/rounds/clone.sh new`
  starts with `rm -rf ~/xw/r-<tag>`, so a colliding tag silently destroys another
  session's live clone (round 45 lost a `temp2` worker that way). Before `new`: check
  `ls -d ~/xw/r-*` and `intercom list`, and prefer tags that carry a round number
  (`m45-temp2`). Two sessions merging, renaming or running `make check` in the same
  tree at the same time is the one thing `make check` cannot catch.
- **No untracked files in the main tree while clones are out.** A worker's
  `git commit -a` sweeps them onto its branch and the merge then dies with "untracked
  working tree files would be overwritten" (round 47: five of twelve merges, until
  `tools/pi/` was committed). Commit them or remove them before `new`.
- Workers hit the 30-minute kill constantly. Fine if they committed; `merge` refuses
  uncommitted work. A lost body is still in `worker_transcript.jsonl` under
  `subagent-artifacts/`.
- **Every child fails to start with `Background children require the host npm package`** and a
  list of `@earendil-works/*` specifiers pointing at `/opt/homebrew/lib/node_modules/...`: pi
  moved to a managed install (`~/.pi/agent/install/releases/<ver>`, what `~/.pi/agent/bin/pi`
  execs) and the extension still resolves the host package from the old npm global path. The
  symptom is 22 `Failed to start async run` children in one second, with no clone touched, so
  nothing is lost. Fix, no reinstall and nothing to undo in the repo:
  `ln -sfn ~/.pi/agent/install/releases/$(cat ~/.pi/agent/install/current-version)/node_modules/@earendil-works/pi-coding-agent /opt/homebrew/lib/node_modules/@earendil-works/pi-coding-agent`
  then confirm with one one-line child before relaunching the round. `@earendil-works/chord` in
  the error is a red herring: the extension only aliases it when the host manifest declares it,
  and pi 1.2.0+ does not ship it.
- **A pool branch is based on `main`, not `master`.** `git log master..HEAD` in a clone
  reports 0 commits for a worker that committed fine; use `main..HEAD` (round 48 read
  eleven merges as lost work that way).
- **Intercom entries outlive their orchestrator.** A dead session's worker clones still
  show in `intercom list` with no process behind them (`ps` has no worker, only this
  session); the truth is `git -C ~/xw/r-<tag>/xenogears-decomp log --oneline main..HEAD`.
  Adopting such a round is just: check that, then merge as usual.
- pi-subagents "needs attention / no activity for 300s" notices are noise. A real
  failure reads `error:Subagent timed out`.
- A `fg: 0` round never drains its `queue`: entries only start on a free FOREGROUND
  lane, so a queued worker in an all-background round does not run at all (round 176,
  the third field worker never started). Put those workers in `workers`, or leave `fg`
  at its default.
- `try_flags.sh` and `park_sweep.py` return `compile-failed` under load; re-run a
  surprising row before believing it.
- After editing a brief:
  `node -e "new Function('args','runs',require('fs').readFileSync('tools/rounds/brief.js','utf8'))"`.
- A worker in the MATCHING brief must carry `unit` (it becomes the run key). A field- or
  naming-round worker (`{tree, file, bins}`) sent to `brief.js` is not validated against
  the schema: the launch dies on the first entry with `TypeError: Cannot read properties
  of undefined (reading 'replace')` at `KEY`, before any clone is touched (round 176,
  nothing lost, one relaunch). One shape per launch - matching and field workers go out
  as two workflows, and the field one gets `fg: 0` so the matching round keeps both
  foreground lanes. **Check each script's own args header before filling workers:**
  `brief.js` (matching and rewrite) wants `unit`, `const-brief.js` wants `bin`,
  `field-brief.js` wants `file` + `bins`, `name-brief.js` wants `file` only. Mixing two
  of those in one launch has now cost three relaunches (rounds 183, 190). **The bad entry need not be the first one:** when it is a later
  entry, the earlier children start for a second and are then killed with the workflow
  (`Workflow stopped before async child completed`), and their clones come back
  untouched (`main..HEAD` empty) - so relaunch those workers too, not just the missing
  shape (round 183: a port worker mixed into a naming launch).
- The workflow resource reads `args.workers` at the TOP level, so
  `subagent({workflow: "tools/rounds/brief.js", args: {workers: [...]}})` - one `args` wrapper, not
  `{args: {args: {...}}}`. The extra wrapper still detaches a run, then dies instantly with
  `TypeError: Cannot read properties of undefined (reading 'map')`; the notice arrives minutes
  later and no clone is touched, so the recovery is a relaunch, not a rebuild.
- **A worker's `unit` becomes the run key, so it must be a bare path.** One worker whose
  `unit` carried a space and parentheses (`"slus_006.64/system/temp2 (plus ...)"`) killed the
  whole workflow in the first second with `runs.all item 0 has an invalid key` - keys allow
  only letters, digits, `.`, `_`, `-`. No clone was touched, so the recovery is to fix the
  string and relaunch. Put the extra files in `extra`, never in `unit`.

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
- Operator-only, so never done unattended: anything that deletes committed work,
  changes a preset in a way that takes an already-matched function below 100.0, or adds
  more than three files for one split. (A whole-unit switch under which every matched
  function stays 100.0 and `make check` is green is the "Presets" procedure, not this.) Log a `DECISION:` line in the tracker row and work on something else (see
  "The operator is mostly absent"). Reverting a function to its stub to get `make
  check` green is not deleting committed work.
- Report to the operator only: functions matched, functions named, globals and fields
  named, the raw-line count, the tracker row, and any `DECISION:` lines you logged. The report is a status
  note, not a stopping point.
