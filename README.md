# Xenogears Decompilation
[![Progress]][progress site]

[Progress]: https://frioux.github.io/xenogears-decomp/badge.svg
[progress site]: https://frioux.github.io/xenogears-decomp/

An in-progress matching decompilation of Xenogears for the PlayStation. The
project targets the US release (SLUS 006.64) and its code overlays, with the
intention to target other releases later.

Progress is tracked per binary, unit and function at
<https://frioux.github.io/xenogears-decomp/>.

<img src="https://i.imgur.com/FfAa7QA.png" />

## Contributing

Read [AGENTS.md](AGENTS.md). It is the runbook for both people and LLM agents:
environment, the build modes, the match-diff-verify loop, conventions, compiler
presets, and where the remaining work is. Agent-assisted contributions are
welcome and expected; the bar is the same for everyone: `make check` passes and
the C looks like something the original developers wrote.

## Quick start

You need a disc 1 image of Xenogears (USA) as a raw `.bin` (`MODE2/2352`).

```sh
git clone --recursive https://github.com/frioux/xenogears-decomp
cd xenogears-decomp
make extract DISC="/path/to/Xenogears (USA) (Disc 1).bin"   # extracts SLUS_006.64 + overlays into disc/
make check                                                   # builds everything and verifies the checksums
```

**Linux x86_64** builds natively: install `make binutils-mips-linux-gnu
cpp-mips-linux-gnu python3 python3-venv`, create a venv and
`pip install -r requirements.txt`.

**macOS and everything else** builds in Docker (`make` does this for you). On
Apple Silicon use a Rosetta-enabled VM, for example `colima start --vz-rosetta`;
plain qemu emulation is unreliable for the assembler. You also need `cargo` to
build the `gears` metabuild tool natively, and Python 3.10+ with
`requirements.txt` installed for the host-side scripts.

## Progress page

```sh
make progress          # render build/progress-site/index.html locally
make progress-publish  # render with history and push to the gh-pages branch
```

The report comes from `objdiff`; the page shows overall, per-binary, per-unit and
per-function match state plus a history chart.

## Thanks
This project leans on a lot of work done by others before.
- [ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp) - the upstream project this fork continues from
- [splat](https://github.com/ethteck/splat) - For splitting the binaries
- [spimdisasm](https://github.com/Decompollaborate/spimdisasm) - For disassembling the binaries
- [The silent hill decompilation](https://github.com/Vatuu/silent-hill-decomp/tree/master) - As a project structure template this project could be adapted from.
- [MASPSX](https://github.com/mkst/maspsx) - For making parts of building less painful
- [objdiff](https://github.com/encounter/objdiff) - For diffing
- [decomp.me](https://decomp.me/) - For decompiling in collaboration
