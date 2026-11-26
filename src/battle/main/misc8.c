#include "common.h"
#include "main/game.h"

// The battle script command these functions are handed (the pointer
// BattleExecScriptOpcode switched on): the opcode at +0, then the slot to store
// into the actor's row at +1 and the tag to match at +2.
typedef struct {
    /* 0x0 */ u8 opcode;
    /* 0x1 */ u8 slot;
    /* 0x2 */ u8 tag;
} BattleSlotArgs; // Size: 0x3

// A slot's battle register file seen from its dword registers (g_BattleSlotRows is
// g_BattleSlotRegs + 0x10): the dwords, words and bytes the script's ops store into,
// indexed by the operand byte.
typedef struct {
    /* 0x00 */ u32 dwordRegs[4];
    /* 0x10 */ u16 wordRegs[8];
    /* 0x20 */ u8 byteRegs[0x10];
    /* 0x30 */ u8 pad30[0x10]; // the head of the next slot's record, untouched here
} BattleSlotRow; // Size: 0x40

// One of the 0x170-byte battle slot entries, seen from g_BattleSlotEntriesBase (the
// entry 0x4C in): the character record's hp at +0 and the state block's scaled hp at
// +0xB8 are the two words the slot pickers minimise. The same bytes are
// BattleTableEntry and its BattleSlotState (at +0x58 here) in include/battle/actor.h,
// whose base is g_BattleWorkBlock; the spans left unk below are its unk57/unk61/unk90.
typedef struct {
    /* 0x000 */ u16 hp;
    /* 0x002 */ u16 maxHp; // BattleResetActorStatDisplay copies it over hp
    /* 0x004 */ u16 mp;
    /* 0x006 */ u16 maxMp;
    /* 0x008 */ u8 unk8[2];
    /* 0x00A */ u8 tag;
    /* 0x00B */ u8 unkB[0x23];
    /* 0x02E */ u16 conditionFlags; // g_BattleSlotStatusRows is this word's own base
    /* 0x030 */ u16 flags;
    /* 0x032 */ u8 unk32[0x22];
    /* 0x054 */ u8 gearId; // the gear the slot's character has equipped
    /* 0x055 */ u8 unk55[0x63];
    /* 0x0B8 */ u32 scaledHp;
    /* 0x0BC */ u32 scaledMaxHp; // written with scaledHp by BattleSlotArgsStoreDwordFromReg, the pair func_80072F38 takes ratios of
    /* 0x0C0 */ u8 unkC0[0xB0];
} BattleSlotEntry; // Size: 0x170

// Per-slot "can be picked" flag the slot-picking loops gate on, one 8-byte
// record per battle slot.
typedef struct {
    /* 0x0 */ u8 inUse;
    /* 0x1 */ u8 unk1[7];
} BattleSlotFlag; // Size: 0x8

// Per-slot tag record: tag at +0, flag byte at +3, the slot's position at +0xA
// (D_800C3EBE is this +0xA, declared as the x/y row misc12.c reads).
typedef struct {
    /* 0x0 */ u8 tag;
    /* 0x1 */ u8 unk1[2];
    /* 0x3 */ u8 flags;
    /* 0x4 */ u8 unk4[6];
    /* 0xA */ u16 x;
    /* 0xC */ u16 y;
    /* 0xE */ u8 unkE[0xE];
} BattleSlotRecord; // Size: 0x1C

extern BattleSlotRow g_BattleSlotRows[];
extern BattleSlotEntry g_BattleSlotEntriesBase[];
extern BattleSlotRecord g_BattleSlotRecords[];
extern s16 g_BattleSlotCountdownTimers[];
extern s16 D_800D2E0C[];
extern u8 g_BattleRowStatValues[];
extern u8 g_BattleCommandCellUses[];
extern u8 g_BattleCommandGridIds[];
extern u8 g_BattleSlotEnabledFlags[];
extern u8 g_BattleSlotBank14TextRows[];
extern u8 D_800CCDEC[];
extern u8 D_800C3EB7[];
extern BattleSlotFlag g_BattleSlotFlags[];
extern u8 *g_BattleSlotParamTable;
extern u8 BattleSlotFlagsC002ClearRecordIdle(u8 arg0, u8 arg1);
extern u16 BattleSlotBitMask(u8 arg0);
extern u8 BattleFindFirstFlagTableMatch(u16 arg0);
extern u8 BattleSlotFlagsC000Clear(u8 arg0);
extern u8 BattleSlotFlagsC002Clear(u8 arg0, u8 arg1);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);

