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

/* R370: 93.64% (344 bytes; retail 348 = 87 insns). The family's row-base copy
   (residual (b), 89.33 -> 93.64) IS source-controllable: give the LOOP its own
   pointer local (`pField = pRow;` after the head store, the loop reads pField) and
   cc1 keeps the sum pseudo live past the copy, so it emits retail's
   `addu v1,v1,v0` / `move s5,v1` instead of coalescing the two. Not measured yet
   on DE78/DA1C/DB78/DCF8/DFD4, which share the shape. Remaining: ONE instruction,
   the `i++`. Retail's `addiu s1,s1,1` is in the beqz delay slot right after the
   ClearRecordIdle call, so its source incremented i BETWEEN the call and the test;
   with `idx = i++` cc1 emits the increment at the loop head and reorg copies
   `slti v0,s1,3` into the FIRST slot instead of the second - 1 short, cannot link.
   Measured worse on top of the two-pointer body: `s32 idle` and `u8 idle` holding
   the call result with `i++` between call and test (87.10%, 332 bytes; idx/i move
   to s2/s0 and the body loses 4 instructions). The body below is the 93.64 one.
   ---- R309 (superseded): 89.33% (340 bytes, retail 348) using func_8007DE78's solved tail - a
   `u16 value` local, then `u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 +
   (s32)g_BattleSlotWordRegs);` and `pOut[(*arg0)[2]] = value;` reproduces
   retail's andi/sll/lw p/lui/addiu/lbu p[2]/addu/sll/addu/sh exactly. The 2
   missing instructions are the base copy (`addu v1,v1,v0` + `move s5,v1` where
   gcc coalesces an `addu s5,v1,a0`; the typed
   g_BattleSlotRegs[arg1 & 0xFF].wordRegs view and an s32 rowBase local measure
   the same) and one duplicated `slti v0,s1,3` reorg puts in a beqz delay slot.
   Previous best: 80.37%. Same family as func_8007DA1C/DB78/DCF8/DE78/DFD4 (all still
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

   R316: 89.33% (340 bytes against retail's 348, read off this stub's own .s:
   0x8007D8C0..0x8007DA1C = 87 insns, NOT the 336 a park note here claimed).
   func_8007DE78 has a byte-for-byte identical instruction stream (its only
   difference is ClearRecordIdle's second argument, 0 here and 1 there), so this
   body scores the same on both and func_8007DA1C/DB78/DCF8/DFD4 are the same
   family with different constants and (DB78/DCF8/DFD4) an extra
   g_BattleSlotFlags gate.

   The family has exactly TWO residuals and neither is source-controlled:
   (a) the `i++`. Retail's RTL has `addiu s1,s1,1` AFTER the
   BattleSlotFlagsC002ClearRecordIdle call and immediately before its `beqz`, so
   reorg fills that delay slot with it; with `idx = i++` gcc puts the increment
   at the loop head instead and reorg then fills the same slot with a COPY of the
   loop test (`slti v0,s1,3`) taken from the branch target. That is one
   instruction of the eight bytes we are short, and it also shifts the tail
   `beqz s2` fill (`addiu a1,s2,-1` vs `move a0,zero`). Measured dead: for-head,
   while-head, do-while and `idx = i; i++;` all put the increment in the same
   place, and `for (...; i++)` + `idx = i` is worse - gcc propagates idx and the
   s3 copy disappears.
   (b) the row base copy. Retail materialises the sum in $v1 and COPIES it into
   $s5 (`addu v1,v1,v0` / `move s5,v1`) and the `la` lands before the remaining
   saves; every spelling here folds it into the pointer's own register
   (`addu s5,v1,a0`) - measured: plain pointer initialiser, the
   (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs) form used below,
   base-first form, s32 base local, s32 base + u16* conversion.
   The tail store IS solved (u16 value local + int-built base + pointer local):
   it reproduces retail's andi/sll/lw p/lui a1/addiu a1/lbu p[2]/addu/sll/addu/sh
   exactly. Because (a) is one instruction at retail's exact size this body can
   never link, so it stays a stub.
void func_8007D8C0(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 idx;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    pField = pRow;
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
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007D8C0);
/* best: 95.38% (352 bytes; retail 0x8007DA1C..0x8007DD1C = 87 insns / 348
   bytes, this body 88). R398: the func_8007D8C0 R370 two-pointer recipe DOES
   transfer to this stub - pField = pRow after the head store, the index-first
   base with an (s32) cast on the symbol and a u16 value local at the tail - and
   the loop has to be a while on i from 3 to 0xB: 75.77 -> 95.38 in one step.
   The ONE remaining instruction is the loop increment. Retail has
   [jal ClearRecordIdle][andi v0,v0,0xFF][beqz v0][addiu s1,s1,1], i.e. i is
   incremented unconditionally between the call and the test on its result, so
   reorg fills that delay slot with the increment and duplicates slti only into
   the beqz v1 slot and the latch (two copies). With idx = i++ gcc increments at
   the loop head, so that slot gets a COPY of the target slti instead and the
   body carries three copies - one instruction too many to link. Measured, all
   on top of this body: increment between call and test via an s32 idle local
   88.25% (336 bytes; the allocator swaps count and idx, the frame shrinks);
   increment at the body end after the if 88.67% (332 bytes; the idx copy in s3
   disappears); the same loop written do-while 93.77% (344 bytes, one SHORT -
   slti is not duplicated at all); idx = i; i++; is identical to idx = i++.
   Cosmetic only: the prologue emits the s6 = arg1 copy before the count and i
   inits (retail after them), and the tail beqz s2 slot holds move a0,zero where
   retail has addiu a1,s2,-1. Not a preset question (cdk 76.66% on sibling DB78).
 */
