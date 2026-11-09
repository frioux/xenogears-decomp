#include "common.h"

// Belongs to libapi?

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_3", ChangeClearRCnt);
/*
BIOS call gate (`li t2,0xC0; jr t2; li t1,0xA`) plus a pad `nop` after `.size`:
no C definition emits either, so convertible.py marks ChangeClearRCnt BLOCKED.
*/
