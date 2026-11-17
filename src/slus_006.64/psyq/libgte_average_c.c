#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotMatrixY on: RotMatrixX (the function before it) carries two tail pad words
// in retail, so the eight bytes at 0x3B7E4 are an `asm` subsegment in
// config/slus_006.64.yaml and this file covers 0x3B7EC-0x3B984. See
// psyq/libgte_average_b.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_c", RotMatrixY);
