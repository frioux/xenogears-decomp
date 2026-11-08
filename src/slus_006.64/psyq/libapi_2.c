#include "common.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", setjmp);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", longjmp);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", func_8004BEC0);
/*
100.00% as `void func_8004BEC0(void) {}` (8 bytes, same as the symbol), but the
stub file also holds two trailing nops after .size (0x3C6C8..0x3C6D0) that the C
emits nothing for, so the unit's .text comes up 8 bytes short and every later
address shifts. Cannot be matched without those bytes.
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", func_8004BED0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", func_8004BED8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", ReturnFromException);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", ResetEntryInt);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", HookEntryInt);