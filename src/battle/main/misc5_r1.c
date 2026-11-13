#include "common.h"

extern u8 D_800D3420[];

void func_8007ABD8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    s32 v = pRow[p[1]] + ((p[3] << 8) + p[2]);

    if (v > 0xFFFF) {
        v = 0xFFFF;
    }
    pRow[p[1]] = v;
}
