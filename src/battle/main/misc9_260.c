#include "common.h"
#include "battle/actor.h"

// One slot of the per-slot operator register block at g_BattleSlotRegs (stride 0x40):
// the same arguments in 32-, 16- and 8-bit form (see src/battle/main/misc9.c, whose
// head half of this unit declares the same view).
typedef struct {
    /* 0x00 */ u8 unk00[0x10];
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
} BattleOpRegs; // Size: 0x40

// The three register banks of BattleOpRegs are symbols of their own, one
// 0x40-byte row per slot: g_BattleSlotRows is its +0x10 array, g_BattleSlotWordRegs
// its +0x20 array and g_BattleSlotByteRegs its +0x30 array.
#define BATTLE_SLOT_REG_ROW_BYTES sizeof(BattleOpRegs)
#define BATTLE_SLOT_REG_ROW_WORDS (BATTLE_SLOT_REG_ROW_BYTES / sizeof(u32))
#define BATTLE_SLOT_REG_ROW_HALFWORDS (BATTLE_SLOT_REG_ROW_BYTES / sizeof(u16))

// The word array BattleWriteWorkBufferWord addresses inside the block
// g_BattlePrimPool points at (misc1.c writes word 0 of it).
#define BATTLE_WORK_BUFFER_OFFSET 0x394

// The tail half of this unit is gcc 2.6.0 code: BattleDwordRegMulImm's product is
// byte-identical under it and not under 2.7.2 (see the note on that function).
extern u32 D_800D2C60[];
extern u8 g_BattleRowStatValues[];
extern u8 g_BattlePendingCharIndex;
extern u8 *g_BattlePrimPool;
extern u8 *g_BattleSlotParamTable;
// A slot's battle register file seen from its dword registers (g_BattleSlotRows is
// g_BattleSlotRegs + 0x10): see misc8.c, which declares the same 0x40-byte row.
typedef struct {
    /* 0x00 */ u32 dwordRegs[4];
    /* 0x10 */ u16 wordRegs[8];
    /* 0x20 */ u8 byteRegs[0x10];
    /* 0x30 */ u8 pad30[0x10];
} BattleSlotRow; // Size: 0x40
// One of the 8-byte per-slot "can be picked" records at g_BattleSlotFlags (see
// misc7.c and misc8.c, which declare the same view).
typedef struct {
    /* 0x0 */ u8 inUse;
    /* 0x1 */ u8 unk1[7];
} BattleSlotFlag; // Size: 0x8

// One of the 0x1C-byte per-slot tag records at g_BattleSlotRecords; only the tag
// byte is read here (misc8.c names the rest of the record).
typedef struct {
    /* 0x0 */ u8 tag;
    /* 0x1 */ u8 unk1[0x1B];
} BattleSlotRecord; // Size: 0x1C

extern BattleSlotRecord g_BattleSlotRecords[];
extern BattleSlotRow g_BattleSlotRows[];
extern u16 g_BattleSlotWordRegs[][BATTLE_SLOT_REG_ROW_HALFWORDS];
extern BattleSlotFlag g_BattleSlotFlags[];
extern u8 g_BattleSlotByteRegs[][BATTLE_SLOT_REG_ROW_BYTES];
extern BattleTableEntry g_BattleSlotStatusRows[];
extern u8 D_800CD1B2[];
extern u8 BattleSlotFlagsC000Clear(u8 arg0);
extern u16 BattleStatusBitMask(u8 arg0);
extern s32 BattleFindFirstFlagTableMatch(u16 arg0);
extern void BattleResetSlotDisplayValues(u8 *arg0);
extern u8 BattleSlotFlagsC002ClearRecordIdle(u8 arg0, u8 arg1);
extern s32 BattleSlotWordVarGetSet(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);
extern u16 BattleSlotBitMask(u8 arg0);

