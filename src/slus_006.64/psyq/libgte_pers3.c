#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotTransPers3 alone, for the same reason: its tail pad is three words at
// 0x3AED0 (an `asm` subsegment), so the file covers 0x3AE7C-0x3AED0.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_pers3", RotTransPers3);
