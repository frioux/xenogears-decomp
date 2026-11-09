#include "common.h"

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCC60);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCD8C);
/* best 45.00% (before the XenoBattleTail preset): retail's aspsx expands the
   bare `sh $0,SYM` macro into `lui v0,%hi; sh $0,%lo(v0)` and fills the jr
   delay slot with the store. gcc leaves the macro to GNU as, which emits
   `lui $at` + `sh 0($at)`: two insns, so the delay slot stays empty and the
   register differs.
extern s16 D_800C3D14;

void func_800BCD8C(void) {
    D_800C3D14 = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCD98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCEAC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCFAC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD024);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD1FC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD2E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD3AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD7A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD810);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD974);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDA1C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDB08);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDB74);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC14);
/* best 83.40% (before the XenoBattleTail preset): only the base register of the
   four accesses in the `if` body differs. gcc hoists the call's `move $4,$16`
   to the top of the block and addresses through $4; retail emits the copy at
   the call and stays on $16. Tried: u8* with pEntry+0x88 casts, a struct type,
   a pTask copy, a callback local, indexed and offset forms - all give $4.
extern void func_800BDF1C(u8 *pEntry);
extern void TimerWorkListSetTaskCallback(void *pEntry, void *pCallback);
extern void func_800BDB74(void);

typedef struct {
    u8 unk0[0x83];
    u8 unk83;
    u32 unk84;
    s32 unk88;
} Misc32Task;

void func_800BDC14(Misc32Task *pTask) {
    func_800BDF1C((u8 *)pTask);
    if (--pTask->unk88 < 0) {
        pTask->unk88 = 0x10;
        pTask->unk83 = (pTask->unk83 | 2) & 0xFE;
        TimerWorkListSetTaskCallback(pTask, func_800BDB74);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDCF8);
/* best 81.60% (before the XenoBattleTail preset): same $at macro-store problem
   as func_800BCD8C. Retail hoists `lui v0,%hi(D_800D2D68)`, expands the store
   with $v0 and reorg fills the first jal delay slot with it; GNU as' `lui $at`
   expansion cannot be stolen.
extern u8 *D_800D2D68;
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);

void func_800BDCF8(u8 *pEntry) {
    D_800D2D68 = 0;
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
}
*/
void func_800BDD34(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDD3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDE58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDF1C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BE0DC);
/* best 99.09% (before the XenoBattleTail preset): the load's macro expansion
   only. Retail has `lui v0,%hi; lw a0,%lo(v0)`; GNU as puts the address in the
   destination register (`lui a0; lw a0,0(a0)`).
extern u8 *D_800D2D68;
extern void func_800BDCF8(u8 *pEntry);

void func_800BE0DC(void) {
    if (D_800D2D68 != NULL) {
        func_800BDCF8(D_800D2D68);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BE108);
/* best 65.00% (before the XenoBattleTail preset): same $at macro-store problem,
   two stores; retail uses $v0 twice and fills the jr delay slot with the second.
extern u8 *D_800D2D68;
extern s32 D_800C374C;

void func_800BE108(void) {
    D_800D2D68 = NULL;
    D_800C374C = 0;
}
*/
