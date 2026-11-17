#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q libgte from SetBackColor to SetGeomScreen. ReadGeomScreen (before it)
// carries a pad word in retail -- the shipped .OBJ pads every function's tail
// to a 16-byte multiple -- so that word at 0x3A8E8 is an `asm` subsegment in
// config/slus_006.64.yaml and this file starts at 0x3A8EC; SetGeomScreen's own
// pad at 0x3A958 ends it and starts psyq/libgte_b_3.

void SetBackColor(long rbk, long gbk, long bbk) {
    gte_ldbkdir(rbk << 4, gbk << 4, bbk << 4);
}

void SetFarColor(long rfc, long gfc, long bfc) {
    gte_ldfcdir(rfc << 4, gfc << 4, bfc << 4);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b_2", SetGeomOffset);

void SetGeomScreen(long h) {
    gte_SetGeomScreen(h);
}

