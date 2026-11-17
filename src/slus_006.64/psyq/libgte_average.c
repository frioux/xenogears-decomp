#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q libgte from RotAverage4 on. It is its own translation unit only because
// RotTransPers4 (the function before RotAverage4) carries two tail pad words in
// retail: the shipped .OBJ pads every function's tail to a 16-byte multiple and
// the linker kept them, so 0x3AFB4 is an `asm` subsegment in
// config/slus_006.64.yaml and this file covers 0x3AFBC-0x3B038.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average", RotAverage4);
