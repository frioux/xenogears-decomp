#include "common.h"

extern u8 D_800C3B74;
extern u8 D_800C402F[];
extern s32 *D_800D3368[];

u8 func_800AA514(s16 arg0, s16 arg1, s32 arg2) {
    s16 v;

    v = arg1 * arg0 / 256 + arg2;
    if (v >= 0x100) {
        v = 0xFF;
    }
    return v;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA564);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA600);
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

    *(u8 *)(D_800D3368[arg0] + 0x34) = 0;
    *(u8 *)(D_800D3368[arg1] + 0x34) = 1;
    tmp = D_800D3368[arg0];
    D_800D3368[arg0] = D_800D3368[arg1];
    D_800D3368[arg1] = tmp;
}
u8 func_800AA7DC(s32 arg0) {
    s32 o;
    u8 v;

    o = arg0 * 0x48;
    while ((v = D_800C402F[o]) == 0xF7) {
        o += 0x48;
    }
    if (v == 0xFF) {
        v = 0xFE;
    }
    return v;
}
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
