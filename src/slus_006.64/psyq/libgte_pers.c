#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotTransPers alone: it carries a tail pad word in retail (Psy-Q pads every
// shipped function to a 16-byte multiple, and the linker kept the padding), so
// config/slus_006.64.yaml owns 0x3AE78 as an `asm` subsegment and this file
// starts at 0x3AE4C, ends at 0x3AE78.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_pers", RotTransPers);
