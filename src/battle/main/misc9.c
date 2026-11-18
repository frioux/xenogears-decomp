#include "common.h"
#include "battle/actor.h"

// One slot of the per-slot operator register block at g_BattleSlotRegs (stride 0x40):
// the same arguments in 32-, 16- and 8-bit form. g_BattleSlotRows, g_BattleSlotWordRegs and
// D_800D3430 are its +0x10, +0x20 and +0x30 arrays; the handlers below read the
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
extern u8 D_800D3430[];
extern u8 D_800C3EB7[];
extern u8 g_BattleSlotStatusRows[];
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
   base or spills), `(s32)g_BattleSlotWordRegs` staged local (76.10%, shape right but one
   extra insn) and the plain inline expression (the $at form);
   (2) retail steals `i++` into the beqz delay slot after the BattleSlotFlagsC002ClearRecordIdle
   call, gcc emits it at the loop head;
   (3) declaration order of the two zeroed accumulators (retail i then count).
void func_8007D8C0(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 count = 0;
    s32 i = 0;
    s32 idx;
    u16 *pField = (u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40);

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
        u8 *pBase = g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40;

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
   two register terms first and emit `sh v0,g_BattleSlotWordRegs(a0)`, i.e. maspsx's
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
    u16 *pRow = (u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40);

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
        u8 *pBase = g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40;

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
   base/pointer split and the (u16 *)((arg1 & 0xFF) * 0x40) + (s32)g_BattleSlotWordRegs
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
    s32 rowBase = (arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs;
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
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

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
       u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
       pOut[(*arg0)[2]] = value;

   That reproduces retail's andi/sll/lw p/lui a1/addiu a1/lbu p[2]/addu a0,a0,a1/
   sll/addu/sh exactly (the (s32) cast stops gcc folding the symbol into the
   store as %lo(sym)(reg), and the separate `value` statement keeps the address
   computation after the two jal's instead of hoisting it). Remaining, 3 insns:
   (1) retail keeps a temp copy of the row base (addu v1,v1,v0 then move s5,v1,
   and the la lands in v0 before the remaining saves) where gcc computes
   addu s5,v1,a0 with the la in a0 after them - tried the plain pointer form,
   (arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs, and an s32 base local converted to
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
    u16 *pRow = (u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40);

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
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

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
    u16 *pRow = (u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40);

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
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

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
