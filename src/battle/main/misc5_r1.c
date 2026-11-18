#include "common.h"

extern u8 g_BattleSlotWordRegs[];

void BattleWordRegAddImmClamp(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(g_BattleSlotWordRegs + arg1 * 0x40);
    s32 v = pRow[p[1]] + ((p[3] << 8) + p[2]);

    if (v > 0xFFFF) {
        v = 0xFFFF;
    }
    pRow[p[1]] = v;
}
