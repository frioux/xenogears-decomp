# Decompilation tracker

Live log of an autonomous 12-hour work session. One row per landed batch.
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
