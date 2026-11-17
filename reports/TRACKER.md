# Decompilation tracker

Live log of autonomous 12-hour work sessions. One row per landed batch.
`make check` must pass for a row to land (all six outputs `OK`).

Baseline at session start (2026-11-08):

| Metric | Value |
|---|---|
| functions matched | 1632 / 2528 (64.56%) |
| code matched | 307928 / 670864 bytes (45.90%) |
| fuzzy match | 46.40% |
| `INCLUDE_ASM` stubs | 761 |

Stubs by area at session start: `slus_006.64` 419 (psyq 174, system 245),
`field` 301, `lib_gear` 29, `shop_menu` 9, `gear_shop_menu` 2,
`member_change_menu` 1.

## Log

| # | Batch | Functions | Total | Notes |
|---|---|---|---|---|
| — | baseline | — | 1632 | session start |
| 1 | `field/main/{misc,misc2,misc8}`, `system/sound`, `shop_menu/main/misc` | +14 | 1646 | 26 claimed; 12 rejected by `make check` (9 wrong size, 3 wrong registers) |
| 2 | `system/temp1`, `system/model` | +17 | 1663 | temp1 12 (gcc 2.7.2-cdk), model 5; 4 more units came back unverified from timed-out workers and were dropped |
| 3 | `system/{sound,rendering}`, `field/main/{misc2,misc11,misc8,misc}` | +21 | 1684 | sound 6, rendering 6 (needed a `XenoRendering` maspsx preset: no li expansion), misc2 6, misc11 2, misc8 1, misc 0; every claim survived `make check` |
| 4 | `field/main/misc9`, `system/temp3` | +9 | 1695 | most units came back as near misses (one register or one delay slot); misc9 5, temp3 4 |
| 5 | 10 units | +29 | 1724 | first round with the shared "known blockers" playbook in the brief; every claim survived `make check` |
| 6 | 10 units | +32 | 1756 | `field/main/misc` 9/9, `system/sound` 5/5; `text_box_render` handed back a file that did not compile (INCLUDE_ASM expanded inside a body) and was reverted |
| 7 | 10 units | +25 | 1780 | crossed 70% of functions; `shop_menu` revert for a 2-byte jump-table data difference |
| 8 | 10 units | +21 | 1801 | reverted a C-side `.rodata` constant in `misc2` (moves the unit's rodata) and a 99.31% `lib_gear` claim whose reloc named another symbol |
| 9 | 10 units | +9 | 1810 | crossed half the code; fixed an INCLUDE_ASM path missing `nonmatchings/` (silent maspsx failure, surfaced only at link) |
| 10 | `field/main/{misc2,misc9,misc12}`, `system/{temp3,archive,system,libarchive}`, `camera_movement`, `text_box{,_render}` | +7 | 1816 | the misc2 `.rodata` VECTOR came back a second time and was reverted again; `text_box.c` broke the compile again and was reverted |
| 11 | 10 units | +7 | 1823 | every worker reported "one register / one delay slot away"; yield has flattened to ~1 per worker |

## Queue

Units worked through one at a time (one agent per unit, no shared files):

1. `field/main/misc2`, `field/main/misc`, `field/main/misc8` — small field logic,
   plenty of named functions, Default preset.
2. `slus_006.64/system/sound` — 86 stubs, the largest single unit.
3. `slus_006.64/system/model`, `system/rendering`, `system/temp1`.
4. `shop_menu/main/misc`, `lib_gear/main/main`, `gear_shop_menu/main/render`.
5. `slus_006.64/psyq/*` — match by identifying the library version; low hit rate.

Known blocked / low-yield:

- `psyq/libgte` handwritten GTE functions (`mtc2`/`ctc2`) cannot be produced from C
  without inline asm, which is banned; they stay `INCLUDE_ASM`.
- `field/main/misc.c FieldScriptWriteActorFlags1` reaches 95.29% only because
  `g_FieldActors` is declared `volatile`; dropping the qualifier makes the
  function match but breaks the `field` binary elsewhere.

## What the first two rounds taught

- A worker's `MATCH` was not trustworthy until `diff_func.sh` normalised branch
  targets: `j 73a0 <Func+0x50>` and `beq v1,v0,7188 <Func+0x58>` are just
  relocated targets, but the old script printed them as differences, so a
  function could look matched while its C was a different size. Require
  `objdiff: 100.00%`, not the `MATCH` line alone.
- Worker turns are slow (5+ min of thinking per edit) and children time out at
  30 minutes, so a batch of 12 functions is roughly 3x more than one worker can
  finish. Keep batches at 6-8 and have them commit after every few matches.
- `ninja` permanently deletes `build/src/<unit>.c.{i,s,o}` when a build fails,
  and the maspsx edge sends stderr to `/dev/null`, so switching a function
  between C and `INCLUDE_ASM` without re-running gears (`make build`) silently
  yields "success" with no object. Always rebuild through `make`.
- `make check` is the only gate that matters; it caught every false match.

## Round 3 notes

- `system/rendering` does not use `ori $r,$zero,imm` at all (33 `addiu` instead),
  so maspsx must not expand `li` there; `gears.toml` now has a `XenoRendering`
  preset for it. `system/sound` is the opposite (64 `ori`), so this is per-unit.
- The four `FieldScriptWriteActorFlagsN` handlers in `field/main/misc.c` all miss
  by one instruction (`li a0,1` hoisted instead of sitting in the jal delay slot),
  and the cause is the `volatile` on `g_FieldActors` in `include/field/actor.h`
  (105 uses). Someone should test removing it with a full `make check`.
- Clone trees need `expected/` and `objdiff.json`: run `make objdiff-config` in
  the main tree *before* `cp -ac`, or the workers burn their budget rebuilding
  report mode by hand.

## Round 5-8 notes (the playbook that works)

A worker that only knows "write C and diff it" burns its 30 minutes rediscovering
the same six things. Putting these in every brief raised the yield from ~15% to
~70% of claims surviving `make check`:

- A hoisted `li a0,N` where the original keeps it in the `jal` delay slot: pass an
  `int` local, not a literal. (Still unsolved for the `FieldScriptWriteActorFlagsN`
  family, where the argument follows a computed `g_FieldActors[...]` address.)
- `%gp_rel(sym)($gp)` vs `%hi/%lo`: maspsx only emits gp-relative when the unit
  itself defines the symbol (tentative `.sbss` definition), not for `extern`.
- An unused stack slot in the original: declare an unused local (`RECT rect;`).
- A global reached as `offset(base)` where the base is held across a call: keep a
  pointer local, or write the store through a one-member struct so gcc cannot
  hoist it.
- `switch` compiles to a jump table, an `if` chain does not.
- Fields read with `lh`/`lhu` at the high half of a `long` (e.g. `lh 0x22`) are
  `CONV_TO_GTE(position.vx)`, not a struct layout bug.

## Still blocked, with evidence

- `slus_006.64/system/model`: every stub is hand-written assembly except four
  large functions. The `func_800315A0`-`func_80031870` family materialises
  constants through `li $at`, which gcc never does.
- `psyq/libgte`: handwritten `mtc2`/`ctc2` GTE wrappers, unreachable from C.
- `field/main/misc.c`: `FieldScriptWriteActorFlags1..4` (95.29%, one delay slot),
  `func_80087FA4` (79.83%, `&D_800B2348` in a register vs `lui $at`).
- `field/main/misc11.c`, `shop_menu/main/misc.c`: switch functions that compile
  to section-relative `.rodata` tables where the original names a `jtbl_` symbol.
  objdiff caps at ~99.7% and the linked bytes are the only proof.
- `system/rendering.c func_8001FB30`, `graphics.c GfxLoadImageAccelerated`: both
  move `$sp` into a heap block; only inline asm reproduces that shape.

## Method notes for whoever runs the next session

- Workers run in `cp -ac` clones of this repo under `~/xw/`, on a branch each, and
  commit as they go; the orchestrator fetches each clone as a git remote and
  merges. No shared `build/`, so rounds can overlap.
- Run `make objdiff-config` in the main tree *before* cloning: clones otherwise
  have no `expected/` and every worker spends budget rebuilding report mode.
- Require both `MATCH:` and `objdiff: 100.00%`. Require the worker to commit
  every 2-3 matches; children are killed at 30 minutes and lose nothing committed.
- `make check` is the gate and it catches roughly one bad claim per round. To find
  the culprit: `cmp build/out/<bin>.bin disc/<bin>.bin`, map the differing offsets
  to text symbols, then revert just those functions.

## Session result

Start: 1632/2528 functions (64.56%), 45.90% of code, 761 `INCLUDE_ASM` stubs.
End:   1824/2528 functions (72.15%), 50.66% of code, 569 stubs.

That is +191 functions and +4.4 points of code, in 11 rounds of ten parallel
workers (110 worker-runs), with every batch gated by `make check`.

## Where the next session should start

1. The single biggest blocker in `field/`: gcc 2.7.2 will not emit
   `addiu dreg, sreg, -K` from a register holding another symbol's address, and it
   hoists `li a0,N` out of a `jal` delay slot when the argument follows a computed
   `g_FieldActors[...]` address. One C idiom that fixes either unlocks dozens of
   parked attempts (many are 95-99% and identical apart from that instruction).
2. Jump-table functions cannot reach objdiff 100% by construction (section-relative
   `.rodata` vs a named `jtbl_`); several are parked at ~99.7% and need a byte-level
   check instead of objdiff.
3. `slus_006.64/system/model` is almost entirely hand-written assembly; do not
   spend worker time there. `psyq/libgte` likewise (`mtc2`/`ctc2`).
4. Untouched and tractable: `system/sound` 60 stubs, `field/main/misc` 37,
   `field/main/misc8` 27, `lib_gear/main/main` 27, `field/main/misc4` 16,
   `misc6` 16, `misc5` 14, `temp1` 13.


# Session 2 (2026-11-08)

Baseline at session start: 1824 / 2528 functions (72.15%), 50.66% of code,
569 `INCLUDE_ASM` stubs. This section logs the second 12-hour session.

Stubs by area at session start: `psyq` 150 (96 libgte, 12 libapi, 8 libapi_2,
8 libsn, 8 libc, 7 libgpu, 6 libcard, 4 libetc), `system` 245 (`sound` 59,
`model` 49, `temp2` 25, `rendering` 13, `temp1` 12, `archive` 9), `field` 301
(`main/misc` 34, `misc8` 27, `misc2` 19, `misc4` 16, `misc6` 15, `misc5` 14,
`main` 10, `scripts/virtual_machine` 8, ...), `lib_gear` 27, `shop_menu` 8.

## Log

| # | Batch | Functions | Total | Notes |
|---|---|---|---|---|
| — | baseline | — | 1824 | session start |
| 12 | 12 units (misc, sound, misc8, libgear, temp2, misc2/4/5/6, rendering, vm, misc11) | +13 | 1837 | `FieldScriptWriteActorFlags1..4` all matched; sound/temp2/misc6/vm timed out |
| 13 | psyq libgte/libc/libapi/libsn/libgpu, shop_menu, archive, temp3, tbr, misc12 | +12 | 1849 | psyq pool is live: 12 of the 14 landed matches are stock library routines |
| 14 | 13 units | +10 | 1859 | `sound`, `temp1`, `field/main`, `misc`, `misc4/5/6`, `misc7`, `misc8`, `gte`; the `g_FieldActors` volatile removal landed |
| — | `shop_menu ShopMenuLoadResources` (1088 insns) | +1 | 1860 | found and fixed the unit-rodata blocking bug: a missing `const` |
| 15 | 13 units | +9 | 1869 | sound 3, field/main/misc 3 (one via the jump-table byte check), temp2 2, misc6 1 |
| 16 | near-miss round: 13 units, 42 parked attempts at >=95% | +5 | 1874 | timeboxed on the last instruction/register of already-written C |
| 17 | 23 workers in two concurrent workflows | +15 | 1889 | 15 of 19 changed files survived the link check; 4 shifted an overlay and were reverted |
| 18 | 14 workers: GTE-family exploit, retries, one TU split | +23 | 1912 | first round where every worker gated its own overlay: integrated with zero reverts |
| 19 | 13 workers | +9 | 1921 | libgte colour family, sound, temp2, misc8 |
| 20 | 14 workers | +10 | 1931 | |
| 21 | 13 workers | +5 | 1936 | |
| 22 | 13 workers | +3 | 1939 | Psy-Q 3.x GTE macro revision found; six conversions held back for a 4-byte link delta |
| 23 | 13 workers | +13 | 1949 | the 4-byte delta was a stray `gte_nop()`; 11 libgte functions landed |
| 24 | 13 workers | +10 | 1959 | hard-coded-scratch GTE macros crack the return-value-coalescing wall |
| 25 | 13 workers | +1 | 1960 | flat: the tail is one-instruction register puzzles and blocked families |
| 26 | 13 workers | +4 | 1964 | SetTransMatrix, `func_8003864C`, `func_800809D0`, `func_80023A48` |
| 27 | 13 workers | +3 | 1967 | libgte `LoadAverage0`/`LoadAverage12`; the `sound` preset question answered |
| — | `GraphicsDrawPauseLetters` split into its own TU (`rendering_pause_text.c`) | — | — | the per-file compiler split works: that function only matches under gcc-2.7.2-cdk |

### Round 12-13 notes

- **`FieldScriptWriteActorFlags1..4` are matched.** The blocker really was the
  `volatile` on `g_FieldActors`. Dropping the qualifier globally matches all four
  immediately and changes exactly one other object: `func_80098A7C` in
  `field/main/misc7.c` stops matching. The worker used a file-local
  `#define FIELD_ACTORS (*((FieldActor**)(void*)&g_FieldActors))` in `misc.c`
  instead of touching the header; the clean fix is still to drop the qualifier in
  `include/field/actor.h` and re-solve `func_80098A7C`.
- **A psyq stub can be 100% in objdiff and still be wrong.** `func_8004BEC0`
  matches as an empty function, but its `.s` owns two trailing nops after `.size`
  that no C definition emits, so the unit's text comes up 8 bytes short. Always
  check a stub's `.s` for bytes past `.size` before trusting objdiff in `psyq/`.
- **The GTE macro headers in this repo are the wrong SDK version.** `gte_NormalColor`
  and friends emit two nops and a raw `.word`, where the original has one nop and
  a real COP2 encoding; `gte_mvmva(1,0,0,0,0)` emits 0x020013BF where the original
  has 0x4A481200. That blocks the whole `Normal*`/`Color*`/`LocalLight`/
  `Square`/`MVMVA` family (libgte and lib_gear) until a v2-style `inline_c.h`
  is added to `include/psyq/`.
- **Unit rodata is the single biggest integration hazard.** Converting the last
  function that owns a unit's `.rodata` deletes those bytes, because the data came
  from that function's own asm file. `misc8` and `virtual_machine` each lose 8
  bytes this way and shift the whole `field` overlay; `shop_menu`'s
  `ShopMenuLoadResources` and `misc2`'s `func_800759E4` lose a 16-byte blob and
  0x10 of a VECTOR. Note that cc1 emits switch jump tables *before* a named C
  array, so redefining the data in C in file order does not reproduce the layout.
  The fix is one job per unit: move the unit's whole rodata region to C in
  original order and drop the range from `config/<bin>.yaml`.

### Round 14 notes

- **The unit-rodata blocker has one root cause: a missing `const`.**
  `u8 D_x[0x10] = {...}` lands in `.data`; `const u8 D_x[0x10] = {...}` lands in
  `.rodata`, which cc1 emits *before* the jump tables it generates for switches.
  `shop_menu`'s `ShopMenuLoadResources` was stuck at "converting it shifts the
  whole overlay" for two sessions; declaring its `"BISLPS-00800"` blob `const`
  made `shop_menu.bin` byte-identical with the 1088-instruction function in C.
  See the new "Unit rodata" section in `AGENTS.md`.
- **`g_FieldActors` is no longer `volatile`.** Dropping the qualifier in
  `include/field/actor.h` (round 14, misc7) matches the four
  `FieldScriptWriteActorFlagsN` handlers *and* re-matches `func_80098A7C`, which
  was the only object in the build that depended on the volatile reload.
- **Parked attempt comments are a build hazard.** Two separate merges this session
  failed to compile because the text inside a `/* ... */` attempt contained `*/`
  (`Normal*/Color`, `Color*/LocalLight`) or because two attempt blocks were
  nested, leaving a stray `*/` inside a function body. `misc8` cost four rounds of
  integration. When parking an attempt, do not write `*/` or `/*` in its text.
- **A worker's "wip leftovers" commit can contain non-matches.** lib_gear's last
  round left three functions (74-87%) in C, which cost `lib_gear.bin` 229 bytes.
  The integration step is now: `make check`, then `culprit.sh <bin>` to name the
  offending functions, then revert just those.

### What the near-miss round learned (round 16)

- 42 of the 533 remaining stubs already have C that objdiff scores 95-100%. They
  are a *different* kind of work: the instruction stream is right and only the
  register allocation or one delay slot differs, so a worker who rewrites from the
  assembly throws away the answer. Round 16's brief pointed at the parked attempt
  and the record of what had already been tried; 5 of the 42 closed.
- `objdiff 100%` was not achievable for some: `func_8004BEC0` (psyq/libapi_2) is
  100% as an empty function but its stub `.s` owns two nops after `.size`, so the
  link is 8 bytes short. Same class as the jump table: the object can be perfect
  and the binary wrong.
- `tools/scripts/lint_sources.py` (new) caught three functions in `misc12` that
  were both `INCLUDE_ASM`'d and defined in C. That mistake is invisible otherwise
  (maspsx's stderr goes to /dev/null) and it happened three times in three rounds.
  Workers should run it before committing; it is now in `AGENTS.md`.

### The GTE header was the wrong DMPSX revision (session 2's biggest unlock)

`include/psyq/inline_c.h` said "DMPSX version 3" and emitted **two** `nop`s plus a
raw `.word` for every Type-2 GTE command. The retail libraries use the one-`nop`
spelling with the canonical COP2 encodings. A worker read the ground truth out of
our own target assembly (the instruction words are printed there), rewrote the
macro bodies, and `NormalColor`, `NormalLightCol`, `ColorCol` and `LocalLight`
went to 100% immediately; `libgte` went from 4 to 9 matched functions and
`slus_006.64` still links byte-identically. Everything that touches a GTE macro
was previously parked as "blocked": the `NormalColor3`/`ColorCol3`/`Dpq3` family,
`Square0`/`Square12`, `VectorNormal*`, `lib_gear`'s `gte_mvmva` case
(`0x020013BF` vs the retail `0x4A480012`) and `system/temp2`'s `GPF`/`mtc2`
functions. That is roughly 40-60 functions of new work that did not exist before.

### Process fix: workers now gate their own overlay

Four rounds in a row lost work at integration because a converted function changed
a unit's `.rodata` or its own size and shifted a whole overlay -- invisible to
`objdiff`, fatal to the link. The worker brief now ends with a mandatory
`tools/scripts/lint_sources.py -q` plus `tools/scripts/check_bin.sh <overlay>`,
and tells them to use `culprit.sh` on failure. That moves a 20-minute bisect from
the orchestrator into the worker's own 35-second check.

### The GTE header is Psy-Q 3.x, not 4.3 (round 22's correction)

After the first header fix (the retail COP2 words), a second class of GTE
functions stayed blocked: the ones that read a GTE register straight into a
general register (`mfc2 $2,$9`, `cfc2 $8,$24`). `include/psyq/inline_c.h` is the
4.3 revision, which only has `swc2`-to-memory stores; the retail library was
built with the 3.x header, where reads come back in GPRs. Adding
`gte_stotz_val` and rewriting `gte_SetRotMatrix`/`SetLightMatrix`/`SetColorMatrix`
to the 3.x shape (five `lw` into `$8`-`$12`, then five `ctc2`) matched
`SetRotMatrix`, `SetLightMatrix`, `SetColorMatrix`, `AverageSZ3`, `AverageZ3` and
`AverageZ4` -- but the six together make `slus_006.64` four bytes *longer* at link
time even though every function's instruction count matches the original, so they
are back to stubs pending that bisect. The header fix itself links.

### A stub can be perfectly matched and still unconvertible

`tools/scripts/convertible.py` exists because of this: a stub whose `.s` has
instructions after `.size` (assembler padding) or owns `dlabel` data cannot be
converted -- the C reproduces the body, not the padding, so the unit shrinks and
every later symbol moves. In `psyq/libgte` about 25 of 71 stubs are unlinkable for
that reason, and `objdiff` cannot see it. Two workers burned their whole budget
discovering this before the tool existed.

## Findings

- `tools/scripts/check_bin.sh <bin>` (new): matching-mode rebuild of one output
  and byte comparison against `config/checksum.sha`, ~35 s. The link is the real
  gate, so a function whose only objdiff difference is a relocation the linker
  resolves identically (section-relative `.rodata` jump table vs a named `jtbl_`)
  can now be *proved* matched instead of being parked at 99.7% forever.
- 150 psyq stubs, classified by their original assembly: 68 contain cop2
  (`mtc2`/`ctc2`) instructions and are handwritten GTE code, 17 are BIOS syscall
  trampolines (`addiu $t2; jr $t2`), and **65 are ordinary C** -- `RotMatrixX`,
  `ScaleMatrix`, `DrawOTagEnv`, `PutDispEnv`, `Sprintf`, `PCopen` and friends.
  That 65 is the largest untouched tractable pool in the repo and was written
  from known Psy-Q library sources.

### Compiler findings (session 2, before the first round landed)

- **`-mcpu=r8000` is wrong for this project.** Crash Bandicoot 2's decomp documents
  that at `-mcpu=r3000` gcc schedules argument setup into the save block where
  retail keeps it in a delay slot, and that `r8000` fixed their whole build. Tested
  here: switching the `Default` preset to `-mcpu=r8000` makes `slus_006.64` and
  `field.bin` fail `make check` while the four 2.6.0-built overlays still pass
  (gcc 2.6.0's `cc1` rejects `-mcpu=r8000` outright, so those units cannot even
  try). r3000 is right; the delay-slot differences are source shape, not `-mcpu`.
- **No SDK version banner in the game binary.** `strings disc/SLUS_006.64` has no
  `PSY-Q`/`SN32`/`cygnus` build string, so the toolchain cannot be read off the
  executable. The existing presets match the published mapping anyway: PSY-Q 4.0
  ships `GNU C 2.7.2.SN32.3.7.0002` + ASPSX 2.56 (= our `PSY-Q` preset) and
  PSY-Q 4.1 ships `cygnus-2.7.2-970404` + ASPSX 2.67 (= our `XenoGraphics`
  preset), with 2.6.0 for prebuilt library code -- the same 2.7.2/2.6.0/ASPSX-split
  structure Tomba's and Crash 2's decomps use.
- **65 of the 150 psyq stubs are ordinary C**, not handwritten asm: 68 contain
  `mtc2`/`ctc2` (GTE, unreachable from C) and 17 are BIOS syscall trampolines
  (`addiu $t2; jr $t2`). The plain ones are stock library routines --
  `RotMatrixX`, `ScaleMatrix`, `TransMatrix`, `memcpy`, `strcmp`, `Sprintf`,
  `PutDispEnv`, `DrawOTagEnv`, `ResetGraph`, `PCread` -- whose C is published in
  the Psy-Q SDK, so they are the highest-yield remaining pool.

- **gcc 2.8.1 is not this project's compiler either.** Swapping the `Default`
  preset to a freshly downloaded `gcc-2.8.1-psx` (Psy-Q 4.4 -- the compiler
  Vagrant Story, Chrono Cross and FF8's `snd_*` used) fails `make check` on
  `slus_006.64` and `field`, and `SoundReset` drops from 100% to 96.76% while
  2.6.0 and 2.7.2 both reproduce it. The existing presets are the right ones; a
  compiler switch is not the answer to the remaining tail.

New tools (all in `tools/scripts/`, all documented):

- `check_bin.sh <bin>` -- matching-mode rebuild + byte check of one output (~35 s).
- `try_flags.sh <unit> <Func> [--gcc g] [--opt -O2] [--gp -G8] [--cpu r3000|r8000]
  [--sweep]` -- score one function under alternative compiler/flag combinations in
  ~1 s per variant by parsing the flags out of `build.ninja` and running cc1 +
  maspsx directly, instead of ~35 s of gears per candidate.
- `culprit.sh <bin>` -- when `make check` fails, turn the differing byte offsets
  into the functions that own them, using the load address in `linker/<bin>.ld`
  and `config/symbol_addrs.<bin>.txt`. Turns "make check failed" into a list of
  suspects instead of a bisect.


# Session 2 result

Start: **1824 / 2528 functions (72.15%)**, 339888 / 670864 bytes (50.66%), 761 stubs.
End:   **1967 / 2528 functions (77.81%)**, 366756 / 670864 bytes (54.67%), 561 stubs.

That is **+143 functions and +4.01 points of code** in 16 rounds of 10-14 parallel
workers (about 200 worker-runs), every batch gated by `make check`.

Landed by area: `psyq/libgte` 4 -> 27 matched (the header revisions), `sound` +18,
`field/main/{misc,misc2,misc4,misc5,misc6,misc8,misc9,misc11,misc12,main}` +35,
`temp1`/`temp3` +14, `lib_gear` +6, `temp2` +7, `shop_menu` +3, plus
`member_change_menu`, `graphics`, `line_scroll` and `rendering`
(`GraphicsDrawPauseLetters` now lives in its own TU with the `XenoGraphics` preset).

## The five things that mattered

1. **The GTE header was the wrong SDK revision, twice.** `include/psyq/inline_c.h`
   shipped the 4.3 spelling: two `nop`s plus a raw `.word` per Type-2 command, and
   read-backs through memory only. Retail is the 3.x revision: one `nop`, canonical
   COP2 words, and reads straight into `$2`/`$8`. Fixing it matched
   `NormalColor`, `NormalLightCol`, `ColorCol`, `LocalLight`, `SetIR0`,
   `SetData32`, `SetBackColor`, `SetFarColor`, `Intpl`, `DpqColor`,
   `SetDQA/DQB/IR123/MAC123/Rii`, `SetRGBfifo`, `SetSZfifo3/4`, `SetSXSYfifo`,
   `SetRotMatrix`, `SetLightMatrix`, `SetColorMatrix`, `AverageSZ3/Z3/Z4`,
   `Square0/12/SS0/SS12/SL0/SL12`, `OuterProduct0/12`, `RotTransSV`,
   `ReadGeomOffset`, `ratan2` and `lib_gear`'s `func_801E6F64`.
2. **Hard-coded scratch registers in a macro's clobber list beat "return-value
   coalescing".** `"$2"` in the clobber list (not `"=r"` outputs) is what makes gcc
   copy `a1`/`a2` into `$v0` at the end instead of storing through `$v0`. That one
   trick closed six GTE functions and has more left (ApplyMatrix is 92% and only
   the `jr` delay slot differs).
3. **A stub can be unlinkable even at objdiff 100%.** `tools/scripts/convertible.py`
   encodes the rule: instructions after `.size` or `dlabel` data in the stub's `.s`
   mean the C cannot reproduce the unit's bytes. About 25 of libgte's 71 stubs are
   blocked this way, and psyq's syscall trampolines (`addiu $t2; jr $t2`) and the
   GTE read-back family are blocked for related reasons.
4. **`const` is load-bearing for unit rodata** (`shop_menu`'s 0x10 shift), and a
   unit's rodata family converts only as an **address-ordered prefix**, because
   cc1 emits C `.rodata` before the `.rodata` that `INCLUDE_ASM`d asm brings in.
   `field/main/misc2` is blocked because its first two rodata owners switch the
   stack into the 0x1F8003FC scratchpad, which no C construct can do.
5. **Toolchain questions are cheap now.** `try_flags.sh` settled that the project
   is neither `-mcpu=r8000` nor gcc 2.8.1, both of which are tempting from the
   Crash 2 and FF8/Vagrant Story write-ups. `culprit.sh` and `lint_sources.py`
   ended the "make check failed, now bisect six binaries" tax.

## Where the next session should start

1. **`psyq/libgte` still has ~45 stubs and is the single biggest tractable pool.**
   The remaining GTE-macro blockers are now narrow: (a) lazy loading of incoming
   stack arguments (`NormalColor3`, `NormalColorCol3`, `NormalColorDpq3`,
   `ColorDpq3`, `DpqColor3`, `LoadAverage0/12`) - gcc materialises them in the
   entry block, retail loads them after the GTE command; (b) the `jr` delay-slot
   fill for `ApplyMatrix`/`ApplyRotMatrix`; (c) `TransMatrix`/`ScaleMatrix`/
   `RotMatrix*` cannot be converted at all (padding after `.size`).
2. **`field/main/misc2`'s scratchpad stack switch** (`func_800739C0`,
   `func_8007554C`) gates six functions and the unit's rodata. It needs an
   `asm("sw $sp, 0($t0)")`-style statement, which the hard rules forbid; this is a
   policy decision, not a technical one.
3. **`system/sound`'s preset is CONFIRMED correct (2.6.0); do not split it.** A
   round-27 worker measured all 235 C-defined functions in the unit: 188 are
   byte-identical under gcc 2.6.0 and 2.7.2, **45 are 100% under 2.6.0 only and
   zero are 2.7.2-only** (the witnesses are spread across the whole unit, not
   clustered). `func_8003D53C` is a lone accident that happens to compile
   identically under 2.7.2; it cannot be carved out without three TUs for one
   24-byte function. (Earlier note retained for the reasoning:) Round 26
   found `func_8003D53C` scores 95.83% under the unit's `XenoLibrary` (gcc 2.6.0)
   flags but **100%** under `Default` (gcc 2.7.2, -O2 -G8); six sampled matched
   functions in the unit are 100% under both. Switching the whole unit to 2.7.2
   fails `make check` — the first differing symbol is `SoundSpuMemoryFindBlock`,
   so *some* functions need 2.6.0 and the rest may need 2.7.2. Splitting
   `system/sound.c` into two source ranges with different presets (the
   `rendering_pause_text.c` pattern) is probably worth a dozen functions.
   **`system/sound` (~45 stubs) and `lib_gear` (~24) are also dominated by
   register-allocation puzzles** documented in their parked comments. A worker
   that only reads the parked note and tries one genuinely new lever per function
   is worth more there than a fresh attempt.
4. **`slus_006.64/system/menu.c` is finished for C purposes**: its last "stub"
   `func_8001C76C` is 94 lines of raw `.word` (a menu/resource descriptor table),
   not code. Do not put it on a work list.
5. **A tools bug to remember**: `try_flags.sh` scored any still-`INCLUDE_ASM`
   function as a bogus 100.00% until round 27, because it preprocessed without
   `-DSKIP_ASM` and so compared the original assembly with itself. Fixed; if a
   preset experiment ever reports a suspiciously perfect score for a stub, that is
   what it was.
6. **`system/model` (49 stubs) remains hopeless** (hand-written asm), and
   `psyq/libgte`'s `mtc2`-only bodies likewise.
7. Overlays still unsplit: `battling`, `world_map`, `battle`, `menu`, `movie`
   (AGENTS.md section 9). That is the largest single source of new work left and
   it is mostly mechanical.


# Session 3 (2026-11-08)

Baseline at session start: 1967 / 2528 functions (77.81%), 366756 / 670864 bytes,
425 `INCLUDE_ASM` stubs at this session's counting (the "561" figure counted
multi-line stub blocks; `grep -c '^\s*INCLUDE_ASM('` is the reproducible one).

## Log

| # | Batch | Functions | Total | Notes |
|---|---|---|---|---|
| 28 | 15 workers: libgte, sound, lib_gear, misc, misc8, temp2, misc4, misc6, rendering, archive, virtual_machine, misc12+misc5, temp1+animation_scripts, psyq libapi+libsn, misc2 (scratchpad job) | +8 | 1975 | libgte 2, animation_scripts 2, misc12 2, misc6 1, libsn 1. Zero from the six saturated `field/main/*` units |

### Decision taken this session: inline asm for misc2's scratchpad stack switch

The operator authorised inline `asm` for the `$sp` scratchpad switch in
`field/main/misc2.c` only, judged per function (this overrides the general "no
inline asm" rule for these two functions and nothing else).

`func_800739C0` was attempted that way: the three `lui/ori $t1,0x1F8003FC;
sw $sp,0($t0); addiu $t0,-4; addu $sp,$t0` switches do assemble from `asm
volatile` statements and the function decodes completely, but it is parked at
**72.03%** because of a register-allocation wall that has nothing to do with the
stack trick: the original reaches `g_CameraAt+8`, `g_CameraEye` and `g_Scene` off
one base register (`$s0`), and naming them as separate symbols makes gcc build
two bases plus an extra callee-saved register. `func_8007554C` (the other
scratchpad function) is untouched.

Two facts for the next attempt:
- maspsx rejects **hex** offsets inside inline asm (`invalid literal for int():
  0x0`), so write `0($t0)`, never `0x0($t0)`. `ninja` hides the message.
- the whole C for `func_800739C0` is parked under the stub, including the three
  `asm volatile` blocks, so the next agent starts from 72% and only has to beat
  the base-register wall.

### What this round taught

- The `field/main/misc*` units are **saturated**: every stub in `misc`, `misc4`,
  `misc8`, `misc2` already had a parked attempt between 72% and 99.6%, and every
  one of them is a gcc scheduling/register-allocation wall, not a source-shape
  gap. Three separate workers re-derived them from the assembly and confirmed it.
  Two consecutive rounds of ~0 in those units: stop spending workers there.
- `select_target.py`'s `C?` column under-reports parked attempts (it found 4 of
  the 60+ that exist, because a parked block that starts with `//` followed by
  `/*` is not recognised). Read the file, do not trust the column.
- `field/main/misc6`'s `func_800A06E8` matched from two idioms worth keeping:
  the explicit `nOffset = idx * sizeof(FieldActor); pElem = (FieldActor*)(nOffset
  + (s32)g_FieldActors);` form (multiply chain before the base load) and
  `((u_short*)&status)[0] &= 0xFFDF` to stop gcc hoisting a signed `-33` mask
  into a callee-saved register across a call.
- untried-but-live: `psyq/libc.c` (strcmp/strcat/Sprintf), `libcard`, `libgpu`,
  `libetc`, `system/system.c`, `work_list`, `temp3`, `camera_movement`,
  `field/main/main.c`, `text_box*`, `shop_menu`, `member_change_menu`,
  `gear_shop_menu` - round 29 targets these.
| 29 | 10 workers: psyq libgpu+libc, psyq libcard+libapi_2+libapi_3, system/system, work_list+temp3, libarchive+graphics, misc3+misc7+misc10, misc9+misc11, text_box+text_box_render, camera_movement+main, member_change_menu+gear_shop_menu+shop_menu | +1 | 1976 | `func_8009BC98` (text_box). Two workers timed out with uncommitted C (one at 95.95%) which I reverted before merging |

### Round 29 notes: what "untouched" really meant

Most of the untouched pools are not tractable C after all, and the evidence is now recorded:

- **`psyq/libcard`, `libapi_2`, `libapi_3` are BIOS call gates**, not wrappers:
  `addiu $t2,$zero,0xA0 / jr $t2 / addiu $t1,$zero,0xAB`, every one with a pad
  `nop` after `.size`. gcc only ever jumps indirect through `$t9`/`jalr`, so no C
  emits them, and `convertible.py` flags them BLOCKED. Do not list them again.
  (`setjmp`/`longjmp` likewise: hand-written register saves to fixed `$a0` offsets.)
- `system/work_list`'s `WorkListAddTask` really did convert (matched in the worker's
  tree) but the worker timed out before committing and its C was reverted; the
  same for `func_80095CC4`. Both are still open and cheap.
- `psyq/libgpu`/`libc`: `strcat` is 85.90% with only a callee-saved pair swapped,
  `SetGraphReverse` 90.84% likewise, `strcmp` 56% (the retail shape keeps the loaded
  byte live across the loop), `MoveImage` 82.73% (aspsx load order).
- `system/system`: `func_80033CF0` 87.46% and `SystemRenderStringEntry` 28.31%.
  The second one's C content is right and only the store scheduling differs, so
  statement order is not the lever.
- `system/libarchive`: `func_8002945C` 90.68% after finding the real shape (an
  `nState = -1` accumulator with a single return, not early returns); the rest is a
  3-register rotation. `GfxLoadClutsAccelerated` 99.11%, only `&rect`/`pData` swap
  `s3`/`s4` - but the two `.sbss` pointer definitions it needs are risky, so it stays
  parked.

### psyq/libgte is now two TUs

`libgte.c` (25 stubs, the GTE/matrix family) and `libgte_b.c` (25 stubs, from
`ReadGeomScreen` at 0x3A8DC to `RotMatrixZ`) so two workers can work the 24
convertible stubs at once. `config/slus_006.64.yaml` has the extra `c` segment and
`make check` is byte-identical. Splitting an all-functions unit is a one-line yaml
edit; the only care needed is that the new file's `INCLUDE_ASM` path must name the
new nonmatchings directory (`nonmatchings/psyq/libgte_b`), because splat generates
the `.s` files per yaml segment.
| 30 | 12 workers: libgte (A), libgte_b, sound, lib_gear, rendering, temp1+animation_scripts, archive, virtual_machine, misc12, temp2, misc6 | +11 | 1987 | **libgte_b 7 (the GTE wall broke)**, libgte 3, animation_scripts 1; sound timed out with a live 59% body which I reverted |

### The two tricks that broke the GTE wall (round 30)

`libgte_b.c` had nine stubs parked at 88-92% with the note "maspsx inserts an extra
nop". Both causes are now known and general:

1. **maspsx's load-reuse heuristic, not the macro revision.** In
   `lwc2 $6,0($a1)` followed by `mtc2 $6,$8`, maspsx decides the `mtc2` reads the
   COP2 register the `lwc2` just loaded and inserts a real `nop` between them.
   Spelling the GPR as `$a2` (`gte_lddp_a2`, identical encoding) hides the collision.
   That alone matched `NormalColorDpq`, `ColorDpq`, `DpqColorLight`.
2. **Stack-passed output pointers read at the point of use** through an `"m"`
   operand on the incoming stack slot (`gte_strgb3_stack`, `gte_strgb3_gpr_stack`,
   `gte_lddp_stack`) bypass cc1's mandatory entry-block copy of a stack argument.
   That matched the `*3` family: `NormalColor3`, `NormalColorCol3`,
   `NormalColorDpq3`, `DpqColor3`.
3. A third, from `libgte.c`'s worker: when retail ends a function with `move v0,a2;
   jr ra; nop` and gcc's reorg steals the copy into the delay slot, **fuse the
   return-value copy into a final `asm volatile` block together with the `swc2`
   stores** - volatile asm is not delay-slot eligible. That matched `ApplyMatrix`
   and `ApplyRotMatrix`. `ApplyMatrixLV` is the opposite: retail has the copy *in*
   the delay slot there, so it wants the plain `return` form.

`LoadAverageByte` matched with three new file-local macros (`gte_ldbv_gpr_lo/hi`,
`gte_stbv_gpr_stack`). Still open in libgte: `CompMatrix`, `ApplyMatrixLV`,
`SetMulMatrix` (the 5x `lw`-into-`$8`-`$12` ctc2 chain), `LoadAverageCol`,
`MulMatrix*`, `RotAverageNclip4`, `TransposeMatrix`, and `InitGeom`/`SquareRoot0`
(need `mfc0/mtc0` and a table respectively).

### Process: two 30-minute timeouts cost 4 functions

Four workers this session reached ~100% or converted a function and then died
without committing: `WorkListAddTask`, `func_80095CC4` (95.95%), a 59% live body
left in `sound.c`, and `SystemRenderStringEntry`. `clone.sh merge` would happily
commit a partial body, so the integration step now is: check `git status` in each
clone, `git checkout -- .` any clone with uncommitted changes, then merge. Workers
are told to commit after every single match and to write by minute 20.
| 31 | 11 workers: libgte, libgte_b (2nd pass), work_list+temp3, misc11+misc9, text_box*, misc3 (FieldFree), camera+main, member_change/gear_shop/shop_menu, libgpu+libc, system/system, libarchive+graphics | +2 | 1989 | strcat, `func_8007F5AC`; four children hit the 30-minute limit with matches uncommitted - see the resume note below |
| 32 | 9 workers: misc6 (flag forensics), sound, lib_gear, rendering, archive, virtual_machine, temp1, misc12, temp2 | +7 | 1996 | sound 2, virtual_machine `func_800A31E8`, archive `func_8002B084`, libarchive `func_8002945C`, temp2 `func_800257F0`, plus `func_8002945C`; restored 6 timed-out children by `subagent resume` instead of re-running them |

## Session 3 result

Start: **1967 / 2528 (77.81%)**, 366756 / 670864 bytes (54.67%), 425 countable
`INCLUDE_ASM` stubs.
End:   **1996 / 2528 (78.96%)**, 372996 / 670864 bytes (55.60%), 396 stubs.

**+29 functions, +0.93 points of code, +1.15 points of functions**, in 8 rounds of
9-15 parallel workers (~95 worker-runs), every batch gated by `make check`.

### The decisive negative: it is not a compiler flag

One worker built a harness that compiles one unit exactly as `build.ninja` does and
scores a batch of functions against a 10-function control group of already-matched
functions, then ran the near-miss corpus (95-99.6%) under every plausible flag:
`-fno-function-cse` and `-fno-peephole` have **no effect at all** on gcc 2.7.2's
output; `-fno-defer-pop`, `-fomit-frame-pointer`, `-fno-inline`,
`-fno-caller-saves` likewise; `-fno-strength-reduce`, `-fno-force-mem` and
`-fno-expensive-optimizations` each **break already-matched functions** and flip
nothing; `-O1` collapses every function in the corpus; `-mno-split-addresses`,
`-foptimize-register-move` and `-Os` are rejected by cc1. Combined with the
earlier `-mcpu=r8000`, gcc 2.8.1, gcc 2.6.0-per-unit and full `-O`/`-G` sweeps,
**the toolchain is settled and the residual is source shape or a maspsx/assembler
detail - not a flag.** Stop looking for a compiler switch.

### What did work: five mechanical root causes

1. **maspsx load-reuse**: `lwc2 $6,0(a1)` + `mtc2 $6,$8` makes maspsx insert a
   `nop`; spelling the GPR `$a2` hides it (7 GTE functions).
2. **`"m"`-slot stack arguments**: reading an incoming stack pointer at its point
   of use through an `"m"` operand bypasses cc1's entry-block copy (the `*3` family).
3. **Second base local beats strength reduction**: `pVTable = pUVTable + 2;` as a
   second local stops gcc folding `+2` into the scaled index - order *and*
   registers both came out right (`func_8007F5AC`, after two sessions parked).
4. **Hoist an assignment to the block that shares it**: `dir = entryFlags & 7` must
   sit *above* the `if`, not inside it, when the original's `andi` is in the shared
   block's delay slot (`func_8001F8E8`, 88.26 -> 91.88%).
5. **Split the expression into three statements** when one C expression cannot
   produce the original's register triple: `nWords = (f+3)/4; nHalf = SIZE/4;
   nHalf = nHalf - nWords;` (not `-=`) is what makes reorg steal the right `li`
   (`func_8002B084` 99.74 -> 100.00%).

### Process: `subagent resume` recovers a timed-out worker

Six workers hit the 30-minute limit with a **100% function in an uncommitted
working tree** this session. Re-running them costs a full 30 minutes and usually
re-derives the same code; `subagent({action:"resume", id})` with the message
"commit anything at 100.00%, park everything else, run the two gates, report"
recovered four of them in about two minutes each. Do that before merging:
`git checkout -- .` in any clone that is still dirty, then merge.

### psyq/libgte is worth its two TUs; the rest of psyq is not

The split paid (10 functions across two workers in two rounds). But the other psyq
pools are dead ends and should not be listed again: `libcard`, `libapi_2` and
`libapi_3` are BIOS call gates (`addiu $t2,0xA0; jr $t2; addiu $t1,N`) with pad
nops after `.size`; `libc`/`libgpu` are down to allocation-only deltas
(`strcat` matched; `strcmp` 56%, `SetGraphReverse` 90.84%, `MoveImage` 82.73%);
6 of libgte's remaining 9 stubs have instructions after `.size`.

### The closest open items, for whoever continues

`func_8002B084` is matched; `func_8002B2F0` (identical body to `func_8002B5D0`, so
one match pays twice) is 96.40% with the residual being two `lhu`s that gcc CSEs.
`func_800A0C94` 99.64% (one `addu` destination). `func_80023804` 99.10% (symmetric
`v0`/`v1` mask swap). `GfxLoadClutsAccelerated` 99.11% (`s3`/`s4` swap - and the
old note's advice to define `D_800592F0/F4` in the unit is *wrong*: they are
already in `asm/slus_006.64/data/49AC0.sbss.s` and maspsx only emits `%gp_rel` for
symbols the unit itself defines, so this one needs a data-ownership change).
`func_801E6578` 98.63% (one callee-saved pair, allocnos identical, so global.c's
tie order). `FieldFree` 97.34% (652 vs 656 bytes, two combine decisions).
| 33 | 11 workers on the closest open items (99.6% down to 90%): archive, misc6, temp1, lib_gear, rendering, system, libarchive, libgpu+libc, misc12, sound, libgte_b | +1 | 1997 | `SetGraphReverse` (a two-session 90.84% flip); everything else moved up but not to 100% |

## Round 33: the tail is allocator tie-breaks, and it says so

Nine workers spent a full 30 minutes each on the *closest* remaining stubs and
moved most of them up (96.40 -> 96.81, 91.88 -> 95.23, 93.87 -> 97.87, 92.98 ->
96.49) without closing them. Every one of them reported the same shape: the
instruction multiset is identical to retail and one register or one delay slot
differs. That is the residual, and it is now measured, not assumed:

- `func_801E6578` (98.63%): the two allocnos have identical def/use counts and
  live ranges; 20+ source shapes leave global.c's tie untouched. Not reachable
  from C.
- `func_800A0C94` (99.64%): `addu` destination only; 24 declaration orders are
  byte-identical, and any shape that stops gcc CSE-ing the `*0x5C` rematerialises
  the multiply per block (82%).
- `func_80023804` (99.10%): mask/value register pair; a sibling function
  (`func_80021BCC`) compiles the identical expression the other way round, so it is
  context-driven allocation.
- `GfxLoadClutsAccelerated` (99.11%) and `func_8003D53C` (95.62%): one swapped
  pair / one extra `move` respectively.

**Recommendation for the next session:** stop spending workers on the 95-99.6%
class. The productive work left is *new* function pools, not the last instruction:
(a) split the remaining `asm` yaml segments (`[0x33F6C, asm]`, `[0x35B7C, asm]`,
`[0x3BCAC, asm]` in `slus_006.64`) into `c` units - the libgte split shows this is a
one-line change plus a directory rename, and it is the largest untouched pool;
(b) the unsplit overlays (`battling`, `world_map`, `battle`, `menu`); (c) symbol
and struct recovery in the units that are already complete.

Useful facts banked this round, all written into the parked comments:
- `D_8004FE26 = *(volatile u16*)&D_8004FE26 + 1;` makes gcc keep retail's two
  `lhu` of an id counter when an intervening store through a pointer does not
  otherwise alias it (`func_8002B2F0`, 96.40 -> 96.81).
- Making the first parameter a *statement copy* (`s32 v; v = value;`) moves gcc's
  parameter copy after the entry-block constant load (`func_80033CF0`, 92.98 ->
  96.49).
- Referencing a global **twice** (`if (g == NULL) return NULL; p = g;`) produces
  the load-before-prologue shape; `p = g; if (!p)` does not.
- `XenoGraphics`/gcc-2.7.2-cdk rejects mixed declarations outright: every local
  must be at the top of its block or cc1 exits 33, and `diff_func.sh` then prints
  a **stale** objdiff score with no MATCH/MISMATCH line. Check for the line, never
  the percentage alone.

## Session 3 final numbers

**1997 / 2528 functions (79.00%)**, 373272 / 670864 bytes (55.64%), fuzzy 56.15%,
396 countable `INCLUDE_ASM` stubs. Session start was 1967 (77.81%), 54.67% of code.
That is **+30 functions and +0.97 points of code** in 9 rounds (~100 worker-runs),
six of which closed real functions and three of which produced only evidence that
the residual is not reachable from C. `make check` was green for every batch and
the public page is published at 55.6%.


# Session 4 (2026-11-09)

Baseline at session start: 1997 / 2528 functions (79.00%), 373272 / 670864 bytes,
396 countable `INCLUDE_ASM` stubs.

The session followed round 33's recommendation: **stop working the 95-99.6% class
(proven allocator tie-breaks) and create a new pool instead.**

## The split: every remaining `asm` segment is now a C unit

`config/slus_006.64.yaml` still listed eleven raw `asm` subsegments -- code with no
`.c` file, so no worker could ever touch it and it did not even appear in
`select_target.py`. Seven of them are now `c` units (89 new functions, +3% of the
whole binary's function count):

| unit | functions | what it is |
|---|---|---|
| `psyq/libgpu_b` | 25 | 0x35B7C first half |
| `psyq/libgpu_c` | 18 | 0x35B7C second half (incl. a 4832-byte `GsSortObject4`-shaped monster) |
| `system/controller_b` | 15 | 0x26644 first half (pad/controller state machine) |
| `system/controller_c` | 6 | 0x26644 second half |
| `psyq/libapi_b` | 8 | 0x3137C, interrupt-hook installers |
| `psyq/libcard_b` | 6 | 0x3F080, libcard hook layer |
| `system/archive_b` | 5 | 0x28078, archive+heap loader |
| `psyq/libgpu_tex` | 3 | 0x33F6C, LoadImage/GetTPage/GetClut helpers |
| `psyq/libgte_c` | 3 | 0x2FF38, `RotMatrix`/`rcos`/`rsin` |

The recipe is a two-step, and it is byte-identical under `make check` (the `.s`
files are produced by the same maspsx path either way): append the function
symbols that spimdisasm already found in the `asm` segment's `.s` to
`config/symbol_addrs.slus_006.64.txt`, write the `.c` with one `INCLUDE_ASM` per
symbol in address order, and change `[0xOFF, asm]` to `[0xOFF, c, unit]`. Split a
big segment with `[0xMID, asm]` in between and repeat; the tail's `.s` is still
the original segment file, so derive its function list from that file filtered by
address.

The five segments left as `asm` (`9D24`, `9D60`, `A164`, `30CF4`, `3BCAC`,
`3CD30`) are single handwritten functions -- `mfc0`/`mtc0`, BIOS gates, the stack
setup in `start` -- and stay asm.

**Naming lead for a future session (unfinished).** The 0x35B7C range is the Gs
library: `func_800477D0` is `GsSortObject4` (it `memset`s a 0x78-byte GsDOBJ2-ish
header and dispatches on GPU packet words `0x2D030709`, `0x25030607`, ...), so
`psyq/libgpu_{b,c}` should be renamed `psyq/libgs*` once they have matches. The
Psy-Q 4.0 SDK archives with full symbol tables are public
(<https://github.com/sozud/psy-q>, `4.0/COFF/LIB/*.A`, R3000 COFF big-endian,
magic 0x0162, 8-byte relocations with the type in the top byte of the second
word); parsing them would name every remaining `psyq/` stub by exact code match
against the game's linked library. The member `.o` files already yield their
symbol names in order via `strings` (`prim.o` gives GetTPage, GetClut, AddPrim,
SetPolyF3, ... SetLineG4); what is missing is the symbol *offsets*, because the
`nlist` entry stride does not come out at 18 bytes with the naive header parse --
someone should finish that parse, it is the largest naming unlock left.

## Log (session 4)

| # | Batch | Functions | Total | Notes |
|---|---|---|---|---|
| 34 | 14 workers: the 9 new split units + libgte/libgte_b/lib_gear/temp2/virtual_machine | +41 | 2038 | `libgpu_b` 11, `system/controller_b` 11 (2 of them via `check_bin`), `libgpu_c` 5, `controller_c` 5, `libapi_b` 4, `libgpu_tex` 3 (all three), `archive_b` 1, `lib_gear` 1. `libcard_b` 0 (all six are hand-written card-BIOS patch asm); temp2/rendering/gte/gteb/vm only parked attempts |
| — | name recovery from the Psy-Q 4.0 library objects | — | — | 16 renames, and the method below |

**Round 34 is the first round in three sessions that paid more than +10, and the
reason is the split, not the workers.** 34 of the 41 matches are in units that had
never been attempted before (they did not exist). The two "saturated" pools
(`libgte`, `libgte_b`) and the synced `temp2`/`rendering` units produced nothing
but parked notes, exactly as round 33 predicted.

### Naming: the Psy-Q 4.0 SDK libraries are public and parseable

The single biggest piece of durable knowledge from this session. `github.com/sozud/psy-q`
carries the real Sony Psy-Q SDK trees (3.3, 3.5, 3.6, 4.0) including
`<ver>/PSX/LIB/*.LIB`, whose objects keep their **symbol tables**. `psy-k`
(<https://github.com/ttkb-oss/psy-k>, `cargo build --release -p psy-k-bin`) reads
them: `psyk extract LIBGPU.LIB` writes the `.OBJ` modules out, and a ~40-line Rust
bin over the `psy-k` API dumps every XDEF as (section, offset, name) plus the
module's code with the relocation-patched words marked wildcard.

Comparing that against every `asm/slus_006.64/*/psyq/**/*.s` here:

- **224 of the 247 functions that match at all match the SAME name in all four
  library versions** -- which is what makes a rename trustworthy. The 8 that
  disagree are all the `*2` family (`InitPAD2`, `StartPAD2`, `InitCARD2`, ...),
  where 3.3/3.5 say `InitPAD` and 3.6/4.0 say `InitPAD2`; the game has both.
- 16 renames landed: `VSyncCallback`, `SetDrawLoad`, `LoadAverageShort0/12`,
  `_96_remove`, `_SendPAD`, `_remove_ChgclrPAD`, `SetPriority`, `siocons`,
  `GPU_cw`, `SetFogNearFar`, `LoadTPage`, `LoadClut`, `LoadClut2`, `MatrixNormal`,
  `_SN_read`. Four of those were already matched C under their `func_` name.
- **Rejected, and the reason is the method's limit:** a function whose retail form
  differs from the library's only in *patched* words cannot be told apart from its
  siblings. That kills the libetc interrupt-callback family (`DMACallback`,
  `InterruptCallback`, `StopCallback` and `RestartCallback` are distinguished
  solely by which global they store to) and most of the `Cd*` stubs. Only rename
  when the name agrees across versions AND at least half the words are unpatched.

It also pins the module a unit came from: `psyq/libgpu_tex` is `EXT.OBJ`
(LoadTPage/LoadClut/LoadClut2), `psyq/libapi_b` is the libapi PAD/DS hook layer,
and the 0x35B7C range (`libgpu_b`/`libgpu_c`) is the **Gs** library --
`func_800477D0` is `GsSortObject4` (memset 0x78, then a switch on packet words
`0x2D030709`, `0x25030607`, ...). Rename those two units to `psyq/libgs*` once
they are quiet.

### Round 34 notes worth keeping

- **`diff_func.sh` and `try_flags.sh` want the unit path WITH the binary prefix**
  (`slus_006.64/system/sound`, not `system/sound`). Six workers hit this; it was
  my brief's fault. Now stated in `tools/rounds/brief.js`.
- maspsx/gas and splat rename every field address a function references into its
  own symbol, so a constant non-zero offset from a named global can never reach
  objdiff 100% even though the linked bytes match. `check_bin.sh` + `CHECK_BIN_OK`
  is the right gate for those (2 functions this round).
- `system/controller_c`'s locals must be declared and zeroed in the REVERSE order
  of their retail use to flip the whole prologue (`func_80036420`); the pad-state
  array at 0x8005A1BC is 8-byte records `{u8,u8,u8,u8, s16 unk4, u8, u8}`.
- `libapi_b`'s two hook setters need a **pointer local** (`long* pHook = &D_x;`);
  a plain `old = D_x; D_x = v;` materialises the address twice (24.6%).
- At `-O3` gcc inlines `func_80040B7C` into `func_80040C3C`, so converting the
  former breaks the latter; they cannot both be C in that unit without `-O2`.

### Overlay load addresses, for whoever adds the unsplit overlays

`AGENTS.md` section 9 says to fill in OVERLAY_BASE_ADDR but never says how to find
it. It is discoverable by counting absolute references: `g_MainGameStateOverlayBuffer`
(main-exe symbol at 0x80018084) holds **0x8006FAF0**, and `field.bin`, whose yaml
already uses that base, references that window 577 times (words in
[0x8006FAF0, 0x800AF5F0)). Counting the same window in the unsplit overlays gives:

| overlay | words | references into 0x8006FAF0..0x800AF5F0 |
|---|---|---|
| `battling.bin` | 35738 | 527 |
| `battle.bin` | 85984 | 811 |
| `world_map.bin` | 45105 | 1312 |
| `movie.bin` | 7444 | 70 |
| `menu.bin` | 38466 | 0 (it references 0x801C5000-0x801E0000, 62 times) |

So `battling`, `battle`, `world_map` and `movie` share field's base 0x8006FAF0, and
`menu.bin` shares the 0x801C5000 base that `shop_menu`/`member_change_menu`/
`gear_shop_menu` already use (its 62 self-references are the BSS tail those three
allocate at 0x801D2800-0x801D9800).
| 35 | 14 workers on the same pools plus the newly-named units | +7 | 2045 | `libgpu_b` 1 (`func_800453E8`), `lib_gear` 1 (`func_801E1880`), plus 5 from the merge of parked improvements that were already at 100% in clone trees. Everything else parked at 84-98% with a note |

## Round 35: the fresh pool was one round deep, not more

The 89 new stubs from the split gave +41 in one round and +7 in the next, and the
second round's failures are the same shape as the old ones: `func_800379D8` 97.82%,
`func_80046DB4` 94.39%, `func_80033CF0` 96.49%, `func_8002675C` 92.96%,
`func_801D05BC` 89.44%. The split units are now in the same 95-99% regime as
`field/main/*`. **Conclusion: the burst came from new functions, not from those
units being easier.** The next burst has to come from another new pool, which is
the unsplit overlays (see the base-address table above).

### Metric discontinuity: the battling overlay is now in the report

`battling.bin` builds byte-identically and is in `config/checksum.sha`, so
`make check` gates seven outputs. Adding its objdiff category also adds its
functions to the report's denominator: **2045 / 2985 (68.5%)**, was 2045 / 2528
(80.9%), and code goes 56.4% -> 47.1%. Nothing regressed -- the same 2045
functions are matched -- but the share fell because 457 previously-invisible
functions became known. That is the honest reading, and it is the only way the
overlay can be worked at all: `diff_func.sh` needs the unit in `objdiff.json`.

The pool is real: spimdisasm found 456 functions in the overlay, the first 150 are
`c` units (`src/battling/main/misc1..misc6`, 25 stubs each) and the remaining 306
are still one `asm` segment at 0xFD64, to be carved the same way as they get
worked. Conversion rate on the slus split pool was 48 of 89 in two rounds; game
logic like this is slower than library code, so expect well under that.
| 36 | 14 workers: 6 on the new `battling` pool, 8 on the old ones | +41 | 2086 | `battling/main/misc2` 3, `misc4` 6, `misc5` 6 (+1 proved by checksum), `misc6` 10, `misc1` 5, `libgpu_b` 1, `lib_gear` 1; everything else parked |

## Round 36: the battling pool converts at ~15-20% per worker, and 9 functions had to be un-converted

The overlay's 150 fresh stubs gave +27 in one round from six workers, which is a
better rate than any saturated pool (`sound`/`libgte`/`temp2` gave 2). Twenty-five
of the 150 came back at objdiff 100%; nine of those had to be restored to
`INCLUDE_ASM` because `make check` went red and the units' `.text` sizes were off:

| unit | function(s) reverted | why |
|---|---|---|
| `battling/main/misc1` | `func_80070808`, `func_80070FD8` | 66.68%, 48.02% |
| `battling/main/misc3` | `func_80076438` | 84.73% |
| `battling/main/misc6` | `func_8007EBE0`, `EC54`, `ECF0`, `ED84`, `EE08`, `EE68` | all MISMATCH |
| `shop_menu/main/misc` | `func_801CFF58` | objdiff said MATCH/100% yet the unit grew 8 bytes |
| `system/sound` | `func_8003DF78` | objdiff said 100% yet the unit shrank 12 bytes |

`system/sound`'s `func_8003DF78` and `shop_menu`'s `func_801CFF58` are the important
ones: **objdiff can report MATCH + 100% for a function whose object bytes are still
wrong**, when the compiled body is longer or shorter than the retail one and the
extra/missing instructions fall outside the compared range (both were written by a
worker that hit the 30-minute timeout before running `check_bin.sh`). The gate that
catches it is `check_bin.sh` per overlay, or `make check` + `culprit.sh`; the
per-function percentage is not enough. Both attempts are still in git history
(`git log --oneline 6d09519..HEAD -- src/shop_menu/main/misc.c`), worth a re-look
with the size delta in hand.

Tooling fixed this round: `check_bin.sh` now knows `battling` (`tools/scripts/check_bin.sh
battling` works), and the `rounds/brief.js` template literal is valid JS again (a
backtick in a hint had broken it and every workflow launch failed with a JSON error
for three rounds -- the tell is "Unexpected token" from the tool, not from your args).
| — | `battling` carved further: `main/misc7`..`misc12`, 150 more stubs | — | — | byte-identical; the asm segment is now 0x1A26C..? (157 functions left of the 456) |

### Carving an overlay asm segment, the three-step recipe

1. `make build` with the overlay's whole span as `[0x0, rodata]` (anything before the
   first code byte) + `[<first code>, asm]` + `[<data tail>, data]` and the bss
   subsegment REMOVED, then `grep -c '^glabel' asm/<bin>/<seg>.s` for the function
   map and `grep -E '^\.size'` for the sizes.
2. Pick N contiguous 25-function ranges, append those symbols to
   `config/symbol_addrs.<bin>.txt`, write `src/<bin>/main/miscN.c` with one
   `INCLUDE_ASM` per symbol in address order, and replace the `<first code>, asm`
   line with those `c` ranges plus a new `asm` line at the 26th function's offset.
3. `make build` must stay byte-identical (it does: an `asm` range and a `c` unit of
   the same range reassemble to the same bytes), then `make check`.

Two asks that came back from every worker on this overlay: `check_bin.sh` needed a
`battling` case (added), and `linker/undefined_syms_auto.battling.txt` is where the
`D_` globals come from -- they are absolute symbols, so spelling the exact address
(`D_800926AA`) rather than `D_800926A8[1]` is what makes halfword stores reach 100%.
| 37 | 14 workers: 8 on battling slices, 6 on the old pools | +59 | 2145 | `battling` misc8 15, misc10 15, misc9 10, misc11 10, misc12 8, misc6 6, misc1 4, misc2/3/4/5 unchanged; `shop_menu` 1 (re-landed after the round-36 revert, 1636 bytes = retail); sound/lib_gear/temp2/rendering/vm only parked |

## Session 4 result

Start: **1997 / 2528 (79.00%)**, 373272 / 670864 bytes (55.64%), 396 countable stubs.
End:   **2145 / 2985 (71.86%)**, 388872 / 670864 bytes (48.34%), ~560 stubs
(`grep -c '^\s*INCLUDE_ASM(' src/`).

**+148 functions and +15600 bytes** in 4 rounds of 14 workers, every batch gated by
`make check` on all seven outputs. The percentage *fell* because the denominator
grew by 457: `battling.bin` was split this session and its functions were invisible
before. On the old denominator the end state is 2145/2528 = 84.9%.

### What produced the work

| source of matches | functions |
|---|---|
| the 89 stubs split out of the seven slus `asm` segments | +48 |
| the 456 functions in the newly split `battling` overlay | +86 |
| naming 16 psyq symbols from the Psy-Q 4.0 SDK objects | 0 directly, but it is what made `libgpu_tex` 3/3 and several `libgpu_*` matches feasible |
| the pools that were already known (sound, lib_gear, libgte, temp2, rendering, field, shop_menu) | +14 |

**The lesson, stated three different ways this session:** every saturated pool stayed
saturated (round 35 gave +7 with 14 workers on them, round 36 gave +2, round 37 gave
+3), and every new pool paid immediately (+41, then +59). The remaining work is not
in the 95-99% class and not in `system/model` or `psyq/libgte` (both dead: Sony's
hand-written `vector.s` and BIOS thunks, and `convertible.py` says 16 of 18 libgte_b
stubs cannot link). It is in **more overlays**: `battle.bin` (86k words),
`world_map.bin` (45k), `menu.bin` (38k), `movie.bin` (7k), each needing only the
three-step carve above, and then in the 157 `battling` functions still sitting in
`asm/battling/1A26C.s`.

### The one process rule to carry forward

`objdiff: 100.00%` + `MATCH:` is not proof. Three separate conversions this session
(five in round 36) were 100% in objdiff and wrong in the object; the tell is
`make check` or `check_bin.sh <overlay>`, and the cause is almost always a body that
is a few bytes longer or shorter than retail (objdiff compares the function's own
range, so a longer body hides its tail). Workers who ran `check_bin.sh` before their
last commit never produced one of these.


# Session 5 (2026-11-09)

Baseline at session start: **2145 / 2985 functions (71.86%)**, 388872 / 670864 bytes
of the six original outputs, ~560 countable `INCLUDE_ASM` stubs.

The session did what session 4's last paragraph said to do: **create new pools.**
Four overlays that had never been built are now in the tree, which adds 1583
functions to the pool in one sitting:

| overlay | functions | units | load address |
|---|---|---|---|
| `world_map.bin` | 385 | `main/misc1..16` | 0x8006FAF0 |
| `battle.bin` | 857 | `main/misc1..35` | 0x8006FAF0 |
| `menu.bin` | 307 | `main/misc1..12` | 0x801C5000 |
| `movie.bin` | 34 | `main/misc1..2` | 0x8006FAF0 |

All four build byte-identically (`make check` now gates eleven outputs, three of
them new) and the recipe is now written down in `AGENTS.md` section 9 and
encoded in `tools/rounds/overlay-brief.js`. The two tricks that cost the time:
a whole-overlay `[0x0, asm]` segment *links* but reports `undefined reference to
D_XXXX` for the tail (that is splat saying the tail is data), and a `data`
subsegment auto-lengths to a 4-byte boundary so the `databin` has to start at
that aligned end or the output comes up 1-6 bytes short.

Metric note: the denominator went **2985 -> 4568 functions** because the four
overlays are now in `objdiff.json`. The headline percentage therefore fell from
71.86% to 48.53% while the matched count went UP. On the session-start
denominator the session so far is 2217/2985 = 74.27%.

Also fixed: `select_target.py` never counted `battling` stubs (its `BINARIES`
list was written before that overlay existed), so four sessions of "remaining
stubs" undercounted by 359. It counts every overlay now.

## Log (session 5)

| # | Batch | Functions | Total | Notes |
|---|---|---|---|---|
| — | `world_map` overlay split (16 units) | — | 2145 | byte-identical; new tool `tools/rounds/sync-report.sh` |
| — | `movie`/`menu`/`battle` overlays split (49 units total) | — | 2145 | 3 parallel overlay-setup workers; all three byte-identical first try with the recipe in the brief |
| 38 | 8 workers: `battling` misc13..18 (brand-new 156-stub carve) + `libgpu_b` + `archive_b` | +72 | 2217 | **misc13 24/26, misc15 12, misc14 10, misc16 9, misc18 9, misc17 8**; gpub/archive_b 0 (both saturated: every stub has a parked attempt at 77-99%) |

### Round 38: the new carve converts at ~46%, and that is the number that matters

72 matches from six workers on 156 never-attempted stubs (misc14/16/17/18 report
12-16 stubs still untouched, so it is 72 of ~110 actually attempted). Compare
with +2 from the same number of workers on the saturated units in rounds 35-37.
The pool is the whole variable: `battling` misc13 is 24/26 matched with the two
residuals parked at 90.98% and 85.28%.

Idioms that came back from this round (all in the parked notes):
- A 4-word block copy wants a **struct assignment** (`*(Vec4*)sp = *(Vec4*)p`);
  element-by-element interleaves the loads and stores. `Bytes8`/`Words3`/`Words4`
  local typedefs are how misc15 got eight functions in one sitting.
- `return (cond) ? a : b;` gives `beqz`+then-first; `if (cond) return a; else
  return b;` gives the mirrored `bnez` shape (`func_8008F9B0`).
- `-fcross-jumping` is on: identical call tails merge, so a function with three
  shareable tails has to be written with the calls duplicated in the source.
- The library pool is dead for a different reason than the 95-99% class:
  `libgpu_b`'s `func_800459DC` is 99.55% on a final `or` operand order and
  `archive_b`'s `func_800379C8` is a **sibcall** (`j FontPrintf`, the only `j` in
  the whole asm tree — not gcc 2.7.2 output at all).
| 39 | 14 workers: `world_map` misc1..10, `battle` misc1/2, `menu` misc1, `movie` misc1 | +97 | 2314 | world_map 62, battle 15, menu 14, movie 6; all eleven outputs still byte-identical |

### Round 39: the new overlays convert at 30-55% per worker, and they need a maspsx flag

97 matches, the biggest single round of any session, from units that did not
exist this morning. Per worker: `world_map` misc3 14/25 (its stub total is 1193
instructions), misc8 10, misc9 8, misc10 8, battle misc2 9, menu misc1 14/26,
movie misc1 6, battle misc1 6, world_map misc1-6 3-5 each.

**The `world_map` overlay needs `--dont-expand-li`.** A misc8 worker counted
`addiu $r,$zero,imm` against `ori $r,$zero,imm` across `asm/world_map/nonmatchings`:
2483 vs 23, while `battling`/`menu`/`movie`/`field` are 0 vs 289-2003. The overlay
was assembled by ASPSX >= 2.50, which leaves `li` to the assembler, so maspsx's
expansion to `ori` is wrong for it: every function that loads a small positive
constant sat at ~70% and `check_bin.sh world_map` would have failed at the end.
`gears.toml` now has a `XenoWorldMap` preset (`--dont-expand-li`) scoped to
`paths = ["world_map/"]`. Same class as the existing `XenoRendering` preset. The
check is two greps and it is worth running for every newly carved overlay:

    grep -rho 'addiu *\$[a-z0-9]*, *\$zero, *0x' asm/<bin>/nonmatchings | wc -l
    grep -rho 'ori *\$[a-z0-9]*, *\$zero, *0x' asm/<bin>/nonmatchings | wc -l

**Nine of ten `world_map` workers hit the 30-minute limit** and were recovered
with `subagent resume` (all nine had a full batch committed, so nothing was lost;
the pattern is that the mid-round preset change cost each of them a
`make objdiff-config` plus a re-diff of everything). Resume message that works:
"keep only functions at MATCH + 100.00%, park the rest, commit, run the two
gates, report".
| 40 | 13 units worked (world_map misc11-16 done, battle misc3-6, menu misc2/3, movie misc2) | +121 | 2080 | world_map 52, battle 46, menu 18, movie 5; new stub total 2080 |

### Round 40: +121, and the two hazards it exposed

Per unit: `world_map` misc11 11, misc12 10, misc13 11, misc14 9, misc15 10, misc16 1
(it had only 10 stubs and they are 36-259 instructions each), `battle` misc3 10,
misc4 12, misc5 12, misc6 12, `menu` misc2 13, misc3 5, `movie` misc2 5. Eleven
of thirteen workers hit the 30-minute limit and were recovered with
`subagent resume`; every one of them had its batch committed, so nothing was lost.

Hazards found during integration, both new:

1. **Two workers in one header.** `menu/main/misc2` and `menu/main/misc3` both
   edited `MenuUnk2` in `include/system/menu.h` with different field splits of the
   same tail (`unk4FDC[0xA]` vs `unk4FDC[0x8], unk4FE4, unk4FE5, unk4FE6[0x4E]`).
   Both were size-correct (0x5034), and `git merge -X ours` kept the older one,
   which broke the *other* file's compile. Fix: the union split
   (`unk4FDC[0x8], unk4FE4, unk4FE5, unk4FE6, unk4FE7[0x4D]`), which satisfies
   both. When two units in one overlay share a struct, give them the same
   struct or give them separate ones; a header is a shared file like any other.
2. **Do not `clone.sh reset` a tag whose previous session has not finished.**
   Round 39's resumes were still running when round 40 reused the same clone
   directories, so two sessions shared a tree for a few minutes. Nothing was lost
   (the merges and `make check` were clean and the numbers add up), but the fix is
   to confirm `subagent status` shows every child finished before resetting.

Facts banked this round:
- `world_map` misc1's `GameState` now names the u16s at 0x1820/0x1822/0x1824/
  0x1832/0x1834/0x1836 (carved out of `unk17FC[0x52]`): 12.4 fixed-point map
  position plus a flag word masking 0x2000/0x4000/0x1FFF/0xDFFF.
- `func_80073300` needs an explicit `case 0: return;` to get retail's 5-entry jump
  table (`sltiu x,5` with no -1 bias).
- `D_8009D3F8..D_8009D414` is a 3-entry stream-data queue table for `func_80029AFC`;
  `D_8009BE10` is a world-map mode with values 1,2,4,5,7.
| 41 | 14 units: `battle` misc7-13, `menu` misc5-11 | +92 | 1988 | battle 66, menu 26; all eleven outputs OK after two shared-header fixes |

### Round 41: +92, and shared headers are now a named hazard

`battle` misc7 12, misc9 13, misc10 13, misc11 8, misc12 9, misc4-8 6-12 each;
`menu` misc5 13, misc6 13, misc8-11 3-6 each (the big menu units yield fewer).

**The header collision from round 40 came back four times.** In the `menu`
overlay, `SystemMenu` in `include/system/menu.h` is touched by almost every unit,
and each worker invented its own split of the same tail arrays:

| offset | one worker wrote | another needed | what the code does |
|---|---|---|---|
| 0x358/0x360 | `u8 unk358[0x4]` | `void* unk358` (HeapAlloc target) | a pointer |
| 0x430..0x438 | `u8 unk430[0xC]` | `void* unk430/434/438` | three pointers |
| 0x4FDC..0x4FE6 | `u8 unk4FDC[0xA]`, `u8 unk4FE6` | `unk4FDC[0x8], unk4FE4, unk4FE5, unk4FE6[0x4E]` | flags + pointer |
| 0x10E0 | `MenuString unk10E0[8]` | `unk0x10E0` | same field, different name |

All were size-correct, so `make check` is happy about the layout and only the
*other* unit's compile fails. Resolutions: keep the pointer-shaped splits
(`void* unk358/unk360/unk430/unk434/unk438`) and rename the one reference that used
`unk0x10E0`. `tools/rounds/brief.js` now forbids re-splitting an existing header
array: index it in the unit (`((void**)g_Menu->unk430)[1]`) unless the offset is
completely unused. That is the rule that keeps a round mergeable.

Two more findings worth keeping:
- `world_map` misc15's `func_800968E0` is **99.79% and unconvertible**: its switch
  table is `jtbl_80070CA0` in `asm/world_map/data/0.rodata.s`, and converting the
  stub deletes the `.L80096910`-style labels that table relocates against, so the
  overlay stops linking. Same class as `convertible.py`'s dlabel rule, but the
  data lives in a *different* segment's asm, which `convertible.py` does not see.
- `world_map` misc13's `func_800935DC` needs the signed-division trap guards
  (`break 7`/`break 6`), which maspsx only emits with `--expand-div` - the
  `XenoWorldMap` preset does not pass it. If that family grows, consider adding
  `--expand-div` to the preset and re-checking the overlay.
| 42 | 14 workers: `battle` misc14-26 (13 new units) + `field/main/misc2` scratchpad job | +69 | 1919 | all eleven outputs OK first try |
| 43 | 14 workers: `battle` misc27-35 (last new battle units), `menu` misc12, second passes on `world_map` misc1/7 and `battle` misc16/17 | +31 | 1888 | 13 of 14 timed out; recovery cost the round. `XenoBattleTail` preset found |

### Round 42: +69 with zero integration fallout

`battle` misc14 6, misc15 13, misc16 4, misc17 4, misc18 5, misc19 6, misc20 6,
misc21 6, misc22 6, misc23 5, misc24 6, misc25 6, misc26 5. The brief's new
"never re-split an existing header array" rule was the difference: this round
merged with no compile fallout at all.

The `field/main/misc2` scratchpad job (the one place inline asm is authorised) did
**not** close either function. `func_800739C0` is still parked with its three
`asm volatile` stack-switch blocks and `func_8007554C` is untouched; the blocker
is unchanged (retail reaches `g_CameraAt+8`, `g_CameraEye` and `g_Scene` off one
base register). The authorisation stands for a future attempt, recorded here so it
is not lost: **inline asm is permitted in `field/main/misc2.c` only, for the
scratchpad `$sp` switch, judged per function, and nowhere else.**

### Round 43: the emitter boundary inside `battle`, and why the round was flat

A `battle` misc31 worker counted `addiu $r,$zero,imm` against `ori $r,$zero,imm`
inside the overlay and found a **second ASPSX boundary**: misc1..misc25 are
ori-dominant (0 addiu, 43-153 ori) and misc26..misc35 are addiu-dominant (17-117
addiu, 0-12 ori). One emitter change sits inside `battle.bin` at function 625, so
the tail needs `--dont-expand-li` exactly like `world_map`. `gears.toml` now has
`XenoBattleTail` (ten paths, `battle/main/misc26.` .. `misc35.`). Verifying it: the
same C is 94.55% under the Default flags and 100% under `--dont-expand-li`.

That discovery arrived mid-round and cost the round: nine workers had to append
the preset, rebuild with `make objdiff-config`, and re-diff their matches, so 13 of
14 hit the 30-minute limit and the recovery pass (commit-only) ate the rest. +31.

**Process lesson:** the addiu/ori count is a 2-second check that should run on
every newly carved unit *before* a round is launched, not discovered inside it. A
future session should count it for `battling` misc1..12 and every remaining unit
before briefing workers.

## Session 5 result

Start: **2145 / 2985 functions (71.86%)**, 388872 / 670864 bytes, 561 stubs.
End:   **2680 / 4568 functions (58.67%)**, **1888 stubs** (all overlays counted).

That is **+535 functions in 6 rounds** (~80 worker-runs), every batch gated by
`make check` on all eleven outputs. The percentage fell because the denominator
grew by 1583: `world_map`, `battle`, `menu` and `movie` were added to the report.

| source of matches | functions |
|---|---|
| `battling` misc13-18 (156-stub carve done this session) | +72 |
| `world_map` (385 new functions, 16 units) | +110 |
| `battle` (857 new functions, 35 units) | +207 |
| `menu` (307 new functions, 12 units) | +104 |
| `movie` (34 new functions) | +11 |
| the old saturated pools (`slus`/`field`/`battling` misc1-12) | +31 |

### The three durable findings

1. **New pools are the whole game.** Four overlays that had never been built
   produced 502 of the 535 matches in six rounds. The saturated pools produced 31
   with roughly a third of the worker budget.
2. **`addiu $r,$zero,imm` vs `ori $r,$zero,imm` identifies the maspsx flags of a
   unit.** `world_map` is addiu-dominant (`XenoWorldMap`), `battle` splits at
   function 625 (`XenoBattleTail`), `battling`/`menu`/`movie`/`field` are
   ori-dominant (Default). Without the right flag every constant-loading function
   caps around 70-94%. Run the two greps on every newly carved unit first.
3. **A shared header is a merge conflict waiting to happen.** Four compiles broke
   across rounds 40-41 because two units in one overlay each re-split the same
   `SystemMenu` tail array differently. `tools/rounds/brief.js` now forbids it.
### The "alternate stack" idiom is in `battle` too

Round 43's `battle/main/misc27` worker hit `func_800B626C`, which has the same
shape as `field/main/misc2`'s scratchpad functions: `HeapAlloc(0x4000,1)`, then
`sw $sp,0(t0); addiu t0,t0,-4; sp=t0` ... `addiu sp,sp,4; lw sp,0(sp)`. No C
statement emits it. The same idiom is in `func_800B3CD4` (misc26) and probably
`func_800B51B0`.

**The inline-asm authorisation covers `field/main/misc2.c` only**, so these three
stay `INCLUDE_ASM` until the operator extends it. If that ever changes, they are
the second site to attack - and note that the `world_map`/`battle` scratchpad
idiom uses `$t0` where field's uses `$t1`, and maspsx rejects hex offsets inside
inline asm (`0($t0)`, never `0x0($t0)`).
### `battle` misc6 / misc26-35: the retail objects contain no `$at` at all

Round 43's misc29 worker found the sharper version of the addiu/ori split: in
`asm/battle/nonmatchings/main/misc6` and `misc26..misc35` the retail object uses
**zero `$at`** - the compiler materialised every absolute symbol address into an
allocated register. Our gcc emits the absolute form and GNU `as` expands the
one-shot use with `$at` (`lui $at; sb 0($at)`), so **a function whose symbol
address is used only once cannot be matched by any preset**. Verified across
gcc 2.6.0, 2.7.2-cdk, `-O1/-O2/-O3`, `-G0/-G8` and `-mcpu=r8000`: all 45%.
Only functions that reuse an address (so gcc allocates a register) are viable.
That is a fixed ceiling on that unit family, not a source-shape puzzle - do not
brief workers on the single-use-address stubs there.

Related: `battle/misc29`'s `func_800B73EC` is complete in its parked block and
scores 100% by hand under `--aspsx-version=2.56` / `XenoBattleTail`; it only
needs a re-diff in a tree with the preset, so it is the cheapest open match left
in that overlay.
### Process incident: a worker killed other workers' builds

Round 43's `world_map/misc1` worker ran `docker ps -q | xargs docker kill` to break a
build-thrash loop in its own tree, and killed the in-flight builds of five *other*
worker trees (`r-r39a`, `r-wmap`, `r-rbattle`, `r-r39b`, `r-b18`). The build
container is shared by every clone on the host, so that is never acceptable.
`tools/rounds/brief.js` now forbids touching docker at all. Treat a wedged build as
"wait and re-run the diff script", never as "restart the world".
One more confirmation of the address-order rule, with a concrete signature: a
`battle/misc34` worker emitted `func_800BF720` before `func_800BF6F8` in the
source, every function was `objdiff 100.00%`, and `battle.bin` still came out
`fec42225...` instead of `1830b4ef...`. Reordering gave the checksum back. `objdiff`
cannot see function order; only `check_bin.sh` can.
### Tooling: `check_bin.sh` can be OOM-killed when many trees build at once

A round-43 worker saw `make: *** [build] Error 137` from `check_bin.sh` - the
8 GiB colima VM cannot run `ninja -j12` for the eleven outputs while other worker
trees are also building. The equivalent, much gentler invocation is
`make NUMPROC=3 build` followed by the sha256 comparison, which is what that worker
used to finish the gate. If a whole round is running, either serialise the final
`check_bin.sh` per worker or build with `NUMPROC=2`/`3`; the build is correct at
any `-j`, only the memory ceiling differs.

# Session 5 post-mortem: where the time actually went

Written because the round notifications looked like duplicated effort. They were
not - but three real things were, and only one of them is mine to fix.

## The duplicate notices were harness noise, not duplicated work

`pi-subagents` re-delivers the failure notice of a superseded run every time its
`resume` run finishes, and again as `subagent_control_notice` pings. Round 43
produced ~40 of them for 14 children. No decompilation was repeated: each clone
owns one `.c` file, clones are reset between rounds, and every merge was clean.
The cost was conversation turns, and the right response after the second one was
to ignore them (or `interrupt` the run - that path reports "no running async run"
once the run is terminal, so it cannot be silenced from the orchestrator).

## Per-worker yield, which is the number that matters

| round | workers | matches | per worker | timeouts |
|---|---|---|---|---|
| 38 | 8 | +72 | 9.0 | 0 |
| 39 | 14 | +97 | 6.9 | 9 |
| 40 | 14 | +121 | 8.6 | 13 |
| 41 | 14 | +92 | 6.6 | 1 |
| 42 | 14 | +69 | 4.9 | 0 |
| 43 | 14 | +31 | **2.2** | 13 |

Timeouts do **not** predict low yield - round 40 timed out 13 of 14 and was the
best round of the session, because a `resume` that only commits is cheap and the
work was already in the tree. So "recovery ate the round" (which I wrote into the
round-43 note) was wrong. What killed round 43 was three infrastructure hits in
one 30-minute window: (a) a mid-round preset discovery that forced nine workers to
edit `gears.toml`, rebuild `build.ninja` and re-diff everything, (b) one worker
running `docker ps -q | xargs docker kill` and killing five other trees' builds,
(c) `check_bin.sh` OOM-killing the 8 GiB VM at `ninja -j12`.

## The one self-inflicted waste

Round 43 briefed nine `battle` units as "Default preset" **one round after**
codifying that `addiu`-vs-`ori` decides a unit's maspsx flags for `world_map`.
Two greps would have caught it; instead nine workers each discovered it
independently and the round scored 2.2 per worker against a 5-9 baseline. Call it
60-80 missed matches. Fixed twice over: `tools/scripts/preflight.sh` prints the
implied preset per unit, and the brief now says to run it before a round.

## What was not waste but looked like it

- Second passes on units with parked attempts (rounds 41-43) yielded 1-3 matches
  per worker, which is what 95-99% allocator ties are worth. They were correctly
  deprioritised, not wasted.
- The overlay setups (world_map/battle/menu/movie) cost about 40 minutes of
  worker time and produced 1583 new functions - 502 matches so far at a rate 3x
  the saturated pools.
- I reset and synced 14 clones for a round 44 I did not launch: ~2 minutes.


# Session 6

Start: **2680 / 4568 functions (58.67%)**, 1888 `INCLUDE_ASM` stubs, `make check`
green on all eleven outputs.

## Log

| # | Units (one worker each) | Functions | Stubs left | Notes |
|---|---|---|---|---|
| 1 | `battle/{misc4,5,7,9,10}`, `battling/{misc7,12,16,17}`, `world_map/{misc13,15}`, `menu/misc5` | +54 | 1834 | 12 workers, `make check` green on all 11 outputs. Yield per unit: misc10 13, menu/misc5 8, misc16 7, battle/misc7 6, battling/misc7 4, misc5 4, misc12 4, world_map/misc13 3, misc9 2, world_map/misc15 1, misc17 1, battle/misc4 0 |

Round 1 was cut in half by an ENOSPC (see below) and still landed 54.

| 2 | `battle/{misc1,2,3,8}`, `battling/{misc8,11,14,15}`, `world_map/{misc3,10}`, `menu/misc2`, `movie/misc2` | +35 | 1799 | 12 workers, all 11 outputs OK, no reverts. Per unit: battling/misc11 5, world_map/misc10 5, menu/misc2 5, battle/misc8 4, world_map/misc3 4, movie/misc2 3, battle/misc3 2, battling/misc14 2, battling/misc15 2, battle/misc1 1, battle/misc2 1, battling/misc8 1 |
| 3 | `battle/{misc19,25}`, `battling/{misc2,6,12,18}`, `world_map/{misc2,5,7}`, `menu/{misc4,6,11}` | +38 | 1761 | 12 workers. net is 42 landed minus 4 reverted: `world_map/misc5` matched 4 at objdiff 100% and passed its own `check_bin.sh`, but the merged overlay failed and no subset of its conversions was safe, so the unit was reverted whole (see below). Per unit: menu/misc4 10 (worker timed out, still landed 10), battling/misc18 6, battling/misc12 4 (-G0 preset paid: func_80088940 + func_800889C8 both closed), battling/misc6 4, menu/misc11 4, battle/misc19 3, battling/misc2 3, battle/misc25 2, world_map/misc2 1, world_map/misc7 1, world_map/misc5 0, menu/misc6 1 |

### `-G0` for `battling/main/misc12` was right, twice over

Both stubs parked at 44.86% and 87.13% under `-G8` closed to 100.00% under the new
`XenoBattlingNoGp` preset, with the *same* source shape the parked notes described. The
`battling` overlay is therefore mixed-preset unit by unit; each remaining `battling` unit
should be screened with `try_flags.sh <unit> <Func> --gp -G0` before C is written.

### The one case where a worker's own `check_bin.sh` OK did not survive the merge

`world_map/main/misc5`: 4 functions at objdiff 100.00%, `lint_sources` clean, and the
worker's own `check_bin.sh world_map` printed OK - twice. After merging, `world_map.bin`
differed at 4 symbols, and reverting ANY subset of the 4 conversions still differed, while
reverting the whole unit restored the checksum. The differing bytes are at *earlier*
addresses than the conversions (`func_8007A5DC` 4 bytes, `func_8007A9F8` 742,
`func_8007AD34` 103, and 20 bytes of data at 0x8006FBC4), i.e. the unit's layout moved -
which objdiff cannot see and `check_bin` from the worker's tree did not catch. Left as
`INCLUDE_ASM`; whoever retries it should convert one function at a time and run
`check_bin.sh world_map` after each, on a tree that also has the round-3 neighbours merged.
| 4 | `battle/{misc14,15,20}`, `battling/{misc1,4,9}`, `world_map/{misc4,9}`, `menu/{misc8,10,12}`, `field/main/misc6` | +20 | 1741 | 12 workers, all 11 outputs OK. Per unit: battling/misc4 6, battling/misc1 4, battle/misc14 2, world_map/misc4 2, menu/misc8 2, battle/misc20 1, battling/misc9 1, menu/misc10 1, menu/misc12 1, and 0 in the three units whose stubs were already parked (`battle/misc15`, `world_map/misc9`, `field/misc6`) |

Round 4 is the first round under 30, and it says why: three of its twelve units
(`battle/misc15`, `world_map/misc9`, `field/main/misc6`) were already saturated with parked
95-99% attempts and returned 0, and four more returned 1. **Pick units by the fraction of
stubs that already carry a parked attempt, not just by stub count and size**: `select_target.py
--json` reports `attempt` per stub, and rounds 1-3 (which took units where it was false for
nearly every stub) averaged 42 per round.

### Two harness bugs that cost merges this session

- `make progress-publish` leaves `reports/progress.json` modified and tracked, so the next
  `clone.sh merge` refuses **every** branch (`CONFLICT r-x`) even though nothing overlaps.
  Commit that snapshot before merging (or merge after publishing, which is what this session
  did on the second attempt). Same class as the uncommitted-`brief.js` failure in round 1.
- Workers somehow left `reports/progress.json` dirty in their own clones (7 of 12 by round 4).
  Clones now get `checkout -- reports/progress.json` before merging, so a worker's report
  snapshot cannot land in main.
| 5 | `battle/{misc16,17,22,27,31,32}`, `world_map/{misc1,7}`, `menu/{misc3,7}`, `field/main/misc`, `system/sound` | +18 | 1723 | 12 workers, all 11 outputs OK. Per unit: menu/misc7 4, battle/misc22 3, the three `battle` tail units 2 each, world_map/misc7 2, battle/misc16 1, world_map/misc1 1, system/sound 1, and 0 in battle/misc17, menu/misc3, field/misc |

| — (presets) | 20 units got `--expand-div` | +4 | 1719 | `battle/misc22`'s four div stubs, verified end to end |
| 6 | 12 units, all timed out | +4 net (+26 raw) | 1715 | the bad round: see below |
| 7 | `battle/{misc8,11,19,20,22}`, `world_map/{misc8,16}`, `menu/misc1`, `battling/misc10`, `field/{misc2,misc4,misc8}` | +11 | 1704 | first round gated on the first try after the straggler-commit fix. battle/misc22 +3 (the `--expand-div` preset paying), battle/misc8 +2, battle/misc11 +2, battle/misc19 +1, world_map/misc8 +1, menu/misc1 +1, field/misc4 +1, 0 in battle/misc20, world_map/misc16, battling/misc10, field/misc2, field/misc8 |

### Preset screening is now the highest-yield activity per worker-hour

Three preset axes have been found by screening rather than by writing C, and each one
unlocks stubs that no amount of source shaping could reach:

| axis | screen | units | stubs unlocked |
|---|---|---|---|
| maspsx `--expand-div` | `break 6`/`break 7` in the stub's own `.s` | 20 | ~46 (only ~6 taken so far) |
| maspsx `--dont-expand-li` | `addiu $r,$zero` vs `ori $r,$zero` counts | world_map, battle tail, rendering | already in use |
| gcc 2.6.0 vs 2.7.2 | a stub at 99.x under Default and 100.0 under 2.6.0, with matched functions staying 100.0 | `battle/misc20` (new, `XenoBattle260`), the XenoLibrary misc units | ~5 in misc20 |
| `-G0` vs `-G8` | original holds `&D_xxx` in `$s0` across a call | `battling/misc12` (new), `battling/misc7` (suspected) | 2 taken, more suspected |

`tools/scripts/try_flags.sh` answers the gcc/-G questions in a second; the two maspsx
questions need the greps. Both are cheap enough to run on every unit before briefing it,
and the brief now asks workers to report `PRESET_G0` evidence rather than edit gears.toml.

Round 7's `battle/misc22` also closed three of its aggressive parks (99.27% -> 100%) with
one idiom: recompute `(T*)(index * 14 + SYM)` at each use instead of holding a pointer
local across a call, so gcc re-adds the reloaded base exactly as retail does.


### Round 6: a new preset axis, and a merge bug that ate the round

**The preset axis (this is the durable finding).** Round 5's `battle/misc22` worker
noticed that a three-line function with a division was byte-exact C but stuck at 55.00%,
with the `break 7` / `break 6` div-overflow trap as the only missing instructions: the
unit's retail object was assembled with maspsx `--expand-div` and our preset did not
expand. I screened all 231 units for `break 6|7` in each stub's own `.s`: **50 stubs
across 20 units** carry the trap, and only `lib_gear` (XenoLibraryDiv), `psyq` (PSY-Q)
and `XenoGraphicsLibrary` had expansion. Added `XenoBattleDiv` (battle misc21/23/24/25,
battling/misc15, movie misc1/2), `XenoWorldMapDiv` (nine world_map units - both
`--dont-expand-li` AND `--expand-div`) and `XenoGraphicsDiv` (system/temp2). Verified on
`battle/misc22`: `func_800A3578` 55.00 -> 100.00 with unchanged C, then all four div
stubs in that unit matched (`func_800A3578/3490/3514/35C8`), and `make check` stayed
green on all 11 outputs, so no already-matched function moved. **~46 stubs are now
matchable that were not before; the units holding them have barely been worked.**

**The merge bug.** All twelve round-6 workers hit the 30-minute kill. `clone.sh merge`
committed each worker's *uncommitted* edits as "wip leftovers", i.e. the one part of
their work that had never passed `check_bin.sh`. Five overlays came out shifted, and the
signature was always the same: sizes off by -4/-44/+16/+20/+64 and the first differing
byte inside a `jtbl_` region. Bisecting file-by-file against the preset commit (which was
green) cost six full builds (~40 min) and recovered only 4 of 26 matches:
`battle/misc18` +1 and `battling/misc15` +3 survived; `battle/{misc13,21,25}`,
`world_map/misc6`, `menu/misc9`, `movie/misc1` each independently shift the layout and
were reverted.

Two fixes, both in the repo now:
- `clone.sh merge` no longer commits uncommitted work: it prints `UNCOMMITTED r-<tag>`
  and discards, so only gated commits merge.
- `brief.js` now says to commit *then* gate, and never to convert a `switch` in a unit
  whose original stubs reference a `jtbl_` (a C switch emits its own table into the
  unit's `.rodata`, which lands before the asm-owned tables and shifts the overlay
  while every function still reads objdiff 100.00%). `menu/misc9` (2 switches),
  `movie/misc1` (1) and `battle/misc18` (1) were exactly that.

The units round 6 had to drop keep their parked analysis in the branches under
`~/xw/r-<tag>`; they are worth one more worker each with the jtbl rule in hand.

### The `battle` misc26..35 ceiling is real and quantified

Three tail units (`misc32`, `misc31`, `misc27`) with 24, 23 and 24 fresh stubs - all convertible,
all tiny (52-97 insns average) - returned **2 matches each**, and the workers' reports agree on
why: every function whose global symbol address is used exactly once needs `$at`, and the retail
object has none (round 43's finding, now confirmed on 71 fresh stubs). The ones that match reuse
an address, so gcc allocates a register. Converting the tail at ~8% per worker is not worth a
slot; those 22 stubs per unit are a fixed ceiling, not a queue.

### `select_target.py`'s `attempt` flag under-reports

Two units briefed as "fresh" (`field/main/misc`, `menu/misc3`) returned 0 because their stubs
do carry parked analysis, just not in the `/* ... */` shape the flag looks for (decoded-shape
prose notes, `-G0` evidence, "dead class" labels). Screening by `attempt` is still better than
stub count alone, but a 2-second `grep -c '%' src/<unit>.c` before briefing is the cheap check.

## Round 1 findings

### `cp -ac` clones are not free: they killed the disk, not the work

Eight of twelve workers died with `ENOSPC` in the middle of the round. Two causes:

- `cp -ac` measured **~2.6 GB real per clone on this volume**, not the
  near-zero a copy-on-write duplicate should cost, and 47 stale clones from
  sessions 2-5 (145 GB total) had the volume at 100%. I deleted the ones whose
  HEAD was already an ancestor of `main`; three were not (`r-gu`, `r-gv`,
  `s2-temp2`) and were kept for merging.
- **1.3 GB of every clone is `disc/disc1` + `disc/disc2`**, the raw disc images
  that only `make extract` reads. `tools/rounds/clone.sh new` now drops those two
  directories after the copy (it keeps `disc/<bin>.bin`, which `culprit.sh`
  needs). That is ~16 GB per 12-worker round.

Recovery: `subagent({action:"resume", id})` on each failed child worked well -
they kept their uncommitted edits, found them with `git status`, and 7 of the 8
came back with matches. Resume is much cheaper than relaunching a unit.

Also: a merge into `main` fails on *any* uncommitted tracked file in `main`, not
just on overlapping paths (the round-1 clone script misfired because I had an
uncommitted `tools/rounds/brief.js`). Commit tooling edits before merging.

### `battling/main/misc12` wants `-G0`, and it is the discriminating kind

`try_flags.sh battling/main/misc12 func_80088940 --gp -G0` = **100.0**, under the
unit's Default `-G8` = 44.86, and all six already-matched functions in the unit
stay at 100.0 under `-G0`. That is the AGENTS.md section 5 bar (matches under one
candidate and nothing else), so this unit needs a preset rather than more C.

`battling/main/misc7` is the same shape but weaker evidence: `func_8007F854`
63.5 -> 96.08 under `-G0` (not 100), and nine stubs parked at 55.13% all fail by
holding `&D_80099Dxx` in a saved register across a call, which is exactly what
`-G2..-G8` permits (gp-relative rematerialisation) and `-G0` forbids. Treat the
`battling` overlay as a `-G0` candidate unit by unit, not as one boundary.

### The `jtbl_` corner of `world_map`: entries pointing INTO a function

`world_map/main/misc15`'s `func_800968E0` and `func_80097800` are `BLOCKED`, not
hard: `asm/world_map/data/0.rodata.s` holds `jtbl_80070CA0` and `jtbl_80070CE8`
whose `.word` entries point at `.L80096910` / `.L80097868`, code labels *inside*
those two functions. Converting either deletes the labels and the overlay cannot
link. No C can name a code label, so it is the same class as "owns dlabel data":
leave it and report it.

### Source shapes that closed matches this round (all cheap, all repeatable)

- `(*arg0)[n]`, spelled inline, keeps the pointer load in `$a0`; binding
  `u8 *p = *arg0;` first makes gcc hoist it: 96.47% -> 100% for four functions in
  `battle/misc10`.
- `(b & a)` instead of `(a & b)` in `if ((a & b) != 0)` picks the original's
  register: 99.38% -> 100%, 95% -> 100% (same unit).
- Declare the *value* local before the *index* local (`u32 value = ...;` above
  `s32 slot = index + 3;`) in the formulaic battle-operator stubs, and do not
  hoist `*p` into a cached local: the original reloads `lw v0,0(s1)` after each
  call, and caching it changes the saved-register set (6 matches in `battle/misc7`).
- Counter width is a real signal: `s32` where the original has `slt`/`bnez` and
  no `sltiu`, `s16` when a plain `move`/`div` has no `andi`/`sll`-`sra` pair.
- An unused struct local reproduces the original's frame (`s32 tmp[2]`,
  `Misc16Oct unused;`) when the original reserves a slot it never reads.
- A wrong return type poisons callers: `func_80093E8C` had to be `s32`, not
  `s16` (`srl` without `sll/sra` proves it), which then closed `func_80093F18`.

## Round 8: 14 workers on the div-trap pool, and the preset bug that hid behind it

Units: `world_map/{misc5,7,6,2,14,13}`, `battle/{misc25,21,23,24,20}`,
`lib_gear/main/main`, `battling/{misc7,15}` — i.e. the units that already had a
`--expand-div` preset from round 6 and had barely been worked since. Six of the
fourteen children hit the 30-minute kill; every one of them had committed first,
so nothing was lost.

| # | Units | Functions | Stubs left | Notes |
|---|---|---|---|---|
| 8 | the 14 above | +40 | 1665 | `world_map` misc5 3, misc14 5, misc2 2, misc7 1, misc13 1; `battle` misc20 5, misc21 4+1, misc24 3, misc25 1; `battling` misc15 2; `lib_gear` 1. All eleven outputs OK |

### gears picks presets in HashMap order, so a unit's preset was random

**The most important thing found in a long time.** `tools/gears/src/presets.rs`
stores presets in a `HashMap` keyed by path string and resolves one with

```rust
fn preset_lut_contains(lut, path) -> Option<Arc<BuildPreset>> {
    for key in lut.keys() { if path.contains(key) { return Some(lut[key].clone()) } }
}
```

Iterating a Rust `HashMap` is randomised per process, so **a unit that matches two
keys gets whichever preset the hash order happened to put first, freshly on every
`gears` run.** `gears.toml` had a `world_map/` directory key *and* nine per-file
`world_map/main/miscN.` keys, so six of the sixteen world_map units silently lost
`--expand-div` on some builds and kept it on others.

The damage it did in this round: a worker matched `world_map/main/misc13`'s
`func_800935DC` (a div function) in a tree whose `build.ninja` had `--expand-div`,
while the orchestrator tree's `build.ninja` did not. `objdiff` reported `MATCH` +
`100.00%`, `check_bin.sh` in the worker's tree said `OK`, and `make check` then
failed `world_map.bin` by exactly 36 bytes — the missing div-trap sequence. A
`make check` twenty minutes earlier had passed with the same source, which is the
tell that something upstream is nondeterministic.

Fix: **never put a directory key and a per-file key in `gears.toml` for the same
tree.** All sixteen world_map units are now listed by name (7 on the plain
`XenoWorldMap`, 9 on `XenoWorldMapDiv`), and two consecutive `make objdiff-config`
runs produce an identical per-unit preset map. Any future preset experiment should
start by checking `python3` map of unit -> maspsx_flags twice.

### A new preset axis: `battle/main/misc21` is gcc 2.6.0 *and* div-expanded

`func_800A23E8` is 100.00% under `gcc-2.6.0-psx -O2 -G8` and 80.53% under
`XenoBattleDiv` (2.7.2), where its C is 0x48 bytes against retail's 0x4c; the ten
functions already matched in the unit stay at 100.0 under 2.6.0. Three of its stubs
also carry the div trap. New preset `XenoBattle260Div` (gcc 2.6.0 +
`--expand-div`), and `func_800A23E8` is matched under it. This is the same shape as
`battle/misc20` (round 7): **screen the `battle` misc units for 2.6.0 the way
`battling` was screened for `-G0`** — a worker's single `try_flags.sh --gcc
gcc-2.6.0-psx` run is enough.

### Two units lost their INCLUDE_ASM at park time

`battle/misc25` lost `func_800AFD98` (undefined reference at link) and
`battle/misc21` lost `func_800A23E8` (4 bytes short, every function still 100.00%)
because the worker replaced the `INCLUDE_ASM` line with C and then parked the C
inside `/* ... */` without putting the stub back. `lint_sources.py` only catches
the *both* case, not the *neither* case. `tools/rounds/brief.js` now says the
`INCLUDE_ASM` line is part of parking.

Cheap global detector, worth running after any round:
compare each C-defined function's compiled size (`nm -S` on `build/src/*.c.o`) with
the same function's span in `asm/<bin>/{matchings,nonmatchings}/<unit>/<Func>.s`.
It found `func_800A23E8` among 193 objects in one call; everything else that
flagged was my heuristic mis-reading a `.s` that owns data.

### What the div units actually convert at

`world_map/misc5` 3 (of 22), `misc14` 5 (of 16), `misc2` 2 (of 19), `misc7` 1,
`misc13` 1, `battle/misc24` 3 (of 19, the small-function unit), `misc25` 1,
`misc20` 5 (of 19), `misc21` 5, `battling/misc15` 2, `lib_gear` 1. The div-trap
presets are necessary but not sufficient: the residuals are the usual allocator
ties (`func_8007EBBC` 99.83%, `func_8007AD34` 94.72%, `func_80075228` 94.80%,
`func_8007F8AC` 94.77%) and several real blockers found and parked:
- `world_map` `func_80080944` uses a raw absolute `0x1F8000A0` with no relocation.
- `battling/misc15` `func_8008C3A8` needs MAC1/MAC3 read-back macros
  (`mfc2 %0,$25/$26/$27`) that `inline_c.h` does not have.
- `world_map/misc5`'s `D_8009C620` is an array of 0x54-byte rotation blocks from
  offset 0x150 (indices 0-6 at 0x150, 0x1A4, 0x1F8, 0x24C, 0x2A0, 0x2F4, 0x348);
  that one fact converted three functions at once.

### Process notes

- `cp -ac` clones are ~220 MB each on this volume now (the disc directories are
  dropped), so 14 clones cost ~3 GB; disk is not the constraint it was.
- A stray untracked `.ninja_lock` in a clone makes `clone.sh merge` treat it as
  UNCOMMITTED and drop that clone's commits. `rm -f r-*/xenogears-decomp/.ninja_lock`
  before merging.
- `try_flags.sh` reports `compile-failed` intermittently when run in a tight loop
  (docker container churn, not a compiler problem); re-run the row before believing
  it.

| 9 | `battle/{misc21,23,25,24,17,16}`, `world_map/{misc6,1,4,9}`, `menu/{misc7,3}`, `battling/{misc2,5}` | +18 | 1647 | battle/misc21 4 (two of them stale parked notes that matched under the new 2.6.0 preset), misc17 1, misc16 1, world_map/misc6 1, battling/misc5 1, and 10 more from the units whose reports were cut off; all eleven outputs OK, zero reverts |

### Round 9: the "stale parked note" vein is real, the rest is the known tail

Two of `battle/misc21`'s four matches were parked attempts that had been scored
under 2.7.2 and became matches the moment the unit's preset changed to 2.6.0. **A
parked percentage is only meaningful under the preset that produced it** - after any
preset change, re-diff every parked attempt in the unit before writing new C. That
is the cheapest match in the repo and it is why the preset screens pay twice.

Preset screens run this round and their verdicts (all negative, so do not re-run):

- `battle/misc23` under gcc 2.6.0: two matched functions stay 100.0 but
  `func_800A96B4` is 99.1 under both, i.e. its 99.10% is allocation, not toolchain.
- `battle/misc25` under gcc 2.6.0: `func_800B15D8` is 71.38% vs 71.84%, so no.

Blockers re-confirmed or newly found:

- `world_map/misc1`'s `func_80073398` reads `jtbl_8006FB24`; a C `switch` would put
  the table in the unit's `.rodata`, which `linker/world_map.ld` places before
  `.text`, shifting every world_map function even at objdiff 100%. Leave it.
- `battle/misc21`'s `func_800A0838` references `jtbl` - same rule.
- `battle/misc16`'s worker lost an `INCLUDE_ASM` line into a parked comment and
  caught it with `lint_sources.py`; lint does catch that case when the line drifts
  inside a block comment, so run it after every parking edit.
- `world_map/misc6`'s `func_80080578`/`func_8007E450` are pure scheduler
  reordering (retail is in strict source order, cc1 hoists independent global
  loads); `-O1` gets 75%, `-O3` 27%, gcc 2.6.0 27%, `-mcpu=r8000` 32%. Same family
  as `misc6`'s three 88-99.8% parks and `world_map/misc5`'s.
- `world_map` `func_8007DE14` needed `D_8009A660` named separately from
  `D_8009A65C+4` (a reloc naming another symbol can never reach objdiff 100%).
- An 8-byte frame with no locals comes from an unused 8-byte local (`RECT rect;`),
  used twice now.

| 10 | `battle/{misc30,12,13,11,18}`, `menu/{misc9,11,4,12}`, `world_map/{misc7,2,5}`, `battling/{misc3,9}` | +10 | 1637 | battle/misc30 2, battle/misc11 1, menu/misc11 1, menu/misc4 1, plus 5 from the units whose reports were truncated; `menu/main/misc9` is a gcc-2.6.0 unit (new `XenoMenu260` preset); all eleven outputs OK |

### Round 10: the `$at` ceiling is real, and now it is measured instead of guessed

The `battle/misc26..misc35` "no `$at` at all" finding (round 43) was a hypothesis about
compiler flags. A worker spent 15 minutes on ten source shapes in
`func_800B9B30` and settled the mechanism: **gcc never emits `lui` for these at all**
- it emits the one-instruction pseudo `lw $d,D_x` / `sw $x,D_x` - and the difference
is entirely in the assembler:

| form | our assembler (GAS via maspsx) | the ASPSX that built those units |
|---|---|---|
| load | `lui $d,%hi; lw $d,%lo($d)` | `lui $r,%hi; lw $d,%lo($r)`, `$r` free |
| store | `lui $at,%hi; sw $x,%lo($at)` | `lui $r,%hi; sw $x,%lo($r)`, `$r` free |

Proof that it is an assembler revision and not a flag: across every *other* unit's
originals, `%lo(sym)($r)` uses `$at` **1657** times and a real register **once**.

`tools/scripts/atfree.py` (new, and hooked into `preflight.sh`) applies the rule per stub:
a stub is unmatchable if it has a store off an absolute symbol whose base is not `$at`,
or a load whose base is neither `$at` nor the destination. `lui`+`addiu` (`la`-style)
and function calls are fine, which is why the rest of those units is still workable.

Measured over `asm/*/nonmatchings`: **123 of 1637 stubs** are unmatchable this way, and
they are almost all in one place - `battle/main/misc{26,27,29,30,31,32,33,34,35}` (5-17
each). Two useful corrections came out of the measurement:

- `battle/misc28` has only 2 of 19 unmatchable and `battle/misc27` 7 of 22, so the
  round-6 "the whole tail is an 8%-per-worker ceiling, do not brief it" was too broad;
  both are back on the work list.
- `slus_006.64/system/sound` has **0 of 31** unmatchable, so its flatness is allocation,
  not the ceiling.

### `menu/main/misc9` is gcc 2.6.0 (new `XenoMenu260` preset)

`func_801DE29C` is 100.0% under 2.6.0 and 67.27% under Default (2.7.2 sinks the
epilogue's `li v0,1` past `lw ra` and steals the delay slot), `func_801DBDB4` is 94.47%
vs 86.97%, and all three already-matched functions stay 100.0. **The `menu` overlay is
therefore mixed-preset like `battling` and `battle`, unit by unit** - round 11 screens
five more menu units. It does not fix the unit's other blocker: retail lowers a 3-case
`switch (x & 0xFF) {0,1,2}` as a binary-search tree (`beq x,1` / `slti x,2` / `beqz x,0`)
and every gcc here lowers it sequentially, so that family stays parked.

### A new gcc quirk worth knowing: the 8-byte phantom frame

A function containing **two register-indexed loads of the same global** gets an 8-byte
frame (prologue/epilogue only, nothing stored) from gcc 2.7.2, 2.6.0 and 2.7.2-cdk,
unaffected by `-O1..-O3`, `-G0/-G4/-G8`, `-fno-common`, `-fno-peephole`. Two *different*
symbols, or one load plus one store, give `vars = 0`. That is `battle/misc12`'s
`func_80085310`/`func_80084D28` blocker and probably a few others' at 95-96%.

Other blockable facts from this round: `battle/misc11` has **8 of 15** stubs that
reference a `jtbl_` (all blocked); `menu/misc9` has 4 of 21 at-free-unmatchable;
`world_map/misc1`'s `func_80073398` and `battle/misc21`'s `func_800A0838` are
jtbl-blocked. Every worker that hit a `jtbl_` stub correctly left it alone this round.

| 11 | five more menu units (2.6.0 screen), `battle/{misc27,28,13,18}`, `world_map/{misc5,2}`, `battling/{misc9,16}` | +21 | 1616 | menu/misc9 2 (the stale park matched under `XenoMenu260`), menu/misc7 2, menu/misc12 1, menu/misc4 1, battle/misc28 3, misc27 1, plus the units whose reports were truncated; all eleven outputs OK |

### Round 11: the menu overlay is mixed-preset, and cdk is the answer to the "dead" 95-99% class

The `XenoMenu260` switch paid immediately: `menu/main/misc9`'s parked attempt
`func_801DE29C` was already a match and only needed the compiler change, plus one new
function. `menu/main/misc7` is **not** a 2.6.0 unit (`func_801D5A50` breaks from 100.0 to
99.88), so menu is mixed like `battling` and `battle`: screen each unit, do not assume.

**The big one.** A `battle/misc27` worker reproduced `func_800B5DC4` byte-identically
under `gcc-2.7.2-cdk-psx` while the same C is 70.00% under the unit's 2.7.2, and found
the mechanism: **2.7.2 emits `la sym` as a single macro** that maspsx expands to an
adjacent `lui`/`addiu` pair, so reorg can never put the `addiu` in a `jal` delay slot;
**cdk emits the two instructions as separate RTL**, so it can. That is precisely the
"one instruction in the jal delay slot" family this repo has called an unsolvable
allocator tie for three sessions. It was a compiler version, not a register allocation.

The catch, measured here: presets are **per file**, and `battle/misc27` is *function*
mixed under cdk - five of its parked bodies are 100% under cdk and three of its
already-matched functions fall to 93.71 under it (`func_800B5854`). The cdk-only and
2.7.2-only functions interleave in address order, so the `rendering_pause_text` TU-split
trick cannot separate them either. Those five stay parked; the screen is still worth
running on every unit that is *uniformly* cdk, which is what round 12 does for the units
with the most 95-99% parks.

Idioms banked this round:
- `addu` operand order: writing an address as **integer** arithmetic with the offset
  first (`(POLY_FT4*)(i * 0x50 + (s32)g_Menu->unk344)`) emits `addu a2,s0,a2`; every
  pointer/array spelling canonicalises base-first.
- A wide parameter truncated with an explicit `(u_char)` cast at each use emits the
  `sw s0` / `move s0,a0` save pair in the opposite order from a narrow parameter
  (`menu/misc12`'s `func_801E8DA8`, 99.51 -> 100%).
- `u_char result = 1;` gives the byte-mode constant its own constant-pool entry, which
  is how the original gets two `li 1`s (`menu/misc4`'s `func_801CD710`).
- A worker's intermediate commit dropped three stubs by matching an `extern` prototype
  line instead of the definition (`menu/misc4`); the tip repaired it, but
  `lint_sources.py` does not catch a *missing* stub - `make check` does.

| 12 | cdk screens on 11 units + `field/main/{misc,misc4,misc6}` etc. | +3 | 1613 | the round was a **screen**, not a batch: cdk is conclusively wrong for 7 of the 11 units, and `battle/main/misc18` is a gcc-2.6.0 unit (three parked bodies just need the preset) |

### Round 12: cdk is settled, and the 2.6.0 screen is the live axis in `battle`

Eleven units were screened for `gcc-2.7.2-cdk` and **every one is function-mixed**, so
none can take the preset (a unit switches only if *every* already-matched function stays
100.0). Do not re-screen these:

| unit | matched functions that break under cdk |
|---|---|
| `field/main/misc` | 42 of 174 (e.g. `FieldSetScreenDimensions` 100 -> 36.29) |
| `field/main/misc2` | `func_8007234C` 100 -> 82.63 |
| `field/main/misc4` | `func_8007B478` 100 -> 98.3, `func_80078C5C` 100 -> 88.1 |
| `field/main/misc8` | `func_8008083C` 100 -> 98.65 |
| `battle/main/misc16` | `func_8008FA60` 100 -> 84.83 |
| `battle/main/misc18`, `world_map/main/misc6` | several, incl. `func_8008032C` 100 -> 44.41 |

So the cdk mechanism found in round 11 is real but cannot be exploited for any unit
screened so far: the units that need it are always *mixed* with units that do not. It
remains the explanation for `battle/misc27`'s five unreachable parks.

**What did pay: `battle/main/misc18` is gcc 2.6.0** (added to `XenoBattle260`).
`func_80096824` 100.0 under 2.6.0 vs 99.74, `func_80095BAC` 100.0 vs 99.90,
`func_80098C6C` 100.0 vs 95.83, and all six matched functions stay at 100.0. Three parked
bodies in the file become matches on the preset switch alone - round 13 un-parks them.
That is the fourth `battle` unit found to be 2.6.0 (with `misc20`, `misc21`), so the
overlay is being screened unit by unit; `misc23` and `misc25` are confirmed *not* 2.6.0.

Structurally unconvertible, newly confirmed:
- `field/main/misc2`'s `func_8007520C`, `func_800759E4`, `func_800752C8` are BLOCKED
  (`convertible.py`: they own the unit's rodata - `"GEAR      "`, `D_8006FB70`) and that
  rodata is gated behind the two scratchpad-stack-switch stubs.
- `field/main/misc4` has four BLOCKED jtbl owners; the rest of the unit is convertible.
- `field/main`'s `func_80087FA4` (79.83%) needs `la` in a register where gcc folds
  `%hi/%lo` - the mirror image of the `$at` ceiling, and equally a source-shape dead end.

| 13 | 10 `battle` units (2.6.0 screen) + `field/{misc5,misc6}`, `lib_gear` | +7 | 1606 | `battle/misc18` 4 (three un-parked by the preset switch plus `func_80099498`), misc2 1, misc9 1, misc14 1; the other seven `battle` units are confirmed **not** 2.6.0 |

### Round 13: seven more battle units are not 2.6.0, and the preset switch paid 4

`battle/main/misc18` under `XenoBattle260`: three parked bodies became matches on the
switch alone, and `func_80099498` closed from 95.15% with two idioms worth keeping:
- a 3-case switch whose lowest case body is empty (`case 1: break;`) is what makes gcc
  2.6.0 emit the signed low-bound check (`slti v0,v1,3; bnez`); two cases give a plain
  `beq/beq/j` chain. The index also wants a *signed* local plus `default: break;`.
- `addu v1,a2,v1` (base first) needs the index multiply in its own statement
  (`s32 off = i * 368;` then `(s32)base + off + 0x7C`); every pointer expression makes
  gcc canonicalise the shift result to op0, in every preset.

Screened and **negative** for gcc 2.6.0 (do not re-screen): `battle/main/misc1`
(`func_80071A08` 100 -> 99.17), `misc2`, `misc3`, `misc7` (all 11 matched stay 100 but no
parked body improves), `misc9` (a full `try_flags --sweep`: 2.6.0 99.0, cdk 96.5, `-O1`
51.9), `misc14` (compatible but nothing needs it), `misc15`, `misc16`.

Additional reusable idiom (paid for `battle/misc2`'s `func_80074F70`): split the address
arithmetic into named locals — `p = D_x; off = p[0x7F4] * 40 + 0x7A4; AddPrim(ot,
(u8*)(off + (s32)p))` — so gcc stops reassociating the constant into the base pointer.
The `743A4/744BC/74554` family in that unit is one allocator decision away now, not three.

| 14 | seven `battling` units (-G0/2.6.0 screen) + five `menu` units + two `world_map` units | +9 | 1597 | battling/misc1 2, misc4 1, misc10 1, misc11 1, menu/misc2 1, plus 3 from the units whose reports were truncated; `menu/main/misc5` is a gcc-2.6.0 unit (new preset, and its `func_801D1258` park is an uncomment away) |

### Round 14: the screens keep paying one unit at a time

`menu/main/misc5` is the fifth 2.6.0 unit found: **all nineteen** matched functions stay at
100.0 under 2.6.0 and `func_801D1258` is 100.00% only there (99.35 under Default - the sole
difference is a canonicalised commutative `addu`). Its parked body un-parks with no C change.
So the preset landscape is now: `menu/misc9`, `menu/misc5`, `battle/misc18`, `misc20`,
`misc21`, `battling/misc12` (which is `-G0`), plus the div-expansion family — everything else
screened is plain Default. `battling/misc1`, `misc4`, `misc6`, `misc8`, `misc10`, `misc11`
are all confirmed **not** `-G0` and not 2.6.0, so that overlay is fully classified too.

A worker did the right thing when it could not commit: `menu/misc5`'s worker found the preset
answer, could not safely convert anything under the *current* preset, and left the tree
link-safe with the evidence in the parked notes. That is the pattern to want - a parked body
plus a preset claim is worth more than a live 99.35% body.

Idioms banked this round:
- `addu` operand order again, both directions: declaring the base field `u32` instead of a
  pointer, or writing the address as integer arithmetic with the offset first, is what flips
  `addu v0,s2,v0` vs `addu v0,v0,s2`. This single idiom has now paid in five units.
- A `switch` is required for `switch (D_80059460) { case 2: case 0: case 6: }` - gcc's
  decision tree for those three values reproduces retail exactly (`menu/misc2`).
- Six halfword reads must be hoisted into `s32` locals, never `s16` (`s16` gives `lhu` +
  `sll`/`sra`) - `battling/misc1`'s `func_800725B0`.

| 15 | `menu/{misc5,8,10,6}`, `battling/{misc6,8,17,2}`, `world_map/{misc11,12,15}`, `battle/{misc4,5}`, `field/misc9` | +14 | 1583 | menu/misc5 4 (the preset un-park plus three more), battling/misc2 3, battling/misc17 2, plus 5 from truncated reports; `menu/misc6` and `menu/misc10` are also gcc-2.6.0 units (added to the preset) |

### Round 15: the menu overlay is mostly gcc 2.6.0, and a preset un-park keeps paying

`menu/main/misc5` gave **4** matches: the free un-park, then three functions that needed a
*named* struct field rather than a cast (`MenuString unk18E0[6]` at Menu+0x18E0,
`unk1BE0[4]` at +0x1BE0, and a `MenuCharacterSpriteRecords` type for the `HeapAlloc(0xA1C)`
block). Nothing was re-split: one opaque array became three named ones covering the same
span, which is the safe form of that edit.

`menu/main/misc6` and `menu/main/misc10` are **also 2.6.0** (both added to `XenoMenu260`),
so five of the twelve `menu` units checked so far are 2.6.0 and six are not - the overlay is
genuinely mixed and every unit needs its own screen. `menu/misc6` has three parked bodies
that are straight uncomments once the preset is on.

Idioms banked this round:
- A loop that the original runs with *two* induction variables (index and byte offset) wants
  two C loop variables (`for (i = 0, offset2 = 0x400; i < 8; i++, offset2 += 0x80)`) - that
  is what reproduces `addu a2,s2,v0` plus `addiu s2,s2,0x80` in the delay slot.
- A flag read-modify-write split into three statements (`flags = lw; flags &= ~2;
  flags |= arg2 << 1;`) moved a whole parameter from `$8` to `$3` and took `battling/misc2`'s
  `func_80073CEC` from 0.00% to 100.00%.
- Re-dereferencing the *full* chain at each use (`*(s16*)(*(u8**)(*(u8**)(p + 0x5C) + 4))`)
  beats a cached pointer local when the original reloads through it every iteration.
- `battling/misc2` has three `/* Handwritten function */` stubs (bare `mtc2`/`mfc2`) that no C
  can produce - same class as `psyq/libgte`'s.

| 16 | `menu/{misc6,10,5,8}`, `battling/{misc17,2,6,8}`, `world_map/{misc11,12,15}`, `battle/{misc4,5}`, `field/misc9` | +12 | 1571 | menu/misc6 3 (the 2.6.0 un-parks), menu/misc5 2, menu/misc10 1, battling/misc17 3, battling/misc2 1, plus 2 from truncated reports |

### Round 16: the induction-base idiom, worth three at once

`battling/main/misc17`'s three loop stubs (99.39/99.54/99.56%) all closed with one change:
write the loop **indexed** (`pSlot[i].field` plus an `s32 i`) instead of with an incrementing
pointer (`p->field; p++`). With `p++` gcc's `combine_givs` re-bases the induction register onto
the address of the body's *last* memory reference (base `&unk2`, offsets 10/-1/2/0); with
`p[i]` the biv is `p + i*stride`, the base register stays the symbol, and every displacement is
the literal struct offset - retail's shape. This is now in the round brief.

Other banked facts:
- Two `0xFF` immediates in one function are CSE'd unless they differ in **mode**: an
  `s32 v = 0xFF;` arm defeats the CSE where a `u8`/literal arm does not (`battling/misc6`).
- `menu/main/misc5` finished its small-function work with *disjoint* offset locals
  (`offset`/`offset2`/`offset3`, each with its own live range) - one local reused for several
  running offsets never matches.
- A worker found `func_80073F34` (battling/misc2) is 97.87% but 428 bytes against retail's
  432, so it is **not** linkable and must stay `INCLUDE_ASM` - the size check is why the
  check_bin gate exists.

| 17 | `battle/{misc8,16,9,12,19,1,3}`, `menu/{misc6,9,10,11}`, `field/{misc3,misc4}`, `world_map/misc14` | +13 | 1558 | battle/misc12 1, misc19 1, misc3 1, menu/misc6 1, plus 9 from truncated reports; the indexed-loop idiom paid again (`battle/misc3`'s `func_80077364`: `POLY_FT4 *p` + `p[i]` **and** a function-scope `s32 offset` assigned at first use inside the loop) |

### Round 17: one idiom, and one hazard that cost a worker its whole gate run

The indexed-loop idiom from round 16 generalised: `battle/misc3`'s `func_80077364` needed
`POLY_FT4 *p` + `p[i]` **and** a function-scope `s32 offset;` assigned at its first use
*inside* the loop body (not declaration-initialised, which puts the multiply before the
hoisted `li s3,0xFF`, and not inlined, which flips the `addu` operands). Both halves are
load-bearing; `p->field; p++` re-bases the induction register and `99.67%` is the best you
get without both.

**Hazard, now recorded:** an *unclosed* `/*` inside a parked attempt silently hides every
following `INCLUDE_ASM` line from splat, because splat strips comments before looking for
them. Those functions then land in the *matchings* directory and the unit silently loses
their bytes - `battle/misc8`'s worker lost a whole gate run to it. `lint_sources.py` does
not catch this; the symptom is `check_bin.sh` failing with sizes that do not add up.

**Preset lead, not yet usable:** `battle/main/misc1` would be a 2.6.0 unit
(`func_800718BC` 100.0 vs 94.76, `func_800716D8` 100.0 vs 78.82) **except** that one
already-matched function, `func_80071A38`, drops to 99.05 under 2.6.0. If that one function
can be re-matched under 2.6.0 the unit flips and two parks become free matches - it is a
single-register/delay-slot miss, which is the most tractable kind.

| 18 | `battle/{misc1,19,9,16,3}`, `menu/{misc9,10,11,8}`, `field/misc4`, `world_map/{misc14,11}`, `battling/{misc6,8}` | +11 | 1549 | `battle/misc1` flipped to gcc-2.6.0 (both blockers re-matched, then its two parked bodies un-parked in integration = 4), `battle/misc16` 1, `menu/misc10` 1, plus the rest from truncated reports |

### Round 18: `battle/misc1` flipped, and the addu mirror rule that made it possible

The unit was one function away from flipping to 2.6.0 - except it was *two*. Both blockers
(`func_80071A38` 99.05, `func_80071A08` 99.17) closed with the same one-line change: 2.6.0
canonicalises `p = base + i; p[k]` to **index-first** `addu v1,a0,v1`, while retail is
**base-first**. Writing `pDst = D_x; *(pDst + i + 0x7C)` (or `(&D_x[i])[k]`) yields
base-first under both compilers; `p[i + k]` never does. That is the mirror image of the
existing "`(i + p->array)` emits index-first" idiom and it only bites when the preset
changes - worth remembering before re-screening any unit.

With the preset flipped, the unit's two parked bodies (`func_800716D8` 78.82 -> 100.0,
`func_800718BC` 94.76 -> 100.0) were un-parked in the integration step: **four functions
for no C writing at all**, and every already-matched function in the unit stays 100.0.

Also learned, and it matters for `menu/misc10`: **five of that unit's twelve stubs are
jtbl-blocked** (`func_801E23CC`, `E2BE4`, `E3088`, `E36D4`, `E3ECC` - the tables'
entries are `.L` labels inside their own `.s`), and `convertible.py` reports all twelve as
OK, so it does not catch this class.

| 19 | `battle/{misc1,16,19}`, `menu/{misc12,8,11,6,7}`, `world_map/{misc11,12,15}`, `battling/{misc6,8}`, `field/misc4` | +5 | 1544 | menu/misc12 1 (`func_801E8978`, its 99.91% wall was the `+0x8C0` offset folding onto the base pointer), menu/misc11 1 (`func_801E5924`, the in-loop offset-giv idiom), plus 3 from truncated reports. Both blockers of the round are allocator walls now documented per unit |

### Round 19: thin, and the tail is now mostly named walls

Yield dropped to 5. Every report this round is the same shape: the function is fully decoded,
the instruction multiset is right, and one named compiler behaviour differs. The four that
keep recurring, all now documented with per-unit instances:

1. **A scalar global's *address* cannot be held in a saved register.** Retail's
   `lui sX,%hi; addiu sX,sX,%lo; ... lw vY,0(sX)` is a materialised `la`; our gcc folds the
   address into each load (`lui/lw %lo(sym)`), so CSE can never hoist it, and pointer-local
   spellings are folded back. Unit-wide in `battle/misc16` (`D_800CCB34`, four functions) and
   `battle/misc19` (`D_800CCE42`/`D_800CCD68`).
2. **`loop.c` hoisting a `li` out of the loop** costs a callee-saved register and changes the
   frame (`battle/misc1`'s `func_80072270`, 0x28 vs 0x20).
3. **`combine_givs` picking the body's last memory reference as the base** - the indexed-loop
   idiom fixes the pointer form, but not a 368-stride giv (`battle/misc8`'s `func_8007D5B0`).
4. **Commuted `addu`/`mfhi` register naming** with identical instruction multisets.

Two useful new idioms did come out of it:
- Keep a >16-bit constant offset in its own `u32 off` local when the original does not fold it
  onto the base pointer (`menu/misc12`'s `func_801E8978`: `numTexts * 0x50 + 0x8C0`).
- A signed product cast to `u32` before dividing gives `mult` + `multu`/`srl` where a `u32`
  product gives `multu` (`battle/misc19`'s `func_8009BD94`) - objdiff's fuzzy score *prefers*
  the wrong spelling (97.33 vs 83.67), so trust the instruction shapes here, not the number.

Process note worth keeping: a worker rebuilt a park block by string-slicing and silently
deleted a matched body in the same file; only `check_bin.sh` caught it (undefined reference at
link). Diff against the previous commit after park/unpark surgery.

| 20 | `menu/{misc7,misc8,misc6,misc9}`, `battle/{misc19,misc16,misc5}`, `world_map/{misc11,misc12,misc1,misc15}`, `battling/{misc6,misc8}`, `field/misc4` | +2 | 1542 | both from `menu/misc9`: `func_801DF890` (182 insns) and `func_801DFB68` (177), the two largest untouched stubs in the unit. Every other unit returned only park notes |

### Round 20: the grind has hit the wall, and here is the measured reason

13 of 14 units returned park notes and nothing else, and the notes all name one of four
behaviours with no source-level lever left:

- **`cse` replacing a single-use sum pseudo at the argument position** - statement order and
  local declarations provably do not survive it (`menu/misc8`'s `func_801DB0A8`, 98.80%).
- **RTL block rotation**: retail's loop-2 `i++` sits at the *top* of the body block
  (`addiu s1,s1,1` in the `beq` delay slot with no latch copy); no `for`/`while`/`continue`
  shape reaches that layout (`menu/misc7`'s `func_801D51EC`/`func_801D53D0`, both 98.31%).
- **gcc cannot hold a scalar global's address in a saved register**, so where retail re-forms
  `lui at,%hi; addu at,at,idx; lbu %lo(at)` per use, ours hoists the base - and the two are
  only equivalent when the two indices are *different* values, so CSE credits the shared one
  (`battle/misc19`, `battle/misc16`).
- **`combine_givs`/`global.c` tie-breaks** with an identical instruction multiset.

What the round did produce is coverage: `menu/misc11` now knows **7 of its 13 stubs are
jtbl-blocked**, `battle/misc5` proved its `func_8007ABD8` body is 100.0 under cdk but the unit
is mixed, `field/misc4` has 4 BLOCKED jtbl owners, and `menu/misc9`'s two matches came with a
reusable idiom (`((u8*)base + i)[K]` puts the constant offset last and matches retail's
`addu` order; two separate `if` statements are what emit `bltz` + `slti` where a compound
range test collapses to one `sltu`).

**Open decision for the operator** (recorded here so it is not lost): the cdk/2.6.0-**mixed**
units are unreachable only because presets are per *file*. Splitting a mixed unit into one TU
per function (the `rendering_pause_text.c` precedent generalised: 3-way yaml range split, a
renamed tail file, one preset entry) would reach `battle/misc27`'s five cdk-only parks,
`battle/misc5`'s `func_8007ABD8` and the rest of that class - perhaps 10-20 functions. It
restructures the tree (many small files) and nobody has authorised it.

# Session 7

Start: **3026 / 4568 functions (66.2%)**, 1542 `INCLUDE_ASM` stubs, eleven outputs OK.

The per-function TU split is **authorised** this session (operator: generalise it), and
it immediately paid, because the screen below shows that most "mixed" units are not
mixed at all - they are cdk units whose *compiler* was never checked.

## The recipe: screen a unit, then either switch it or split it

Presets are keyed per file, so a unit carries exactly one compiler. Two cheap tools
decide between "just switch the unit" and "split it":

    tools/scripts/try_flags.sh <unit> --all                      # every C function, unit preset
    tools/scripts/try_flags.sh <unit> --all --gcc gcc-2.7.2-cdk-psx   # ... under the variant
    tools/scripts/screen_unit.sh <unit> --gcc gcc-2.7.2-cdk-psx       # per-function, slower

- Every C function 100.0 under the variant -> **switch the unit's preset**. No new
  files. Then un-park every body the switch made exact (that is where the matches are).
- Some C functions below 100.0 under the variant -> **split**: `config/<bin>.yaml`
  gets one `c` range per run of same-preset functions, the runs become
  `main/miscN` + `main/miscN_psx` + `main/miscN_b` files, and `gears.toml` gets a
  per-file path entry per run (never a directory key plus a per-file key for the same
  tree - see round 8).
- Only *C-defined* functions constrain the split. `INCLUDE_ASM` stubs are retail asm
  and ignore the preset, so a stub goes in whichever run is adjacent; put the stubs
  with a known parked cdk body on the cdk side.

Mechanics that cost time and are now known:
- `tools/scripts/try_flags.sh` used to read `maspsx_flags` off the *cc* edge, where the
  preset does not put it, and so silently fell back to Default's flags: every
  `--dont-expand-li` unit was under-scored under cdk (misc34's `func_800BF7C8` read
  98.38 instead of 100.0). Fixed; it now reads the *maspsx* edge.
- `try_flags.sh --all` scores every C symbol of the unit in one compile.
- Splat puts the stubs of a split file in their own `asm/<bin>/nonmatchings/<run>/`
  directory, so the new file's `INCLUDE_ASM` paths must name the *run*, not the unit.
- Each split file needs its own preamble (includes, `extern`s, and any typedef that
  was declared mid-file in the original - a `BattlePoint` typedef declared halfway down
  cost one build).
- Duplicate parked bodies are common (the same `/* body */` twice, once before and once
  after the stub). Un-park by name; keep one copy.
- After un-parking, `make check`: a body whose C is the wrong *size* still reads 100.00%
  in objdiff.

## Log

| # | Units | Functions | Stubs left | Notes |
|---|---|---|---|---|
| 1 | `battle/{misc6,7,8,9,28,33,34}`, `battling/{misc7,14,16}`, `world_map/{misc7,9}`, `system/sound` | +21 | 1516 | all eleven OK. Per unit: battling/misc7 7, battle/misc34 4, battling/misc14 3, world_map/misc7 3, battle/misc28 2, battle/misc8 1 (`func_8007D7B4`), battle/misc33 1 (`func_800BE11C`); 0 in battle/misc6, battle/misc7, battle/misc9, battling/misc16, world_map/misc9, system/sound. Ten of the thirteen units turned out to have accurate parked notes from earlier rounds, so the "fresh stub" counts in `select_target.py` are not to be trusted (`battle/misc6` reported 13 fresh stubs and had 13 parked bodies). |
| 2 | `battle/misc27` split (cdk) | +5 | 1511 | `func_800B5854` is 100.0 under 2.7.2 and 93.71 under cdk; the rest of the unit is cdk-only. 3 files: `main/misc27` + `main/misc27_b` on the new `XenoBattleTailCdk` preset, `main/misc27_psx` on `XenoBattleTail`. Un-parked 56E4/5924/5DC4/5FBC/61B0. `func_800B4EDC` is 84.19 under *both* compilers and `func_800B5CC0` 98.15 under both - the split proves those two are not preset questions. |
| 3 | `battle/misc28`, `battle/misc34` (uniformly cdk -> preset switch, no split) | +8 | 1503 | `screen_unit.sh` says every C function in both units is 100.0 under cdk (11 and 8 of them), so they move to `XenoBattleTailCdk`. Un-parked misc28's 63F0/6438/6808/6A7C/6E84/73A0 and misc34's BF7C8/BF954. misc34's `func_800BF8CC` is 92.21 under cdk too (register swap, not a preset), BF5E8/BF730 are atfree-blocked, BF998 is a frame/allocator tie. |

### Round 1's finding: `select_target.py --json`'s `attempt` flag is now useless

Ten of thirteen units briefed as "fresh stubs, never attempted" already had parked bodies
under every one of them, and the round landed 21. The flag only looks for `/*` directly
below the `INCLUDE_ASM`; the repo's parks come in four shapes (block below, block above
with `//` notes between, `}*/` on the body's closing brace, and duplicated blocks). Grep
`^/\*` before briefing, or just accept that every remaining unit is screened.

### The cdk switch is worth much more than one function per unit

`battle/misc28` moved six functions with **no C written** - the bodies were already
parked from round 21 and only the compiler was wrong. Same for misc34 (+2). The
ordering rule that makes this cheap: **screen first, write nothing, un-park second**.

# Session 8: the preset sweep (park-level and unit-level), +55 with almost no C

Start: **1517 `INCLUDE_ASM` stubs** (3051 / 4568 functions), eleven outputs OK.
End: **1462 stubs** (3060 / 4568 = 67.0% functions), eleven outputs OK, public page
republished.

The session was one idea applied twice: the existing per-unit preset screens
(`try_flags.sh --unit --all`) only score *C-defined* functions, so every parked body
- which is exactly where the free matches live - is invisible to them. Un-parking the
bodies first makes the screen see them.

## The two sweeps

`/tmp/park_sweep.py` (not checked in; 40 lines, uses `tools/scripts/park.py` as a
library): for every unit with parked bodies, un-park *all* of them, run
`try_flags.sh <unit> --all` under the unit's own flags and under each candidate, score
every body, `git checkout` the file. 139 units in ~20 minutes because `try_flags` is
~1 s per compile. A second sweep (`/tmp/unit_screen.py`) scores every C function of
each unit under the same candidates, which says **which already-matched functions
break** under a candidate - the other half of the switch/split decision.

Join the two: `breaks == 0 && harvest > 0` means the unit takes the compiler wholesale
and its parked bodies un-park for free. `breaks > 0` means a TU split, and the address
order decides how many runs (use `split_unit.py --dry-run`).

Results, all gated by `make check`:

| # | What | Functions | Stubs left |
|---|---|---|---|
| 23 | 4 whole-unit preset switches found by the park sweep (battle/misc15, misc23, misc25, menu/misc12) | +7 | 1510 |
| 23 | 14 workers on fresh-stub units | +9 | 1501 |
| 24 | 9 more whole-unit switches (battle/misc6, misc8, misc17, misc22, misc24, misc33, misc28, misc34, field/misc2, battling/misc9) | +19 | 1482 |
| 24 | menu/misc11 split (2 bodies on 2.6.0), battle/misc1 stale un-park | +3 | 1479 |
| 24 | 14 workers (one unit each) | +13 | 1462 |
| 25 | `battle/misc16` split (unit -> 2.6.0, func_8008FA60 into a new plain-2.7.2 TU) | +1 | 1461 |
| 25 | 13 workers on the next tier of units | +6 | 1455 |

Whole-unit switches, with the new presets they needed: `XenoBattle260` gains
battle/{misc1 (round 18), misc6, misc8, misc15, misc17, misc18, misc20} and
`XenoBattle260Div` gains battle/{misc21, misc22, misc23, misc24, misc25};
`XenoBattleTailCdk` gains battle/misc33; **new** `XenoBattleTailCdk0` (cdk +
`--dont-expand-li` + `-G0`) takes battle/{misc28, misc34}, `XenoFieldNoGp` (`-G0`)
takes field/main/misc2, and `XenoBattlingNoGp` gains battling/misc9. `menu/misc12`
and `menu/misc11_r1` join `XenoMenu260`.

## What is exhausted (do not re-run)

- **maspsx axes**: un-parking all bodies and scoring them with `--expand-div`,
  without `--dont-expand-li`, with `-G4`, and with `-O1` (in that order) yields **zero**
  additional 100.0 bodies across all 139 units. The div/no-li landscape really is
  fully classified.
- **Combined variants**: `gcc-2.6.0 + -G0` and `gcc-2.7.2-cdk + -G0` break exactly the
  same functions as `2.6.0` and `cdk` alone, so they never rescue a split candidate.
- **`-G0` in `world_map`**: `func_8007F8AC` (misc6) is 100.00% in objdiff under `-G0`
  but its C links 45 bytes the wrong length - the overlay's globals are absolute
  symbols from `undefined_syms_auto`, and `-G0` changes how they are addressed. Do not
  give `world_map` a `-G0` preset.

## Remaining split candidates (measured, not guesses)

`harvest` = parked bodies that are 100.0 under the candidate; `breaks` = already-matched
functions in the unit that fall below 100.0 there. Files the split would cost:

| unit | preset | harvest | breaks | runs |
|---|---|---|---|---|
| battle/misc16 | 2.6.0 | func_8008CDE4 | 1 (func_8008FA60 99.33) | 2 |
| battle/misc19 | 2.6.0 | func_8009A9D0, func_8009ADA0 | 3 (99.57/95.71/99.77) | 5 |
| menu/misc8 | cdk | func_801D9C84 | 6 | 3 |
| field/main/misc | 2.6.0 / `-G0` | func_8008848C / func_80087FA4 | 61 / 2 | many |
| battle/misc6 | cdk | func_8007AD6C/ADB0/ADF4 | 1 | 2 |

battle/misc6 is now a 2.6.0 unit (func_8007AEF0, +1 taken); the three cdk bodies there
need a second TU, which is why they are still parked.

## Hazard: two workers deleted an adjacent matched body while re-parking

`battling/misc17` lost func_8008E620/E67C/E6F8 *and* stub func_8008E3CC, and
`battle/misc1` lost func_800718BC, when the worker replaced a park block by
string-slicing and its slice ran past the end of the block into the next function.
`lint_sources.py` does not catch it, objdiff cannot see a function that is simply
absent, and the only symptom is `undefined reference` at link. Both were restored from
the pre-round commit.

**The gate that does catch it** - run it after every merge:

```sh
python3 - <<'PY'
import re,subprocess
DEF=re.compile(r'^[A-Za-z_][\w \t*]*?\b(\w+)\s*\([^;]*\)\s*\{',re.M)
for f in subprocess.run(["git","diff","--name-only","<pre-round>","HEAD","--","src/"],
                        capture_output=True,text=True).stdout.split():
    old=subprocess.run(["git","show","<pre-round>:"+f],capture_output=True,text=True).stdout
    new=open(f).read()
    missing=sorted(set(DEF.findall(old))-set(DEF.findall(new)))
    missing=[m for m in missing if not re.search(r'/\*[\s\S]*?\b'+m+r'\s*\([^;]*\)\s*\{[\s\S]*?\*/',new)]
    if missing: print(f,"MISSING:",missing)
PY
```

(a definition still present inside a park block is fine; that is the filter above).

## Other things this session settled

- `tools/scripts/split_unit.py` computed a new file's asm directory as
  `asm/<bin>/nonmatchings/<last component>`, dropping `main/`, so every `INCLUDE_ASM`
  in a split file pointed at a directory splat never writes. maspsx then failed with
  `can't open ...` **hidden by the maspsx rule's `2>/dev/null`**, and the only symptom
  was `cannot find build/src/....c.o` at link time. Fixed.
- A parked percentage is only valid under the preset that produced it: `menu/misc12`'s
  `func_801E7E68` was matched at objdiff 100.00% under 2.7.2, and after the unit moved
  to 2.6.0 the same C links 12 bytes wrong. Re-parked. (Fourth time this pattern has
  bitten; see round 9.)
- gcc 2.6.0 emits jump tables and balanced binary-search switch trees for dense cases
  (a `menu/misc9` worker corrected an old park note that claimed no preset could).
- Workers burn 10-15 minutes of a 22-minute writing budget per function on these units
  now: 14 workers produced 13 matches in round 24 and 9 in round 23, i.e. ~0.8 per
  worker, against ~1.2 for the two preset rounds that wrote no C at all.

## Round 25 (13 workers) and the second form of the park-surgery hazard

Yield 6 from 13 workers, and 9 of them committed nothing at all: the units left in the
pool (battle/misc3, misc4, misc30, world_map/misc8/10/16, field/misc2,
system/archive, menu/misc11_r2) are the "every remaining stub already carries a 95-98%
park" class. Conversion per worker is now ~0.5, against 1.2-1.5 per *preset* round
that writes no C at all.

The park-surgery hazard now has **two** forms and `lint_sources.py` catches neither:

1. a parked body's `/* ... */` block is rebuilt by slicing and the slice runs past its
   end, swallowing the *next function's definition* (battling/misc17 lost three
   matched functions, battle/misc1 lost one);
2. the same slice swallows the next function's `INCLUDE_ASM` *line*, leaving a stub
   that no longer exists (battle/misc21 lost `func_800A1CF4`, which battle/misc24
   calls - the only symptom is `undefined reference` at link).

Both were restored from the pre-round commit. The definition-set check above catches
form 1; form 2 needs the stub set as well:

```sh
diff <(git show <pre-round>:src/X.c | grep -o 'INCLUDE_ASM([^;]*' | sort) \
     <(grep -o 'INCLUDE_ASM([^;]*' src/X.c | sort)
```

Anything that disappears from that set and is not a function you deliberately converted
is a lost stub.

# Session 9: the cdk split generalises to single functions, +12

Start: **3067 / 4568 functions (67.14%)**, 1455 `INCLUDE_ASM` stubs, eleven outputs OK.
Now: **3079 / 4568 (67.40%)**, **1443 stubs**, eleven outputs OK. Fuzzy 35.30 -> 35.47.

## The split that works and the split that does not

Three units were split onto the new **`XenoCdk`** preset (plain gcc-2.7.2-cdk, Default
maspsx) or onto an existing one, and the parked bodies un-parked for **+5 with no C
written**:

| unit | split | why |
|---|---|---|
| `battle/misc6` | 3 files (`misc6`, `misc6_r1` cdk, `misc6_r2`) | the AD6C/ADB0/ADF4 run is 100.0 under cdk, 97.x on the unit's 2.6.0 |
| `battle/misc5` | 2 files (`misc5`, `misc5_r1` cdk) | `func_8007ABD8` is the unit's last function, so the cheapest split in the repo |
| `menu/misc8` | 3 files (`misc8`, `misc8_r1` cdk, `misc8_r2`) | `func_801D9C84` is 100.0 under cdk, 92.55 on Default (loop rotation) |

**The recipe's hard limit, found the expensive way: a unit that has `.rodata`,
`.data` or `.bss` subsegments in its `config/<bin>.yaml` cannot be split this way.**
`linker/<bin>.ld` places `build/src/<unit>.c.o(.rodata)` (and `.data`, `.bss`) by the
*old* file name, so the split objects' data is unplaced and the overlay shifts even
though every function still reads objdiff 100.00%. `field/main/misc` and
`system/sound` are both in that class; both splits were built, failed `make check`
by 301 bytes, and were reverted. To split those the yaml's data subsegments have to
be reassigned to the new file (and the new file must be the one that owns them),
which is a different job from the text split.

`tools/scripts/split_unit.py` still cuts park blocks: it computes chunk boundaries
from `DEF_HEAD`/`INCLUDE_ASM` matches, and a park block's `/*` sits *before* its
`DEF_HEAD`, so the previous chunk keeps the `/*` and the new file starts with a
dangling body; it also copies the head up to the first mark, which for a file that
opens with stubs or a prototype list is *not* the file's preamble (three files got
a duplicate `INCLUDE_ASM`). The working script (/tmp, 130 lines, worth checking in)
takes the earliest of {INCLUDE_ASM line, park block start (matched only when the
block really contains that function's definition), definition} as each function's
chunk start, and builds the new files' preamble as head + `#define`s + the typedefs
and `extern` lines from the rest of the file, dropping a declaration the body or the
head already carries. Two more traps it had to learn: a mid-file `typedef` used by an
`extern` in the preamble must come *before* the extern, and a declaration that only
appears inside a park block (a comment) does not count as present.

## Round 26 (14 workers, +7)

| unit | result |
|---|---|
| `psyq/libgpu_c` | +1 `func_80047638` (TMD object-table patcher); `SetFogNearFar` parked at 98.66 with the shift-splitting idiom |
| `battling/misc11` | +1 `func_8008820C` (worker timed out before committing; gated here) |
| `battling/misc14` | +1 `func_8008A63C` |
| `battling/misc18` | +1 `func_8008F5B4` |
| `world_map/misc8` | +1 `func_8008355C` |
| `menu/misc7` | +1 `func_801D5794` (uncommitted by a timed-out worker; gated here) |
| `menu/misc10` | +1 `func_801E1014` (same) |
| `psyq/libgte_b` | 0, and the negative is now proven: **16 of its 18 stubs are unlinkable** (1-3 filler `nop`s after `.size`) |
| `battle/{misc9,misc3,misc12}`, `battling/{misc14}`, `world_map/misc10`, `system/temp1`, `field/misc6` | park notes only |

`psyq/libgte_b`'s two "convertible" stubs are `TransposeMatrix` (92.81, allocator) and
`RotAverageNclip4` (59.52, handwritten asm), so that unit is finished: do not brief it
again. Same shape as `libgte`'s earlier finding - in `psyq/`, `convertible.py` and the
trailing-`nop` check are the whole story.

## Park surgery, third form

`battle/misc12`'s worker closed a parked body with `}` where the block needed `*/`, so
the comment ran from `func_80084750`'s park to `func_80084D28`'s and swallowed
`func_80084854`'s `INCLUDE_ASM` line: the only symptom was `undefined reference to
func_80084854` at link, with both `lint_sources.py` and the definition/stub-set diff
from session 8 reporting clean (the stub line was still *in the file*, it was just
inside a comment). The definition-set check does catch it when the swallowed function
is C-defined; for a swallowed *stub* neither check fires, so **the gate for a round
that touches park blocks is `make check`, not the set diffs**.

## Round 27 (14 workers, +2) and two preset wins it produced

Round 27 briefed five psyq units and nine overlay units. Only `psyq/libsn` converted
(`PCread`, `PCwrite`; the idiom is a single `register char *pBuff asm("$19");` pin plus a
plain `nLeft = len;` copy, which is the minimum that reproduces retail's save block).
The seventeen timer expiries and the twelve parks are worth less than what the round
proved:

- **`psyq/libapi`, `psyq/libcard`, `psyq/libcard_b` are entirely hand-written BIOS gates**
  (`addiu t2,0xB0; jr t2; addiu t1,<index>` with pad `nop`s after `.size`, and `$ra`
  saved into a global). Zero convertible stubs in each. Do not brief those three again.
  Together with `libgte_b` (16 of 18 unlinkable, trailing nops) and `libcard`'s sibling
  finding, the whole `psyq` card/pad/SN corner of the binary is now classified.
- `world_map/main/misc15` has a **third** jtbl owner, `func_80096A6C`, besides
  `func_800968E0`/`func_80097800`.

Then two preset decisions taken from worker reports, both gated by `make check`:

| unit | change | value |
|---|---|---|
| `menu/main/misc2` | joins `XenoMenu260` (gcc 2.6.0) | all 19 matched functions stay 100.00; `func_801C7BF4` (96 insns) is 100.00 only there -> converted, +1 |
| `battle/misc23` | new `XenoBattle260Div0` (2.6.0 + `--expand-div` + **-G0**) | all 11 C functions stay 100.00 and its park `func_800A6F98` goes 84.20 -> 96.96, handed to round 28 |

`world_map` also gained the `D_8009BB48` label (inside its data segment, missing from
`undefined_syms_auto`), which is what kept `misc15`'s `func_800979C8` park from ever
linking - naming a label that lives inside another unit's data blob is enough, no
`config/*.yaml` change needed.

Two process notes: a timed-out worker's C body is **not** lost if it was written in an
edit tool call - `worker_transcript.jsonl` under `subagent-artifacts/` still has it (that
is how `menu/misc2`'s body was recovered after `clone.sh merge` discarded the uncommitted
file). And `try_flags.sh --all` under load returns `compile-failed` or an empty score
list for roughly half the units while a 14-clone round is building; always re-run a
surprising row before believing it.

## Round 28 (8 workers, +5): the two preset switches paid exactly as predicted

| unit | result |
|---|---|
| `menu/main/misc2` (new `XenoMenu260`) | **+3**: `func_801C7F34` (the stale note's divisor chain ends at **60**, not 120), `func_801C80B8` (store the quotient directly instead of through a `digit` temp), `func_801C7D78` (input parsing; `input = MENU_INPUT_IDLE;` must sit after the unplugged loop and the shared tail over inputs 0-3 + L1/R1 is what cross-jumps six case blocks into one) |
| `battle/main/misc23` (new `XenoBattle260Div0`) | **+1** `func_800A6F98` — the park reused `p` for the `&D_800D3628` value, so gcc shared one allocno and gave the loop pointer a callee-saved register; `u8 *pRect` splits it. `func_800A9540` went 77.73 -> 90.14 for free on the -G0 preset |
| `world_map/main/misc15` | **+1** `func_80097070` (matrix to scratchpad); `func_800979C8` 92.13 -> 96.83 with three separate pointer locals |
| `battle/misc19`, `battling/misc5`, `world_map/misc16`, `psyq/libgpu_b` | parks only; `menu/misc3` timed out with an uncommitted non-match and was dropped |

Banked idioms: `g_Menu->digits[i] = number / divisor;` with no quotient temp keeps
`number` in `$a0`; splitting one pointer local into per-loop copies is what stops gcc
sharing an allocno across two lifetimes; `count` must be an `s32` read from a `u8`
array (not a `u8` local) to get `slt`/bare `beqz` instead of `sltu`/`andi 0xff`.

`world_map/misc16`'s `func_80099708` reached 78.20 and is **provably not finishable in
C**: retail's tail has `addiu t0,t0,%lo(D_1F800008)`, a splat reloc naming `pPkt++`'s
+8 that no C can emit, so objdiff can never reach 100 there.

### The remaining split candidate, measured (not done)

`battle/main/misc19` is mixed and its head run is clean under gcc-2.6.0: `func_8009A9D0`
is **100.00 under 2.6.0** (99.66 on the unit's 2.7.2, the only diff being the `mfhi`
destination) and the already-matched `A0DC/A1AC/A7B8/A7E4/B098/B104/C0E0` all stay
100.00 — but `AA44/AB00/AC48` break, so it needs a **5-run** split (offsets are
`0x2A5EC / 0x2AEE0 / 0x2AF54 / 0x2B2B0 / 0x2B40C` in `config/battle.yaml`). One function
for five files; worth doing only if someone is walking the unit anyway.

Session 9 total: **1455 -> 1435 stubs (+20)**, 3067 -> 3088 functions (67.6%), eleven
outputs byte-identical throughout, progress page republished twice.

# Session 10: the park sweep over cdk / 2.6.0 / -G0, and two rounds

Start: **3088 / 4568 functions (67.6%)**, 1435 `INCLUDE_ASM` stubs, eleven outputs OK.
Now: **1416 stubs**, eleven outputs byte-identical.

## The park sweep, now checked in: `tools/rounds/park_sweep.py`

Session 8's park sweep scored every parked body under the **maspsx** axes
(`--expand-div`, `--dont-expand-li`, `-G4`, `-O1`) and found nothing left. It did not
score them under the three **compiler** axes that have produced every other recent
win - `gcc-2.7.2-cdk`, `gcc-2.6.0`, `-G0`. `try_flags.sh --all` cannot see a parked
body at all (a park is a comment), which is why this needed a tool:

    tools/rounds/park_sweep.py                 # every unit that has a park: own + cdk + 260 + g0
    tools/rounds/park_sweep.py --one --own-only <unit>...   # one body at a time, own preset only

It un-parks every body of a unit, compiles the unit once per candidate, prints one row
per parked function, and `git checkout`s the file. 106 units in ~70 minutes on a busy
box (the container is shared with a worker round; expect `compile-failed` rows and
re-run them).

**The three compiler axes are now exhausted at park level too.** The whole sweep's
nonzero harvest is four units:

| unit | parked body | own preset | verdict |
|---|---|---|---|
| `battle/misc20` | `func_8009D948` | **100.00** | stale park - un-parked |
| `battle/misc33` | `BED30 BEDE8 BF354 BF3A4` | 73.6/68.1/57.5/71.4 | **100.00 under cdk + -G0** -> unit moved to `XenoBattleTailCdk0`, all four un-parked |
| `battle/misc12` | `func_80084D28` | 95.21 | 100.00 under 2.6.0, and every C function in the unit stays 100.00 there -> **unit switched to `XenoBattle260`**, un-parked |
| `battle/misc19` | `A9D0`, `ADA0` | 99.66/88.68 | 100.00 under 2.6.0 but `AA44/AB00/AC48` drop to 99.57/95.71/99.77, so it needs the **5-run split**; left undone (2 functions for 4 new files) |

Nothing else, in any unit, under any of the three. Combined with session 8's maspsx
result, **a parked body that is not 100.0 under some preset is now a dead park** and
the only way forward in that unit is new C.

Coverage caveat: 44 units produced `failed` rows (un-parking *every* body at once does
not compile, because one parked body is not standalone-valid C). The `--one --own-only`
mode exists for exactly those; it found no stale park in the 40 rows it managed to score,
but under a full worker round most rows come back `compile-failed` - run it on an idle
container if it matters.

## Log

| # | Units | Functions | Stubs left | Notes |
|---|---|---|---|---|
| 29 | `battle/{misc18,misc13,misc23,misc20,misc14}`, `battling/{misc5,misc4}`, `menu/{misc12,misc3}`, `world_map/{misc16,misc13}`, `movie/{misc1,misc2}` | +3 | 1432 | battle/misc14 `func_80088990`, battle/misc18 `func_80099CF0`, menu/misc12 `func_801E8B4C`. Nine of thirteen workers returned park notes only; four timed out with uncommitted tails that merge (correctly) discarded |
| - (sweep) | park sweep over 106 units, cdk/2.6.0/-G0 | +6 | 1426 | misc33 +4 (preset switch, no C written), misc20 +1 (stale park), misc12 +1 (preset switch) |
| 30 | `battle/{misc24,misc25,misc3,misc8}`, `battling/misc3`, `menu/{misc7,misc4}`, `world_map/{misc9,misc7,misc1}`, `field/{misc8,misc6}`, `psyq/libgpu_c` | +10 | 1416 | battle/misc24 +4 (`AA564 AAA20 ADF1C AE098` - tiny 42-104 insn functions), battle/misc25 +3 (`AF2C4 AF518 B10EC`), world_map/misc1 +2 (`72BB0 737EC`), battle/misc3 +1 (`77454`). Two workers timed out after committing; nothing lost |

## What rounds 29-30 taught

- **`battle/misc24` and `misc25` (2.6.0 + `--expand-div`) convert at 4 and 3 per worker**
  because most of their stubs are 42-172 instructions. The tiny-stub tail of `battle`
  is the highest-yield C-writing pool left: `misc24`, `misc25`, `misc34`, `misc28`,
  `misc7` (605 insns over 14 stubs) and `battling/misc14` (965 over 10).
- **`func_80085310`'s "8-byte phantom frame" is not a compiler wall**: it is 100.00 under
  gcc 2.6.0 in `battle/misc12` and 100.00 under 2.7.2 in the units that already match.
  The round-10 conclusion ("two register-indexed loads of the same global always give an
  8-byte frame") is real, but it does not stop a match when the frame is what retail has.
- A wrong *parameter type on a callee* costs a delay slot: `func_801D1EE0`'s first
  parameter must be `s32`, not `u_char`, or every call site gets an extra
  `andi a0,a0,0xff` and reorg cannot put the following `addiu` in the `jal` slot
  (`menu/misc12`'s `func_801E8B4C`). Check a callee's prototype before blaming your caller.
- `battle/main/misc24` was listed in **both** `XenoBattleDiv` and `XenoBattle260Div` in
  `gears.toml` - the round-8 hash-order hazard, with two identical path keys instead of a
  directory key. Removed from the 2.7.2 preset; two consecutive `objdiff-config` runs now
  agree that the unit's gcc is 2.6.0.
- 156 of the remaining 1423 stubs (11%) reference a `jtbl_` symbol and cannot be converted
  without moving the unit's whole rodata to C; per binary that is battle 58, world_map 42,
  battling 15, menu 13, slus_006.64 12, field 6, movie 5.
| 31 | `battle/{misc4,misc9,misc20}`, `battling/{misc14,misc15,misc16,misc7,misc9}`, `menu/{misc11_r2,misc3}`, `world_map/misc16`, `field/main/misc`, `psyq/libgte` | +11 | 1405 | battle/misc4 3 (`79674 792F8 78B34`), battling/misc7 3 (`F8E4 80780 FBEC`), psyq/libgte 2 (`VectorNormal VectorNormalSS` on the PSY-Q preset), battling/misc14 1 (`8ACB8`), misc16 1 (`8DDFC`), misc9 1 (`82A70`). All 13 workers committed; two of them also added GTE macros to `include/psyq/inline_c.h` |

### Round 31's harvest: the tailored brief is what moved the needle

The same harness, same model, same 30-minute kill: round 29 (units picked by stub count)
returned 3, round 30 (units picked by *insns per fresh stub*) 10, round 31 (the same
ranking, plus an explicit exclusion list per unit from `atfree.py` and `grep -l jtbl` and a
per-unit note) 11. Two things did it:

- **Give the worker the reachable subset, not the unit.** `battle/main/misc4` has 8 fresh
  stubs and all 8 were listed; `battle/main/misc9` has 15 stubs but only 4 that are not
  already 95%+ parks, and the brief named those 4. Workers stop burning 10 minutes per
  stub on functions the repo has already declared dead.
- **The tiny-stub tail really is the pool.** 6 of the 11 came from stubs under 100
  instructions (`8ACB8` 89, `8DDFC` 20, `FBEC` 166->half, `79674/792F8/78B34` 66-95).

### `gte_gpf12()` and `gte_gpf0()` were literal data (fixed)

`battling/misc16`'s worker found that both macros expanded to `.word 0x000012bf` /
`0x000012ff`, which maspsx passes through untouched and GAS assembles as *data*, not a
COP2 command - so any function using them was unmatched by construction. The canonical
3.x form (two nops plus the `0x4b9_003d` COP2 word, exactly what `gte_gpf12_b()` emits)
is now what the two macros expand to. Nothing used them yet, so the fix is byte-neutral
(`make check` green), but it was a trap for every future GTE function.

### Also banked this round

- **A call in its own statement keeps the address computation after the calls.** Putting
  `value = f(buf[g(...)])` on its own line *before* computing
  `pOut = (u16*)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420)` is what stops gcc hoisting the
  address into a callee-saved register, and the `(s32)` cast on the symbol stops it
  folding to `%lo(sym)(reg)` (the `$at` form retail does not have). 75.6 -> 89.33 in
  `battle/misc9`'s `func_8007DE78`, and the family tail is now reproducible.
- **Swapping `if`/`else` arms** is the lever for a `bnez`-vs-`beqz`-only mismatch: two of
  `battle/misc4`'s three matches needed nothing else.
- **`register s16 v fill asm("$8")`-style pins** are required when a GTE read-back must
  land in a specific GPR: `"=r"` outputs get `a3/v0/v1`. `battling/misc16` added
  `gte_stmac123_gpr_ldir123()` and `gte_stir123_gpr(r0,r1,r2)` to `inline_c.h` for this.
- The `battling` GTE bodies are largely hand-written or need `mfc2` of `$12-$14`/`$24`
  into arbitrary GPRs (`misc15`'s `8C4B0`/`8C620`), and `misc15`'s `func_8008BC04` is a
  full register dump - `convertible.py` says OK for all of them, so it does not catch
  this class either.
| 32 | `slus_006.64/psyq/libgte`, `battle/{misc27,misc27_b,misc30,misc31,misc32,misc33,misc9}`, `battling/{misc12,misc14,misc15,misc16}`, `field/main/misc12` | +16 | 1389 | psyq/libgte 4 (`PushMatrix PopMatrix SetMulMatrix func_80048DD8`), battle/misc33 3 (`BEF24 BEF8C BF1EC`), battle/misc27 2 (`B572C B4F88`), misc27_b 2 (`5B3C 5C18`), misc30 1 (`B9B54`), misc31 1 (`BCBB4`), misc32 1 (`BD974`), battling/misc12 1 (`89534`), misc15 1 (`8BE4C`), misc16 1 (`8DCB8`). First round that broke the overlay - see below |
| - (preset) | `battle/main/misc30` -> `XenoBattleTailCdk0` (cdk + `--dont-expand-li` + `-G0`) | +3 | 1386 | a round-32 worker screened it: all five of its C functions 100.0 under cdk+-G0. Three of its parked bodies (`B9258 B9B30 B9C00`) are 100.00 under that preset and were un-parked |

### The worst failure mode found so far: a conversion can shorten an ALREADY-MATCHED CALLER

`battle/misc27_b`: a worker converted `func_800B59BC` (100.00%, byte-identical to the
stub), and `make check` then failed `battle.bin` by **8 bytes**, with 44290 differing
bytes over 334 symbols - the classic shifted-overlay signature. The culprit was not the
converted function:

    retail: func_800B5AC4 = 120 bytes      ours after the conversion: 112 bytes

`func_800B5AC4` is an *already-matched* C function that calls `func_800B59BC`. With only
the old-style `extern void func_800B59BC();` in scope, gcc emits a callee-saved
save/restore pair that retail has; with the definition *visible* it drops the pair
(8 bytes exactly). Fix: keep the stub **and** the `extern void ...()` declaration.
`objdiff` reports the caller at 100.00% either way - only the overlay checksum can see
this class, and it is the fourth distinct form of "a conversion moved bytes it does not own".

Cheap detector for the whole repo (sizes, not objdiff):

```sh
python3 - <<'PY'
import re,subprocess,glob
syms={}
for f in glob.glob("config/symbol_addrs.*.txt"):
    for line in open(f):
        m=re.match(r'(\w+)\s*=\s*0x([0-9A-Fa-f]+);',line)
        if m: syms.setdefault(f.split('.')[-2],{})[m.group(1)]=int(m.group(2),16)
for binname,tab in syms.items():
    addr=sorted((a,n) for n,a in tab.items() if n.startswith('func_'))
    size={addr[i][1]:addr[i+1][0]-addr[i][0] for i in range(len(addr)-1)}
    for o in glob.glob("build/src/%s/**/*.c.o"%binname,recursive=True):
        out=subprocess.run(["nm","-S",o],capture_output=True,text=True).stdout
        for line in out.splitlines():
            p=line.split()
            if len(p)==4 and p[2]=='T' and p[3] in size and int(p[1],16)!=size[p[3]]:
                print(binname,o.split('/')[-1],p[3],"retail",size[p[3]],"ours",int(p[1],16))
PY
```

Run it after any round that converted more than a handful of functions; it names the
function whose size is wrong instead of making you bisect a 334-symbol diff.

### A third unconvertible class: the `$sp` stack switch

Round 32 found the same shape in six units at once: retail allocates a block with
`HeapAlloc`, stores `$sp` into it, and moves `$sp` onto the block around one call
(`sw $sp` / `$sp = block-4` / `lw $sp`). No C construct and no builtin emits it -
`__builtin_alloca` gives an `$fp`-relative `addu`. Confirmed instances: `battle/misc27_b`
(`func_800B626C`, `func_800B61F8`, `func_800B62C8`), `battle/misc31` (`BB620`, `BB6E0`),
`battle/misc33` (`BEE2C`). Treat a stub with `sw $sp` / `lw $sp` as BLOCKED and say so;
the one authorised exception in the repo is `field/main/misc2.c`.

### psyq/libgte is now the cheapest psyq unit, and maspsx gotchas that cost a worker an hour

Four matches in one worker (`PushMatrix`, `PopMatrix`, `SetMulMatrix`, `func_80048DD8`)
after the round-31 precedent. Two macros' worth of knowledge, now in the file's header note:

- maspsx **aborts** (`invalid literal for int()`) on an `0x..` displacement inside inline
  asm - write displacements in decimal. Splat's own lines survive only because their
  `/* hex */` prefix keeps maspsx out of that parser.
- maspsx appends a nop after every `jal`/`j` unless it has seen `.set noreorder`; a block
  containing a call needs `.set\tnoreorder` … `.set\treorder`.
- GAS has no `sqr`/`gpf`/`mvmva` mnemonics; spell those `.word` (the `gte_*_b` values).
- gcc hoists the return-value `move` to the entry block unless the asm block declares the
  return register as an output: `register T *pRet __asm__("$2")` plus `addu $2,$4,$0`
  inside the block.
- `VectorNormalS` (5 insns) branches into the *middle* of `VectorNormalSS`; no C
  definition can, so it stays a stub.
| 33 | `slus_006.64/psyq/libgte` (2nd), `battle/{misc30,misc4,misc29}`, `battling/{misc14,misc12,misc17,misc18,misc1}`, `world_map/{misc5,misc6,misc15,misc3}` | +13 | 1373 | world_map/misc5 **4** (`7C36C 7CD20 7B200 7B604`), psyq/libgte 2 (`CompMatrix ApplyMatrixLV`), battle/misc30 2 (`BAB0C BB13C`), world_map/misc3 2, battling/misc1 1 (`88D1C`), battling/misc12 1 (`...`), world_map/misc6 1 (`7D4A4`); 0 in battle/misc4, misc29, battling/misc14, misc17, misc18, world_map/misc15 (parks only). Overlay green on the first try |
| 34 | `battle/{misc8,misc30,misc7,misc19,misc13,misc2,misc26}`, `battling/misc15`, `world_map/{misc1,misc10}` + a 6-worker naming round | +11 | 1354 | battle/misc8 **5** (`7CD10 7CEA4 7CB20 7D1DC 7D344`), battle/misc30 **4** (`BA614 BA768 BAEB8 BADD4`, the last two were parks), battling/misc15 1 (`8CA84`), battle/misc2 1 (`74D4C`); 0 new in misc7, misc19, misc13, world_map/misc1, world_map/misc10 (parks improved instead); battle/misc26's worker timed out with an uncommitted body and was dropped. 73 names applied, 27 skipped (lib_gear has no symbol_addrs entries for them) |
| 36 | 11 workers: `world_map/{misc7,misc9,misc10}`, `battling/misc3`, `slus_006.64/system/sound`, `battle/{misc12,misc13,misc26,misc31}`, `field/misc8`, `menu/misc3` + a 10-worker naming round | +7 | 1338 | `battle/misc12` **3** (`83FF4 84108 84750`), `battle/misc26` **3** (`B3B6C B1EA0 B3658`), `world_map/misc9` 1 (`86700`); 0 in world_map/misc7, world_map/misc10, battling/misc3, system/sound, battle/misc13, battle/misc31, field/misc8, menu/misc3 (parks improved or walls re-measured; most of them were already parked). ~450 names applied |
| 35 | 13 workers: `battle/{misc32,misc34,misc28,misc8,misc4,misc5,misc6,misc6_r2,misc27_b}`, `battling/misc16`, `slus_006.64/system/{archive_b,controller_b,work_list}` + a 10-worker naming round | +9 | 1345 | `controller_b` **2** (`80035F1C 8003634C`, the unit is now 3 stubs from done), `battle/misc34` **2** (`BF600 BF9EC`), `battle/misc6_r2` **2** (`7AE38 7AE98`, via the new clamp-copy idiom), `battle/misc27_b` 1 (`B5CC0`), `battle/misc28` 1 (`B6990`), `battling/misc16` 1 (`8DA48`); 0 in misc32 (16/21 atfree), misc8, misc4, misc5, misc6, archive_b, work_list (parks improved instead). 244 names applied |
| 37 | 13 workers: `battle/{misc12,misc16_r2,misc21,misc23}`, `field/{misc,misc6}`, `world_map/{misc2,misc5}`, `menu/{misc7,misc12}`, `slus_006.64/system/temp2`, `lib_gear/main/main`, `battling/misc6` + a 10-worker naming round | +4 | 1334 | `world_map/misc2` **3** (`758C0 7634C 76594`, landed by a worker killed before its gate), `battle/misc12` 1 (`84548`); 0 in the other eleven - park notes only, or the worker timed out (misc12, field/misc, world_map/misc2 all hit the 30-minute kill). 252 names applied, 38 skipped (33 are addresses that are a different function in another overlay) |
| 38 | 15 workers: `battle/{misc3,misc16,misc27,misc27_b,misc28,misc29,misc31,misc33}`, `battling/{misc11,misc15,misc16}`, `menu/misc2`, `field/misc6`, `world_map/misc15`, `slus_006.64/system/rendering` + a 10-worker naming round | +0 | 1334 | **every one of the 25 children was killed at 30 min**; 21 made no file edit at all and the four that did only un-parked a neighbour's 90-96% park. One name landed (`field/misc8` `func_800864B4` -> `FieldClearActorSoundSlots`) |
| 39 | 8 workers: `field/{main/misc,main/misc6}`, `battle/main/misc3`, `battling/main/{misc6,misc15}`, `world_map/main/{misc1,misc15}`, `menu/main/misc11_r2` + a 4-worker naming round | +5 | 1329 | `world_map/misc15` **2** (`97DC0 97440`), `world_map/misc1` **2** (`71FEC 74594`), `field/main/misc` 1 (`8D808`); 0 in the other five - park notes only, but this 8-child round did not starve: all eight committed and gated. 104 of 105 names applied |
| 41 | 10 workers: `battle/{main/misc12,misc13,misc19,misc29,misc33}`, `battling/main/{misc11,misc12,misc16}`, `world_map/main/misc15`, `menu/main/misc2` + an 8-worker naming round | +6 | 1320 | `battle/misc33` **2** (`BEC18 BEBC4`), `battle/misc19` 1 (`A9B38`), `battling/misc11` 1 (`8832C`), `menu/misc2` 1 (`1C8A10`), `battle/misc29` 1 (`B8284`, unlocked by the new XenoBattleTailCdk preset); 0 in `battling/misc12` (its stub is a 4-byte pointer datum, not code), `battling/misc16`, `world_map/misc15`, `battle/misc12`, `battle/misc13`. 145 names applied, 17 new `symbol_addrs.member_change_menu` entries |
| 42 | 13 workers: `field/main/misc`, `lib_gear/main/main`, `slus_006.64/system/temp2`, `world_map/main/{misc1,misc2,misc5,misc15}`, `battle/main/{misc3,misc11,misc12,misc22,misc16_r2}`, `field/scripts/virtual_machine` + a 7-worker naming round | +4 | 1324 | `field/main/misc` 1 (`8BDD8`), `world_map/misc2` 1 (`7565C`), `world_map/misc5` 1 (`7AD34`), `battle/misc12` 1 (`85454`, CHECK_BIN_OK - the only diff is a `%hi/%lo(D_800D2D67)` relocation objdiff cannot resolve); 0 in the other nine. 57 of 59 names applied. Three naming files (`world_map/misc3/misc6/misc8`) had no unnamed C definition - the parked-body census over-reports even after stripping `/* */` |
| 43 | 12 workers: `slus_006.64/system/{sound,rendering,animation_scripts}`, `battle/main/{misc2,misc3,misc11,misc12}`, `menu/main/{misc12,misc11_r2}`, `world_map/main/{misc1,misc6}`, `lib_gear/main/main` + an 8-worker naming round | +5 | 1329 | `system/sound` 1 (`3EFE4`, via a position-independent `for (i = NUM; i != 0; i--)` counter), `system/rendering` 1 (`1F750`, un-parked from 89.74 with a `u8` flag copy), `battle/misc3` 1 (`77074`, 18-call packet init, first try), `menu/misc11_r2` 1 (`1E4A28`), `world_map/misc6` 1 (`7FF70`); 0 in the other seven but `battle/misc11`'s `80C94` went cold -> 86.16 park and `menu/misc12`'s `1E8F60` 85.90 -> 95.69. 106 of 109 names applied |
| 44 | 8 workers: `slus_006.64/system/rendering`, `battle/main/{misc3,misc24,misc28}`, `world_map/main/misc14`, `battling/main/misc8`, `menu/main/{misc6,misc12}` + a 4-worker naming round | +3 | 1308 | `system/rendering` 1 (`1F8E8`), `battle/misc24` 1 (`AAB34`, via the unit's new `psyq/inline_c.h` include), `menu/main/misc6` 1 (`1D1EE0`, byte-identical twin of `GearShopMenuSetSelectionCursor`); 0 in the other five - `battle/misc3`'s `76EA4` is a one-`addu`-operand-order wall (98.19), `battle/misc28`'s `B6F0C` 85.91, `world_map/misc14`'s `95CD4` 87.03 (scratchpad address CSE), `battling/misc8`'s `81ECC` 70.16 (retail merges field stores into wider ones), `menu/misc12`'s `1E86C8` 91.12 -> 99.79. 67 of 68 names applied |


### Round 36's per-unit detail

- `world_map/misc9`'s `func_80086700` (`WorldMapStepTileVertices`) needed the **indexed** form
  (`pDst[i].unk0`) where the pointer form stayed at 92.29: the biv stays on the symbol base and
  the displacements become the literal struct offsets. `func_80087904` went 92.44 -> 98.83 with an
  unused `long tmp[2]` (frame 0x30) and `i = 0;` before the `if (count > 0)` guard.
- `battle/misc12` converted 3: its parks were **wrong**, not just incomplete - `func_80083FF4`/
  `func_80084108` had the branch polarity inverted and `func_80084750` was missing the `n++`,
  so `backup/`'s parked C is not evidence. New idiom (now in `brief.js`): when retail emits
  `sltiu v0 / addu t0,v0,zero` in the `j` delay slot, the boolean local is **`u8`**, not `u32`.
- `battle/misc26` matched 3 (`B3B6C B1EA0 B3658`); the parked `func_800B3B6C` note (94.00) was
  stale - under this tail unit's `--dont-expand-li` the `li 1` is `addiu v0,zero,1`, as retail.
- Parks only (measured, re-parked): `world_map/misc7` 8 walls (its 36-row `try_flags --sweep`
  tops out at the default's 84.76, preset settled), `world_map/misc10` (SCALAR_ADDR confirmed,
  `-G0` gives the same 90.66), `battling/misc3` (8 delay-slot walls; **`func_80077584` cannot take
  a prototype** - its matched caller `func_8007762C` passes one argument), `system/sound` (all 22
  already parked 60-98.91; `func_8003D53C` is 100.0 under gcc 2.7.2, a per-file preset case that
  cannot be split out of `sound`), `battle/misc13` (`func_80085D34` 90.42 -> 95.68, its second
  `func_80076A10` arg is the constant `0x19`), `battle/misc31`, `field/misc8`, `menu/misc3`.
- **Naming (: ~450 of 502 applied).** `rename_sym.py` only renames symbols that already have a
  `symbol_addrs` entry, which most `field/` and overlay `func_` do not. A plain sed is NOT enough:
  splat names a binary's own relocations (calls and `.data` function tables) from `symbol_addrs`,
  so the entry has to be *added* - `NewName = 0xADDR;` (the address is in the old name) - not just
  rewritten in `src/`. Where to put it: the file of every binary whose `asm/` references the
  address, and never in a file that the same yaml already loads (`gear_shop_menu` loads
  `lib_gear`'s; `slus_006.64`'s is loaded by every overlay), or splat aborts with "Duplicate
  symbol detected" and the whole round stops linking. Adding a function symbol inside another
  binary's rodata range also makes splat carve it (`.L8...` undefined references in shop_menu, from
  two `GearShopMenu*` names that a global sed had put at `shop_menu` addresses). 4 address twins
  across overlays were reverted to `func_`.

### Round 35's per-unit detail

- `battle/misc6_r2`'s `func_8007AE38`/`func_8007AE98` (93.33/94.55 -> 100.00, first try) are the
  **clamp-copy idiom**: retail's `addu v1,v0,zero; slti v1,v1,0x100` is a separate narrow
  variable, so write `s32 value = sum; u8 result = value; if (value > 0xFF) result = 0xFF;`.
  One variable gives a test-in-place. Now in `brief.js`.
- `atfree.py` OVER-reports: it flags any `%lo(sym)(reg)` where reg is neither `$at` nor the
  destination, but a function that reads the same global twice matches fine (retail reuses the
  register). The real wall is an address used exactly ONCE. `misc34` matched 2 of the 9 stubs
  the script flagged; `misc32` is genuinely 16/21 atfree and yielded 0.
- `battle/misc6`/`misc6_r2`/`misc28` share one dead shape: retail's table base `la D_800D3420`
  lands in `$v1` where gcc insists on `$v0`, everything else byte-identical (97.0-99.0%, 19
  source shapes plus `-O3 -G0`). `misc5`'s parks are the same class (`mult v1,v0` vs `v0,v1`).
- `battle/misc4`'s `func_800799C8` went 87.14 -> 98.71 with `cmd = *pCursor; while (cmd != 0xFD)`,
  leaving only branch polarity, which is a cdk question - candidates `try_flags.sh --gcc
  gcc-2.7.2-cdk-psx` / `func_80078E24` (the unit's only unparked stub); `misc34`'s
  `func_800BF4F0` and `func_800BF85C` are also still unattempted.
- Naming: 244 of 252 names applied. `rename_sym.py` skipped 178 for having no `symbol_addrs`
  entry at all (most `field/` and overlay `func_` have none); a whole-word `sed` over `src/`
  and `include/` applied those, which is the same edit the tool makes. 8 left alone: 5 symbols
  that are `INCLUDE_ASM` in a second overlay, 1 name collision, 2 still-stub functions.

Session 10 total: **1435 -> 1345 stubs (+90)**, eleven outputs byte-identical throughout,
and three durable tools/findings that outlive the matches:

### Round 33's per-unit detail (from the workflow receipt)

- `world_map/misc5` converted 4: the shape that gets 100% on `func_8007C36C` is a **field-pointer
  local** (`u32 *pCount = &pPlayer->unk50; ... D_8009A4E8[*pCount]`), and `func_8007CD20` is a
  legit `CHECK_BIN_OK` case - objdiff 99.94% only because the relocation spells
  `addiu $s1,$s2,%lo(D_1F8000A0)` where we emit the immediate `160`, same encoded bytes.
  Its `switch` compiled to a compare tree, so no jtbl was added.
- `battle/misc30` (now cdk + -G0): both matches needed an **unused aggregate local**
  (`RotTransPersOut.sxy/flag/pad[4]`, `u32 unused[2]`) because gcc places aggregate locals
  before scalars - a plain `unused[]` cannot sit above `sxy`/`flag` in the frame.
- `psyq/libgte`: `CompMatrix` and `ApplyMatrixLV` matched by **pinning the asm output to the
  ARGUMENT register** (`register VECTOR *pRet __asm__("$6")`) rather than `$2`: that makes gcc
  emit the `move v0,aN` itself, so reorg puts it in the `jr $ra` slot (98.18% the other way).
  The unit's remaining 14 stubs all own trailing `nop`s after `.size` and are unconvertible -
  `libgte` is finished.
- `battling/misc12`'s `func_80088D1C` needed a 3-case `switch` (an if/else chain compiles
  inverted) plus an unused `RECT rect;` for retail's 0x40 frame.
- Dead ends re-confirmed: `battling/misc14`'s `func_8008A7E0`/`func_8008BB3C` are
  `/* Handwritten function */` trampolines, `func_8008B13C` owns two jtbls, and
  `func_80088BFC` starts with a dead absolute `lb` (the real callback body starts at
  0x80088C00 - that needs a splat symbol, not C).

1. `tools/rounds/park_sweep.py` - the cdk / 2.6.0 / -G0 park sweep, run over 106 units. The
   three compiler axes are now exhausted at park level, exactly as session 8's maspsx
   sweep was, so **a parked body that is not 100.0 under some preset is a dead park**.
2. `tools/scripts/sizes.py` - names the function whose compiled size is wrong, instead of
   bisecting a 334-symbol `culprit.sh` diff. Zero flagged in the tree as of this commit.
3. The short-caller failure mode (`battle/misc27_b`): converting a callee can shrink an
   already-matched caller by dropping a callee-saved pair, and objdiff reports 100.00 for
   both. Only the overlay checksum sees it; `sizes.py` names it.

Still open, with evidence: `battle/misc19`'s 5-run split (2 functions, 4 new files) and the
`field/main/*` units whose `.rodata`/`.data` subsegments keep them unsplittable.

### Round 37's per-unit detail

- 77 reachable stubs across 13 of the freshest units closed 4. "Fresh" (no percentage near
  the stub) is not "achievable": 11 of the 13 workers wrote park notes only, and three of
  those did not finish. The three matches that did land came from a worker that committed
  before its kill.
- `battle/misc16_r2` corrected an old belief: `s32 *p = &D_800CCB34;` + `*p` DOES give
  retail's `lui/addiu` + `lw` per use. The unit's other parks should be re-shot with it.
- `menu/misc7`'s four window borders (`func_801D49D0/3FF8/433C/4688`) are one problem, not
  four: `member_change_menu/main/misc.c` has all four **matched** as
  `MemberChangeMenuSetWindowBorder{Top,Bottom,Left,Right}` - copy its phrasing.
- Trap in `battle/misc21`: naming `BattleBlock`'s byte at 0x3 (`unk1[2]` -> `tag`) drops the
  already-matched `func_800A2ACC`/`func_800A2BB8` from 100 to 91% (array-index spelling
  changes gcc's induction). Struct-field renames are not free.

### Round 38: 25 children, 0 matches - the round was starved, not wrong

- All 15 matching workers and 9 of 10 naming workers were killed at 1800000 ms with
  `changed tracked files: none` or a single un-park. Nine of the fifteen matching
  workers' reports open with "I'll start by reading ..." - they never reached a second
  edit. That is queue/latency starvation, not a documentation or targeting failure:
  round 37 ran 23 children on the same unit mix and closed 4.
- **Do not run 25 children again.** Cap a round at ~12 (8 matching + 4 naming) and
  expect the survivors to do the work; a killed child's clone is worth nothing because
  `merge` refuses uncommitted edits, and four of this round's five un-parked bodies were
  *other* functions' parks (90.55%, 96.44%) that the brief never assigned.
- The naming round also proved the target-selection error: ranking files by a bare
  `grep -c func_` counts `extern` prototypes (world_map/misc3: 5 stubs, 174 `func_`
  mentions, 0 unnamed C definitions). Count *definition* lines (return type + `{`) with
  comments stripped, or ask `objdiff`'s matched set.

### Round 39: the 8+4 cap works, and "fresh" now means *reachable*

- 36 reachable stubs across 8 units closed 5, and **every child committed and passed its gate** -
  the 8 matching + 4 naming split did not starve (contrast round 38's 25 children, +0).
- Ranking by instructions per fresh stub is only half the filter: the *highest* rows were
  `battle/misc33` (11 of 15 stubs atfree) and `misc31` (16 of 20), which would have burned two
  workers. `atfree.py` per candidate unit before briefing, always.
- The reachable set is cheap to compute: strict "no `NN.NN%` within 25 lines of the stub" gave
  430 of 1334 stubs; after atfree + jtbl + `convertible.py`, the top 8 units held 36.
- Naming is still the cheap number: 4 read-only workers on `field/misc12` (31 unnamed C defs),
  `field/misc7` (26), `battle/misc10` (23), `field/misc4` (19) proposed 99 names, 98 applied.
- Naming fallout to expect: `func_8009A1AC` is a *different* function in `field` and in `battle`
  at the same address, so a global rename names both and field's asm data table
  (`g_FieldScriptVMHandlers`) then links undefined - add the entry to
  `symbol_addrs.field.txt` by hand. One name (`func_8007EEE8`) was proposed for a stub and
  correctly skipped.
- A re-park appended after a note line that already ends with ` */` makes the whole attempt
  plain code: the only symptom is ninja's `.i` failing with `parse error before '39'`, which
  `check_bin.sh` reports as a bare `make: *** [build] Error 33`.

### Round 40: 8 workers close 3; the naming round is 84 names and needed a tool fix

- **`name_addrs.py` skipped every main-exe function that an overlay calls.** Its guard demanded
  that the address be mentioned by the `src/` of exactly ONE binary, which is never true for
  `system/sound` (field/movie/world_map call it), so 26 of 83 names were dropped as
  "no single owner binary". The owner is the binary whose C *defines* the function; a genuine
  address twin is already caught by "2+ C definitions". Patched, and the 26 entries landed in
  `symbol_addrs.slus_006.64.txt` with `make check` green - the per-binary add loop below it was
  always written for this case.
- Yield: 40 reachable stubs across 8 units closed 3. Every worker committed and gated, but each
  spent its whole 30 minutes on ITS FIRST stub, because this round's reachable stubs averaged
  150-250 instructions (round 39's averaged smaller). **Brief 2-3 stubs per worker when they are
  over ~150 insns**; a 4-7 stub list only looks like capacity.
- Mechanism found in `battling/misc2`: an argument global must be read into a first-declared local
  (`s32 n = D_80092648;` as the first statement) to get retail's load-hoisted-above-the-prologue
  shape; `func_8007334C` went 76.19 -> 100.00 on that alone.
- `menu/misc1`'s pair (`func_801C55A0`/`func_801C58EC`, 96.86/97.66) needs the switch written in
  *case address order* 4,1,3, not value order; the remaining wall there is retail's double `lbu`
  of the tested `u_char` against our single live value.

### Round 41: 10 workers close 6, and the jtbl blocker has a fatal variant

- The yield came from the selection filters, not from the briefs: of 15 "fresh" stubs three were
  `sw $sp`/`lw $sp` alloca bodies (`system/graphics`, `system/rendering_pause_text`, `battle/misc33`
  `func_800BEE2C`) and one (`battling/misc12` `func_80088BFC`) is a 4-byte pointer datum whose real
  code starts at +4, so the reachable list was 14 before the round started. Both filters are now in
  PROMPT.md item 2.
- `battle/misc12` proved the fatal jtbl variant: 11 of its 16 stubs dispatch with `jr $v0` through
  tables owned by `asm/battle/data/0.rodata.s`, so an if/else chain cannot match and a C `switch`
  emits a *second* copy of the table - `make build` fails at link instead of shifting bytes. The same
  holds for `world_map/misc15`'s `func_80096A6C`/`func_80097800` (table `jtbl_80070CB8`/`CE8`).
  `convertible.py` reports all of them convertible: `grep -l jtbl` is the only gate. One
  address-ordered migration of those tables would unlock 11 battle functions at once - a config job.
- `battle/misc29` split into its own preset (`XenoBattleTailCdk`, 2.7.2-cdk + XenoBattleTail's maspsx
  flags) for `func_800B8284`: 100.00 under cdk, 77.69 under 2.7.2, five matched functions unaffected.
- Naming: 8 read-only workers, 145 names, 17 new `symbol_addrs.member_change_menu` entries. A
  two-column names file silently neuters `name_addrs.py` (see PROMPT.md, Names).

### Round 44's per-unit detail

- **A convert-once caller can be shortened by a rename, not by a conversion.** `battle/main/misc28`'s
  matched `BattleSetTaskCallback` took its callback as `extern u8 D_80025A88[]`; naming that function
  `SpriteDrawModelPartsInBaseMatrix` made battle's `undefined_syms_auto` carry the new name, so the C
  spelling had no definition and the link failed with `undefined reference to D_80025A88`. Neither
  `rename_sym.py` nor `name_addrs.py` touches `D_<addr>` spellings - grep for them (now in PROMPT.md).
- gcc does **not** drop a declared-but-unused local: `system/rendering`'s `func_8001F8E8` got retail's
  0x40 frame from an unused `VECTOR vec;`, against the parked note's claim. Its other lever was
  `u8 unkFlag` (a 32-bit one lets reorg steal `andi s5` into a delay slot).
- `for (x = 0; x != N; x++)` reproduces retail's `li v0,N; bne`; `<` gives `slti/bnez` (battle/misc28,
  worth 6 instructions on its own). Both are now in `brief.js`.
- Raw is **2663 -> 2678**, not a regression in old code: 3 new bodies, two of them ROUGH
  (`battle/misc24`'s indexed matrix address, `system/rendering`'s `dirTransforms` operand order) and the
  rest park-note text, which the raw grep counts. Those two owe a rewrite round.
- `tools/scripts/sizes.py` now flags **177** functions, all in units this round did not touch, e.g.
  `battle/misc24`'s `func_800AAA20` "retail 820 ours 276" - a matched function whose bytes are
  right (`make check` green). Its retail size is `next symbol_addrs address - this one`, so it is only valid where the symbol file is
  contiguous; treat a flag as a hint, not a finding, until that is fixed.
| 45 | 14 workers + 10 naming workers | +5 | 2680 | `SoundInitialize` (system/sound, 2.6.0), `func_80085B58` (battle/misc13), `func_801C5FE4`/MenuFreeMenuData (menu/misc1), +2 un-parked when menu/main/misc1 switched to XenoMenu260 (func_801C55A0, func_801C58EC both 100.0 under 2.6.0). 83 of 97 names applied, 14 skipped (addresses in no symbol_addrs file). 11 of 14 workers died on the 30-minute kill with a live park. See the freshness-test note below. |
| 46 | 14 workers (batt10 batt12 batt2 bmisc22 bmisc23 bmisc8 fmisc5 fmisc8 mmisc3 mmisc4 mov2 sound wm10 wm4) | +0 | 2680 | Nothing merged: no clone produced a commit, three held dead in-progress bodies (battle/misc22 `func_800A32D8` volatile-pointer retry of the 74.17% park, battle/misc8 `func_8007C1A4`, world_map/misc4 `func_80078E2C` at 35.90%), all below their parks. **Strike one for those 14 units** - do not brief them in round 48. |
| 47 | 12 matching + 10 naming workers | +4 | 2680 | `func_80084B48` (battling/misc10), `func_8007CF78` (battling/misc5), `func_8007625C` (movie/misc2), `func_80082F64` (world_map/misc8, typed actor fields). 64 of 64 names applied; the three fresh matches below the naming round's files are still `func_`. Parks: field/misc4 `func_8007BAC0` 76.42 (probe/retry structure decoded), menu/misc11_r2 `func_801E5178` 94.34 / `func_801E6668` 98.90, world_map/misc8 `func_8008390C`, world_map/misc10 (3), temp2 `func_80026F44` 74.63 (loop shape solved). Three naming workers were duds (world_map/misc3, misc6, misc5 have no unnamed C: the file picker counted column-0 `extern` prototypes). Every clone committed the then-untracked `tools/pi/`, which blocked five merges - it is tracked now. |
| 48 | 13 workers (b13 b15 b2 bt4 bt5 bt6 bt8 fm2 fmain lg me7 wm1 wm11) | +1 | 2696 | `func_8008AAA0` -> BattleParseNumberToDigits (battle/misc15), a 9-digit 0xFF-padded stat formatter its misc14/misc17 callers confirm. Two struct recoveries rode along: battling/misc5 typed BattleSlot 0x6C-0x78, battling/misc8 extended BattleTextList with the 0x1C frame area. The other eight clones committed park notes only (battle/misc2 65.07, battling/misc5 85.17, misc6 96.74, misc8 77.44, lib_gear/main 98.63, menu/misc7 95.19 twins, field/main/main 87.32, battle/misc13 85.48), and wm1/wm11 committed nothing. The orchestrator session that launched this round died mid-flight; recovering it cost one `git log ee7ddc37..HEAD` per clone, since the pool branches are not `master`. Raw rose 2680 -> 2696: park-note prose the grep counts, plus the new body.
| 49 | 13 matching (misc3 bmisc21/22/23 bt2 bt10 wm4 wm8 wm9 sound temp2 mov1 mmisc12) + 8 naming workers | +7 | 2685 | `func_80078A60`/`func_8007A5DC` (world_map/misc4), `func_800866D4`/`func_800864B4` (battling/misc10), `func_801C9D34` (menu/misc3), `func_801E8474` (menu/misc12), `func_800747AC` (movie/misc1). Parks committed: world_map/misc4 `func_8007A1B4` 99.90, movie/misc1 `func_80070DCC` 99.78, sound `func_8003C4C4` 96.45 (u16 field retypes), world_map/misc9 `func_8008615C` 95.40, battle/misc21 `func_8009EF3C` 94.94, world_map/misc8 `func_80085158` 91.97, battle/misc23 `func_800A6AE8` 84.16, menu/misc12 `func_801E78C8` 84, battle/misc22 `func_800A48EC` 78.13; wm8/wm9/wm4 gained WorldMapPos/Mesh types. Naming: 83 NAME lines from 8 workers, but feeding `rename_sym.py` + `name_addrs.py` the union of every NAME line in `subagent-artifacts/` (1753, both tools self-skip) applied 177 names, so ~94 were proposals from earlier rounds that never landed. Orchestrator changed hands mid-round (the launching session died at the 9-minute mark); recovery was 13 `git log main..HEAD` + a merge. |
| 50 | 9 matching (arch bt12 bt4 fmisc fmisc5 mm4 mov2 wm1 wm11) + 8 naming workers | +1 | 2685 | `func_8008A7DC` -> FieldPartyLoadMemberSkin (field/main/misc: party member skin archive into a HeapAlloc buffer, `g_GamePartyMemberSkins`, GameCharacterGetGearID on the non-map path). The other eight clones committed park notes only: system/archive `func_8002BB50` 82.00, battling/misc4 `func_8007A768` 92.44, battling/misc12 `func_800898BC` (MATRIX* first param), field/main/misc `func_8008EA58` 74.57, field/main/misc5 `func_800A6E70` 99.56 re-verified, movie/misc2 `func_800763BC` 89.61 / `func_80076AF0` 98.87 / `func_8007519C` 91.70, world_map/misc1 and misc11 negative results (misc11 JTBL-blocked). Naming: 73 names (63 renames + 10 new `symbol_addrs` entries) from the 1832-address NAME union. **The fmisc worker left out its park block `*/`, swallowing the next `INCLUDE_ASM`; the only symptom was `undefined reference to func_8008EC30` from a field data table.** `tools/scripts/lint_sources.py` now flags an `INCLUDE_ASM` inside a comment (check 3, negative-tested). The launching session died ~1 minute in; this session adopted the orphaned round 30 minutes later and all nine clones had committed. |
| 51 | 13 matching (sound fm2 fm8 mm9 fm4 btm6 btm5 btm9 bt8 bt22 wm4 wm10 lg) + 10 naming workers | +1 | 2689 | `func_8007E634` (battling/misc6, the smallest stub in the unit, first try). 18 park notes corrected or added; the round's real product is the negative results: system/sound's whole fresh list was already parked and the unit is 79-96.6% dead band, field/misc2's four smallest stubs are all rodata-BLOCKED (they own D_8006FB1C/FB70/FB34..FB64), field/misc4's six assigned stubs were all already parked (batch premise was stale), battling/misc9 `func_80082178` is a single s5/s6 pairing tie at 99.47. 82 names applied from a 1983-address NAME union (1884 skipped: still stubs; 8 name taken). 4 of 13 workers hit the 30-min kill, all after committing; their ungated in-tree leftovers were dropped before the merge, so nothing ungated was merged. `make check` green on all eleven. |

Round 51 detail:
- DECISION: `slus_006.64/system/sound` needs a per-file preset split - `func_8003D53C` is 95.83% under the unit's 2.6.0 and 100.00% under 2.7.2-psx/2.7.2-cdk, but sound owns `.rodata`/`.data`/`.bss` subsegments in `config/slus_006.64.yaml`, which PROMPT.md says makes a unit unsplittable. Cost: one sound TU plus a yaml subsegment rewrite; gain: 1 match. Not taken without the operator.
- Freshness test misses parks two ways: field/misc4's "four smallest" had all six stubs parked (the test looks 60 lines up, the note was further), and system/sound's 24-of-29 stubs carry notes. Both units burned a worker on re-deriving known walls.
- New wall class recorded in notes: gcc elides `sll 16`/`sra 16` pairs the retail has (system/sound X2), and mirrors two-register pairings across a whole body (battling/misc9 `s5/s6`, field/misc4 `s0/s1/s2`, battle/misc8).
- raw rose 2685 -> 2689 from park-body prose only (`world_map/misc7` +6, `misc10`/`misc6`/`battle-misc27_b` +2 each); next round owes a rewrite pass over `world_map/main/misc7` and `battle/main/misc24` (157 raw).
| 52 | 11 matching + 3 rewrite + 8 naming workers | +2 | 2667 | `func_800C0828` -> BattleComputeRotationFromPoints (battle/misc35, the one stub atfree.py does not flag there) and `func_800BA984` -> BattleCreateAnimTask (battle/misc30). The rewrite round paid: `include/battle/actor.h` (BattleActor/BattleSprite/BattleBlock) landed from battle/misc24, MenuCharacterWindow from menu/misc7, typed group actors in world_map/misc5; raw **2689 -> 2667**. 47 names applied (34 renames + 13 new symbol_addrs entries). Freshness test rewritten this round (whole-file name test + 140-line percentage test) cut the pool from 381 claimed-fresh stubs to 71 real ones, and the round's 14 briefs were drawn from those; yield was still 2, so the constraint is the walls, not the picking. |

Round 52 detail:
- DECISION: `battle/main/misc16_r2` should move to the 2.6.0 preset - `func_8008FAD8` is 90.53% under Default and 100.00% under gcc-2.6.0 (parked with its body), and `try_flags.sh --all` shows all four already-matched functions stay 100.00 under 2.6.0. Not taken: Ground rules make a preset change that matched functions depend on an operator action. Cost to switch: one per-file gears.toml key + delete one park comment pair; gain: 1 match.
- New walls recorded, all from this round's stubs: STACK_SWITCH by hand (`slus_006.64/system/rendering_pause_text` func_8001FB30 - `sw $sp`/`lw $sp` around a HeapAlloc block, twin GfxLoadImageAccelerated only reproduces with forbidden inline asm; battling/misc15 func_8008BC04 the register-save variant), and `slus_006.64/system/menu` func_8001C76C is not code at all - 92 `.word` lines of unreferenced tail data that splat keeps in `.text`, which `convertible.py` reports as "OK 92 insns" because it counts words.
- Two orchestrator-side failures worth knowing: `clone.sh merge` aborts a conflicting branch and prints only in the middle of its output - verify with `git merge-base --is-ancestor` per clone commit rather than trusting the scrollback (two matches sat unmerged until checked), and `name_addrs.py` can put a main-exe symbol in an OVERLAY's symbol file when only the overlay's C mentions the address (`SpriteDrawFrame` in world_map.txt), which links the main exe fine and breaks battle with `undefined reference`.
| 53 | 12 matching + 4 rewrite, no naming (union exhausted) | +4 | 2642 | `func_80077698` (battle/misc3), `func_8009EC4C` + `func_800A2D5C` (battle/misc21), `func_800A2FD8` (battle/misc22 - the unit's last untouched stub). Rewrite workers typed the sprite buffer in battle/misc22, the part-task node in misc25 and the structs in misc2/misc18: raw 2667 -> 2642. 11 of 16 workers hit the 30-min kill; the naming pool is exhausted (2034 proposals in the union, exactly 1 still landable), so naming rounds are over until new matches accumulate. |

Round 53 detail:
- Two merge repairs, both from killed workers: `battle/misc3` `func_80078508` had its INCLUDE_ASM deleted with no C body written (undefined reference from misc9's BattleResetDisplayValues; the phantom counted as a match until `make check` - stub count went back up by one), and `battling/misc18.c`'s rewrite commit does not parse (unbalanced construct, `BattleWork *p;` at statement position). misc18 reverted; the 76 raw lines there are owed again.
- Killed workers are now the biggest cost: 11/16 this round, 4/13 last round, 4/13 the round before. Most had a body in flight at the kill. Capping a round at ~10 workers, or briefing two small stubs per worker instead of five, is the next thing to try.
- New findings worth a brief line: `include/psyq/inline_c.h`'s `gte_ldlvl`/`gte_stlvl_gpr` reproduce the GPF stubs in `system/temp2` exactly (`func_80025D4C` at 87.89%, right instruction multiset, 604 bytes) - the file's old comment blaming those on hand-written asm was wrong and is corrected; and battle/misc8's `func_8007C1A4` (83.91%) is stuck on the same `D_800C3EB4` single-use lo_sum hoist as `func_8007C840` (82.94%), so those two should be briefed together with one idiom.
- `battle/main/misc34` `func_800BF4F0` has timed out in two separate rounds: do not brief it again without splitting the unit.
| 54 | 11 matching + 4 rewrite workers (launched by a session that died; this one adopted the round after its children exited) | +1 | 2603 | `func_800AF678` -> BattleModelQueueTransformCommand (battle/misc25). Only 2 of 11 clones had committed when the round was orphaned; the other nine had ungated bodies. Two thirds salvaged by checking their diffs here: `battle/misc19`'s BattleRow rewrite merged as committed, and `battling/misc18`'s break-threshold rewrite was committed after verifying its three functions at 100.00 (then three mechanical merge repairs: stale externs with conflicting prototypes, and the missing 0xBE field it used). Dropped, each having broken a previously matched function or failed to match: battle/misc2 (BattleUpdateRotationBuffer 100 -> mismatch), world_map/misc5 (two Group*ScaledPrims 100 -> 93.58), lib_gear/main (func_801E1258 un-park that does not match). raw **2642 -> 2603**. Naming: the 2034-address union plus the one new match landed exactly 1 name, which confirms the pool is exhausted; it refills only as matches land. |

Round 54 detail:
- Orphaned rounds are recoverable at about 15 minutes of orchestration: `git log main..HEAD` per clone finds the commits, and for each dirty clone a `diff_func.sh` on one function it touched tells compile-failure from rewrite-fine in 5 seconds. That test is cheaper than re-briefing the whole unit and it caught all three bad rewrites.
- The un-gated-rewrite failure mode is consistently a *stale declaration*, not bad C: `battling/misc18` had two `extern` prototypes whose parameter types no longer agreed with the definitions the worker wrote. Dedupe externs in any brief that un-parks a park block.
| 55 | 5 unpark-screen + 5 matching workers | +16 | 2605 | **The cdk -G0 preset harvest paid: 12 matches for zero C.** An unpark screen of the five `XenoBattleTail`/`XenoBattleTailCdk` units showed every already-matched function staying 100.00 under `gcc-2.7.2-cdk-psx --gp -G0`, and their parks written under the old compiler reaching 100.00 under it: `battle/misc26` 5 (`func_800B16A4`, and `func_800B3588/383C/397C/3C2C` from parks of 99.29/99.33/69.65/90.83), `battle/misc31` 4 (`func_800BB844/BB690/BB7F8/BCB54`; 3 of them 41.79-86.08 under 2.7.2), `battle/misc29` 3 (`func_800B7C28/B8048/B8D04`, 45.00/45.00/81.63 before). All three units moved to `XenoBattleTailCdk0`, the now-empty `XenoBattleTailCdk` misc29-only entry was deleted, two `make objdiff-config` runs agree and `make check` is green. The other 4 are written C: `func_800BE6A0` + `func_800BF2B8` (battle/misc33), `func_8008151C` (battling/misc8), `func_8007EE08` (battling/misc6, from a 95.83% park via the permuter). 4 names applied. `battle/misc27` and `misc27_b` have no harvestable parks (identical under -G0, C-shape walls). raw 2603 -> 2605: the round's park notes only, owed back by a rewrite on `battle/misc33` and `battling/misc8`. |

Round 55 detail:
- The preset-harvest queue from PROMPT.md is now empty: all five units are screened, three switched, two proved to be C-shape. Do not re-screen the `battle` tail.
- The un-parked bodies come with their own `extern`s and the stale ones bite on the first `make check`: `battling/misc18` (round 54) and none here, but the misc26/misc31 merge needed no repair at all because the screen workers ran `diff_func.sh` on every un-parked body before committing.
- Two new permuter blockers, both worth a brief line: `permute.sh` refuses a park whose `/* ... */` block starts with a typedef instead of `void Func(`, so `battling/misc12` `func_80088A40` and `menu/misc11_r2`'s 95-99% parks (`func_801E6668`, `func_801E5178`) are unreachable by it until the body is un-parked live first.
- `battle/misc33` is ROUGH: `Misc33Point` and `BattleMisc33Actor` describe the same sprite object and should become one struct (3 matched bodies affected). `func_800BED4C` there is a real wall (one dead reload of `D_800C3610` after a call) and must stay a stub.
| 56 | 8 matching/rewrite + 1 permuter-apply worker | +4 | 2596 | `func_80073380` -> BattleRenderActorValueDigits (battle/misc1, the sweep's first ZERO: the 99.82% park was not the `arg0 * 484` temp but splitting the pointer base into two statements so gcc emits retail's `addu v1,v1,v0` order), `func_801DBDB4` -> MenuFitInventoryWindowToItems (menu/misc9), `func_800BABDC` -> BattleFreeActorAnimTask + `func_800BA4E0` -> BattleLoadActorSlotAnimation (battle/misc30, both via the permuter on 89% bodies). Rewrite: `battle/misc33`'s two sprite structs merged into one `BattleActorSprite` (0xB4, named union at 0xAC/0xAF) with every matched body converted. 4 names. raw 2605 -> 2596. |

Round 56 detail:
- **The jump-table stub class is smaller than the "about 110 stubs" note says, and the reason is layout, not C.** Five workers hit it and none converted a switch: `battle/misc23` `func_800A6884` compiles instruction-identical (99.74%) into 612 bytes but retail's table has a **sixth, trailing 0 word** that a `switch` never emits, so the overlay loses 4 bytes (`convertible.py` BLOCKED is right); `world_map/misc12`'s `func_80090C68`/`func_80090E14` are instruction-identical at 99.91/99.90% but gcc aligns the C table to 8 while retail packs it 4-byte aligned, and because the tables before them belong to BLOCKED stubs the offset cannot be fixed for one function. Before briefing a jtbl stub: count the `.s` table words against the case count, and check the table's alignment against its neighbours. Both are pre-conversion checks; neither shows up in objdiff.
- The permuter is doing real work now (3 of this round's 4 matches came from it, one from a sweep ZERO and two from `PERMUTER_BETTER` leads), but it has three reachability holes, all now hit: a park whose `/* */` block starts with a typedef, a unit whose other parks break `permute.py`'s prototype extraction (`world_map/misc11`), and a park whose prose note sits in a separate block from its body (`battling/misc5` — fixed locally by putting note and body in one block next to the `INCLUDE_ASM`).
- `menu/misc9` `func_801DBDB4` is ROUGH in a new way: gcc 2.6.0 count-down-transforms an ascending `slti/bnez` loop, and the only source lever found to stop it is a zero-statement `do { } while (0);` block at the end of the body. Not acceptable as final form; a later rewrite needs another construct that emits an empty trailing block.
| 57 | 6 matching + 2 permuter-apply workers | +6 | 2635 | `func_80093E8C` -> WorldMapGetTileValue (world_map/misc13, the sweep's second ZERO: the 99.43% tie broke by folding the 0x510 base offset into a local), `func_800B8840` -> BattleInitStateWorkBuffers + `func_800B8DA4` -> BattleActorInitSpriteFromSlot (battle/misc29, first round under its new cdk -G0 preset, the second via the permuter), `func_80082BB0` -> BattleSetActorSlotMenuMode + `func_800829F4` -> BattleSetActorSlotDisplayMode (battle/misc12, both MATCH-on-jtbl + CHECK_BIN_OK), `func_800855C8` -> FieldPlaySoundElementPair (field/misc8, the smallest stub in the pool, permuter). 6 names. Permuter: `FieldScriptMemoryWriteU16` improved 97.14 -> 98.57 and re-parked. raw 2596 -> 2635 (+39: new park prose in battle/misc11 and battle/misc12 plus two ROUGH bodies), owed back in round 58. |

Round 57 detail:
- **The jtbl class is now settled as two distinct blockers, both layout.** `JTBL_LAYOUT` (gcc 2.7.2 hardcodes `.align 3` for a jump table, so any table retail packed 4-byte aligned grows the unit by 4 bytes: `battle/misc11`'s tables are at 0x194/0x1B4/0x1D4/0x1F4/0x214/0x234/0x254, only 0x0 and 0x214 are 8-aligned, so that unit's whole switch family is a single all-or-nothing job) and `JTBL_PAD` (retail's table carries a trailing 0 word C never emits, `battle/misc23`). A jtbl stub that converts cleanly does so because its table happens to land 8-aligned - `battle/misc12` got two that way - so **brief jtbl stubs one unit at a time and expect the conversion set, not the function, to decide**.
- `convertible.py` over-reports BLOCKED for jtbl stubs whose `.s` merely opens with the table (it counts the table's words as "insns after .size"): `battle/misc30`'s `func_800B9284`/`func_800B9C78` are ordinary stubs by that reading. Treat BLOCKED on a jtbl stub as a hint and check the `.size` line by hand.
- gcc 2.6.0 emits switch case bodies in **source order**, so in a `XenoBattle260*` unit the source case order must equal retail's body order (`battle/misc12` is 4/6/7, 0, 2, 3, 1). In brief.js for the 2.6.0 presets.
- ROUGH owed: `battle/misc12` (both matches address `D_800C3EAC` slots with raw byte arithmetic - they want a 0x40-byte slot struct), `field/misc8` `func_800855C8` (two different expression trees are load-bearing to stop a CSE), `menu/misc9` `func_801DBDB4` (`do { } while (0)` to block a countdown transform).
| 58 | 9 permuter-apply + 1 rewrite worker | +13 | 2625 | **Every ZERO in the sweep report landed: 13 of 13.** `WorldMapSetupPrimTemplates` + `WorldMapUpdateGroup4And5Scale` + `WorldMapUpdateGroup6And7Scale` (world_map/misc4, misc5 x2), `FieldScriptVMHandlerSwitchPartyLeader` (field/misc6), `MenuUploadStringTextures` (menu/misc12), `MenuIsSlotUsable` (menu/misc3), `BattleSpriteProjectToScreen` (battle/misc30), `BattleAllocateArchiveFilePair` (battle/misc23), `BattleSetPrimFlagsTwoSlots`/`ThreeSlots`/`FourSlots` (battle/misc15 x3), `BattleDrawArcOutline` + `BattleDrawHexagonOutline` (battling/misc9 x2). 13 names. Rewrite: `battle/misc12`'s two slot-switch matches now go through a declared `BattleActorSlots`/0x40-slot struct instead of raw byte arithmetic. raw 2635 -> 2625. `make check` green on all eleven. |

Round 58 detail:
- The port-and-gate shape is now the cheapest round in the project: one worker per unit, `permute-brief.js`, no C written from scratch, and every one of the 13 closed on the first port. Budget for it whenever `permute_sweep.py --report` lists results.
- What the ZEROs were, as a class: **all 13 were 99.1-99.9% parks where an allocator tie, not a structure, was wrong** - the edits that closed them were a folded constant into a local (`world_map/misc13` 0x510), a hoisted shift (`battle/misc29`), an alias chain, and a two-statement pointer base. None needed a new type or a new loop shape. That is the argument for running the permuter before parking anything above ~99.
- Still open from the sweep: 3 BETTER leads (`battle/misc18` func_800968C0 40->10, `world_map/misc9` func_80087904 45->30, `battle/misc8` func_8007CDD0 40->30) and 410 bodies not yet tried in their current form - the sweep keeps producing ZEROs as it goes, so re-read `--report` at the start of every round rather than at the end.
| 59 | 7 permuter-apply + 3 matching workers | +7 | 2626 | Permuter ports: `BattleSlotStoreWord0` (battle/misc7), `BattleAcquireSoundSlot` (battling/misc17), `BattleRollPercentAndSwapActiveActor` (battle/misc18, BETTER), `BattleAppendEntry` (battling/misc11, BETTER), `WorldMapSetSlotTPages` (world_map/misc9, BETTER). Written C: `func_80071EF0` + `func_80073448` (world_map/misc1) and `func_800AA7DC` (battle/misc24, a 88.24% park closed by the permuter). 5 of 7 names applied (the two world_map ones have no `symbol_addrs` entry and `name_addrs.py` declines the address). raw 2625 -> 2626. |

Round 59 detail:
- The permuter results keep being **one edit**: `do { ... } while (0);` around a flag store to give gcc an extra basic block (`field/misc6`), a shared local split into three (`battle/misc30`), a hoist of `arg0 + 2` (`battle/misc23`), a dead `if (idx) return result;` to keep a variable live (`menu/misc3`), an inline shift instead of a local (world_map/misc5 x2). Port-then-tidy in that order: every one of these read worse than the park until it was verified 100.00%, and the tidy was always "rename the temp / drop the copy", never "find a nicer shape".
- `battle/misc12` is CLEAN now (round 58's rewrite): `BattleActorSlot` (0x40) and `BattleActorSlots` (0x2F8) are local typedefs and both slot-switch matches stayed at MATCH/jtbl with `check_bin` OK. The struct lives in the .c on purpose - retyping `D_800C3EAC` itself would cost 7 instructions in these two because a function-scope typed pointer lands in `$s0` across a `jal`.
- BETTER leads do not survive a port as-is: three of four needed a re-run, and two of those re-runs found nothing (score stayed flat), so a BETTER port is worth ~15 minutes and no more.
| 60 | 7 permuter-apply + 3 matching workers | +6 | 2632 | Permuter ports, all first-try: `SoundSpuMemoryAllocateBlockFit` (slus_006.64/system/sound - the unit's first ZERO after rounds of 79-96% dead band), `DrawMode` (psyq/libgpu_b, SDK), `FieldGetVec2FromPolar` (field/misc4), `WorldMapFindZoneAtPosition` (world_map/misc11, the unit round 56 could not even permute). Written C: `func_800BE1C4` (battle/misc33, MATCH RELOC 99.89% on a typed-global addend) and one more in battle/misc28. 4 names. raw 2626 -> 2632. |

Round 60 detail:
- The sweep is now deep enough that it is finding ZEROs in units this project had written off: `system/sound` (2.6.0) and `psyq/libgpu_b` both closed without any C being written, which are the two areas PROMPT listed as "match by finding the library version" and "a dead band". **Run the sweep over any unit before declaring it a dead band** - a 98.9% park that the permuter closes in one edit looks identical to a wall from the outside.
- 3 of the 10 clones committed nothing at all (slus_006.64/system/archive, menu/misc3, and one permute unit): both fresh-stub lists there were large stubs (184-280 insns) and the workers ran out of budget reading rather than writing. Fresh-stub *count* is not the predictor - **average instructions is**: the rounds that land 6+ take units whose median stub is under ~100.
| 61 | 3 permuter-apply + 6 matching workers | +12 | 2631 | Best round of the session. Permuter: `BattleUpdatePartPositions` (battle/misc22, 88.24% park), `BattleAcquireSoundSlot` (battling/misc17), `BattleGetActorActionConditionMask` (battle/misc19), `GfxSetRenderContext` (system/temp2 - a GTE unit), `BattleBuildGrey80BlendSpriteColumn2`/`Column5` (battle/misc14). Written: `BattleAdvanceSlotMenuState` + `Random` + `Text` + `BattleSelectTargetSlotLoop` (battle/misc12, 4 from one unit), `WorldMapUpdateSlotRotationMatrices` + `WorldMapSetSceneLayerRotMatrices` (world_map/misc9). 10 names. raw 2632 -> 2631. |

Round 61 detail:
- Briefing by **median stub size under ~100** is what produced 12: `battle/misc12` gave 4 matches from 5 stubs assigned (122-138 insns) because the unit was already half-matched and its `BattleActorSlot` typedef existed. The two units where this rule was ignored (`slus_006.64/system/archive`, `menu/misc3` in round 60) gave zero.
- `field/main/misc2` is confirmed unwinnable as a per-function job and should be dropped from the pool: **every stub under 103 instructions is rodata-BLOCKED** (each owns a `D_8006FB*` string) and all 10 convertible ones start at 103 and carry the same `$s0` base-across-a-call wall. The worker moved `func_8007254C` 62.01 -> 95.08 by taking a local pointer's address (`pCamRef = &pCam`) to defeat constant folding - useful trick, recorded in the park - but Retail has no spill slot, so it is structural.
- `world_map/main/misc1` is the cleanest proof yet of the jtbl ordering wall: its whole `.rodata` range is 19 words spanning three tables owned by three stubs, C `.rodata` lands *before* asm-owned `.rodata` in the same object, and one of the tables has 6 words for a 5-case switch. Only a whole-family in-order conversion of all three could work and the pad word makes even that 4 bytes short. Do not brief its jtbl stubs.
- Two reusable shapes from this round, both now the cheapest thing to try on a 98-99.x% park: split a base pointer into two statements (`pBase = D_8009A988; p = pBase + i;`, round 59-61 hit this three times), and give a parameter its own live copy to move a prologue save pair (`WorldMapSetSlotTPages`).
| 62 | 10 permuter-apply workers (one unit each, no written C) | +10 | 2632 | All ten ZEROs ported on the first attempt: `FieldCameraUpdateFromTarget` (field/misc2), `FieldScriptWritePartyMemberPosition` (field/misc6), `BattleDrawFrameHud` + `BattleActorAdvanceMove` (battling/misc3), `BattleDrawTextSprites` (battling/misc8), `func_800828F8` (battling/misc9), `BattleEffectAllocEntries` (battling/misc16), `MenuCreateUnk430Panel` (menu/misc9), `func_8003D53C` (slus_006.64/system/sound), `func_8007BA44` (battle/misc7). 6 names. `battle/misc25` `func_800B136C` re-parked at its BETTER score (110 -> 70), not a match. raw 2631 -> 2632. |

Round 62 detail:
- **`func_8003D53C` in `slus_006.64/system/sound` matched under the unit's own 2.6.0 preset**, which retires round 51's DECISION line asking for a per-file preset split there: the 95.83% was a parkable allocator tie, not a compiler mismatch. No `gears.toml` change was needed and none was made.
- `field/main/misc2` is no longer a dead end either - `func_80072A38` was one of its 10 convertible stubs and closed from a ZERO, so the unit's real filter is `convertible.py` (10 of 16 stubs) and not its stub count.
- Ten permuter ports in 12 minutes with zero rework is the best tokens-per-match shape measured: no C written, no design decisions, one file per worker, and `permute-brief.js` carries the whole playbook.
| 63 | 4 permuter-apply + 4 matching workers | +3 | 2634 | `WorldMapLoadTextureClut` (world_map/misc8, permuter ZERO), `WorldMapUpdateSceneLayerRotationMatrices` (world_map/misc9), `SoundHeapFree` (slus_006.64/system/sound, written C). Two BETTER ports re-parked at their new best instead of matching (`func_8007CDD0` 99.02 -> 99.21, `func_8007BA04` 95.29 -> 98.12, `TransposeMatrix` unchanged). 2 names. raw 2632 -> 2634. |

Round 63 detail:
- **Orchestrator error worth recording: I briefed `src/slus_006.64/system/sound.c` to two workers in one round** (a permuter port and a matching worker) and the merge conflicted. Resolved by taking the matching worker's version of `SoundHeapFree` (a 100.00% match) over the permuter port (a park), which is the rule: a real match outranks a BETTER park on the same function. One `.c` per round, even when the two workers would touch different functions.
- `menu/main/misc3` and `slus_006.64/system/archive` gave 0 again (both now at two consecutive rounds of ~0) - stop briefing them. The archive worker also found `permute.sh` broken for that unit (its generated `base.c` is 1110 lines but cc1 sees a ~1292-line file), so "permuter first" advice is worthless there until that is fixed.
- `world_map/main/misc11`'s lesson from the permuter: a semantically DEAD duplicate branch arm (`else if (pEntry->unk14 || pEntry->unk00)` with the same body) was the whole fix for a 99.37% tie, and an unread `s32 tmp[4]` is load-bearing for the frame size. Dead code as an allocator lever is new; it is worth trying on a 99.x% park before a fourth permuter run.
| 64 | 5 permuter-apply + 5 matching workers | +7 | 2635 | `WorldMapInitRandomTimers` (world_map/misc2, permuter ZERO at a 94.80% park), `BattleUpdateSlotGaugeDisplay` (battling/misc3), `SoundSetElementVolumeByIndex` (slus_006.64/system/sound), `func_800BDC78` (battle/misc32), `BattleCreateNumberSpriteTask` + `BattleAttachActorAnimScripts` (battle/misc33), `BattleBuildGrey80BlendSpriteColumn2`/`Column5` (battle/misc14, via the round-63 leads). 5 names. **battle/misc12's three new jtbl conversions were reverted** (below). raw 2634 -> 2635. |

Round 64 detail:
- **`battle/misc12` is the worked example of the all-or-nothing jtbl rule.** A worker matched `func_80082D4C`, `func_80083340`, `func_80085084` (all MATCH-on-jtbl, instruction-identical) and its own `check_bin.sh battle` passed, but the merged tree failed: I bisected (file at each of the worker's two commits, then fully reverted) and only the fully reverted file links. The unit already has four converted jtbl functions from round 61 whose tables are 8-aligned; three more tables change the alignment chain for every table after them. `check_bin` in a worker's own tree does not catch this when other units of the same overlay changed in earlier rounds - the orchestrator's `make check` is the only gate. Bodies preserved in git (`e5e8558c`, `73b1d36f`) for a future whole-family conversion.
- `world_map/main/misc6`'s worker proved a scheduling wall precisely: `func_80080578` compiles to retail's exact instruction multiset at retail's size with the three `D_8009C5xx` loads in a different order, and `-fno-schedule-insns2` reproduces retail's whole tail. A per-file `-fno-schedule-insns2` preset is now the one preset axis never screened - but that worker also measured that the same flag drops six already-matched functions in that file, so it is not a free switch.
| 65 | 8 permuter-apply + 2 matching workers | +8 | 2637 | All four ZEROs plus four BETTER-closers: `BattleApplyNodeOffset` (battle/misc25, 98.68% park), `BattleDrawCharStatusIfFlagged` (battle/misc17), `FieldScriptWritePartySpriteFrame` (field/misc), `FieldScriptVMHandlerWriteActorRotationByIndex` (field/misc7), `BattleFreeActorEntryAndChildren` (battle/misc24, 97.43 -> 20 -> match), `WorldMapBuildPolyFT4Templates`/`WorldMapInitSlotActor`/`WorldMapInitializeModeWithAudioReload` (world_map/misc6, 89-95% parks). 7 names. raw 2635 -> 2637. |

Round 65 detail:
- The BETTER-to-match conversion rate is now measurable: of 8 BETTER leads worked across rounds 63-65, 4 closed (50%) and the rest re-parked 1-2% higher. Both of the "far off" ones (score 90-120) stayed parked, so **a BETTER lead is worth briefing only at score <= 60**; anything above that has been a 30-minute loss every time.
- The permuter sweep has also started closing units this orchestrator had marked as walls two rounds earlier (`world_map/misc6` was "sched2 blocked" in round 64; three of its parks closed in round 65 from ZEROs). Re-read `--report` before believing any "do not brief" note.
| 66 | 2 permuter-apply + 4 matching workers | +8 | 2641 | `SoundStopElementsByIndex` + `SoundFadeElementVolumeByIndex` + `SoundApplyPendingKeyOff` (slus_006.64/system/sound, three of its six smallest stubs, from parks at 83-92%), `BattleDrawLineQuads` (battling/misc6), `BattleActivateTwoSlotCommand` (battle/misc15), `BattleInitActiveSlotAction` (battle/misc17, permuter BETTER 170 -> 20 -> match), `func_8003E1F8` (sound), and one more in battle/misc3. `_SN_read` (psyq/libsn) did not close. 6 names. raw 2637 -> 2641. |

Round 66 detail:
- `system/sound` yielded three matches from parks the unit had carried for 15+ rounds, all by the same class: callee-saved register mirror + a delay-slot placement. It is now the highest-yield unit left in the tree (15 fresh stubs when this round started) and no longer a dead band - **the "79-96% dead band" label on it cost about ten rounds of progress**.
- Verdicts worth never re-opening, both proven with instruction-form evidence rather than a percentage: `slus_006.64/system/temp2` `func_80026F44` is hand-written Psy-Q Lite asm (it contains `addi $a1,$zero,0x20` and `bne $zero,$t2`, forms that appear zero times in the 946 matched `.s` files, i.e. no gcc preset emits them) and `func_80025C04` drives the scratchpad as absolute immediates `D_1F800000..1C` that no `.c` can name.
- The reusable lever of the round is again the delay slot: `for (pEntry = pCurrent, pPrev = NULL; ...)` instead of two statements put retail's `addu v0,s0` before the `beq` (SoundHeapFree 96.62 -> 100). Combined with the non-void return type from round 65, "move a copy/increment across the branch by changing the init form or the return type" is now the standard first move on a 99% park.

Session note (rounds 54-66, one orchestrator, ~100 matches):
- **Stubs 1276 -> 1176**, raw 2603 -> 2641, `make check` green on all eleven at every commit, `progress-publish` run three times. 73 names applied.
- ORCHESTRATOR MISTAKE to remember: at the end of this session I deleted the stale `~/xw/r-m53-*` and `~/xw/r-m54-*` clones after seeing commits in `git log main..HEAD`. In a clone made in an *earlier* session that ref is the frozen old `main`, so the count is a false positive - but if any of those branches really were unmerged, their commits are gone with the clones. Rule: only delete clones whose `main` ref is the current one, or check ancestry against the orchestrator's main rather than the clone's.
- The permuter sweep produced **31 ZEROs and 30+ BETTER leads over the session, and 31 of 31 ZEROs ported to matches on the first attempt**. That is the single highest-yield source of matches in the project now; the sweep takes ~20 minutes per body on a loaded VM and had 300 of ~460 parked bodies done at the end.
- Largest single-round win: **round 55's cdk -G0 preset harvest, +12 for zero C written**, which also emptied PROMPT.md's preset queue. Second: round 58, all 13 sweep ZEROs ported.
- Units that turned out to be wrongly labelled dead bands and now yield: `slus_006.64/system/sound` (4 matches in the session), `field/main/misc2` (2), `battle/main/misc30` (3), `world_map/main/misc6` (3). Units two consecutive rounds of ~0 confirmed for the next session: `slus_006.64/system/archive`, `menu/main/misc3`, `battle/main/misc11`, `world_map/main/misc12`.
| 67 | 2 permuter-apply workers (the last ZEROs in the sweep) | +2 | 2640 | `MovieUpdateStream` (movie/misc1, a 99.78% park) and `BattleScriptResetActorState` (battle/misc4, 98.71%). 2 names. Only BETTER leads above score 60 are left in the sweep (84-89), which rounds 63-66 measured as not worth a worker, so the port-and-gate queue is empty until the sweep covers the remaining ~160 parked bodies. |
| 68 | 5 rewrite workers (the worst five raw files) | +0 | 2521 | No new matches, by design: this round paid back the session's raw-line debt. `battle/misc24` (139 -> typed command-queue/part-table structs), `menu/misc7` (129), `world_map/misc5` (111, one 0x54-byte scene-layer struct used by all five matched bodies), `battling/misc2` (103), `battle/misc8` (87). raw **2640 -> 2521** (-119) with every match still at MATCH+100.00% and `make check` green on all eleven. |

Round 68 detail:
- A rewrite round on the five worst raw files is the cheapest -119 raw lines the project has had; it needs no new C and no design decisions, and the `REWRITE <what the struct is>` line in `extra` plus a grep of the file is a complete brief. Give the worker the grep itself rather than a function list - the raw sites are the definition of the work.
- `world_map/misc5`'s struct is the one to reuse: a 0x54-byte scene-layer entry at `D_8009C620` (`MATRIX` +0x20, halfword +0x1A, angles +0x50/+0x54) that `WorldMapSetSlotTPages`, `WorldMapSetSceneLayerRotMatrices`, `WorldMapUpdateSlotRotationMatrices` and the group setup/update pairs all index. The next rewrite pass on `world_map` should extend it rather than declare a second one.
- Worth remembering from round 68's matching side: `battle/misc12`'s three jtbl conversions (`func_80082D4C`, `func_80083340`, `func_80085084`) are preserved in git and are all instruction-identical; they fail only on the unit's table-alignment chain, so the whole-family conversion (all 7 tables in one address-ordered batch) is the only path.

Handover (end of the rounds 54-68 session). The sweep is still running in `~/xw/r-cl-perm` (278 of ~460 parked bodies scored) and `permute_sweep.py --report` already lists three unported ZEROs - `menu/main/misc10` func_801E35BC, plus two more below the fold. Start the next session with a port-and-gate round over them (`clone.sh new` one clone per unit, `permute-brief.js`, one worker per unit) exactly as rounds 58/62/65/67 did; every ZERO this session ported on the first attempt.
| 69 | 3 permuter-apply workers (the last three ZEROs) | +3 | 2640 | `MenuApplyDressingRoomSlotHpBonus` (menu/misc10, 99.87% park), `BattleTryQueueMessage5` (battling/misc3), `BattleWordRegSetReg` (battle/misc7). 3 names. raw 2640 -> 2640 (2637 at the rewrite round's end plus park prose; the round's own raw count is unchanged). |

Round 69 detail: the port-and-gate round is now a 4-minute, 3-worker operation end to end (launch to green `make check`), and it closed the last ZEROs the sweep had produced. `permute_sweep.py --report` is the first command of every round from now on.

Pending at handover: the sweep's ZERO queue is now **empty** (0 ZEROs). One BETTER lead qualifies under the score<=60 rule and is worth one worker: `battle/main/misc7` `func_8007B8D4` (parked 95.94%, score 230 -> 40) - `permute_sweep.py --report --briefs` prints its worker args. Everything else listed is score 70+ and has never closed. The sweep is still running with ~180 parked bodies unscored; leave it running and re-read the report at the start of the next session.
| 70 | 1 permuter-apply worker (the last qualifying BETTER lead) | +0 | 2640 | `battle/main/misc7` `func_8007B8D4` (230 -> 40) and `func_8007B9C8` (230 -> 80) both re-parked with better bodies, neither closed - the first was the only lead at score <= 60 and it failed, which makes 5 of 13 BETTER leads closed overall. Nothing else in the sweep qualifies, so the port-and-gate queue is empty until more bodies are scored. |

Round 70 detail: **the permuter queue is now empty** - no ZEROs and no BETTER lead under score 60. Remaining sweep work is scoring the ~180 untried parked bodies, which needs no orchestrator attention. Next session's first move: read `--report`; if it lists ZEROs run a port round, otherwise brief fresh stubs by median stub size (under ~100) and run a rewrite round on the worst raw files - those two are the only dependable sources left.
| 71 | 3 permuter-apply + 5 matching + 2 rewrite + 6 naming workers | +10 | 2468 | Six from one unit: `BattleFreeAnimTask`/`BattleAnimTaskTick`/`BattleInitAnimTask` (battle/misc31, 78-102 insns, all four others in the file too), `BattleActorSelectEdgeForSegment` (battling/misc3), `BattleSetCommandCodeForFlagBit` (battle/misc20), `WorldMapInitSpriteSets` (world_map/misc8), `GpuSendWordsByCpu` (psyq/libgpu_b, permuter ZERO). Two permuter leads improved past 98% but stayed parked: `func_80085D34` 95.68 -> 99.63 and `func_8009BD94` 86.33 -> 98.33. 34 names. raw 2520 -> 2468. |

Round 71 detail:
- **`fresh_stubs.py` was over-reporting fresh stubs by 247** (433 in 111 units -> 186 in 76) because a park is written directly BELOW its stub and the tool only scanned the 60 lines above. Fixed in the tool and recorded in PROMPT.md. Round 71's brief was built on the corrected list and every worker got a stub with no percentage anywhere near it.
- **The `volatile` lever is new and closed a function outright**: `psyq/libgpu_b` `func_8004659C` went 81.75 -> 100.00 from `volatile` on two shared pointer externs (`D_800569A0`/`A4`) - the body was already right, volatile stopped gcc reordering the GP0 stores around the loop branch so reorg could fill the delay slots. Both of the round's improved-but-unclosed leads want the same class of edit (explicitly blocking CSE so a load re-materialises where retail has one).
- `battle/misc18` rewrite took 55 -> 11 raw code lines with all 10 matched functions still at 100.00% (a `BattleWorkBlock` for `D_800C34B0`, offset-commented fields on `BattleTableEntry`/`BattleRow`). Two functions stay ROUGH with measured reasons: the typed `&D_800C34B0->slots[sel]` gives the index-first `addu` where retail has base-first, and keeping the pointer live across `rand()` grows the frame 24 -> 32.
- `world_map/misc5` rewrite: 2 CLEAN, 5 ROUGH. The typed variant of `WorldMapUpdateGroup4And5Scale` costs 6 instructions (retail has a `nop` in the `beqz` delay slot and sinks the `sra`), so the raw body stays. Reuse `WorldMapSceneLayer` (0x54, already in the file) and `WorldMapPlayer` (0x80) - the table base is `D_8009BE24`.
- **Grep raw counts before briefing a rewrite**: `menu/main/misc7` still tops the grep at 120 lines but 120 of them are inside park comments, i.e. no work. Counting only code lines outside `/* */` moved the real worst files to world_map/misc5 (95) and battle/misc18 (55). The comment/code split is one python one-liner and is now the first step of picking a rewrite target.
- `slus_006.64/system/sound` was the only matching unit to give 0: its first assigned stub (`func_8003B424`) is a ~60-store element initialiser that ate the whole budget (parked at 84.32%). Next round should start with `func_8003E900`, which that worker fully decoded (g_SoundChannels[0..23] -> SPU_VOICE_REG, the only awkward spot being the `lbu` of the low byte of `adsr[0]`).
- `world_map/misc8` `func_800848F4` is splat-marked `/* Handwritten function */` (GTE + scratchpad) - never brief it.
- `battle/main/misc31` shows the cdk -G0 preset's yield again: 6 functions for one worker, and 4 of the 6 were ROUGH only for an untyped work-list entry, which a later rewrite round can finish.
| 72 | 7 permuter-apply + 1 matching + 4 naming workers | +7 | 2481 | All seven ZEROs ported on the first attempt, every one at 100.00%: `SoundApplyTriangleHalfRateEnvelope` (sound, `func_8003DF78`), `FieldScriptCopyCharacter` (field/misc), `WorldMapResetActorSlotWithSound` (world_map/misc4), `BattleReleaseCharSlot` (battle/misc13), `BattleQueueMessagesAndSetScaledDelay` (battling/misc18), `BattleScriptUploadSpriteImage` (battle/misc27), `BattleApplyEffectSpriteColorRamp` (battle/misc32). 27 names. raw 2468 -> 2481 (owes a rewrite round on those three files). |

Round 72 detail:
- **The `do { ... } while (0);` block is a scheduling lever, not just an idiom**: it closed two of the seven ports by forcing a store cluster to schedule before unrelated global stores (`world_map/misc4` 70.00 -> 100, `battle/misc13` 92.28 -> 100 with a `do/while` wrapped around one call). Where a 90%+ park differs only in the order of two store clusters, try wrapping the smaller cluster in a do/while block before the permuter.
- Port-round CPU budget: 7 port workers each run the permuter at `-j 2` on a 6-core VM, and with the sweep also running the load hit 22 and several workers sat in a single compile for 4+ minutes. Cap a port round at 5 workers (or pause the sweep) - the round still closed 7/7, but only because a port is short work.
- `battle/misc30` was 0 and the cause was a **briefing filter**, not the unit: both stubs I passed are jtbl BLOCKED and the fresh-stub list cannot see that (it does not run convertible.py). The unit's real pool is `func_800BB080` (47) and `func_800BAF48` (78). Also recorded there: misc30's four jump tables are a **prefix chain** (`func_800B9284` -> `B9508` -> `B9C78` -> `B9F78`), so only a prefix of that list can be converted at a time.
- Two more merged-locals/statement-order wins worth reusing: `sound`'s park note claimed the velocity-square deltas were a wall, and deleting the separate `s32 volume` local (reusing the `s16 velocity` one) fixed both; `battle/misc27` matched by moving one store ahead of two others, the exact inverse of its own failed note.
| 73 | 3 rewrite + 3 matching + 4 naming workers | +2 | 2405 | `BattleFinishActorAction` (battle/misc30, `func_800BAF48`) and `SoundScriptSetTriangleEnvelope0` (sound, `func_8003D8B8`). Three rewrite rounds closed their targeted files: battle/misc32 (11 raw casts, two effect structs), battle/misc27 (`SpriteData`/`SpriteFrameData` from include/field/actor.h), battling/misc18 (all 19 live functions typed). raw 2481 -> 2405 (**-76**, the session's best single-round drop). 21 names. |

Round 73 detail:
- `sound`'s `func_8003D8B8` closed exactly where round 72's port said to look (carry the volume in the `s16 velocity` local) **plus one more merge**: the note and its pitch product must be the *same* `s32` local (`s32 note = (s8)pScript[1]; note = note * -note;`), which puts the load, both `mult`s and the `mflo` in `$a0`; a separate `s32 pitch` sits at 99.41%. The `(s8)` cast is load-bearing (retail `lb`, not `lbu`). Two sibling parks in that file are the same family.
- `world_map/misc4` `func_80078E2C` 19.97 -> 93.33 confirms the do/while lever generalises but is **layout-sensitive**: the winning shape is an outer do/while around `unk54/unk50` + the two scroll globals with a *nested* do/while around one of them, and the constants outside it; the sibling's flat 4-statement form gives 79.93%. One `li v0,0x40` 4 instructions late is all that is left; two permuter runs (1740 iters from 93.33) found nothing.
- A rewrite worker can complete a whole unit's typing in one round: `battling/misc18` went 54 -> 0 raw lines across 19 functions with every match intact, growing `BattleActor` to 0x166A and adding a `BattleWorkFlags` union. Give a rewrite worker the file's raw grep and the struct it needs, not a function list.
- **Header signedness is now the blocking issue for the SpriteData family** (battle/misc27 ROUGH, and battlefield-wide): `include/field/actor.h` spells the direction-transform entry as `u8 translationX/Y` + `s16 rotationX/Y/Z` while retail loads `lb` (s8) and `lhu` (u16) there, so going through the header's fields loses the match. That is an operator-level header decision, not a worker's, and it recurs in at least two units - a future round should decide one spelling and re-diff the family.
| 74 | 3 permuter-apply + 3 rewrite + 4 naming workers | +2 | 2358 | `func_80093660` (world_map/misc13, 99.43% park) and `BattleActorLoadUnk15CCFlags` (battling/misc4). Three rewrite rounds: battle/misc32's three timer-task bodies onto one 0x8C struct, battle/misc27's direction-transform table + 0xB8 task block, world_map/misc5's player accesses. raw 2405 -> 2358 (**-47**). 7 names. |

Round 74 detail:
- **New lever, generic: overwrite the parameter instead of introducing a local.** `world_map/misc13`'s last register was `sra v0,a0,0xc; andi a0,v0,0x7ff` vs a coalesced `sra a0,a0,0xc`; a fresh local has no hard-register preference so gcc coalesces it with the dying argument register, while a *parameter* keeps its incoming preference. `x = (x >> 12) & 0x7FF;` and using `x` afterwards reproduces retail. Same class as the "give the parameter a live copy" entry in brief.js, in the opposite direction.
- `battle/misc32` proved what the battle timer tasks are: **two embedded `WorkListEntry` nodes (0x1C each) followed by the effect data**, so the "callback at 0xC" is `timerEntry.onFreeCallback` and `WorkListRemoveTask(pEntry + 0x1C)` unlinks the render node. Any unit that writes `pEntry + 0x1C` into a work-list call should start from `WorkListEntry` at offset 0.
- `world_map/misc5` is a **measured** partial: typing the player through `WorldMapPlayer` is byte-exact; typing the layer stores or the `0x1F800000` scratchpad is not (73.27%/62.97%), because retail interleaves load/store per field and does not hoist. The five bodies stay half-typed deliberately - do not re-open them without a new idea.
- Orchestrator slip worth avoiding: I ran `make progress-publish` in the main tree **before** `clone.sh new`, which left `reports/progress.json` modified, so a worker's `git commit -a` swept it onto its branch and that clone merged with a conflict. Publish (or commit the snapshot) either well before cloning or after the merge, never in between.
- DECISION (logged in the round-74 commit): `menu/main/misc11_r2` `func_801E5B88` is 100.00 under gcc 2.6.0 but its neighbour drops to 93.06, so it needs a three-way split - two new files for one function, below the ratio this project accepts. Left parked.
- `psyq/libgpu_c` is a naming dud (every C body there is an internal helper with no claimable SDK name); `battle/misc34` now has zero unnamed definitions. Both should be skipped by future naming rounds.
| 75 | 2 permuter-apply + 3 rewrite + 2 deep matching + 4 naming workers | +2 | 2264 | Both ZEROs ported: `WorldMapInitMapAndActorGroups` (world_map/misc5) and `BattleEffectCreateWork` (battling/misc16). Three rewrite rounds took battle/misc8 and battling/misc15 to **zero** raw lines and battle/misc19 to 12 CLEAN. Two deep-shape 60-min workers gave 0 (both functions were walls, with the exact wall now documented). raw 2358 -> 2264 (**-94**). 2 names. |

Round 75 detail:
- **`lint_sources.py` was blind to the break that ended this round**, and now is not. A worker parked `func_801E36D4` by wrapping a comment from 335 to 495 which also covered the live `MenuComputeCharacterStats` definition at 458; `menu` then failed to link with `undefined reference to MenuComputeCharacterStats` while lint exited 0, because strip_comments() blanks the swallowed definition before the INCLUDE_ASM-vs-definition check runs. New check: a `/* */` block over 200 bytes containing two *different* function definitions is a park that swallowed its neighbour (same name twice is the known duplicated-body case and passes). Verified against the reverted tree (clean) and the broken file (exit 1).
- Two new reusable findings from ports: (1) when a 97%+ park's note blames "the address setup", check whether two unrelated uses should share ONE local - sharing `s32 *pScratch` between a fixed-position store and a later function pointer argument is what flipped `func_8007BF50` to 100, and splitting it into two locals drops it back; (2) a `s16` temp holding a small constant is load-bearing when its removal drops a body from 100 to 93 (`BattleEffectCreateWork`).
- `menu/main/misc10`'s `func_801E3ECC` is **jtbl-pad BLOCKED**: retail's table is 7 cases plus a trailing 0x00000000 word that no C switch emits, so converting it shortens `menu.bin` by 4 bytes (proved with check_bin while live). Do not brief that stub or any unit whose table carries a pad word.
- Rewrite rounds are now the most reliable per-round product: three of them closed 94 raw lines in one round, and `battle/misc8` reached 0 by making the dead `BattleSlotArgs` typedef the parameter type.
| 76 | 2 permuter-apply + 3 rewrite + 3 naming workers | +0 | 2209 | **No matches.** Both leads failed narrowly: lib_gear `func_801E63A8` is a cse `reg_eqv` tie at 99.87% (464 bytes = retail's size, identical instruction multiset), battle/misc7 `func_8007B9C8` is an emission-order problem at 85.33%. Rewrites: `battling/misc17` 42 -> 0 raw across 13 functions, `battle/misc28` two bodies onto `SpriteData`, `battle/misc3` three experiments reverted with exact measurements. raw 2264 -> 2209 (**-55**). 7 names. |

Round 76 detail:
- **Two rounds of ≤2 matches from permuter leads: the ≤60 threshold has stopped paying.** 5 of 13 BETTER leads have ever closed, and the two that qualified this round both sit on a compiler tie-break rather than a body error. The sweep's remaining work is what stays NONE for another, longer pass (`--minutes 10 --retry --max-base 120`), which is the next CPU-only experiment.
- **Never restate a sweep number as fact in a brief.** The `--briefs` line said `func_8007B9C8` "park 98.82%"; the file's own note says 85.33% (98.82% belongs to a different function in the same unit) and the worker spent its round discovering that. Quote the sweep's line verbatim and tell the worker the percentage may be stale.
- `lib_gear`'s lead documents a genuine new trick: **a second block around one store changes cse's basic-block view** even when jump-opt deletes the block again, which moved four type tests onto the load register (99.18 -> 99.87). Its remaining delta is three register names with no size or opcode change - permuter-clean, so leave it parked.
- `battle/misc3`'s reverted experiments are worth keeping as measurements: the cursor-arrow buffer IS eight `POLY_G3` (stride 0x1C) plus a 0xC tail, but typing it flips every address `addu` from retail's index-first to base-first (94.69%, then 97.82% with an index-first spelling), so `(i*2+j)*0x1C + (u8*)D_800C3E24` is retail's own expression and the raw form stays.
- Naming now needs a real comment scanner to pick targets: a `/*.*?*/` regex stops at the first `*/`, so a park containing an inner comment leaks its definitions and looks like 5 unnamed functions. With a proper scanner this round's three files gave 7 names and no duds.
| 77 | 3 rewrite + 3 naming workers | +0 | 2129 | Deliberate rewrite round. `battle/misc28` landed the battle-side SpriteData **view** types in `include/battle/actor.h` (16.16 position halves, u16 rotation tri, the 0x3C word) and took five bodies CLEAN at 100.00%; `battling/misc2` typed five bodies and recovered the `Actor+0x5C` two-level chain; `battle/misc17` typed `D_800D2D28`/`D_800C3E34`/`D_800C3E00`/`D_800C3DFC`. raw 2209 -> 2129 (**888 code-only lines**, from 1143 two rounds ago). 6 names. |

Round 77 detail:
- **The SpriteData header question is settled without touching `SpriteData`**: four rounds of workers asked for a signedness change in `include/field/actor.h` (u8 translation vs retail `lb`, s16 vs `lhu` rotation, IVEC3 vs 16.16 int halves). The answer that matched is a **battle-side view struct** in `include/battle/actor.h`, now landed; other units' signed-off matches cannot move because the field/actor header is untouched. Brief later battle work against those view types instead of re-diagnosing.
- `battling/misc2` recovered `Actor+0x5C`: it is a two-level chain, `Actor.unk5C` -> `BattleModelLink.pModel` (+4) -> `BattleModel{count, pParts, pAnims}`, and `BattlePart` is 0x6C (`type` at 0, `pDraw` at 4 with flags at +0/mesh at +0x14, a full `MATRIX` at 0x4C). The non-transformed path reads the matrix's own translation, not a separate position field.
- `BattleEffectSetMatrix` is a measured "typing breaks it" case: six stores must stay `*(s16 *)(p + 0x14) = D_80092A24;` because a byte-pointer store cannot be moved across the `D_8009xxxx` loads, and a `BattleEffect*` base lets gcc hoist all three loads above all three stores (63.89%).
- **Naming rounds are done.** Two of three files had zero unnamed C definitions; every remaining `func_` is a stub or a parked body. From here, names come from the NAME lines of matching rounds only - do not spend workers on dedicated naming rounds unless new C lands.
| 78 | 2 park + 2 rewrite workers | +1 | 2074 | `WorldMapInitActorSlotTimer40` (world_map/misc4) closed the 93.33% park. `battling/misc3` 31 -> **0** raw across 14 functions (one unit-local `BattleActor` 0x166A + message-queue/action/gauge structs), `battle/misc30` 29 -> 8 (battle-side SpriteData view + a 16.16 union). raw 2129 -> 2074. 3 names. |

Round 78 detail:
- **A 93.33% park needed two local-variable levers at once, and the brief named only one.** Reusing the dead `scroll` local put the `li v0,0x40` at retail's position but in `$a1` (92.63%); a SECOND `s32 timer` assigned inside the outer `do { } while (0)` immediately before the next store gave retail's position AND `$v0`. Both the second local and the inner one-statement do/while are load-bearing (the inner one holds `sw zero,80(v1)` out of the delay slot). When a park's note says "one instruction to go", plan for the position and the register to be *separate* levers.
- `battle/misc7`'s `func_8007B9C8` advanced its diagnosis without matching: the load-first header IS reproducible (two shapes found), and what is left is *which pseudo wins the register freed by the dead parameter*, not statement order. Its four neighbouring parks (`func_8007B424` 90.81, `func_8007B6C0` 89.53, `func_8007B7B0` 87.67, `func_8007BA04` 98.12) need the same class of lever, and the matched `BattleWordTableStoreReg`/`BattleSlotStoreDwordFromWordReg` are the base-first reference.
- The long sweep pass (`--minutes 10 --retry --max-base 120`) has scored 291 bodies and produced **no ZEROs yet**, and no result better than the 3-minute pass found. Record: a second, longer pass on what stayed NONE has not paid in its first session; the 3-minute pass's leads are what exist.
| 79 | 1 deep park worker + 1 park + 2 rewrite workers | +1 | 2074 | `BattleByteRegSetMatched` (battle/misc7, `func_8007B424`, 90.81 -> 100.00). The other three parks in that unit advanced their diagnoses (96.17/98.12/88.95) without closing. `battle/misc13`'s 99.63% park is now a **proven canonicalisation wall**. 1 name. raw unchanged at 2074. |

Round 79 detail:
- `func_8007B424`'s lever: **split the base from the slot in the source and evaluate the callee into a `u8` local BEFORE the pointer**, so the `addu` forms after the `jal` where retail has it (90.81 -> 100.00). Same family as the "sum at the point of use" idea.
- **`battle/misc13` `func_80085D34` is closed as a wall** - do not brief it again. Integer/add-cast sums reproduce retail's `addu` operand order but gcc folds 0x9C8 onto the base register (74.25/75.54%); a `volatile` declaration on `D_800C3EA4` fixes the order and mirrors the registers (82.94%); the pointer form is the only one that leaves the offset chain in the dead second-argument register. Register split and operand order cannot be had together from gcc 2.7.2.
- `battling/misc2`'s `BattleEffectSetMatrix` is settled the other way: the six stores must stay byte-pointer stores, because only a `char`/`u8*` store stops gcc 2.7.2 hoisting a global load across it (63.89% typed, 58.50% typed+volatile). Two units now have this exact note - when a matched body interleaves a global load with stores, do not type the stores.
| 80 | 3 deep workers (the three nearly-finished overlays) | +1 | 2072 | **`member_change_menu` is 100% complete - 0 stubs**, the first finished overlay: `MemberChangeMenuSwapCharacters` (115 insns) matched and the binary links byte-identical. `shop_menu` `func_801CE480` re-parked at **99.71%** (size exact), `gear_shop_menu` `func_801D18F8` 92.05 -> **95.81%**. raw 2074 -> 2072. no names (both parks are still stubs). |

Round 80 detail:
- The last stub of `member_change_menu` closed with **three permuter edits together**, each moving gcc 2.6.0's live ranges: `do { } while (0)` around the three initializations, `do { } while (0)` around the first two retry stores, and zeroing `i` before the count init. Two of the three are in the source with comments saying they are load-bearing - **do not tidy them away**.
- `shop_menu` `func_801CE480` is the cleanest outstanding park in the tree: **99.71%, size exact, five identical address-coalescing ties** (retail puts the element address in the base register, gcc coalesces the sum into the scaled index). The permuter went 241 -> 140 -> 80 on it, and the next step is a re-run from the 99.71 body.
- `gear_shop_menu` `func_801D18F8`'s old note was **wrong**: it called the size gap an "instruction count is right" case, but the source sets the RECT up twice, once before each `LoadImage` (two `setRECT`s = 1564 bytes vs retail 1576, one = 1556). Re-run the permuter from the new 95.81% body.
- Unit ownership trap: `gear_shop_menu/main/main.c` has **zero** stubs; the overlay's remaining two live in `main/render.c`. `select_target.py --binary <overlay>` plus a grep is the cheap way to get this right before writing a brief.
| 81 | 2 deep park workers + 1 rewrite worker | +1 | 2039 | `ShopMenuComputeEquipStatDelta` (shop_menu `func_801CE480`) closed from 99.71% (size exact) - the overlay drops to **4 stubs**. `gear_shop_menu` `func_801D18F8` 95.81 -> **98.39%** (one allocno swap left). `battle/misc27_b` nine functions rewritten, all CLEAN. raw 2072 -> 2039. 5 names. |

Round 81 detail:
- **The element-pointer-local lever, now in `brief.js`**: `addu dst,v0,v1 / lbu v0,K(v0)` (yours) vs retail's `addu v1,v0,v1 / lbu v0,K(v1)` on a `table[idx].field` access is closed by `pElem = &table[idx]` and `pElem->field`, one site at a time. It closed 99.71 -> 100.00 on four sites. The old park note said the opposite ("pointer locals cost the item's addu its base register") and the permuter from that same body found nothing in 2168 iterations - a wrong note can outlive the wall it describes.
- `gear_shop_menu`'s last function is now **one allocation tie**: which loop-carried value wins `$fp`. The 95.81% pointer-free body is the structurally exact one (13 loop-carried values, 9 registers, 4 slots, right instruction count); a `RECT*` local makes `&sp20` win `$fp` but adds a pseudo (98.39%). Ruled out: pRect inline, pRect for setRECTs only, for LoadImages only, assignment in four different positions, a ref-count probe.
- Overlay status: `member_change_menu` 0 stubs (complete), `gear_shop_menu` 2, `shop_menu` 4, `movie` 17. The small overlays are the best remaining target: one closed function per round is realistic there, versus 1 in 150 elsewhere.
| 82 | 1 deep park + 1 deep matching + 1 rewrite worker | +1 | 2004 | `MovieStartStreamWithMode` (movie/misc2 `func_800763BC`) at MATCH RELOC, byte-identical. `gear_shop_menu` `func_801D18F8` **98.73% - a confirmed plateau** (frame 200 vs 192: gcc keeps a second pseudo for the digit-loop giv base). `battle/misc3` 13 functions typed + a new `include/battle/battle_work.h`. raw 2039 -> 2004. 2 names. |

Round 82 detail:
- **Collect NAME lines with a tolerant prefix.** Workers write them as `**NAME func_X ...**` about as often as `NAME func_X ...`, and a `^NAME` scan drops those silently. Use `grep -hoE '(^|\*\*|[-*) ])NAME func_[0-9A-Fa-f]{8} \w+ [-—] .*'` then strip the prefix; verified against the artifact corpus this round (12 bold lines existed that the strict scan lost).
- `gear_shop_menu` `func_801D18F8` is a **plateau at 98.73%**: all four spill slots are in retail's exact order, and the only delta is one extra live value - gcc keeps a second pseudo for the digit-loop giv base (a copy of `&sp20` spilled to 0x78), which makes the frame 200 instead of retail's 192. Ruled out this round: permuter run 4 (base 639 -> 539, but the candidate is 94.39), the three preset screens (2.7.2-cdk 94.65, 2.7.2 93.31), declaration order, and the workspace pointer type. Do not re-brief without a new idea about killing that duplicate pseudo.
- Spelling the digit-buffer accesses through the pointer (`((u8*)pRect)[8 + k*2]`) bought 98.39 -> 98.73 by killing one reload; the same trick using `&sp20` directly gives retail's *size* but 94.39.
| 83 | 2 deep workers + 1 rewrite worker | +0 | 1975 | Preset harvest, no matches yet: **`movie/main/misc2` moved to plain gcc 2.7.2 with -G0** (new `XenoMovieG0` preset), which makes its parked `func_80076AF0` a 100.00 body and takes `func_80076CA4` from 73.42 to 88.94 - both are round 84's unpark. `gear_shop_menu` `func_801D2B74` parked at **97.35%** (639 insns, with a layout-safe jump-table note). `world_map/misc2` rewrite: 12 functions CLEAN. raw 2004 -> 1975. 0 names. |

Round 83 detail:
- **The preset fan-out was worth one worker: `movie/main/misc2` is mis-preset, and my hypothesis was wrong.** The `$at` symbol-form store pointed at cdk -G0, but cdk scores 80.36 there; plain `gcc-2.7.2 -O2 -G0` is the only 100.00 (`try_flags.sh movie/main/misc2 func_80076AF0 --gp -G0`). Ask for the whole axis screen, not the theory - the worker's `--all` run is what settled it.
- The switch is a straight one: every already-matched function in the unit scores identically under -G8/-G0 (objdiff), per-unit flags are identical across two `make objdiff-config` runs (222 `-G0` rules both times), and `make check` is green on all eleven. A first attempt with **cdk** -G0 failed the `movie` link - that failure is what exposed the wrong compiler.
- `gear_shop_menu` `func_801D2B74` (639 insns) is at 97.35% with retail's prologue and all ten of its spilled givs in retail's slots; two hard-won notes are in its park: `itemId` must NOT be a local (retail re-reads `g_Menu->shopItemIDs[...]` per use) and the digit writes go through a `pRect` local while the `setRECT`/`LoadImage` pairs use `&rect`. Its jump table is 5 words + an `.align 3` pad and links at 0x801C5068 exactly, so converting it does not move the overlay.
- A pointer-local form is not always the right one: `D_8009D30C[index].vx` is byte-exact while `&D_8009D30C[index]` flips $a2/$a3 (98.89%) - the element-pointer lever is for *ties*, not a default.
| 84 | 1 unpark worker | +1 | 1975 | `MovieInitGteState` (movie/misc2 `func_80076AF0`) matched at 100.00 the moment round 83's `-G0` preset landed - the harvest needed no C written for it. `func_80076CA4` parked at 89.13 with a full characterisation. raw unchanged at 1975. 1 name. |

Round 84 detail:
- **A preset switch is the cheapest match in the project when the screen is right**: round 83's `XenoMovieG0` turned a 98.87% park into a 100.00 match, and the only source change it needed was one extern's type (`s16` -> `MATRIX` for `D_80077070`).
- `func_80076CA4`'s remaining gap is a NEW class worth recording: gcc will not share one symbol's `%hi` across two offsets, so retail's `addiu a1,a3,-0x14` (one base plus a displacement) comes out as a separate `lui a1` plus a `%lo(%at)` store. Size already matches (420 B). Every compile-time-known global address folds to a bare symbol store, so the suggested lever is a source shape where the trans stores go through a *runtime* base.
- The other movie units are still on `XenoBattleDiv` (no `-G0`); `movie/main/misc1` should be screened the same way round 83 screened `misc2` before anyone assumes the overlay wants one preset.
| 85 | 1 preset-screen worker + 1 deep park + 1 rewrite worker | +0 | 1964 | The `-G0` harvest does **not** transfer to `movie/main/misc1` (proved: `-G` is irrelevant there because every global is an extern supplied by splat asm; every matched function is 100.00 under both). `gear_shop_menu` `func_801D2B74` reached 97.37% and is now 110 of 112 segments exact. `battle/misc7` 14 functions retyped onto `BattleOpRegs`. raw 1975 -> 1964. 0 names. |

Round 85 detail:
- **A preset finding does not generalise - screen each unit.** `movie/main/misc2` wanted `-G0` (round 83) but `movie/main/misc1` scores identically under `-G8` and `-G0`, because every global it touches comes from splat asm, so maspsx never takes the gp-relative path and `-G` has nothing to choose between. The generalisable rule: `-G` only matters for a unit that DEFINES small data itself; a unit whose data is all externs is preset-insensitive in that dimension.
- `movie/main/misc1` has one jtbl stub (`func_80072A08`) that is a **config blocker**: its table lives in the overlay's single shared `[0x0, rodata]` blob, and `own_rodata.py --report movie` calls it FOREIGN (data labels `D_8006FD88/FDB0/FDBC/FDE0` sit inside the range). Hand work, not a carve.
- `gear_shop_menu` `func_801D2B74`: the old park note was wrong twice (retail is 2532 bytes / 633 insns, not 2464, and the case-block order was already correct). What is left is two segments: retail emits the per-case `equipFlags` load/move/spill suffix twice (ARMOR+FRAME share one copy, ENGINE has its own because its element address is in `$v1`) where gcc 2.6.0 merges all three, and the `places[]` base spelling. The `value = equipFlags; equipFlags = value;` idiom unmerges it (97.37%) but changes what the digit loop renders, so it is not shippable.
- `battle/misc7`'s "raw" lines were the `D_800D3400` family: `D_800D3410/3420/3430/343F` are `+0x10/+0x20/+0x30/+0x3F` of one 0x40-byte struct, now `BattleOpRegs`, with all 14 functions byte-identical.
| 86 | 1 park worker + 2 rewrite workers | +0 | 1945 | Two rewrite batches: `battle/misc20` 15 functions onto the battle structs, `battle/misc10` 22 functions onto the `D_800D3400` family (16 of them read 99.2-99.4% only as the typed-addend class, `check_bin` green). `movie/main/misc1`'s two parks sharpened, not solved. raw 1964 -> 1945. 0 names. |

Round 86 detail:
- `movie/main/misc1` `func_80072D84` is a **live-range** tie, not a spelling one: gcc writes the div-by-3 sign fixup into `v1` only when the dividend register `v0` is still live across the `subu`, and no plausible source of that function keeps it live (proved by keeping the masked byte live, which flips one site). Eight shapes plus two permuter runs all land at 99.40%.
- `movie/main/misc1` `func_800734B8` (93.54%) has TWO coupled causes that must be solved together: a pointer-allocation permutation AND the CSE difference it implies (retail re-reads three channel bytes after the divisions, which is where the missing 16 bytes are). Declaration order is not a lever there - six orders compile identically.
| 87 | 2 permuter-apply + 1 rewrite worker | +2 | 1925 | Both ZEROs ported: `BattleSetPrimFlagsKindThree` (battle/misc15) and `FieldScriptMemoryWriteU16` (field/scripts/virtual_machine). `battle/misc25` three functions CLEAN. raw 1945 -> 1925. 0 names (both functions already had their symbols). |

Round 87 detail:
- **Converting a stub to C can shorten a CALLER in the same file** - the second entry of the "what objdiff cannot see" list, hit again. `FieldScriptMemoryWriteU16` ported at 100.00% but the overlay grew 4 bytes: with a real prototype the four later call sites narrowed their arguments (`andi a1,a1,0xffff`, `lhu`) and `FieldScriptUpdateMemoryEachFrame` gained an instruction. Retail has `lh`/no `andi`, i.e. its callers saw the `(int, int)` prototype the repo already declares in `misc.c`/`misc6.c`. **Before converting a stub in a unit that calls it, grep the repo for its existing prototype and use exactly that signature.**
- The permuter's winning edit was pure allocator forcing; the worker replaced it with the shape two matched siblings already use (`int slotStride = 0x40;` as a local, because a literal gives the index-first `addu`). When a port is "load-bearing garbage", look for the sibling that matches and copy its spelling.
| 88 | 1 deep park + 2 rewrite workers | +0 | 1899 | `world_map/misc10` nine functions typed, `battle/misc19` ten typed (four keep an index-first cast on purpose), `gear_shop_menu` `func_801D2B74` root cause narrowed to a hard-register difference. raw 1925 -> 1899. 0 names. |

Round 88 detail:
- `gear_shop_menu` `func_801D2B74` is a **hard-register** problem, not a statement-order one: whether gcc keeps one or two copies of the `equipFlags`/`price`/spill suffix is decided by which register holds the element pointer (retail `$a0` for the ARMOR+FRAME copy, `$v1` for ENGINE's inline copy; gcc 2.6.0 gives `$v0` at all six sites). Every ordering, pointer-local, do/while and temp lever is a no-op, and the preset is right (96.75 under the unit's own 2.6.0 vs 96.4 for 2.7.2 and 83.59 for cdk). 1 instruction of the gap is the `places[]` base (`lw t1,120(sp); addiu a1,t1,16` vs `addiu a1,sp,48`).
- **A true prototype can be the wrong prototype.** `world_map/misc10` must keep the extern for its `QueueZoneEntry*` callee as `(s32, s32*, s32, s32)` even though the callee in `misc11.c` is `(s16, s32*, s16, s16)`: with the true signature gcc truncates at all three call sites and the functions go from 56 to 64 bytes (85.00%). Retail's game code evidently never saw the s16 prototype - do not "fix" that extern.
- The element-pointer lever has a direction: `&D_800C34B0->slots[arg0]` emits base-first where retail is index-first, so four functions in `battle/misc19` keep `(BattleTableEntry *)(arg0 * 0x170 + (u32)D_800C34B0)` deliberately (99.77-99.82% with the cast, 100.00% only with it).
| 89 | 2 deep-stub workers + 1 rewrite worker | +0 | 1890 | `battle/misc20` fully typed (10 functions) via a discovered **`BattleSlotState` at slot entry +0xA4**; `lib_gear` two parks advanced (97.40, 75.14), `menu/misc11_r2` one to 99.49. raw 1899 -> 1890. 0 names. |

Round 89 detail:
- The unlock worth reusing: `D_800D2DC8`/`D_800D2D6C` are pointers to the per-slot action state a slot entry carries at **+0xA4 (0xCC bytes)**, which had no declared type. Declaring `BattleSlotState` made ten functions typeable in one pass, including two that an earlier round had excluded as untypeable. Cross-check: `state.unk64` and `BattleTableEntry.unk108` are the same bytes, so `unk104`/`unk108` are duplicate outer names for fields the nested type already covers.
- `lib_gear` `func_801E8030` (97.40%): the pool address MUST be a local inside each block under `-G8` (`Temp4* pPool = &D_801E86A8;`) - passing the literal makes gcc re-form `lui/addiu` into `a0` at every call instead of holding it in `s0` and copying it into the `jal` slot (94.08 -> 96.93). Remaining delta is two teardown loops where retail copies the slot address into `s2` at loop entry (`addu s2,s0,zero`) and schedules `offset = 0` after the exit test.
- `menu/misc11_r2` `func_801E6B70` (99.49%): five `addu` operand slots that gcc 2.7.2 always orders destination-second where retail has destination-first. Array form, `u8*` form, `(u32)base + offset` and offset-written-first all compile identically; cdk is 94.29 and the permuter's best candidates scored *worse* than the parked body. Treat as a wall unless a new lever appears.
| 90 | 1 two-file structural worker + 1 deep park + 1 permuter-apply | +1 | 1885 | `LibGearFreeModel` (lib_gear `func_801E8030`) closed from 97.40%. `BattleSlotState` is now nested inside `BattleTableEntry` at +0xA4, replacing the opaque `unk90[0x74]`, with all 31 matched functions in misc20+misc19 re-verified. `battle/misc7` `func_8007B9C8` ported 85.33 -> 98.33. raw 1890 -> 1885. 1 name. |

Round 90 detail:
- **The induction-base idiom works in reverse too.** `lib_gear`'s park carried a hand-written byte accumulator (`s32 offset; offset += 0x30; (Temp8*)((u8*)p->unk118 + offset)`); rewriting the teardown loops as *indexed* accesses on the declared array (`g_LibGearModels[m]->unk118[i]`) made gcc strength-reduce `i * 0x30` back into retail's own `addiu s0,s0,48` accumulator, which then freed `s0` for it and produced retail's `move s2,s0` slot-pointer copy. The permuter's 98.85% candidate had found the same insight (a scaled index) with the wrong semantics.
- The shared-header risk paid off this time: nesting `BattleSlotState` at +0xA4 (replacing the opaque `unk90[0x74]`) was done by ONE worker owning both `misc20` and `misc19` plus the header, with a mandate to revert on any regression - all 31 matched functions in both units came out unchanged and `make check` is green. When a header split needs edits in two units, give one worker both units explicitly (they are serialized by ownership, not by the one-file rule when nobody else touches them).
- `battle/misc7` `func_8007B9C8` is now a **mapped mutual exclusivity** at 98.33%: every index-first spelling of the sum gives retail's `addu` operand order but materialises the base into `v1` *before* the `*p` load (85.33%), and base-first keeps the load-first header and differs only in the sum's destination register (98.33%). Two levers and a 2084-iteration permuter run from the 98.33 body changed nothing.
| 91 | 1 ZERO port + 1 deep sound worker + 1 rewrite worker | +2 | 1872 | `WorldMapBeginSlotAnimFrame` (world_map/misc7, permuter ZERO) and `SoundApplyElementStatusFlags` (sound `func_8003EBF0`). `battle/misc5` 14 functions onto the `D_800D3400` register file. raw 1885 -> 1872. 2 names. |

Round 91 detail:
- **The `do/while` lever also forces a load HOIST order, and an EMPTY block can be the whole match.** `world_map/misc7` `func_80081174` was 95.26% parked; the load-bearing statement is an empty `do { } while (0);` after the element-pointer local, which stops gcc 2.7.2 hoisting the `D_8009A6C0` load above the slot address formation - with it the natural pointer form is 100.00%, without it 65.26%. The permuter's `new_var` scaffolding was a workaround for missing that statement, not a shape to port.
- `sound` `func_8003B644` is parked at 94.82% with a warning that matters for whoever takes it: six already-matched functions call it through an **implicit** declaration, so converting it can shorten all six (the callee-visible-in-TU trap). Its argument shapes are recorded in the park note.
- `battle/misc5`: the `D_800D3430 + arg*0x40` family is `D_800D3400[arg].unk30`; 14 functions typed, the sub-100% ones being the typed-addend class with `check_bin` green.
| 92 | 1 ZERO port + 1 deep sound worker + 1 rewrite worker | +1 | 1865 | `BattleResetForNextRound` (battling/misc4, permuter ZERO) - the fix was a non-void return type keeping retail's `nop` in a `beqz` delay slot. `system/temp2` typed via a new `SpriteBaseExt`. raw 1872 -> 1865. 1 name. |

Round 92 detail:
- **`sound`'s caller hazard is now disproved, not just suspected.** With the real `(s16, s32, s32, s16)` prototype in scope, all six implicit-declaration callers of `func_8003B644` stay at 100.00% - so the park at 94.82% (a pure `s8/fp` vs `s6` register swap) can be converted later without breaking the link. That is the *opposite* outcome to `FieldScriptMemoryWriteU16` in round 87; the difference is that these callers already declared it implicitly, so no codegen changes.
- `system/temp2` note worth reusing: the `0x3C` word's bits 24-27 are `field_0x3C_2` (bit 24 == `field_0x3C_2 & 1`), and since bitfields cannot be address-taken, any spelling that needs `&word` must stay raw - every typed form loses the `lbu 0x3F` retail emits.
| 93 | 1 deep sound worker + 1 rewrite worker | +1 | 1855 | `sound` `func_8003C020` (281 insns) closed from its 86.26% park by fixing four diagnosed deltas plus two new ones. `func_8003B644` re-parked 94.82 -> 96.32. `battling/misc14` six functions typed onto `Misc14ModelPart`/`ModelNode`/`DisplayWork`. raw 1865 -> 1855. 0 names (the matched function already had a symbol). |

Round 93 detail:
- A park note can be **structurally incomplete, not just mis-spelled**: `func_8003C020`'s 86.26% body was missing a whole STATEMENT (`unk_0x50 += 0x10000;` at the top of the inner loop - retail hoists `lui a0,0x1` for it), and the field's type was wrong (`AudioManager.unk_0x36` must be `u16`, giving `andi 0xffff` instead of `sll 16`). When a park's percentage is stuck below ~90 with "shape only" diffs, re-read the retail loop for a missing statement before re-shaping what is there.
- The last 0.18% was a **dead-but-live assignment** (`if (cdVolume = pManager->unk_0x48 != 0)`), the same class as round 63's dead duplicate branch arm: an assignment that changes nothing semantically but keeps a value live and so reproduces retail's register choice. It is documented in the body.
- `sound`'s remaining park `func_8003B644` is at 96.32% with exactly 11 instructions of `$s6`<->`$s8` allocno priority left; the tone-table byte needed a LEFT-associative pointer sum (`*((u8*)pSoundFile + off + idx)`) to get retail's `addu v0,s2,v0` then `+ idx`.
| 94 | 1 lead port + 1 rewrite worker | +0 | 1851 | `slus_006.64/system/archive_b` `func_800379B4` took its park from 77.80 to **96.00%** (the fix: an `extern volatile u32 D_80050618[3]` array declaration makes gcc materialise the address instead of the `sw $4,sym` macro form). `system/temp1` seven functions typed. raw 1855 -> 1851. 0 names. |

Round 94 detail:
- The `$at`-macro class is not always a preset: `func_800379B4` reaches retail's 5-instruction shape by declaring its global as `extern volatile u32 D_80050618[3]` (array decay + volatile -> a materialised address instead of `sw $4,sym` through `$at`), and then stops at 96.00 on `la $v0` vs `la $t0` - a register that only a second live pseudo could move, and retail's body has none. 5643 permuter iterations found nothing, and cdk is 77.80 both with and without `-G0`.
- `include/field/actor.h` splits the `0x3C` word into **bitfields**, so any whole-word read-modify-write there must stay raw: a bitfield store adds `lui/ori 0xFFFFF` + `and` (63.52% measured). Four functions in `system/temp1` and two in `system/temp2` keep raw RMW for that reason - it is a header decision, not laziness, and the workaround is to index the bits through a byte pointer.
| 95 | 2 deep workers + 1 rewrite worker | +2 | 1832 | Two jump-table functions: `func_801E36D4` (menu/misc10, 235 insns, 99.96 JTBL + `check_bin` OK) and `WorldMapUpdateActorState` (world_map/misc5, 99.95 JTBL + OK). `func_801E3ECC` **confirmed** blocked (8 table words for 7 cases). raw 1851 -> 1832. 2 names. |

Round 95 detail:
- **A `check_bin` failure on a switch conversion can be the CASE ORDER, not the table.** `func_801E36D4`'s bodies are instruction-identical in any order (objdiff 99.94 vs 99.96), but the bytes inside `.text` differ - retail emits the case bodies in source order 1,2,3,4,**7**,5,8,9,10. Two more closures from the same function: the base must be **accumulated in place** (`pWeapon = pDressingRoom->pWeaponsData; pWeapon += pChar->unk6A[0];`) because `&arr[i]` and `arr + i` both give index-first `addu`; and the accessory flag tests add the item byte as the **left** operand (`value + pChar->f`, not `+=`).
- `world_map/misc5`'s jump-table family is now a documented conversion order: **ADD4 -> BBEC -> C3B8 -> C7D8**. C `.rodata` lands before asm `.rodata` in the object, so only a prefix of the list can go at a time (same rule as round 56's misc30 chain).
- `convertible.py`'s BLOCKED reason is not always the real one: for `func_801E3ECC` it counts instructions after the `jtbl_`'s own `.size` line (which every switch stub in that unit has) while the actual blocker is a trailing 0x00000000 pad word. The pad-word check is: table word count vs `sltiu` bound.
| 96 | 1 whole-family jtbl worker + 1 rewrite worker | +0 | 1830 | The `world_map/misc5` family batch is **gated on `func_8007ADD4`**: converted to a byte-close 81.35% body (1080 vs retail 1068) and parked, so BBEC/C3B8/C7D8 cannot follow yet - the prefix rule working as documented. `menu/misc9` three functions CLEAN. raw 1832 -> 1830. 0 names. |

Round 96 detail:
- The 12-byte gap in `func_8007ADD4` is **three read-modify-write globals** (`D_8009BE30` case 4, `D_8009BD3A` case 5, `D_8009BD40` in the shared tail) that retail addresses through a held register (`a2`/`a3`/`t1`) where our compile folds to `%hi/%lo` through `$at`. That is the same family as round 94's `func_800379B4` fix (an `extern volatile T name[N]` array declaration makes gcc materialise the address) - the next attempt should try it here, and `cdk -G0` is 69.06 so the preset is not the cause.
- Its first switch needs an **`s16` local** (`s16 nState = pPlayer->unk4 - 1;`) to reproduce retail's `lhu` / `addiu -1` / `sll 16` / `sra 16` / `sltiu 7`; a bare `switch (p->unk4 - 1)` gives `lh` and a different size. Second instance of that pattern in world_map.
- A whole-family batch brief must be gated on its FIRST member: the worker correctly stopped instead of converting later tables out of order. Budget such a round for the first member's closure only.
| 97 | 2 park workers + 1 rewrite worker | +2 | 1817 | `func_801E6B70` (menu/misc11_r2) closed from 99.49% and `BattleAccumulateSlotValueByRowFlag` (battle/misc17) from 99.77%; `ADD4` 81.35 -> 87.56. raw 1830 -> 1817. 2 names. |

Round 97 detail:
- **THE EMBEDDED-ASSIGNMENT LEVER, now in `brief.js`, is the strongest answer to addu operand order.** Retail emits the base-first `addu d,base,offset` only when that sum is the *result of an assignment*; as a plain sub-expression gcc canonicalises it to offset-first whatever the C says (`(u8*)base + off`, `base[off]`, `(u32)base + off`, offset-first, bracket forms - all measured identical at 99.80%). Write `*((pSlot = (u8*)base + off) + K) += x` or hand the sum to a local first, and keep the embedded locals **separate** (reusing one lets gcc CSE them and the address stops being re-derived). This closed `func_801E6B70` after two workers had declared its five `addu` orders uncontrollable - **a "not source-controllable" note from an earlier round is a hypothesis, not a fact.**
- The shared structs keep paying: round 77's "`D_800C3EA4 + 0xA230` wall" dissolved because `battle_work.h`'s `BattleGraphicsWork.pPacketPrims` covers that offset exactly, and `BattleAccumulateSlotValueByRowFlag` closed by typing `D_800C34B0` as `BattleWorkBlock*`. Re-brief old walls after a header lands.
- `world_map/misc5` `func_8007ADD4` 81.35 -> 87.56: the three "held register" globals were never an allocator tie - they are **unbounded array declarations** (`extern SVECTOR D_8009BD38[];`), and with the array form gcc materialises the base for the **first read-modify-write of a basic block** while loads and stores still fold to `%hi`/`$at`. The last 4 bytes need a third symbol that does not exist at 0x8009BD40.
| 98 | 2 wall re-tests + 1 rewrite worker | +0 | 1809 | Both `addu`-order parks re-tested with round 97's lever: **`battle/misc7` `func_8007B9C8` and `battle/misc13` `func_80085D34` are now PROVEN walls** (20 shapes each plus 2716/2398 permuter iterations). `sound` six functions typed. raw 1817 -> 1809. 0 names. |

Round 98 detail:
- `func_8007B9C8` (98.33%): retail's `addu` destination and its load-first header are **one register decision**, not two levers. The separate-locals family reproduces retail's `addu` destination and the entire instruction multiset, but then materialises the base address into `v0`/`v1` and hoists it above the `*p` load (84.00-85.33%); only the coalesced base-first form puts the address temp in `a0`, and that same `a0` is what pins the address pair in the load's delay slot. Do not re-brief.
- `func_80085D34` (99.63%): **round 79's note was wrong** - integer sums do NOT reproduce retail's operand order. Every int-typed sum lets gcc reassociate `0x9C8` onto the base (`addiu v0,v0,2504`) and still emits base-first; only the pointer form keeps `0x9C8` on the offset chain, and that form always emits base-first. Do not re-brief.
- Both negatives came from *testing the strongest available lever*, which is what makes them worth recording: a wall is only a wall once the current best lever has failed on it. The corollary also holds - round 97's lever DID break a wall that two earlier workers had called uncontrollable, so "closed as a wall" must carry the date and the lever it was closed with.
| 99 | 1 deep-stub worker + 2 rewrite workers | +0 | 1782 | `battling/misc1` is now **fully CLEAN** (a `BattleActorView` partial struct explains the 0x34 `VECTOR` and why +0x38 is skipped), `battle/misc24` three functions typed, `battling/misc10` `func_80084C88` parked at 89.37 with two blockers. raw 1809 -> 1782. 0 names. |

Round 99 detail:
- Two recurring blockers named precisely: (1) the retail position of a mid-function store (`pActor->unk5C = pNode` in the *middle* block) makes that local block-scoped so local-alloc gives it `s0` and the actor falls to `s1` - moving the store after the switch fixes every s-register but leaves it 3 instructions late; (2) `p->unk90A |= 1; p->unk90A |= 2;` is CSE-d into `|= 3` and gcc cross-jumps the three case tails. The untried lever for (2) is a union member / bitfield for that flag byte.
- A partial **view** struct in one unit is the cheapest way to type a shared actor: `BattleActorView` in `battling/misc1.c` covers only the fields that unit touches and leaves everything else `unk` - no header edit, no risk to other units, and the file went to zero raw casts.
- ORCHESTRATOR MISTAKE to avoid: `git commit --amend` on a commit that was already pushed rewrites history and the next push is rejected. When a commit step prints "nothing to commit", the tracker row still has to be committed as a NEW commit - never amend.
| 100 | 1 two-file header worker + 1 rewrite worker | +0 | 1767 | `BattleSprite` (0x7C element) now carries its **full view** in `include/battle/actor.h` (pParent/doTransform/doRotate/rotDirection/jointIndex/count/localTransform/globalTransform/scale), `misc21.c`'s unit-local copy is deleted, and `battle/misc24` is typed. `world_map/misc9` ten functions typed. raw 1782 -> 1767. 0 names. |

Round 100 detail:
- The round-90 pattern worked again: **one worker owning both consuming units plus the header** landed a shared-struct migration with every matched function in both units still at 100.00% and `make check` green. Two unit-local duplicates (`BattleSprite`, `BattleBlock`) disappeared into the header, which is the opposite of divergence.
- The element-`addu` direction is per-site and must be measured: in `misc24` `BattleFreeActorEntryAndChildren` retail is *offset-first*, while `BattleSpriteUpdateWorldPosition` is *base-first* - so the cast-with-`sizeof` spelling stays in one function and the honest `&pSprites[i].field` stays in the other. A worker that "cleans up" both to the same form breaks one of them.
- `world_map/misc9` went CLEAN on ten functions the same way the overlay's other units did: typing `D_8009BE24`/`D_8009C620` as pointers and indexing the 0x54 layer table removed `*(u16*)(pSrc + 0x40A)` and `(MATRIX*)(pEntry + 0x20)`-style arithmetic wholesale.
| 101 | 2 deep park workers + 1 rewrite worker | +0 | 1767 | Two more parks are now **proven walls with the new levers tested**: `gear_shop_menu` `func_801D2B74` (96.75%, the suffix merge is a register coincidence and no source shape moves it - third permuter run too) and `lib_gear` `func_801DFAC4` (79.40%, one allocator decision cascading into 28 bytes; the body is now retail's instruction-for-instruction in ORDER, only the choice of what lives in memory differs). `battling/misc2` is fully CLEAN except the settled `BattleEffectSetMatrix`. raw unchanged at 1767. 0 names. |

Round 101 detail:
- `lib_gear` `func_801DFAC4` is now a **pressure** problem, not a shape problem: 242 instructions in retail's exact order, frame size and every slot offset matching (0x78, 0x20/0x28/0x30/0x38), and only the *choice* of what spills differing (retail keeps the loop constant `1` in `a1` with caller-saves around two calls and reads `type` from 0x88(sp); gcc 2.6.0 puts the constant in `s7`, the vertex base in `fp` and spills `flag1`/`type`). Two real source fixes got it there: `flag1` as `u8` (75.14 -> 79.38) and splitting a chained `unk4=unk8` assignment into one temp so the two `sw`s keep retail's order (a chained assignment stores 8 before 4, and gcc refuses to CSE the doubled `pVertexBase + pKeyframe[0]`, which cost 24 bytes).
- `gear_shop_menu` `func_801D2B74` (96.75%) is settled: the ENGINE/FRAME/ARMOR suffix merge is a register coincidence (ours lands the element address in `$v0` at every site; retail has `$a0` at ARMOR/FRAME and `$v1` at ENGINE) and the three new levers plus a third permuter run changed nothing. The 97.37% `value = equipFlags; equipFlags = value;` variant is semantically wrong.
- Both walls now carry the date and the levers that failed, which is what the round-98 lesson asks for.
| 102 | 1 fresh-stub worker + 2 rewrite workers | +0 | 1747 | `battle/misc23` `func_800A64E4` parked at **98.71%** on an 18-`addu` operand-order tie (every other instruction in retail order, same frame and registers; 7 shapes and a 2036-iteration permuter all failed). `battle/misc34` three functions onto typed `SpriteData`/task blocks, `battling/misc8` 14 lines (five globals retyped as `RECT[]`/`DR_MOVE[]`/`DR_TPAGE[]`/`BattleTextList`). raw 1767 -> 1747. 0 names. |
| 103 | 5 permuter-apply + 16 naming workers | +2 | 1130 | `BattleUploadImageFrame` (battle/misc14 `func_8008A144`, park 91.84 -> permuter 90 -> hand fix 100.00) and `BattleRemapActorModelId` (battle/misc34 `func_800BF8CC`, PERMUTER_ZERO on its 9th iteration). Three parks advanced and re-parked: `battle/misc24` `func_800AA934` 88.71 -> 96.25, `battle/misc34` `func_800BFDA8` 90.75 -> 95.75, `battle/misc7` `func_8007B9C8` unchanged at 98.33. Naming round over 16 files: **23 names**, unnamed C definitions 129 -> 106. raw 1747 -> 1747. |
| 104 | 8 fresh-stub workers (deep shape) | +6 | 1124 | `battle/misc34` `func_800BFA9C`/`func_800BFBA0` and `battle/misc20` `func_8009C198` **BattleExecuteActionRow**, `func_8009CA90` **BattleAdvanceActionRow**, `func_8009DB54` **BattleScaleValueByRowBitPercent**, `func_8009DA04` **BattleApplySlotPercentToValue** (three of the misc20 four were 93-99% parks; the levers were a flat `A && B && C` chain and a break-edge index copy that is unassigned when the scan finds no bit). Seven parks from the other six workers: misc32 94.66/91.07, misc6 96.29, movie/misc2 92.41, menu/misc8_r2 91.33, misc15 88.52, temp2 80.74, misc34 `func_800BF85C` 71.61. raw 1747 -> 1748. |
| 105 | 8 rewrite workers | +0 | 1124 | Live raw (comments stripped, so parked bodies excluded) **684 -> 585**. 50+ cast-and-offset sites replaced by typed structs with every byte unchanged: `Misc16Entry`/`Misc16Object` (battling/misc16, all 4 functions CLEAN), `BattlingSlot` + the SDK `setLineF2` macro (battling/misc11, 11 of 13 CLEAN), `BattleDigitState` 0xF8 view (battle/misc1), `WorldMapScriptSlot` 0x80 (world_map/misc3, 7/7), `BattleTableEntry`/`BattleSlotState` (misc18/misc19). Measured walls, all left raw with a comment: `SystemInitializeFont`'s six u16 header reads (every typed spelling batches the `lhu`s, 66.41%; `int dataOffset` is load-bearing, 99.74% as u16), `BattleSetSlotEntryFieldsTo1/2000`'s `int offset` form, `BattleClearLineSlots`'s byte-offset counter, misc18's `D_800D2200` base (splat emits no symbol for an unreferenced address). |
| 106 | 8 fresh-stub workers (deep shape) | +1 | 1123 | Only `BattleUpdateActorMovement` (battling/misc3 `func_80077770`, 178 insns). Below the 3-match stop rule: the fresh pool at this depth is 200-500 insn stubs and one worker per file does not close them. Seven parks, several worth a lever: battling/misc3 `func_80077038` **99.06%** (sole diff: retail RELOADS `p->flags` after the `|= 0x400` store, gcc forwards the stored value - keeps the function 4 bytes short), battle/misc18 `func_800995A0` **97.11%** (2.6.0 emits `beq`+extra block where retail inlines the last arm), battle/misc14 `func_8008A3EC` **97.61%** (BLOCK_ROTATION: retail's `i++` is the if-body's first insn and steals into the `beqz` slot), menu/misc9 `func_801DD5E8` **94.01%** (one `addu`: retail forms the read address and the call argument separately, every C spelling lets gcc CSE them), misc20 `func_8009D3A0` **91.80%** (permuter base 4514 -> 1813 but its edit costs objdiff), misc4 84.48, world_map/misc8 76.46, battling/misc10 71.59. |
| 107 | 7 rewrite workers (one owning a shared header) | +1 | 1122 | Live raw **585 -> 508**. The **`SpriteFlagWord` header lever landed**: `include/field/actor.h` now types `flags_0xAC`/`flags_0xB0` as a NAMED union (`s32 flags` + `u8 bytes[4]`) and five battle files were respelled to `.flags` - proved along the way that **anonymous unions are rejected by all three cc1s here** (`structure has no member named 'w'`), so a field can never carry two names. animation_scripts.c is now fully typed off `.bytes[]`; temp1/temp2 have the header but their 42 lines remain. Also `func_800B35C0` -> **BattleGetScaleTask** (battle/misc26, new match, `BattleScaleTask`/`BattleModelFrame`); the 25 one-line register-op helpers in misc6_r2 + misc10 now index ONE `BattleOpRegs` view at `D_800D3400` (objdiff reads 99.4-99.6% RELOC only: the original names `D_800D3420`, the addend is identical); world_map/misc7's two fade loops and misc20's 14 lines CLEAN. Wall recorded: `BattleApplyGearWeaponRecord` and `BattleClearSpriteTables` must keep their array-index form - a pointer local deletes 12 instructions (85.13%, 86.26%). |
| 108 | 6 rewrite workers + 1 park worker | +0 | 1122 | Live raw **508 -> 457**. All 30 target functions CLEAN and byte-identical: temp1/temp2 off the new `SpriteFlagWord` byte view (only the 0x3C word stays raw - see below), battle/misc14 (13 of 14), misc29 (`BattleEnvBlock`: the 0xDISPENV screen offsets are the same function's `SetDefDispEnv` targets), misc34, world_map/misc4 (`WorldMapEntry` shared with misc7). **`archive_b func_80037878` is now a PROVEN TOOLCHAIN WALL, not a C problem**: `maspsx --print-output` shows its mflo/mult hazard nop is appended *after* the window instruction with the loop label emitted after the nop, while retail's aspsx put the label first; `_handle_nop_before_next_instruction` only fires after a load, so no source shape can move that label. Five loop shapes + a 503-iteration permuter re-run all reproduce 99.94% exactly. Do not re-brief it. |
| 109 | 5 SDK-name workers + 5 rewrite workers | +0 | 1122 | Live raw **457 -> 425**, unnamed C definitions **109 -> 102**. Seven psyq functions named by hand because `rename_sym.py` cannot (`'X' is already used` - it counts any identifier anywhere, and `include/psyq/*.h` declares these): **GetODE, StopPAD, OpenTMD, SetDrawLoad, MargePrim, SetGraphQue, CheckPrim**, each `diff_func` MATCH after the rename. `StopPAD` needed a `symbol_addrs` INSERT (splat had auto-named the glabel) and one call site in `main/main_loop.c`. Two reverted for a reason worth keeping: **SetTexWindow/SetDrawArea** break the repo link (battling/misc12+misc14 and world_map/misc1 declare their own `extern func_800453E8/453AC` and `linker/undefined_funcs_auto` stops exporting those names) and **SetDrawOffset** must keep `func_` because `libgpu.h`'s `u_short *ofs` yields `lhu` where retail has `lh` (92.94%). temp1's 0x3C word measured again: the `field_0x3C_0` store spelling is 63.52%, the read works. |
| 110 | 7 near-miss park workers | +2 | 1120 | `battle/misc19` `func_8009AFD8` (**MATCH 100% JTBL**, `check_bin` OK) and `battle/misc25` `func_800B0164` -> **BattleSetBlockSlotCommand**. Two levers worth the round on their own: (1) retail's decrement immediate is `addiu 0xFF`, so `arr[i] = arr[i] + 0xFF` matches where `arr[i]--` / `-= 1` emits `-1` (0xFFFF) - and the same spelling already exists in the matched `BattleDecrementConditionTimers`; (2) **a park note that blames global.c must first be checked that the parked body COMPILES** - misc25's body called `BattleAllocBlock()` with no argument against an extern taking one, so its 99.70% came from some earlier variant and the real fix was `BattleAllocBlock(pPool)`. Five parks re-parked with sharper notes and three walls dated: world_map/misc3 `func_800771D8` (`negu v1,v1` vs `negu v1,a2` - 28 shapes, fold_rtx equivalence class, no C lever), battle/misc16 `func_8008DC34` (`addu s0,s4,s6` canonicalised after expand, same under 2.7.2), lib_gear `func_801E63A8` (a1 register-equivalence class, 3 permuter runs), menu/misc12 `func_801E86C8` (which of `D_801EA1EC`/`+4` materialises first). |
| 111 | 1 near-miss + 7 park-triage workers | +3 | 1117 | `battle/misc14` `func_8008963C` -> **BattleBuildGrey80BlendSpriteColumn7** (the permuter found it in 87 s once the unit was on 2.6.0) and `func_80089B50` -> **BattleRandomRange** (`(u16)rand() % (range+1)`; the note's claim that every masking spelling duplicates the call was 2.7.2-specific), `battle/misc4` `func_80078D6C` -> **BattleQueueCommandF9** (`do { } while (0)` around the store kept it ahead of the read-modify-write). **The park pool was mostly unbuildable and is now scored:** all 34 triaged bodies compile for the first time, and the causes were mundane - a `void func_X(void) {` header line duplicated *inside* the park block (park.py matched the outer one, so un-parking produced two headers), declarations written inside the comment, symbols retyped since (`D_800C3DFC[0x11]` -> `->unk11`), and a wrong return type (`void` where the extern says `s32`). All 34 are re-parked with their real scores: menu/misc12 `func_801E86C8` **99.79%**, `func_801E8F60` **99.51%** (new shape: each K != 0 loop needs the one-expression `off = (i*2 + renderContext)*0x28 + K`), battle/misc19 `func_8009A9D0` 99.66 (100.00 under 2.6.0 only), `func_8009BD94` 98.33, battle/misc4 `func_80079AB0` 98.55, battle/misc18 `func_800995A0` 97.11, world_map/misc8 `func_80085158` 91.97 (8 bytes short: retail keeps a sixth saved register). |
| 112 | 8 near-miss park workers | +4 | 1113 | `battle/misc14` `func_800891E4` -> **BattleBuildGrey80BlendSpriteColumnPair**, `battle/misc18` `func_800995A0` -> **BattleSetSlotConditionTimer** and `func_8009892C` -> **BattleResetSlotConditionCounters**, `battle/misc32` `func_800BD7A0` -> **BattleUnlinkAndFreeTask**. Four general levers went into `brief.js`: **gcc 2.6.0 emits switch case bodies in SOURCE order and gives the fall-through arm to the FIRST-LISTED case** (misc18 97.11 -> 100 by listing 0x8000, 0x4000, 0x1000 instead of ascending); an early exit gcc folds to a constant is fixed by **one shared `goto done` exit** (misc4 98.55 -> 99.78); **take a member-subobject's address into a pointer local and pass the struct separately** when retail reaches the base by a negative displacement (movie/misc2 89.13 -> 99.24); and **read the objdiff BYTE COUNT, not just the percentage** - misc32's park was one instruction short because a wrong `(void)` prototype made the real call impossible to write. Near misses advanced and re-parked: movie/misc2 `func_80076CA4` **99.24%**, battle/misc4 `func_80079AB0` **99.78%**, world_map/misc8 `func_80085158` **94.26%** (its note's "8 bytes short" was wrong; the body is one instruction LONG), battle/misc18 done, menu/misc12 `func_801E8F60`'s round-111 99.51% was **not reproducible** - treat it as a hypothesis. DECISION: `battle/misc19` has two parks at 100.00% under gcc-2.6.0 (`func_8009A9D0`, `func_8009BD94`) but four matched functions drop under it (BattleClearSlotFlag15A1 95.71, BattleSetSlotFlag15A1 99.57, BattleSetSlotEntryFieldsTo1 99.71, ...To2000 99.77), so closing them costs a per-file split that adds five files for two functions - not taken unattended; the operator would gain 2 matches plus a corrected `func_8009A854` park (76.74 -> 84.95). |
| 113 | 6 deep park workers (2 duplicates stopped) | +1 | 1112 | `battle/misc14` `func_8008A3EC` -> **BattleUpdatePauseMenu**, and that unit now has **no parks left**. Its note's "allocator tie" was wrong twice over: the else-branch guard tests the **mute latch**, not the wait flag (which alone removed the s0/s1 swap, the prologue reorder and a phantom `andi`, 668 -> 664 bytes), and the button test must be `if (!(btn & 0x800)) continue;` then `if (D_800CCC58 == 0) break;` - the `&&` form sends both falls to the `while` top. **`movie/main/misc2` switched to gcc 2.6.0 at -G0 (new preset `XenoMovie260G0`)**: every live C function scores identically under 2.6.0 and 2.7.2, but the unit's park `func_80076CA4` is instruction-identical to retail under 2.6.0 (`mflo` in `$v0`; 2.7.2 puts it in `$v1` and 12 shapes could not move it), and its other park `func_80075D8C` goes 92.41 -> 93.66. Two walls dated: `battle/misc32` `func_800BD2E4`'s `%hi` hoist is **loop.c's move_movables cost threshold** (`threshold 29`, `savings 1`, `lifetime 1`, loop body 27-29 insns, boundary measured exactly at 30 by padding), not a source shape; `battle/misc4` `func_80079AB0` is one delay-slot register (`lw a0,16(sp)` vs `lw v0,16(sp)`) after 12 hand shapes + 2 permuter runs. Two workers shared `battle/main/misc4` and `movie/main/misc2` by my mistake; both duplicates stopped mid-round (cheap) rather than merged. | 
| 114 | 6 workers (5 parks + 1 fresh-stub unit) | +4 | 1108 | `movie/misc2` `func_80076CA4` -> **MovieBuildViewMatrix** (the preset switch paid off immediately: under gcc 2.6.0 the parked body is instruction-identical, objdiff names `D_800770D8+24` where retail names `D_800770F0` with the same addend, `check_bin` OK), `world_map/misc8` `func_80085158` -> **WorldMapProjectTriangleNormal** (permuter ZERO; the fix was to **remove** the `pC` local and reach the scratchpad through the view struct - a pointer local forces its `lui`/`ori` pair to the definition point and loses the delay-slot steal), `battle/misc4` `func_80079C24` -> **BattleRunActiveSlotScripts** (its stored body was also semantically wrong: `BattleTableEntry *` + `off` scaled by 0x170; then the neighbour's exact lever closed the last register - test `*pCursor` directly in the `while`, no pre-load), `battle/misc32` `func_800BDB74` -> **BattleStepEffectColorFadeTimer** (a second live task pointer as `pEffect` at the top of the function, before the first call, is what survives cse). Nearby parks advanced: movie/misc2 `func_80075D8C` 93.66 -> **99.14** (three levers: `FontPrintf` takes one argument; `j = entry` and `count = 0x1249` go INSIDE the do-while body so they hoist; hex/dec as if/else not a ternary), battle/misc32 `func_800BDC14` 91.60 (retail's stores go through the same `$s0` the call copies to `$a0`; cse forwards the copy). New wall with mechanism: `field/main/misc2`'s two parks - plain 2.7.2 emits `la` as ONE RTL insn so cse folds the pointer into `sym+disp`, while 2.7.2-cdk splits it into `lui`+`addiu` so cse cannot fold and the register IS kept, but cdk then merges all the `g_Scene` addresses into one base; retail mixes `$at`-macro stores with a kept pointer register and no compiler here emits both, so stop running `try_flags` on that unit. | 
| 115 | 6 workers (3 parks + 3 fresh/bare units) | +1 | 1107 | `battle/misc4` `func_80079AB0` matched -> the child's stream died upstream AFTER it committed, so the work merged fine and `make check` confirms it: the hint was right, its neighbour's lever closed it. Its remaining work is two parks (`func_80078998` 89.11%, `func_80078E24`) plus two untriaged stubs. Parks advanced with mechanisms: `movie/misc2` `func_80075D8C` 99.14 -> **99.66** (one local `pFileName = ArchiveGetFilePath(j + 1)` reproduces retail's `nop` in the `beqz` slot; the residual is now a single `move s0,s1` - `j = entry` must be before the loop for that, but then gcc carries `j+1` in a derived induction register), `world_map/misc8` `func_80085CDC` 89.31 -> **93.47** at retail's exact size (typing the sprite position fixed the `lhu`->`lh` delta outright; `GfxSetBaseMatrix(&D_80009C808)` instead of a local rematerialises the argument; the permuter found `cur = ang;` outside the inner `if`), `lib_gear/main` `func_801E1258` 44 -> **87.69** (two `RECT` locals reproduce the 0x78 frame; residual is offset-first `addu` operand order). Walls with RTL evidence: `battle/misc32` `func_800BDC14` - local-alloc's `combine_regs` merges the parameter pseudo into `$a0` because the call's argument copy is its last use, and retail's own object disproves the "cse forwarded the copy" theory (its `move $a0,$s0` precedes the stores, which still use `$s0`); `menu/misc9` `func_801DB920` 92.79 is instruction-identical at retail's size with three callee-saved register assignments swapped, and register choice does NOT follow declaration order under 2.6.0 (two very different orders gave byte-identical output). | 
| 116 | 6 park workers (one per mechanism) | +0 | 1107 | Below the 3-match rule, and it is the second sub-3 park round in a row, so **park rounds stop here**: every remaining park in these units is now pinned to an allocator/register argument rather than a missing shape. Advances worth keeping: `world_map/misc8` `func_80085CDC` 93.47 -> **97.70** (assign `i = 0` before `pCam` materialises for the save order; read `pScratch[i + 6]` inline or sched2 hoists it; the slew arms assign to `d`, not `cur`, which puts the result in `$v1` and frees `$a0` for `cur`), `menu/misc9` `func_801DB920` 92.79 -> **96.76** (the note was partly wrong: `idx` and `pQuantity` belong INSIDE the while body so loop.c hoists them - and gcc's global-alloc ranking `floor_log2(n_refs) * n_refs * 10000 / live_length` reproduces our build exactly, so no reachable live-range change produces retail's rotation), `lib_gear/main` `func_801E1258` 87.69 -> 88.98 with the old park's second loop proven semantically wrong, and its `addu` order IS source-controllable - **in the mirror direction of the documented lever**: retail's offset-first `addu` needs INTEGER sums (`dstRect.x * 2 + (u32)pDstBase`), every pointer spelling gives base-first. `battle/misc32` `func_800BDC14` is a local-alloc `combine_regs` tie: keeping the parameter live past the call does defeat the merge but costs a register (108 bytes, 70.36%). Preset data: `battle/misc4` cannot move to -G0 (it lifts `func_80078998` to 94.59 but drops matched `BattleQueueCommandFD` to 99.67). | 
| 117 | 1 permuter-ZERO port + 5 fresh-stub workers | +4 | 1103 | `field/misc6` `func_800A0C94` -> **FieldApplyActorPositionToMatrices** (ported the permuter's ZERO by hand, then removed its gratuitous index temp - what the match actually needs is three separate `(FieldActor*)(offset + (s32)g_FieldActors)` sums whose assignments sit between the stores), `battle/misc26` `func_800B3878` -> **BattleDrawScreenFade** and `func_800B36BC` -> **BattleStepScreenFade** (65 and 96 insns, the smallest fresh stubs left in the tree), `battle/misc14` `func_8008946C` -> **BattleBuildGrey80BlendSpriteColumn6**. New levers: the `do { } while (0)` wrapper can put EVERY other instruction in retail's order and then add one redundant load (field/misc6 `func_8009FB98` 63.17 -> 90.00, parked); and the permuter's own metric can disagree with objdiff on a whole function (field/misc4 reported `BETTER 1553 -> 1042` for a body that measures 94.16% under objdiff - reverted). **Failure mode worth the brief line it got**: a worker's in-place rewrite swallowed the matched `WorldMapSetSceneSpriteSetParents` between its target and the next `INCLUDE_ASM`; `check_bin`'s undefined reference caught it, objdiff never would have. |
| 118 | 6 fresh-stub workers | +0 | 1103 | No matches; the fresh pool at 300-470 insns does not close in a 30-minute window. Every park came out better diagnosed, and three are new walls with mechanism: `battling/misc6` `func_8007F258` needs **two symbol names at one address** (retail stores, loads, stores the same word at 0x80092718; `symbol_addrs` has no duplicate addresses and the array-decay lever cannot spell it), `world_map/misc1` `func_800740B8` is the documented bare-`ctc2`/`lwc2` GTE class (15 handwritten instructions, interleaved `lw $t4/$t5/$t6` + `ctc2 $0..$7` that no macro in inline_c.h emits), and `shop_menu` `func_801D05BC` proves the overlay's last stubs pass an **untruncated `y`** (`0x32 + FONT_LETTER_HEIGHT * i` with no `andi` in 407 instructions) which only a K&R/no-prototype declaration produces - and that is not expressible once the callee's definition shares the TU. Also fixed a stale park: `battle/misc21`'s `func_800A1B50` body did not compile (`pBlock->unk1[2]`; `BattleBlock.unk1` is now a scalar `u8`) so its 97.90% was measured on an older header - the two parks beside it have the same invalid spelling. Levers: `WorldMapEntry[]` indexing replaces `pWork + 0x464` (misc4 `func_800783E8` 91.53 with retail's exact size), the entry pointer must be re-assigned per copy run or gcc CSEs it into one register, and an unused `s32 tmp[8]` local is what reproduces retail's 0x50 frame in misc6. | 
| 119 | 2 permuter-ZERO ports + 6 naming workers | +3 | 1100 | Three ZEROs ported cleanly from the re-parked bodies, each teaching one lever: `battle/misc9` `func_8007E8AC` -> **BattleSetRowOpByte** - the whole edit was `D_800D3364[(u32)(idx + 0x140)]`, and the cast must wrap the **sum** (`u32 idx` or `(u32)idx + 0x140` both fall back to 95.38%), which broke a v0/v1 register mirror six hand attempts could not; `func_8007E98C` -> **BattleWordRegEqImm** - pure declaration order (`s32 value` before `u16 *pField`); `menu/misc4` `func_801CE660` -> **MenuAddUnk358Prims** - a **CFG-shape** lever: writing the twelve-call block twice behind an uninitialised `if (pData) … else …` changes the flow graph enough that the allocator picks retail's `$v1`, and gcc's cross-jump deletes the duplicate so nothing survives. Naming round: 20 names proposed, 7 applied (`BattleQueueCommandF9`, `BattleScriptScanForOpcode62`, `BattleRenderSpriteSlots`, `BattleTransformPartPositions`, `ShopMenuSetPolyGradientColor` + a new `symbol_addrs` entry, plus the three above); the rest are INCLUDE_ASM stubs (the tool refuses those by design) - **17 names sit in every report waiting for their function to match**. | 
| 120 | 6 lever-application workers | +2 | 1098 | `field/misc6` `func_8009FB98` -> **FieldScriptVMHandlerReloadSceneMapData** and `func_800A0EE8` -> **FieldScriptVMHandlerReleaseActorModel**; the first closed because the round-117 `do { } while (0)` had to wrap the **IP increment**, not the store - that gives retail's store order AND avoids the redundant third `lhu`. Levers proved and recorded on the five parks that did not close: `battle/misc9` `func_8007E740` 95.31 -> **98.75** (write the slot registers INLINE as `D_800D3400[arg1].unk10[p[1]]`; a `u32 *pRow` local stays mirrored), `battle/misc21` `func_8009EF3C` 94.94 -> **98.90** with the note's "six excess instructions" proven **stale** (retail is 648 bytes / 162 insns and the body now matches the multiset; what is left is the parameter/walker register pair swapped), `world_map/misc9` `func_80087C6C` 96.77 -> **97.30** at retail's exact size (the `do { } while (0)` must contain the two scratchpad stores **and** the following call - narrower or wider boundaries cost the base register), `movie/misc2` `func_80075D8C` (permuter run from the outside-init body, still 99.66: the residual is `combine_givs` deriving `j+1`, not a tie-break). DECISION: `menu/misc4` `func_801CE540` is **100.00% under gcc-2.6.0** and 13 of the unit's 14 functions stay 100 there, but `MenuUpdateSubMenuChoice` drops to 98.13, so switching the unit would take a matched function below 100 - that is operator-only, and a split buys one function. Its other finding is reusable: `extern s32 X[];` + `pCounter = X; (*pCounter)--;` is what produces retail's `lui/addiu` + `lw/sw 0(v1)` for a read-modify-write. | 
| 121 | 6 workers (4 park/lever units + 2 rewrite) + 4 naming workers | +4 | 102 | 1997 | `menu/misc7` **four matches** off ONE new lever: `func_801D433C` (write the accumulated x back into the parameter before the second `MenuSetQuadVertices`), `func_801D5BA4`, `func_801D51EC`, `func_801D53D0`. The lever: when retail has the counter's `addiu` in the branch delay slot of a `digits[..] != 0xFF` test and no latch copy, the source read-and-bumped in ONE expression (`digit = g_Menu->digits[i++]` with `for (i = 0; i < 9; )`) AND kept the x argument in a **separate running variable incremented at the end of the body** (`for (i = 0, x = a0; ...; ) { ...; x += 8; }`), which is what puts `addiu s1,s1,8` in the back-branch slot. Elsewhere 0 matches; 6 parks advanced and 2 new parks written, both at retail's exact size: `world_map/misc8` `func_80085418` 94.52 -> **99.73%** (840 B, one mirrored register pair - the next permuter target), `menu/misc12` `func_801E8F60` 96.65 -> **99.61%** (612 B = retail's size; residual is `move a1,s1` where retail has `andi a1,s1,0xFF`), `battle/misc15` `func_8008ADD0` 96.69 -> **98.37%**. `battle/misc18` rewrite: **20 of 21 live raw sites typed** (`D_800D2D6C` -> `BattleSlotState *`, the 14 `g_GameState + 0xNNNN` casts -> `chars[8].agility` / `gears[15].armorId` / `unk16BC[8].unk6 |= 0x800`, `D_800CCD72/CCCE8/CCE44` -> one typed table), all four functions still 100% + CHECK_BIN_OK. Naming: **+3** from the union of every NAME line ever reported (2154 unique addresses); the residue is address twins and stubs. |
| 122 | 4 permuter-apply + 1 rewrite worker | +2 | 102 | 1990 | Both from permuter leads, and the round's product is again the LEVER rather than the count. `psyq/libgpu_c` `SetFogNearFar` **MATCH 100%**: the permuter's winning edit was semantically bogus (`f = d / d`) and bought 0.07%, but reading it for the shape it wanted showed the real answer - **each shift chain must be one expression, not split statements** (`n = ((-a * b) / d << 8) / h;`), which is what keeps retail's separate `mflo`/`sll`; the old park note's "splitting the shifts fixed the reordering" was the wrong hypothesis and was why it was stuck at 98.66%. `battling/misc17` `func_8008EB4C` -> **BattlePlaySoundEffectById**: its PERMUTER_ZERO was a DECLARATION edit - `extern s32 D_80059488` had to be `volatile` so gcc stops hoisting the global load above the `lui/addu` argument setup (the signedness half of the candidate was a red herring: `volatile s32` matches, `u32` alone stays 73.33%). Six parks advanced hard: battle/misc19 `func_8009A258` 95.32 -> **99.68%** (five-step permuter chain; the load-bearing step is `short sum`, not `s32`), battling/misc17 `func_8008ED6C` 90.34 -> **99.43%** (a non-void return type fixes the arg-4 load order), psyq/libsn `PCcreate` 84.38 -> **99.38%** and `PClseek` 84.38 -> **91.67%** (the latter also fixed a **real bug**: the parked body's argument-shift asm ran ascending where retail shifts down), battle/misc15 `func_8008ADD0` still 98.37. `battle/misc27` rewrite: **four CLEAN** (`func_800B57E4` retyped onto the existing `BattleSpriteState` view - the header entry from misc28 was indexed, not re-declared). Naming: **+1**. Stubs 1094 -> 1092, live raw 448 -> 442. |
| 123 | 3 park-lever + 2 rewrite workers | +1 | 98 | 1945 | `world_map/misc8` `func_80085418` -> **WorldMapCheckTriangleEdgeSide** (MATCH, 840 B), closing the mirrored register pair with a new general lever: **when only one register pair in a load/compute cluster is mirrored, give the operand retail loads FIRST its own local declared just before the expression** - gcc then assigns registers in load-creation order (`entryZ = pEntry->unk10; z = entryZ - (pPos->z >> 12);`). The unit's last stub was also written and parked (78.64%, full structure map). The other two park workers closed nothing and both **date their walls**: `menu/misc12` `func_801E8F60` is a dead end, not a shape problem (7 more spellings - `(u_char)` cast, `u8` local in its own block, `s32` assigned-then-declared, `mode & 0xFF`, `do{ }while(0)`, `u_char mode` - plus `goto` and a duplicated if/else arm all emit `move`; a compiled probe shows gcc 2.6.0 proves `mode` in {2,3} in straight-line code and elides the narrowing; 2.7.2 is worse at 99.15), and `battle/misc19` `func_8009A258` (an 8-minute permuter run, 3886 iterations, base 10 -> 10, plus 15 shapes: any merged-accumulator spelling gives retail's two-address `addu d,d,s` but homes it in `a1`/`v1`; only the HImode two-statement form reproduces both `lbu` and then `sum` goes to `$a0`). Rewrites: `menu/misc7` **five of seven CLEAN** (16 raw UV stores -> `polysWindowBorderBottom`, `MenuCharacterWindow*`, a new `MenuGoldWindow` view) and `world_map/misc1` CLEAN on four (typed `POLY_FT4 D_8009C744[]`, `D_8009BE3C->pOt[]`, and a `WorldMapQuadPrim` 0x48-byte buffer). Live raw **442 -> 397**, stubs 1092 -> 1091, naming **+5** (unnamed C defs 102 -> 98). |
| 124 | 5 rewrite workers | +0 | 98 | 1913 | By design: the round paid raw debt on the five worst live files. Live raw **397 -> 365**, every rewritten function still `MATCH` at its original byte count and `make check` green on all eleven. `slus_006.64/system/temp2` is now **7 CLEAN** and its animation/part drawers read off the `SpriteFlagWord` byte view; `battle/misc22` typed all 8 sites, including `D_800D3344`/`D_800D39CC` as the pointers they demonstrably are, a `BattleMisc22PartSlot` 0x14-byte table replacing three per-field symbols, and a `LineScroll D_800C3DA0[]` that `include/system/graphics.h` already had; `battle/misc15` turned the two/three/four-slot prim-flag setters into ONE shared `D_800C3EAC->slots[arg0].unk3C` view (five CLEAN). The other two workers' residue is now **justified, measured walls** rather than unmade work. |
| 125 | 7 read-only naming workers + a tool fix | +0 | 1913 | **+14 named** (unnamed C definitions 98 -> 84) with all eleven outputs green. The round's real product is a **`name_addrs.py` bug fix**: the twin test matched an UNANCHORED `func_X\s*\([^;]*?\)\s*\{`, so a call site inside `if (func_X()) {` counted as a second DEFINITION and the tool refused every main-exe function an overlay calls - `SoundGetVolumeMode`, `SoundGetActiveMethodMask`, `SoundGetPendingSpuCommandType` and `GameRandomByteRange` were all blocked that way. Anchoring the pattern at column 0 (`(?m)^[A-Za-z_][\w \*]*?\b`) took this round's own 11 proposals from 1 applied to 9, and re-running the NAME union afterwards landed 5 more from earlier rounds. Functional names added include `MenuInitializeCamera`, `MenuUpdateAndRender`, `ArchiveLoadStagedFileIfChanged`, `GameRandomByteRange`, `FontSetPositionsByOffsetClamped`, `FontSetExternalBuffer`, `SoundGetVolumeMode`, `SoundGetActiveElementMask`, `SoundGetPendingSpuCommandType`. The naming workers also recovered a large block of DATA symbol evidence (pad-driver flag/handlers, the libgte matrix stack and its two error strings, the sound heap and RCnt2 tick counter, the debug-menu overlay names) which is committed in the reports but not yet applied. |
| 126 | 1 permuter-apply + 3 near-complete-unit workers | +2 | 1914 | Both from the "close a unit" push and its permuter feed. `battle/misc16` `func_8008DC34` -> **BattleDrawSpriteQuad**: the deeper sweep's new ZERO (score 10 -> 0) and a park round 110 had **dated as a wall** - the lever is a **parameter-copy**: retail's `addu s0,s4,s6` will not come out of any operand order, but `dy = arg2;` first and then `(s16)(dy + arg4 - 8)` stops gcc folding the two parameters into one commutative pair (the explicit cast form is required; `dy += arg4 - 8` is 4 bytes bigger and 97.05%). Into `brief.js`. `movie/misc2` `func_8007519C` -> **MovieCheckStreamFileHeader** (540 B): a size-check early-out is a bare `return;` in a NON-VOID function, so gcc leaves `$v0` = the `slt` result, which is exactly what retail returns. The other two near-complete units each left one precise park: `shop_menu` `ShopMenuSellMenu` 95.12% (2028 vs 2032 B - retail keeps `pItemSellQtys` in `$s7` and spills `itemCount` INSIDE the coalesce loop, so the ShopMenu version increments `itemCount` itself instead of reusing the twin's shape; a 4-minute permuter returned only a semantics change), and `gear_shop_menu` `func_801D18F8` 98.73 -> **98.98%** with a measured three-way table showing the pointer walk fixes the candidate count (frame 200 -> 192) and `setRECT(&sp20,…)` fixes `&rect`'s priority, but combining them re-ties the register file - so the next permuter seed must start from the **1580-byte** form, not the 1584 one. Stubs 1091 -> 1089, live raw 365 -> 378 (the two new parks), unnamed 84. |
| 127 | 3 park/lever workers (2 closest parks + 1 fresh stub) | +0 | 1914 | No matches, and the round is worth keeping for three measurements. `gear_shop_menu` `func_801D18F8` went 95.95 -> **98.73%** by rebuilding the 1580-byte pointer-walk seed and running the permuter FROM IT (`base=1041 best=378`, which found a `u32 polyColor = 0x1000;` hoist for the `tensDigit` call's last argument); the frame is now retail's 192 with retail's spill order, and what is left is a `li t2,0xc3` reloaded inside the k-loop where retail hoists it, plus two equal-priority register swaps. `shop_menu` `func_801CEB3C` was reconstructed from scratch to **99.47%** with retail's exact byte count and frame - the whole residual is which of two live induction variables takes `$s6` vs `$s5` - and its third parameter is genuinely unused by retail (`s32`, not `u_char`, or gcc masks both at entry). `battle/misc11` `func_80080C94` 86.16 -> **88.65%** with four decoded facts, including that the function is `void` (retail never copies the flag to `$v0`) and that `slot = i++` in one statement is what reproduces retail's `move s1,s0` + `sb`. |
| 128 | 1 jump-table family experiment + 2 park workers | +1 | 1909 | **The jump-table family theory is REFUTED, with a structural proof** - which closes a question that has blocked ~110 stubs since round 56. cc1 emits `.align 3` (8-byte) before EVERY C jump table; retail's tables in `battle/misc11` sit at unit-relative +0x1B4/+0x1D4 (both 4-mod-8) and only the first is 8-aligned. `misc11` owns `.rodata` 0x38C..0x600 and it is jump tables only: one 101-word table (0x194) then seven 0x20-byte tables, ending exactly at +0x274. Because 0x194 is not a multiple of 8, a pad is forced from the SECOND table on, so unit `.rodata` ends at 0x278 instead of 0x274 and every `.text` address shifts +4 (measured: `battle.bin` first diff at 0x4, retail 0x80074630 vs ours 0x80074634; `culprit.sh` then reports 851 symbols, which is the shift, not 851 regressions). Converting the whole family in address order cannot fix it, and the pre-existing park note's "only convertible with a change that makes the offset 8-aligned" is **wrong - no such change exists inside the unit**. Both tables here are 8 words for 8 cases with no trailing pad word, so word counts were never the problem either. Plus **+1 match**: `gear_shop_menu` `func_801D18F8` -> **GearShopMenuRenderSellItemRows** (394 insns, 1576 B), taking that overlay to **ONE stub from complete**, and `shop_menu` `func_801CEB3C` 99.47 -> **99.89%**. |
| 129 | 1 near-complete-unit worker + 1 tool worker | +0 | 1909 | No match, but the tool worker turned round 128's structural proof into a **work queue of 108 stubs**: `tools/scripts/jtbl_align.py` reports, per unit, which jump-table stubs can ever be converted to C (`battle 42/50`, `battling 13/15`, `world_map 29/41`, `menu 10/12`, `field 6/6`, `slus_006.64 4/8`, `lib_gear 3/3`, `shop_menu 1/1`, `movie 0/5`). Validated on both known cases - `battle/misc11` reports PAD+4 for every table after the first, `battle/misc12` reports OK for the three round 64 converted - and it corrected my own model twice: alignment is **range-relative, not absolute** (gas `.align` is section-relative and the linker places each unit's object with `SUBALIGN(4)`, so the tool works in FILE offsets and `misc11`'s `jtbl_8006FE7C` at +0x0 is fine despite being 4-mod-8 in vram), and the vram must not be derived from the yaml's `vram:` line (`slus_006.64`'s segment starts at 0x800, which produced `PAD+2048` for `psyq/libc`'s `Sprintf`). `shop_menu` `func_801CDD14` - the last writable stub in that overlay - was written to **97.44%** and parked with a complete shape list (frame 200 = retail's, `li s2,0x28` CSE'd, bzero delay slot correct; residual is stack-slot ORDER plus a missing `andi` on the `SetVertices` y that `func_801D05BC` shares - one wall, not two). |
| 130 | 5 jump-table workers (advance-screened) | +3 | 1909 | First jtbl round since round 64 and it works: `battle/misc18` **`func_80095D4C` -> BattleRollSlotConditionAction** (JTBL, 716 B = retail's size, `check_bin` OK) and **`func_80096018` -> BattleSetSlotConditionActionValue** (RELOC, 1148 B), `menu/misc10` **`func_801E3088` -> MenuFreeSubMenuPanels** (JTBL, 312 B, instruction-identical). Stubs 1088 -> 1085. Three round-129 assumptions were corrected by measurement, and two of them were MINE: (1) **jump tables do NOT need address-order conversion** - cc1 emits a switch table as `.rdata` INLINE, immediately after the function's own `jr`, so each table lands at its own function's text position (already retail's `.rodata` order), and two workers proved it independently by converting out of order with `check_bin` OK; (2) **the screener needs a word-count filter** - `menu/misc10` `func_801E3ECC` passes `jtbl_align.py` (pad +0) but its table is 8 words for a 7-case switch (`sltiu v0,s0,0x7`, entry[7] = 0), so a C switch emits 7 words and the unit's `.rodata` ends 4 bytes early: PADWORD, exactly round 56's second check, still missing from the tool; (3) **read the case COUNT from the code, not the word count** - `addiu a0,a0,-1 / sltiu a0,0xD` means min case 1, max 13, so 13 entries and the source needs the empty `case 8: ... case 13: break;` labels or the table is 6 words short and every later table shifts. Also recorded: the permuter's own score is not objdiff's score in `battle/misc30` (its two "best" candidates measure 5 points WORSE under `diff_func.sh`; use objdiff's per-instruction JSON alignment, not the text diff). |
| 131 | 1 tool worker + 4 jump-table workers | +4 | 1909 | The best matching round in many sessions, and it also closed the screener. **`battle/misc12` converted THREE switches** (`func_80082D4C` -> BattleAdvanceSlotMenuStateCounter, `func_800830A8` -> BattleAdvanceSlotMenuStateSlotFlags, `func_80083340` -> BattleAdvanceSlotMenuStateSubMode, all JTBL, 560/664/576 B, `check_bin` OK after each) - and that is the unit round 64 lost three conversions in, so the per-conversion gate did its job. `world_map/misc7` `func_800811C0` -> WorldMapUpdateSlotState (65-case switch, 100.00% + CHECK_BIN_OK). Stubs 1085 -> 1081. `jtbl_align.py` gained the PADWORD/SHORTWORD filter with two corrections that matter: **the case count is the `sltiu` immediate N itself** (the bound applies after retail's `addiu -M`, so cases are M..M+N-1 and a switch emits N words - `N-M` would have wrongly rejected `func_8008BB40`'s 13-word table), and **a trailing word is only retail's pad when cc1 cannot regenerate it** (`align8(off + 4N) != end` or nothing 8-aligned follows), which separates `func_8008A72C`'s 66-word/65-case table (fine) from `func_801E3ECC`'s 8/7 and `func_800A6884`'s 6/5 (PADWORD). Dispatch is now anchored on `lui %hi(jtbl_X)` rather than `lw %lo(...)($at)`, which took NOCASE from 12 to 3. Queue after filtering: `battle 32/45`, `world_map 26/40`, `menu 7/11`, plus the smaller binaries. |
| 132 | 5 jump-table workers | +1 | 1909 | `battle/misc19` `func_8009B684` -> **BattleSetCommandCodeForSlotKindFlag** (JTBL, 1088 B = retail's 0x440, 272/272 instructions, `check_bin` OK) and `func_8009BAC4` written from nothing to **99.72%** at retail's exact 720 bytes (residual: the destination of four `mfhi` in the `% 100` sequences). Four other units each parked a fully-decoded switch body (see detail) - the conversions are now structure-run-limited, not screener-limited. |
| 133 | 5 jump-table workers | +0 | 1909 | Zero matches, and the round's lesson is about MY selections, not the workers': I ranked the jtbl queue by **table word count** and it has nothing to do with body size. Every assignment turned out to be a 300-1200 instruction function (`field/misc4` 479/426/405/273; `lib_gear` 387/1238), so four workers spent their hour on one function each and parked. **The queue must be ranked by BODY INSTRUCTIONS and by the unit's ADDRESS ORDER**: `battle/misc13` and `world_map/misc4` cannot convert any jtbl stub until their first, largest one is done, because a C table is emitted before a still-asm-owned table in the same object. One worker escalated rather than guess and was redirected - the right call, and the redirect then found that its list was wrong too (`lib_gear`'s two "unparked" stubs are both parked: 92.95% and 99.87% with four permuter runs already). |
| 134 | 5 jump-table workers (queue ranked by BODY SIZE) | +5 | 1909 | **Best round of the session, and it vindicates the round-133 lesson: rank the jtbl queue by body instructions, not table words.** Five matched: `world_map/misc1` `func_80073398` -> **WorldMapSetPositionFromState** (44 insns), `battling/misc5` `func_8007D190` -> **BattleEffectAddSlotFromVector** (JTBL) and `func_8007D0B4` -> **BattleEffectJitterSVector** (100.00%, 55 insns), `battling/misc7` `func_80080570` -> **BattleReloadSlotEntries** (JTBL, 53 insns), `battling/misc18` `func_8009031C` -> **BattleUpdateWorkState1** (JTBL, 132 insns). Stubs 1081 -> 1076, +5 names, all eleven green. The batch shape holds: **a worker closes 1-2 conversions when the stub is under ~130 instructions and parks it when it is over ~200.** |
| 135 | 5 jump-table workers (body-size ranked) | +2 | 1909 | `battle/misc5`'s **twin pair** both converted: `func_80079ED8` -> **BattleSlotByteVarGetSet** and `func_8007A280` -> **BattleSlotWordVarGetSet** (936 B each = retail's size, 234 instructions, RELOC+JTBL, `check_bin` OK). Stubs 1076 -> 1074, +2 names, eleven green. Four workers parked bodies at 87-97%, and every one of them hit the same class: a scheduler/allocator ordering that survives its own permuter run. Two screener-level facts matter more than the count: **`convertible.py`'s BLOCKED is a false positive because it latches the FIRST `.size` in the `.s`, which is the `jtbl_` one at the top of the file** (two independent workers found this), and **"jtbl-OK" is not sufficient for a lone conversion** - `world_map/misc12` `func_8008E4F4` is at 96.21% with every instruction right, but converting it ALONE puts its C table at `.rodata+0` while `func_8008E190`'s still-asm tables sit at +0 and +0x18, so `check_bin` says MISMATCH: the two must be converted together, in address order. |
| 136 | 5 deep jump-table workers | +1 | 1909 | `world_map/misc11` `func_8008B644` -> **WorldMapUpdateActorSlot** (JTBL+RELOC, 319/319 insns, 1276 B, `check_bin` OK), and its neighbour `func_8008B2BC` 87.47 -> **97.39%**. Stubs 1074 -> 1073. Two workers were **killed at 30 minutes because I briefed `minutes: 60` but launched without `options: {timeoutMs: 3600000}`** - the brief told them to plan for minute 52 while the harness cap was 30, so both died mid-park with uncommitted C (`menu/misc10`, `battle/misc15`) and that work was reverted. Every launch from here passes the explicit timeout. |
| 137 | 5 deep jump-table workers (60-min budget, explicit timeoutMs) | +2 | 1909 | Two matches and a **harness bug fixed**: `world_map/misc8` `func_80083264` -> **WorldMapUpdateSpriteSetPrims** (JTBL, 620 B, 155 insns) and `battle/misc4` `func_800793F0` -> **BattleDispatchCommandRecords** (644 B = retail's `0x284`, 17 case labels, `check_bin` OK). Stubs 1073 -> 1071, +2 names, eleven green. The 60-minute `options.timeoutMs` is what let all five finish and park cleanly this time. |

Round 102 detail:
- `battle/misc23` `func_800A64E4` (98.71%) is the cleanest remaining park in the tree: 232 instructions, same frame, same registers, every load/store/displacement/biv increment in retail's order, and ONLY the operand order of 18 address `addu`s differs. Proved futile: all indexed forms, both embedded-assignment forms, per-store pointer locals, `mem_addr` integer sums, and an offset variable instead of a biv; `try_flags` says the preset is right (2.6.0 -O3 is the best).
- `battling/misc8`'s rewrite is the pattern to copy for text/screen globals: `D_8009A0D8` became `Misc8SlotEntry[0xF8]` with a `RECT rect` head, `D_80095498` became `DR_MOVE[]`, `D_80092734`/`D_80092738` became `BattleTextList *`, which killed cast-and-offset sites wholesale.
- `battle/misc34` recorded a load-bearing alias: `pSelf = pTask` (a same-type copy) is the only spelling that reproduces retail's `move s2,s1` in a `jal` delay slot; the faithful one-pointer form merges to one register under cdk and measures 91.43%.


Session note (rounds 71-102, one orchestrator, 30 matches, ~750 raw lines):
- **Stubs 1171 -> 1132, raw 2520 -> 1747, `make check` green on all eleven at every commit**, progress published five times. `member_change_menu` is the first overlay at **0 stubs**. Overlay state now: gear_shop_menu 2, shop_menu 4, movie 15, lib_gear 20, menu 115, field 123, battling 128, world_map 145, slus_006.64 232, battle 348.
- The session's product is levers, not volume. In the order they earned their keep: the EMBEDDED-ASSIGNMENT lever (retail's base-first `addu` only when the sum is an assignment's RESULT) broke a 99.49% park two workers had called uncontrollable; the array-plus-`volatile` declaration materialises an address for a block's first read-modify-write; the `do/while` block forces a store cluster OR a load order and may be EMPTY; a partial VIEW struct per unit is the cheap way to type a shared actor, and when two units need the same view ONE worker owns both plus the header and must revert on any regression; the `-G` flag only matters for a unit that DEFINES small data (a unit whose data is all externs is preset-insensitive); and `check_bin` failing on a switch conversion can be the CASE ORDER, not the table.
- Walls were closed with dates: `movie/misc1` func_80072D84 (live-range), `battle/misc7` func_8007B9C8 (one register decision), `battle/misc13` func_80085D34 (pointer form only), `gear_shop_menu` func_801D2B74 (register coincidence), `lib_gear` func_801DFAC4 (pressure cascade), `menu/misc10` func_801E3ECC (jump-table pad word), `menu/misc11_r2` func_801E5B88 (needs a 3-way preset split; DECISION logged). Do not re-brief any of these without a NEW lever.
- **Next session's first moves**: (1) `permute_sweep.py --report` for ZEROs - the 3-minute pass is the only source of near-certain matches left, and this session's ZEROs all ported first try; (2) re-test old walls against the lever list above, since round 97 proves a wall is only a wall for the current toolset; (3) rewrite rounds still pay ~20 lines each with no risk (`world_map/misc5` at 61 is a measured wall - its layer stores and the 0x1F800000 scratchpad must stay raw); (4) the remaining 1132 stubs are almost all 150+ instructions, so a matching round is now 1-3 deep workers on fresh stubs, not a broad fan-out.
- Two orchestrator mistakes to avoid, both recorded above: never `git commit --amend` after a push, and never run `make progress-publish` between `clone.sh new` and the merge (the modified `reports/progress.json` gets swept onto worker branches and conflicts).
- Harness facts confirmed this session: workers hit `cwd does not exist` instantly when a clone tag is mistyped (no work lost), a `permute-brief.js` brief given to a rewrite worker can be STEERED mid-run, and the `**NAME func_X ...**` bold form is as common as the plain one when collecting names.

Round 121 detail:
- **`diff_func.sh` (report mode) is not a sufficient gate in `XenoBattle260` units.** `battle/misc15` measured four cleanups at MATCH + 100.00% under objdiff that emit `addu v1,s1,v1` instead of retail's `addu v1,v1,s1` in matching mode (`(D_800C3EAC + arg0 * slotStride)[0x3C]` rewritten to `D_800C3EAC[arg0 * 0x40 + 0x3C]`); `check_bin.sh battle` caught it and objdiff never would. Two lessons: the embedded-assignment lever cuts BOTH ways (that rewrite is its reverse direction, and the worker's original spelling was the one that matched), and every raw-site cleanup in that unit must be proved with `check_bin`, not `diff_func`. The same worker also hit the documented `*/`-in-a-park-note trap, which report mode cannot see because it never compiles the `INCLUDE_ASM` path.
- **Park rounds cost raw lines.** The grep total went 1544 -> 1997 because five workers committed new parked bodies; live raw (comments stripped) went 426 -> 448. A park-heavy briefing therefore owes a rewrite pass on the same files, and the live number is the one worth watching.
- **A park body can be STRANDED.** `menu/misc12`'s `func_801E8070` had only its `INCLUDE_ASM` left: the two long notes under it describe `func_801E86C8`, and the body itself is gone - an earlier round's slicing edit swallowed it. Neither `check_bin` nor `lint_sources` can see a body that lives in a comment. Confirm the body exists before briefing a park.
- **The naming backlog is nearly drained: 100 unnamed C definitions in 62 files.** The union feed (2644 NAME lines on disk, 2154 unique addresses) now lands ~3 names because the residue is address twins (`func_800925A0` and `func_80093740` are `INCLUDE_ASM` stubs in `world_map/misc12`/`misc13`, so the tool refuses) and stubs. Build the union with THIS round's proposals first: keeping one line per address from an alphabetically ordered union let an older, worse name win three times.
- **Run `convertible.py` on the exact stub, not the unit.** `menu/misc3`'s `func_801CA750` was briefed as a park and is BLOCKED (92 insns after `.size`, owns `jtbl_801C50C8`); `func_801C9270` is a pure schedule mirror at identical multiset (9 shapes flat); `func_801C90B0` is a GIV tie the permuter could not move (base 80 -> best 80, 1621 iterations). `world_map/misc8` `func_80085CDC` is now measured as not source-controllable (`s16 ang` pins ang->$a0 in every variant; `s32` scores worse).
- **A cast on an already-bare symbol does not defeat cse's store-to-load fold** (`*(s16 *)&D_8009BD3A` scores identically to `D_8009BD3A`), so that is not a typing lever - the blocker has to be a real memory barrier in the source.

DECISION: `menu/misc12` `func_801E8F60` sits at 99.61% whose entire residual is one `andi a1,s1,0xFF`, i.e. retyping the final argument `u8`; left as a park rather than applied because the worker ran out of budget and the body is still an `INCLUDE_ASM` stub. No preset changed, nothing operator-only taken.

Round 122 detail:
- **A `volatile` on a global is a first-class lever.** `battling/misc17` `func_8008EB4C` closed from a park whose permuter score was 0 and whose entire winning edit was `extern s32 D_80059488;` -> `extern volatile s32 D_80059488;`: gcc was hoisting the load above the `lui`/`addiu` argument setup. Before writing a park note that blames an allocator tie on a global read, try `volatile` (and check which half of a permuter candidate is load-bearing - the `u32` half here changed nothing).
- **Retail's separate `mflo`/`sll` mean the whole shift chain was ONE expression.** `SetFogNearFar` had been at 98.66% for rounds because a note said splitting the shifts into statements fixed a reordering; folding them back (`n = ((-a * b) / d << 8) / h;`) matched on the first try at retail's 66 instructions. A park note is a hypothesis: when a permuter candidate is semantically bogus, read it for the SHAPE it was reaching for.
- **A park body can carry a real bug, not just a shape.** `psyq/libsn` `PClseek`'s argument shift ran ascending (`$5,$4 / $6,$5 / $7,$6`) where retail shifts down; reversing three moves took it 89.44 -> 91.67% before any permuter work. `report` mode compiles only C, so a parked body is never checked against retail's actual register usage until someone reads it.
- **The `select` wall, now documented in `psyq/libsn`:** retail emits `beqz v0 / addu v0,v1,zero / addiu v0,zero,-1` with the branch operand and the copy source in DIFFERENT registers. Every honest select form (ret-then-if, if/else, ternary, separate return) folds the condition copy-propagated and lands at 84.38% with `li v1,-1` after the `jr ra`; the permuter's `value = status; status = value != 0; if (status) value = -1;` dance gets 99.38% but tests `$v1` instead of `$v0`. Three permuter runs and six shapes failed to close it. `PCopen` and `_SN_read` in the same unit are the same wall.
- **`battle/misc19` `func_8009A258` (99.68%, 124 B = retail's size) is one register from a match**: sum's home is `$a0` (lowest free) where retail kept it in `$v0`. Recorded as tried-and-rejected: `u8 sum` 95.65, `u_short sum` 99.03, s32 with the byte pushed into a separate statement 99.03, `value = p[0x14]` first 98.39, declaration-order swap (no change). The chain that got there is worth copying: duplicate the store into both arms of an if/else to stop gcc coalescing a local into the sum pseudo, then narrow the type (`short`), then make the condition read the value twice.

DECISION: `battle/misc19`'s two 100.00%-under-2.6.0 parks still need a five-file per-file split for two functions, and `menu/misc12` `func_801E8F60`'s residual is one `andi` - both stay logged, not taken unattended. No preset changed this round.

Round 123 detail:
- **STOP RULE FIRED: park-lever rounds stop.** `menu/misc12`, `battle/misc19` and round 122's `psyq/libsn` all ended pinned to an allocator argument, and this is the second sub-3 round in a row on that kind of brief. The deeper permuter pass (`--minutes 10 --retry --max-base 120`) produced **0 ZERO and only 3 weak BETTER** (165/175/165 -> 150/165/160) over 317 bodies, against round 122's 1 ZERO + 8 BETTER: the park pool is exhausted under the current toolset. Rewrite rounds are the reliable product now.
- **Diff-layer trap worth a brief line: `gcc 2.6.0` exits 33 on a mid-block declaration, and `diff_func.sh` then prints ONLY the `objdiff:` line** - no diff, no `MISMATCH`, exit 33. A worker can read that as a match. Declare all locals at the top of the block, and treat a lone objdiff line as a compile failure.
- **The scratchpad cannot be typed, confirmed twice.** `(long*)((u8*)pRot + 0x48)` -> `+0x4C` in `world_map/misc1` stays raw: a typed scratchpad view turns retail's absolute `sh`/`lw` into register-relative form, and declaring the absolute symbols the retail object names (`D_1F800048`/`D_1F80004C`) makes it worse. Same conclusion `world_map/misc5` and `misc9` reached.
- **A typed pointer sum is not interchangeable with array indexing.** `world_map/misc1` `func_800737EC` (99.91%) needs `pPrim = (WorldMapQuadPrim*)(i * sizeof(WorldMapQuadPrim) + (s32)pBase)` - integer arithmetic; `&pBase[i]`, `pBase + i` and `i + pBase` all emit `addu s0,v0,s2` where retail has `addu s0,s2,v0` and drop it to 99.83%. Recorded in a comment at the site.
- **Two shared-header fields are load-bearing opaques.** `menu/misc7` `MenuSetCharacterWideNumberDigits` reads `g_GameState.characters[id]` at +0x44/+0x48, inside `GameCharacter`'s `unk2D[0x1F]` blob; `MenuSetGearWindowContents` has one raw byte at `g_Menu->unk358 + 0x2AE0`. Both stay raw: splitting the blob is a shared-header edit, and a 0x2AE0-byte view for one byte is scaffolding.

DECISION: nothing operator-only taken this round. Logged for the operator: `menu/misc12` `func_801E8F60` is now PROVEN unclosable under gcc 2.6.0 by argument narrowing (the compiler eliminates it in straight-line code), so its 99.61% park should not be re-briefed without a new compiler-level lever; `battle/misc19`'s two 100.00%-under-2.6.0 parks still cost a five-file split for two functions.

Round 124 detail:
- **The rewrite pool is nearly drained, and what is left is now documented per site.** `system/temp1` reverted every remaining site: typing `SpriteDataI3.field_0x18` as `RECT*` costs 81.22%/100 B because storing through the `void*` field makes gcc alias-conservatively reload `field_0x7C` before each statement (retail's three `lw v0,124(s0)`), and `dirTransforms[i].field` measures 98.29% with five `addu` operand flips - the `(i + pBase->pDirTransforms->dirTransforms)->field` spelling is the ONLY one that gives retail's index-first `addu`. `field/main/misc` kept its three `Copy*` helpers because a typed member assignment reads the volatile `g_FieldActors` twice per statement (80.43%), and `SpriteClearDirTransforms`-style accessors must stay raw for the same reason. Two of those are header changes with cross-unit blast radius, so they are recorded, not attempted.
- **A byte offset held in a saved register cannot be typed.** `battle/misc15` `BattleQueueCommandF3WithSlotMask`: `BattleTableEntry D_800CCD1C[]` + `for (i = 3; i < 11; i++) D_800CCD1C[i].flags` keeps retail's `$at`-based address form but drops the `idx = i + 3` `addiu` (81 vs 82 insns, 4 bytes short, unlinkable), and `[idx + 3]` is far worse (gcc decomposes `(idx+3)*0x170` into three shifts and a subtract). Retail accumulates a byte offset and re-forms the base through `$at`, so `D_800CCD1C` stays `u8[]` in that unit.
- **A repeated inline index expression is load-bearing.** `BattleActivateTwoSlotCommand`'s four `D_800D2CE0[arg2 * 2 + arg1]` uses must stay inline: a typed 2-column array measures 89.49% (408 vs 392 B) and a named `s32 idx = arg2 * 2 + arg1` local 77.41% - the repeated expression is what makes gcc CSE the byte offset into the register retail then reuses as `addu at,at,a1`.
- **Typed-table residue reads 99.x% in objdiff and is still a match** (`battle/misc22` `BattleTransformPartPositions`: 100 instructions / 404 bytes, identical count, only the relocation NAMES changed). `check_bin` proved it. This is the sanctioned RELOC case, not a near miss.

DECISION: nothing operator-only taken. This round confirms the file's own stop rule: park, preset and rewrite pools are all exhausted under the current toolset, so the next rounds are naming plus whatever the permuter sweep still produces.

Round 125 detail:
- **The naming backlog was mostly a tool bug, not a naming problem.** With the anchored definition test, `name_addrs.py` stops inventing address twins from call sites. The remaining SKIP reasons are genuine: `LoadAverageShort12`/`LoadAverageShort0` ("new name already used" - `libgte.h` already declares them, the same wall round 109 hit and hand-applied), and `func_801CA8C0`-style stubs.
- **`git stash` is a trap in this session, twice.** A stray `git stash -q` + `git stash drop -q` discarded one uncommitted hour of renames AND the tool patch, and the next `make check` was green because nothing was left to break. Recovery is not a stash pop: re-running `rename_sym.py` + `name_addrs.py` on the same names file reproduces the result exactly (the tools are deterministic and self-skipping), which is why no work was lost. Do not stash in this repo while clones are out - commit or re-run.
- **Workers over-refuse psyq names by following the brief's own rule.** The brief says a psyq function with no XDEF gets `NAME_UNKNOWN`, and the workers complied even where the mechanism was unambiguous (`func_80045D5C` is a textbook OT DMA6 transfer with CHCR polling). For internal SDK helpers a descriptive mechanism name is worth more than `func_`; that is an operator call recorded here rather than argued round by round.
- The DATA evidence has real value for a later round: `D_80056414` pad-init flag, `D_8005A200` the 16-byte intrp entry, `D_80056D30` the 20-entry MATRIX stack with `D_80056FB0`/`D_80056FE1` as its two error strings, `D_80065B0C` the 0x6300-byte sound heap, `D_8001833C..0x80018390` the debug-menu printf formats. Applying them means editing declarations across overlays, so it is a rewrite-sized job, not a rename batch.

DECISION: none of the three operator-only actions taken. Logged for the operator: the psyq `SetTexWindow`/`SetDrawArea`/`SetDrawOffset` renames are correct and were re-proposed this round, but they cannot land without either (a) deleting the overlays' own `extern func_800453AC` declarations plus a `linker/undefined_funcs_auto` adjustment, or (b) settling `libgpu.h`'s `u_short *ofs` parameter type (`lhu` vs retail's `lh`, 92.94%). Both are cross-cutting edits.

Round 126 detail:
- **A park note that dates a wall is a hypothesis, and the permuter can still break it.** `func_8008DC34` was closed as "`addu s0,s4,s6` canonicalised after expand, same under 2.7.2" in round 110. A 10-minute sweep run scored it 0 and the whole edit was the parameter-copy lever. Corollary for next session: re-run the sweep over dated walls; the pass that closed this one was `--minutes 10 --retry --max-base 120`.
- **A non-void function whose size-check "returns" a value gcc already has is the shape retail wrote.** `movie/misc2` `func_8007519C`: `extern void Vsync` -> `extern int Vsync` plus `if (size != 0x160) return;` and the else-if chain gives retail's block rotation for free (the state-1 body lands after the state-2 `return -1`). Two separate recorded residuals dissolved at once.
- **The permuter's seed form decides what it can see.** `gear_shop_menu` `func_801D18F8` has two forms - 1584 bytes (98.98%, `&rect` spilled) and 1580 bytes (95.95%, frame and spill order correct but the register file re-tied). Four permuter runs all started from the 1584 form and therefore never explored the 1580 neighbourhood. Record the seed's byte count in a park note, not just its percentage.
- `movie/misc2` `func_80076488` is parked at 82.97% at retail's exact size and frame (1076 B, 0x308).

DECISION: nothing operator-only taken. `ShopMenuSellMenu` (95.12%) and `func_801D18F8` (98.98%) are the two closest parks in near-complete overlays and both are now one allocator decision from matching; they are the natural first briefs for the next session, with the seed-form caveat above.

Round 127 detail:
- **Park-lever rounds are saturated for the third time; stop briefing them as the main body of a round.** Three workers, three precise one-decision residuals, zero matches - the same shape as rounds 116 and 123. The two candidate parks are now at 98.73% and 99.47% with every remaining delta pinned to an allocator/register-priority argument, which is permuter territory with a *new seed* or nothing.
- **A park note should record the SEED'S BYTE COUNT, not just its percentage.** `func_801D18F8`'s four earlier permuter runs all started from the 1584-byte form; the 1580-byte form the worker rebuilt reached a neighbourhood the earlier runs could never see (98.98 -> 98.73 while adopting retail's frame, then the permuter found the hoist). Two forms of one function can be four percentage points apart and still be different search spaces.
- **An unused parameter must keep its real width.** `func_801CEB3C(itemIndex, scrollOffset, u8 *pUnused)`: declaring the first two `u_char` makes gcc emit an `andi` pair at entry that retail does not have. "Unused" does not mean "any type".
- **`rename_sym.py` + `name_addrs.py` cannot rename a DATA symbol at all**: they require a `symbol_addrs` entry (or a C *function* definition) for the old name, and the 74 DATA names recovered in round 125 all skip with "not in any symbol_addrs file". Renaming data needs either a new tool or a hand job per binary - both tools would otherwise be the natural place.

DECISION: nothing operator-only taken. The next round is deliberately NOT a park round (see the stop rule): it runs one bounded experiment on the jump-table family problem (a BLOCKED jtbl stub converted together with its adjacent sibling, judged by check_bin) plus the two closest parks only as secondary briefs.

Round 128 detail:
- **`gear_shop_menu` `func_801D18F8` needed FOUR changes and three of them were misdiagnosed in the park note** (98.73 -> 99.31 -> 99.33 -> 99.85 -> 100.00): (1) `places[j]` indexed, not a `s32* pPlace` local - the pointer walk makes gcc form an absolute frame base with a 0-offset load where retail has `addiu a1,fp,0x10` + `lw 0x28(a1)`, and moves `addiu a1,-4` out of the delay slot; (2) compute the argument from the frame, `SystemDecodeGlyphCodesN((u16*)((u8*)&sp20 + 8), ...)`, because retail computes `addiu a0,sp,40` while the pseudo form gives `addiu a0,fp,8`; (3) the `li 0xc3` hoist is NOT a scalar local (93.4-93.9%, it steals an s-register) - it is the permuter's DOUBLE STORE, `[8+k*2] = 0x10; [8+k*2] = quotient + [8+k*2];`, which gcc merges back to retail's single `sb` and still leaves 0xC3 in `$t0`; (4) the `pWorkspace` `$s7`-vs-`$s6` "allocator tie" was not a tie at all - it was a stray `do { ... } while (0);` around the `tensDigit` call, and removing it was `PERMUTER_ZERO` (base=60 best=0). The park's "retail is 1572 bytes" was stale; retail is 1576.
- **`shop_menu` is one match and one BLOCKED stub from finished.** `ShopMenuSellMenu` is **BLOCKED** per `convertible.py` (508 instructions after `.size`, owns `jtbl_801C5028`) so it is unlinkable and its 95.12% park should be left alone - the earlier "don't touch, it's close" note was wrong. `func_801CDD14` is the only remaining writable stub there.
- **A loop-carried IV tie can be decided in the `for` init clause.** `func_801CEB3C` 99.47 -> 99.89 by initialising the column base **in the outer loop's init clause and last in it** (`for (i = 0, j = 0, x = 0x49; i < 0x10; i++)`); as a statement before the loop, or listed first in the clause, the preheader `li` lands in the wrong slot. Two permuter runs (5,337 iterations) and four declaration-order variants then changed nothing, and `try_flags.sh` confirms 2.6.0 -O2 -G8 is the best preset (99.89 vs 98.95 for 2.7.2), so the last 0.11% is a genuine 2.6.0 allocator decision.

DECISION: nothing operator-only taken. The jump-table family question is now answered rather than pending: only units whose tables are all 8-aligned can ever convert a `switch` to C. A screener that reports, per BLOCKED jtbl stub, its table's alignment inside the unit's `.rodata` would turn that into a work queue, and is the next natural tool job.

Round 129 detail:
- **Two corrections that matter for anyone re-deriving this**: (1) a jump table's alignment is decided relative to the unit's `.rodata` RANGE, not to its vram address - `linker/<bin>.ld` places the unit's object at the yaml offset with `SUBALIGN(4)`, so a table at range +0x0 is fine at any vram, and vram-based reasoning reports a phantom `PAD+4` for `battle/misc11`'s first table; (2) `slus_006.64`'s yaml `vram:` is 0x800 short of the real base, so offsets must come from the blob's own file-offset column.
- **`shop_menu` `func_801CDD14` (97.44%) records three real levers** from its own measurement: `new_var = 0xD` before the `digitGlyphs[8]` store with `0x32 + (new_var * i)` in the first `ShopMenuSetVertices` y is what CSEs the `0x28` rect width into `$s2` and grows the frame to retail's 200; `pItemAffordable[i] = FALSE` must come BEFORE the `bzero` (it fixes the delay-slot order); and the price needs **two** variables (`price` for the digit loop, then `itemPrice = price` after the table load) or retail's spill-and-reload for the affordability compare never appears (one `price` gives `slt v0,t1,s4`, 2 instructions short, 90.38%).
- The function is the shop twin of the now-matched `GearShopMenuRenderSellItemRows`, which is the shape reference for all of it - a matched twin in a sibling overlay is the cheapest spec there is.

DECISION: nothing operator-only taken. The natural next rounds are jtbl conversion rounds off the new tool's OK list: 108 stubs, `battle/misc30`, `misc18`, `misc12`, `world_map/misc6`, `misc11`, `menu/misc10` holding 4 each.

Round 130 detail:
- **The jtbl queue is real: 108 screened stubs, +3 matches from 5 workers, and the workers' budgets went on the first function each.** `battle/misc30` `func_800B9284` went 79.10 -> 96.61% at retail's exact 644 bytes / 169 instructions with identical order in all eight arms (residual: `$v1`/`$a0` swapped for the slot-table base vs index root), `world_map/misc11` `func_8008BB40` 87.64% at retail's exact 476 bytes, `world_map/misc6` `func_8007F968` 96.00%, `battle/misc18` `func_80097964` 95.67%, `menu/misc10` `func_801E2BE4` 75.76%. The three largest functions in each list were never started - the deep shape plus a 60-minute kill means ONE conversion per worker, so a jtbl round should be briefed 1-2 functions per worker, not four.
- **The scratchpad lever, measured hard:** ONE `u8 *pScratch = (u8 *)PSX_SCRATCH;` local for the whole function, with every derived pointer declared inside the block that uses it, is worth 21 points (70.01 -> 91.83%) on `world_map/misc6` `func_8007F968`. Five separate `(u8 *)PSX_SCRATCH + N` pointer locals each cost an `lui`/`ori` pair plus an s-register.
- **A per-arm base pointer is the emission-order lever for a scaled global table**: `pBase = D_800C3EB0; pSlot = pBase + D_800C360C * 0x48;` puts the base `lui`/`addiu` around the index load exactly as retail does; the one-statement form does not.
- `world_map/misc6` offset 0x04 is genuinely read BOTH ways by retail (`lhu` here, `lh` in the already-matched `WorldMapUpdateGroup16Actor`), and the field must stay `s16` (measured: `u16` drops the matched function to 99.15), so the new function reads it through a `*(u16 *)&pActor->unk04` view. That is the same "one header field, two widths" class the repo has hit before.

DECISION: nothing operator-only taken. Next round: extend `jtbl_align.py` with the PADWORD check (its one open gap), and brief jtbl conversions **one or two functions per worker** on the units the screener clears.

Round 131 detail:
- **A case-LISTING order is a first-class lever for switch conversions, and it is measurable in one shot**: `menu/misc9` `func_801DBE54` went 67% with ascending cases to **94.5% first try** with source order `4, 5, 1, 3, 0, 2, 9, 10` (= retail's body layout). Two more functions in that unit share the 12-word table shape and almost certainly the same lever.
- **`MenuCreateWindow`'s 4th argument is `u_short`, not `u_char`** - `misc9.c` declared `u_char a3` and silently truncated `0x124` to `0x24` in the first window call. A wrong-width prototype is invisible until the window is in the wrong place.
- **Two structural walls got precise mechanisms**: `battle/misc15` `func_8008B478` 98.42% needs retail's `(j-1)*16` kept in `$v0` across a `j--` (gcc 2.6 rematerialises it in that one path, +2 instructions, so it cannot link), and `world_map/misc7` `func_80081FD8` 99.85% differs in ONE init-order instruction in case 5 (`addiu v0,zero,0x2` before `addiu s0,zero,0xBF`) that no local assignment moves.
- A merge conflict this round was caused by nothing but a dirty `reports/progress.json` (`make check` rewrites it); `git checkout -- reports/progress.json` then re-merging worked. Worth knowing: `clone.sh merge` reports CONFLICT without listing files when the only obstacle is that file.

DECISION: nothing operator-only taken. The jtbl queue is now ~90 screened stubs and the round-131 shape (2 functions per worker, gate after every conversion) is the one to keep running.

Round 132 detail:
- **The "inline `.rdata` means order does not matter" rule from round 131 has a condition, and `world_map/misc5` measured it**: `func_8007BBEC` cannot be converted before `func_8007ADD4` because its table must follow ADD4's two tables in `.rodata` - a C table goes BEFORE a still-asm-owned table in the same object, so a later switch converted alone shifts the overlay. The rule is therefore: **order does not matter only when every EARLIER jtbl stub in the unit has already been converted**; otherwise convert in address order.
- **`convertible.py`'s BLOCKED is a false positive for overlays whose tables were carved by `own_rodata.py`** (`world_map/main/misc6`: both assigned stubs report "insn after .size"), because the trailing insns are the stub's own jtbl data. `jtbl_align.py` is the right filter for those units; `convertible.py` still governs units with `dlabel` data.
- **Three levers that a switch conversion needs, measured:** `case 0: break;` must be present in the second switch or gcc subtracts the minimum case and emits `sltiu 25` instead of `sltiu 26`; `extern s32 X[];` + `X[0]` read-modify-write is what makes gcc materialise the address like retail; a field read both ways in one unit must keep the signed type for the matched user and be read through a `*(u16 *)&` view in the new function (`WorldMapActor.unk5C[]`/`unk7C` retyped `s32`, all three matched users re-verified).
- **A `do { } while (0)` around a single store is load-bearing when retail reloads that value later** (`battling/misc1` `func_8007099C` 91.24 -> 92.82 and restores retail's byte count), and retail's shared case tails are ONE copy reached by `goto`, not duplicated source - duplication costs 52 bytes because gcc does not cross-jump them.
- **An uninitialized register read in retail is not a wall.** `menu/misc10` `func_801E23CC` reads `$s2` before it is ever written (an initializer gcc DCE'd); the next worker is told not to chase it.

DECISION: nothing operator-only taken. The jtbl queue is ~90 stubs; with 5 workers producing ~1-2 matches per round the honest rate is ~3 rounds per 4-5 functions, which is the best return available now that parks, presets and rewrite are exhausted.

Round 133 detail:
- **`world_map/misc4` `func_80077E68` is at 89.87% and the ENTIRE residual is one extra `nop` after the `jr $v0` of the jump-table dispatch** - retail's case bodies start one instruction earlier, and that single nop desynchronises objdiff's aligner so every switch body below it reads as unmatched even though it is correct. Worth remembering when a jtbl conversion "looks" wrong.
- **A jtbl conversion can be blocked by a strength-reduction mismatch, not by the table**: `battle/misc13` `func_80085618` needs retail's ONE raw index biv (`s0 = i*0x170`) plus exactly two address bivs, where gcc strength-reduces all seven `SYM + i*0x170` accesses into their own pointer bivs (1060 vs retail 1196 bytes, best 60.52%). Slot-index arrays at stride 0x170 are **separate symbols** in retail's object - typing them as fields of one struct reaches `MATCH` but never objdiff 100.00%.
- **`field/main/misc4`'s clip-walk family takes 10-11 arguments with a by-value `SVECTOR[2]`** (16 bytes = exactly the four stack words at 0x90/0xA4/0xA8), args 5-8 never read, triangle record 0xE (`s16 v0,v1,v2`, `s16 n0,n1,n2`, `u_char material`, `s8 stepHeight`), and the clip mask's bit0 needs an UNSIGNED shift (`srl`) of `NormalClip(...)`.
- **`lib_gear` `func_801E5D44` reads `lhu 0x50($sp)` for a stack slot that is never written** anywhere in its 387 instructions - recorded as an anomaly (the original body had an initializer gcc DCE'd), not a wall.
- Orchestrator process note: the jtbl queue is now generated with `select_target.py --json` instruction counts joined against `jtbl_align.py --report`, and only units whose SMALLEST ok-jtbl stub is small get briefed.

DECISION: nothing operator-only taken.

Round 134 detail:
- **Empty `case` labels are load-bearing in both directions.** `battling/misc7` `func_80080570` needs 23 EMPTY labels (5, 6, 8, 9, 10, 12..29) purely so cc1 sees min 4 / max 31 and emits the `jtbl_` - every shorter spelling (4 real cases) gave a compare chain at 59.91%. The mirror case is `battle/misc20` `func_8009E788`, where gcc **cross-jumps retail's two identical arms (cases 0 and 15) into one block** and the honest spelling is 200 bytes instead of 224; the only thing that stops the merge is a `volatile` QI load, which costs one instruction (98.66% parked).
- **`s32` vs `u32` decides `slti` vs `sltiu`** (`func_80080570` needed `u32` locals for the `sltiu` form), and `battling/misc5` `func_8007D0B4` needed a `u8 offset = 0x18` LOCAL because a literal `- 0x18` makes gcc put `-0x18` in a saved register and CSE it across three statements (+12 bytes, no match).
- **A jtbl-dispatch `nop` desynchronises objdiff**, and a case-folded arm makes the body N bytes short and unlinkable - both were parked with exact byte counts rather than chased.
- `battle/misc10`-class facts recorded: `BattleReloadSlotEntries`, `g_BattleDifficultyLevel` (`D_80099D98` read as 0..7), `g_BattleSpriteEntries` (0xFB entries of 0xA bytes), `D_80092700/704` cursor indices.

DECISION: nothing operator-only taken. Queue rule for the next rounds: `fresh_stubs.py` INTERSECTED with `jtbl_align.py --report` (both already account for park notes and the alignment/pad-word filters), sorted by instruction count.

Round 135 detail:
- **The slot-table twin-pair conversion is the cleanest shape in the queue**: same 0x170-byte battle-slot entry at `D_800CCCE8`, a switch over register number 0..23 picking one field, then `if (get == 0) *p = value; else result = *p; return result;` - and the shared epilogue copies ONE variable, so the source really does have a single return of a local assigned only in the read arm (two `return`s give a different epilogue).
- **`world_map/misc12`'s `D_8009C620` is the same 0x54-stride table `misc4` already declares as `WorldMapEntry`** - the two units should share one type in a header.
- **`battle/misc5` `func_8007AA60` went 53.44 -> 99.09% in one edit** by taking the row pointer from the declared register file (`D_800D3400[arg1].unk30`) instead of a stale extern that no longer compiled; its residual is the multiply's operand order, which is coupled to the two load orders (permuter 1370 iterations, base=10/best=10).
- **A stale park is a trap**: that function's old body did not compile against today's externs, so its recorded 53.44% was fiction. Same class as round 111.
- `battling/misc1` `func_80072170` needs a SIGNED loop bound (`slt`, not `sltu`), `D_8009260C` read signed, and an `(s16)` colour argument - three separate `lh` vs `lhu` fixes in one function.

DECISION: nothing operator-only taken. Next rounds stay on the same queue rule (fresh ∩ jtbl-OK, body size), with the added requirement that a unit's jtbl stubs are converted EARLIEST-FIRST and, where the tool reports several OK stubs, given to ONE worker as a family.

Round 136 detail:
- **An empty `case` label can be the whole difference between a table and no table:** `WorldMapUpdateActorSlot` needs `case 64:` listed empty because retail does `sltiu 0x41` (65 entries); without it gcc trims the table to `case 50` and the overlay shrinks.
- **A shift in a branch DELAY SLOT may not be dead.** `sll v0,s2,7` there clobbers `v0`, so the else branch's `addiu v0,v0,0x180` actually computes `index*0x80 + 0x180` - the source is `pTarget = &D_8009BE24[index + 3];`, not a byte-offset hack.
- **Declaration order is the prologue lever**: `s32 result = 1;` and the `pScratch` local must be declared BEFORE `pActor` to get retail's `sw s4 / li s4,1 / sw s3` ahead of the load, and a scratchpad pointer local is what puts 0x1F800000 in `$s3` at all.
- **`field/main/misc4`: pointer locals vs direct indexing is a 40-point lever** — `pCurTri`/`pVert` locals score 51.58% (gcc re-bases the induction register and spills both), while `pVerts[pTri[nTri].v0].vx` scores 92.29%.
- **A parked comment containing `/* ... */` breaks matching mode with a silent `Error 33`** - the documented trap, hit again this round; fixed by rewording the note.
- `battling/misc3` `func_80077A9C` reached **97.90% at retail's exact 1720 bytes** with both jump tables in case order; `BattleDequeueMessage` must return `s32` (a `u8` return adds an `andi` at every call site), and the residual is cc1 keeping the FIRST copy of a cross-jumped store block where retail keeps the LAST.

DECISION: nothing operator-only taken. Two process fixes for the next rounds: pass `options.timeoutMs`, and treat a timed-out child's uncommitted file as lost (revert) rather than trying to gate it by hand.

Round 137 detail:
- **`diff_func.sh` truncated the ORIGINAL column at the first splat `.L` label** (`dump()`'s awk exits on `^[0-9a-f]+ <.*>:$`, and `expected/*.s.o` keeps `.L` labels as symbols). A worker in `battling/misc1` reported its left column stopping ~39 instructions in - i.e. part of every diff in such a unit was invisible. Fixed: the exit pattern is now `<[^.>][^>]*>` so it only stops at a real symbol, not a local label. Any park note written from a truncated left column should be re-read.
- **The permuter's score does not track linkability**: `battling/misc1` `func_80072170` went 97.21 -> 98.87% through two permuter finds (a `pCounter` copy of `pFrames`, and assigning the add-slot element address to a local BEFORE the `if`, which produced retail's offset-first `addu a0,s0,<base reg>`), but every improvement it found grew the object past retail's 1080 bytes - the LINKABLE fallback is the previous 97.21% park, now recorded as such.
- **A "RODATA-BLOCKED" park note can be stale**: `battle/misc10`'s note claimed the table would shift the overlay, but `config/battle.yaml` gives that unit its own `.rodata` range, so the switch is fine; the function is at 99.30% (684 vs 688 B) and the residual is one copy in the loop tail (`acc = next; ret = acc;` folds away a copy retail has; 12 spellings and 1225 permuter iterations land on one of two attractors).
- **A widened callee prototype can be byte-inert**: `battle/misc4` added an unused third `u8` parameter to ten handlers because retail sets `$a2` before every `jal` - `check_bin` proves all ten still link identically. Same class as `WorldMapSetSpritePrimColor`'s `u8` -> `s32` params in `world_map/misc8` (a `u8` param is impossible against a call site that does `lw` with no `andi`).

DECISION: nothing operator-only taken.
| 138 | 5 rewrite workers + 20 naming workers | +0 | 328 | No matches by design (rewrite + naming), and the rewrite round finished 5/5 workers with every function still matching. Live raw **378 -> 328**, stubs 1071 unchanged. **The lever nobody had measured: in `world_map/misc5` the `u8 *` cast spelling is load-bearing.** Typing `pPlayer`/`pWork` lets gcc 2.7.2 prove the constant-offset stores independent, so it hoists and batches the loads (624 -> 616 bytes, 94.49%); the file now types everything around them (`pLayer[5].unk0`, `pLayer[4].mat`, `pScratch[2]`, `D_8009BE24[idx].unk4`) and keeps the six `pos` stores raw with a comment giving the offsets and the reason. Same shape twice more: `battle/misc16` typed `BattleSlotGraphics`/`BattleSpritePackage`/`BattleTableEntry` (BattleDrawSpriteQuad CLEAN) but `func_8008CDE4` -> **BattleApplySlotStatusDamage** must keep byte arithmetic on `D_800CEF10`, since the typed row form is 86.39%; and `system/system`'s new `SystemFontFileHeader` is read as `*(u16*)((u8*)pFontFile + offsetof(...))` because every field-lvalue form is 66.54% (gcc hoists all six `lhu` above the two pointer stores, retail interleaves 1:1). `system/temp1` 3/3 100% (`SpriteData`'s 0x3C word now named bitfields) and `menu/misc7` 5/5 CLEAN (480/192 B re-verified). **Naming: pass 1 gave seven workers six files each and lost SIX of them to the 30-minute cap with nothing in the output file** (the one that finished reported 46 lines); pass 2 at three files per worker finished 11 of 13. Union of every `NAME`/`DATA` line ever reported (2654) applied: **+25 function names** (unnamed C definitions 84 -> 59; the other proposals are `INCLUDE_ASM` stubs, which the tools refuse by design), **+395 `D_` globals -> `g_Name`** (distinct 2588 -> 2193, `name_addrs`/`rename_data` added 385 `symbol_addrs` entries), unk field declarations 2828 -> 2838 (the rewrite round's splits), eleven green. Tooling: `name-brief.js` now sanitises its run key, so one worker can cover several files; PROMPT.md records the 2-3-file limit. DECISION: nothing operator-only taken.
| 139 | 5 permuter-port workers | +3 | 343 | First apply round off the 2026-11-16 sweep, 3 of 5 BETTER leads closed: `world_map/misc1` `func_80073530` -> **WorldMapDecompressMapTables** (97.42 -> ZERO, and the last diff was reorg: re-forming the scan end as `pEnd + 10` *inside* the loop compare stops gcc folding it to `pEnd - 2` and filling retail's `nop; addiu v1,a0,20` delay slot - three permuter runs, base 135 -> 0), `system/system` `func_80033CF0` -> **SystemFormatNumberGlyphs** (parked 96.49 -> ZERO in 10 iterations once the ported `pEnd` reuse was in) and `battle/misc24` `func_800AA934` -> **BattleAppendActorCommandBytes** (96.25 -> 100; the last step is the plain pointer-plus spelling `*(u8 *)(arg0 + *pCount + 0x2A)`, which fixes an `addu` operand order). Stubs 1071 -> 1068, eleven green. `world_map/misc6` `func_8007F968` 96.00 -> **99.81%** (permuter 235 -> 15) and re-parked: the whole residual is 8 bytes of stack frame retail allocates and never stores into, which no C edit produces. **A BETTER score is not always an improvement**: `battle/misc7` `func_8007B6C0`'s candidate (230 -> 145) duplicates the case body behind `if (p)` to win retail's `lw a0,0(s2)` and loses the case-2 slot-sum registers doing it - 95.25% against the parked body's 96.17%, so it was measured and rejected. Naming-tool lesson recorded in PROMPT.md: `name_addrs.py` renamed `func_800AA934` in the C but left `func_800AA934 = 0x800AA934;` in `symbol_addrs.battle.txt`, so splat kept naming three stubs' calls with the old spelling and the link failed; the grep after applying a name must cover `config/` too. Sweep paused for the round (358 parked bodies still untried in their current form), restarted afterwards. DECISION: nothing operator-only taken.
| 140 | 5 deep fresh-stub workers (60 min) | +1 | 341 | `slus_006.64/system/archive` `func_8002AC24` -> **ArchiveCdReadyReadQueuedFile** (100.00%, 280 insns, 1120 B): the CdReady callback for the queued multi-entry stream path. Stubs 1068 -> 1067, eleven green. **The pool is now allocator/park-shaped, not C-shaped**: four of five workers spent their hour and produced 83-98% parks with every instruction present - `battle/misc16` `func_8008CFB8` **98.03%** at retail's exact 880 bytes (only the POSITION of two prologue inits differs, and moving them trades the registers that made them land there), `battle/misc22` `func_800A48EC` 78.13 -> **97.30%** (one store's place), `field/misc8` `func_80082620` 95.34% (one extra callee-saved word), `menu/misc3` `func_801CA1D4` 83.27% newly parked. Two screener lessons: **read `convertible.py`'s whole list, not its head** - `field/misc8` `func_8008110C` was BLOCKED (owns five `dlabel` strings) and I had picked it from a truncated listing; and `battle/misc22` `func_800A4654` is a **stack switch** (`sw $sp`), which no C emits. One reusable idiom went into `brief.js`: gcc 2.6.0 addressing of a 6th-and-later argument, `frame_size + 16 + (N-6)*4`, which turns "my prototype is wrong" into "the function has fewer parameters" (`func_800A48EC`'s `pOt` at 0x54 = 8 params exactly). `DATA-LEAD` for the next rewrite round: `menu/misc3`'s `D_801E9810/18/20/28` are ONE s32 table read at index +0/+2/+4/+6, not four arrays. DECISION: nothing operator-only taken.
| 141 | 5 rewrite workers + 8 read-only naming workers | +0 | 328 | No matches (rewrite + naming by design): live raw **341 -> 328**, unnamed C definitions 59 -> 49, distinct `D_` **2193 -> 2094**, stubs 1067 unchanged, eleven green. The rewrite round's product is the typed form of things the parks have been guessing at: `battle/misc24` re-typed `BattleAppendActorCommandBytes` and three neighbours onto the existing `BattleActor`, re-verifying 100.00% each (the actor struct in `include/battle/actor.h` already covered all 14 offsets the body touches); `world_map/misc1` gave `WorldMapDecompressMapTables` two local header typedefs (`WorldMapMapDataHeader` 0x6C, `WorldMapPointTableHeader` 8) and kept the three block-relative relocations, each commented; `battling/misc15` added `VertexIndexSet` (8-byte vertex run) and a zero-length `PacketNode.verts[]` tail after deriving the stride from `func_8008C4B0`'s `lw 0(a0)/lhu 4(a0)/a0+=8`; `battle/misc30` took its park from 86.54 to **94.47%** while typing the 0xA8/0xAC mask. **Two load-bearing spellings found (do not tidy them):** `field/main/misc`'s `FieldActorCopyPlacement` copy helpers must keep `*(s32*)((u8*)dst + offset)` - the typed field form is 83.35% for three words and 51.63% for all seven, because `g_FieldActors` is `FieldActor* volatile` and retail's single re-read per copy only happens when the array is passed to the helper; and `battle/misc24`'s `(BattleSprite *)(index * sizeof(BattleSprite) + (s32)pSprites)` is the only spelling that gets retail's `addu` order (`&pSprites[index]` flips it to 99.85%). Everything the 8 naming workers proposed is applied: **+10 function names** and **+99 `D_` globals** (`rename_data` added 102 `symbol_addrs` entries). One link lesson recorded in PROMPT.md: renaming a `lib_gear` function that an overlay calls needs the entry in the *caller's* symbol file too (`undefined reference to LibGearFreeModel` in `field`). DECISION: nothing operator-only taken.
| 142 | 4 field workers (one per declaring file) | +0 | 328 | First field round of the session, 4/4 workers finished with the build proved per binary: `unk<offset>` declarations **2853 -> 2739** (-114) and no matches attempted by design. `include/system/menu.h` (5 bins) named the render gates the menu overlays share (`shouldRenderPointerCursor`, `shouldRenderStrings`, `shouldRenderModalStrings`, `shouldRenderSubMenuChoiceStrings`, `shouldRenderCharacters`) plus `MenuString.hasVertices`, `MenuUnk1.drawModeRenderContext` and `shadeWidth` - each with the AddPrim it gates as evidence. `include/battle/actor.h` (3 bins) named `BattleActor.unk2B commandCount`, `unk5C parentActorSlot`, `BattleTableEntry.unk4C hp` / `unk4E maxHp` (the `* 50` and `0x270F` clamp pair), `BattleSlotState.unk60/unk64`, `BattleBlock.unk0 inUse`, `BattleWorkBlock.unk5FC7 commandCode`, and SPLIT `BattleSlotRegs.unk0` into `pScriptCursor/pScanCursor` - the split is what the parks in `battle/misc4` were casting. `battling/misc4.c` named 27 fields of its seven .c-local structs (the actor's `x/y/z/vx/vy/vz`, `floorY`, `modelIndex`, `pPart`), and `world_map/misc10.c` named the position triples (`WorldMapVec.x/y/z`, `WorldMapPlayer.unk28..30`, `WorldMapActor.angle/pSprite`) and the trail entries. Eleven green, nothing else touched. DECISION: nothing operator-only taken.
| 143 | 4 field workers | +0 | 328 | Second field round, 4/4 finished and every dependent binary proved: `unk<offset>` declarations **2739 -> 2646** (-93), eleven green, no C touched. `include/main/game.h` (8 bins) named the cross-overlay `GameState` spans that field scripts and overlays agree on: `unk1930 fieldScriptMemory` (the 0x200 words `FieldScriptSyncMemoryWithGameState` copies), `unk1D30 characterFlags` (bit per character id), `unk1834 worldMapState` (bit 0x4000 picks the view width, `& 0x1FFF` picks the actor setup), `unk231A mapNum`, `unk2322 wdsBankIndex`, and reported `FIELD_UNKNOWN` for `unk16BC` with the evidence that the `GearState` typedef name is wrong. `battling/misc3.c` and `battling/misc2.c` named their twin `BattleActor` fields to match `battling/misc4.c` from round 142 (x/y/z, modelIndex/partIndex, state, flags, gaugePoints/gaugeLevel, pModelLink, pPartTimings), and `battle/misc5.c` **dropped its local `BattleSlotRegs` duplicate in favour of the identical declaration in `include/battle/actor.h`** and named the 18 `BattleSlotVarEntry` byte registers from the accessor's own switch cases (`byteVar0..` by case index). DECISION: nothing operator-only taken.
| 144 | 4 field workers | +0 | 328 | Third field round: `unk<offset>` declarations **2646 -> 2552** (-94), eleven green. `include/libgear/main.h` named the gear-model structs (`Vertex.age/lifetime`, `Temp3.inUse/type/endFrame`) and reported `FIELD_UNKNOWN` where the only writers disagree; `battle/misc33.c` named the whole `BattleActorSprite` animation set (`x/y`, `targetX/targetY`, `pCallback`, `rotZ`/`scaleX/Y/Z`, `countdown`/`rotZStep`, `colorR/G/B`, `pTargetSprite`, `pArchiveEntryIndex`, `flags`) which is the sprite `BattleRenderSpriteBody` (round 139's name) drives; `battling/misc5.c` **recognised `SlotMotion` as a POLY_FT4 work packet** from the retail stores (code byte at 0x7, clut at 0xE from LoadClut2, tpage at 0x16 from GetTPage, u/v at 0xC/0x14/0x1C/0x24) and split its three `unk` arrays into x0/y0/u0/v0-style fields; `world_map/misc10.c` second pass named the actor state machine (`command`, `targetSlot`, `state`, `stateTimer`, `dirX/Y/Z`, `moveSpeed`, `trailDelay`) from misc11/misc15's switches. DECISION: nothing operator-only taken.
| 145 | 4 field workers | +0 | 328 | Fourth field round: `unk<offset>` declarations **2552 -> 2468** (-84), eleven green. `battling/misc12.c` named the list primitives (`BattleListNode.pList/pNext/pHead`) and the `BattleSprite` draw-area pair (`drawAreaMode0/1`, `drawOffsetMode0/1`) straight off the `func_800453E8`/`func_8004546C` calls; `battling/misc16.c` named 42 of its 58 from the effect-spread code (`Misc16Work.gravity/count/entrySize/pEntries`, `centerX/Y/Z`, `rangeX/Y/Z`), reusing the `x/y/z`, `rotX/Y/Z` spellings the same struct already carries in `misc2.c`; `include/system/sound.h` named the SPU voice ADSR set (`attackMode/sustainMode/releaseMode` and the four rates, each tied to the exact ADSR0/ADSR1 bit range its writer produces) and the envelope stepper (`pStepHandler`, `stepDelta`, `delayCount/delayReload`, `rampLevel/rampStep`, `target`, `handlerIndex`) across its five dependent bins; `battling/misc4.c` second pass named `ActorPart.angle`, `Actor.angle`, `Actor.effectColor` and SPLIT `Actor.pad60` into a `prevPosition` VECTOR. DECISION: nothing operator-only taken.
| 146 | 6 deep fresh-stub workers (60 min, fresh-intersect-convertible, smallest first) | +4 | 328 | Best matching round of the session, and it validates the round-134 ranking method: **screen `fresh_stubs.py` against `convertible.py`'s OK list, then take the smallest bodies first**. Matched `battle/misc12` **BattleSelectTargetCandidates** (168 insns, 672 B), `battling/misc1` **BattleRecenterViewPositions** (193 insns, 772 B), `battle/misc15` **BattleActivateSelectedCommand** (206 insns, 824 B) and `lib_gear/main` **LibGearComputeScaledJointMatrices** (251 insns, 1004 B); stubs 1067 -> 1063, eleven green. Three levers reported that other workers should have: **arg0 IS the flags word** in `func_80084DE4` - the parameter is reassigned in the per-mode switch so the source has no `flags = arg0;` copy, and adding one swaps the two callee-saved parameter homings in emission order (that was the whole 99.81% park); writing `m.vy = 0; m.vz = scale;` *inside each switch case* rather than after the switch keeps `m.vy` unknown on the `default` path so gcc keeps the following `a.vy += m.vy` adds and cross-jumps the case tails into retail's shared merge (90.13 -> 100.00); and a **fresh scratchpad pointer local at the top of a nested block** splits a live range so gcc re-derives `lui $s1,0x1f80` like retail instead of pinning it in a callee-saved register for the whole function (that closed `func_801DC848` on the first try, where one shared pointer was 97.18%). Parks with full diagnoses: `battle/misc12` `func_800841E0` 90.21% (stack-array layout + count/pointer pairing; a good permuter seed), `battle/misc15` `func_8008B224` 89.19% (2.6.0 re-forms the symbol address at all three sites where retail keeps `arg0 * 0x170`), `world_map/misc12` `func_80091C18` 87.81% (switch case LIST ORDER 14,17,9,10 instead of ascending was worth 13 points), `field/misc2` `func_8007254C` 91.81% at retail's exact 408 B - whose note is now corrected: the `u_char** pCamRef = &pCam;` address-taken launder makes the whole camera block address as `disp(base)` instead of the `$at` macro, so that part of the unit's "wall" IS beatable and the residual is one callee-saved allocation. `battling/misc1` `func_80072858` is `/* Handwritten function */` in splat and fuses the GTE rot+trans load in a shape no `inline_c.h` macro emits. DECISION: nothing operator-only taken.
| 147 | 6 deep fresh-stub workers | +2 | 328 | Two matches with the same recipe as 146: `battle/misc18` **BattleDecrementSlotConditionTimers** (280 insns, 1120 B, first attempt, then re-verified with the realistic spelling) and `menu/misc9` **MenuRenderEquipmentSlotStrings** (310 insns, 1240 B); stubs 1063 -> 1061, eleven green. Four parks, each with a decoded residue: `menu/misc3` `func_801CBA4C` **94.12%** (retail reloads `pManager->currentCharacterIDs[i]` for the header store where gcc reuses the test's `lbu`, plus an `s0`/`s1` registration tie; the park note now carries the whole 0x24-byte save-slot header layout and the `unk54[-4]`/`unk54[-2]` trick that gets retail's `%lo(g_GameState+0x2BC)` folding), `battle/misc21` `func_8009F1C4` **91.58%** (ONE allocator tie: the four `1`-constants share a pseudo hoisted into `$s8`, costing the 10th saved register and spilling `$a0`; writing `if (x)` instead of `== 1` gives *exactly* retail's frame at 91.86% but `!= 0` emits `beqz` where retail has `li/bne`, so the `== 1` spelling must stay), `menu/misc10` `func_801E31C0` **89.92%** (a literal `hp + amount * 50` makes gcc 2.6.0 emit `sll/addu` where retail has `mult` with the 50 in the `bne` delay slot, so the source multiplied by a VARIABLE; declared-unused `s32` locals do not grow a 2.6.0 frame, which retires the round-44 note), `world_map/misc1` `func_800740B8` **82.80%** (three cross-jumped `/341` arms that retail duplicates, plus the `0x1F8000F0`-in-`$s5` frame delta). **My selection bug, recorded:** the preset lookup took the FIRST preset listing a path, and `battle/main/misc21` is listed by `XenoBattleDiv` before `XenoBattle260Div` - the worker caught it and reported `PRESET 2.6.0`, but the brief was wrong. Take the LAST matching entry in `gears.toml`, which is how the preset file overrides. DECISION: nothing operator-only taken.
| 148 | 6 deep workers (two of them on the tree's smallest stubs) | +1 | 328 | `battle/misc27_b` **BattleGetActorJointOffset** (264 B) harvested from a parked body, stubs 1061 -> 1060, eleven green - and the round's value is two screener corrections plus one runbook entry. **`fresh_stubs.py` and `convertible.py` both pass stack-switch stubs**, because a stack switch has no post-`.size` instructions and no `dlabel` data: two of the six workers (`slus_006.64/system/rendering_pause_text` `func_8001FB30`, `battling/misc15` `func_8008BC04` - a coroutine register save) were spent proving that, and both independently cited precedents (`GfxLoadImageAccelerated`, `func_800BEE2C`). The `grep -l 'sw $sp, 0x0'` filter in PROMPT.md is now marked mandatory with this evidence. **The caller-shrinks hazard has a lever:** with a callee newly defined in the same TU, gcc hoists the second `lhu point.y` above the first store and copies `pOut` into `$v0` mid-body (8 bytes); K&R and unprototyped declarations do not help, but wrapping the call plus the two offset adds in `do { } while (0);` does, and the permuter found it in 11 s. Parks: `battle/misc21` `func_800A1CF4` 86.05% (the rot/pos tween-queue builder, fully decoded - 6-byte per-sprite frame records, wrapped deltas with `& 0xFFF` + `0x800`, `0xFF` marker in `unk3`), `menu/misc9` `func_801DF0D4` 78.74% (every block right, `vars=40` vs 24 and an `addu a0,a1,zero` coalescing), `battling/misc5` `func_8007BBA0` **94.47%** (only the six-block corner prologue), `battling/misc15` `func_8008BD70` 90.49% (the loop guard is the whole function: retail's preheader has no `D_80059528` store and only a bottom test, gcc rotates the loop). DECISION: nothing operator-only taken.
| 149 | 5 permuter-port workers + 6 read-only naming workers | +3 | 3514 | Three of five BETTER leads closed: `battle/misc32` `func_800BD2E4` -> **BattleUpdateCommandRows**, `menu/misc4` `func_801CE198` -> **MenuRenderTransformedQuads**, `battle/misc5` `func_8007A92C` -> **BattleWordRegSetImm**; stubs 1060 -> 1057 (Total here = 4571 functions - stubs; earlier rows in this table count something smaller). **The permuter's own top candidate is not the match**: in `battle/misc5` the ported edit measured 71.67% against the park's 84.67% (a second permuter run from the park reconverged on the same loser) - what closed it was a two-part hand fix now in `brief.js`: the base as a `mem_addr` local ASSIGNED IN A STATEMENT (`base = (mem_addr)D_800D3420;`, an initializer folds it above the index loads) and the address written INDEX-FIRST (`((u16 *)((p[1] << 1) + (arg1 * 0x40 + base)))`) for retail's `addu` destination/order. `battle/misc32`'s winning edit was a duplicated `if (flag) call; else call;` that pushes the loop body past cc1's hoist threshold, so `D_800C360C` stops being hoisted (jump opt merges the copies back); `menu/misc4`'s was to drop an `index` local (givs order) plus retail's real vertex stride of **four** SVECTORs, which the five-year-old park note had wrong on both counts. Two parks improved, not closed, both with the wall written down: `field/misc8` `func_800831F4` 92.51 -> **96.86** (a `do { angleCopy = angle; ... } while (0);` wrapper is what holds the second `sh` out of the `jal` delay slot; residual is a `rotation.vy` copy retail has and gcc folds) and `field/misc` `func_8008861C` 92.50 -> **99.50** (gcc folds the `directions` field offset into the base register; eight spellings measured 56-99.5%). Naming: union of every `NAME`/`DATA` line ever reported (2914) applied - **+113 function names** (unnamed C definitions 277 -> 252; the rest are asm stubs that now carry real names in `symbol_addrs`), **+75 distinct `D_` globals** (2097 -> 2022), `unk` fields 2473 flat, raw **1363 -> 1361**. Tooling: `fresh_stubs.py` now drops stubs whose `.s` stores `$sp` (a stack switch is not C; 118 -> 113), which retires round 148's waste. Housekeeping: deleted 130 stale clones from rounds m104-m120 after verifying every one was merged into main (10 GB), and the sweep was paused for the round (11 BETTER / 335 parks untried in their current form) and restarted after. DECISION: nothing operator-only taken.
| 150 | 8 deep fresh-stub workers (60 min, fresh ∩ convertible) | +2 | 3516 | Two matches in `battling/misc14`: `func_8008A7E0` -> **BattleUpdateNodeTree** (243 insns, written from scratch; the case bodies had to be listed **2, 0/1, 5** because gcc lays out switch bodies in source order, and the state-5 payload must be re-read through `pNode->pData`), and `func_8008B0D8` -> **BattleResetPartAnim** (a 99.80% park the permuter closed in one iteration: the fix was statement position, `pEntry->unk4 = pEntry->unk0;` BEFORE `pEntry->unk10 = 0;` - the old note blamed `combine_givs`). Stubs 1057 -> 1055, eleven green. **The fresh pool is now nearly empty and conditional on my own filter being right**: `fresh_stubs.py` sees 36 stubs in 31 units after round 149's stack-switch and park-window fixes, and of the 8 briefed stubs six were walls that no worker could convert - `slus_006.64/system/temp2` `func_80026F44/80026FE8` are Psy-Q handwritten asm (opcode-8 `addi`/`add` that gcc never emits), `battling/misc14` `func_8008BB3C` restores `$sp/$gp/$k0/$k1/$fp` and returns through the reloaded `$ra`, and the rest are allocator ties at retail's exact size: `battle/misc23` `func_800A5EB4` 99.16% (30 address `addu`s in the other operand order, the SAME tie as the neighbouring `func_800A64E4` park - one lever settles both), `battling/misc4` `func_80078920` 99.13% (instruction-identical at 0x90 frame; needs a non-void return type or gcc steals the second `slt` into the `beqz` slot and the body is 4 bytes short), `battling/misc9` `func_80082488` -> **BattleGetGridPlaneHeight** (98.96%, prologue register choice + one `addu` order), `battling/misc3` `func_80078194` 89.19%, `field/dialogue/text_box` `func_8009C154` 83.89% (four `addu` row addresses CSE into one register; the typed form hits the documented `FieldLoadUITextures` wall), `field/dialogue/text_box_render` `func_8008004C` 91.11% (the second parameter is the render context `box + 0xC*ctx`). One more real finding: `slus_006.64/system/temp2` `func_80025C04` is NOT unreachable as its old note claimed (only `mtc2` is non-gcc; the 3.x `gte_*` macros reproduce it) and it is parked at 87.30% at retail's exact size - but it can never read 100.00% because splat renders the scratchpad address as `%lo(D_1F800004)`, an `R_MIPS_LO16` reloc with a zero immediate, which no C emits. Naming: the extraction now also reads NAME/DATA lines inside worker prose (3084 lines, up from 2914) - **+21 function names, +18 `D_` globals** (2011 distinct), raw 1361 -> 1365 (the new parks), unk fields flat. DECISION: nothing operator-only taken.
| 151 | 2 permuter-port workers + 6 rewrite workers | +3 | 3519 | The sweep's first ZERO and one BETTER closed, and one rewrite worker's function came in with them: `world_map/misc7` `func_80081FD8` -> **WorldMapUpdateSlotColorFade** (the permuter's whole edit was `s32 value` -> `short value` in the case-5 fill), `battle/misc21` `func_8009EF3C` -> **BattleUpdateSpriteTransforms** (permuter 40 -> 25 was a DECOY for a two-pseudo role split: the prologue and the second `doTransform` pass must go through the walker copy, the scale matrix and `MulMatrix0` through the parameter - with the roles right the ported `new_var` copy became gratuitous and was dropped) and `battle/misc21` `func_8009F1C4` (99.73% park, un-parked by the same worker); stubs 1055 -> 1052, eleven green. **My rewrite briefs were built from a grep that counts PARKED bodies, and that is what four of the six workers spent their hour on**: the per-function attribution read a park's raw lines as the stub's, so `world_map/misc10`'s 47 "raw lines" are in `INCLUDE_ASM` stubs and the file has **0** live ones, and three workers reported back that their "matched functions" list was in fact a list of parks (`battle/misc2` `func_800765C4/80076710/80073B64`, `battle/misc19` `func_8009A258/8009A854/8009ADA0`). Fixed at the source: **`tools/scripts/raw_lines.py`** strips comments before counting and attributing, and PROMPT.md now tells the next session to use it - the real backlog is **288 -> 284 live raw lines in 68 files**, not 1365. What the six rewrite workers still landed: `menu/misc7` `MenuSetPlayTimeDigitPrims` retyped onto `g_Menu->unk344->polys[]` (raw 7 left, address sum must stay in integer space), `battle/misc2` `BattleUpdateRotationBuffer` retyped onto a `BattlePulseWork` (`POLY_G3 prims[8]` + a `BattleColorRamp` union, matching what `battle/misc3` already builds), `battling/misc2` `BattleEffectSetMatrix` given a real `MATRIX *` parameter, `battle/misc19` `BattleIsCharacterInParty` onto a declared 0x10-byte `BattlePartyRecord`, and four measured ROUGH verdicts worth keeping: in `world_map/misc5` the `u8 *` spelling is load-bearing for six `pos` stores (typed form 62.97% / 73.27%), the typed `pWork->prims[i]` in `BattleUpdateRotationBuffer` is 91.78%, the typed `&pEntry->state.unkA4` in `BattleGetActorActionConditionMask` is 94.58%, and folding `BattleSetSlotEntryFieldsTo1`'s row offset into the base is 99.71%. Naming: +7 names (2005 distinct `D_`, unk fields 2495). DECISION: nothing operator-only taken.
| 152 | 4 field workers (one per declaring file) | +0 | 3519 | Fifth field round: `unk<offset>` declarations **2495 -> 2447** (-48 net; the four workers reported 124 FIELD/SPLIT lines and every renamed binary rebuilt byte-identical), stubs unchanged, eleven green. `include/battle/actor.h` named the mirror set against `lib_gear`'s model struct at the SAME offsets - `pMesh`, `scale`, `height`/`width`/`depth`, `isAttached`, `parentJointIndex`, `attachOffsetX/Y/Z`, `posX/Y/Z`, `groundTriangleIndex` - which is the strongest evidence class available here (two independent compilations of one layout). `include/main/game.h` named the equipment-bonus split (`GameCharacter.unk0[0x2C]` -> `unk0[0x28]` + `attackFromEquipment`/`defenseFromEquipment`/`agilityFromEquipment`/`unk2B`, mirroring the already-named `etherDefenseFromEquipment`) plus `weaponId`/`extraWeaponIds`/`accessorySlots` from the shop menu's `pWeaponsData[characters[cid].weaponId[0]]` pair. `include/system/menu.h` named the prim counts and render contexts the menu overlays thread through `MenuRenderString` (`polys5F0PrimCount`, `polys27B0PrimCounts`, `polys3020RenderCtxs`, `shouldRender`, `palette`, `cardCheckTimer`). `battling/misc3.c` named 18 of its file-local `BattleActor` fields to the twin spellings `misc2/misc4/misc5` already use (`vx`/`vy`/`vz` at 0x10/0x14/0x18, `prevPartIndex`, `angle`, `prevPosition`, `floorY`, `gaugeValue`, `gaugePoints`, `pActionEntry`, `unk84`). One process note: I gave a worker a clone path with a typo (`r-m152-h-bt3` instead of `r-m152-bt3`), it died at launch with `cwd does not exist` and nothing was lost; the relaunch is what merged. DECISION: nothing operator-only taken.
| 153 | 2 permuter-port workers + 6 read-only naming workers | +0 | 3519 | No matches: both leads were allocator ties that the permuter could not close - `battle/misc18` `func_80098AF8` 99.35 -> **99.84%** (the 15-point find was hoisting `0x1058` into an `int rowOffset` before the `if`; the `0x2228` branch keeps retail's base fold and three hand variants all drop to 99.68, so the note now names the wall) and `field/main/main` `func_80077E10` 96.33 -> **96.67%** (the permuter's only edit is a DEAD `new_var = (a != 1) || (b != 0);` that moves a load from `$v1` to `$v0` - dead code that is load-bearing, recorded). Stubs unchanged, eleven green. **Naming: the function backlog is not what this file claimed** - `tools/scripts/unnamed_c.py` (new) counts a `func_` as a definition only when a depth-0 signature reaches `{` before any `;`, and the real total is **49 in 23 files, 29 of them under `psyq/`**, i.e. ~20 non-SDK functions are left; the 277/252 in earlier rows were prototypes and call sites. Round 153's six workers found that several assigned files define no `func_` at all and came back with `D_` proposals instead, which is where the value now is: **+62 `D_` globals** (`rename_data` added 62 `symbol_addrs` entries, 2005 -> **1943** distinct), including the whole `D_801E9Bxx` menu coordinate table (14 entries: `g_MenuCharacterHpDigitsX/Y`, `MaxHpDigitsX/Y`, `MpDigitsX/Y`, ...), `g_BattleWorkBlock`-relative interiors, `g_MemberChangeMenuLockedCharacterFlags`, `g_LibGearColorMatrix`, `g_BattlePauseActors`, `g_MenuMainRowStringIds`. Also landed `func_80081FD8` -> **WorldMapFillFadeTable** by hand after `rename_sym` refused: the worker's `WorldMapUpdateSlotColorFade` was already taken by the 0x80081868 sibling. unk fields flat (2447), live raw **284 -> 277**, stubs 1052. DECISION: nothing operator-only taken.
| 154 | 4 field workers + 4 read-only naming workers | +0 | 3521 | No matches by design; both halves paid. Fields: `unk<offset>` **2447 -> 2366** (-81), with the four tables now named off their live readers - `battling/misc4.c` (12: `Actor.pScript`, `ActorSlotRecord.inUse`, `prevPartIndex`, `pActionEntry`, the gauge block `gaugeValue`/`gaugePoints`/`prevGaugeValue`/`gaugeMax`), `battling/misc10.c` (23: `Misc10SpriteHeader.visiblePartCount` + the three `effectColor` bytes + `visiblePartIndicesOffset`, `Misc10ActionSlot.flags`, `Misc10Actor.pModelLink`/`pPartTargetTimings`), `battling/misc17.c` (the whole sound-slot machine: `BattleSlot.soundId`/`active`/`elementIndex`/`flags`/`timer`/`spuAddr`/`elementMask`, `BattleWork.delay`/`targetAngle`/`maxDistance`) and `world_map/misc8.c` (22: `WorldMapActor.command`/`state`/`x`/`y`/`z`/`angle`/`pSprite`/`rotY`/`scaleAngle`/`scale`/`scaleStep`). Data: **+173 `D_` globals** (`rename_data` added 165 entries; distinct 1943 -> **1770**), the biggest single haul of the session - the movie unit's file paths and player state (`g_StrMovieMainSeWd`, `g_MovieClearRect`, `g_MovieCurrentFrame`, `g_MovieStopCountdown`), the battle script globals (`g_BattleScriptCursor`, `g_BattleScriptWait`, `g_BattleActor0/1`, `g_BattleResultCode`), and the world-map zone/area set (`g_WorldMapZoneProximity`, `g_WorldMapZoneId`, `g_WorldMapZoneAreaRects`, `g_WorldMapAreaRect`, `g_WorldMapAreaId`). Live raw flat at **277**, stubs 1052, eleven green. Two identical worker-launch failures again from a hand-typed clone path (`r-m154-wm8`); the relaunch into the spare clone worked. The 8-minute sweep pass (below) has already produced two ZERO results, so the next round is a port round. DECISION: nothing operator-only taken.
| 155 | 5 permuter-port workers | +2 | 3523 | The 8-minute sweep pass paid: both ZERO results closed on the first port - `world_map/misc3` `func_800771D8` -> **WorldMapApproachValue** (the winning edit is a no-op self-assignment of the parameter inside the `if (limit < 0)` branch, which stops cse folding `negu` onto the parameter: 99.67 -> 100) and `menu/misc5` `func_801D09F0` -> **MenuRenderWindow** (the `0x70` AddPrim base offset wants a source LOCAL, not a per-expression literal: 94.75 -> 100). Stubs 1052 -> 1050, eleven green. **A BETTER candidate can be wrong code, not a closer match**: in `world_map/misc11` the permuter's 163 -> 123 edit passes the STALE `unk30` to `WorldMapGetTilePlaneHeight` where retail passes the value stored two instructions earlier - it buys 1.5% by changing what the callee receives, and objdiff cannot see it because the callee is a relocation. The worker reverted to the semantically correct body (stays parked at 97.39%) and the lesson is now in `brief.js` with the two levers above. Parks improved with their residue named: `world_map/misc9` `func_80088570` -> **WorldMapInitializePlayerSlot** 95.92 -> **99.65%** (one `addu`/`sw` destination register for `pSlot->unk30`; permuter stopped at the 3-run cap, not converged - worth a resweep) and `battling/misc2` `func_80073424` 98.09 -> **98.79%** (retail keeps a redundant induction copy: 540 vs 544 bytes, a structural fold). Two more brief-vs-tree mismatches cost time: the round-151 hint for `func_80073424` described a different body (`p1[1] == 0x10` vs the 9-slot `lbu 0x67C` search loop), and three `extern`s had been left INSIDE that park's comment, so un-parking produced a unit that does not compile - the worker moved them above the stub. D_ globals flat at 1770, live raw flat at 277. DECISION: nothing operator-only taken.
| 156 | 4 field workers + 4 read-only naming workers | +0 | 3523 | No matches by design; the biggest field+data round of the session. Fields: `unk<offset>` **2366 -> 2281** (-85) - `battling/misc3.c` named the movement machine (`stepDistance`, `distance`, `gaugeLevel`, `actionPhase`, `gaugePercent`, `facingAngle`, `angleOffset`, `typeId`, each cross-checked against `misc2`/`misc17`'s existing spelling), `battling/misc14.c` named the model-node set `BattleUpdateNodeTree` drives (`localMatrix`, `worldMatrix`, `sourcePos`, `position`, `rotation`, `renderMode`, `depthShift`, `pPointerList`, `pImage`) and left 21 `FIELD_UNKNOWN`, `world_map/misc11.c` matched misc8.c's `WorldMapActor` (`command`, `targetSlot`, `state`, `x/y/z`, `dirX/Y/Z`, `angle`, `moveSpeed`, `pSprite`, `targetX`), `battling/misc4.c` named the second-pass remainders (`ActorScript.radius`, `prevState`, `baseVertexPos`, `edgeMidPos`, `BattleSlotReset.percent`, `flags2`) and reported two honest `FIELD_UNKNOWN`s. Data: **+103 `D_` globals** (distinct 1770 -> **1667**), including the archive debug format strings and CD state (`g_StrArchiveDebugStreamState`, `g_ArchivePendingReadCount`, `g_ArchiveStreamWritePointer`, `g_ArchiveHeaderOffsetBase`), the battle option-row strings (`g_StrFpsFormat`, `g_StrUser1`, `g_StrVibrationOn`, `g_StrNumberPlaceholder`) and one new function name (**WorldMapDrawQuadPrims**). All four naming workers independently reported that their files define no `func_` function in live C, which is the `unnamed_c.py` result (48 left, 22 files, mostly `psyq/`). Stubs flat at 1050, live raw flat at 277, eleven green. DECISION: nothing operator-only taken.
| 157 | 1 permuter-port worker + 4 field workers (2nd pass on 3 headers + misc18) + 4 naming workers | +0 | 3523 | No matches; `unk<offset>` **2281 -> 2222** (-59 after the naming pass), **+59 `D_` globals** (distinct 1667 -> **1608**), stubs flat at 1050, eleven green. Headers: `include/system/menu.h` took 44 more (`GearArmor.baseDefense`/`baseEtherDefense`, `GearFrame.agility`/`etherAmplification`, `GearWeapon.attack`, `equipGroup` in three record types, `GfxEnvironment.otBuffer`, five `shouldRenderStrings*`09E0/4E0/10E0/14E0/17E0 gates), `include/battle/actor.h` took 14 (`BattleBlock` flags/opcode/start*/target*/endFrame - the same 0x14 layout `include/libgear/main.h` already names - plus `BattleActor.hasSedsEntry`/`stepIndex`/`stepCount`/`pointIndex`), `include/main/game.h` took 7 including a real **SPLIT**: `GameCharacter.unk0[0x28]` -> `slots[4]` of an 8-byte `GameCharacterSlot` (flags, attack, characterId) plus `hitPercentageFromEquipment`/`evadePercentageFromEquipment`/`etherFromEquipment`, which removes every cast in the equipment-bonus code. Sound: both leads ported for +0.47 each (`func_8003E360` 94.84 -> **95.31%**, `func_8003E1F8` 85.66 -> **86.18%**) and both re-parked with the wall named; a second permuter run from each found nothing. **Two process notes:** (1) the `game.h` worker and the `menu.h` worker both renamed the same source lines, so their clones conflicted in `src/menu/main/misc10.c` and `src/shop_menu/main/misc.c`; `clone.sh merge` reports CONFLICT and leaves the main tree clean, so the fix is a manual `git fetch <clone> round-<tag>:tmp<tag>` + `git merge`, resolving each hunk to the header's split spelling (`slots[0].attack`) with the other header's field names - ten minutes, and the answer for the future is to keep two header workers on headers that do not share users in one round. (2) A commit I did not make (`86035cde`, "round scripts: two foreground lanes with a queue") arrived through a worker's branch: it adds `args.queue` and two foreground lanes to every `tools/rounds/` script and documents it in PROMPT.md; all six briefs still parse and the next round uses it. DECISION: nothing operator-only taken.
| 158 | field round with the new foreground lanes: 2 foreground + 3 queued workers, plus a 3-worker naming round at fg:0 | +0 | 3523 | No matches by design, and the new lane/queue harness worked exactly as its commit describes (six field workers from five slots, no idle foreground lane). `unk<offset>` **2222 -> 2121** (-101, the best field round since 143), **+13 `D_` globals** (1608 -> **1595**), eleven green. The battle actor's motion/anim machine is now typed from both sides: `battling/misc3.c` (pushVx/Vy/Vz, steerVx/Vy/Vz, steerSpeed, targetAngle, flashTimer, actionPhaseTimer, prevCenterPosition), `battling/misc2.c` (animSpeed, lastFrame, frameLimit, frameAccum, animFrame, frameDelta, gaugeMax, gaugePercent, typeId, pActionEntry) and `battling/misc4.c` (gaugeLevel, gaugeDamage, actionPhase, typeFlags, soundTimer, prevVertex0Y/1Y, animFrame, animFrameStep, actionIndex, speedScale) - three independent TUs agreeing on 0xBA/0xBC/0x644/0x909/0x998/0x99A. `include/battle/actor.h` (3rd pass) added the register banks `BattleSlotRegs.dwordRegs/wordRegs/byteRegs` (the `p[1]` operand the park notes kept casting), `BattleTableEntry.characterId`, `BattleRow.actionFlags/opcode/chance`, `BattleWorkBlock.actorIndex/rowIndex`, `BattleBlock.tag` and a SPLIT of `BattleActor.unkA8` into `pModelFile`/`unkAC`. `battling/misc6.c` named the sprite sheet record from `GetClut`/`GetTPage`/`LoadImage` call arguments (`clutX/Y/W/H`, `texX/Y/W/H`), which is as good as evidence gets. `world_map/misc10.c` named only 4 and correctly reported 41 FIELD_UNKNOWN - the honest outcome for a file whose readers are stubs. DECISION: nothing operator-only taken.
| 159 | field round: 2 foreground + 3 queued workers | +0 | 3523 | `unk<offset>` **2121 -> 2072** (-49), `D_` flat at 1595, stubs flat at 1050, eleven green. Best evidence of the round is again cross-file agreement: `battling/misc1.c` and `misc3.c` renamed 15 and 4 fields to the spellings `misc2/misc4/misc17/misc18` already carry (`modelIndex`, `gaugeValue`, `gaugePoints`, `flags`, `distance`, `targetAngle`, `typeFlags`, `baseVertexPos`, `savedModelIndex`, `actionWaitTimer`), and `world_map/misc9.c` named the scene layer's `pPrimHeader`/`pPrims`/`pPrimsCopy` plus the player's `x/y/z` and `dirX/Y/Z` to match `misc10.c`. `include/system/menu.h` named 11 more and SPLIT three opaque spans into real byte arrays (`shouldRenderStrings1BE0[4]`, `isGearScreenActive[3]`, `isPanelActive[9]`) - the first time a cast-into span in that header became plain field access. `battling/misc12.c` recognised its `BattleEntry` as a SPRT packet from the retail stores (tag length 3 at +0x77, code 0x7D at +0x7B, uv at +0x80, clut at +0x82) and split the span to give the tag its address field. Several workers reported FIELD_UNKNOWN with the cross-file reason (`unk4F` has a name in misc2 but no use here), which is the right shape of report. Sweep is on a 20-minute pass over bodies within 150 points. DECISION: nothing operator-only taken.
| 160 | field round: 2 foreground + 4 queued workers, plus a 3-worker naming round at fg:0 | +0 | 3523 | `unk<offset>` **2072 -> 2012** (-60), **+26 `D_` globals** (1595 -> **1569**), stubs flat at 1050, eleven green. Two renames today came from the *other* compilation of the same layout rather than from the file's own readers: `battle/misc22.c` named `BattleMisc22Image.mode`/`isLoaded` by matching `libgear`'s `Temp8`/`LibGearFlushTexture` at the same offsets (its `BattleFreeImageEntry` is the same function), and `battling/misc5.c` named its sprite-slot `x/y/z/life` from `battling/misc6.c`'s committed declaration of the same 0xC table. `slus_006.64/system/system.c` typed `SystemString` from the string-drawing code (`lineWidth`, `charIndex`, `x`, `y`, `lineHeight`, `lineOffset`, `bufferCount`), which is the struct every overlay's text goes through. `world_map/misc6.c` and `battling/misc17.c` renamed the actor/work fields to the spellings their six siblings now share (`command`, `state`, `stateTimer`, `x/y/z`, `dirX/Y/Z`, `gaugeValue`, `gaugeMax`, `typeId`, `difficulty`). A useful negative from `include/main/game.h`: the worker tried the `u16 actorAngles[3]` split of `unk1826`, `culprit.sh world_map` named `WorldMapInitPartyActorSprite0/1/2` as the regressions, so the split was reverted with the evidence left in a comment - the first time a worker used `culprit.sh` to reject its own change. DECISION: nothing operator-only taken.
| 161 | field round: 2 foreground + 3 queued workers, plus 1 port worker on the last non-psyq BETTER lead | +0 | 3523 | `unk<offset>` **2012 -> 1949** (-63), `D_` flat at 1569, stubs flat at 1050, eleven green. `include/battle/actor.h` (4th pass) took 10 more from the battle units' live readers (`BattleBlock.curFrame`, `BattleRow.power`/`chanceBonus`/`conditionKind`/`conditionFlags`/`element`, `BattleTableEntry.chance`/`conditionKind`) and SPLIT two rows into the words the code actually indexes. `world_map/misc12.c` and `misc15.c` renamed the slot/player records to the spellings the other eight world_map units use, and `battling/misc13.c` matched `misc14`'s node/part layouts field-for-field. **The port worker settled the last non-psyq sweep lead: `battle/misc7` `func_8007B6C0`'s candidate is the same false gain round 139 measured** (95.25% against the park's 96.17%, and a fresh 1840-iteration run from the body returns the identical candidate), so the BETTER list is now psyq-only and the sweep has nothing left to feed a port round. Also measured, for the next session's planning: of the 1050 remaining stubs, **879 are `convertible.py`-OK and 175 are link-BLOCKED**, and the fresh ∩ convertible pool is 15 units - i.e. the remaining work is parked C at 90-99% against codegen walls, not unwritten C. One launch mistake of mine cost nothing: two workers were given the same clone, the second was interrupted before it wrote, and it was relaunched into the free clone (its report arrived normally). DECISION: nothing operator-only taken.
| 162 | 4 table-typing (REWRITE) workers + 2 read-only naming workers | +1 | 3524 | **First match in eight rounds**, and it came out of the REWRITE shape rather than a sweep lead: `battle/misc12` `func_800841E0` -> **BattleBuildTargetCandidateListSameRowFirst** (872 bytes, CLEAN), closed by discovering that the parked 90.21% body wrapped its filter/copy loops in `if (count > 0)` when retail has no `if` at all - the `blez` is gcc's own zero-trip test, so removing both guards and writing plain `for (i = 0; i < count; i++)` with `n = 0;` dropped exactly the two extra instructions (96.61 -> 100.00); the two sorts are a selection pass against `cand[0]`, not a bubble pass on `[i-1]`. Stubs 1050 -> 1049, live raw **277 -> 275**, `D_` globals **1569 -> 1504** (+18 names, and typing deleted 39 `D_80077xxx` halfword externs in movie alone), eleven green. Typed: `movie/misc2` now declares `VECTOR g_MovieViewTarget/g_MoviePrevViewPosition/g_MovieViewPosition` (16-byte VECTORs, which is why they sit 0x10 apart) and `MATRIX g_MovieLightMatrix/g_MovieRotationBaseMatrix` - and gcc still emits retail's `lui $at / sh %lo` macro forms for every member store, so nothing moved; `world_map/misc1` typed `g_WorldMapStreamQueue` as `StreamDataQueueEntry` records. **Two lessons for the runbook:** (1) a table that retail addresses entry-by-entry does NOT want one array object - `world_map/misc1` measured the array form at 87.22% (gcc hoists the base across the HeapAlloc calls where retail re-forms `lui $at` per store), so the shipping form is one typed object per entry; (2) **a park note whose percentage came from a body that does not compile is fiction** - `battle/misc8` had FOUR such notes (`g_BattleSlotFlags[idx * 8] != 0` against a `BattleSlotFlag[]` array, cc1 exit 33, and `diff_func.sh` silently diffing a stale object), so their 83-91% readings are unreliable until re-measured; the worker fixed the expressions and re-parked with the real numbers (98.55% for the one it re-measured). DECISION: nothing operator-only taken.
| 163 | park-repair round: 2 foreground + 4 queued workers, one unit each | +3 | 3527 | Best round since 149. **The sweep's 82 ERROR rows were not dead ends, they were parks whose bodies never compiled**, and repairing the compile often moved the percentage 5-9 points: stubs **1049 -> 1046**. `battle/misc8` `func_8007C33C` -> matched (was "89.69%", really 100%) and `battling/misc18` `func_80090CC0` + `func_8008FFEC` (RELOC) -> matched, while every other repaired park got a REAL number: `battle/misc8` `func_8007C1A4` 83.91 -> **91.52%**, `func_8007C840` 82.94 -> **91.44%**, `func_8007CDD0` 99.21 -> **99.43%**, `field/main/misc2` `func_8007520C` 91.00 -> **100.00%** (still BLOCKED: owns `D_8006FB1C`, so it needs its dlabel data moved to C in the same commit - a follow-up job) and `func_80072D74` 89.60 -> **96.11%**, `world_map/misc6` `func_80080578` 34.97 -> **64.06%** and `func_8007FC8C` 88.66 -> **93.41%**, `world_map/misc4` `func_8007A1B4` -> **99.90%** (retail's exact size, one register). **The three root causes of the fiction, all now fixable in one line:** indexing a struct array with a BYTE offset (`g_BattleSlotRecords[idx * 0x1C]`), comparing two structs (`a != b`), and an `extern` signature that disagrees with the parked definition (`extern void f(void)` vs `s32 f(s32)`, `extern s32 arr[]` written as a scalar) - cc1 exits 33 and `diff_func.sh` quietly scores the STALE object, so the note keeps whatever number it had. Two more levers reported: `do { pWork->breakThreshold = thr; } while (0);` around a single store stops cc1 sinking it below a following load (`func_80090CC0`, a "scheduler wall" note from an earlier round), and in `world_map/misc6` the same `do/while` got +29 points on `func_80080578` (34.97 -> 64.06), contradicting that unit's "PROVEN: no preset can fix this shape" note. Naming: +3 function names, +8 `D_` globals (1569-side count now **1494**), live raw 275 -> 280 (the repaired parks are honest bodies). DECISION: nothing operator-only taken.
| 164 | park-repair round: 2 foreground + 4 queued workers | +4 | 3531 | Stubs **1046 -> 1042**, eleven green, and **`battle/main/misc6_r2` is now a finished unit (zero stubs)**. Four matches: `battle/misc6_r2` `func_8007B2C0`/`func_8007B310`/`func_8007B360` -> **BattleWordRegAnd/Or/Xor** (RELOC, 80 bytes each; the register tie that three shapes and 2151 permuter iterations could not move was closed by spelling the whole `g_BattleSlotRegs[index].wordRegs[...]` chain at each of the three operands) and `battle/misc5` `func_8007A628` -> **BattleSlotFlagsC002ClearRecordIdle** (RELOC, 160 bytes; un-parked through the entry structs then one `do { } while (0);`). Every other repaired park now carries a real number, and four of them moved a lot: `battle/misc8`-style stale-typing hit `battle/misc16` (`func_8008F6E4`'s note said 57.38%, real **22.33%** - it was measured when `g_BattleDisplayState` was a `u8*` and the body does a struct assignment today) and `battle/misc4` (`func_80078E24` real 70.25%, `func_80078998` real **95.44%** after a lever that generalises: `extern u16 g_X[];` + `pValue = g_X;` materialises the address into a saved register where the scalar spelling folds it into every load - the same wall is parked in `battle/misc15` `func_8008ADD0`). `battle/misc5` `func_8007AA60` re-parked at **99.09%** (its body did not compile: `D_800D3430` undeclared). `world_map/misc8` produced a correction worth keeping: **`objdiff`'s `(NNN bytes)` is OUR compiled size, not retail's** - so a note that calls it "retail's exact size" may be wrong, and `func_8008390C` is 20 bytes from a match, not 4. New levers: offset-chain-first/base-last spelling for a u16 `|=` store (`battle/misc16` `func_8008D328` 93.85 -> **96.12%**), and `u32` vs `u8` on a stack parameter decides a 0x48 vs 0x50 frame in the 2.6.0 unit. Naming: +2 function names. DECISION: nothing operator-only taken.
| 165 | park-repair + 1 rodata conversion: 2 foreground + 5 queued workers | +2 | 3533 | Stubs **1042 -> 1040**, eleven green. **The "misc2 cannot be converted" wall was fiction**: gcc 2.7.2 emits a C data definition at its OWN position in the source, so a stub's string converts on its own when the `const` is written between the same two functions - `field/main/misc2` `func_8007520C` -> **FieldRenderGearFrame** with `const char D_8006FB1C[0xC] = "GEAR      "` in the same commit, `check_bin field` OK, and no yaml edit needed. That reopens `func_800752C8` (99.60%), `func_8007554C` (owns 5 strings) and `func_800759E4` (owns the `VECTOR D_8006FB70`) in that unit. Second match: `slus_006.64/system/graphics` `GfxLoadClutsAccelerated` 99.11 -> **100.00%** (a load-bearing `do { } while (0)` and an integer-sum CLUT address with the offset FIRST - every pointer spelling canonicalises base-first); its sibling `GfxLoadImageAccelerated` really is unconvertible (it switches `$sp` into the HeapAlloc'd block, which is not `alloca`). Every other repaired park got a real number, and several were dead bodies: `field/misc8` `func_80080F44` 71.39 -> **90.26%** (`D_800B2180[0]++` materialises the address, plus the do/while block) and `func_80081F80` unbuildable since an `ActorFlags12CView` typedef was deleted; `battle/misc32` `func_800BD1FC` re-parked at 92.59% with a typed row view and **4 bytes short of retail** (retail copies the sprite into `$a0` in the jal delay slot); `field/dialogue/text_box_render` `func_8008004C` confirmed at 91.11% and its callee `func_8007DCF8` given its real 3-parameter signature (which is why the park never compiled); `lib_gear` `func_801E1258`'s body had been dead for at least one header rename. **DECISION logged, not taken (operator-only):** `menu/main/misc4` is one-function-exception-each-way - `MenuUpdateSubMenuChoice` is 100.00 under Default and 98.13 under 2.6.0, while `func_801CE540` is 100.00 under 2.6.0 and 97.78 under Default - so the unit wants a split at `func_801CD81C`, which is two files and one preset entry and would buy one match; PROMPT's ground rules make a preset change that takes an already-matched function below 100.0 operator-only, so it is recorded for the operator rather than done.
| 166 | park-repair + 2 data-owning conversions: 2 foreground + 4 queued | +2 | 3535 | Stubs **1040 -> 1038**, eleven green. `field/main/misc2` `func_800759E4` -> **FieldMatrixBuildRotationFromVector** with `const VECTOR D_8006FB70` in C (292 bytes) - second data-owning stub of that unit converted with no yaml edit, so the `convertible.py` "owns data" class is real work; `battle/misc22` `func_800A32D8` -> **BattleSetupPositionTween** (428 bytes) via a new lever: a `*(volatile s16 *)&pCmd->startX = ...` store defeats gcc's memory CSE, which had been folding `startX = a*64; deltaX = (startX - d*64)/divisor` into `(a-d)*64` and rescheduling everything (74.21 -> 100.00); `volatile` on the STRUCT FIELDS is wrong (lhu + sll/sra). Every other repaired park got a real, and often much better, number: `field/misc2` `func_80072D74` -> **98.25%** at retail's exact size (a `pAnimFileData = pAnimFile;` first statement keeps a copy retail has), `system/rendering` `func_8001E148` 90.89% and `func_8001F1D4` 77.38% (both bodies had been dead: a nonexistent `field_0xAC`, an extern signature that disagreed with the definition, undeclared `POLY_FT4`), `system/temp1` `func_8002435C` 89.07 -> **99.19%** (a `u8 isBattle = g_IsBattle;` local before the flag read-modify-write). **Tooling: `convertible.py` was calling every data-owning stub blocked** because it matched the FIRST `.size` in the `.s` (the rodata label's) instead of the function's own; after the fix the 175 BLOCKED become **122 owns-data (convertible, write the const between the same two functions) + 52 real post-`.size` padding**. Also recorded in `brief.js`: a park block must not contain a star-slash - a `/* 0x38 */` offset comment inside it closes the enclosing comment, the body reads as live C, lint reports "both INCLUDE_ASM'd and defined in C", and `permute.sh` dies with `Syntax error in base.c` (temp2's `func_8002709C` had been in that state since an earlier round, so its 80.74% came from a stale object). Naming: +1 function, +3 data. DECISION: nothing operator-only taken.
| 167 | close-the-99% round: 2 foreground + 4 queued workers | +2 | 3537 | Stubs **1038 -> 1036**, eleven green. `field/main/misc2` `func_800752C8` -> **FieldRenderActors** (400 bytes, 100.00%, with `const char D_8006FB28[0xC] = "CHAR      "` in source position - third data-owning stub of that unit converted with no yaml edit) and `world_map/misc11` `func_8008B2BC` -> **WorldMapModeInitActorSlot** (JTBL exception: 99.92% with only the `jtbl_80070638` operand differing, `check_bin world_map` OK). **Both closures used a lever this session found, on a DIFFERENT class of tie than it was found for:** the `do { } while (0);` block (found for store-cluster order and for a single sunk store) also fixes a GIV tie - `func_800752C8` was a pure pActor/pActorData `$a1`/`$a2` swap that no declaration order moved, and the permuter's ZERO candidate's load-bearing edit was exactly that wrapper (its other edit, a `volatile` return type, was not needed). And the volatile-store lever, found for a folded write-back, works as a **barrier**: `*(volatile s32 *)&pActor->x = g_WorldMapPlayerPosition;` stopped gcc hoisting the `D_8009C5B4` load above the store, which put retail's `nop` back in the load-delay slot and finished `func_8008B2BC` (the round-155 candidate for that function was wrong code; the correct body had been parked at 97.39%). The three other parks got mechanism-level diagnoses instead of guesses: `world_map/misc4` `func_8007A1B4` is a `combine_regs` MERGE whose live range spans a `li`/two stores (so `$v0` is never free), `battle/misc8` `func_8007CDD0` is a commutative-sum tie with eight spellings and a 2167-iteration permuter at 30/30, `battle/misc5` `func_8007AA60` established that gcc swaps `mult` operands only when BOTH are registers (so row-first source and retail's load order are mutually exclusive), and `battle/misc16` `func_8008CFB8` improved to 98.49% with the remaining blocker named (the `arg0 * 0x170` scale looks like a source statement but its local needs a stack slot). Naming: +1 function (`FieldRenderActors`), unnamed C definitions 49 -> 54 (the two conversions added definitions faster than names landed). DECISION: nothing operator-only taken.
| 168 | close-the-99% cluster round: 2 foreground + 4 queued workers | +2 | 3539 | Stubs **1036 -> 1034**, eleven green: `battle/misc3` `func_80078508` matched (100.00%, new C; retail's save+copy ORDER was the lever - a separate `pBase` local plus `pFlags = pOut;` as a statement so the parameter copy lands third) and `battling/misc3` `func_8007661C` -> **BattleDrawSlotGaugeText** (99.72 RELOC, `check_bin battling` OK) - the canonical "permuter ZERO but not a match" case, closed by a typing bug the note had wrong: `D_80096FB8[...* 5]` on a 0x14-byte record is a 0x64 stride, and once the gauge table is read through one typed record array the frame lands at retail's 0x30 with no permuter run at all. **The 99.94% cluster I selected from was stale data**: `permuter-results.tsv`'s park column is from the run date, and re-measuring gave 94.68 / 75.45 / 56.44 in `battle/misc3` and 96.00 / 0.00 / 97.82 / 99.94 in `archive_b` - so brief selection must re-measure first (every worker this round did, and reported the real numbers). Two walls are now named precisely: `world_map/misc12`'s `func_80090C68`/`func_80090E14` are **JTBL_LAYOUT** - instruction-identical at 99.90-99.91% but gcc emits `.align 3` tables where retail packs them 4-byte aligned, so the C table lands 4 bytes late and shifts `world_map.bin` (a family conversion cannot help; padding only ever adds); and `battling/misc2` `func_80073F34` (99.91%, one `lbu` destination register) is now explained by gcc 2.7.2's `local_alloc` CHARGING a register to a basic block (`lh a2,6(s1)` earlier in the block) and refusing to reuse it for a later unrelated pseudo - no source shape moves it, and the `do/while` lever backfires here (81.74%). `menu/misc11_r2` `func_801E5B88` is **100.00% under gcc 2.6.0** and 99.94% under the unit's preset with the rest of the unit regressing under 2.6.0 - a one-function split; the worker wrote the exact `config/menu.yaml` three-range edit into the park note. DECISION logged, not taken (operator-only): that `menu/misc11_r2` split and the round-165 `menu/misc4` split (at `func_801CD81C`) are each a new source file plus two config entries - over the three-file line for an unattended split - and each would buy one match.
| 169 | close-the-99% round on freshly re-measured parks: 2 foreground + 3 queued | +2 | 3541 | Stubs **1034 -> 1032**, eleven green. `world_map/misc6` `func_8007F968` -> **WorldMapUpdateSlotGroupTransform** (99.88 RELOC, `check_bin world_map` OK) - and the whole residual was a FRAME SIZE: retail's 0x30 vs our 0x28, restored by one declared-but-unused `SVECTOR vec;` local, after which every instruction byte matches; the old park's `if (1) {}` permuter artifact was unnecessary. `battle/misc11` `func_800819A4` matched 100.00% with a positional lever worth keeping: the shared base `pTarget = g_BattleCommandQueue + idx;` must be assigned BETWEEN two specific stores so its load CSEs with the right neighbour (assigned first it CSEs with a different load: 99.63%, same size, wrong register). Every other target got a floor, not a guess: `battle/misc19` `func_8009A258` 99.68% and `func_8009BAC4` 99.72% are one allocator choice each and both go BACKWARDS under cdk (90.57 / 95.97) - so the pending cdk split for that unit must exclude them and can only cover `func_8009A9D0`/`func_8009ADA0`/`func_8009BD94`, i.e. per-RANGE, which is a bigger job than the note said; `battle/misc24` `func_800AA898` is one displaced `addiu` whose placement looks like a dead-store hoist in the original source; `battle/misc1`'s two sweep ERRORs turned out to have NO parked body at all (they are the unit's two largest stubs, a from-scratch write) while `func_80071964` is a 99.51% allocator tie the permuter calls converged (2081 iterations, 20/20). Naming: +2. DECISION correction: the round-168 `battle/main/misc19` cdk split is narrower than logged - only three functions stay in it, and `func_8009BAC4`/`func_8009A258` must stay on Default, so it needs a per-range split (more files than the three-file line) rather than a unit switch.
| 170 | close-the-99% round: 2 foreground + 4 queued | +2 | 3543 | Stubs **1032 -> 1030**, eleven green, both matches in `battle/misc8`: `func_8007C9D4` -> **BattlePickCharSlotByFlagRetry** (99.88 RELOC) and `func_8007D478` (99.74 RELOC), each closed by a NEW structural lever that is now in `brief.js`: when retail's preheader ends with a copy of a loop-invariant pointer, the source had a SECOND variable with the same value re-derived after a loop-body use of another invariant - gcc's `move_movables` then hoists both in body order and the copy survives (a 4-minute permuter run on the 98.43% body found nothing). `battle/misc11` `func_800826CC` went 84.29 -> **96.00%** (exact size) from a second new lever: two references to one global get separate `lui $at / addu $at / %lo(at)` forms only when their RTL differs, which needs the index in a NAMED local and the symbol spelled on OPPOSITE sides of the sum (`off + (u32)sym` vs `(u32)sym + off`) - that also explains `battle/misc19`'s 94.23% park of the same access, which had ten failed spellings. Everything else got a floor: `field/misc2` `func_80072D74` stays 98.25% (cse hoists a displacement-free global's address; the fix is a shared-header split, which would break `field/main/misc` at merge), `system/rendering` `func_8001E148` 90.89% (gcc only keeps a symbol-address pseudo in a register when it is born in a different basic block, and then it sorts LAST into s3), `world_map/misc11` `func_8008B54C` 93.37% (the same three arg-setup insns fail in two neighbouring long sprite-init functions, so it is a context effect, not a spelling), `battle/misc4` `func_80078998` 95.44% and `func_80078E24` 70.25% (a pure scheduler difference: retail interleaves the copy block, ours hoists all four loads). Naming: +2. DECISION: nothing new; the three logged split candidates (menu/misc4, menu/misc11_r2, battle/misc19 per-range) still stand for the operator.
| 171 | 7 deep workers (jump-table harvest) + 4 read-only naming workers | +5 | 3548 | Stubs **1030 -> 1025**, eleven green. The queue this round mined was the 111 stubs that own a `jtbl_` table in a unit that already owns its rodata - untouched since round 56 - and the harvest is real where the layout allows it: `world_map/misc15` matched THREE switch stubs together (`func_80097800` -> **WorldMapUpdateSlots**, `func_800968E0` -> **WorldMapStepCdReadState**, `func_80096A6C` -> **WorldMapCdReadCallback**, all 99.79-99.90 JTBL, `check_bin world_map` OK) plus `func_800979C8` -> **WorldMapLoadClutAndTpageIds** at 100.00% (permuter ZERO in 40 s: `s32 i = 0;` -> `s32 i;` with `i = 0;` after the `HeapFree`, which stops sched2 hoisting the `sw s2`/`move s2,zero` pair above the `LZSSHeapDecompress` argument setup), and `battle/misc18` `func_80097964` -> **BattleTryApplySlotCondition** (JTBL, 932 bytes = retail's, `check_bin battle` OK). **My jump-table screen was measuring the wrong thing and that is the round's lesson:** I ordered the queue by vram-address alignment, but cc1's `.align 3` pads against the start of the unit's `.rodata` RANGE, so `battle/misc11`'s five 98-99.9% parks (range starts 0x8006FE7C = 4 mod 8; seven tables at +0x194 + n*0x20, all 4 mod 8 in section space) are each +4 bytes: `func_800816F8` converted to an instruction-identical 420 bytes and `check_bin battle` still said 343940 vs 343936. A unit's tables are a CHAIN - the first table's size sets the parity of the rest - so the whole switch family converts in address order or none does; both PROMPT.md and `brief.js` now say section-relative and whole-family. Every other failure got a mechanism: `battle/misc19` `func_8009BAC4` is **100.00% under gcc 2.6.0** (180/180 instructions and all 11 table words identical to `expected/`) but the unit is Default/2.7.2 and four of its matched functions regress under 2.6.0 (`BattleClearSlotFlag15A1` 95.71 worst), so it is a per-range split for the operator - and my brief said XenoBattle260 for that unit, which `gears.toml` does not support; `field/misc12` `func_80099AC0` is 1076 vs 1080 bytes (one load-delay `nop`, 12 shapes and 3777 permuter iterations, note consolidated); `gear_shop_menu/render` `func_801D2B74` is a 633-insn global-alloc tie (110 of 112 segments exact, 18 `addu` destinations) and the only stub left in that unit; `world_map/misc4` `func_80077E68` was decoded from a bare stub to 94.17% with the 3-control-point quadratic-interpolation idiom written down. REWRITE on `world_map/misc5`: live raw **52 -> 40** (both `WorldMapUpdateGroup{4,5,6,7}Scale` typed onto the 0x54-byte scene-layer record, CLEAN at 100.00%) and four measured ROUGH verdicts where the byte view is load-bearing for retail's `lw global / nop / sw slot` order (73.27% typed). Counts: live raw **288 -> 276**, `D_` globals **1482 -> 1462** (+27 entries, 2 name clashes), unnamed C definitions 54 in 25 files (29 of them `psyq/`), `unk` fields 1957, +7 function names. Also: 117 merged stale clones deleted (9 GB), the 20-minute sweep killed (it had been running 7.5 h at `-j 4` and was starving the round's compiles). DECISION logged, not taken (operator-only): `battle/main/misc19` needs a per-range split at func_8009BAC4's run to get a 2.6.0 preset for it - one new file plus yaml ranges, and four matched functions must stay on Default. DECISION: nothing else operator-only taken. |
| 172 | 8 deep workers + 2 queued, jump-table harvest with the CORRECTED screen | +8 | 3556 | Stubs **1025 -> 1017**, eleven green: the best matching round since 149, and all eight came from stubs that own a `jtbl_` table. Screen (section-relative, now in PROMPT.md): of 55 units owning rodata with tables, **42 pass** and 7 are walls, including the ones round 171 wasted. `battle/misc25` `func_800B12D0` -> **BattleTestActorCommandMask** (JTBL) plus `func_800AFC68` -> **BattlePartMatrixTask** at 100.00% (a 91.71% park closed by building the scaled node address with the offset as the LEFT operand: `(u8 *)offset + (u32)pMat`; `pMat + offset` folds back); `battle/misc12` `func_80085084` -> **BattleRunTargetSlotSelection** (JTBL, base-first `addu` from the opposite-side spelling `*(g_BattlePulseTrianglePrims + i + 0xE6)`); `battle/misc23` `func_800A6884` -> **BattleApplyColorTint** (100%, and this one breaks a wall nine rounds called unbreakable: the six-word table is five case words plus a trailing pad that belongs to the UNIT's `.rodata` RANGE, so `config/battle.yaml` now splits at 0xA78, splat writes the pad as `asm/battle/data/A78.rodata.s`, and the stub un-parked with `check_bin` staying OK at every step - the general fix for every PADWORD verdict is a yaml split, not a C spelling); `battle/misc13` `func_80085EB4` -> **BattleCheckCommandKeyStatusBit** (JTBL, and it proves a CHAIN is not chained: C tables interleave with the stubs' asm rodata in function order, so a mid-chain table converts alone, unlike world_map/misc15); `menu/misc10` `func_801E2BE4` -> **MenuRunCharacterDressingRoomLoop** (JTBL, `lastSlot` wants a two-statement split and case 3 a pre-increment expression); `battling/misc1` `func_80072170` -> **BattleStepDisplayState** (JTBL); `battling/misc17` `func_8008F280` -> **BattleRunActorStateHandler** (JTBL, and its park had been scoring a STALE object since a shared-header rename: `pWork->unk2C.flags` no longer compiled). Two more levers recorded by workers: `ppWork = &pActor->pWork; pWork = *ppWork;` pins a load above a global store where the plain form sinks below it, and `check_bin` failing with `Error 33 / parse error before '/'` means a re-park left its body outside the comment. Counts: +8 function names, +8 `D_` globals (**1462 -> 1461**), `unk` 1957 -> 1960, live raw **276 -> 289** (the new bodies' ROUGH spellings: `*(g_BattlePulseTrianglePrims + i + 0xE6)`, the `BattleEffectCells` cast form, `D_800CCD36[...]`) - so `battle/misc12`, `battling/misc1` and `battle/misc13` owe a rewrite round. Table for the next session: 42 PASS units, biggest remaining parks `world_map/misc12` (9 tables, 4 parks 91-99.9) and `menu/misc9` (3 parks 93.9). DECISION: nothing operator-only taken (the battle.yaml split is the documented per-unit rodata carve, and `make check` stayed green). |
| 173 | 8 deep workers + 2 queued, jump-table harvest third pass | +3 | 3559 | Stubs **1017 -> 1014**, eleven green. `menu/misc12` `func_801E8070` -> **MenuSetRowQuadVertices** (99.96 JTBL) and `battle/misc32` `func_800BD3AC` -> **BattleAttachEffectSprite** (99.96 JTBL, plus `Misc32Task` unified with the effect-task struct four already-matched neighbours use), and one non-jtbl: `battling/misc3` `func_80075A4C` -> **BattleActorHitTestCircle** at 100.00% where a `short` return type had been adding `sll/sra` (the old park note's byte count was wrong, 260 not 268). **The jtbl queue is thinning and it is now clear why:** the tables were never the problem - the residuals are ordinary allocator/scheduler ties, and 7 of 8 workers came back at 95-99.5% with the instruction stream already identical. The most useful levers the round produced: in `battle/misc15` case 4 the state arms must be written **store-before-increment** (`g_BattleCursorScrollY = (j + 1) * 13; j++; i--;`) which is what produces retail's per-arm `andi v1,(u8)j; addiu v1,v1,+-1` and the `-1` shared between states 2 and 4, and `func_8008BED8`'s only residual is one missing `li s0,1`: gcc reuses `pagePending` where retail rematerialises the literal, and moving the initialisation after `BattleInitPulseTrianglePrims` produces the `li` but moves the save pairs (97.17%). `battle/misc34` `func_800C0564` is exact-in-size with 3 pure allocation differences and its tables must move with `func_800BFE48`'s (the family is a chain). `world_map/misc10` `func_8008A2C8` is 612 bytes = retail's with only the `SpriteAllocFromAnimFile` prologue differing, and needs two **volatile** casts that are load-bearing, not debris. `lib_gear/main` `func_801E5D44` was decoded from nothing to 88.82% with a new `ModelScriptCommand` type whose union payload at 0x7 is load-bearing (flat bytes give `lbu` where retail has `lhu`). `shop_menu/misc` `ShopMenuSellMenu` is 98.61% with one memory-resident coalesce counter left. `battle/misc10` `func_8007F8C0` is 4 bytes short in the accumulator copy only, and `--sweep` rules the preset out under every 2.7.2 variant. Counts: +3 function names, +8 `D_` globals (**1461 -> 1462**), `unk` 1960 -> 1966 (workers split fields in their own files), live raw flat at **289**. Both queues that produced this session's 16 matches are now thin: the PASS jtbl set is down to ~35 units and their parks are 90-96%, so the next round should be a REWRITE round on the raw debt (world_map/misc5 40, battle/misc25 11, battle/misc8 10) and a field round on the `Model`/`BattleActor` splits the workers reported, with a fourth jtbl pass only on the smallest parks. DECISION: nothing operator-only taken. |
| 174 | permuter port + 7 rewrite workers + 2 queued | +1 | 3560 | Stubs **1014 -> 1013**, eleven green, and **live raw 289 -> 235 (-54, 68 files -> 64)**: the rewrite round the jtbl matches owed. `battle/misc8` `func_8007CDD0` -> **BattleStoreSlotTagIdByte** matched at 100.00% from the sweep's ZERO (the winning edit is a dead local reused as the stride source, `slotTag * (op = 0x40)`); its file also retyped four neighbours' parameters to `BattleSlotArgs **ppArgs, u8 row`, which is reloc-only and `check_bin`-proven. Rewrites that came out clean at 100.00%: `world_map/misc1` (0 raw, whole unit - `WorldMapPointTableBlock` is a union so the six cast sites become `pTable->block + pTable->offsets.pointOffset`, and it must stay ONE pointer: a second base local costs an instruction), `battling/misc15` (0 raw - the PsyQ `POLY_F4`/`POLY_F3` types reproduce retail's 0x18/0x14 strides via `sizeof`, and the cursor advance must keep its CAST spelling), `battling/misc14` (0 raw - three real types split out of one conflated 0x14 record), `world_map/misc11` (0 raw - one `WorldMapSprite` view cleared seven functions at once), `battle/misc25` (`BattlePartMatrixTask` typed onto `BattlePartTask`/`BattlePartNode`/`MATRIX`/`LVEC3`, and the typed array index KEPT round 172's offset-first `addu`, so that hack is retired), `system/sound` (2 of 7), `lib_gear/main` (7 -> 3). Five ROUGH verdicts are now measured, not guessed, and each says the same thing: gcc hoists a typed field load above a store where the u8-cursor form interleaves it - `world_map/misc5`'s three WorldMapPlayer functions (73.27 / 62.97 / 82.99, re-measured, plus a `s32 *pPos` form at 54.53), `SystemInitializeFont` (13 spellings, best struct form 73.18), `lib_gear`'s two appended-block casts. Those 40 lines in `world_map/misc5` are the last big block and they look un-typeable without changing the load order; the file is now small enough to leave alone. Counts: +1 function name, +2 `D_` globals (**1462 -> 1460**), `unk` flat 1966, unnamed C definitions flat 54. Next round is the field round the header backlog needs (include/battle/actor.h 126, menu.h 125, game.h 120, plus the .c-declared structs in battling/misc3/misc4/misc12), which cannot share a round with rewrites because a field worker edits its header's users. DECISION: nothing operator-only taken. |
| 175 | field round: 6 workers + 2 queued, one per declaring file | +0 | 3560 | `unk<offset>` **1966 -> 1889 (-77)**, stubs flat at 1013, eleven green, and this is the round that closes the header backlog's easy half. `include/battle/actor.h` (126 declarations) resolved the command/argument block from `BattleGetActorActionEntry` and `BattleAppendActorCommandBytes` (`commands`, `commandArgs`, a SPLIT), `isVisible` from the draw gate in misc22, `doUpdate` from the per-frame gate in misc24, and `BattleSlotState.unkB6` -> `flags` from 13 masking sites; `include/system/menu.h` (125) took 12 renames at the primitive-buffer level (`pStringBank`, `pSoundFile`, `pGoldWindow`, `pPlayTimeDigits`, `pCursorPrims`, `pSubmenuPrims`, `pEquipmentPanel`, `characterWindows[3]`, `shouldRenderCharacterWindows`); `include/battle/battle_work.h` (37) named the whole primitive-list state machine (`spriteColumnPending`/`PackageCount`/`PrimStart`, `menuColumn*`, `cursor*`, `primStart`) which is the pattern three per-column lists share; `include/libgear/main.h` named `Model.modelSlot/frame/frameCount/commandIndex/numCommands/pScriptCursor/pScriptStart/numModelListEntries/pModelListEntries` and SPLIT `unk90` into an s16 plus three u16 exactly as a round-173 measurement predicted. The four `.c`-declared files split their opaque spans rather than guessing: `battling/misc12` turned `BattleMisc12Obj.unk2C/34/44` into `sourcePos`/`position`/`rotation` because misc14's `BattleUpdateNodeTree` reads them as SVECTOR/VECTOR; `world_map/misc10` named `targetX`/`targetZ`/`drawAngle` from misc11's movement code and correctly reported 31 fields UNKNOWN (their only readers are still stubs). One conflict to note for the future: the actor.h and battle_work.h workers both touched `src/battle/main/misc17.c` and `git merge` auto-resolved it, which is the case round 157 lost ten minutes to - so keep header workers on non-intersecting USER sets (actor.h users are battle/field/slus/world_map, menu.h's are menu/shop_menu/gear_shop_menu/member_change_menu) and accept the same-bin risk only between headers that name different offsets. Counts: `D_` globals flat 1460, live raw flat 235, unnamed C definitions flat 54. DECISION: nothing operator-only taken. |
| 176 | 5 matching workers + 1 queued fresh stub, 2 field workers | +1 | 3561 | Stubs **1013 -> 1012**, eleven green, `unk` **1889 -> 1878**, live raw flat 235. `battling/misc6` `func_8007E8AC` -> **BattleGetFontGlyph** (JTBL 99.76%, check_bin OK) took three levers nobody had written down: the case index must live in the PARAMETER (`c = c - '0'`, a separate `s32 glyph` allocates to `v0` and costs 19 points), the table's last word is a real case (`case ':'` -> glyph 0x25) so the case bodies are listed in glyph-index order not character order, and the case range is 27 entries (`' '`..`':'`). Fields: `include/main/game.h` named the gear-weapon cache (`GearWeaponData.weaponId`/`flags`) and **SPLIT `GameCharacter.unk34`** into `unk34[0x10]`+`unk44`+`unk48`, which deleted the `MenuCharacterWideNumbers` cast workaround and every cast in that code; `battle/misc12.c` named the curve tables (`BattleUnk48.curveTargets/Modes/Values`), the slot-entry flag words and `counter`/`counterLimit`, and found a RANK VIEW of `g_BattleSlotEntriesBase` at +0x4C. Every other assigned function parked with a real number and a named wall: `battle/misc17` `func_80094EE4` reached **99.86% with retail's exact 1964 bytes** and needs two things to un-park - the pad-word carving I approved for its table (`- [0x894, rodata]`, the round-172 misc23 recipe) AND an `lbu`+`bltz/slti` guard no spelling produces (retail compares amountSource signed off an unsigned load); `world_map/misc4` `func_80079778` decoded from nothing to **94.77% at retail's exact 2292 bytes** with the residue reduced to a callee-saved rotation; `battle/misc30`'s four stubs re-measured (`func_800B9284` 96.61% = exact size, two-register tie; `func_800B905C` 94.47%; and `func_800BB080` screened under every 2.7.2/2.6.0 -G0 variant, all 77.23%, so its preset is not the cause). `menu/misc9`'s two parks are now pinned to unmovable tie-breaks (retail rematerialises the a6=1 per call; four constant pseudos of 1 cannot survive cse). **Screen fix: `slus_006.64/system/menu` `func_8001C76C` is NOT a function** - 92 `.word` lines, no `jr $ra`, 368 bytes, unreferenced, and the linker puts that unit's `.text` immediately before the next unit's so it can never be C (fresh_stubs.py counts it because convertible.py counts words; the tracker had already recorded it twice). Process: I launched the round with field-shape workers in brief.js and the workflow died at the key with `TypeError ... 'replace'` - nothing touched, one relaunch, and PROMPT.md now says one shape per launch; `fg: 0` rounds never drain their queue, so a queued worker in one does not run. Counts: +1 function name, +1 `D_` global (**1462 -> 1461**). DECISION: nothing operator-only taken. |
| 177 | field round second pass: 8 workers (one relaunched after a clone-path typo) + 3 read-only naming workers | +0 | 3561 | `unk<offset>` **1878 -> 1805 (-73)**, `D_` globals **1461 -> 1448** (+12), stubs flat at 1012, eleven green, live raw flat 235. This round went to fields and names because two matching rounds in a row landed under 3 (the stop rule), and both paid: `include/battle/actor.h` took `BattleActor.pBuffer`/`isBusy`, `BattleSlotState.penaltyPercent`/`conditionTimers`, `BattleTableEntry.mp`/`maxMp`/`ether` (from the hp/maxHp mirror `BattleSetSlotConditionActionValue` and `func_80097D5C`), `BattleRow.evadeModifier`, and SPLIT `BattleTableEntry.unk04`; `include/main/game.h` took `UnkGearState.statusFlags` (the status mask decoded through `D_800C3468[]` at four sites) and `GameGear.etherAmplificationFromEquipment`; `include/system/menu.h` split `MenuManager.unk5` into the gold/play-time/unk358 gates and named the gear-shop inventory tables. The `.c` files split what the asm actually indexes rather than guessing: `battling/misc3` named the four input ramps from `func_80076884` and SPLIT `unk98`/`unkCC`/`unk918`/`unk1604` (four spans, from the load widths in the stub); `battling/misc4` named the actor-script camera block (`cameraAngleOffset`/`cameraYOffset`/`cameraRadius`) and the push velocities; `battling/misc17` named `BattleWork`'s four `actionChance` words and `breakThreshold` from misc18's live C on the same struct; `world_map/misc4` named the actor position/target block from `func_80079778`'s own decode. **Integration: the actor.h and game.h workers both renamed the same line of `battle/misc19.c`** (`unkB7[7]` -> `conditionTimers[7]` vs `unk1E` -> `statusFlags`) and `clone.sh merge` reported CONFLICT; I resolved it by hand keeping both renames, which is the round-157 failure mode appearing again - the answer is to keep same-bin header workers on offsets that do not overlap, or accept one manual hunk. Naming rounds are now done for this session: 3 workers on the six largest `D_` files produced **1 NAME (already applied) and 12 DATA**, the rest `DATA_INTERIOR` (table interiors, which is rewrite work, not naming). Also: the field workflow's `f5` died at launch on a hand-typed clone path (`r-m177-wm4f` for a clone named `r-m177-wm4`), and my relaunched worker then shared that clone with a queued worker on the neighbouring file - both committed cleanly (different files) and the merge was fine, but that is the round-161 collision pattern and the tag should have been reused deliberately or not at all. DECISION: nothing operator-only taken. |
| 178 | field round third pass: 8 workers | +0 | 3561 | `unk<offset>` **1805 -> 1762 (-43)**, stubs flat at 1012, eleven green, raw flat 235. The third pass still pays, and the evidence class that produced most of it was **retail asm read as a struct**: `battle/misc33` named its whole state machine out of the stubs (`pUpdate` from the `jalr` through `g_BattleState+0x8`, `timer` from `BattleIncrementStateTimer`'s own body, `pathIndex` from the 6-byte walk, `pendingStep` from `BattleApplyStateStep`, `pTargetSprite` from the callee's declared 2nd parameter); `battle/misc32` named the effect sprite colour/ramp block (`colors` and `prim` from `SpriteData`'s shared layout, `posX/Y/Z` from a `TransMatrix` on `+0x3C`, `pPrimBuffer` from the `HeapAlloc` store); `battling/misc1` named the script block's render fields from `BattleInitRenderScene`'s copies into the `g_BattleRender*` globals. Splits came from load widths in the stubs: `battling/misc3` SPLIT `unk1664` (a word pointer at 0x1664, a u16 at 0x1668) and `unk1604` (the two s32 gauge accumulators misc2 adds into), `menu.h` SPLIT `unk4F84` into a write cursor plus two per-card arrays. Two useful corrections: the `shouldRenderModalStrings`/`shouldRenderCursors` offsets in `menu.h` had been recorded three bytes early (the fields existed, the comments were wrong), and `battling/misc4` found `Actor.unk98`/`unk9C` are `stepForward`/`stepBack` because a MATCHED function in another file reads them. Counts: `D_` flat 1448, unnamed C definitions flat 54. This is the fourth field round of the session (-77, -11, -73, -43 from 1966 to 1762) and the headers still hold ~110 `unk` each; the evidence rule is what keeps it honest - every round reports FIELD_UNKNOWN where the only reader is a stub or an all-zeroing. DECISION: nothing operator-only taken. |
| 179 | field round fourth pass: 8 workers | +0 | 3561 | `unk<offset>` **1762 -> 1716 (-46)**, stubs flat at 1012, eleven green, raw 235 -> 234. The pass earned its keep on the accessory-effect family: `include/main/game.h` named the whole `accessoryEffectFlags{1,2,3,4,5,7,9}` set on GameCharacter AND the same offsets on GameGear (two independent switches over `effectType` in `MenuApplyEquippedItemBonuses` and `GearShopMenuRecalculateGearStats` agree, which is the strongest evidence class here - note case 5 ADDS while the others OR), plus `GameState.entryAngle`. `include/battle/actor.h` split `BattleActor.unk20` into `actorSlot` and took `blockTag`, `BattleTableEntry` `blockFlags` + `element`, and `BattleCommand.opcode/operand1..3` from the four byte/word/dword register ops. `battling/misc3` split `unkC6`/`unk911` and named `breakThresholdOffset`/`moveReentryTimer`/`lightAlpha`/`moveGoalPosition`; `battling/misc10` named `pObject`/`pTimData`/`typeFlags`/`requiredProgress`; `world_map/misc10` named the sprite trail geometry (`lifeTimerInit`, `trailStart`/`trailEnd` from the two `ApplyMatrix` calls, `trailStartSpread`/`trailEndSpread`, a SPLIT of `unk48` into `tpage`); `field/misc8` named `FieldExtentZone`'s half-extents/centres from the corner arithmetic and the Seds frame offset/sound-effect pair. `menu.h` took the inventory-row string blocks and a SPLIT of `MenuUnk9.unk0` into 16 name rows, 16 quantity rows and three prim groups. **Integration: the misc1 worker found its file declares no structs of its own and worked the same `include/system/menu.h` as the header worker**, so `clone.sh merge` hit three CONFLICTs; I resolved them by hand - both sides had named the same offsets, so it was a spelling choice (`itemNameStrings`/`itemQuantityStrings` from the header worker, `slotStrings` from the misc1 worker, which is the better evidence for the per-slot sprite record). That is the round-157 failure mode for the third time; the answer is that a `.c` file that only allocates header types must be briefed with `bins` on a DIFFERENT header, or not at all. DECISION: nothing operator-only taken. |
| 180 | 7 field workers + 1 matching worker on the 99.86% park | +1 | 3562 | Stubs **1012 -> 1011**, `unk` **1716 -> 1692**, eleven green. **`battle/misc17` `func_80094EE4` -> BattleComputeActionAmount** (99.98 JTBL, 1964 bytes = retail's) - the park that had been stuck since round 176 at retail's exact size on two residuals. Both fell: the pad-word blocker to the round-172 `config/battle.yaml` carve (`- [0x894, rodata]`, applied ALONE first with `check_bin` proving it byte-neutral before any C moved), and the `lbu`-compared-SIGNED guard to the shared-exit rewrite. That makes the carve a two-for-two recipe (misc23, misc17) and it is the general fix for a PADWORD verdict. Fields kept paying on the evidence classes that work: `slus_006.64/system/system.c` named the text renderer's row state out of the STUB asm (`scrollRow`, `rowCount`, `stopCode`, `clut`), `include/system/menu.h` recognised the PS1 save header (`saveFileMagic` = 'S','C', `saveIconDisplayFlag` = 0x11, `saveBlockCount`, `saveTitle`) and replaced two `unk` spans with a real `MATRIX` and `SVECTOR` (sizeof landing exactly on the next field), `battle/misc30` named `BattleState.stateStep`/`actorIndex`/`timer`/`pendingStep` from the state loop's own switch and `BattleActorSlot.x/z` from the comparisons against the sprite position, `battle/misc3` took `pPart` plus three SPLITs from two byte-identical matched functions, and `include/battle/actor.h` found the `accessoryEffectFlags` family mirrored on `BattleTableEntry` at GameCharacter's offsets. Counts: +1 function name, +1 `D_` global (1448), raw 234 -> 236 (two bodies' ROUGH spellings), unnamed C definitions flat 54. Note for the next session: `func_8009382C` improved 84.56 -> 89.66% in the same unit (retail keeps `&g_BattlePrimStartIndex` in a callee-saved register where gcc 2.6.0 folds every local spelling), and `g_BattleCurveTargets` is declared `u8[]` but every use indexes it `*4` - typing it is a two-function change and would clear a ROUGH line. DECISION: nothing operator-only taken. |
| 181 | 4 port workers (the non-psyq BETTER leads) + 5 field workers with 2 queued | +1 | 3563 | Stubs **1011 -> 1010**, `unk` **1692 -> 1635 (-57)**, `D_` 1448 -> 1447, live raw 236 -> 235, eleven green. **`world_map/misc13` `func_80092C70` -> WorldMapUpdateAreaNameString** (100.00%): the permuter candidate only reached 99.32% because the park's diagnosis was a symptom, not an allocator tie - retail's case-1 guard compares `g_WorldMapAreaId` against **-1**, which is why reorg had `li v0,-1` in the `beq` delay slot, and that in turns is the general rule "when a constant is materialised in a switch arm's branch delay slot and the next arm compares `$v0` without reloading, the arm's constant IS that value". The other three leads were measured and re-parked, not converted: `battling/misc2` 98.79 -> **98.97** and `archive_b` 97.82 -> **98.01** both took the `do { } while (0);` shim (the one lever that keeps paying across units), `world_map/misc7` `func_80080A28`'s lead was a **false gain** (83.64 vs the parked 92.64 - the permuter's metric anti-correlates with objdiff there, its own rerun's best was 89.10). Fields paid on the headers again: `menu.h` named titleStrings/characterSlot and SPLIT the dressing-room pointer block into `pCharacterSlots[20]`/`pGearBlocks[11]` (the 0x28-stride records tiling 0x20..0x9B by MAX_GAME_CHARACTERS/MAX_GAME_GEARS), `actor.h` named the whole `activeEffectFlags{1,2,3,4,7}` family on BattleTableEntry from two independent switches (the additive-vs-OR distinction recorded), `sound.h` named the AudioElement volume-ramp quartet, `libgear/main.h` the `ModelJoint` pRotation/pTranslation/pScale triple from a channel switch plus the Temp3 start/target lerp pair, and three `.c` files used the cross-file rule (battling/misc13 against misc14's matched BattleUpdateNodeTree, battling/misc3 against misc4/misc10/misc2, world_map/misc2 against misc10/misc11). Naming: **the proposal union is now exhausted** - 2336 NAME lines fed, 1 new function name landed; 1163 DATA lines fed, **1085 skipped "no C uses it"** and the rest were twins or malformed, so the `D_` backlog needs fresh evidence from a naming round, not a union pass. Process notes: 11 clones merged with **zero conflicts** (declaring files screened first, and the two port units' includes 
checked against each field worker's user set), and the three "needs attention: bash open 240s" notices were noise from a saturated VM (load 95, 4 build containers, 10 wibo compilers) - no worker was wedged. DECISION: the `misc27`/`misc27_b` tail of the cdk -G0 harvest was screened and buys nothing (every C function in both is already 100.00 under their cdk -G8 preset), so it is left as a cosmetic preset tidy rather than spent. |
| 182 | 6 field workers + 2 queued, plus a 3-worker read-only naming round at fg:0 | +0 | 3563 | Stubs flat at **1010**, `unk` **1635 -> 1598 (-37)**, `D_` **1447 -> 1440 (-7)**, five function names, live raw flat 235, eleven green. **The naming rounds were not done after all:** round 177 concluded they were ("1 NAME, 12 DATA") because the proposal *union* was drained, but three fresh workers over the six `D_`-heaviest non-SDK files produced 11 proposals with real evidence - `g_BattleFrameWaitCount` (BattleInitGraphics sets it to 2, the battle frame loop `lh`s it into BattleCompactOrderingTable's timeout), `g_BattleDebugDrawCallback` (cleared beside g_BattleDebugFlags, called by `jalr` with a0 = g_BattleRootOrderingTable when bit 0x10 is set), the field stream-player argument block (`g_FieldStreamFileIndex`/`TotalFrames`/`LoopPlayback`, named by matching the movie path's slots in movie/misc2), `g_SoundWdsEntryFile3`, `FieldFreeParticlePrimitives`, `FieldPlayStreamTextSegment` - and 710 DATA_INTERIOR lines correctly ruled not-names (they are table interiors, i.e. REWRITE work). So `name-brief.js` is worth re-running on a *new* file set every few rounds; the union pass alone cannot reach a `D_` nobody has read the surrounding C for. Fields kept paying on the mirrored-layout rule: `battling/misc4` took three `BattleActor` names purely from misc3's spellings at the same offsets, `world_map/misc8` took the dirX/Y/Z + colour-step family from misc9/misc10, `game.h` extended the accessoryEffectFlags family to `accessoryEffectFlags8` (SUMMED in case 8, not OR'ed - the additive/OR distinction already recorded) and `GameGear.accessoryEffectAmounts` (one summed amount per 0x8000>>i bit), and `system/system.c` typed the `SystemStringChar` SPRT block out of four stubs (`sprt[2][2]`, imageRect at 0x50, paletteIndex selecting g_SystemPalette1/2 by parity). Integration: 8 clones merged with one `git merge` auto-resolution (gear_shop_menu/main/render.c), no CONFLICT. A worker also reported that round 176's `GameCharacter.unk34` split is **not in this tree's history** - the unk44/unk48 fields it claims exist, the `unk34[0x10]` span does not, so that row's SPLIT line is wrong. DECISION: nothing operator-only taken. |
| 183 | 6 field workers + 2 queued, 3 read-only naming workers, 1 permuter port worker | +0 | 3563 | Stubs flat at **1010**, `unk` **1598 -> 1571 (-27)**, **`D_` 1440 -> 1422 (-18)**, live raw flat 235, eleven green. The one matching attempt was a permuter lead that turned out to be a **false gain again** (`battle/misc24` `func_800AA898`: the candidate moved one store below another and measured 69.18% against the park's 74.36%), but the worker derived the real shape from the round-169 hint and re-parked at **97.44%**: retail's insn-1 `addiu v1,zero,-1` is the constant load of an **early redundant `s16` -1 store** in the original source - repeating that `0x98 = -1` store at the top of the body puts every one of retail's 39 instructions in order, and the only residual is gcc 2.6 being unable to delete the redundant store (160 bytes vs retail's 156). That is the second park this session whose note was a symptom, and both were closed by asking what the ORIGINAL SOURCE did with the constant, not what the allocator did. Naming keeps paying on fresh evidence (18 globals: the battle anim-task state block `g_BattleAnimPositionGroupA/B`, `g_BattlePendingAnimState`, `g_BattleAnimTaskAllocCount`, `g_BattleActiveAnimTaskCount`, `g_BattleAnimGroupStates` from BattleAnimTaskTick's `(field_0x40 >> 13) & 0xF` test and temp1's read-back; the menu row-quad coordinate tables `g_MenuRowQuadMode{1,2,3,6}{Xs,Ys}`; `g_MoviePreservePrevDrawBuffer`, `g_MovieDrawBufferDoubleBuffered`, `g_GameStagedFileSyncCount`), and fields took the mirrored-layout rule to the effect-word grid (`GameGear.unk7C/80/84` -> `activeEffectFlags1/2/3`, merged from u8 pairs to the u16 the grid implies) and to `GameState.mapPosX/mapPosZ/mapPosZFraction` (three units agree: field writes them, world_map/misc1 divides by 315/341 as tile coords, misc9 forms `x = mapPosX*0x1000 + frac`). Integration: 8 clones, one `git merge` auto-resolution (world_map/main/misc9.c), no CONFLICT. Two worker reports are worth keeping: `src/battle/main/misc12.c`'s `BattleSprite` record is byte-identical to misc14's Misc14DisplayWork and misc13's Misc13Buffer (pOt[2]/pPrim[2]/count/flags/depthShift), and `Misc10Block`'s 0x00/0x04/0x08 are the part/model/anim list pointers misc14's relocator rebases. DECISION: nothing operator-only taken. |
| 184 | 5 field workers + 2 queued, 3 read-only naming workers, 1 permuter port worker | +0 | 3563 | Stubs flat at **1010**, `unk` **1571 -> 1525 (-46, the biggest field drop since r178)**, **`D_` 1422 -> 1404 (-19)**, live raw flat 235, eleven green. The park round produced another mechanism instead of a match: `battle/misc16` `func_8008CFB8` **98.49 -> 99.13%** - the permuter edit was worth +0.05 (a `do { } while (0)` wrapper, now in the note), and the real gain came from the round-167 wall being right for the wrong reason: the `(0x25 + i) * 0x28` row offset must be written **inline in the damage address**, because ANY `rowOffset` local makes gcc emit `li s5,0x5c8` as a movables initial value in the preheader; residuals left named (retail's running offset starts at 0x5C8 off `D_800CEF34` where ours strength-reduces from 0, and the `cursor`/codes pointer pair is s3/s2 swapped). Fields paid on the cross-file rule again, and one report is worth keeping: `battling/misc2`'s 12 renames came from misc3/misc4/misc17/misc18 already having the same offsets (`actionId`, `gaugePercent`, `partIndex`, `animSpeedBonus`, `gaugeValue`, `savedModelIndex`, `speedScale`), and `battling/misc14` - whose MATCHED functions are what misc10/misc12/misc13 were named FROM this session - named its own `Misc14Reloc.pPartList/pModelData` from the retail asm of func_8008B38C, closing the loop. `system/sound.h` got the WDS tone record (`toneTune`, `adsrModes` as three 3-bit modes) plus `AudioElement.startTick` and `AudioManager.playTicks`/`restoreTicks` from matched bodies. Naming: 19 globals landed, the best block being the SYSTEM FONT header (`g_SystemFontLeadByte`, `g_SystemFontDoubleByteGlyphOffset`, `g_SystemFontNarrowGlyphCount`, `g_SystemFontSingleByteCodeBase`, `g_SystemFontGlyphData` - all from SystemGetGlyphWidth's two-byte-code path and SystemInitializeFont's stores) and the battle slot/command-cell words (`g_BattleCommandCellFlags`, `g_BattleTargetSlotEntry`, `g_BattleCurSlotState`); three function names for the field script VM were skipped because those functions are still stubs (the names are recorded in the round's outputs for whoever matches them). Integration: 8 clones, no CONFLICT, but `reports/progress.json` (left dirty by the publish before the round) made `clone.sh merge` print "UNCOMMITTED ... not merged" for five clones before merging them anyway - harmless here, and the fix is to revert that file after `make progress-publish` and before `clone.sh new`. DECISION: nothing operator-only taken. |
| 185 | 7 park-repair workers (one unit each, from the sweep's ERROR rows) + 3 read-only naming workers | +3 | 3566 | Stubs **1010 -> 1007**, `D_` 1424 -> 1390 in two steps (**-14** this round), `unk` flat 1525, live raw flat 235, eleven green. **`battle/main/misc8` is now stub-free**: its three ERROR parks were fiction the same way round 163's were - the repair had been half done, so the bodies still compared two `BattleSlotRecord` structs with `!=` (invalid C, cc1 exits) AND double-scaled the record index (`g_BattleSlotRecords[(arg1 + 3) * 0x1C]` on an array of 0x1C-byte records = `(arg1+3)*0x1C*0x1C` bytes). Written as `g_BattleSlotRecords[row + 3].tag != g_BattleSlotRecords[idx].tag` they compile at 90.7, and the rest fell to the typed access `g_BattleSlotRows[row].unk10[...]` reproducing retail's preheader copy: `func_8007C1A4` (408 bytes), `func_8007C840` (404) and `func_8007D0CC` (124) all 99.68-99.80 RELOC, `check_bin battle` OK. **The round's most important negative: the sweep's ERROR rows are mostly STALE, not a repair queue.** Every other worker found its bodies compiling fine and measured them honestly (world_map/misc21's five parks: 87.12/97.90/86.05/72.45/69.38; `battling/misc18` `func_8008FACC` 89.78 with six spellings and a permuter run proved dead; `battle/misc21` `func_8009F1C4` 87.12 is TEN INSTRUCTIONS short of the 992 bytes its round-118 note described, i.e. a missing statement block, not the allocator tie the note blamed). **One real tooling bug found:** `park.py unpark` cannot handle three park shapes present in `world_map/misc6` - the signature on the `/*` line (`/* int func_x(int index) {`), and a note block with no closing `*/` of its own so note+body is one comment - and that is what created phantom ERROR rows; unpark those by hand. Two parks also improved measurably: `battle/misc32` `func_800BD1FC` **92.59 -> 96.55** (three permuter edits: a `u8` colour-mode local loaded before the store that keeps the parameter live, a `u16` timer local, the row re-derivation split over two statements; residual is two swapped independent loads) and `field/misc2` `func_80072D74` **98.25 -> 98.99** (an `s16 *pShake = (s16 *)g_Scene.unk98` local used only by the guard). `field/misc2` `func_800739C0` is a STACK_SWITCH body (emulated with `asm volatile`, which game code may not use) and is not a C job at all. Naming: +14 globals (the battle effect block `g_BattleEffectSlots`/`Actions`/`Color`/`SpawnTick`, world_map `g_WorldMapModeMatrix`/`DrawMode`/`ExitFlag`, menu `g_MenuPanelRowXOffsets`/`g_MenuItemRowStringIds`). DECISION: nothing operator-only taken. |
| 186 | 5 field workers + 2 queued (both queued died on an upstream provider error, no work lost), 3 read-only naming workers, 2 deep port workers | +0 | 3566 | Stubs flat at **1007**, `unk` **1525 -> 1517**, **`D_` 1404 -> 1379 (-4 this round: +15 names)**, five function names, live raw flat 235, eleven green. The two port workers were selected because each park was ONE residual away, and both came back with a proof that the residual is not C: `battle/misc32` `func_800BD1FC` (96.55, retail's 232 bytes) has the correct instruction multiset with exactly one permuted independent load pair and is insensitive to statement order, comma-folding, pointer aliasing and store order (store order is load-bearing the other way: mode-store-first drops to 93.28, timer-store-first 92.76), and round 185's two permuter runs had already converged on it; `field/misc2` `func_80072D74` (98.99, retail's 1212 bytes) resisted pointer local, volatile store, `((s32*)&sym)[0]`, `*(long*)&sym` and `*(s32*)&sym` - all byte-identical - and the DIAGNOSTIC that settles it is worth keeping: **retargeting just that one store at a different symbol shows gcc 2.7.2's cse shares any field address used twice (load+store), so retail's separate `lui $at / sw` is a reload tie, not a spelling**. Field evidence was mostly *negative* this round and that is the honest result: `battling/misc3` (31 unk, third pass) could name only `BattleActionData.percent` because every other reader is a stub or another TU's copy; `menu.h` named 3 of 136; `libgear/main.h` took the Temp8 interpolator quartet and `Model.sourceModelSlot` (the 0xFF/0xFE/0xFD bitmask-code model reference). One genuinely useful cross-struct find: `game.h`'s `GameCharacter.characterId` is proven by `include/battle/actor.h`'s declaration that its first 0xA4 bytes ARE the GameCharacter record, so battle's spelling now back-fills the main header, and the 0x79-0x8E effect grid got its missing `activeEffectFlags1` word at 0x7C. Integration: 9 clones, no CONFLICT, all eleven green after applying 4 function names + 11 globals. DECISION: nothing operator-only taken. |
| 187 | 5 named-constants workers (one .c each) + 3 read-only naming workers | +0 | 3566 | Stubs flat at **1007**, `unk` flat 1517, `D_` 1379 -> **1370**, live raw 235, eleven green. First round on the **named-constants** queue (4,439 numeric comparison sites, a PROMPT section untouched until now) and it is a good recurring round: a worker that must justify a number writes the reason into the report, so the product is C like `if (count > SOUND_EFFECT_CHANNEL_COUNT_MAX)` plus a map of what is NOT nameable. 46 constants landed across 5 files plus reuse of constants that already existed (`ACTOR_ID_INVALID`, `GEAR_ID_NONE`, `MAX_PARTY_MEMBERS`, `SCREEN_HEIGHT`) - one of those reuses was a CORRECTION: two field-party handlers had been spelling the gear-id sentinel `CHARACTER_ID_NONE` because both are 0xFF. Best-evidenced new names: `WORLD_MAP_SLOT_FACING_COS/SIN` (0xDA6/0x85A, a 1.12 cosine/sine pair whose squares sum to 4096.2, used to build the slot's rotation matrix and recovered with `ratan2` in the state switch) and `SOUND_SPU_MEMORY_SIZE 0x80000` (the SPU address space, tested as `0x80000 - address` and allocated at `0x80000 - size`). **The honesty rule held:** workers reported the literals they could not justify (158/59 distinct in world_map/misc6, 262 of 274 in battle/misc19 - masks, fixed-point, VRAM args, command ids) rather than inventing names, which is the outcome that makes this queue safe to repeat. Hoisting (my job) put the seven battle constants in `include/battle/actor.h` - the header that declares `slotStates`/`slotAmounts`/`hp`/`maxHp`, so the arrays now use `BATTLE_SLOT_COUNT` as their dimension and the `hp` comment says `BATTLE_ACTOR_HP_SCALE`; `WORLD_MAP_SCREEN_*` stay file-local because there is no world_map header to own them (creating one for two constants is more files than it is worth), and `0x7F` to `SoundPlayAudioManager` stays a literal because `SPU_VOL_MAX` lives in a .c one binary away. Integration: 5 clones, no CONFLICT, one `make check` after the hoist, eleven green. DECISION: nothing operator-only taken. |
| 188 | 5 rewrite workers + 2 queued, 3 read-only naming workers | +0 | 3566 | Stubs flat at **1007**, **live raw 235 -> 202 (-33, 64 files -> 61)**, `D_` 1370 -> **1357** (+13 names), `unk` flat 1517, eleven green. The rewrite shape is still the most reliable non-matching round: six of seven files came out CLEAN at 100.00% because the struct was already known from a field round, and the report that follows a rewrite is the best evidence in the repo (`battle/misc33` worked out that its 0x68 word **is the packed prim colour** - `func_800BD810` stores its second argument straight into the prim's word at +4 - so the file now has a `color` word/channels union, and twelve per-channel stores were retargeted through it; `battle/misc31` declared `AnimTask`/`BattleSpriteColorTask`/`BattleArchiveLoadTask` and six functions went 100.00%; `battle/misc17` retyped `BattleAccumulateSlotValueByRowFlag` onto BattleWorkBlock and found that its loop bound is `sizeof(pWork->slots)/sizeof(pWork->slots[0])` = the THREE party slots, **not** `BATTLE_SLOT_COUNT` (12) - exactly the trap the new header constant invites). Four ROUGH verdicts are measured, not guessed, and all say the same thing in a new place: **a typed field store makes cc1 hoist the following global load above it**, so `BattleEffectSetMatrix`'s six position stores cost 36 points (63.89%) typed against the byte view's 100, and misc16_r2's package-count store loses retail's load-delay nop (172 vs 180 bytes) - the third instance this session of "the cast is load-bearing". Left `world_map/misc5`'s 40 lines alone (four rounds have proved that block un-typeable) and `system/system.c`'s 6 (round 174 measured 13 spellings, best 73.18). Naming: +13 globals and `GfxQueueSpriteTileData`/`BattlingNoOp`, both found by a worker noticing an unnamed C definition the old greps missed. Integration: 7 clones, no CONFLICT, eleven green after applying 1 function + 12 globals. DECISION: nothing operator-only taken. |
| 189 | 3 deep fresh-stub workers + 3 rewrite workers, 3 read-only naming workers | +1 | 3567 | Stubs **1007 -> 1006**, **live raw 202 -> 191 (61 -> 59 files)**, `D_` flat 1357, eleven green, and the match came from the **preset** axis rather than from C: `menu/main/misc11`'s single stub `func_801E433C` -> **MenuApplyGearAccessories** is 100.00 under gcc 2.6.0 and 95.20 under Default, because retail's case 6-8 tests are three UNMERGED `andi/beqz` pairs where 2.7.2 folds them into one `(x & 0x1800) == 0x1800` test - a compiler difference, not a source one. The unit's only matched function stays 100.0 under 2.6.0, so it is a plain whole-unit switch (`menu/main/misc11.` added to `XenoMenu260`), `check_bin menu` OK, and **that unit is now finished**. That is the third compiler-axis match this session and it cost one screening compile: **the fresh-stub pool should be screened for preset first, because a from-scratch body that only fails by a merged branch test is not a C problem.** The other two fresh-stub workers confirm the pool is dead as a C queue: `field/dialogue/text_box` `func_8009C154` reached **81.50% with the whole data layout recovered** (6-byte-stride portrait slots, 0x10-byte TIM rect entries `{x,y,clutX,clutY,secondX,secondY,secondClutX,secondClutY}`, an 8-byte `{size, pad, pBuffer}` request array) and fails only on CSE of the rect-address adds; `world_map/misc5` `func_8007C3B8` reached **98.13% at retail's exact 876 bytes** and is **JTBL_LAYOUT blocked** - its tables sit at 4 mod 8 in the unit's rodata (`jtbl_8006FC50` at +0x8C) and the last three of its seven tables share that parity, so the "convert the whole chain" cure cannot apply. Both parks now carry structure instead of a bare percentage, which is what makes the next attempt cheaper. Rewrite: `world_map/misc15` 5 -> 0 and `battle/misc5` 5 -> 0 (the whole cursor family now takes `BattleCommand **pCommand` and reads `p->operandN`, which is the `include/battle/actor.h` view battle/main/misc7 already uses); `battle/misc19` closed 1 of 6 and measured the rest: `BattleSetSlotEntryFieldsTo1` needs retail's **index-first `addu`** (99.71% at exact size, base-first is what array syntax gives) and `BattleGetActorActionConditionMask` needs **three base pointers off one `lui/addiu`** (87.56% typed). DECISION: nothing operator-only taken - the preset switch keeps every matched function at 100.0, which is the documented procedure, not the operator-only case. |
| 190 | 4 named-constants workers + 3 rewrite workers | +1 | 3568 | Stubs **1006 -> 1005**, **live raw 191 -> 188 (59 -> 58 files)**, `D_` flat 1353, `unk` flat 1525, eleven green. The match came out of the REWRITE lane: `slus_006.64/system/sound` `func_8003DEE4` -> **SoundScriptStartPanRamp** was a 76.59% park and closed on a permuter find - an *unconditional* `panDistance = target - current;` in the outer block plus a second `delta = panDistance;` inside the `if (frames)`; the extra copy is what makes gcc emit retail's entry `move a1,a0` and the `andi a0`/`mflo a0` divisor chain. The same worker then judged its five assigned raw lines **irreducible and said why**: every one is a cast on a runtime offset into a variable-length blob (`(u8*)pWdsFile + pWdsFile->adpcmDataOffset`, `(u8*)pManager + SoundCalculateAudioManagerSize(...)`) where the honest fix is a flexible-array member retail's struct does not evidence - a ROUGH verdict of exactly the kind that should stop a rewrite, not a failure. Constants: **40 landed** (the five `MENU_*_ID_COUNT` table sizes and `MENU_DIGIT_NONE`, `WORLD_MAP_GRID_ANGLE_STEP`/`PROJECTION_DISTANCE`/`WORK_BUFFER_SIZE`/`QUAD_COUNT`/`SLOT_COUNT`/the two archive indices, the field zoom-fade and particle-grid and text-segment-window constants, `BATTLE_SLOT_NONE` across 11 sites and `BATTLE_SOUND_MENU_DENIED` across 43). Hoisting, and the trap in it: `MENU_*_ID_COUNT` went to `include/main/game.h` and `MENU_DIGIT_NONE` to `include/system/menu.h`, but `BATTLE_SOUND_MENU_DENIED` had to go BACK into `src/battle/main/misc12.c` because that unit includes only `common.h` - **a CONST_SHARED name is only hoistable to a header the declaring unit actually includes**, so `make check` after the hoist is not optional. Rewrite: `battle/misc30` reports **0 live raw lines for all 21 functions**, `battling/misc8` closed 1 of 2 and measured why the rest cannot go (any `&obj->sprt[i]` makes cc1 fold the +4 into the symbol address and emit an extra addiu where retail has `addu v1,a1,a2` on the bare symbol). Also recorded from this round: `battle/misc12` left ~150 literals as unjustifiable (55 `menuState` assignments where each handler has its own numbering, 95 `g_BattleMenuEvent` case labels), which is the right call. DECISION: nothing operator-only taken. |
| 191 | 6 field workers (3 headers + 3 .c-declared files) | +0 | 3568 | Stubs flat at **1005**, `unk` **1525 -> 1495 (-30)**, `D_`/raw flat (1353 / 188), eleven green, 6 clones merged with no CONFLICT. The evidence class that carried the round is the one PROMPT calls strongest - **the same layout in a second compilation**: `include/battle/actor.h` named `BattleActor.unk36 skipGroundSnap` and `unk37 useScaledMatrices` from `include/libgear/main.h`'s `Model` at the SAME offsets, where `LibGearApplyGroundHeight` makes the identical test and `LibGearStepModelAnimation` picks `LibGearComputeScaledJointMatrices` over the plain path exactly as this unit's `BattleStepActorAnimation` picks `func_8009F1C4`; the same worker extended the effect grid to `BattleSlotState` (`activeEffectFlags1/2/3` at 0x7C/0x80/0x84, `accessoryEffectFlags2` at 0x82) by showing `misc19`/`misc20`/`misc18` read the gear-side word where the actor side is already named. The .c-declared files show what the remaining `unk` really is: `battling/misc16` classified 13 of 17 fields as FIELD_UNKNOWN with a per-field reason (write-only, `sh $zero` in an init, or unreachable), and `battling/misc15` found `VertexIndexSet.pad6` is **not padding** - `func_8008C620` reads it as the fourth vertex index after `addiu $a0,$a0,0x8`, i.e. the fourth SXY word a POLY_F4 needs - which is the third time this session a `pad<offset>` turned out to be a live field. `battling/misc11` typed its quad packet end to end (`tagLength` 9 = 0x24 non-tag bytes in words, `color` 0x2C101010 with the STP bit OR'ed to 0x2E, `uv0..uv3` from `g_BattleQuadTexU/V` with the +0x3F far-corner walk, `clut`/`tpage` from the named globals). Four passes on `system/menu.h` now leave it at 97 with most of the tail write-only, so the header queue is thinning the same way the function-name queue did. DECISION: nothing operator-only taken. |
| 192 | 4 named-constants workers + 2 rewrite workers | +0 | 3568 | Stubs flat at **1005**, **live raw 188 -> 176 (58 -> 56 files)**, `D_`/`unk` flat (1353 / 1495), eleven green, 6 clones no CONFLICT. Constants: ~40, and the lib_gear batch is the best-evidenced of the session because `main.c` IS the code the record names come from - the whole `LIBGEAR_*` family (`JOINT_INDEX_NONE`/`PARENT_INDEX_NONE` 0xFFFF as the two sentinels the skeleton loop branches on, the three `JOINT_RELEASE_*` free bits, `JOINT_FLAG_ADD`/`RECURSE`, `MODEL_FLAG_OWN_UPDATE`/`USE_WIDTH`, `ANIM_ID_QUEUED_BASE` 0xFE with `ANIM_ID_HIGH_BASE 0x40` and `HIGH_INDEX_OFFSET 0x3F` for the two entry tables, `INTERPOLATE_RISE`/`FALL`). Two corrections came out of the round, both recorded: **(1) round 190's "typed sprite stores lose the match" verdict was WRONG** - a *field* store folds the array's +4 into the store displacement while *taking the address* folds it into the scaled index, so `pText->sprt[i].w = …` with `AddPrim(p, &pText->sprt[i])` reproduces retail instruction-for-instruction and only the pointer-local spelling loses it; both rewritten files are now at **0 live raw lines**, and that unblocks the same sites misc30/misc32/misc33/misc34 still spell raw. **(2) The hoist trap from round 190 recurred and was caught before the build this time:** `WORLD_MAP_ARCHIVE_DIRECTORY_INDEX` stays file-local because `world_map/misc6.c` does not include `system/archive.h`, while `BATTLE_PACKET_PRIM_POOL_SIZE` went to `include/battle/battle_work.h` where both declaring units already include it. `battle/misc28` also handed forward two hoist candidates with a named user list: `BattleSpriteTask` (what `SpriteData.field_0x6C` points at) and `BattleSpriteSlotTable` (the 0x8C8C per-slot sprite pointer table of `g_BattleStateWork`). DECISION: nothing operator-only taken. |
