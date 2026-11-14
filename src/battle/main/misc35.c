#include "common.h"
#include "psyq/libgte.h"

extern void SoundFreeWdsEntry(void *);
extern void SoundRemoveSedsEntry(void *);
extern s32 *D_800C3A6C;

void func_800C0828(SVECTOR *pFrom, SVECTOR *pTo, u16 *pRot) {
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0F70);
/* Best 67.93% (Default and 2.7.2-cdk). ATFREE wall: retail keeps %hi(D_800C3A6C)
   in a callee-saved $s0 across the call and stores through it; gcc folds the
   address into the load ($a0 as base) and re-forms it with $at for the store.
   Also tried a pointer local s32 *p = &D_800C3A6C; same 67.93%.
void func_800C0F70(void) {
    if (D_800C3A6C != 0) {
        SoundFreeWdsEntry(D_800C3A6C);
    }
    D_800C3A6C = 0;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0FAC);

void func_800C1140(s32 *pArg) {
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
