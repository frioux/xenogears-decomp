# AGENTS.md — working on the Xenogears decompilation

This is the runbook for anyone, human or agent, making progress on this repo. It
assumes you will use LLM agents heavily and tells you how to do that well. Read it
once, then follow the loop in **Contribution loop**.

The goal is a **matching** decompilation of Xenogears (US, `SLUS_006.64`) and its
overlays: C that the original compilers turn into byte-identical output. Progress
is measured by `objdiff` and published at <https://frioux.github.io/xenogears-decomp/>.

## 1. What is in the repo

| Path | Purpose |
|---|---|
| `config/*.yaml` | splat configs, one per binary: where each C unit sits in the binary |
| `config/symbol_addrs.<bin>.txt` | symbol names and addresses, one file per binary |
| `src/<bin>/...` | C source. One directory per binary: `slus_006.64`, `field`, `member_change_menu`, `shop_menu`, `gear_shop_menu`, `lib_gear` |
| `include/` | headers. `common.h` is included first by every unit |
| `asm/` (generated) | splat output. `asm/<bin>/nonmatchings/<unit>/<Func>.s` is the target for each unmatched function |
| `expected/` (generated) | the original code assembled to `.o` files, for diffing |
| `build/` (generated) | compiler output, `build/out/*` final binaries, `build/progress.json` report |
| `gears.toml` | compiler and flag presets per source path. Read this before touching a unit |
| `tools/gears` | Rust metabuild tool that runs splat and writes `build.ninja` |
| `tools/gcc-*-psx`, `tools/maspsx` | the period compilers (Linux i386 binaries) and the assembler shim |
| `tools/scripts/` | `extract_exe.py`, `extract_overlays.py`, `verify_disc.py`, `select_target.py`, `diff_func.sh` |
| `tools/progress/` | progress site renderer and publisher |
| `disc/` (gitignored) | `SLUS_006.64` and overlay `.bin` files extracted from the disc. Never commit these |

Binaries and overlays currently built and checked: `slus_006.64`, `field.bin`,
`member_change_menu.bin`, `shop_menu.bin`, `gear_shop_menu.bin`, `lib_gear.bin`.
Extracted but not yet split: `battling.bin`, `world_map.bin`, `battle.bin`,
`menu.bin`, `movie.bin` (movie has a splat config but is not in `gears.toml`).

## 2. Environment

The compilers are Linux x86 binaries, so the build runs natively only on
linux/x86_64. Everywhere else `make` transparently runs the build inside the
`ethos:latest` Docker image (built from `./Dockerfile`).

**macOS (this machine):** Docker is provided by colima with Rosetta enabled.
Under plain qemu emulation the assembler segfaults intermittently; Rosetta is
required for a reliable build.

```sh
colima status || colima start --vz-rosetta   # once per boot
make check                                   # ~30 s: clean build + sha256 of all outputs
```

**Inputs.** Extract the game binaries from a disc 1 image (raw `MODE2/2352` `.bin`):

```sh
make extract DISC="/path/to/Xenogears (USA) (Disc 1).bin"   # writes disc/SLUS_006.64 + overlays, verifies sha1
python3 tools/scripts/verify_disc.py                          # re-check any time
```

A host Python venv (`.venv`, Python 3.13) with `requirements.txt` installed exists
for running splat and the scripts on the host; the container has its own.

**Make targets**

| Target | What it does |
|---|---|
| `make check` | clean, build in *matching* mode, verify every output's sha256. The gate before any commit |
| `make build` | same without the checksum |
| `make objdiff-config` | clean, build in *report* mode, assemble originals into `expected/`, write `objdiff.json`. Prerequisite for `diff_func.sh` |
| `make report` | `objdiff-config` + `build/progress.json` |
| `make progress` | `report` + render `build/progress-site/index.html` |
| `make progress-publish` | `report` + render with history + push to `gh-pages` |
| `tools/scripts/check_bin.sh <bin>` | rebuild in matching mode and byte-check one output (~35 s). Use it when objdiff refuses to reach 100% for a function whose only difference is a relocation the linker resolves identically (a section-relative `.rodata` jump table against a named `jtbl_`). Leaves the tree in matching mode. |
| `make shell` | bash inside the build container with the repo mounted |
| `make extract DISC=...` | pull binaries out of a disc image |

