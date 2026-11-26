#include "common.h"
#include "battle/actor.h"

// One slot of the per-slot operator register block at g_BattleSlotRegs (stride 0x40):
// the same arguments in 32-, 16- and 8-bit form. g_BattleSlotRows, g_BattleSlotWordRegs and
// g_BattleSlotByteRegs are its +0x10, +0x20 and +0x30 arrays; the handlers below read the
// operand indexes from the slot's current instruction (BattleCommand) and apply
// the operation to that slot's registers.
typedef struct {
    /* 0x00 */ u8 unk00[0x10];
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
} BattleOpRegs; // Size: 0x40

// The occupancy a slot must have for BattleStoreCharSlotFlagMask to set its bit; any other
// operand byte sets the bit for every slot.
#define BATTLE_SLOT_TEST_IN_USE 1
#define BATTLE_SLOT_TEST_NOT_IN_USE 2

// One of the 0x1C-byte per-slot tag records at g_BattleSlotRecords: tag at +0, the slot's
// character at +2, flag byte at +3 (the byte BattleSetSlotRecordByteAndRelease writes), the
// slot's position at +0xA (D_800C3EBE is this +0xA, as in src/battle/main/misc8.c).
typedef struct {
    /* 0x0 */ u8 tag;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 charId; // BATTLE_SLOT_CHARACTER_NONE (0x7F) when no character holds the slot
    /* 0x3 */ u8 flags;
    /* 0x4 */ u8 unk4[6];
    /* 0xA */ u16 x;
    /* 0xC */ u16 y;
    /* 0xE */ u8 unkE[0xE];
} BattleSlotRecord; // Size: 0x1C

// Per-slot "can be picked" flag these loops gate on, one 8-byte record per
// battle slot; D_800D32B1 is the record the slot table is walked down from.
typedef struct {
    /* 0x0 */ u8 inUse;
    /* 0x1 */ u8 unk1[7];
} BattleSlotFlag; // Size: 0x8

extern BattleOpRegs g_BattleSlotRegs[];
extern BattleSlotRecord g_BattleSlotRecords[];
extern BattleSlotFlag g_BattleSlotFlags[];
extern u32 D_800D2C60[];
extern u8 g_BattleRowStatValues[];
extern u8 g_BattlePendingCharIndex;
extern u8 *g_BattlePrimPool;
extern u8 *g_BattleSlotParamTable;
extern u8 g_BattleSlotRows[];
extern u8 g_BattleSlotWordRegs[];
extern u8 g_BattleSlotByteRegs[];
extern u8 D_800C3EB7[];
extern u8 g_BattleSlotStatusRows[];
extern u8 D_800CD1B2[];
extern u8 D_800D32B1[];
extern u8 D_800D32F1[];
extern u8 BattleSlotFlagsC002ClearRecordIdle(u8 arg0, u8 arg1);
extern s32 BattleSlotWordVarGetSet(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);
extern u16 BattleSlotBitMask(u8 arg0);

void BattlePickSlotFromOperatorWord(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s16 last;
    s32 count = 0;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * sizeof(BattleOpRegs) + (s32)g_BattleSlotWordRegs);
    s32 rowIndex;
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    while (i < MAX_PARTY_MEMBERS) {
        u8 idx = i;
        u8 idle = BattleSlotFlagsC002ClearRecordIdle(idx, 0);

        i++;
        if (idle != 0) {
            if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    }
    last = count - 1;
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, last)]);
        u16 *pOut = (u16 *)((rowIndex & 0xFF) * sizeof(BattleOpRegs) + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}
void func_8007DA1C(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 count = 0;
    s32 i = MAX_PARTY_MEMBERS;
    s16 last;
    u16 *pRow = g_BattleSlotRegs[arg1 & 0xFF].wordRegs;
    s32 rowIndex;
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    while (i < BATTLE_ACTOR_SLOT_COUNT) {
        u8 idx = i;
        u8 idle = BattleSlotFlagsC002ClearRecordIdle(idx, 0);

        i++;
        if (idle != 0) {
            if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    }
    last = count - 1;
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, last)]);
        u16 *pOut = g_BattleSlotRegs[rowIndex & 0xFF].wordRegs;

        pOut[(*arg0)[2]] = value;
    }
}


void BattlePickPartySlotFromOperatorWordWithFlag(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s16 last;
    s32 count = 0;
    u16 *pRow = (u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * sizeof(BattleOpRegs));
    s32 rowIndex;
    u16 *pField;
    s32 offset = 0;
    s32 rowMask;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    rowMask = 0xFF;
    for (; i < MAX_PARTY_MEMBERS; offset += sizeof(BattleSlotFlag)) {
        u8 idx = i;
        u8 idle = BattleSlotFlagsC002ClearRecordIdle(idx, 0);

        i++;
        if (idle != 0) {
            if (((u8 *)g_BattleSlotFlags)[offset] != 0) {
                if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
    }
    last = count - 1;
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, last)]);
        u16 *pOut = (u16 *)(g_BattleSlotWordRegs + (rowIndex & rowMask) * sizeof(BattleOpRegs));

        pOut[(*arg0)[2]] = value;
    }
}


