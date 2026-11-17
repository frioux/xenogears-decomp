#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q's shipped .OBJ pads every function's tail to a 16-byte multiple and
// retail's linker kept the pad, so ReadGeomScreen's is an `asm` subsegment at
// 0x3A8E8 in config/slus_006.64.yaml and the rest of this unit's code starts
// psyq/libgte_b_2 at SetBackColor.

long ReadGeomScreen(void) {
    long h;

    gte_stgeom_screen_val(h);
    return h;
}
