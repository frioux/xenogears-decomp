#include "common.h"
#include "main/game.h"

// Argument block the slot-picking functions are handed: at +1 the slot to store
// into the actor's row, at +2 the tag to match.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 slot;
    /* 0x2 */ u8 tag;
} BattleSlotArgs; // Size: 0x3

// Per-actor 0x40-byte work row: a dword, a word and a byte field per slot.
typedef struct {
    /* 0x00 */ u32 unk00[4];
    /* 0x10 */ u16 unk10[8];
    /* 0x20 */ u8 unk20[0x20];
} BattleSlotRow; // Size: 0x40

// One of the 0x170-byte battle slot entries: ranking word at +0, tag byte at
// +0xA, flag word at +0x30, dword ranking value at +0xB8.
typedef struct {
    /* 0x000 */ u16 unk000;
    /* 0x002 */ u8 unk002[8];
    /* 0x00A */ u8 tag;
    /* 0x00B */ u8 unk00B[0x25];
    /* 0x030 */ u16 flags;
    /* 0x032 */ u8 unk032[0x86];
    /* 0x0B8 */ u32 unk0B8;
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
extern u8 D_800D2CB0[];
extern u8 g_BattleCommandGridIds[];
extern u8 g_BattleSlotEnabledFlags[];
extern u8 D_800CCD3E[];
extern u8 D_800CCDEC[];
extern u8 D_800C3EB7[];
extern BattleSlotFlag g_BattleSlotFlags[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u8 *g_BattleSlotParamTable;
extern u8 BattleSlotFlagsC002ClearRecordIdle(u8 arg0, u8 arg1);
extern u16 BattleSlotBitMask(u8 arg0);
extern u8 BattleFindFirstFlagTableMatch(u16 arg0);
extern u8 BattleSlotFlagsC000Clear(u8 arg0);
extern u8 BattleSlotFlagsC002Clear(u8 arg0, u8 arg1);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C1A4);
// best: 91.52% (396 bytes vs retail 392). Re-measured after fixing the tag
// compares so the body compiles: the old 83.91% cannot have come from a building
// body, because `g_BattleSlotRecords[a] != g_BattleSlotRecords[b]` on a struct
// array is not valid C - cc1 exited and diff_func diffed a stale object.
// The tags are read with the SCALED struct index (retail emits the x28 chain for
// both sides and hoists the invariant (arg1 + 3) * 0x1C into s4), so the source
// is `g_BattleSlotRecords[arg1 + 3].tag` / `g_BattleSlotRecords[idx].tag`, not a
// 0x1C byte offset.
// What is left: 4 bytes over; retail re-forms `lui/addiu D_800D3420` inside the
// if-block for the tail store instead of reusing the preheader base, and keeps
// arg1 * 0x40 in a2 and copies it to s5 at the loop preheader.
/*
void func_8007C1A4(u8 **arg0, u8 arg1) {
    u8 buf[3];
    s32 row = arg1 * 0x40;
    u16 *pRow = (u16 *)(D_800D3420 + row);

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    do {
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*arg0)[2]) != 0
                && g_BattleSlotRecords[(arg1 + 3) * 0x1C] != g_BattleSlotRecords[idx * 0x1C]
                && g_BattleSlotFlags[idx].inUse == 0) {
                u16 result = BattleSlotBitMask(idx);
                s32 index = (*arg0)[1];
                u16 *pTail = (u16 *)(D_800D3420 + row);

                pTail[index] = result;
                break;
            }
            buf[idx] = 1;
        }
    }
    } while (0);
}
*/
void BattlePickActorSlotWithDifferentTag(u8 **arg0, u8 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);

    pField[(*arg0)[1]] = 0;
    for (; i < 0xB; i++) {
        u8 idx = i;

        if (BattleSlotFlagsC002ClearRecordIdle(idx, (*arg0)[2]) != 0
            && g_BattleSlotRecords[arg1 + 3].tag != g_BattleSlotRecords[i].tag
            && g_BattleSlotFlags[i].inUse == 0) {
            buf[count++] = idx;
        }
    }
    if (count != 0) {
        u16 result = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        s32 index = (*arg0)[1];
        u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

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
    pRow = g_BattleSlotRows[row].unk10;
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
    pRow = g_BattleSlotRows[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickCharSlotByLowestEntryWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && best >= g_BattleSlotEntriesBase[i].unk000) {
            best = g_BattleSlotEntriesBase[i].unk000;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickActorSlotByLowestEntryWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && best >= g_BattleSlotEntriesBase[i].unk000) {
            best = g_BattleSlotEntriesBase[i].unk000;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C840);
// best: 91.44% (392 bytes, retail's exact size). Re-measured after fixing the
// tag compares so the body compiles - the old 82.94% came from a body comparing
// two BattleSlotRecord structs with ==, which is not valid C, so cc1 exited and
// diff_func diffed a stale object. As in func_8007C1A4 the tags are read with
// the SCALED struct index: `g_BattleSlotRecords[arg1 + 3].tag`.
// What is left: retail keeps the invariant offset in s4 and re-forms
// `lui at,%hi + addu at,at,s4 + lbu %lo(at)` per read, and its tail store puts
// the base and the index in the opposite registers from ours.
/*
void func_8007C840(u8 **arg0, u8 arg1) {
    u8 buf[3];
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    do {
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*arg0)[2]) != 0 && g_BattleSlotFlags[idx].inUse != 0
                && g_BattleSlotRecords[(arg1 + 3) * 0x1C] == g_BattleSlotRecords[idx * 0x1C]) {
                u16 result = BattleSlotBitMask(idx);
                s32 index = (*arg0)[1];
                u16 *pTail = (u16 *)(D_800D3420 + arg1 * 0x40);

                pTail[index] = result;
                break;
            }
            buf[idx] = 1;
        }
    }
    } while (0);
}
*/
void func_8007C9D4(u8 **arg0, u8 arg1) {
    u8 buf[3];
    u16 *pRow = g_BattleSlotRows[arg1].unk10;

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;
        u8 tried = buf[idx];
        /* a second live copy: retail keeps the row pointer in a callee-saved reg
           across the calls and re-copies it here, so one variable does not link */
        u16 *pLoopRow = g_BattleSlotRows[arg1].unk10;

        if (tried == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*arg0)[2]) != 0 && g_BattleSlotFlags[idx].inUse != 0) {
                pLoopRow[(*arg0)[1]] = BattleSlotBitMask(idx);
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
    u16 *pField = g_BattleSlotRows[row].unk10;

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
        u16 *pRow = g_BattleSlotRows[row].unk10;

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
    g_BattleSlotRows[row].unk20[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, 0) != 0 && g_BattleSlotRecords[i].tag == (*ppArgs)->tag) {
            count++;
        }
    }
    g_BattleSlotRows[row].unk20[(*ppArgs)->slot] = count;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CDD0);
