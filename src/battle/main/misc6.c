#include "common.h"

// Operator argument tables: D_800D3420 is the 16-bit field table, D_800D3430
// the 8-bit one; both are indexed by (*p + 0x3) << 6-style slot * 0x40 bytes.
extern u8 D_800D3420[];
extern u8 D_800D3430[];

void func_8007AC30(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    s32 value = pField[pArgs[1]] - (pArgs[2] + (pArgs[3] << 8));

    if (value < 0) {
        value = 0;
    }
    pField[pArgs[1]] = value;
}
void func_8007AC80(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    s32 value = pField[pArgs[1]] * (pArgs[2] + (pArgs[3] << 8));

    if (value > 0xFFFF) {
        value = 0xFFFF;
    }
    pField[pArgs[1]] = value;
}
void func_8007ACDC(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] /= pArgs[2] + (pArgs[3] << 8);
}
void func_8007AD24(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] %= pArgs[2] + (pArgs[3] << 8);
}
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
void func_8007AE38(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;
    s32 value = pTable[pArgs[1]] + pTable[pArgs[2]];

    if (value > 0xFF) {
        value = 0xFF;
    }
    pTable[pArgs[3]] = value;
}
void func_8007AE98(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;
    s32 value = pTable[pArgs[1]] - pTable[pArgs[2]];

    if (value < 0) {
        value = 0;
    }
    pTable[pArgs[3]] = value;
}
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
void func_8007B2C0(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] & pField[pArgs[2]];
}
void func_8007B310(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] | pField[pArgs[2]];
}
void func_8007B360(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[3]] = pField[pArgs[1]] ^ pField[pArgs[2]];
}
void func_8007B3B0(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_80079ED8((u8)(index + 3), pArgs[1], pArgs[2], 0);
}
void func_8007B3E4(u8 **p, u8 index) {
    u8 *pArgs = *p;

    func_8007A280((u8)(index + 3), pArgs[1], pArgs[2] | (pArgs[3] << 8), 0);
}