void BattlePickSlotFromOperatorWordFlagged(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = MAX_PARTY_MEMBERS;
    s16 last;
    s32 count = 0;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * sizeof(BattleOpRegs) + (s32)g_BattleSlotWordRegs);
    s32 rowIndex;
    u16 *pField;
    s32 offset = MAX_PARTY_MEMBERS * sizeof(BattleSlotFlag);
    s32 rowMask;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    rowMask = 0xFF;
    for (; i < BATTLE_ACTOR_SLOT_COUNT; offset += sizeof(BattleSlotFlag)) {
        u8 idx = i;
        u8 idle = BattleSlotFlagsC002ClearRecordIdle(idx, 0);

        i++;
        if (idle != 0) {
            if (((u8 *)g_BattleSlotFlags)[offset] != 0) {
                if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
    }
    last = count - 1;
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, last)]);
        u16 *pOut = (u16 *)((rowIndex & rowMask) * sizeof(BattleOpRegs) + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}

void func_8007DE78(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s16 last;
    s32 count = 0;
    u16 *pRow = g_BattleSlotRegs[arg1 & 0xFF].wordRegs;
    s32 rowIndex;
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    while (i < MAX_PARTY_MEMBERS) {
        u8 idx = i;
        u8 idle = BattleSlotFlagsC002ClearRecordIdle(idx, 1);

        i++;
        if (idle != 0) {
            if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    }
    last = count - 1;
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, last)]);
        u16 *pOut = g_BattleSlotRegs[rowIndex & 0xFF].wordRegs;

        pOut[(*arg0)[2]] = value;
    }
}


void BattlePickPartySlotFromOperatorWordWithFlagTag1(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s16 last;
    s32 count = 0;
    u16 *pRow = (u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * sizeof(BattleOpRegs));
    s32 rowIndex;
    u16 *pField;
    s32 offset = 0;
    s32 rowMask;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    rowMask = 0xFF;
    for (; i < MAX_PARTY_MEMBERS; offset += sizeof(BattleSlotFlag)) {
        u8 idx = i;
        u8 idle = BattleSlotFlagsC002ClearRecordIdle(idx, 1);

        i++;
        if (idle != 0) {
            if (((u8 *)g_BattleSlotFlags)[offset] != 0) {
                if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
    }
    last = count - 1;
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, last)]);
        u16 *pOut = (u16 *)(g_BattleSlotWordRegs + (rowIndex & rowMask) * sizeof(BattleOpRegs));

        pOut[(*arg0)[2]] = value;
    }
}

void BattleSetSlotRecordByteAndRelease(BattleCommand **ppCmd, s32 arg1) {
    s32 idx = (arg1 & 0xFF) + 3;

    g_BattleSlotRecords[idx].flags = (*ppCmd)->operand1;
    if ((*ppCmd)->operand1 & 0x80) {
        func_80087EDC((u8)(arg1 + 3), (u8)(arg1 + 3));
    } else {
        BattleReleaseCharSlot((u8)(arg1 + 3));
    }
}
void BattleWordRegSetSlotBitMask(BattleCommand **ppCmd, s32 arg1) {
    u16 value = BattleSlotBitMask((arg1 + 3) & 0xFF);
    u16 *pRegs = g_BattleSlotRegs[arg1 & 0xFF].wordRegs;

    pRegs[(*ppCmd)->operand1] = value;
}
void BattleStoreCharSlotFlagMask(BattleCommand **ppCmd, u8 arg1) {
    s32 acc = 0;
    s32 i;
    u16 *pRegs;

    // the slot table is walked downwards from its third record
    for (i = 2; i >= 0; i--) {
        if ((u8)BattleSlotFlagsC000Clear((u8)i) != 0) {
            switch ((*ppCmd)->operand2) {
            case BATTLE_SLOT_TEST_IN_USE:
                if (g_BattleSlotFlags[i].inUse != 0) {
                    acc |= 1;
                }
                break;
            case BATTLE_SLOT_TEST_NOT_IN_USE:
                if (g_BattleSlotFlags[i].inUse == 0) {
                    acc |= 1;
                }
                break;
            default:
                acc |= 1;
                break;
            }
        }
        acc <<= 1;
    }
    pRegs = g_BattleSlotRegs[arg1].wordRegs;
    pRegs[(*ppCmd)->operand1] = (u16)acc >> 1;
}
