#include "common.h"

extern void SoundFreeWdsEntry(void *);
extern void SoundRemoveSedsEntry(void *);
extern s32 *D_800C3A6C;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0828);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C08CC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0D18);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0F70);
/* Best 81.0% (-O1) / 67.93% (Default). Retail keeps &D_800C3A6C in a callee-saved
   $s0 across the call so the store reuses it; gcc 2.7 folds the symbol address into
   the load/store operands and re-materialises it with $at.
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
