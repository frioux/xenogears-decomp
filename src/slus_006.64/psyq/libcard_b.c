#include "common.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E880);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E8D8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E960);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E990);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E9FC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004EA20);

/*
Hand-written card-BIOS patch layer (the tail of libcard that got split off, see
the note at the end of libcard.c). All six stubs are unreachable from C: each one
keeps its return address in a global instead of a stack frame (D_800593E4 for the
BIOS B0-0x57 group, D_800593F4 for the 0x56 group) and ends with
`lw ra,<global>; nop; jr ra; nop`. gcc cannot write $ra to a global, and any C
spelling of these calls out through a 0x20 frame that saves ra, s0 and s1
(measured: func_8004E990, the most C-shaped of them, 40.30%). They also reach the
BIOS through the constant vector `li t2,0xB0; jalr t2; li t1,0x56|0x57`; gcc
emits `jal <symbol>`, never a jalr through a register holding a constant.

The unit is one 0x210-byte blob whose first 5 words are runtime-patched
trampolines, which is why two of the stubs look like nonsense as code:

- func_8004E880 (0x3F080) is a trampoline header -- lui, addiu of D_8004E894,
  `jr $v0` to the very next instruction, two nops -- and D_8004E894 is the real
  hook body: it takes its base pointer in $v1, which gcc never uses for an
  argument. It returns unless bit 0x80 of +0x1074 is set, spins while bit 0x80 of
  +0x1044 is set, then jumps through the RAM pointer jtbl_800593E8.
- func_8004E8D8 copies those same 5 words the other way: from func_8004E880 into
  the B0-0x56 work struct at +0x28, and stores the end pointer in
  jtbl_800593E8, so the body above is the pre-patch original.
- func_8004E960 (0x3F160) is the other trampoline header, a `jalr` to itself,
  followed by a 200-iteration delay loop whose bnez delay slot is a bare nop
  (gcc always fills it).
- func_8004E990 copies the 5 words at B0-0x57 work +0x9C8 over func_8004E960 and
  FlushCache()s.
- func_8004EA20 copies its own inline data -- D_8004EA88, the two zero words that
  end the unit's range -- into the B0-0x56 struct at +0x6C.
- func_8004E9FC is a hand-written delay loop; three pad nops follow its .size.

On top of that, tools/scripts/convertible.py marks func_8004E9FC (instructions
after .size) and func_8004EA20 (owns D_8004EA88) BLOCKED.

Names: EnterCriticalSection/ExitCriticalSection/FlushCache (libapi), BIOS B0
function numbers 0x56/0x57, and the patch pointer jtbl_800593E8.
*/
