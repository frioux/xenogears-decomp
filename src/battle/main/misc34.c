#include "common.h"

extern s32 D_80059464;
extern s16 D_800D3678;
extern s32 D_800D363C[];
extern u16 D_800D2D4C;
extern u16 D_800D36BC;
extern s32 D_800C3610;
extern s32 D_800C3628;
extern s32 D_800C3CE8;
extern s32 D_800D2D68;

extern void func_800A9FF0(s32);
extern void func_800BD2E4(void);
extern void *func_800BFC80(void *, s32, s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF4F0);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF5E8);
// 55.00%: retail keeps the address in one register and reuses it for the load
// and the store (lui $v1; lw $v0,%lo($v1); sw $v0,%lo($v1)). gcc emits the bare
// `sw $2,D_800C3CE8` and maspsx expands it with $at, so the store cannot reuse
// the address register. Not affected by --dont-expand-li (XenoBattleTail).
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
// 45.00%: same maspsx store-expansion wall as func_800BF5E8 - retail emits
// `lui $v0` itself and puts the `sw` in the jr delay slot.
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
// 96.76%: identical apart from an allocator tie - retail keeps the index in
// $a1 and the advancing pointer in $v1, gcc 2.7.2 swaps them (four source
// shapes tried: while/for, indexed/deref, decl order, split increments).
/*
s32 func_800BF954(s32 arg0) {
    s16 count = D_800D3678;
    s32 i = 0;

    if (count != 0) {
        s32 *pEntry = D_800D363C;

        for (i = 0; i != count; i++) {
            if (pEntry[i] == arg0) {
                break;
            }
        }
    }
    return i;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF998);
// Untested (the round ended before the preset fix landed).
/*
void func_800BF998(void) {
    s16 first = D_800D2D4C + 1;
    s16 second = D_800D36BC + 1;

    D_800D2D4C = first;
    D_800D36BC = second;
    if (first == 2) {
        func_800A9FF0(0xB);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF9EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFA9C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFBA0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFC80);

void func_800BFD88(void *arg0, s32 arg1) {
    func_800BFC80(arg0, arg1, 1);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFDA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFE48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0314);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0564);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C06E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0758);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C07CC);
