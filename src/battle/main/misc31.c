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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BCBB4);
