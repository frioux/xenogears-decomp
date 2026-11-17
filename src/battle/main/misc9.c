#include "common.h"
#include "battle/actor.h"

// One slot of the per-slot operator register block at g_BattleSlotRegs (stride 0x40):
// the same arguments in 32-, 16- and 8-bit form. g_BattleSlotRows, D_800D3420 and
// D_800D3430 are its +0x10, +0x20 and +0x30 arrays; the handlers below read the
// operand indexes from the slot's current instruction (BattleCommand) and apply
// the operation to that slot's registers.
typedef struct {
    /* 0x00 */ u8 unk00[0x10];
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
} BattleOpRegs; // Size: 0x40

// One of the 0x1C-byte per-slot tag records at g_BattleSlotRecords: tag at +0, flag byte
// at +3 (the byte BattleSetSlotRecordByteAndRelease writes).
typedef struct {
    /* 0x0 */ u8 tag;
    /* 0x1 */ u8 unk1[2];
    /* 0x3 */ u8 flags;
    /* 0x4 */ u8 unk4[0x18];
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
extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u8 D_800C3EB7[];
extern u8 D_800CCD62[];
extern u8 D_800CD1B2[];
extern u8 D_800D32B1[];
extern u8 D_800D32F1[];
extern u8 BattleSlotFlagsC002ClearRecordIdle(u8 arg0, u8 arg1);
extern s32 BattleSlotWordVarGetSet(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);
extern u16 BattleSlotBitMask(u8 arg0);

/* best: 80.37%. Same family as func_8007DA1C/DB78/DCF8/DE78/DFD4 (all still
   stubs). Structure recovered from the asm: zero the row slot, then a do-while
   over the operator slots collecting the matching ones into an 8-byte stack
   buffer, then pick one at random. The `idx = i++` copy is real: retail keeps
   it in a separate register (s3) and re-forms it at the loop latch, and the
   buffer stores idx, not i. Remaining misses, all at 80.37% (336 bytes, frame
   and saved-register set already exact): (1) the final store re-forms the row
   base with a fresh `lui a1 / addiu a1 / addu a0,a0,a1` instead of reusing the
   loop's base register - tried a pOut pointer (77-80%, gcc reuses the loop
   base or spills), `(s32)D_800D3420` staged local (76.10%, shape right but one
   extra insn) and the plain inline expression (the $at form);
   (2) retail steals `i++` into the beqz delay slot after the BattleSlotFlagsC002ClearRecordIdle
   call, gcc emits it at the loop head;
   (3) declaration order of the two zeroed accumulators (retail i then count).
void func_8007D8C0(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 count = 0;
    s32 i = 0;
    s32 idx;
    u16 *pField = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pField[(*arg0)[2]] = 0;
    idx = i;
    do {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 0) != 0) {
            if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    } while (i < 3);
    if (count != 0) {
        u8 *pBase = D_800D3420 + (arg1 & 0xFF) * 0x40;

        *(u16 *)(pBase + (*arg0)[2] * 2) = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
    }
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007D8C0);
/* best: 75.77%. Same family as func_8007D8C0/DE78 (see D8C0's note). Structure
   recovered and confirmed byte-for-byte against the -(13) prologue, the row
   zero, the count loop and the call sequence; two misses remain:
   (1) the tail store. Retail re-forms the row base with `lui a1,%hi / addiu a1 /
   addu a0,a0,a1` (a real register via `la`) and then adds the index; every shape
   tried (inline byte expression, (u16*)cast array index, s32 base local in both
   operand orders, u8 *pBase local, reuse of the pRow local) makes gcc fold the
   two register terms first and emit `sh v0,D_800D3420(a0)`, i.e. maspsx's
   $at form. So it is gcc's combine merging (sym+reg) into the store address that
   retail's compiler did not do, not a source detail.
   (2) retail steals `addiu s1,s1,1` into the beqz delay slot after the
   BattleSlotFlagsC002ClearRecordIdle call; gcc emits it at the loop bottom.
   func_8007DE78 is this function with i starting at 0, the loop bound 3 and
   BattleSlotFlagsC002ClearRecordIdle's second argument 1, so it shares both misses.
void func_8007DA1C(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 count = 0;
    s32 i;
    u8 idx;
    u16 *pRow = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pRow[(*arg0)[2]] = 0;
    for (i = 3; i < 0xB; i++) {
        idx = i;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 0) != 0) {
            if (pRow[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                buf[count] = i;
                count++;
            }
        }
    }
    if (count != 0) {
        u8 *pBase = D_800D3420 + (arg1 & 0xFF) * 0x40;

        *(u16 *)(pBase + (*arg0)[2] * 2) = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
    }
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DA1C);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DB78);
/* R165 second pass: still 85.10% (376 bytes vs retail's 336); the head and the
   loop are byte-exact apart from the three deltas below. permute.sh --minutes 4
   -j 2 drove it to 813 from 1450, and the candidate that did it is WRONG CODE
   (it redirected the tail store through a reused index), but it names the
   missing lever precisely: retail's row base is a VALUE in v0
   (`addu v0,v0,v1`) that is then COPIED into the loop's saved pointer
   (`move s6,v0`), while gcc coalesces the two pseudos. The permuter only broke
   the tie by keeping the s32 sum live past the copy (`rowBase = 2;` after
   `u16 *pRow = (u16 *)rowBase;`). So the next thing to try is a legitimate use
   of a live s32 row base that does not change an operand - not a new shape of
   the expression, which is measured dead: plain pointer initialiser, the
   (s32)-cast form, an s32 base local (all 85.10%), a two-statement
   base/pointer split and the (u16 *)((arg1 & 0xFF) * 0x40) + (s32)D_800D3420
   form (identical output).
   The rest, unchanged: retail duplicates `slti v0,s1,3` into three places
   (delay slots of the a628 beqz and of both beqzs, plus the latch) where gcc
   emits it twice, and retail has `addiu a1,s2,-1` in the beqz s2 delay slot
   with `move a0,zero` after it, mine the other way round.
void func_8007DB78(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 offset = 0;
    s32 idx;
    s32 rowBase = (arg1 & 0xFF) * 0x40 + (s32)D_800D3420;
    u16 *pRow = (u16 *)rowBase;

    pRow[(*arg0)[2]] = 0;
    while (i < 3) {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 0) != 0) {
            if (((u8 *)g_BattleSlotFlags)[offset] != 0) {
                if (pRow[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
        offset += 8;
    }
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420);

        pOut[(*arg0)[2]] = value;
    }
}
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DCF8);
/* best: 89.33% (336 bytes; mine 340). This is the func_8007D8C0/DA1C
   family and the family's documented blocker (the tail store re-forming the row
   base) IS SOLVED here: write the tail as a u16 value local, then a pointer
   local whose base is built in INT arithmetic, then the store:

       u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
       u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420);
       pOut[(*arg0)[2]] = value;

   That reproduces retail's andi/sll/lw p/lui a1/addiu a1/lbu p[2]/addu a0,a0,a1/
   sll/addu/sh exactly (the (s32) cast stops gcc folding the symbol into the
   store as %lo(sym)(reg), and the separate `value` statement keeps the address
   computation after the two jal's instead of hoisting it). Remaining, 3 insns:
   (1) retail keeps a temp copy of the row base (addu v1,v1,v0 then move s5,v1,
   and the la lands in v0 before the remaining saves) where gcc computes
   addu s5,v1,a0 with the la in a0 after them - tried the plain pointer form,
   (arg1 & 0xFF) * 0x40 + (s32)D_800D3420, and an s32 base local converted to
   u16* (all 89.33%, gcc coalesces the copy away).
   (2) sw s6/move s6,a1 is emitted before the i/count zero-inits here, after
   them in retail (declaration order i then count already matches retail's
   s1-then-s2 zeroing).
   (3) retail steals addiu s1,s1,1 into the beqz delay slot after the
   BattleSlotFlagsC002ClearRecordIdle call; with idx = i++ the whole loop body is byte-exact
   otherwise, and that shape is required (idx must stay a separate register;
   idx = i inside the body makes gcc propagate it and lose s3).
void func_8007DE78(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 idx;
    u16 *pRow = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pRow[(*arg0)[2]] = 0;
    do {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 1) != 0) {
            if (pRow[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    } while (i < 3);
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420);

        pOut[(*arg0)[2]] = value;
    }
} */
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DFD4);
/* void func_8007DFD4(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 offset = 0;
    s32 idx;
    u16 *pRow = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pRow[(*arg0)[2]] = 0;
    while (i < 3) {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 1) != 0) {
            if (((u8 *)g_BattleSlotFlags)[offset] != 0) {
                if (pRow[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
        offset += 8;
    }
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420);

        pOut[(*arg0)[2]] = value;
    }
}
 */
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
/* best: 95.31%. Same family as BattleStoreCharSlotFlagMask (walk the slot table downwards,
   `(i - 0xA) * 8` keeps D_800D32F1 as the base symbol). Only difference: the
   original doubles the accumulator into a temp and copies it back (sll a1,s0,1
   / move s0,a1) and reuses a1 for the final shift, gcc does it in place
   (sll s0,s0,1). Tried: acc*=2, acc=acc*2, acc=2*acc, acc=acc+acc,
   next=acc*2;acc=next, doubling in the for-increment clause (91.06%).
void func_8007E334(u8 **arg0, u8 arg1) {
    s32 acc = 0;
    s32 i;
    u16 *pRow;

    // the slot table is walked downwards from D_800D32F1
    for (i = 0xA; i >= 3; i--) {
        if ((u8)BattleSlotFlagsC000Clear((u8)i) != 0) {
            switch ((*arg0)[2]) {
            case 1:
                if (D_800D32F1[(i - 0xA) * 8] != 0) {
                    acc |= 1;
                }
                break;
            case 2:
                if (D_800D32F1[(i - 0xA) * 8] == 0) {
                    acc |= 1;
                }
                break;
            default:
                acc |= 1;
                break;
            }
        }
        acc *= 2;
    }
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = acc << 2;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E334);
/* best: 80.63%. The record table takes an explicit byte offset
   (g_BattleSlotRecords[offset], offset from 0x38 stepping -0x1C): that is the only form
   that reproduces the original's two `lui at / addu at / lbu %lo(at)` loads and
   the `li s2,0x38` prologue. Remaining: gcc splits the row base into
   `(arg1 & 0xFF) << 6` plus a hoisted `&D_800D3420` (the original keeps the
   whole sum in s4) and hoists a third symbol base, so the frame grows to 0x38.
   Tried: row base as a pRow local (77.49%, gcc CSEs it with the store's), row
   base inline + pRow local for the store (80.63%).

   R163: builds only through a (u8 *) byte view of the record/flag table
   (the bare g_X[byteOffset] spellings are invalid C); remeasured, the
   percentage above stands.
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E438);
/* R165 second pass: 80.63% (308 bytes) confirmed as recorded; unchanged, no
   lever found. Deltas. (1) The first record load is
   `g_BattleSlotRecords[BattleFindFirstFlagTableMatch(field) * 0x1C]` reached
   inline: retail builds the row base D_800D3420 + arg1 * 0x40 + p[2] * 2 with
   a single register sum, gcc splits off `&D_800D3420` and the <<6 and hoists
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
                    *(u16 *)(D_800D3420 + arg1 * 0x40 + (*arg0)[2] * 2)) * 0x1C] ==
                ((u8 *)g_BattleSlotRecords)[offset]) {
                acc |= 1;
            }
        }
        acc <<= 1;
        offset -= 0x1C;
    }
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = (u16)acc >> 1;
}
 */