// best: 99.43% (212 bytes = retail's exact size). Identical instruction stream
// except one register swap: retail accumulates the a*8 term into its own register
// (`addu v1,v1,v0`) and then loads g_BattleSlotParamTable into v0; ours puts the
// index in v0 and the table base in v1. Hand shapes tried (all 95.75-99.43%):
// the constant order swapped, an s32 index local (initialiser and += forms), the
// table pointer hoisted to a local, `(op << 3) + (slotTag << 6)`, and the two tag
// locals declared in the opposite order. Permuter (4 min, 2167 iterations) found
// nothing: base=30 best=30. The tag reads are the typed form
// `g_BattleSlotRecords[i].tag`. The two D_ address spellings are what the
// link-identical forms would be (g_BattleSlotRows[row].unk00 / a 0x10 byte view).
/*
void func_8007CDD0(u8 **arg0, u8 arg1) {
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    u8 value = BattleFindFirstFlagTableMatch(pField[(*arg0)[2]]);
    u8 *pRow = (u8 *)(D_800D3420 + 0x10) + arg1 * 0x40;
    u8 op = g_BattleSlotRecords[value].tag;
    s32 opOffset = op * 8;
    u8 slotTag = g_BattleSlotRecords[arg1 + 3].tag;

    pRow[(*arg0)[1]] = g_BattleSlotParamTable[opOffset + slotTag * 0x40 + 0x140];
}
*/

void BattleStoreCharSlotMatchedTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i = 0;
    u16 *pField = g_BattleSlotRows[row].unk10;

    for (; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, 0) != 0) {
            u8 other = BattleFindFirstFlagTableMatch(pField[(*ppArgs)->tag]);

            if (g_BattleSlotRecords[i].tag == g_BattleSlotRecords[other].tag) {
                count++;
            }
        }
    }
    g_BattleSlotRows[row].unk20[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotMatchedTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i = 3;
    u16 *pField = g_BattleSlotRows[row].unk10;

    for (; i < 0xB; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, 0) != 0) {
            u8 other = BattleFindFirstFlagTableMatch(pField[(*ppArgs)->tag]);

            if (g_BattleSlotRecords[i].tag == g_BattleSlotRecords[other].tag) {
                count++;
            }
        }
    }
    g_BattleSlotRows[row].unk20[(*ppArgs)->slot] = count;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D0CC);
