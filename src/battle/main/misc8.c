#include "common.h"
#include "main/game.h"

// Argument block the slot-picking functions are handed: at +1 the slot to store
// into the actor's row, at +2 the tag to match.
typedef struct {
    /* 0x0 */ u8 unk0;
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
// +0xB8 are the two words the slot pickers minimise.
typedef struct {
    /* 0x000 */ u16 hp;
    /* 0x002 */ u8 unk002[8];
    /* 0x00A */ u8 tag;
    /* 0x00B */ u8 unk00B[0x25];
    /* 0x030 */ u16 flags;
    /* 0x032 */ u8 unk032[0x86];
    /* 0x0B8 */ u32 scaledHp;
    /* 0x0BC */ u8 unk0BC[0xB4];
} BattleSlotEntry; // Size: 0x170

// Per-slot "can be picked" flag the slot-picking loops gate on, one 8-byte
// record per battle slot.
typedef struct {
    /* 0x0 */ u8 inUse;
    /* 0x1 */ u8 unk1[7];
} BattleSlotFlag; // Size: 0x8

// Per-slot tag record: tag at +0, flag byte at +3.
typedef struct {
    /* 0x0 */ u8 tag;
    /* 0x1 */ u8 unk1[2];
    /* 0x3 */ u8 flags;
    /* 0x4 */ u8 unk4[0x18];
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
extern u8 D_800CCD3E[];
extern u8 D_800CCDEC[];
extern u8 D_800C3EB7[];
extern BattleSlotFlag g_BattleSlotFlags[];
extern u8 D_800D3420[];
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
void BattleStoreSlotParamTableByte(u8 **arg0, u8 arg1) {
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    u8 value = BattleFindFirstFlagTableMatch(pField[(*arg0)[2]]);
    u8 *pRow = (u8 *)(D_800D3420 + 0x10) + arg1 * 0x40;
    u8 op = g_BattleSlotRecords[value].tag;
    s32 opOffset = op * 8;
    u8 slotTag = g_BattleSlotRecords[arg1 + 3].tag;

    pRow[(*arg0)[1]] = g_BattleSlotParamTable[(opOffset + (slotTag * (op = 0x40))) + 0x140];
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

        if (BattleSlotFlagsC000Clear(idx) != 0 && D_800CCD3E[record] == (*ppArgs)->tag) {
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
