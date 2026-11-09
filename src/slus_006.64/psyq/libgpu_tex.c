#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_tex", func_8004376C);

u_short func_80043858(u_long *p, int x, int y) {
    RECT rect;

    rect.x = x;
    rect.y = y;
    rect.w = 0x100;
    rect.h = 1;
    LoadImage(&rect, p);
    return GetClut(x, y);
}

u_short func_800438C0(u_long *p, int x, int y) {
    RECT rect;

    rect.x = x;
    rect.y = y;
    rect.w = 0x10;
    rect.h = 1;
    LoadImage(&rect, p);
    return GetClut(x, y);
}
