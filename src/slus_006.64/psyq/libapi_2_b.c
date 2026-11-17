#include "common.h"

// The tail of the unit after func_8004BEC0's two pad words at 0x3C6C8 (an `asm`
// subsegment in config/slus_006.64.yaml), so this file covers 0x3C6D0-0x3C720.
// See psyq/libapi_2.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2_b", func_8004BED0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2_b", _96_remove);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2_b", ReturnFromException);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2_b", ResetEntryInt);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2_b", HookEntryInt);