Two build modes exist and they overwrite each other's `build.ninja`:

- **matching** (`make build` / `make check`): `INCLUDE_ASM` pulls the original
  assembly in, output must be byte-identical.
- **report** (`make objdiff-config` / `make report`): `INCLUDE_ASM` expands to
  nothing, so each `.o` holds only the C we have written, and objdiff compares it
  against `expected/`. This is the mode for iterating on a function.

Every `make build|check|objdiff-config|report` starts with a clean, so `asm/`,
`expected/` and `build/` are regenerated each time (~30 s). Inside a mode, use
`ninja` directly for incremental rebuilds.

## 3. Contribution loop

### 3.1 Pick a target

```sh
python3 tools/scripts/select_target.py --binary field --max-insns 60 --named-only
python3 tools/scripts/select_target.py --file misc3            # everything left in one unit
python3 tools/scripts/select_target.py --json                  # for programmatic selection
```

Good targets, in order: functions that already have a semantic name, functions
whose data references are already named, small functions in a unit that is nearly
complete (finishing a unit is worth more than one function elsewhere), and
functions called by code you just matched. Poor targets: tiny functions whose
globals are all `D_8xxxxxxx`, and Psy-Q SDK (`src/slus_006.64/psyq`) functions,
where matching usually depends on finding the right library version rather than
on writing C.

The `C?` column marks stubs that already have a commented-out attempt below the
`INCLUDE_ASM`. Those are often one small change from matching.

### 3.2 Read the target

The assembly is `asm/<bin>/nonmatchings/<unit>/<Func>.s` after any build. Also
read: the surrounding C in the unit (types already recovered, helpers already
matched), `config/symbol_addrs.<bin>.txt` for every symbol the function touches,
and the headers under `include/` for the structs involved. Callers and callees
tell you parameter types; search for the function name across `src/`.

### 3.3 Write the C

Replace the `INCLUDE_ASM("...", Func);` line with a definition, in place. Functions
in a unit must stay in address order. Follow **Conventions** below. Resist pasting
a decompiler's output unchanged: write what the original programmer plausibly
wrote, then adjust for the compiler.

### 3.4 Diff

```sh
make objdiff-config                                    # once per session, or after make build/check
tools/scripts/diff_func.sh field/main/misc3 FieldSomething   # rebuilds that unit, side-by-side diff, exit 0 on MATCH
```

`<unit>` is the path under `src/` without `.c`. The script rebuilds only that
unit with ninja (a second or two) and prints `MATCH: ...` or `MISMATCH` with the
original on the left and your compile on the right. If `tools/objdiff/objdiff-cli`
exists (a native objdiff build, gitignored; download the release for your platform
from <https://github.com/encounter/objdiff/releases>, on macOS then
`xattr -c` and `codesign --force -s -` it) the function's fuzzy match percentage is
printed too, which is the number to watch when you are far from matching. Iterate
here; this is the inner loop. Common knobs when the structure is right but details
differ:

- register allocation and instruction order: change the order of local variable
  declarations, split or merge temporaries, turn a repeated field load into a
  local (or the reverse), use `++i` vs `i++`, swap operands of commutative ops.
- signed vs unsigned extensions (`sll/sra` pairs, `andi 0xff`): the type of a
  parameter or field is wrong. `char` is unsigned by default here (`-funsigned-char`).
- `addiu`/`lui` pairs vs `gp` relative loads: the unit's `-G` setting or whether a
  global is in `.sdata`/`.sbss`. Check `gears.toml` and how the symbol is declared.
- division or modulus expanded inline: `--expand-div` is a per-preset maspsx flag.
- a different compiler is in use for the unit. `gears.toml` presets select gcc
  2.6.0, 2.7.2 or 2.7.2-cdk per path prefix.

### 3.5 Verify the whole build

```sh
make check
```

All six outputs must print `OK`. This is the only definition of "matched". If a
function diffs clean but `make check` fails, the usual cause is data: a global you
defined moved or resized, a `.rodata` string or jump table moved, or an unnamed
symbol was still referenced from asm elsewhere.

