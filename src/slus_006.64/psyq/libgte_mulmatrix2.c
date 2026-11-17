#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// MulMatrix2 (LIBGTE/MTX_04.OBJ) is its own translation unit only because the
// shipped .OBJ pads its tail to a 16-byte multiple: the nop at 0x3A4E8 is the
// `asm` subsegment in config/slus_006.64.yaml, so psyq/libgte_tail_b starts at
// ApplyMatrix. Nothing else separates the three files.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_mulmatrix2", MulMatrix2);
