#include "common.h"

extern u8 D_800C3EB7[];
extern u16 D_800CCD64[];
extern u8 D_800D301C[];
extern u16 D_800D39DC;
extern u8 D_800D3420[];
extern u8 D_800D3430[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EA08);
/* 96.47% - identical 17-insn multiset, only the register pick of the `p`
   pseudo differs: the original loads arg0[0] in place into a0 and gives the
   D_800D3420 base v0; gcc here gives p v0 (base v1, p[3] a0). A u16 `value`
   local is required for the unsigned compare (without it gcc emits slt).
   Tried: a u16 pointer for pRow, value declared before/after pRow,
   the direct expression with no local.

s32 func_8007EA08(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    u16 value = p[2] | (p[3] << 8);

    return value >= pRow[p[1]];
} */
s32 func_8007EA4C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return !(pRow[p[1]] < p[2]);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EA84);
/* 96.47% - identical 17-insn multiset, only the register pick of the `p`
   pseudo differs: the original loads arg0[0] in place into a0 and gives the
   D_800D3420 base v0; gcc here gives p v0 (base v1, p[3] a0). A u16 `value`
   local is required for the unsigned compare (without it gcc emits slt).
   Tried: a u16 pointer for pRow, value declared before/after pRow,
   the direct expression with no local.

s32 func_8007EA84(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3420 + arg1 * 0x40;
    u16 value = p[2] | (p[3] << 8);

    return ((u16 *)pRow)[p[1]] >= value;
} */
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EC10);
/* 96.47% - identical 17-insn multiset, only the register pick of the `p`
   pseudo differs: the original loads arg0[0] in place into a0 and gives the
   D_800D3420 base v0; gcc here gives p v0 (base v1, p[3] a0). A u16 `value`
   local is required for the unsigned compare (without it gcc emits slt).
   Tried: a u16 pointer for pRow, value declared before/after pRow,
   the direct expression with no local.

s32 func_8007EC10(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3420 + arg1 * 0x40;
    u16 value = p[2] + (p[3] << 8);

    return (((u16 *)pRow)[p[1]] & value) != 0;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EC54);
/* 99.38% - only the two lbu destinations are swapped: gcc puts the
   pRow[p[2]] value in v0 (it swaps the AND operands), the original puts
   pRow[p[1]] in v0. Tried: s32/u8 temp with `v &= pRow[p[2]]` in a separate
   statement (97.81/96.56, fixes the load order but moves the D_800D3430 base
   into v1 and p into a0), u8 lhs local, reversed declaration order.
s32 func_8007EC54(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    u8 lhs = pRow[p[1]];

    return (lhs & pRow[p[2]]) != 0;
} */
s32 func_8007EC94(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    u16 a = pRow[p[1]];
    u16 b = pRow[p[2]];

    return (a & b) != 0;
}
s32 func_8007ECDC(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return (pRow[p[1]] ^ p[2]) != 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007ED14);
/* 96.47% - identical 17-insn multiset, only the register pick of the `p`
   pseudo differs: the original loads arg0[0] in place into a0 and gives the
   D_800D3420 base v0; gcc here gives p v0 (base v1, p[3] a0). A u16 `value`
   local is required for the unsigned compare (without it gcc emits slt).
   Tried: a u16 pointer for pRow, value declared before/after pRow,
   the direct expression with no local.

s32 func_8007ED14(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3420 + arg1 * 0x40;
    u16 value = p[2] | (p[3] << 8);

    return (((u16 *)pRow)[p[1]] ^ value) != 0;
} */
s32 func_8007ED58(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return (pRow[p[1]] ^ pRow[p[2]]) != 0;
}
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
