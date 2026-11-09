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