/* best: 89.08%. Structure: for (i = 0xA; i >= 3; i--) over the operator slots,
   comparing g_BattleSlotRecords[(u8)BattleFindFirstFlagTableMatch(pField[p[2]]) * 0x1C] against
   g_BattleSlotRecords[offset] with offset = 0x118 stepping -0x1C, then storing
   `acc << 2` into the row. The row base MUST be a local pointer computed before
   the loop (inline gives 74.68%: gcc hoists &D_800D3420 and the <<6 separately
   and does the addu inside the loop, growing the frame to 0x38).
   Remaining: (1) gcc hoists &g_BattleSlotRecords into a register for the first of the two
   record loads only (retail uses the `lui at / addu at / lbu %lo(at)` form for
   both, so its 6th saved reg is absent); spelling the index through a separate
   `s32 slot` local is the next thing to try. (2) the accumulator doubling: retail
   emits `a1 = s1 << 1` on both paths and copies back at the join (sll a1,s1,1 /
   move s1,a1, and the final store shifts a1), gcc doubles in place
   (sll s1,s1,1) - same miss as func_8007E334, so `acc = acc * 2` vs
   `acc <<= 1` should be re-screened here.

   R163: builds only through a (u8 *) byte view of the record/flag table
   (the bare g_X[byteOffset] spellings are invalid C); remeasured, the
   percentage above stands.
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E554);
/* R165 second pass: 89.08% (300 bytes) confirmed. `acc = acc * 2` measures the
   same as `acc <<= 1` (both 89.08), so the doubling delta is not the operator:
   retail emits `sll a1,s1,1` in the bne delay slot and copies back at the join
   (`move s1,a1`), gcc doubles in place into s1. The other delta is
   &g_BattleSlotRecords: gcc hoists it into a saved register across the loop
   for the FIRST of the two record loads (`addu v1,v1,s6 / lbu v1,0(v1)`) where
   retail re-forms it with `lui $at / addu $at / lbu %lo(at)` for BOTH, which
   is also why mine saves one more register than retail (6 against 5) in the
   same 0x30 frame.
void func_8007E554(u8 **arg0, u8 arg1) {
    s32 acc = 0;
    s32 offset = 0x118;
    s32 i;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    u16 *pRow;

    for (i = 0xA; i >= 3; i--) {
        if ((u8)BattleSlotFlagsC000Clear((u8)i) != 0) {
            if (((u8 *)g_BattleSlotRecords)[(u8)BattleFindFirstFlagTableMatch(pField[(*arg0)[2]]) * 0x1C] ==
                ((u8 *)g_BattleSlotRecords)[offset]) {
                acc |= 1;
            }
        }
        acc <<= 1;
        offset -= 0x1C;
    }
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = acc << 2;
}
 */
