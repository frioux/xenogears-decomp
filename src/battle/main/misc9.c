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
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    s32 rowIndex;
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    while (i < 3) {
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
        u16 *pOut = (u16 *)((rowIndex & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}
/* best: 95.38% (352 bytes / 88 insns; retail 0x8007DA1C..0x8007DD1C = 87 insns /
   348 bytes). The BattlePickSlotFromOperatorWord R370/R398 two-pointer recipe carries this stub:
   `pField = pRow;` after the head store, the index-first base with an (s32) cast
   on the symbol, a u16 value local at the tail, and the loop written as a
   `while (i < 0xB)` from 3 - that is 75.77 -> 95.38 in one step.
   NAMED residual: THE LOOP INCREMENT'S POSITION. Retail has
   [jal ClearRecordIdle][andi v0,v0,0xFF][beqz v0][addiu s1,s1,1], i.e. i is
   incremented unconditionally between the call and the test on its result, so
   reorg puts the increment in that beqz slot and the body needs only TWO slti
   (DAF0 plus the beqz v1 slot). With `idx = i++` cc1 emits the increment at the
   loop head, so the same slot takes a third COPY of slti - one instruction too
   many to link (88 vs 87). The increment's position is source-controllable only
   at the cost of the preheader copy, and the two are exclusive (measured on this
   body): the 'ok' shape (u8 idle result local, i++ between the call and the
   test, `idx = i` as the last body statement) DOES put the increment in retail's
   beqz v0 slot, but then the preheader copy folds to `li s3,3` and reorg does
   not duplicate slti into the beqz v1 slot at all -> 86 insns, one SHORT, 92.51%
   (344 bytes; identical with the two branches merged into one amp-amp test).
   Also measured: s32 idle local 88.25% (336), increment at the body end after
   the if 88.67% (332, the s3 copy disappears), copy-at-bottom only 89.03%,
   do-while 93.77% (344, short), `idx = i; i++;` identical to `idx = i++`, count
   declared first 90.75%. So this is a genuine two-way wall, not a spelling left
   to find (tools/rounds/levers.md,
   increment-in-a-branch-slot-versus-the-preheader-copy-are-exclusive).
   Cosmetic only: the prologue emits the s6 = arg1 copy before the count and i
   inits (retail after them), and the tail beqz s2 slot holds move a0,zero where
   retail has addiu a1,s2,-1.
   Preset screened: -O3 95.38, -G0 95.38, gcc-2.6.0 95.78, cdk 76.66% on sibling
   DB78 - Default (2.7.2, -O2 -G8) is right.
   R482: measured the other side of the wall from source. The ok-shape (`u8 idle`
   result local, `i++;` between the call and the test, `idx = i` as the last body
   statement, do-while) reproduces the increment in the beqz v0 slot AND the idx
   copy in the bnez slot - retail's two filled slots - and then lacks exactly ONE
   instruction, reorg's obstruction duplicate of the latch `slti` in the second
   beqz's slot: 86 insns / 344 bytes / 92.51%. So the wall is a single instruction
   seen from both sides (88 insns with a third slti copy, 86 without the second),
   not two residuals, and no spelling has both. Also measured on that body: the
   test written `0 < 3 - i` reaches retail's exact 87 insns / 348 bytes at 90.43%
   by spending three instructions on the test instead of slti+bnez, so byte count
   alone is not the gate. Dropping the pField local: 85 insns / 87.08%.
 */
void func_8007DA1C(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 count = 0;
    s32 i = 3;
    s16 last;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    s32 rowIndex;
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    while (i < 11) {
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
        u16 *pOut = (u16 *)((rowIndex & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DA1C);


// best: 95.64% (388 bytes / 97 insns; retail 0x8007DB78..0x8007DCF8 = 96 insns /
// 384 bytes), i.e. ONE instruction OVER, so it cannot link. (Earlier notes in this
// file said retail was "336 / 84"; the stub's own .s is 96 insns.)
// The shape that gets here, worth +7 points over the while/do-while forms
// (88.59% / 380 bytes / 95 insns, measured in R495): the loop written as
// `for (i = 0; i < 3; offset += 8)`, so `offset += 8` is the for-clause and the
// body is `idx = i++; if (...) {...}`; the D8C0 R370 two-pointer recipe
// (`pField = pRow;` after the head store, the loop reads pField); DE78's solved
// tail (u16 value local, int-built base with an (s32) cast on the symbol,
// pointer local). The for-clause makes reorg duplicate the loop test
// `slti v0,s1,3` into all THREE conditional-branch delay slots, which is where
// retail's copies sit.
// NAMED residual, exactly one instruction: the `i++`'s position. Retail's
// `addiu s1,s1,1` is in the FIRST beqz's delay slot (the one after the
// ClearRecordIdle result test), so that branch is not threaded and carries no
// slti copy; with the increment at the loop head that slot takes a copy of slti
// instead, giving four slti against retail's three. Every spelling that moves
// the increment into retail's slot loses the for-clause's copies or the loop
// shape: `while (i<3) { idx = i; idle = call(idx,0); i++; if (idle) ... }`
// 82.77% / 364; increment at the body end 84.55% / 368; `for (i=0;i<3;i++,offset+=8)`
// 89.28% / 368; increment + offset at the body end of a while 88.59% / 380;
// do-while 88.59% / 380. So this is the family's documented two-way wall
// (tools/rounds/levers.md, increment-in-a-branch-slot-versus-the-preheader-copy-are-exclusive),
// not a spelling left to find. Preset is right: Default (2.7.2, -O2 -G8);
// gcc-2.7.2-cdk 76.66% on this stub (R163).
// func_8007DFD4 is this function with ClearRecordIdle's second argument 1 - the
// two stubs differ in exactly that instruction - and scores the same.
/*
void func_8007DB78(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 offset = 0;
    s32 idx;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    pField = pRow;
    for (i = 0; i < 3; offset += 8) {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 0) != 0) {
            if (((u8 *)g_BattleSlotFlags)[offset] != 0) {
                if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
    }
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DB78);


void BattlePickSlotFromOperatorWordFlagged(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 3;
    s16 last;
    s32 count = 0;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    s32 rowIndex;
    u16 *pField;
    s32 offset = 0x18;
    s32 rowMask;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    rowMask = 0xFF;
    for (; i < 0xB; offset += 8) {
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
        u16 *pOut = (u16 *)((rowIndex & rowMask) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}

/* best: 95.25% (352 bytes / 88 insns; retail 0x8007DE78..0x8007E080 = 87 insns /
   348 bytes). Byte-for-byte the func_8007DA1C stream with only ClearRecordIdle's
   second argument (1) and the loop bounds (0..3 instead of 3..0xB) changed, and
   DA1C's recipe transfers unchanged: `pField = pRow;` after the head store, the
   index-first base with an (s32) cast on the symbol, a u16 value local at the
   tail, and the loop as `while (i < 3)`. R469: the WHILE form is worth 1.6
   points over the do-while this stub used to park with (93.64% / 344 bytes, 86
   insns, one SHORT); the while form is 88 insns, one OVER, the same way DA1C is.
   NAMED residual: the loop increment's position - retail increments i
   unconditionally between the ClearRecordIdle call and the test on its result
   ([jal][andi v0,v0,0xFF][beqz v0][addiu s1,s1,1]), so reorg fills that beqz
   slot with the increment and the body carries only TWO slti. `idx = i++` puts
   the increment at the loop head and that slot takes a third COPY of slti (one
   instruction too many); the 'ok' shape that moves it into retail's slot loses
   reorg's duplicate of slti in the beqz v1 slot instead (86 insns, one SHORT,
   91.33%). The two are exclusive - see DA1C's park note above and
   tools/rounds/levers.md,
   increment-in-a-branch-slot-versus-the-preheader-copy-are-exclusive.
   Cosmetic only: the prologue emits the s6 = arg1 copy before the i/count
   inits (retail after them), and the tail beqz s2 slot holds move a0,zero where
   retail has addiu a1,s2,-1.
   R482: the DA1C ok-shape measured here too - the increment in the beqz v0 slot,
   the idx copy in the bnez slot, and only reorg's obstruction duplicate of the
   latch `slti v0,s1,3` missing: 86 insns / 344 bytes / 92.95%, against this
   while body's 88 / 352 / 95.25 and retail's 87 / 348. Same single-instruction
   wall as DA1C.
 */
void func_8007DE78(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s16 last;
    s32 count = 0;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    s32 rowIndex;
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    rowIndex = arg1;
    pField = pRow;
    while (i < 3) {
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
        u16 *pOut = (u16 *)((rowIndex & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DE78);


/* best: 85.10%. func_8007DFD4 is this function with BattleSlotFlagsC002ClearRecordIdle's second
   argument 1 (verified: the two stubs differ in exactly that instruction), and
   func_8007DE78 is the same loop without the g_BattleSlotFlags gate. Shape confirmed:
   the row zero, the idx = i++ copy that keeps idx in its own register (s3) and
   i in s1, the gate read as g_BattleSlotFlags[offset] with offset stepping +8, and the
   tail store via DE78's solved recipe (u16 value local + int-built base with an
   (s32) cast on the symbol + pointer local). The gate is indexed in BYTES:
   g_BattleSlotFlags is u8[] and the asm does lbu with at = %hi + s4.
   Remaining deltas, none source-visible so far (376 bytes / 94 insns vs retail
   336 / 84):
   (1) retail computes the row base as a whole register sum before it loads
   *arg0 (andi/sll/lui/addiu/addu into v0, then a separate move s6,v0); gcc
   folds the sum into the first use, so mine emits lui/addiu only after
   lw v0,0(s5) and adds straight into s6. Tried the plain pointer initialiser,
   the (s32)-cast form and an s32 rowBase local: all 85.10%.
   (2) retail duplicates slti v0,s1,3 into THREE places (delay slots of the
   a628 beqz and of the gate/and beqzs, plus the latch); gcc emits it twice.
   Tried do-while, while and a for-head loop: identical output.
   (3) idx = i++ is emitted at the loop top; retail has the increment in the
   delay slot of the beqz after the BattleSlotFlagsC002ClearRecordIdle call (DE78's nit 3).
   (4) tail: retail puts addiu a1,s2,-1 in the beqz s2 delay slot and
   move a0,zero after it; mine the other way round (an s32 last = count - 1
   local does not move it).
   gcc-2.7.2-cdk scores 76.66% here, so this is not a cdk unit; Default is
   right.

   R163: builds only through a (u8 *) byte view of the record/flag table
   (the bare g_X[byteOffset] spellings are invalid C); remeasured, the
   percentage above stands.
 */
// best: 95.64% (388 bytes / 97 insns; retail 0x8007DFD4..0x8007E150 = 96 insns /
// 384 bytes), ONE instruction over. Byte-for-byte func_8007DB78's stream with
// ClearRecordIdle's second argument 1 instead of 0, so DB78's note above applies
// unchanged (the for-clause shape, the pField two-pointer recipe, and the
// one-instruction residual: retail's `addiu s1,s1,1` in the first beqz delay slot
// versus reorg's fourth `slti v0,s1,3` copy).
/*
void func_8007DFD4(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 offset = 0;
    s32 idx;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    pField = pRow;
    for (i = 0; i < 3; offset += 8) {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 1) != 0) {
            if (((u8 *)g_BattleSlotFlags)[offset] != 0) {
                if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
    }
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DFD4);

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
        acc <<= 1;
    }
    pRegs = g_BattleSlotRegs[arg1].wordRegs;
    pRegs[(*ppCmd)->operand1] = (u16)acc >> 1;
}
