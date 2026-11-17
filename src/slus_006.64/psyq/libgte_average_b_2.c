#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotMatrixZYX on: RotMatrixYXZ (the function before it) carries one tail pad
// word in retail, so the four bytes at 0x3B3B8 are an `asm` subsegment in
// config/slus_006.64.yaml and this file covers 0x3B3BC-0x3B648. See
// psyq/libgte_average_b.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_b_2", RotMatrixZYX);

