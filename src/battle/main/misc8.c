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

extern BattleSlotRow D_800D3410[];
extern BattleSlotEntry D_800CCD34[];
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
extern u8 func_8007A628(u8 arg0, u8 arg1);
extern u16 BattleSlotBitMask(u8 arg0);
extern u8 BattleFindFirstFlagTableMatch(u16 arg0);
extern u8 BattleSlotFlagsC000Clear(u8 arg0);
extern u8 BattleSlotFlagsC002Clear(u8 arg0, u8 arg1);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C1A4);
/* best: 83.91% (408 bytes vs retail 392). Structure is certain (same body as
   func_8007C9D4 plus the g_BattleSlotRecords[actor slot tag] reject test): buf[0..2]
   zeroed, row word zeroed, `while ((buf[0] & buf[1] & buf[2]) == 0)` picking a
   random 0..2 entry, three tests, then the row word = bitmask and break.
   Two blockers, both about how gcc 2.6 treats the g_BattleSlotRecords tag reads:
   (1) retail keeps the invariant offset (arg1 + 3) * 0x1C in s4 and issues
   each tag read as lui at/%hi + addu at,at,reg + lbu %lo(at) - the
   single-use lo_sum form BattleTableEntryFlag8000 also has. gcc instead
   hoists g_BattleSlotRecords + offset into s5 AND materialises the base into s6
   (cse sees the symbol twice), so the frame carries one extra saved register
   and the load pair becomes `addu v0,v0,s6` + two 0(reg) loads.
   (2) retail computes arg1 * 0x40 into a2 right before the first store and
   copies it to s5 at the loop preheader (`move s5,a2`); a `s32 row` local is
   instead born in the prologue (sll s4,a1,0x6 sits above the frame stores).
   Tried: inline (arg1+3)*0x1C (83.91%, this), s32 record local for the
   invariant offset (78.65%), both row stores inlined (77.96%), an
   element-typed BattleSlotRecord cast so gcc generates the * 0x1C scaling,
   field indexing `[arg1 + 3].tag`/`[idx].tag`, and a plain u8* pRow for both
   stores. Also screened the preset: gcc 2.7.2 gives the same 83.91, cdk
   79.49, -O3 83.91 - so this is a source/cse question, not a compiler one.

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
            if (func_8007A628(idx, (*arg0)[2]) != 0
                && g_BattleSlotRecords[(arg1 + 3) * 0x1C] != g_BattleSlotRecords[idx * 0x1C]
                && g_BattleSlotFlags[idx * 8] == 0) {
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C33C);
/* best: 89.69% (364 bytes vs retail 356). The whole structure matches: the
   prologue builds (arg1 + 3) * 0x1C into s5, the "record" offset 0x54, the
   three-condition body, `buf[count++] = idx`, and the tail store with a local
   index + row pointer. Two remaining differences: (1) gcc strength-reduces
   g_BattleSlotFlags[i * 8] into an induction pointer (&g_BattleSlotFlags + 0x18 stored in a
   saved register) where retail keeps `sll a0,s0,3` and the at-form load, which
   also costs the extra saved register; (2) with that block shared by three
   predecessors retail keeps a nop in the `beqz` delay slot and increments i in
   the fall-through block, gcc steals the addiu into the slot.

void func_8007C33C(u8 **arg0, u8 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 slot = (arg1 + 3) * 0x1C;
    s32 record = 0x54;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);

    pField[(*arg0)[1]] = 0;
    for (; i < 0xB; i++, record += 0x1C) {
        u8 idx = i;

        if (func_8007A628(idx, (*arg0)[2]) != 0 && g_BattleSlotRecords[slot] != g_BattleSlotRecords[record]
            && g_BattleSlotFlags[i * 8] == 0) {
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
*/
void BattlePickCharSlotByLowestSlotWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u8 best = 0xFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (func_8007A628(i, (*ppArgs)->tag) != 0 && best >= g_BattleSlotCountdownTimers[i]) {
            best = g_BattleSlotCountdownTimers[i];
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = D_800D3410[row].unk10;
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
        if (func_8007A628(i, (*ppArgs)->tag) != 0 && best >= D_800D2E0C[i - 3] && i != actorSlot) {
            best = D_800D2E0C[i - 3];
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = D_800D3410[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickCharSlotByLowestEntryWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (func_8007A628(i, (*ppArgs)->tag) != 0 && best >= D_800CCD34[i].unk000) {
            best = D_800CCD34[i].unk000;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = D_800D3410[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickActorSlotByLowestEntryWord(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (func_8007A628(i, (*ppArgs)->tag) != 0 && best >= D_800CCD34[i].unk000) {
            best = D_800CCD34[i].unk000;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = D_800D3410[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C840);
/* best: 82.94% (408 bytes vs retail 396). Whole structure is right (pre-test
   rotation, the (arg1+3)*0x1C preheader value, s5 = arg1*0x40 reused by the
   tail store, the sh/andi/sb tail). Two blockers: (1) gcc hoists the invariant
   ADDRESS g_BattleSlotRecords + (arg1+3)*0x1C into s4 and keeps &g_BattleSlotRecords in an extra
   saved register s6 (frame 0x40 vs 0x38), where retail keeps the OFFSET in s4
   and re-forms `lui at/%hi + addu at,at,s4 + lbu %lo(at)` per use; (2) the tail
   store's base/index land in the opposite registers (v1/a0 vs a0/v1).
   Tried: the offset inline in the loop (this, 82.94%) and as a pre-loop s32
   `record` local (78.44%: the multiply stops being strength-reduced).

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
            if (func_8007A628(idx, (*arg0)[2]) != 0 && g_BattleSlotFlags[idx * 8] != 0
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C9D4);
/* best: 91.53% (340 bytes vs retail 336). Everything matches except two
   allocation details: (1) gcc computes the row base straight into s4
   (`addu s4,a1,v1`) where retail forms it in a caller-saved temp and copies it
   into s4 at the loop preheader (`addu a1,a1,v1` / `move s4,a1`); (2) gcc hoists
   `&g_BattleSlotFlags` into s5 (frame 0x38 vs 0x30) where retail re-forms it per use
   as lui at/%hi + addu at,at,v0 + lbu %lo(at).
   Tried: inline ((u16 *)(D_800D3420 + arg1 * 0x40))[(*arg0)[1]] for both stores
   (92.07%, 344 bytes: two copies of the base), for the zeroing store only
   (92.07%/344), and the plain u8 *pRow + pRow[i] form (this one).

void func_8007C9D4(u8 **arg0, u8 arg1) {
    u8 buf[3];
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    do {
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (func_8007A628(idx, (*arg0)[2]) != 0 && g_BattleSlotFlags[idx * 8] != 0) {
                pRow[(*arg0)[1]] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
    } while (0);
}
*/
void BattlePickSlotByFlagTable(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 record = 0x18;
    u8 *pBuf = buf;
    u16 *pField = D_800D3410[row].unk10;

    pField[(*ppArgs)->slot] = 0;
    for (; i < 0xB; i++, record += 8) {
        u8 idx = i;

        if (func_8007A628(idx, (*ppArgs)->tag) != 0 && ((u8 *)g_BattleSlotFlags)[record] != 0) {
            *pBuf++ = idx;
            count++;
        }
    }
    if (count != 0) {
        u16 result = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        s32 index = (*ppArgs)->slot;
        u16 *pRow = D_800D3410[row].unk10;

        pRow[index] = result;
    }
}
void BattleStoreCharSlotTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (func_8007A628(i, 0) != 0 && g_BattleSlotRecords[i].tag == (*ppArgs)->tag) {
            count++;
        }
    }
    D_800D3410[row].unk20[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (func_8007A628(i, 0) != 0 && g_BattleSlotRecords[i].tag == (*ppArgs)->tag) {
            count++;
        }
    }
    D_800D3410[row].unk20[(*ppArgs)->slot] = count;
}
// best: 99.21% (212 bytes = retail's exact size; everything matches except the
// frame (0x30 vs retail 0x28, two dead spill slots) and which of v0/v1 holds the
// table base and which the offset sum: retail `addu v1,v1,v0` then `addu
// v0,v0,v1`, mine the reverse pair). Permuter mischief 30 (was 40); two more
// 4-minute runs from this body both came back PERMUTER_NONE base=30 best=30, so
// what is left is not a tie the permuter can reach in minutes.
// Permuter edit ported: `s32 opOffset = a * 8;` as its own local, used by the
// store index (99.02 to 99.21). Temp-first vs row-first in the index compiles
// identically. What the earlier round established:
// at 99.02%:  (212 bytes = retail's exact size; every instruction matches
//    except the frame and one commutative addu):
//    - `u8 *pRow = (u8 *)(D_800D3420 + 0x10) + arg1 * 0x40;` is what reproduces
//      retail's `lui %hi(D_800D3420)` / `addiu %lo` / `addiu s0,s0,0x10` in the
//      jal delay slot plus `addu s2,s2,s0` after it. Spelling the row as
//      `D_800D3430 + ...` gives two separate symbol loads (78.26%) and
//      `(u8 *)pField + 0x10` makes gcc CSE the row base into one register (89.77%).
//    - `u8 a = g_BattleSlotRecords[value * 0x1C]; s32 slot = arg1 + 3;` in THAT order is
//      what puts `addiu s1,s1,3` between the two index chains, like retail.
//    - `g_BattleSlotParamTable[row * 0x40 + op * 8 + 0x140]` (row first) is what gets the two
//      slls in retail's order (99.02% vs 98.26%).
//    Remaining: (1) frame 0x30 vs retail 0x28 - two dead spill slots; the stack
//    offset shift proves they exist but no sw/lw to sp is emitted. Dropping one
//    live local (inlining the call result) removes the 8 bytes but costs 2
//    instructions (80.42%, 204 bytes). (2) mine puts the table base in v1 and the
//    sum in v0, retail the reverse (`addu v0,v0,v1` vs `addu v1,v1,v0`).
//    Also tried: pRow declared first (85.23%, picks D_800D3430 then subtracts
//    0x10), slot before the loads (89.75%), inline pField, inline b, (void)pRow,
//    and the store address inlined instead of a pRow local (76.43%).
/*
void func_8007CDD0(u8 **arg0, u8 arg1) {
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    u8 value = BattleFindFirstFlagTableMatch(pField[(*arg0)[2]]);
    u8 *pRow = (u8 *)(D_800D3420 + 0x10) + arg1 * 0x40;
    u8 a = g_BattleSlotRecords[value * 0x1C];
    s32 slot = arg1 + 3;
    s32 opOffset = a * 8;

    pRow[(*arg0)[1]] = g_BattleSlotParamTable[g_BattleSlotRecords[slot * 0x1C] * 0x40 + opOffset + 0x140];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CDD0);

void BattleStoreCharSlotMatchedTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i = 0;
    u16 *pField = D_800D3410[row].unk10;

    for (; i < 3; i++) {
        if (func_8007A628(i, 0) != 0) {
            u8 other = BattleFindFirstFlagTableMatch(pField[(*ppArgs)->tag]);

            if (g_BattleSlotRecords[i].tag == g_BattleSlotRecords[other].tag) {
                count++;
            }
        }
    }
    D_800D3410[row].unk20[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotMatchedTagCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i = 3;
    u16 *pField = D_800D3410[row].unk10;

    for (; i < 0xB; i++) {
        if (func_8007A628(i, 0) != 0) {
            u8 other = BattleFindFirstFlagTableMatch(pField[(*ppArgs)->tag]);

            if (g_BattleSlotRecords[i].tag == g_BattleSlotRecords[other].tag) {
                count++;
            }
        }
    }
    D_800D3410[row].unk20[(*ppArgs)->slot] = count;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D0CC);
/* best: 85.97% (identical instruction multiset, 4 insns of ordering). Retail
   forms the D_800D3430 base in a1, then orders
   [sll][lui/addiu][lw *arg0][addu base][lbu idx][move i=0][addu][sb];
   gcc emits [move i=0][sll][lw *arg0][lui/addiu][lbu][addu base][addu][sb] -
   i.e. the scheduler hoists `i = 0` to the very top and interleaves the two
   address chains the other way round. Tried: i = 0 between store and loop,
   the inline *(D_800D3430 + arg1*0x40 + (*arg0)[1]) spelling (folds into $at,
   frame 8), index local. This is the scheduler-priority tie class.

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
    u16 value = D_800D3410[row].unk10[(*ppArgs)->tag];

    pDst[index * 8 + (*ppArgs)->slot] = value;
    *(pDst + (index * 8 + (*ppArgs)->slot) + 1) = value >> 8;
}
void BattleStoreGoldWord32(BattleSlotArgs **ppArgs, u8 row) {
    D_800D3410[row].unk00[(*ppArgs)->slot] = g_GameState.gold;
}
void BattlePickSlotByActorTag(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[0x10];
    s32 i = 0;
    s32 count = 0;
    s32 record = 0;
    u8 *pBuf = buf;
    u16 *pField = D_800D3410[row].unk10;

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
        u16 *pRow = D_800D3410[row].unk10;

        pRow[index] = result;
    }
}
void BattleStoreStatWord32(BattleSlotArgs **ppArgs, u8 row) {
    D_800D3410[row].unk00[(*ppArgs)->slot] = g_BattleRowStatValues[row];
}
void BattlePickActorSlotWithFlag80(BattleSlotArgs **ppArgs, u8 row) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 record = 0x54;
    u8 *pBuf = buf;
    u16 *pField = D_800D3410[row].unk10;

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
        u16 *pRow = D_800D3410[row].unk10;

        pRow[index] = result;
    }
}
// best: 98.46%. Everything matches except the row-base copy in the preheader:
// retail computes the base in caller-saved a1 (addu a1,a1,v1), uses it for the
// zeroing store (addu v0,v0,a1), then copies it into s2 at loop entry
// (move s2,a1) - one instruction more than gcc 2.6 emits, which homes the base
// in s2 from the start (addu s2,a1,v1). Saved-register assignment does match
// retail now (s1=arg0, s2=base, s0=buf).
// How it got here: the do-while(0) wrapper alone changes nothing; the duplicated
// if (arg0) branch, whose two arms are identical (a permuter artifact kept only
// because it is what flips arg0 to s1 and base to s2), is needed - remove either
// and the function drops to 98.05% (s1=base, s2=arg0). Hoisting the flags word
// into a local is neutral on its own but keeps retail's single lhu.
// permute.sh ran twice from this body (576 and 976 iterations): no gain.
// Also tried: the zeroing store spelled inline instead of through pRow (97.24%,
// base materialised twice: addu s2,a1,v1 plus addu v0,a1,v0 / addu v0,v0,v1);
// both stores inline (88.06%); u8 buf declared first vs u16 *pRow first
// (identical); buf[idx] = 1 vs buf[slot] = 1 (loses the move/andi split, 92.79%).
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
        if ((D_800CCD34[i].flags & 0xC000) == 0) {
            count++;
        }
    }
    D_800D3410[row].unk20[(*ppArgs)->slot] = count;
}
void BattleStoreActorSlotFlagsC000ClearCount(BattleSlotArgs **ppArgs, u8 row) {
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (g_BattleSlotEnabledFlags[i] != 0 && (D_800CCD34[i].flags & 0xC000) == 0 && g_BattleSlotRecords[i].flags == 0) {
            count++;
        }
    }
    D_800D3410[row].unk20[(*ppArgs)->slot] = count;
}
void BattlePickCharSlotByLowestEntryDwordInUse(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (func_8007A628(i, (*ppArgs)->tag) != 0 && g_BattleSlotFlags[i].inUse != 0
            && best >= D_800CCD34[i].unk0B8) {
            best = D_800CCD34[i].unk0B8;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = D_800D3410[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
void BattlePickActorSlotByLowestEntryWordInUse(BattleSlotArgs **ppArgs, u8 row) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (func_8007A628(i, (*ppArgs)->tag) != 0 && g_BattleSlotFlags[i].inUse != 0
            && best >= D_800CCD34[i].unk000) {
            best = D_800CCD34[i].unk000;
            bestIndex = i;
        }
    }
    result = BattleSlotBitMask(bestIndex);
    pRow = D_800D3410[row].unk10;
    pRow[(*ppArgs)->slot] = result;
}
