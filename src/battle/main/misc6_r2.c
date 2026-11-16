#include "common.h"
#include "battle/actor.h"

// One slot of the per-slot operator register block at g_BattleSlotRegs (stride 0x40):
// the same register file in 32-, 16- and 8-bit form. g_BattleSlotRows, D_800D3420 and
// D_800D3430 are its +0x10, +0x20 and +0x30 arrays; the handlers below read the
// three operand indexes from the slot's current instruction (BattleCommand) and
// apply the operation to that slot's registers.
typedef struct {
    /* 0x00 */ u8 unk00[0x10];
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
} BattleOpRegs; // Size: 0x40

extern BattleOpRegs g_BattleSlotRegs[];

void BattleByteRegAddClamp(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;
    s32 value = pRegs[pCmd->unk1] + pRegs[pCmd->unk2];
    u8 result = value;

    if (value > 0xFF) {
        result = 0xFF;
    }
    pRegs[pCmd->unk3] = result;
}
void BattleByteRegSubClamp(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;
    s32 value = pRegs[pCmd->unk1] - pRegs[pCmd->unk2];
    u8 result = value;

    if (value < 0) {
        result = 0;
    }
    pRegs[pCmd->unk3] = result;
}
// 100.0% under gcc 2.6.0-psx, 98.52% under 2.7.2-cdk and 96.5% under 2.7.2: needs a
// 2.6.0 TU of its own.
void BattleByteRegMulClamp(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;
    s16 value = pRegs[pCmd->unk1] * pRegs[pCmd->unk2];

    if (value > 0xFF) {
        value = 0xFF;
    }
    pRegs[pCmd->unk3] = value;
}
void BattleByteRegDiv(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;

    pRegs[pCmd->unk3] = pRegs[pCmd->unk1] / pRegs[pCmd->unk2];
}
void BattleByteRegMod(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;

    pRegs[pCmd->unk3] = pRegs[pCmd->unk1] % pRegs[pCmd->unk2];
}
void BattleByteRegAnd(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;

    pRegs[pCmd->unk3] = pRegs[pCmd->unk1] & pRegs[pCmd->unk2];
}
void BattleByteRegOr(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;

    pRegs[pCmd->unk3] = pRegs[pCmd->unk1] | pRegs[pCmd->unk2];
}
void BattleByteRegXor(BattleCommand **p, u8 index) {
    u8 *pRegs = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;

    pRegs[pCmd->unk3] = pRegs[pCmd->unk1] ^ pRegs[pCmd->unk2];
}
void BattleWordRegAddClamp(BattleCommand **p, u8 index) {
    u16 *pRegs = g_BattleSlotRegs[index].wordRegs;
    BattleCommand *pCmd = *p;
    s32 value = pRegs[pCmd->unk1] + pRegs[pCmd->unk2];

    if (value > 0xFFFF) {
        value = 0xFFFF;
    }
    pRegs[pCmd->unk3] = value;
}
void BattleWordRegSubClamp(BattleCommand **p, u8 index) {
    u16 *pRegs = g_BattleSlotRegs[index].wordRegs;
    BattleCommand *pCmd = *p;
    s32 value = pRegs[pCmd->unk1] - pRegs[pCmd->unk2];

    if (value < 0) {
        value = 0;
    }
    pRegs[pCmd->unk3] = value;
}
void BattleWordRegMulClamp(BattleCommand **p, u8 index) {
    u16 *pRegs = g_BattleSlotRegs[index].wordRegs;
    BattleCommand *pCmd = *p;
    s32 value = pRegs[pCmd->unk1] * pRegs[pCmd->unk2];

    if (value > 0xFFFF) {
        value = 0xFFFF;
    }
    pRegs[pCmd->unk3] = value;
}
void BattleWordRegDiv(BattleCommand **p, u8 index) {
    u16 *pRegs = g_BattleSlotRegs[index].wordRegs;
    BattleCommand *pCmd = *p;

    pRegs[pCmd->unk3] = pRegs[pCmd->unk1] / pRegs[pCmd->unk2];
}
void BattleWordRegMod(BattleCommand **p, u8 index) {
    u16 *pRegs = g_BattleSlotRegs[index].wordRegs;
    BattleCommand *pCmd = *p;

    pRegs[pCmd->unk3] = pRegs[pCmd->unk1] % pRegs[pCmd->unk2];
}
void func_8007B2C0(BattleCommand **p, u8 index) {
    BattleCommand *pCmd = *p;

    // Spelled out at each operand on purpose: with the base in one local gcc 2.6.0
    // keeps it in $v1, with three accesses it allocates retail's $a2.
    g_BattleSlotRegs[index].wordRegs[pCmd->unk3] =
        g_BattleSlotRegs[index].wordRegs[pCmd->unk1] & g_BattleSlotRegs[index].wordRegs[pCmd->unk2];
}

void func_8007B310(BattleCommand **p, u8 index) {
    BattleCommand *pCmd = *p;

    // Spelled out at each operand on purpose: with the base in one local gcc 2.6.0
    // keeps it in $v1, with three accesses it allocates retail's $a2.
    g_BattleSlotRegs[index].wordRegs[pCmd->unk3] =
        g_BattleSlotRegs[index].wordRegs[pCmd->unk1] | g_BattleSlotRegs[index].wordRegs[pCmd->unk2];
}

void func_8007B360(BattleCommand **p, u8 index) {
    BattleCommand *pCmd = *p;

    // Spelled out at each operand on purpose: with the base in one local gcc 2.6.0
    // keeps it in $v1, with three accesses it allocates retail's $a2.
    g_BattleSlotRegs[index].wordRegs[pCmd->unk3] =
        g_BattleSlotRegs[index].wordRegs[pCmd->unk1] ^ g_BattleSlotRegs[index].wordRegs[pCmd->unk2];
}

void BattleSlotSetByteField(u8 **p, u8 index) {
    u8 *pArgs = *p;

    BattleSlotByteVarGetSet((u8)(index + 3), pArgs[1], pArgs[2], 0);
}
void BattleSlotSetWordField(u8 **p, u8 index) {
    u8 *pArgs = *p;

    BattleSlotWordVarGetSet((u8)(index + 3), pArgs[1], pArgs[2] | (pArgs[3] << 8), 0);
}
