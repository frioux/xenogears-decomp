#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// The tail of Psy-Q libgte from SetGeomScreen on. SetGeomOffset (the function
// before it) carries two pad words in retail -- the shipped .OBJ pads every
// function's tail to a 16-byte multiple -- and they are the `asm` subsegment at
// 0x3A944 in config/slus_006.64.yaml, which this file starts after at 0x3A94C.

void SetGeomScreen(long h) {
    gte_SetGeomScreen(h);
}