void BattleClearSlotDwordAndSetStat4(u8 arg0) {
    D_800D2C60[arg0] = 0;
    g_BattleRowStatValues[arg0] = 4;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E6A0);
/* best: 99.00%. Identical instruction multiset and order; the only difference
   is the register holding the g_BattleSlotRows sum base: original a2 (and it is never
   reused), gcc v1 (which the p[1] scale temp then reuses). Same allocator tie as
   func_8007E6F0/E740/E98C/E8AC and the misc6 D_800D34x0 family. R201 re-tested,
   every shape 99.00%: `u8 arg1` (the andi then comes from the u8 multiply, same
   two insns), a `u8 slot` narrowing copy, an s32 rowBase local, `register s32 *pRow`,
   pRow declared before p, and a 4-minute permuter run (base=20 best=20, 2357
   iterations, nothing better). Two shapes that break the shape instead: a
   `mem_addr base` assigned then summed index-first, and a staged
   `u8 *pRows = g_BattleSlotRows`, both 79.00% (one extra insn). Tried before:
   split summand locals (84.35%, reorders the three lbu's RHS-first), three
   explicit `s32 i3/i1/i2 = p[..]` index locals (99.00%, same base v1), the
   byte-pointer form `(s32 *)((u8 *)g_BattleSlotRows + (arg1 & 0xFF) * 0x40)` (99.00%),
   and `g_BattleSlotRows + ((arg1 & 0xFF) << 6)` (99.00%). Also tried before: pRow/p in
   both orders, `s32 arg1` + explicit mask, element-unit base, all references
   inline. No try_flags.sh row reaches 100 (2.6.0 99.0, cdk 96.5, -O1 51.9), and
   the unit cannot move to cdk at all: BattleSetSlotRecordByteAndRelease 79.68%, BattleClearSlotDwordAndSetStat4 10.45%
   under cdk while matched under Default.
void func_8007E6A0(u8 **arg0, s32 arg1) {
    u8 *p = *arg0;
    s32 *pRow = (s32 *)(g_BattleSlotRows + (arg1 & 0xFF) * 0x40);

    pRow[p[3]] = pRow[p[1]] + pRow[p[2]];
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E6F0);
/* best: 99.00%. Same miss as func_8007E6A0 (the g_BattleSlotRows sum base: original
   a2, gcc v1); the subtraction itself is byte-exact. See the note there for the
   shapes already tried, including R201's permuter run.
void func_8007E6F0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    s32 *pRow = (s32 *)(g_BattleSlotRows + arg1 * 0x40);

    pRow[p[3]] = pRow[p[1]] - pRow[p[2]];
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E740);
/* best: 98.75% under this unit's Default (2.7.2) preset; BYTE-EXACT under gcc 2.6.0.
   This stub is 2.6.0 code. With the body below, gcc-2.6.0 (-O2 or -O3, -G4 or -G8)
   compiles retail's exact instruction stream, mult a0,v0 / mflo v0 / sw v0,0(v1); the
   only remaining objdiff miss is the typed access's relocation (g_BattleSlotRegs+0x10
   against retail's g_BattleSlotRows - the same address). Under 2.7.2 the product gets
   a fresh a2 (mflo a2; sw a2,0(v1)) where retail reuses the element value's dead v0,
   and no spelling moves it: element / product / qty locals, u32 and s32, pReg pointer,
   element-pointer and `BattleOpRegs *pSlot` locals, the inline double index, and
   `return elem = p[2] * elem` all land at 98.75, as did a 4-minute permuter run
   (base=10 best=10, 1383 iterations). The self-assignment form
   (`elem = pCmd->operand2 * elem;`) does give mflo v0 with every other register right,
   but gcc then prints the commutative mult as mult v0,a0 instead of retail's a0,v0.
   PRESET SCREEN, do not redo: compiling the WHOLE unit under gcc-2.6.0 leaves every
   other matched function instruction-identical (func_8007E6A0/E6F0 included) EXCEPT
   BattleStoreCharSlotFlagMask, where 2.6.0 folds the final `(u16)acc >> 1` into
   `andi v0,s0,0xfffe` against retail's `andi v0,s0,0xffff`. So misc9 wants a preset
   SPLIT at BattleStoreCharSlotFlagMask (2.7.2 for the head, 2.6.0 for the
   func_8007E6A0 - func_8007E740 run), not a unit switch.
void func_8007E740(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    g_BattleSlotRegs[arg1].dwordRegs[p[1]] = p[2] * g_BattleSlotRegs[arg1].dwordRegs[p[1]];
} */
void BattleDwordRegDivImm(BattleCommand **ppCmd, u8 arg1) {
    u32 *pRegs = g_BattleSlotRegs[arg1].dwordRegs;
    BattleCommand *pCmd = *ppCmd;

    pRegs[pCmd->operand1] /= pCmd->operand2;
}
void BattleWriteWorkBufferWord(BattleCommand **ppCmd) {
    *(u16 *)(g_BattlePrimPool + (*ppCmd)->operand1 * 2 + 0x394) = (*ppCmd)->operand2;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E7E4);
