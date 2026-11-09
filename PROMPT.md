# Xenogears matching decompilation — long autonomous run

Read `reports/TRACKER.md` first (especially "Session 2 result" and "Where the next
session should start"), then `AGENTS.md`, then `CLAUDE.md`. Then run:

    colima status || colima start --vz-rosetta
    make check                      # all six outputs must print OK before you touch anything
    python3 tools/scripts/select_target.py --json --limit 5000 | python3 -c "import json,sys,collections; d=json.load(sys.stdin); print(len(d)); print(collections.Counter(x['file'] for x in d).most_common(15))"

## The job

Take the function match count from its current number to as high as it will go,
in as many parallel worker rounds as you can sustain. Today it is 1967/2528
(77.81%) with 54.67% of code matched; 561 `INCLUDE_ASM` stubs remain. Work for
hours, not minutes: plan on ~15 rounds of 10-14 workers.

**After every round, and only when `make check` prints OK for all six outputs:
commit, push, and add a row to `reports/TRACKER.md`.** Run
`make progress-publish` every few rounds so the public page tracks. Never commit
`asm/`, `build/`, `expected/`, `linker/`, `objdiff.json`, `build.ninja`, `disc/`,
or notes.

## The round loop (use the checked-in harness, do not rebuild it)

One worker = one clone = **one `.c` file**. Two agents in one file will conflict;
two agents in different files will not.

    make objdiff-config                                  # clones need report mode
    tools/rounds/clone.sh new  a b c ...                 # cp -ac clones under ~/xw/r-<tag>
    # launch: subagent({workflow: "tools/rounds/brief.js", async: true,
    #                   args: {workers: [{tree, file, unit, asm, preset, symbols, funcs, extra}, ...]}})
    # wait for the round (~30 min), then:
    tools/rounds/clone.sh merge a b c ...
    python3 tools/scripts/lint_sources.py -q
    make check                                           # if it fails, tools/scripts/culprit.sh <bin>
    tools/rounds/clone.sh reset a b c ...                # reuse the clones next round

`tools/rounds/brief.js` already contains the worker playbook and the two
mandatory gates. Read it, extend it when a round teaches something new, and give
every worker 6-8 *named* functions plus the exact preset and the relevant
`symbol_addrs` file — the facts, never your hypothesis.

Pick targets from `select_target.py` by unit, not by function: prefer small named
functions in nearly-finished units, and prefer a unit that is 1-3 stubs from
complete over one function somewhere else. `sound`, `field/main/*`, `lib_gear`,
`system/temp{1,2,3}` and `psyq/libgte` are the live pools.

## Do not redo these (all settled, with evidence in the tracker)

- `-mcpu=r8000` and gcc 2.8.1 are **not** this project's compiler; both fail
  `make check`. The `gears.toml` presets are correct, including `system/sound` at
  gcc 2.6.0 (45 of its functions are 2.6.0-only, zero are 2.7.2-only).
- A stub whose `.s` has instructions after `.size`, or owns `dlabel` data, cannot
  be converted — it shifts the whole overlay. `tools/scripts/convertible.py <bin/unit>`
  lists the ones that can.
- `objdiff 100%` is not proof: run `tools/scripts/check_bin.sh <overlay>` and
  require `OK`. Jump tables and section-relative data are exactly where the two
  disagree.
- `slus_006.64/system/model` (49 stubs) and the psyq `mtc2`-only bodies are
  hand-written assembly. Leave them.

## Two things that need a decision, not more workers

1. **`field/main/misc2`'s scratchpad stack switch.** Two stubs switch `$sp` into
   `0x1F8003FC` with four instructions (`sw $sp,0($t1); addiu $t1,-4;
   addu $sp,$t1,0`), which no C construct emits. They gate six functions plus the
   unit's rodata. The hard rules forbid inline asm; ask me before using it, and if
   I say no, leave it alone and document it.
2. **The unsplit overlays** (`battling`, `world_map`, `battle`, `menu`, `movie`).
   `AGENTS.md` section 9 is the recipe and `config/overlay-template.yaml` is the
   skeleton. This is the largest remaining source of new work and it is mostly
   mechanical — it is a good thing to do with two workers while other rounds run.

## Ground rules

- `make check` green is the only definition of done. Never edit
  `config/checksum.sha`, `expected/`, or the extracted binaries.
- No inline asm in game code, no `#pragma`, no per-function compiler flags. A
  function either matches as C or stays `INCLUDE_ASM`.
- Ask me before: adding inline asm, touching `tools/gcc-*` / `tools/maspsx` /
  `tools/gears/prebuilt`, changing a preset that units already depend on, or
  anything that deletes committed work.
- If two consecutive rounds land almost nothing in a unit, stop spending workers
  there and say why in the tracker — the residual is usually a compiler codegen
  puzzle that needs an idea, not another attempt.
- Report to me only: functions landed, the tracker row, and any decision you need.
  Put the detail in `reports/TRACKER.md` where the next session reads it.
