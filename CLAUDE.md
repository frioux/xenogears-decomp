# CLAUDE.md

Read `AGENTS.md` first; it is the full runbook (environment, build modes, the
match loop, conventions, presets, hard rules). This file only adds what is
specific to Claude Code sessions on this machine.

## Session start

```sh
colima status || colima start --vz-rosetta     # Docker for the Linux-only compilers
make objdiff-config                            # ~30 s, puts the tree in report mode for diffing
python3 tools/scripts/select_target.py --named-only --max-insns 80
```

If `disc/SLUS_006.64` is missing, the disc archives are on the Desktop
(`Xenogears (USA) (Disc 1).7z`); extract with `7zz` into `disc/disc1/` and run
`make extract`.

## Inner loop

1. Replace one `INCLUDE_ASM` with C.
2. `tools/scripts/diff_func.sh <unit> <Func>` until it prints `MATCH`.
3. Add names to `config/symbol_addrs.<bin>.txt` and declarations to `include/`.
4. After a batch: `make check` (all eleven `OK`), then commit. `make check` leaves
   the tree in matching mode, so run `make objdiff-config` again before diffing.

## Orchestration

Fan work out by unit (one `.c` file per worker) on
`lunaroute/deepseek-4.1-flash-background`, which is the pi-subagents
`subagents.defaultModel` (never name an `anthropic/*` or other non-lunaroute model:
those route through OpenRouter and fail with a 402). Workers only run `ninja` and
`diff_func.sh`, never `make build|check|objdiff-config`. The
orchestrator integrates, runs `make check`, commits, and runs
`make progress-publish`. Do not use git worktrees in this repo; the build writes
to fixed paths (`build/`, `asm/`, `expected/`) and `gears` locates the project by
the directory name `xenogears-decomp`.

## Known sharp edges

- The bundled Linux `tools/objdiff/objdiff` (3.0.0-beta.5) generates reports
  fine but its `diff -o -` JSON output is empty. `tools/objdiff/objdiff-cli` is a
  native macOS build of objdiff 3.8.3 (gitignored, ad-hoc signed by hand);
  `diff_func.sh` uses it for the match percentage and objdump for the text diff.
  Direct use: `tools/objdiff/objdiff-cli diff -1 expected/asm/<unit>.s.o -2 build/src/<unit>.c.o -o - --format json <Func>`.
- Under qemu (colima without Rosetta) `mips-linux-gnu-as` segfaults randomly
  with `-j6`. Rosetta fixes it.
- `ninja` does not rebuild `build/src/<unit>.c.{i,s,o}` when a *header* changes, so
  delete those three files (or run `make objdiff-config`) after editing a header or
  you will diff a stale object.
- Object layout follows the C file: `INCLUDE_ASM` lines must stay in address order,
  and converting a tiny function whose original has trailing padding shifts every
  later symbol. Both cases link fine but fail `make check`.
- `make build` and `make objdiff-config` each start with a clean and take ~30 s.
  Everything incremental goes through `ninja` inside the container
  (`make shell`, or `diff_func.sh` which does it for you).