void func_8007E334(BattleCommand **ppCmd, u8 arg1) {
    s32 acc = 0;
    s32 next;
    s32 i;
    u16 *pRow;

    // the operator slots are walked downwards
    for (i = 0xA; i >= 3; i--) {
        if ((u8)BattleSlotFlagsC000Clear((u8)i) != 0) {
            switch ((*ppCmd)->operand2) {
            case 1:
                if (g_BattleSlotFlags[i].inUse != 0) {
                    acc |= 1;
                }
                break;
            case 2:
                if (g_BattleSlotFlags[i].inUse == 0) {
                    acc |= 1;
                }
                break;
            default:
                acc |= 1;
                break;
            }
        }
        // the doubling goes through a temp: retail keeps the shifted value in a
        // caller-saved register and copies it back at the join
        next = acc * 2;
        acc = next;
    }
    pRow = g_BattleSlotWordRegs[arg1];
    pRow[(*ppCmd)->operand1] = next << 2;
}
/* best: 80.63%. The record table takes an explicit byte offset
   (g_BattleSlotRecords[offset], offset from 0x38 stepping -0x1C): that is the only form
   that reproduces the original's two `lui at / addu at / lbu %lo(at)` loads and
   the `li s2,0x38` prologue. Remaining: gcc splits the row base into
   `(arg1 & 0xFF) << 6` plus a hoisted `&g_BattleSlotWordRegs` (the original keeps the
   whole sum in s4) and hoists a third symbol base, so the frame grows to 0x38.
   Tried: row base as a pRow local (77.49%, gcc CSEs it with the store's), row
   base inline + pRow local for the store (80.63%).

   R163: builds only through a (u8 *) byte view of the record/flag table
   (the bare g_X[byteOffset] spellings are invalid C); remeasured, the
   percentage above stands.

   R458: the doubling is solved on the two functions below this one - write it
   through a temp that the tail reads (`next = acc * 2; acc = next;` then
   `pRow[...] = next << 2`), which reproduces retail's `sll a1,s0,1 / move s0,a1`
   (func_8007E334 95.31 -> 100, func_8007E554 89.08 -> 100). The row base is
   `g_BattleSlotWordRegs[arg1]` (0x40-byte rows), not `g_BattleSlotWordRegs + arg1 * 0x40`,
   and g_BattleSlotRecords is now the typed 0x1C record view (`[i].tag`).
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9_260", func_8007E438);
/* R165 second pass: 80.63% (308 bytes) confirmed as recorded; unchanged, no
   lever found. Deltas. (1) The first record load is
   `g_BattleSlotRecords[BattleFindFirstFlagTableMatch(field) * 0x1C]` reached
   inline: retail builds the row base g_BattleSlotWordRegs + arg1 * 0x40 + p[2] * 2 with
   a single register sum, gcc splits off `&g_BattleSlotWordRegs` and the <<6 and hoists
   them, which is what grows the frame to 0x38. (2) gcc keeps
   &g_BattleSlotRecords in a saved register across the loop for the first of
   the two record loads where retail re-forms it with `lui $at / addu $at`.
   (3) the accumulator doubling, same miss as func_8007E334/E554: retail
   computes it into a temp and copies it back (`sll a1,s1,1 / move s1,a1`),
   gcc doubles in place - `acc = acc * 2` measured identical to `acc <<= 1`
   in func_8007E554, so the shape is not the operator.
void func_8007E438(u8 **arg0, u8 arg1) {
    s32 acc = 0;
    s32 offset = 0x38;
    s32 i;
    u16 *pRow;

    for (i = 2; i >= 0; i--) {
        if ((u8)BattleSlotFlagsC000Clear((u8)i) != 0) {
            if (((u8 *)g_BattleSlotRecords)[(u8)BattleFindFirstFlagTableMatch(
                    *(u16 *)(g_BattleSlotWordRegs + arg1 * 0x40 + (*arg0)[2] * 2)) * 0x1C] ==
                ((u8 *)g_BattleSlotRecords)[offset]) {
                acc |= 1;
            }
        }
        acc <<= 1;
        offset -= 0x1C;
    }
    pRow = (u16 *)(g_BattleSlotWordRegs + arg1 * 0x40);
    pRow[(*arg0)[1]] = (u16)acc >> 1;
}
 */
void func_8007E554(BattleCommand **ppCmd, u8 arg1) {
    s32 acc = 0;
    s32 next;
    s32 i = 0xA;
    u16 *pField;
    u16 *pRow;

    pField = g_BattleSlotWordRegs[arg1];
    for (; i >= 3; i--) {
        if ((u8)BattleSlotFlagsC000Clear((u8)i) != 0) {
            if (g_BattleSlotRecords[(u8)BattleFindFirstFlagTableMatch(pField[(*ppCmd)->operand2])].tag
                == g_BattleSlotRecords[i].tag) {
                acc |= 1;
            }
        }
        next = acc * 2;
        acc = next;
    }
    pRow = g_BattleSlotWordRegs[arg1];
    pRow[(*ppCmd)->operand1] = next << 2;
}

