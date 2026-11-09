#include "common.h"

extern u8 D_800C3EB7[];
extern u16 D_800CCD64[];
extern u8 D_800D301C[];
extern u16 D_800D39DC;
extern u8 D_800D3430[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EA08);
s32 func_8007EA4C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return !(pRow[p[1]] < p[2]);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EA84);
s32 func_8007EAC8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return pRow[p[1]] == pRow[p[2]];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EB08);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EB50);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EB90);
s32 func_8007EBD8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    s32 v = pRow[p[1]];

    v &= p[2];
    return v != 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EC10);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EC54);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EC94);
s32 func_8007ECDC(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return (pRow[p[1]] ^ p[2]) != 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007ED14);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007ED58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007ED98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EDE0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EE28);
s32 func_8007EE70(u8 **arg0) {
    u8 *p = *arg0;

    return D_800CCD64[p[1] * 0xB8] >> 15;
}
s32 func_8007EEA8(u8 **arg0) {
    u8 *p = *arg0;

    return D_800D301C[p[1] * 4] == 0;
}
s32 func_8007EED0(void) {
    return (D_800D39DC & 3) == 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EEE8);
s32 func_8007EF44(u8 arg0) {
    s32 idx = arg0 + 3;

    return D_800C3EB7[idx * 28] >> 7;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EF6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007F8C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007FB70);
