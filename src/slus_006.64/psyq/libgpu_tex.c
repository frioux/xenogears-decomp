#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"

u_short func_8004376C(u_long *p, int tp, int abr, int x, int y, int w, int h) {
    RECT rect;

    rect.x = x;
    rect.y = y;
    rect.h = h;
    switch (tp) {
    case 0:
        rect.w = w / 4;
        break;
    case 1:
        rect.w = w / 2;
        break;
    case 2:
        rect.w = w;
        break;
    }
    LoadImage(&rect, p);
    return GetTPage(tp, abr, x, y);
}

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