/* best: 85.97% (identical instruction multiset, 124 bytes = retail's size).
   Retail orders the block [sll a1][lui/addiu D_800D3430][lw *arg0][addu base a1]
   [lbu idx][move a3,zero][addu addr][sb]; gcc hoists `i = 0` to the very top and
   interleaves the two address chains the other way round. Tried: i = 0 between
   the store and the loop, the inline *(D_800D3430 + arg1*0x40 + (*arg0)[1]) form
   (folds into $at, frame 8), an index local. permute.sh ran once from this body
   (1414 iterations): its best candidate (45 vs 480) reassigns pRow = *arg0 in the
   loop, which changes what the tail store writes - wrong code, not portable.
   This is a scheduler-priority tie.

void func_8007D0CC(u8 **arg0, u8 arg1) {
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 i;

    pRow[(*arg0)[1]] = 0;
    for (i = 0; i < 0x30; i++) {
        if (g_BattleCommandGridIds[i] == (*arg0)[2]) {
            pRow[(*arg0)[1]] = D_800D2CB0[i];
            break;
        }
    }
}
*/
void BattleWriteRowFieldBytes(BattleSlotArgs **ppArgs, u8 *pDst, u8 row, u8 index) {
    u16 value = g_BattleSlotRows[row].unk10[(*ppArgs)->tag];

    pDst[index * 8 + (*ppArgs)->slot] = value;
    *(pDst + (index * 8 + (*ppArgs)->slot) + 1) = value >> 8;
}
void BattleStoreGoldWord32(BattleSlotArgs **ppArgs, u8 row) {
    g_BattleSlotRows[row].unk00[(*ppArgs)->slot] = g_GameState.gold;
}
void BattlePickSlotByActorTag(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[0x10];
    s32 i = 0;
    s32 count = 0;
    s32 record = 0;
    u8 *pBuf = buf;
    u16 *pField = g_BattleSlotRows[row].unk10;

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
        u16 *pRow = g_BattleSlotRows[row].unk10;

        pRow[index] = result;
    }
}
void BattleStoreStatWord32(BattleSlotArgs **ppArgs, u8 row) {
    g_BattleSlotRows[row].unk00[(*ppArgs)->slot] = g_BattleRowStatValues[row];
}
void BattlePickActorSlotWithFlag80(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 record = 0x54;
    u8 *pBuf = buf;
    u16 *pField = g_BattleSlotRows[row].unk10;

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
        u16 *pRow = g_BattleSlotRows[row].unk10;

        pRow[index] = result;
    }
}
// best: 98.33% (308 bytes vs retail 312). The only missing instruction is
// `move s2,a1` at loop entry - the same single-instruction gap, on the same
// preheader, as func_8007C9D4: retail forms the row base in caller-saved a1,
// uses it for the zeroing store, then copies it into s2 after the initial
// three-byte loop test. Re-measured after fixing the body to compile: the old
// 98.46% body read `g_BattleSlotEntries[idx].flags`, and that symbol was never
// declared in this unit. The field is the u16 at +0 of the 0x170-byte entry, so
// g_BattleSlotEntriesBase[idx].flags is the same address and the identical code (98.33%).
// The duplicated `if (arg0)`, whose two arms are identical, is what puts arg0 in
// s1 and the base in s2 the way retail has them; removing it drops the score.
/*
void func_8007D478(u8 **arg0, u8 arg1) {
    u8 buf[3];
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    u32 flags;

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    do {
        while ((buf[0] & buf[1] & buf[2]) == 0) {
            s32 slot = GameRandomByteRange(0, 2);
            u8 idx = slot;

            if (buf[idx] == 0) {
                flags = g_BattleSlotEntries[idx].flags;
                if ((flags & 0x8000) && (flags & 0x4002) == 0) {
                    if (arg0) {
                        pRow[(*arg0)[1]] = BattleSlotBitMask(idx);
                        break;
                    } else {
                        pRow[(*arg0)[1]] = BattleSlotBitMask(idx);
                        break;
                    }
                }
                buf[idx] = 1;
            }
        }
    } while (0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D478);
void BattleStoreCharSlotFlagsC000ClearCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((g_BattleSlotEntriesBase[i].flags & 0xC000) == 0) {
            count++;
        }
    }
    g_BattleSlotRows[row].unk20[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotFlagsC000ClearCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (g_BattleSlotEnabledFlags[i] != 0 && (g_BattleSlotEntriesBase[i].flags & 0xC000) == 0 && g_BattleSlotRecords[i].flags == 0) {
            count++;
        }
    }
    g_BattleSlotRows[row].unk20[(*ppArgs)->slot] = count;
}
void BattlePickCharSlotByLowestEntryDwordInUse(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (BattleSlotFlagsC002ClearRecordIdle(i, (*ppArgs)->tag) != 0 && g_BattleSlotFlags[i].inUse != 0
            && best >= g_BattleSlotEntriesBase[i].unk0B8) {
            best = g_BattleSlotEntriesBase[i].unk0B8;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].unk10;
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
            && best >= g_BattleSlotEntriesBase[i].unk000) {
            best = g_BattleSlotEntriesBase[i].unk000;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = g_BattleSlotRows[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
