#include "common.h"

extern s32 D_80059464;
extern s32 D_800C3610;
extern s32 D_800C3628;
extern s32 D_800C3CE8;
extern s32 D_800D2D68;

extern void func_800BD2E4(void);
extern void *func_800BFC80(void *, s32, s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF4F0);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF5E8);
// 55.00%: retail reuses one address register (lui $v1; lw $v0,%lo($v1); sw
// $v0,%lo($v1)); maspsx expands the store `sw $2,sym` with $at, so this class
// (single read-modify-write of an absolute global) cannot match under Default.
/*
void func_800BF5E8(void) {
    D_800C3CE8++;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF600);

void func_800BF6CC(void) {
    if (D_800C3610 != 0) {
        func_800BD2E4();
    }
}

s32 func_800BF720(void) {
    return D_800D2D68 != 0;
}

s32 func_800BF6F8(void) {
    return D_80059464 - func_800BF720();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF730);
// 45.00%: same maspsx store-expansion wall as func_800BF5E8; retail puts the
// `sw` in the jr delay slot behind a gcc-emitted lui.
/*
void func_800BF730(s32 arg0) {
    D_800C3628 = arg0;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF73C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF7C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF85C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF8CC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF954);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF998);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF9EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFA9C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFBA0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFC80);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFD88);
// 92.50%: only difference is retail's `addiu $a2,$zero,1`; gcc emits `li` and
// maspsx (expand_li, the Default preset) turns a positive li into `ori`.
/*
void func_800BFD88(void *arg0, s32 arg1) {
    func_800BFC80(arg0, arg1, 1);
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFDA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFE48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0314);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0564);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C06E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0758);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C07CC);
