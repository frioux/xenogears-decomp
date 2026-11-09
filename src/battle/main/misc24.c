#include "common.h"

extern u8 D_800C3B74;
extern u8 D_800C402F[];
extern s32 *D_800D3368[];

u8 func_800AA514(s16 arg0, s16 arg1, s32 arg2) {
    s16 v;

    v = arg2 + arg1 * arg0 / 256;
    if (v >= 0x100) {
        v = 0xFF;
    }
    return v;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA564);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA600);
/* 99.00% - only the two "mflo" destinations differ (original mflo $v0, this
   mflo $a1); the single-expression form is otherwise instruction-identical.
s32 func_800AA600(s32 arg0) {
    u8 *p;
    s32 r;

    r = 0;
    p = (u8 *)D_800D3368[arg0];
    if (p != 0) {
        r = (*(s16 *)(p + 0x24) * ((*(s16 *)(p + 0x1C) * *(s16 *)(*(u8 **)(p + 4) + 0x4E)) >> 12)) >> 12;
    }
    return r;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA650);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA6E0);
void func_800AA760(s32 arg0, u8 arg1) {
    s32 p;

    p = D_800D3368[arg0];
    if (p != 0) {
        *(u8 *)(p + 0x2A) = arg1;
    }
}
void func_800AA788(u32 arg0) {
    D_800C3B74 = arg0 & 1;
}
void func_800AA79C(s32 arg0, s32 arg1) {
    s32 tmp;

    ((u8 *)D_800D3368[arg0])[0x34] = 0;
    ((u8 *)D_800D3368[arg1])[0x34] = 1;
    tmp = D_800D3368[arg0];
    D_800D3368[arg0] = D_800D3368[arg1];
    D_800D3368[arg1] = tmp;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA7DC);
/* 88.24% - instruction multiset identical, only the "li a1,0xF7" of the loop
   condition sits after the arg0*0x48 preheader instead of before it.
u8 func_800AA7DC(s32 arg0) {
    s32 o;
    u8 v;

    o = arg0 * 0x48;
    do {
        v = D_800C402F[o];
        o += 0x48;
    } while (v == 0xF7);
    if (v == 0xFF) {
        v = 0xFE;
    }
    return v;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA820);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA898);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA934);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AAA20);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AAB34);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AAD54);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800ADF1C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE1BC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE220);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE2A4);
void func_800AEEEC(u8 *pArg) {
    *(s16 *)(pArg + 0x98) = -1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AEEF8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AEF68);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AF180);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AF270);
/* 85.71% - identical except the loop guard's delay slot (nop there, the
   arg0 += 7 lands there here) and one insn shorter for it.
void func_800AF270(u8 *arg0, u8 *arg1) {
    s32 n;
    s32 i;

    n = *(u16 *)(arg0 + 0xA);
    arg0 += 7;
    for (i = 1; i < n; i++) {
        arg1 += 0x7C;
        arg0 += 0x7C;
        if (*arg0 != 0) {
            *arg0 = 0;
            arg1[7] = 1;
        }
    }
}
*/
