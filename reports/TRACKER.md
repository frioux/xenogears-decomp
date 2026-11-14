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