// R455 remeasured 95.38% (352 bytes, 88 insns vs retail 87) - the parked body is still the best. New measurements on top of it, all stopping one instruction short: the 'ok' shape (result in an s32 local, i++ between the call and the test, idx = i at the body bottom) puts the increment in retail's beqz slot and the copy in retail's latch slot but folds the pre-loop copy into a prologue move s3,3 (91.46% / 86 insns; with 'idx = i' as a statement 92.51%; with count declared first 90.75%). Loop copy at the bottom only (idx = i as the last body statement) 89.03%. So: retail's preheader move s3,s1 comes only from idx = i++ and retail's beqz-slot increment only from the ok shape; no spelling gives both, and the two reorg duplicates of slti follow from which one is used.
/*
void func_8007DA1C(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 idx;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    pField = pRow;
    while (i < 0xB) {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 0) != 0) {
            if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    }
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}*/
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
/* R333: best 84.33%, 376 bytes against retail's 384 (0x8007DCF8..0x8007DE78 =
   96 insns, read off this stub), i.e. this body is TWO instructions short and
   can never link. It is the func_8007D8C0/DA1C/DB78/DE78/DFD4 family -- the
   same loop with the counter starting at 3, the bound 0xB, the flag-gate byte
   offset starting at 0x18, and ClearRecordIdle's second argument 0 -- and its
   two residuals are the family's documented ones, both already measured dead in
   several spellings by earlier rounds:

   (a) the `i++`. Retail's RTL is [jal ClearRecordIdle][andi v0,v0,0xFF]
   [addiu s1,s1,1][beqz], so the increment executes unconditionally right after
   the call and reorg puts it in that beqz's delay slot; with `idx = i++` gcc
   emits the increment at the loop head (move s3,s1 / addiu s1,s1,1) and reorg
   fills that slot with a COPY of the loop test (slti v0,s1,0xB) instead. The
   for-loop form (`for (i = 3; i < 0xB; i++) { idx = i; ... }`) is worse: gcc
   propagates idx out of s3 and the frame shrinks to 0x38 with a different
   saved set. `idx = i; i++;`, the increment at the loop bottom after the
   `if`, and while-head/do-while forms all leave it at the head.
   (b) the row-base copy. Retail forms the sum EARLY, before the remaining
   saves, into a temp (`andi v0,s7,0xFF / sll v0,v0,6 / lui v1 / addiu v1 /
   addu v0,v0,v1`) and then COPIES it into the loop's saved register
   (`addu s6,v0,zero`); gcc sinks the symbol's `lui/addiu` to the store and
   coalesces the sum into s6 (`addu s6,v1,a0`), one instruction fewer. Measured
   identical (84.33%): the typed `g_BattleSlotRegs[arg1 & 0xFF].wordRegs` view
   (relocation g_BattleSlotRegs+0x20, same code), the base-first
   `(u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40)`, and an s32
   `rowBase` local with `(u16 *)rowBase`.
   The tail store and the latch ARE solved and this body reproduces them
   byte-exactly: `u16 value` local, then an int-built base with the symbol
   cast, then a pointer local whose index is (*arg0)[2] (retail's andi/sll/
   lw/lui/addiu/lbu/addu/sll/addu/sh), and [slti v0,s1,0xB][addiu s4,s4,8 in
   the bnez slot] at the latch. The gate read is a u8 byte view with an
   incrementing offset, which is what produces retail's `$at` form
   (lui $at / addu $at,$at,$s4 / lbu %lo($at)).

   Preset screens on this body: gcc-2.6.0 85.32%, gcc-2.7.2-cdk 75.83%, so
   Default (2.7.2, -O2 -G8) is right and this is not a compiler question.
   permute.sh --minutes 3 -j 2: PERMUTER_BETTER base=1500 best=1100, but the
   candidate reads 85.39% at 368 bytes -- it keeps the scaled row offset live
   across the loop in an s32 local (bringing the early `andi a1,a1,0xFF / sll
   a1,a1,6` back) and then uses that local at the tail store, which DELETES
   retail's tail andi/sll: a worse body (4 instructions short), so it was not
   taken. Reducing (b) once in exchange for (a) twice is the trade the scoring
   rewards and linking does not.

void func_8007DCF8(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 offset = 0x18;
    s32 idx;
    u16 *pRow = (u16 *)(g_BattleSlotWordRegs + (arg1 & 0xFF) * 0x40);

    pRow[(*arg0)[2]] = 0;
    while (i < 0xB) {
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
 */