### 3.6 Record symbols and types

- New names go in `config/symbol_addrs.<bin>.txt` (`Name = 0x8XXXXXXX; // type:T size:N`)
  and are declared in a header under `include/`. Rename `func_`/`D_` symbols as
  soon as you know what they do; naming is as valuable as matching.
- Field overlay symbols go in `symbol_addrs.field.txt`; main-exe symbols the
  overlay uses stay in `symbol_addrs.slus_006.64.txt` (the overlay configs include
  both).
- Struct layouts belong in headers with offset comments and a size comment.

### 3.7 Checkpoint

Commit when `make check` passes and the change is coherent: one function, one
cluster, or one unit's worth of matches plus the symbols and headers they needed.
Commit message: what was matched or renamed and anything learned about the unit
(compiler, flags, a struct). Do not commit generated directories (`asm/`, `build/`,
`expected/`, `linker/`, `objdiff.json`, `build.ninja`), anything in `disc/`, or
notes. After a batch, `make progress-publish` updates the public progress page.

## 4. Conventions

**Types** (from `include/types.h` and the Psy-Q headers):
`char`/`short`/`int` for game code where signedness does not matter;
`u_char`/`u_short`/`u_int` for unsigned; `s_char`/`s_short`/`s_int` only when a
signed type is needed to match; `u8`..`u32`, `s8`..`s32` for SDK code or when the
width is semantic; `mem_addr` for an address held in an integer;
`undefined8/16/32` for fields whose type is not yet known.

**Structs**: every field gets an offset comment, the struct a size comment.
Unknown fields are `unk<offset>`, padding `pad<offset>`.

```c
typedef struct {
    /* 0x0 */ u_short x;
    /* 0x2 */ undefined2 unk2;
    /* 0x4 */ u_short z;
    /* 0x6 */ u16 pad6;
} Vector; // Size: 0x8
```

**Names**: functions are PascalCase with an area prefix (`FieldLoadUITextures`,
`ShopMenuParseNumberToString`, `HeapGetBlockUser`). Globals are `g_Name`.
Pointer parameters and locals are prefixed `p` (`pHeapBlock`). Unrecovered
functions stay `func_<addr>`, unrecovered data `D_<addr>`.

**Files**: `#include "common.h"` first. Data definitions are grouped under a
section comment (`// .sbss`, `// .rodata`, `// .data`) in address order. Keep a
matched function free of analysis debris: no commented-out alternatives, no
"TODO match", no address banners. A commented-out attempt is acceptable only
directly below an `INCLUDE_ASM` it has not yet replaced.

**Macros**: `INCLUDE_ASM(FOLDER, NAME)` includes `FOLDER/NAME.s` through inline
asm (`include/include_asm.h`). `INCLUDE_RODATA` exists but is unused. Both expand
to nothing under `SKIP_ASM`, `M2CTX` and `PERMUTER`.


### Is the preset wrong? (`tools/scripts/try_flags.sh`)

Before suspecting a struct layout, check the compiler. `try_flags.sh` recompiles
one unit under another gcc / `-O` level / `-G` / `-mcpu` in about a second per
variant, parsing the real flags out of `build.ninja`, and scores the result with
objdiff:

```sh
tools/scripts/try_flags.sh slus_006.64/system/sound SoundSpuMemoryAllocateBlock \
    --gcc gcc-2.7.2-psx --opt -O3 --gp -G0
```

Pick a *discriminating* function — one that already matches 100% — and see whether
the candidate reproduces it. This is how the scene establishes a toolchain
(Crash 2's `docs/COMPILER.md` states the method; a version counts only when a
function matches under it and nothing else).

Already settled this way, do not redo:

