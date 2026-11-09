#include "common.h"

extern void WorkListSetTaskCallback(void *pEntry, void *pCallback);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void DrawSync(s32 mode);
extern void HeapFree(void *pMem);
extern void func_80025180(void *pArg);
extern void func_800245D8(s32 arg0, s32 arg1);
extern void func_800B7160(void);
extern u_long *g_GfxCurOT;
extern u_long *D_800C3CB4;
extern u8 D_80025A88[];
extern u8 D_800C3EB0[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B63F0);
void func_800B6438(u8 *p) {
    WorkListSetTaskCallback((u8 *)*(u32 *)(p + 0x6C) + 0x1C, D_80025A88);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6464);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B64D4);
/* best 77.59%: retail adds 0x8C8C as a separate li+addu on the runtime base
   (li v1,0x8C8C; addu v0,v0,v1; lw a0,0(v0)), gcc always folds it into the
   symbol address as lui at,0x1 / lw a0,-29556(at).  Tried: local base pointer,
   separate offset statements, u32 integer base, gcc-2.6.0 and 2.7.2-cdk.
void func_800B64D4(s32 arg0, u8 *p) {
    u32 nEntry = (u32)D_800C3EB0 + (p[1] << 2);

    func_800245D8(*(u32 *)(nEntry + 0x8C8C), p[0]);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6518);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B65B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6808);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6930);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6990);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B69E4);
void func_800B6A50(u8 *p) {
    u8 *pSrc = *(u8 **)(p + 0x70);

    *(u32 *)(p + 0xC) = *(u32 *)(pSrc + 0xC);
    *(u32 *)(p + 0x10) = *(u32 *)(pSrc + 0x10);
    *(u32 *)(p + 0x14) = *(u32 *)(pSrc + 0x14);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6A7C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6B98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6BFC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6C44);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6C98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6CEC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6DC0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6E84);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6F0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B7134);
/* best 65.36%: retail stores D_800C3CB4 through a register base
   (lui v0,%hi; sw v1,%lo(v0), in the jal delay slot) where gcc+maspsx emit
   lui at,%hi / sw v0,0(at).  Same $at store as misc26's func_800B3588.
void func_800B7134(void) {
    D_800C3CB4 = g_GfxCurOT;
    func_800B7160();
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B7160);
void func_800B7330(void *p) {
    DrawSync(0);
    HeapFree(p);
}
void func_800B7364(u8 *p) {
    WorkListRemoveTask(p + 0x1C);
    TimerWorkListRemoveTask(p);
    func_80025180(p);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B73A0);
