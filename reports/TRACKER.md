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