// R455: the R398 two-pointer recipe transfers to DE78 - pField = pRow after the head store, index-first base with the (s32) cast on the symbol, u16 value local: 89.33 -> 93.64% (344 bytes, 86 insns; retail 87). That is the parked body. The one missing instruction is reorg's duplicate of slti in the beqz v1 delay slot (mine threads addiu v0,sp,16 there instead) plus the inc's position; the preheader row-base copy (addu s5,v1,zero) IS reproduced by this body.
// Also measured R455: the 'ok' shape (u8 result in an s32 local, i++ between the call and the test, idx = i at the body bottom so the copy lands in the latch slot) puts the increment in retail's beqz slot and the copy in retail's latch slot, but gcc then constant-folds the pre-loop copy into a prologue move (move s3,zero) and drops the s3 save order: 91.33% / 86 insns. Same on DA1C (90.75-92.51). Not the answer, but it localises the wall: retail's preheader 'move s3,s1' only appears with idx = i++ (95.38 on DA1C), and retail's beqz-slot increment only appears with the ok shape - no spelling measured gives both.
/*
void func_8007DE78(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 idx;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);
    u16 *pField;

    pRow[(*arg0)[2]] = 0;
    pField = pRow;
    do {
        idx = i++;
        if (BattleSlotFlagsC002ClearRecordIdle(idx, 1) != 0) {
            if (pField[(*arg0)[3]] & BattleSlotWordVarGetSet(idx, (*arg0)[1], 0, 1)) {
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
}*/
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
/* R316: 85.10%, 376 bytes against retail's 380 (0x8007DFD4..0x8007E150 = 95
   insns), with this body: the while form, idx = i++, the gate read as
   ((u8 *)g_BattleSlotFlags)[offset] with offset += 8, and D8C0's solved tail
   recipe. 4 bytes short, and the missing instruction is the family's `i++`
   residual (see D8C0's note): retail's first `beqz` delay slot holds
   `addiu s1,s1,1` and its second and third hold copies of `slti v0,s1,3`,
   whereas here the loop head holds the increment and the FIRST slot takes the
   slti copy while slots two and three take `move a0,s0` / `addiu v0,sp,16`.
   Retail's `move s6,v0` row-base copy (D8C0 residual (b)) is a second
   instruction it has and we do not. Everything else - the `$at`-form gate load
   (lui $at / addu $at,$at,$s4 / lbu %lo($at)), the latch `addiu s4,s4,8`, the
   frame and the saved set - is byte-exact. gcc-2.7.2-cdk scores 76.66% on it,
   so the preset is not the answer.
void func_8007DFD4(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 offset = 0;
    s32 idx;
    u16 *pRow = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

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
    i = 3;
    if (count != 0) {
        u16 value = BattleSlotBitMask(buf[GameRandomByteRange(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)g_BattleSlotWordRegs);

        pOut[(*arg0)[2]] = value;
    }
}
 */
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
