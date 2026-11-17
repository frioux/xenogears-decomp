#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// NormalClip. It is its own translation unit because every neighbour carries tail
// pad words in retail (the shipped .OBJ pads every function's tail to a 16-byte
// multiple and the linker kept them): RotTrans's two words at 0x3AF04,
// NormalClip's three at 0x3AF30 and RotTransPers4's two at 0x3AFB4 are `asm`
// subsegments in config/slus_006.64.yaml, so this file covers 0x3AF0C-0x3AF30.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_nclip", NormalClip);