void BattleClearSlotDwordAndSetStat4(u8 arg0) {
    D_800D2C60[arg0] = 0;
    g_BattleRowStatValues[arg0] = 4;
}
// Spelled out at each operand on purpose (see BattleWordRegAnd in misc6_r2.c):
// with the base in one local gcc 2.6.0 keeps it in $v1, with three accesses it
// allocates retail's $a2.
void BattleDwordRegAdd(BattleCommand **ppCmd, u8 index) {
    BattleCommand *pCmd = *ppCmd;

    g_BattleSlotRows[index].dwordRegs[pCmd->operand3] =
        g_BattleSlotRows[index].dwordRegs[pCmd->operand1] + g_BattleSlotRows[index].dwordRegs[pCmd->operand2];
}
void BattleDwordRegSub(BattleCommand **ppCmd, u8 index) {
    BattleCommand *pCmd = *ppCmd;

    g_BattleSlotRows[index].dwordRegs[pCmd->operand3] =
        g_BattleSlotRows[index].dwordRegs[pCmd->operand1] - g_BattleSlotRows[index].dwordRegs[pCmd->operand2];
}
// The product is 2.6.0's: retail reuses the element value's dead $v0 for the
// mflo (mult a0,v0 / mflo v0 / sw v0,0(v1)); gcc 2.7.2 gives it a fresh $a2.
void BattleDwordRegMulImm(BattleCommand **ppCmd, u8 arg1) {
    BattleCommand *pCmd = *ppCmd;
    u32 *pRow = g_BattleSlotRows[arg1].dwordRegs;

    pRow[pCmd->operand1] = pCmd->operand2 * pRow[pCmd->operand1];
}
void BattleDwordRegDivImm(BattleCommand **ppCmd, u8 arg1) {
    BattleCommand *pCmd = *ppCmd;
    u32 *pRow = g_BattleSlotRows[arg1].dwordRegs;

    pRow[pCmd->operand1] /= pCmd->operand2;
}
void BattleWriteWorkBufferWord(BattleCommand **ppCmd) {
    BattleCommand *pCmd = *ppCmd;

    ((u16 *)(g_BattlePrimPool + BATTLE_WORK_BUFFER_OFFSET))[pCmd->operand1] = pCmd->operand2;
}
void BattleSetStatusBitAllSlots(BattleCommand **ppCmd) {
    u8 flag = (*ppCmd)->operand2 != 0;
    BattleTableEntry *pRow = g_BattleSlotStatusRows;

    do {
        if (flag) {
            pRow->flags |= BattleStatusBitMask((u8)((*ppCmd)->operand1 + 7));
        } else {
            pRow->flags &= ~BattleStatusBitMask((u8)((*ppCmd)->operand1 + 7));
        }
        pRow++;
    } while ((s32)pRow < (s32)D_800CD1B2); // the symbol one past the last row
}
void BattleGetFontGlyph(BattleCommand **ppCmd) {
    BattleCommand *pCmd = *ppCmd;
    s32 idx = pCmd->operand2 * 8 + pCmd->operand1 * 0x40;

    g_BattleSlotParamTable[(u32)(idx + 0x140)] = pCmd->operand3;
}
void BattleSetPendingCharIndex(BattleCommand **ppCmd, u8 arg1) {
    u16 *pRow = g_BattleSlotWordRegs[arg1];

    g_BattlePendingCharIndex = BattleFindFirstFlagTableMatch(pRow[(*ppCmd)->operand1]) + 1;
}
void BattleResetDisplayValues(void) {
    u8 buf[0x10];

    BattleResetSlotDisplayValues(buf);
}
s32 BattleSetTextScale(BattleCommand **ppCmd, u8 arg1) {
    u8 *pRow = g_BattleSlotByteRegs[arg1];

    return pRow[(*ppCmd)->operand1] == (*ppCmd)->operand2;
}
s32 BattleWordRegEqImm(BattleCommand **ppCmd, s32 arg1) {
    BattleCommand *pCmd = *ppCmd;
    s32 value = pCmd->operand2 | (pCmd->operand3 << 8);
    u16 *pRow = g_BattleSlotWordRegs[arg1 & 0xFF];

    return pRow[pCmd->operand1] == value;
}
s32 BattleByteRegLeImm(BattleCommand **ppCmd, u8 arg1) {
    u8 *pRow = g_BattleSlotByteRegs[arg1];

    return (*ppCmd)->operand2 >= pRow[(*ppCmd)->operand1];
}