| Candidate | Result |
|---|---|
| `-mcpu=r8000` (which fixed Crash 2's delay-slot scheduling) | **No.** `slus_006.64` and `field` fail `make check`; gcc 2.6.0's `cc1` rejects the flag outright. |
| gcc 2.8.1 (Psy-Q 4.4, used by Vagrant Story / Chrono Cross / FF8's `snd_*`) | **No.** Switching `Default` to `tools/gcc-2.8.1-psx` fails `make check`; `SoundReset` drops to 96.76%. |
| gcc 2.7.2-cdk + ASPSX 2.67 (Psy-Q 4.1) | In use: `XenoGraphics`. |
| gcc 2.6.0 (Psy-Q 3.3/3.5) | In use: `XenoLibrary`. |

To re-test 2.8.1 (or add 2.8.0 for Psy-Q 4.3), the prebuilt PS1 compilers are
release assets of <https://github.com/decompals/old-gcc> (tag `0.17`,
`gcc-2.8.1-psx.tar.gz`, sha256 `f6f6e883942d4d3289d048236c672e71ed410e546aaae8ff655952f1567e1be0`,
Linux i386 static). Untar into `tools/gcc-2.8.1-psx/` and either use `try_flags.sh
--gcc gcc-2.8.1-psx` or add a preset; pair it with `--aspsx-version=2.79`.

## 5. Compiler presets (`gears.toml`)

| Preset | gcc | Flags that differ | Applies to |
|---|---|---|---|
| Default | 2.7.2-psx | `-O2 -G8`, maspsx `--use-comm-section` | everything not listed below |
| PSY-Q | 2.7.2-psx | `-O3 -G0`, maspsx `--aspsx-version=2.56 --expand-div` | `slus_006.64/psyq/` |
| XenoLibrary | 2.6.0-psx | Default flags | `system/libarchive`, `system/sound`, `member_change_menu/main/misc`, `shop_menu/main/misc`, `lib_gear/main/main`, `gear_shop_menu/main/main`, `gear_shop_menu/main/render` |
| XenoGraphics | 2.7.2-cdk-psx | maspsx `--aspsx-version=2.67` | `system/animation_scripts`, `system/temp1`, `system/temp2` |
| XenoMain | 2.7.2-psx | `-G0` | `slus_006.64/main/main_loop` |
| XenoGraphicsLibrary | 2.6.0-psx | maspsx `--expand-div` | `slus_006.64/graphics/line_scroll` |

Common `cc1` flags: `-mips1 -mcpu=3000 -funsigned-char -fpeephole -ffunction-cse
-fpcc-struct-return -fcommon -fverbose-asm -msoft-float -mgas -fgnu-linker`.
If a whole unit refuses to match on register allocation or `gp` usage, suspect
the preset before suspecting your C, and add a preset entry rather than hacks.
The decomp.me preset "Xenogears" (<https://decomp.me/preset/171>) mirrors Default
and is handy for sharing a scratch, but the local `diff_func.sh` loop is faster.


### Unit rodata: the most common way a 100% match breaks the link

`objdiff` compares objects; the linker is the real gate. The trap that has cost
this repo the most time: **a unit's `.rodata` often lives in the asm of the very
function you are converting.** When you turn that function into C, those bytes
disappear, the overlay shifts, and `make check` fails while objdiff says 100%.

Check before converting:

```sh
grep dlabel asm/<bin>/matchings/<unit>/<Func>.s   # or nonmatchings/
```

If the stub's `.s` defines data labels, the data has to move to C in the same
commit. Rules that make that work:

- **`const` is mandatory.** `u8 D_x[0x10] = {...}` lands in `.data`; `const u8
  D_x[0x10] = {...}` lands in `.rodata`. Getting this wrong is not subtle-looking
  but it is exactly the difference between linking and not: with a non-const
  array, a unit's switch jump tables become its only `.rodata`, they move to the
  start of the unit, and everything shifts (`shop_menu`, 0x10 bytes).
- **cc1 emits named `.rodata` objects before the jump tables it generates for
  `switch`es**, in declaration order. So if the original layout is
  `[named data][jump tables]`, plain C in file order reproduces it.
- **C `.rodata` lands before asm `.rodata`** in the same object, because the asm
  is `.include`d at each function's wrapper in `.text` and its `.section .rodata`
  switches section at that point. So if the original layout is
  `[asm-owned data][C data]`, you cannot reproduce it by defining only the C
  part: every asm owner in the unit has to move to C in the same change (this is
  why `field/main/misc2`'s `VECTOR D_8006FB70` cannot be converted until the five
  stubs that own the strings before it are converted too).
- Do not reach for section attributes, `#pragma` or per-object hacks. If the
  ordering cannot be expressed, leave the function as `INCLUDE_ASM` and say so in
  a comment; a wrong layout costs a whole overlay.

## 6. Hard rules

- Matching means `make check` passes. Never edit `config/checksum.sha`,
  `expected/`, or the extracted binaries to make it pass.
- No inline assembly in game code, no `#pragma`s, no per-function compiler flags.
  A function either matches as C or stays `INCLUDE_ASM`.
- Do not hard-code addresses in C; name the symbol in `symbol_addrs` instead.
- Do not edit `tools/gcc-*`, `tools/maspsx` (submodule) or `tools/gears/prebuilt`.
- Do not commit anything from `disc/`.
- Keep `symbol_addrs` and headers in sync; a name that exists in only one place
  breaks the next person's build.
- Prefer defining a global in the unit that owns it (per the splat yaml section
  layout) over `extern` crutches.

## 7. Working with agents

Context is the scarce resource, so split work by **unit**, not by function:
two agents editing the same `.c` file will conflict. A workable pattern for an
orchestrator:

1. Run `make objdiff-config` once, then `select_target.py --json` and group the
   candidates by `file`.
2. Give each worker one unit and the exact list of stubs to attempt, with the
   relevant symbol file and headers named. Workers edit only that unit, its header
   and `symbol_addrs`, and verify each function with `diff_func.sh`. They do not
   run `make build`/`check`/`objdiff-config` (those wipe shared state).
3. The orchestrator integrates, runs `make check`, resolves any data layout
   fallout, commits, and publishes progress.

Give workers the facts, not the hypothesis: the `.s` file, the current C, the
preset for that unit, and the conventions above. Ask them to report exactly which
functions matched, which did not and why, and every symbol they named.

Cheap models do well on small, named functions in nearly finished units and on
symbol recovery. Reserve expensive models for units that are stuck across many
functions, which usually means a wrong struct layout or wrong preset.

**Models.** Workers run on `lunaroute/deepseek-4.1-flash-background` (the
`-background` lunaroute model). It is the pi-subagents `subagents.defaultModel`,
so an unqualified launch already picks it; there is no reason to name a
per-provider model such as `anthropic/claude-sonnet-4-*` — that routes through
OpenRouter, which is out of credits and fails every worker with a 402 before it
writes anything. Pass `model` explicitly only to override this.

## 8. Where the work is

Remaining `INCLUDE_ASM` stubs by area at the time of writing (November 2026):

| Area | Stubs | Notes |
|---|---|---|
| `src/field/main` | 483 | the bulk. `misc*.c` are unnamed groupings of field logic |
| `src/slus_006.64/system` | 417 | engine: sound (large), menu, rendering, model, archive |
| `src/slus_006.64/psyq` | 174 | SDK; match by identifying the library version |
| `src/lib_gear/main` | 62 | gcc 2.6.0 unit |
| `src/field/*` other | 38 | dialogue, camera, scripts, effects, party |
| `src/shop_menu/main` | 22 | |
| `src/member_change_menu/main` | 4 | nearly done |
| `src/gear_shop_menu/main` | 3 | nearly done |
| `src/slus_006.64/{main,graphics}` | 3 | |

Beyond matching: split the remaining overlays (`battling`, `world_map`, `battle`,
`menu`, `movie`) following `config/overlay-template.yaml`, and recover names for
`func_`/`D_` symbols in matched units.

## 9. Adding an overlay

1. Add it to `tools/scripts/overlays.yaml` if missing, run `make extract`.
2. Copy `config/overlay-template.yaml` to `config/<name>.yaml`, fill in the
   placeholders (sha1 from `shasum disc/<name>.bin`, load address, size).
3. Add `config/symbol_addrs.<name>.txt`, `src/<name>/`, the overlay to
   `overlays` in `gears.toml`, a category in `tools/objdiff/config.yaml`, and its
   expected sha256 to `config/checksum.sha` once it links byte-identical.
