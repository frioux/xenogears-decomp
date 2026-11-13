#include "common.h"

extern s32 D_800C367C;
extern s16 D_800C3740;
extern u8 D_800C37C8;

extern void func_800BC2F0(s32);
extern void HeapFree(void *pMem);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void func_8001F6B0(u8 *pEntry);
extern s32 rcos(s32 a);
extern s32 rsin(s32 a);
extern void TimerWorkListSetTaskCallback(void *pEntry, void *pCallback);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB248);

void func_800BB314(u8 *pEntry) {
    u32 unused[2];
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB350);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB540);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB620);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB690);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB6E0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB760);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB7F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB844);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB9D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBAB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBEE0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC018);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC158);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC2F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC3F8);
/* best 45%: unmatchable macro-store, see func_800BC454 below. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC404);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC454);
/* best 45%, and 45.0 under every gcc/-O/-G in try_flags --sweep: gcc emits the
   one-insn macro store `sw $a0,D_800C3740` and maspsx expands it with $at
   (`lui at,%hi` + `sw ...,0(at)`), while retail ASPSX picks $v0 and reorg then
   fills the jr delay slot with the store. Same $at wall as misc32/misc28. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC460);

void func_800BCAA4(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(4);
    }
}

void func_800BCAD0(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(1);
    }
}

void func_800BCAFC(u8 *pEntry, s32 angle) {
    s32 color;

    color = rcos(angle << 6);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pEntry[0x28] = color;
    pEntry[0x29] = color;
    pEntry[0x2A] = color;
    func_8001F6B0(pEntry);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BCB54);
/* best 86.08%: retail keeps the address of D_800C3748 in $s1 across the calls
   (lui s1,%hi then two %lo(...)($s1) uses); gcc rematerialises it per use
   because the load and the store sit in different basic blocks, so we get
   lui s0 / lw s0 plus a lui $at store, and the frame is 24 instead of 32.
   Plain `pEntry = D_800C3748`, a u8 **pSlot = &D_800C3748 local, and moving the
   store out of the if all fold back to the same RTL. */
/* void func_800BCB54(void) {
    u8 *pEntry;

    pEntry = D_800C3748;
    if (pEntry != NULL) {
        u8 *pFlags;

        pFlags = *(u8 **)(pEntry + 0x1C);
        pFlags[0x2B] |= 1;
        TimerWorkListRemoveTask(pEntry);
        HeapFree(pEntry);
        D_800C3748 = NULL;
    }
} */
void func_800BCBB4(u8 *pEntry) {
    s32 color;
    s32 angle;
    u8 *pData;
    /* the incoming $a0 only survives the angle load; the rest reads a saved copy */
    u8 *pSaved = pEntry;

    angle = *(s32 *)(pEntry + 0x20) << 6;
    pData = *(u8 **)(pSaved + 0x1C);
    color = rcos(angle);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pData[0x28] = color;
    color = rsin(angle);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pData[0x29] = color;
    pData[0x2A] = color;
    func_8001F6B0(pData);
    *(s32 *)(pSaved + 0x20) += 1;
}
