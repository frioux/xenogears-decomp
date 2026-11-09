#include "common.h"

// Operator argument tables: D_800D3420 is the 16-bit field table, D_800D3430
// the 8-bit one; both are indexed by (*p + 0x3) << 6-style slot * 0x40 bytes.
extern u8 D_800D3420[];
extern u8 D_800D3430[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AC30);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AC80);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007ACDC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AD24);
void func_8007AD6C(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] &= pArgs[2] + (pArgs[3] << 8);
}
void func_8007ADB0(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] |= pArgs[2] + (pArgs[3] << 8);
}
void func_8007ADF4(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] ^= pArgs[2] + (pArgs[3] << 8);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AE38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AE98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AEF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AF5C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AFAC);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B0C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B134);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B198);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B208);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B264);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B2C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B310);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007B360);
void func_8007B3B0(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_80079ED8(index + 3, pArgs[1], pArgs[2], 0);
}
void func_8007B3E4(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_8007A280(index + 3, pArgs[1], pArgs[2] | (pArgs[3] << 8), 0);
}