/* best: 96.50%. Structure found: do-while with the pointer compared as a SIGNED
   int (a u16* compare gives sltu, the original has slt), and the flag tested as
   `(u8)flag` so it is truncated at each of the two test sites. Only remaining
   difference: gcc fuses the truncation into the preheader (`andi s2,v0,0xff`)
   and drops the copy at the loop back edge, the original keeps the untruncated
   value in s2 (`move s2,v0; andi v0,s2,0xff`) and re-emits the andi in the
   bnez delay slot. Tried: u8 flag (94.60%), s32+cast (96.50%), flag&0xFF
   (94.60%), u8 copy inside the loop (96.50%). R201: a `s32` return type with
   `return flag;` does keep the s32 live across the loop (it moves to s3) but the
   truncation is STILL hoisted - the latch delay slot becomes `move v0,s3` and the
   frame grows to 0x28, 92.00%. So the hoist is cse's, not register pressure.
void func_8007E7E4(u8 **arg0) {
    s32 flag = (*arg0)[2] != 0;
    s32 addr = (s32)D_800CCD62;

    do {
        if ((u8)flag) {
            *(u16 *)addr |= BattleStatusBitMask((u8)((*arg0)[1] + 7));
        } else {
            *(u16 *)addr &= ~BattleStatusBitMask((u8)((*arg0)[1] + 7));
        }
        addr += 0x170;
    } while (addr < (s32)D_800CD1B2);
} */
void BattleGetFontGlyph(u8 **arg0) {
    u8 *p = *arg0;
    s32 idx = p[2] * 8 + p[1] * 0x40;

    g_BattleSlotParamTable[(u32)(idx + 0x140)] = p[3];
}
void BattleSetPendingCharIndex(BattleCommand **ppCmd, u8 arg1) {
    u16 *pRegs = g_BattleSlotRegs[arg1].wordRegs;

    g_BattlePendingCharIndex = BattleFindFirstFlagTableMatch(pRegs[(*ppCmd)->operand1]) + 1;
}
void BattleResetDisplayValues(void) {
    u8 buf[0x10];

    BattleResetSlotDisplayValues(buf);
}
s32 BattleSetTextScale(BattleCommand **ppCmd, u8 arg1) {
    u8 *pRegs = g_BattleSlotRegs[arg1].byteRegs;

    return pRegs[(*ppCmd)->operand1] == (*ppCmd)->operand2;
}
s32 BattleWordRegEqImm(u8 **arg0, s32 arg1) {
    u8 *p = *arg0;
    s32 value = p[2] | (p[3] << 8);
    u16 *pField = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    return pField[p[1]] == value;
}
s32 BattleByteRegLeImm(BattleCommand **ppCmd, u8 arg1) {
    u8 *pRegs = g_BattleSlotRegs[arg1].byteRegs;

    return (*ppCmd)->operand2 >= pRegs[(*ppCmd)->operand1];
}
