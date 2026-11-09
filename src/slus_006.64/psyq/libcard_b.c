#include "common.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E880);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E8D8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E960);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E990);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004E9FC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard_b", func_8004EA20);

/*
Hand-written card-BIOS patch layer (the tail of libcard that got split off, see
the note at the end of libcard.c). None of the six stubs is reachable from C:

- func_8004E880 is a hook body installed into the card BIOS copy: it reads $v1
  (set by its caller), jr's through the constant D_8004E894 and dispatches via
  the RAM pointer jtbl_800593E8.
- func_8004E8D8 / func_8004E990 / func_8004EA20 start with the SDK's inline
  `li t2,0xB0; jalr t2; li t1,0x56|0x57` BIOS gate and park the return address
  in the global D_800593E4 / D_800593F4 instead of on the stack, so gcc emits a
  `jal` to a symbol plus a real frame instead (best attempts scored 32% and 50%).
  They swap the words of the hooks below into the BIOS work struct and back.
- func_8004E960 materialises its own address and jalr's it.
- func_8004E9FC is a hand-written delay loop (three pad nops follow its .size) and
  func_8004EA20 owns D_8004EA88, so both are BLOCKED for tools/scripts/convertible.py
  as well.

Names: EnterCriticalSection/ExitCriticalSection/FlushCache (libapi), BIOS B0
function numbers 0x56/0x57, and the patch pointer jtbl_800593E8.
*/
