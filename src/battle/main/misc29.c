#include "common.h"
#include "system/memory.h"

/*
 * This unit's retail object materialises absolute symbol addresses into allocated
 * registers (lui $v0 / $v1 / $s0 ...) for every access, and it was assembled with
 * li left unexpanded (the XenoBattleTail preset's --dont-expand-li). Our assembler
 * always expands a one-shot absolute load/store's address into $at, so such a
 * function matches under no preset: 17 of the 21 stubs here are that class
 * (atfree.py). Re-checked with try_flags.sh --all: gcc 2.6.0 and 2.7.2-cdk score
 * identically to the unit's own preset.
 */

extern int ArchiveDataSync(void);
extern u8 D_800D2FDC;
extern void *D_800C3E1C;
extern s32 D_80059464;
extern void *D_800C3618;
extern void func_800BE790(void);
extern void func_800245D8(void *, s32);
extern void func_80021BF8(void *, void *);
extern void func_8002A498(s32);

void func_800B73EC(void) {
    u8 *pWork = HeapAlloc(0x10F7C, 1);

    *(u8 **)(pWork + 4) = pWork;
    *(u8 **)(pWork + 0x20) = pWork;
    func_800B7424(pWork);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7424);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7870);
// 45.00% under every preset tried (unit preset, gcc 2.6.0, gcc 2.7.2-cdk):
// retail keeps the address in $v0 and the store in the jr delay slot, while our
// assembler always expands this one-shot absolute store with $at (atfree.py).
/*
void func_800B7C28(void) {
    D_800D2FDC = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C28);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C34);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7E94);
// 45.00% under every preset tried (unit preset, gcc 2.6.0, gcc 2.7.2-cdk): the
// same at-free one-shot absolute store as func_800B7C28, retail is lui $v0 / jr /
// sw $a0 and we get $at.
/*
void func_800B8048(void *pArg) {
    D_800C3E1C = pArg;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8048);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8054);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8068);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B81BC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8284);

void func_800B8354(void) {
    while (ArchiveDataSync() != 0) {
        func_800BE790();
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B838C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B853C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8774);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8840);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B88C4);

void func_800B89F4(void) {
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B89FC);
// 81.63% under every preset tried (unit preset, gcc 2.6.0, gcc 2.7.2-cdk): retail
// CSEs both symbol addresses into $s0 and stores in the delay slot; the one-shot
// D_80059464 load goes through $at here (atfree.py).
/*
void func_800B8D04(void) {
    func_800B8354();
    func_800BF9EC();
    while (D_80059464 != func_800BF720()) {
        func_800BE790();
    }
    func_800BF3A4();
    if (D_800C3618 != NULL) {
        HeapFree(D_800C3618);
        D_800C3618 = NULL;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8D04);

void func_800B8D7C(void) {
    func_8002A498(0);
    func_800B8D04();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8DA4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8EBC);

void func_800B9020(u8 *pSprite) {
    func_800245D8(pSprite, *(s8 *)(pSprite + 0xB0));
    func_80021BF8(pSprite, 0);
}
