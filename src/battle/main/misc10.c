#include "common.h"

extern u8 D_800C3EB7[];
extern u16 D_800CCD64[];
extern u8 D_800D2D24[];
extern u8 *D_800D2D28;
extern u8 D_800D301C[];
extern u16 D_800D39DC;
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];

s32 func_8007EA08(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] | ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return value >= pRow[(*arg0)[1]];
}
s32 func_8007EA4C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return !(pRow[p[1]] < p[2]);
}
s32 func_8007EA84(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] | ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return pRow[(*arg0)[1]] >= value;
}
s32 func_8007EAC8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return pRow[p[1]] == pRow[p[2]];
}
s32 func_8007EB08(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return pRow[p[1]] == pRow[p[2]];
}
s32 func_8007EB50(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return pRow[p[1]] <= pRow[p[2]];
}
s32 func_8007EB90(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return pRow[p[1]] <= pRow[p[2]];
}
s32 func_8007EBD8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    s32 v = pRow[p[1]];

    v &= p[2];
    return v != 0;
}
s32 func_8007EC10(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] + ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return (pRow[(*arg0)[1]] & value) != 0;
}
s32 func_8007EC54(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    u8 lhs = pRow[p[1]];

    return (pRow[p[2]] & lhs) != 0;
}
s32 func_8007EC94(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    u16 a = pRow[p[1]];
    u16 b = pRow[p[2]];

    return (b & a) != 0;
}
s32 func_8007ECDC(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return (pRow[p[1]] ^ p[2]) != 0;
}
s32 func_8007ED14(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] | ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return (pRow[(*arg0)[1]] ^ value) != 0;
}
s32 func_8007ED58(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return (pRow[p[1]] ^ pRow[p[2]]) != 0;
}
s32 func_8007ED98(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return (pRow[p[1]] ^ pRow[p[2]]) != 0;
}
s32 func_8007EDE0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    return (pRow[p[1]] ^ pRow[p[2]]) == 0;
}
s32 func_8007EE28(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    return pRow[p[1]] <= pRow[p[2]];
}
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
s32 func_8007EEE8(void) {
    s32 ret = 1;
    s32 i = 0;
    s32 ofs = 0x54;

    for (; i < 8; i++) {
        if ((D_800D39DC >> 5) != 0) {
            if ((D_800C3EB7[ofs] & 0x80) == 0) {
                ret = 0;
                break;
            }
        }
        ofs += 0x1C;
    }

    return ret;
}
s32 func_8007EF44(u8 arg0) {
    s32 idx = arg0 + 3;

    return D_800C3EB7[idx * 28] >> 7;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EF6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007F8C0);
void func_8007FB70(u8 arg0) {
    if (D_800D2D24[arg0] == 4 && D_800D2D28[0x8E] != 0) {
        func_8008FA60(0);
        func_8007765C();
        D_800D2D28[0x8E] = 0;
    }
}
