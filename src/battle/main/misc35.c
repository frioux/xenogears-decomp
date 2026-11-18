#include "common.h"
#include "psyq/libgte.h"

extern void SoundFreeWdsEntry(void *);
extern void SoundRemoveSedsEntry(void *);
extern s32 *g_BattleWdsEntry;

void BattleCalcLookAtRotation(SVECTOR *pFrom, SVECTOR *pTo, u16 *pRot) {
    MATRIX mtx; // unused: reproduces retail's 0x20-byte stack slot
    VECTOR diff;
    VECTOR sq;
    s32 len;

    diff.vx = pFrom->vx - pTo->vx;
    diff.vy = pFrom->vy - pTo->vy;
    diff.vz = pFrom->vz - pTo->vz;
    Square0(&diff, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    pRot[1] = ratan2(diff.vz, diff.vx);
    pRot[2] = ratan2(diff.vy, len);
    pRot[0] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C08CC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0D18);

void BattleFreeWdsEntry(void) {
    if (g_BattleWdsEntry != 0) {
        SoundFreeWdsEntry(g_BattleWdsEntry);
    }
    g_BattleWdsEntry = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0FAC);

void BattleRemoveSedsEntries(s32 *pArg) {
    s32 *pEntry = pArg;
    s32 i = pEntry[0] - 3;

    pEntry += 4;
    for (; i > 0; i--) {
        s32 *pChunk = (s32 *)(pEntry[0] + (s32)pArg);

        if (*pChunk == 0x73646573) {
            SoundRemoveSedsEntry(pChunk);
        }
        pEntry++;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C11CC);