void BattlePickCharSlotWithDifferentTag(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[3];

    buf[0] = buf[1] = buf[2] = 0;
    g_BattleSlotRows[row].wordRegs[(*ppArgs)->slot] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*ppArgs)->tag) != 0
                && g_BattleSlotRecords[row + 3].tag != g_BattleSlotRecords[idx].tag
                && g_BattleSlotFlags[idx].inUse == 0) {
                g_BattleSlotRows[row].wordRegs[(*ppArgs)->slot] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
void BattlePickActorSlotWithDifferentTag(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    u16 *pField = g_BattleSlotRows[row].wordRegs;

    pField[(*ppArgs)->slot] = 0;
    for (; i < 0xB; i++) {
        u8 idx = i;

        if (BattleSlotFlagsC002ClearRecordIdle(idx, (*ppArgs)->tag) != 0
            && g_BattleSlotRecords[row + 3].tag != g_BattleSlotRecords[i].tag
            && g_BattleSlotFlags[i].inUse == 0) {
            buf[count++] = idx;
        }
    }
    if (count != 0) {
        u16 result = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        s32 index = (*ppArgs)->slot;
        u16 *pRow = g_BattleSlotRows[row].wordRegs;

        pRow[index] = result;
    }
}
void BattlePickCharSlotByLowestSlotWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u8 best = 0xFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && best >= g_BattleSlotCountdownTimers[i]) {
            best = g_BattleSlotCountdownTimers[i];
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].wordRegs;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickActorSlotByLowestSlotWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u8 best = 0xFF;
    s32 bestIndex = 0;
    s32 i = 3;
    s32 actorSlot = row + 3;

    for (; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && best >= D_800D2E0C[i - 3] && i != actorSlot) {
            best = D_800D2E0C[i - 3];
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].wordRegs;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickCharSlotByLowestEntryWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && best >= g_BattleSlotEntriesBase[i].hp) {
            best = g_BattleSlotEntriesBase[i].hp;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].wordRegs;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickActorSlotByLowestEntryWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && best >= g_BattleSlotEntriesBase[i].hp) {
            best = g_BattleSlotEntriesBase[i].hp;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].wordRegs;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickCharSlotWithSameTag(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[3];

    buf[0] = buf[1] = buf[2] = 0;
    g_BattleSlotRows[row].wordRegs[(*ppArgs)->slot] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*ppArgs)->tag) != 0 && g_BattleSlotFlags[idx].inUse != 0
                && g_BattleSlotRecords[row + 3].tag == g_BattleSlotRecords[idx].tag) {
                g_BattleSlotRows[row].wordRegs[(*ppArgs)->slot] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
void BattlePickCharSlotByFlagRetry(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[3];
    u16 *pRow = g_BattleSlotRows[row].wordRegs;

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*ppArgs)->slot] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;
        u8 tried = buf[idx];
        /* a second live copy: retail keeps the row pointer in a callee-saved reg
           across the calls and re-copies it here, so one variable does not link */
        u16 *pLoopRow = g_BattleSlotRows[row].wordRegs;

        if (tried == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*ppArgs)->tag) != 0 && g_BattleSlotFlags[idx].inUse != 0) {
                pLoopRow[(*ppArgs)->slot] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
void BattlePickSlotByFlagTable(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 record = 0x18;
    u8 *pBuf = buf;
    u16 *pField = g_BattleSlotRows[row].wordRegs;

    pField[(*ppArgs)->slot] = 0;
    for (; i < 0xB; i++, record += 8) {
        u8 idx = i;

        if (BattleSlotFlagsC002ClearRecordIdle(idx, (*ppArgs)->tag) != 0 && ((u8 *)g_BattleSlotFlags)[record] != 0) {
            *pBuf++ = idx;
            count++;
        }
    }
    if (count != 0) {
        u16 result = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        s32 index = (*ppArgs)->slot;
        u16 *pRow = g_BattleSlotRows[row].wordRegs;

        pRow[index] = result;
    }
}
void BattleStoreCharSlotTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, 0) != 0 && g_BattleSlotRecords[i].tag == (*ppArgs)->tag) {
            count++;
        }
    }
    g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, 0) != 0 && g_BattleSlotRecords[i].tag == (*ppArgs)->tag) {
            count++;
        }
    }
    g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = count;
}
void BattleStoreSlotParamTableByte(u8 **ppCommand, u8 slot) {
    u16 *pWordRegs = g_BattleSlotRows[slot].wordRegs;
    u8 *pByteRegs;
    u8 record;
    u8 op;
    u8 slotTag;
    s32 opOffset;

    record = BattleFindFirstFlagTableMatch(pWordRegs[(*ppCommand)[2]]);
    pByteRegs = g_BattleSlotRows[slot].byteRegs;
    op = g_BattleSlotRecords[record].tag;
    slotTag = g_BattleSlotRecords[slot + 3].tag;
    opOffset = op * 8;
    // the op reassignment is load-bearing: it is what makes gcc emit retail's
    // operand order for the 0x40 slot stride and the table base addition
    pByteRegs[(*ppCommand)[1]] = g_BattleSlotParamTable[(opOffset + (slotTag * (op = 0x40))) + 0x140];
}

void BattleStoreCharSlotMatchedTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i = 0;
    u16 *pField = g_BattleSlotRows[row].wordRegs;

    for (; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, 0) != 0) {
            u8 other = BattleFindFirstFlagTableMatch(pField[(*ppArgs)->tag]);

            if (g_BattleSlotRecords[i].tag == g_BattleSlotRecords[other].tag) {
                count++;
            }
        }
    }
    g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotMatchedTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i = 3;
    u16 *pField = g_BattleSlotRows[row].wordRegs;

    for (; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, 0) != 0) {
            u8 other = BattleFindFirstFlagTableMatch(pField[(*ppArgs)->tag]);

            if (g_BattleSlotRecords[i].tag == g_BattleSlotRecords[other].tag) {
                count++;
            }
        }
    }
    g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = count;
}
void BattleStoreCommandUseCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 i;

    g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = 0;
    for (i = 0; i < 0x30; i++) {
        if (g_BattleCommandGridIds[i] == (*ppArgs)->tag) {
            g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = g_BattleCommandCellUses[i];
            break;
        }
    }
}
void BattleWriteRowFieldBytes(BattleSlotArgs **ppArgs, u8 *pDst, u8 row, u8 index) {
    u16 value = g_BattleSlotRows[row].wordRegs[(*ppArgs)->tag];

    pDst[index * 8 + (*ppArgs)->slot] = value;
    *(pDst + (index * 8 + (*ppArgs)->slot) + 1) = value >> 8;
}
void BattleStoreGoldWord32(BattleSlotArgs **ppArgs, u8 row) {
    g_BattleSlotRows[row].dwordRegs[(*ppArgs)->slot] = g_GameState.gold;
}
void BattlePickSlotByActorTag(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[0x10];
    s32 i = 0;
    s32 count = 0;
    s32 record = 0;
    u8 *pBuf = buf;
    u16 *pField = g_BattleSlotRows[row].wordRegs;

    pField[(*ppArgs)->slot] = 0;
    for (; i < 0xB; i++, record += 0x170) {
        u8 idx = i;

        if (BattleSlotFlagsC000Clear(idx) != 0 && g_BattleSlotBank14TextRows[record] == (*ppArgs)->tag) {
            *pBuf++ = idx;
            count++;
        }
    }
    if (count != 0) {
        u16 result = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        s32 index = (*ppArgs)->slot;
        u16 *pRow = g_BattleSlotRows[row].wordRegs;

        pRow[index] = result;
    }
}
void BattleStoreStatWord32(BattleSlotArgs **ppArgs, u8 row) {
    g_BattleSlotRows[row].dwordRegs[(*ppArgs)->slot] = g_BattleRowStatValues[row];
}
void BattlePickActorSlotWithFlag80(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 record = 0x54;
    u8 *pBuf = buf;
    u16 *pField = g_BattleSlotRows[row].wordRegs;

    pField[(*ppArgs)->slot] = 0;
    for (; i < 0xB; i++, record += 0x1C) {
        u8 idx = i;

        if (BattleSlotFlagsC002Clear(idx, (*ppArgs)->tag) != 0 && (D_800C3EB7[record] & 0x80) != 0) {
            *pBuf++ = idx;
            count++;
        }
    }
    if (count != 0) {
        u16 result = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        s32 index = (*ppArgs)->slot;
        u16 *pRow = g_BattleSlotRows[row].wordRegs;

        pRow[index] = result;
    }
}
void BattlePickCharSlotByEntryFlags(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[3];
    u16 *pRow = g_BattleSlotRows[row].wordRegs;

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*ppArgs)->slot] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;
        u8 tried = buf[idx];
        /* second live copy of the row pointer, as in BattlePickCharSlotByFlagRetry */
        u16 *pLoopRow = g_BattleSlotRows[row].wordRegs;
        u32 flags;

        if (tried == 0) {
            flags = g_BattleSlotEntriesBase[idx].flags;
            if ((flags & 0x8000) && (flags & 0x4002) == 0) {
                pLoopRow[(*ppArgs)->slot] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
void BattleStoreCharSlotFlagsC000ClearCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((g_BattleSlotEntriesBase[i].flags & 0xC000) == 0) {
            count++;
        }
    }
    g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotFlagsC000ClearCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (g_BattleSlotEnabledFlags[i] != 0 && (g_BattleSlotEntriesBase[i].flags & 0xC000) == 0 && g_BattleSlotRecords[i].flags == 0) {
            count++;
        }
    }
    g_BattleSlotRows[row].byteRegs[(*ppArgs)->slot] = count;
}
void BattlePickCharSlotByLowestEntryDwordInUse(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && g_BattleSlotFlags[i].inUse != 0
            && best >= g_BattleSlotEntriesBase[i].scaledHp) {
            best = g_BattleSlotEntriesBase[i].scaledHp;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].wordRegs;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickActorSlotByLowestEntryWordInUse(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && g_BattleSlotFlags[i].inUse != 0
            && best >= g_BattleSlotEntriesBase[i].hp) {
            best = g_BattleSlotEntriesBase[i].hp;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].wordRegs;
    pRow[(*ppArgs)->slot] = result;
}
