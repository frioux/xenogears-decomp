#include "common.h"
#include "system/memory.h"

/*
 * This unit's retail object materialises absolute symbol addresses into
 * allocated registers (lui $v0 / $v1 / $s0 ...) for every access, and it was
 * assembled with li left unexpanded. Our Default preset does neither: gcc emits
 * the absolute form and GNU as expands it with $at, and maspsx expands
 * `li $a1,1` to `ori` where retail has `addiu`. So functions whose symbol
 * address is used only once cannot match under the Default preset, and any C
 * using a small positive constant is off by one instruction. Confirmed with
 * try_flags.sh: gcc 2.6.0 / 2.7.2-cdk / -O1 / -O3 / -G0 / -mcpu=r8000 all
 * score the same.
 */

extern int ArchiveDataSync(void);
extern void func_800BE790(void);
extern void func_800245D8(void *, s32);
extern void func_80021BF8(void *, void *);
extern void func_8002A498(s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B73EC);
/*
 * 100.00% once maspsx runs with --aspsx-version=2.56 (li not expanded); under
 * the Default preset the only difference is `ori $a1,$zero,1` vs `addiu`.
 *   100.00%
void func_800B73EC(void) {
    u8 *pWork = HeapAlloc(0x10F7C, 1);

    *(u8 **)(pWork + 4) = pWork;
    *(u8 **)(pWork + 0x20) = pWork;
    func_800B7424(pWork);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7424);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7870);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C28);
/*
 * 45.00%: retail keeps the address in $v0 and the store in the jr delay slot,
 * we get `lui $at / sb $zero,0($at) / jr` because gcc emits the absolute form.
 *   45.00%
extern u8 D_800D2FDC;
void func_800B7C28(void) {
    D_800D2FDC = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C34);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7E94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8048);
/*
 * 45.00%, same cause: retail is `lui $v0,%hi(D_800C3E1C) / jr / sw $a0,%lo($v0)`.
 *   45.00%
extern void *D_800C3E1C;
void func_800B8048(void *pArg) {
    D_800C3E1C = pArg;
}
*/
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8D04);
/*
 * 81.63%: retail CSEs both symbol addresses into $s0; gcc emits the absolute
 * form twice, so the load gets its destination as scratch and the store gets
 * $at.
 *   81.63%
extern s32 D_80059464;
extern void *D_800C3618;
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
