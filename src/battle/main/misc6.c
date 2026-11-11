#include "common.h"

// Operator argument tables: D_800D3420 is the 16-bit field table, D_800D3430
// the 8-bit one; both are indexed by (*p + 0x3) << 6-style slot * 0x40 bytes.
extern u8 D_800D3420[];
extern u8 D_800D3430[];

// The 9 parked stubs below are one and the same miss: retail holds the table base
// in $v0/$a2 while our gcc gives it $v1, the second entry of mips REG_ALLOC_ORDER
// (a local_alloc priority tie between the `la` temp and (*p)[k]). func_8007AE38 and
// func_8007AE98 add a second shape: retail keeps *p in $a0, gcc renames it to $a2.
//
// Four bodies are un-parked because they are byte-exact under another compiler, so
// this unit does not link at its current Default preset: func_8007AD6C, func_8007ADB0
// and func_8007ADF4 under gcc 2.7.2-cdk, func_8007AEF0 under gcc 2.6.0. func_8007B264
// is exact only under 2.7.2/2.6.0 (96.96 under cdk), so a switch to cdk must split
// the unit at func_8007AD6C rather than move it whole.

// best 97.00% (2.7.2 and 2.6.0; 86.75 cdk): identical instruction multiset, only
// the D_800D3420 base register differs - retail $v0/$a2, gcc $v1.
/*
void func_8007AC30(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    s32 value = pField[pArgs[1]] - (pArgs[2] + (pArgs[3] << 8));

    if (value < 0) {
        value = 0;
    }
    pField[pArgs[1]] = value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AC30);
// best 97.39% (2.6.0; 96.52 2.7.2; 89.35 cdk): same single-register miss as
// func_8007AC30, the D_800D3420 base lands in $v1 instead of retail's $v0/$a2.
/*
void func_8007AC80(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    s32 value = pField[pArgs[1]] * (pArgs[2] + (pArgs[3] << 8));

    if (value > 0xFFFF) {
        value = 0xFFFF;
    }
    pField[pArgs[1]] = value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AC80);
// best 83.06% (cdk; 81.94 2.7.2/2.6.0; --expand-div drops it to 31.94, so not the
// div trap): base in $v1 rather than $v0 renames the table loads and swaps div operands.
/*
void func_8007ACDC(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] /= pArgs[2] + (pArgs[3] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007ACDC);
// best 83.06% (cdk; 81.94 2.7.2/2.6.0): modulus form of func_8007ACDC, same $v1 base
// register miss, which renames the loads and the div/mflo pair.
/*
void func_8007AD24(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] %= pArgs[2] + (pArgs[3] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AD24);
// 100.0% under gcc 2.7.2-cdk, 97.x under this unit's 2.7.2: the whole run of
// three functions (AD6C, ADB0, ADF4) wants a cdk TU. Splitting it needs
// tools/scripts/split_unit.py, whose chunking still cuts park blocks - finish that
// first (see reports/TRACKER.md, session 7).
/*
void func_8007AD6C(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] &= pArgs[2] + (pArgs[3] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AD6C);
// 100.0% under gcc 2.7.2-cdk, 97.x under this unit's 2.7.2: the whole run of
// three functions (AD6C, ADB0, ADF4) wants a cdk TU. Splitting it needs
// tools/scripts/split_unit.py, whose chunking still cuts park blocks - finish that
// first (see reports/TRACKER.md, session 7).
/*
void func_8007ADB0(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] |= pArgs[2] + (pArgs[3] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007ADB0);
// 100.0% under gcc 2.7.2-cdk, 97.x under this unit's 2.7.2: the whole run of
// three functions (AD6C, ADB0, ADF4) wants a cdk TU. Splitting it needs
// tools/scripts/split_unit.py, whose chunking still cuts park blocks - finish that
// first (see reports/TRACKER.md, session 7).
/*
void func_8007ADF4(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] ^= pArgs[2] + (pArgs[3] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007ADF4);
// best 93.33% under every compiler: retail keeps *p in $a0 and copies the sum to $v1
// before slti, gcc renames *p to $a2 and tests the sum in place.
/*
void func_8007AE38(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;
    s32 value = pTable[pArgs[1]] + pTable[pArgs[2]];

    if (value > 0xFF) {
        value = 0xFF;
    }
    pTable[pArgs[3]] = value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AE38);
// best 94.55% under every compiler: retail keeps *p in $a0 and copies the difference
// to $v1 before bgez, gcc renames *p to $a2 and tests it in place.
/*
void func_8007AE98(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;
    s32 value = pTable[pArgs[1]] - pTable[pArgs[2]];

    if (value < 0) {
        value = 0;
    }
    pTable[pArgs[3]] = value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AE98);
// 100.0% under gcc 2.6.0-psx, 98.52% under 2.7.2-cdk and 96.5% under 2.7.2: needs a
// 2.6.0 TU of its own.
/*
void func_8007AEF0(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;
    s16 value = pTable[pArgs[1]] * pTable[pArgs[2]];

    if (value > 0xFF) {
        value = 0xFF;
    }
    pTable[pArgs[3]] = value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AEF0);
void func_8007AF5C(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;

    pTable[pArgs[3]] = pTable[pArgs[1]] / pTable[pArgs[2]];
}
void func_8007AFAC(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;

    pTable[pArgs[3]] = pTable[pArgs[1]] % pTable[pArgs[2]];
}
void func_8007AFFC(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;

    pTable[pArgs[3]] = pTable[pArgs[1]] & pTable[pArgs[2]];
}
void func_8007B040(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;

    pTable[pArgs[3]] = pTable[pArgs[1]] | pTable[pArgs[2]];
}
void func_8007B084(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;

    pTable[pArgs[3]] = pTable[pArgs[1]] ^ pTable[pArgs[2]];
}
void func_8007B0C8(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    s32 value = pField[pArgs[1]] + pField[pArgs[2]];

    if (value > 0xFFFF) {
        value = 0xFFFF;
    }
    pField[pArgs[3]] = value;
}
void func_8007B134(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    s32 value = pField[pArgs[1]] - pField[pArgs[2]];

    if (value < 0) {
        value = 0;
    }
    pField[pArgs[3]] = value;
}
void func_8007B198(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    s32 value = pField[pArgs[1]] * pField[pArgs[2]];

    if (value > 0xFFFF) {
        value = 0xFFFF;
    }
    pField[pArgs[3]] = value;
}
void func_8007B208(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] / pField[pArgs[2]];
}
void func_8007B264(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] % pField[pArgs[2]];
}
// best 99.00% (2.7.2 and 2.6.0; 96.50 cdk): 22 instructions, the only miss is the
// D_800D3420 base register - retail $a2, gcc $v1.
/*
void func_8007B2C0(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] & pField[pArgs[2]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B2C0);
// best 99.00% (2.7.2 and 2.6.0; 96.50 cdk): same one-register miss as func_8007B2C0
// (D_800D3420 base in $v1 instead of $a2).
/*
void func_8007B310(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] | pField[pArgs[2]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B310);
// best 99.00% (2.7.2 and 2.6.0; 96.50 cdk): same one-register miss as func_8007B2C0
// (D_800D3420 base in $v1 instead of $a2).
/*
void func_8007B360(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] ^ pField[pArgs[2]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B360);
void func_8007B3B0(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_80079ED8((u8)(index + 3), pArgs[1], pArgs[2], 0);
}
void func_8007B3E4(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_8007A280((u8)(index + 3), pArgs[1], pArgs[2] | (pArgs[3] << 8), 0);
}
