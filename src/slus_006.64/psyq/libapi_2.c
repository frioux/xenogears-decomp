#include "common.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", setjmp);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", longjmp);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", func_8004BEC0);
/*
Not a function: longjmp ends at 0x3C6BC (its own .size stops before its return)
and this stub is longjmp's tail, `jr $ra; nop` (0x3C6C0), plus the 8 bytes of
16-byte alignment padding the original libapi.obj emits with `.align 4`
(0x3C6C8..0x3C6D0, func_8004BED0 starts 16-aligned). `void func_8004BEC0(void) {}`
reproduces the two instructions (objdiff 100%) but nothing emits the padding
short of the `.align 4` in the original source, so the unit's .text lands 8
bytes short; the other entries in this unit
(func_8004BED8, ReturnFromException, ResetEntryInt, HookEntryInt) are syscall
stubs with 4..12 bytes of the same padding, and setjmp/longjmp save/restore ra,
gp, sp, fp, s0-s7 from fixed offsets, so no C definition can reproduce the
layout. The whole unit has to stay INCLUDE_ASM.
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", func_8004BED0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", func_8004BED8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", ReturnFromException);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", ResetEntryInt);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", HookEntryInt);