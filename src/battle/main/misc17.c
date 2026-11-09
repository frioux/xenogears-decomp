#include "common.h"

extern u8 D_800D2D24[];
extern u8 *D_800D2D28;
extern u8 D_800C3E50;
extern u8 *D_800C34B0;
extern u8 *D_800C3EA4;
extern s32 D_800CCB34;

void func_8007FB70(void);
s32 func_80076A10(s32, u8 *, s32, s16);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091064);
/* 94.88%: three addu operand orders + one load-scheduling slot differ; the
   instruction multiset and every load/store are identical. Tried =f()+a
   instead of += (flips the accumulate but moves the 0xF8 reload) and both
   source orders of offset+arg0 (gcc prints arg0 first regardless). */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091604);
/*
void func_80091604(s16 arg0) {
    s32 i;
    s32 offset;

    *(s32 *)(D_800D2D28 + 0xF8) = 0;
    for (i = 0, offset = 0x38; i < 8; i++, offset += 8) {
        *(s32 *)(D_800D2D28 + 0xF8) += func_80076A10(0x66,
            D_800C3EA4 + 0xBA8 + *(s32 *)(D_800D2D28 + 0xF8) * 80, 0x20,
            (s16)(offset + arg0));
    }
    D_800D2D28[0xA5] = *(u8 *)&D_800CCB34;
    D_800D2D28[0x9C] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800916D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009187C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091B38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091D38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091EC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009209C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092298);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092784);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092B74);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800930AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093578);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009382C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800939CC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093B08);
void func_8009413C(u8 arg0, u8 arg1) {
    if (D_800D2D24[arg0] == 4) {
        D_800D2D28[0xB7] = 0;
        D_800D2D28[0xB0] = 0;
        if (arg1 != 0) {
            func_8007FB70();
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800941A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800946F4);
/* 99.77%: instruction multiset identical; the only difference is the operand
   order of one addu (i*0x170 + pBase). Tried array-ref, pointer-plus, integer
   base and a pointer local for that sum; gcc always emits index-first there. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094C78);
/*
void func_80094C78(void) {
    u8 *pBase = D_800C34B0;
    u8 *pOther = (u8 *)(D_800C3E50 * 4 + (u32)pBase);
    u8 i;

    for (i = 0; i < 3; i++) {
        if (((u8 *)(pBase + i))[0x5FA0] != 0) {
            continue;
        }
        if (*(u8 *)((u32)pBase + i * 0x170 + 0x15A) & 0x80) {
            *(u32 *)(pBase + i * 4 + 0x5F60) += *(u32 *)(pOther + 0x5F6C);
        } else {
            *(u32 *)(pBase + i * 4 + 0x5F54) += *(u32 *)(pOther + 0x5F6C);
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094D24);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094EE4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80095690);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800957D8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800958D8);
