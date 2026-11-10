# Xenogears matching decompilation — long autonomous run

Read `reports/TRACKER.md` **from Session 6 onward** (the sessions before that are
historical and several of their conclusions have since been corrected — see "Do not
re-derive" below), then `AGENTS.md`, then `CLAUDE.md`, then
`tools/rounds/brief.js` (it is the worker playbook and it is more current than
`AGENTS.md` on presets and idioms). Then:

    colima status || colima start --vz-rosetta
    make check                      # all ELEVEN outputs must print OK before you touch anything

## The job

Raise the match count. Today: **3026 / 4568 functions (66.2%), 1542 `INCLUDE_ASM`
stubs**, eleven outputs byte-identical (`slus_006.64` plus `field`, `battling`,
`world_map`, `battle`, `menu`, `movie`, `member_change_menu`, `shop_menu`,
`gear_shop_menu`, `lib_gear`). Stubs by binary: `battle` 549, `slus_006.64` 264,
`world_map` 207, `battling` 190, `menu` 146, `field` 137, `lib_gear` 21, `movie` 20,
`shop_menu` 5, `gear_shop_menu` 2, `member_change_menu` 1.

**After every round, and only when `make check` prints OK for all eleven: commit,
push, and add a row to `reports/TRACKER.md`.** Run `make progress-publish` every
three or four rounds (the last session never did; the public page is stale). Never
commit `asm/`, `build/`, `expected/`, `linker/`, `objdiff.json`, `build.ninja`,
`disc/`, or notes.

## Read this before choosing targets: the preset landscape

Presets are keyed **per file** in `gears.toml` and several binaries mix compilers
*inside one overlay*, even between adjacent functions. This is the single most
important fact about the remaining work, and the source of most of the last
session's gains:

| unit | preset | evidence |
|---|---|---|
| `battle/main/{misc1,misc18,misc20,misc21}` | gcc **2.6.0** (`XenoBattle260`) | a parked body is 100.0 there and 78-95 under 2.7.2, and every matched function stays 100.0 |
| `menu/main/{misc5,misc6,misc9,misc10}` | gcc **2.6.0** (`XenoMenu260`) | same bar; `menu/misc9`'s 19/19 matched functions verified |
| `battling/main/misc12` | **`-G0`** | `func_80088940` 100.0 under `-G0`, 44.86 under Default |
| `battle/main/{misc21,23,24,25}`, `battling/misc15`, `movie/misc{1,2}`, `world_map/misc{2,5,6,7,8,10,13,14,16}`, `lib_gear`, `psyq`, `graphics/line_scroll`, `system/temp2` | **`--expand-div`** family | the retail object carries ASPSX's `break 6`/`break 7` div trap |
| `world_map/*`, `battle/main/misc26..35`, `system/rendering` | **`--dont-expand-li`** family | the retail object is `addiu $r,$zero,imm`-dominant, not `ori` |
| everything else | Default (gcc 2.7.2, `-O2 -G8`) | |

Screening is cheap and pays 1-4 functions per unit; **screened and negative, do not
re-run**: `battle/{misc2,misc3,misc7,misc9,misc14,misc15,misc16,misc17,misc19,misc23,
misc25}`, `battling/{misc1,misc2,misc4,misc6,misc8,misc10,misc11,misc17}`,
`menu/{misc1,misc2,misc3,misc4,misc7,misc8,misc11}`, `field/*` (four units, all
function-mixed under cdk), `world_map/misc6`.

**`gcc-2.7.2-cdk` is the answer to the "one instruction in the jal delay slot"
family** — 2.7.2 emits `la sym` as one macro that maspsx expands adjacently (reorg
can never split it), cdk emits `lui`/`addiu` as two RTL instructions and can. But
**every unit screened for cdk so far is function-mixed**, so no unit has been
switched: `battle/misc27` has five bodies that are 100.0 under cdk while three of its
matched functions drop to 93.71 under it, and the two groups interleave in address
order.

### The decision the last session could not take

Those mixed units are unreachable **only because presets are per file**. Splitting a
mixed unit into one TU per function — the `system/rendering_pause_text.c` precedent
(`XenoPauseText` preset) generalised: a 3-way `c` range split in `config/<bin>.yaml`,
a renamed tail file, one `gears.toml` path entry — would reach `battle/misc27`'s five
cdk parks, `battle/misc5`'s `func_8007ABD8` (proved 100.0 under cdk, unit mixed), and
the rest of that class. **Estimate 10-20 functions. It restructures the tree into many
small files, so ask the operator before doing it.** If they say yes, do one unit first
and prove `make check` before generalising.

## Do not re-derive (settled, with evidence in the tracker)

- **`gears` resolved presets in HashMap order** until round 8: a unit matching two path
  keys got a random preset per run, which silently broke an overlay by 36 bytes. Fixed
  by enumerating keys. **Never put a directory key and a per-file key for the same tree
  in `gears.toml`**, and after any preset edit confirm two consecutive
  `make objdiff-config` runs produce an identical per-unit flag map.
- **123 of the 1542 stubs cannot be matched by any preset** (assembler revision, not a
  compiler flag): retail's ASPSX picked a free register where our GAS always uses
  `$at` for a one-instruction load/store off an absolute symbol. `python3
  tools/scripts/atfree.py <binary>/<unit> asm/<binary>/nonmatchings/<unit>` lists them
  per stub; they are almost all of `battle/main/misc26..misc35`. Do not spend workers
  there — except `misc27` (7 of 22) and `misc28` (2 of 19), which are mostly reachable.
- **A `jtbl_`-referencing stub is unconvertible** unless the unit's whole rodata moves
  to C in address order: a C `switch` emits a private table into the unit's `.rodata`,
  which lands before the asm-owned tables and shifts the overlay while every function
  still reads objdiff 100.00%. `convertible.py` does **not** catch this. Check
  `grep -l jtbl asm/<bin>/nonmatchings/<unit>/*.s` first. Known blocked counts:
  `menu/misc10` 5 of 12, `menu/misc11` 1, `battle/misc11` 8 of 15, `battle/misc12` 11,
  `world_map/misc11` 7 of 13, `field/misc4` 4 of 15, `world_map/misc15` 2.
- `slus_006.64/system/model` (49 stubs) and the psyq `mtc2`-only bodies are hand-written
  assembly. Leave them. `psyq/libcard`, `libapi_2`, `libapi_3` are BIOS call gates.
- `-mcpu=r8000` and gcc 2.8.1 are **not** this project's compilers; both fail
  `make check`.
- **`objdiff: 100.00%` + `MATCH:` is not proof.** A body a few bytes longer or shorter
  than retail hides its tail, and function *order* is invisible to objdiff. The gate is
  `tools/scripts/check_bin.sh <overlay>` (or `make check` + `culprit.sh`).
- **A parked percentage is only valid under the preset that produced it.** After any
  preset change, re-diff every parked attempt in the unit before writing new C - this
  has produced free matches four separate times (`battle/misc1`, `misc18`, `misc21`,
  `menu/misc5`, `menu/misc6`).

## The round loop (use the checked-in harness, do not rebuild it)

One worker = one clone = **one `.c` file**. Two agents in one file will conflict; two
agents in different files will not.

    make objdiff-config                                  # clones need report mode
    tools/rounds/clone.sh new  a b c ...                 # cp -ac clones, ~220 MB each
    # launch: subagent({workflow: "tools/rounds/brief.js", async: true,
    #                   args: {workers: [{tree, file, unit, asm, binary, overlay,
    #                                    preset, symbols, funcs, extra}, ...]}})
    # wait for the round (~30 min), then:
    rm -f ~/xw/r-*/xenogears-decomp/.ninja_lock          # else merge sees UNCOMMITTED
    tools/rounds/clone.sh merge a b c ...
    python3 tools/scripts/lint_sources.py -q
    make check                                           # if it fails: culprit.sh <bin>
    rm -rf ~/xw/r-<tags>                                 # then `new` fresh clones next round

Harness facts learned the hard way:

- `clone.sh new` (not `reset`) after a preset change: `reset` keeps the clone's stale
  `build.ninja`/`expected/`, and a stale preset map is exactly the round-8 bug.
- A stray untracked `.ninja_lock` makes `merge` treat a clone as UNCOMMITTED and drop
  its commits. Remove them first.
- Workers hit the 30-minute kill constantly and that is fine — **if they committed**.
  Commit-then-gate is the rule in `brief.js`; `merge` refuses uncommitted work.
- `tools/rounds/brief.js` is a JS template literal. **A raw backtick in any hint breaks
  every workflow launch** ("Unexpected token"). Validate with
  `node -e "new Function(require('fs').readFileSync('tools/rounds/brief.js','utf8'))"`
  after editing it.
- The pi-subagents "needs attention / no activity for 300s" notices fire for every
  worker and are noise; a real problem looks like `error:Subagent timed out`.
- Tell every worker the exact preset and **do not let them edit `gears.toml`**; they
  report `PRESET_260` / `PRESET_G0` / `PRESET_CDK` with both percentages *and the
  scores of every already-matched function*, and the orchestrator decides.
- Prefer units and stubs with **no parked note** over 95-99% parks: the latter are
  documented allocator ties and are worth ~0 per worker.

## The tail: four named compiler behaviours with no source lever

When a report says one of these, believe it and move on (the per-unit instances are in
the tracker):

1. **`cse` replaces a single-use sum pseudo at the argument position** — statement
   order, locals and declaration order provably do not survive it.
2. **RTL block rotation** — retail puts a loop's increment at the *top* of the body
   block (in a `beq` delay slot, no latch copy); no `for`/`while`/`continue` shape
   reaches that layout.
3. **gcc will not keep a scalar global's *address* in a saved register** — retail's
   `lui sX,%hi; addiu sX,sX,%lo; … lw vY,0(sX)` is a materialised `la`; ours folds the
   address into each load, and pointer-local spellings get folded back. Only differs
   when the two uses have *different* indices (then CSE cannot credit the shared one).
4. **`combine_givs` / `global.c` tie-breaks** with an identical instruction multiset.

## Idioms that each closed several functions

- **Indexed loops, not incrementing pointers**: write `pSlot[i].field` with an `s32 i`
  instead of `p->field; p++`. With `p++`, `combine_givs` re-bases the induction register
  onto the body's last memory reference, shifting every displacement.
- **`addu` operand order is source- and compiler-version-dependent**: `(i + p->array)`
  emits index-first; `pDst = base; *(pDst + i + K)` emits base-first where `p[i + k]`
  never does (this is what unblocked flipping `battle/misc1` to 2.6.0). `((u8*)base +
  i)[K]` keeps the constant offset last.
- **Unused locals reproduce frames**: `RECT rect;` for an 8-byte frame, `s32 tmp[4];`
  for 16 unreferenced bytes.
- A constant the original loads once before a `switch` must be a source local; a
  `switch` compiles to a jump table and an `if` chain does not; three dense cases with an
  empty lowest case make gcc 2.6.0 emit the range check retail has.
- Two `0xFF` immediates are CSE'd unless they differ in **mode** (`s32 v = 0xFF;`).
- A signed product cast to `u32` before dividing gives `mult` + `multu`/`srl`; a `u32`
  product gives `multu`.
- `-fcross-jumping` merges identical call tails: duplicate the call in the source when
  retail has two.
- `char` is **unsigned** (`-funsigned-char`). `s16` divisor/argument where there is no
  `andi`/`sll`-`sra` pair. `lhu` vs `lw` tells you a field's width.

## Ground rules

- `make check` green is the only definition of done. Never edit
  `config/checksum.sha`, `expected/`, or the extracted binaries.
- No inline assembly in game code, no `#pragma`, no per-function compiler flags. A
  function either matches as C or stays `INCLUDE_ASM`. (Inline asm is authorised for
  the scratchpad `$sp` switch in `field/main/misc2.c` only, and it does not solve that
  function anyway - the blocker there is a register wall.)
- Ask the operator before: the per-function TU split (above), anything that deletes
  committed work, or changing a preset a unit already depends on.
- Two consecutive rounds of ~0 in a unit means stop spending workers there and write why
  in the tracker.
- Report to the operator only: functions landed, the tracker row, and any decision you
  need. Put the detail in `reports/TRACKER.md`.
