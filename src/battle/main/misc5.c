#include "common.h"

extern u8 *D_800D2D28;
extern u8 D_800D3725[];
extern u16 D_8005A3A0[];
extern u8 D_800D3430[];
extern u8 D_800D3420[];
extern u8 D_800D3410[];

void func_80079E18(u8 arg0) {
    D_800D2D28[0xB4] = 1;
    D_800D3725[arg0 * 0x60] = 1;
}

void func_80079E4C(u8 arg0) {
    D_800D2D28[0xB4] = 0;
    D_800D3725[arg0 * 0x60] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_80079E7C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_80079ED8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A280);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A628);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A6C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A744);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A7BC);
u8 func_8007A828(u8 **arg0, u8 *arg1, u8 arg2) {
    u8 *p = *arg0;

    arg1[arg2 * 8 + p[1]] = p[2];
    if ((*arg0)[1] == 0) {
        arg2++;
    }
    return arg2;
}
void func_8007A874(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u8 *p = *arg0;
    s32 idx = arg3 * 8 + p[1];
    u8 *pRow = D_800D3430 + arg2 * 0x40;

    arg1[idx] = pRow[p[2]];
}
void func_8007A8B4(u8 **arg0, u8 *arg1) {
    u8 *p;
    s32 i;

    for (i = 0; i < 8; i++) {
        p = *arg0;
        arg1[p[2] * 8 + i] = arg1[p[1] * 8 + i];
    }
}
void func_8007A900(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] = p[2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A92C);
/* 18.33% - same 15 insns, gcc materialises the D_800D3420 lui/addiu before the
   p[2]/p[3] value (into a0, pushing the value to a2); the original forms the base
   after the value, reusing v1 once p dies. Tried: value-before-pointer, u16/s32
   value local, split |= statement, u8* base with cast store, direct index (gives
   the at-register form instead of a register base).
void func_8007A92C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 val = p[2] | (p[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    pRow[p[1]] = val;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A968);
/* 53.44% - the value expression and the index come out right, but gcc forms the
   D_800D3410 lui/addiu at the top (into a0) instead of after the value, so the
   store base is 2 insns early. Tried: pRow pointer local before/after the value,
   idx local first, single-expression store (gives the at-register form instead).
void func_8007A968(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 val = (p[2] + (p[3] << 8)) * 0x10;
    s32 idx = arg1 * 0x10 + p[1];

    ((u32 *)D_800D3410)[idx] = val;
} */
void func_8007A9A8(u8 **arg0) {
    D_8005A3A0[(*arg0)[1]] = (*arg0)[2];
}
void func_8007A9D0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] + p[2];
    s32 res;

    if (v < 0x100) {
        res = v;
    } else {
        res = 0xFF;
    }
    pRow[p[1]] = res;
}
void func_8007AA1C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] - p[2];

    if (v < 0) {
        v = 0;
    }
    pRow[p[1]] = v;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007AA60);
void func_8007AAB8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] /= p[2];
}
void func_8007AAF4(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] %= p[2];
}
void func_8007AB30(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] &= p[2];
}
void func_8007AB68(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] |= p[2];
}
void func_8007ABA0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] ^= p[2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007ABD8);
