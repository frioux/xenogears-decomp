#include "common.h"

extern u8 D_800D3420[];
extern u8 D_800D3430[];

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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6_r2", func_8007AE38);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6_r2", func_8007AE98);
// 100.0% under gcc 2.6.0-psx, 98.52% under 2.7.2-cdk and 96.5% under 2.7.2: needs a
// 2.6.0 TU of its own.
void func_8007AEF0(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;
    s16 value = pTable[pArgs[1]] * pTable[pArgs[2]];

    if (value > 0xFF) {
        value = 0xFF;
    }
    pTable[pArgs[3]] = value;
}
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6_r2", func_8007B2C0);
// best 99.00% (2.7.2 and 2.6.0; 96.50 cdk): same one-register miss as func_8007B2C0
// (D_800D3420 base in $v1 instead of $a2).
/*
void func_8007B310(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] | pField[pArgs[2]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6_r2", func_8007B310);
// best 99.00% (2.7.2 and 2.6.0; 96.50 cdk): same one-register miss as func_8007B2C0
// (D_800D3420 base in $v1 instead of $a2).
/*
void func_8007B360(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] ^ pField[pArgs[2]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6_r2", func_8007B360);
void func_8007B3B0(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_80079ED8((u8)(index + 3), pArgs[1], pArgs[2], 0);
}
void func_8007B3E4(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_8007A280((u8)(index + 3), pArgs[1], pArgs[2] | (pArgs[3] << 8), 0);
}
