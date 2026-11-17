#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotTrans alone: it carries two tail pad words in retail (the shipped .OBJ pads
// every function's tail to a 16-byte multiple and the linker kept them), so
// config/slus_006.64.yaml owns 0x3AF04 as an `asm` subsegment and this file
// starts at 0x3AEDC, ends at 0x3AF04.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotTrans);

