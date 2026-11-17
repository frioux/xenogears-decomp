#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotMatrixZ on: RotMatrixY (the function before it) carries two tail pad words
// in retail, so the eight bytes at 0x3B984 are an `asm` subsegment in
// config/slus_006.64.yaml and this file covers 0x3B98C-0x3BB24. See
// psyq/libgte_average_c.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_d", RotMatrixZ);
