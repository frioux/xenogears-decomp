#include "common.h"
#include "main/game.h"

// The sound the battle menu plays when it refuses an input; battle/main/misc12 holds the same
// define for the units that do not include include/battle/actor.h either.
#define BATTLE_SOUND_MENU_DENIED 0x4F
// Command byte of a command record that holds no command: the fill loop that runs before a
// command is queued writes it to every record's byte (battle/main/misc13 has the same define).
#define BATTLE_COMMAND_NONE 0xFF
// The low bits of a g_BattleCommandEntries flags word that pick the target mode.
#define BATTLE_COMMAND_TARGET_MODE_MASK 0x7
// The actor slots the u16 slot mask walks, party slots included: BATTLE_ACTOR_SLOT_COUNT and
// BATTLE_PARTY_SLOT_MASK are include/battle/actor.h's, which this unit does not include.
#define BATTLE_ACTOR_SLOT_COUNT 11
#define BATTLE_PARTY_SLOT_MASK 0x7
#define BATTLE_OTHER_SLOT_COUNT (BATTLE_ACTOR_SLOT_COUNT - MAX_PARTY_MEMBERS)
// The digits g_BattleNumberDigits holds, one per: BattleParseNumberToDigits walks them.
#define BATTLE_NUMBER_DIGIT_COUNT 9

// Command slot descriptor: one 0x40-byte entry per battle actor slot, in the
// head array of the block g_BattleCommandQueue points at.
typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ u8 targetSlot; // the actor slot this entry targets
    /* 0x3D */ u8 pad3D[3];
} BattleCommandSlot; // Size: 0x40

// Partial view of the battle command queue (g_BattleCommandQueue): the head slot array
// and the queued-command counter. src/battle/main/misc4.c holds the same block
// under the same name.
typedef struct {
    /* 0x000 */ BattleCommandSlot slots[4];
    /* 0x100 */ u8 unk100[0x1C4];
    /* 0x2C4 */ u8 unk2C4[8];
    /* 0x2CC */ u8 unk2CC[0xE];
    /* 0x2DA */ u8 count; // queued commands
    /* 0x2DB */ u8 commandFinished; // cleared while a queued command runs, set once it has finished
    /* 0x2DC */ u8 unk2DC;
    /* 0x2DD */ u8 unk2DD[9];
    /* 0x2E6 */ u8 commandId; // the picked command: indexes the 0x10-byte g_BattleCommandEntries rows
} BattleCommandQueue; // Size: 0x2E7

// Partial view of the per-frame battle display state (g_BattleDisplayState); the shared
// type is BattleWork in include/battle/battle_work.h.
typedef struct {
    /* 0x000 */ u8 unk0[0x7C];
    /* 0x07C */ u8 slotMarks[3]; // raised for each slot the action's target mask covers
    /* 0x07F */ u8 unk7F[0x1D];
    /* 0x09C */ u8 spriteColumnPending;
    /* 0x09D */ u8 menuColumnPending;
    /* 0x09E */ u8 cursorPending;
    /* 0x09F */ u8 unk9F[0x10];
    /* 0x0AF */ u8 unkAF;
    /* 0x0B0 */ u8 primFlags[4]; // per slot: prim flags the *PrimFlags*Slots pairs below raise
    /* 0x0B4 */ u8 unkB4[3];
    /* 0x0B7 */ u8 primState; // which prim set is up; func_800745EC switches five ways on it, 0 for none
    /* 0x0B8 */ u8 unkB8[0xE];
    /* 0x0C6 */ u8 pulseTriangleActive;
    /* 0x0C7 */ u8 unkC7[4];
    /* 0x0CB */ u8 primListsPending;
} BattleDisplayState; // Size: 0xCC

extern BattleCommandQueue *g_BattleCommandQueue;
extern u8 g_BattleNumberDigits[];
extern BattleDisplayState *g_BattleDisplayState;
extern u8 g_BattleSoundEnabled;
extern int ArchiveDataSync(void);
extern void BattlePlaySoundWithBaseId(u8 arg0);
extern void BattleUpdateFrame(void);
extern void BattleFreePacketPrimitives(void);
extern void BattleClearFlagC6(void);
extern void BattleFreeSlotBuffers(s32 arg0);
extern u16 BattleSlotBitMask(u8 arg0);
extern void BattleBeginAnimTask(u16 arg0);
extern void BattleApplySlotMaskToEffects(u16 arg0);
extern u16 BattleMaskHasSlotBit(u16 arg0, u8 arg1);
extern u16 BattleSlotBitMaskInverted(u8 arg0);
extern u8 D_800C400B[];
extern u16 D_800C4022[];
extern u8 D_800C402F[];
extern u8 D_800CCD1C[];
extern u8 g_BattleCommandGridIds[];
typedef struct {
    /* 0x0 */ u16 flags; // & 7 picks the target mode, 0xF and 0x1000/0x4000 pick the drawn icon
    /* 0x2 */ u8 pad2[0xE];
} BattleCommandEntry; // Size: 0x10

extern BattleCommandEntry g_BattleCommandEntries[];
extern u8 g_BattleCommandGridAltUses[];
extern u8 g_BattleCommandGridAltIds[];
extern u8 g_BattleCommandRecords[];
extern u8 BattleRunTargetSlotSelection(u16 arg0, u8 arg1, s32 arg2);
extern s32 BattleIsCharacterInParty(s32 arg0);
extern s32 BattleMaskHasStatusBit(s32 mask, u8 idx);
extern void func_8009A854(u8 arg0);
extern u16 g_BattleActionSlotMask;
extern u16 g_BattleActionParam;
extern u8 g_BattleSlotFlags[];
extern u16 g_BattleTargetSlotMask;
extern u8 g_BattleTargetSlot;
extern u8 g_BattleCellActionRows[];
extern u8 D_800CCD38[];
extern u8 g_BattleActorCharIds[];
extern u8 g_BattleActionRows[];
extern void BattleClearSlotCurveEntry(void);
extern void func_80079840(u8 arg0, u8 arg1);
extern void BattleQueueCommandFE(u8 arg0);
extern void BattleApplySlotCurveAndStep(u8 arg0);
extern void BattleBeginActorAction(u8 arg0, u16 arg1, u16 arg2);
extern s32 BattleFindMinValueOtherSlot(u8 arg0);
extern void func_800B89FC(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
extern s32 func_8008C81C(u8 arg0);
extern void BattleNoOpCallback(s32 arg0, s32 arg1);
extern void func_800877E0(u8 arg0, u8 arg1);
extern void func_80087EDC(u8 arg0, u8 arg1);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern u8 D_800C4928;
extern u8 g_BattleSlotCurveRunning;
extern u8 g_BattleCommandCellRow;
extern u8 g_BattleCommandCellColumn;

// Partial view of the battle command cell state based at g_BattleCommandCellFlags: the command flags
// word, the per-cell use counters (g_BattleCommandCellUses) and the per-cell command ids
// (D_800D2CE0, which the rest of the code reaches as g_BattleCommandGridIds), the last
// two indexed by the cursor cell.
typedef struct {
    /* 0x00 */ u16 flags;
    /* 0x02 */ u8 unk2[0x10];
    /* 0x12 */ u8 uses[0x30];
    /* 0x42 */ u8 ids[0x10];
} BattleCommandCellState; // Size: 0x52

extern BattleCommandCellState g_BattleCommandCellFlags;

void BattlePlaySoundIfEnabled(u8 arg0) {
    if (g_BattleSoundEnabled != 0) {
        BattlePlaySoundWithBaseId(arg0);
    }
}
void BattleParseNumberToDigits(u32 arg0) {
    s32 i;
    u32 divisor;

    divisor = 100000000;
    for (i = 0; i < BATTLE_NUMBER_DIGIT_COUNT; i++) {
        g_BattleNumberDigits[i] = arg0 / divisor;
        arg0 %= divisor;
        divisor /= 10;
    }
    for (i = 1; i < BATTLE_NUMBER_DIGIT_COUNT; i++) {
        if (g_BattleNumberDigits[i] == 0) {
            g_BattleNumberDigits[i - 1] = 0xFF;
        } else {
            if (g_BattleNumberDigits[i - 1] == 0) {
                g_BattleNumberDigits[i - 1] = 0xFF;
            }
            break;
        }
    }
}
void BattleSetArchive20Entry0(void) {
    ArchiveSetIndex(0x20, 0);
}
void BattleSetArchive20Entry2(void) {
    ArchiveSetIndex(0x20, 2);
}
void BattleSetArchive20Entry3(void) {
    ArchiveSetIndex(0x20, 3);
}
s32 BattleAllocHeapBlock(s32 arg0, s32 arg1) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc(arg0, arg1);
}
s32 BattleAllocTextBuffer(s32 arg0) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc((arg0 + 3) * 26, 0);
}
void BattleWaitForArchiveSync(void) {
    while (ArchiveDataSync() != 0) {
        BattleUpdateFrame();
    }
}
void BattleQueueCommandF3WithSlotMask(u16 arg0, u8 arg1) {
    u16 mask;
    s32 i;
    s32 off;
    u8 idx;

    mask = arg0 & ~BATTLE_PARTY_SLOT_MASK;
    for (i = 0, off = 0x450; i < BATTLE_OTHER_SLOT_COUNT; i++) {
        idx = i + MAX_PARTY_MEMBERS;
        if (BattleMaskHasSlotBit(mask, idx) != 0) {
            if ((*(u16 *)(D_800CCD1C + off) & 0x800) == 0) {
                mask &= BattleSlotBitMaskInverted(idx);
            }
        }
        off += 0x170;
    }
    if (mask == 0) {
        return;
    }
    D_800C400B[g_BattleCommandQueue->count * 72] = arg1;
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xF3;
    D_800C4022[g_BattleCommandQueue->count * 36] = mask;
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008ADD0);
/* Best: 98.37% (205 insns vs the original's 206 -- ONE instruction short, so it
   cannot link yet; the 96.69% park was 204). The lever that produced the pointer
   register retail wants: assign `new_var = &g_BattleActionSlotMask;` BEFORE the
   g_BattleSlotFlags branch and copy it (`pMask = new_var;`) at the use site. Without
   that early assignment gcc 2.6 substitutes the constant address into every use
   (no saved register, 3 saved regs instead of retail's 4 s0-s3, 204 insns).
   What is left is only WHERE the address is formed, now that the register exists:
   retail puts `nop; bnez` after the g_BattleSlotFlags load and forms s1 (= &g_BattleActionSlotMask)
   AT the first use (BattleBeginAnimTask's argument); gcc forms s3 two
   instructions earlier, filling the slots retail leaves as `nop`, and so the two
   straight-line uses read 0(s3) instead of retail's fresh lui/addiu+lhu.
   Also still off: `li v1,0xff / li v0,0x8b8` comes out swapped (value/off tie),
   `li <i>,0x3` sits one call later than retail's BattleClearSlotCurveEntry
   delay slot, and the saved-register numbering is s3=pointer where retail has
   s1 (retail s3=arg0, s2=i). Permuter run from the 96.69% body found the
   new_var lever (base=1011 best=472); a second run from THIS body is the next
   step. Already ruled out: -G8/-G0 preset (99.73% under -G0, still short),
   u16 pointer, register and volatile pMask, pMask[0], moving the
   assignment site, -O1/-O3. */
/*
void func_8008ADD0(u8 arg0) {
    s32 off;
    s32 i;
    u16 mask;
    u16 sel;
    u8 value;
    u16 *new_var;
    u16 *pMask;

    g_BattleSoundEnabled = 0;
    BattleApplySlotMaskToEffects(0);
    value = 0xFF;
    for (off = 0x8B8; off >= 0; off -= 0x48) {
        D_800C402F[off] = value;
    }
    new_var = &g_BattleActionSlotMask;
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        g_BattleCommandQueue->unk2DC = g_BattleCommandQueue->commandId + 0x17;
        off = (g_BattleCommandQueue->commandId + 0x16) * 40 + arg0 * 1520;
        sel = *(u16 *)(g_BattleCellActionRows + off) & 7;
    } else {
        g_BattleCommandQueue->unk2DC = g_BattleCommandQueue->commandId + 0x16;
        off = (g_BattleCommandQueue->commandId + 0x15) * 40 + arg0 * 1680;
        sel = *(u16 *)(g_BattleActionRows + off) & 7;
    }
    if (sel != 0) {
        mask = g_BattleTargetSlotMask;
    } else {
        mask = BattleSlotBitMask(g_BattleTargetSlot);
    }
    pMask = new_var;
    BattleQueueCommandF3WithSlotMask(mask, arg0);
    BattleClearSlotCurveEntry();
    BattleBeginActorAction(arg0, mask, g_BattleCommandQueue->unk2DC - 1);
    BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
    mask = BattleSlotBitMask(arg0);
    BattleBeginAnimTask(*pMask | mask);
    func_800B89FC(1, arg0, g_BattleCommandQueue->slots[arg0].targetSlot, BattleFindMinValueOtherSlot(arg0));
    D_800C402F[g_BattleCommandQueue->count * 72] = g_BattleActionParam;
    D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
    *(u16 *)(g_BattleCommandRecords + g_BattleCommandQueue->count * 72) = *pMask;
    for (i = 3; i < 11; i++) {
        if (BattleMaskHasSlotBit(*pMask, i) != 0) {
            func_80079840(arg0, i);
        }
    }
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    BattleQueueCommandFE(arg0);
    while (g_BattleCommandQueue->commandFinished == 0) {
        BattleUpdateFrame();
    }
}
*/

void BattleClearPrimFlagsFourSlots(u8 arg0) {
    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 0;
    g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = g_BattleDisplayState->primFlags[2] = g_BattleDisplayState->primFlags[3] = 0;
    g_BattleDisplayState->primState = 0;
    if (arg0 == 0) {
        g_BattleDisplayState->primListsPending = 1;
    }
}
void BattleSetPrimFlagsFourSlots(u8 arg0) {
    u16 value;

    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 1;
    g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = g_BattleDisplayState->primFlags[2] = g_BattleDisplayState->primFlags[3] = 1;
    g_BattleDisplayState->primState = 1;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot);
    BattleBeginAnimTask(value);
    BattleApplySlotMaskToEffects(BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot));
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B224);
/* Best: 89.19% (584 bytes vs the original's 596). Same calls, same branch tree,
   same register set (s0=arg0, s1=arg0*0x170 offset, s2=inParty, s3=count,
   s4=entry, s5=canAct, s6=result); what is left is two shapes:
   1. the D_800CCD38 accesses. Retail keeps only the OFFSET in s1 and re-folds
      `lui at,%hi(D_800CCD38); addu at,at,s1; lhu/sh %lo(at)` at each of the
      three sites; gcc forms base+offset once into s1 (`lui/addiu` + `addu`).
      Tried: three separate full spellings, an s32 off local, a u32 off.
   2. the arg1 + arg2*2 sum: retail computes it into a1 (`addu a1,a1,v0`, the
      incoming arg1) before the offset sum; gcc leaves arg1 in t0 (`move
      t0,a1` at entry) and computes the call argument last.
   Everything else, including the asymmetric second branch (retail's else path
   is `if ((u16)BattleMaskHasStatusBit(...) != 0) inParty = 1;`, not an
   assignment -- that asymmetry is what stops gcc cross-jumping the two calls),
   already matches instruction for instruction. NEXT: permute this body. It
   also needs `#include "main/game.h"`, D_800CCD38/g_BattleActorCharIds and
   BattleMaskHasStatusBit, which are declared above.
   Round 146 re-tested this body against today's externs: still 89.19%, same two
   shapes of the note. New data point on shape 1: retail keeps ONLY the product in
   s1 (`s1 = arg0 * 0x170`, built once after the branch merge) and re-forms
   `lui at,%hi(D_800CCD38); addu at,at,s1` at all three sites -- including the load
   and the store of the `-= count`, which is NOT CSE'd. Tried this round and still
   89.19%: `extern u8 D_800CCD38[0x170]` (bounded) instead of `[]`, and spelling the
   index inline as `D_800CCD38 + arg0 * 0x170` instead of through the `off` local.
   gcc 2.6 materialises the whole symbol+offset address into s1 either way. Shape 2
   as it now reads: retail copies arg1 at entry (`move v1,a1`) for the masked
   offset term and computes the call's `arg1 + arg2*2` destructively on the raw a1
   (`addu a1,a1,v0`) BEFORE the offset sum; ours copies arg1 to t0 and computes the
   call argument last. */
/*
s32 func_8008B224(u8 arg0, u8 arg1, u8 arg2) {
    s32 result;
    u8 canAct;
    u8 inParty;
    u16 entry;
    u8 count;
    s32 off;

    result = 0;
    canAct = 1;
    inParty = 0;
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        off = (arg2 * 2 + arg1 + 0x16) * 40 + arg0 * 1520;
        entry = *(u16 *)(g_BattleCellActionRows + off);
        count = *(u8 *)(g_BattleCellActionRows + off + 0x13);
        inParty = (u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].unk6, arg1 + arg2 * 2) != 0;
    } else {
        off = (arg2 * 2 + arg1 + 0x15) * 40 + arg0 * 1680;
        entry = *(u16 *)(g_BattleActionRows + off);
        count = *(u8 *)(g_BattleActionRows + off + 0x13);
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].unkA, arg1 + arg2 * 2) != 0) {
            inParty = 1;
        }
    }
    if (*(u16 *)(D_800CCD38 + arg0 * 0x170) >= count && inParty != 0) {
        BattleClearPrimFlagsFourSlots(1);
        g_BattleDisplayState->pulseTriangleActive = 1;
        if (BattleRunTargetSlotSelection(entry, arg0, 0) != 0) {
            *(u16 *)(D_800CCD38 + arg0 * 0x170) -= count;
            result = 1;
        } else {
            g_BattleDisplayState->pulseTriangleActive = 0;
            BattleSetPrimFlagsFourSlots(arg0);
        }
        canAct = 0;
    }
    if (canAct != 0) {
        BattlePlaySoundIfEnabled(0x4F);
    }
    return result;
}
*/
extern u8 g_BattleMenuEvent;
extern u8 g_BattleCursorScrollState;
extern s32 g_BattleCursorScrollY;
extern u8 func_8008B224(u8 arg0, u8 arg1, u8 arg2);
extern void BattleInitSlotGraphics(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6);
extern void func_80091064(u8 arg0);
extern void BattleInitPulseTrianglePrims(void);
extern void BattleDrawCharStatusIfFlagged(u8 arg0, char arg1, u8 arg2);
extern void BattleBuildSpriteColumn8(s32 arg0);
extern void func_80091EC4(s32 arg0);
extern void BattleStepAnimatedCursorPrim(s32 arg0, s32 arg1, s32 *pIndex, u8 *pCount);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B478);
/* Best: 98.42% (1176 vs the original's 1168; the switch table is at .rodata+0 so
   this function's table is the unit's first). Two shapes left:
   1. the fall-through state arm (case 4 of the switch on g_BattleCursorScrollState):
      retail keeps the compare's `(j - 1) * 16` in $v0 across the `j--` and stores it
      through the shared tail at .L8008B878; gcc 2.6 rematerialises it there
      (`andi v0,(u8)j; sll v0,v0,4`, +2 insns). Cases 1..3 reuse v0 fine. Tried this
      round: an s32 nextY local holding the compare value (1192, both the initializer
      and the assignment form), the store before the `j--` (1192),
      `g_BattleCursorScrollY = (j - 1) * 16` with j-- after (1192), `(j - 1 + 1) * 16`
      (1176, in the body below).
   2. the i/j tie: retail keeps i in $s2 and j in $s3, gcc gives i $s3 (the prologue's
      two sw/move save pairs swap order with it). Swapping the declarations of i and j
      does not move it.
   Permuter: 1487 iterations, base=439, and the only lower candidates (384, 419) got
   their score by substituting prevI for j, which is wrong code. No usable lead.
   Everything else is identical: both switches' case order (5,4,0,2,1,3 and 1,2,3,4),
   the 0x50 frame, the andi placement, every delay slot, the jump table. */
/*
u8 func_8008B478(u8 arg0) {
    u8 i;
    u8 j;
    u8 result;
    u8 prevI;
    u8 prevJ;
    s32 prevY;
    s32 primIndex;
    u8 primCount;
    u32 iv;

    i = 0;
    j = 0;
    prevI = 0xFF;
    prevJ = 0xFF;
    prevY = 0xFFFF;
    result = 2;
    primIndex = 4;
    primCount = 0;
    g_BattleCursorScrollY = 0;
    g_BattleCursorScrollState = 0;
    g_BattleDisplayState->primListsPending = 0;
    BattleInitSlotGraphics(0, 0x1C, 0xA0, 0xAC, 0x38, 0, 1);
    BattleInitSlotGraphics(1, 0x20, 0x2C, 0x118, 0x70, 0, 1);
    BattleInitSlotGraphics(2, 0xD0, 0xA4, 0x50, 0x18, 0, 1);
    BattleInitSlotGraphics(3, 0xE6, 0xC0, 0x4C, 0x18, 0, 1);
    func_80091064(arg0);
    BattleInitPulseTrianglePrims();
    do {
        if (i != prevI || j != prevJ) {
            BattleDrawCharStatusIfFlagged(arg0, i, j);
            prevI = i;
            prevJ = j;
        }
        if (g_BattleCursorScrollY != prevY) {
            BattleBuildSpriteColumn8(g_BattleCursorScrollY);
            func_80091EC4(g_BattleCursorScrollY);
            prevY = g_BattleCursorScrollY;
        }
        iv = i;
        BattleStepAnimatedCursorPrim((iv & 1) * 128 + (iv & 1) * 4 + 0x2A, (iv >> 1) * 16 + 0x38, &primIndex, &primCount);
        BattleUpdateFrame();
        switch (g_BattleMenuEvent) {
        case 5:
            result = 0;
            break;
        case 4:
            if (func_8008B224(arg0, i, j) != 0) {
                result = 1;
                g_BattleCommandQueue->commandId = i + j * 2;
            }
            break;
        case 0:
            if (j * 2 + i != 0xF) {
                if (i + 1 == 0xC) {
                    g_BattleCursorScrollState = 1;
                } else {
                    i++;
                }
            }
            break;
        case 2:
            if (j * 2 + i > 0) {
                if (i == 0 && j != 0) {
                    g_BattleCursorScrollState = 2;
                } else {
                    i--;
                }
            }
            break;
        case 1:
            if (j * 2 + i < 0xE) {
                if (i + 2 >= 0xC) {
                    g_BattleCursorScrollState = 3;
                } else {
                    i += 2;
                }
            }
            break;
        case 3:
            if (j * 2 + i >= 2) {
                if (i < 2 && j != 0) {
                    g_BattleCursorScrollState = 4;
                } else {
                    i -= 2;
                }
            }
            break;
        }
        switch (g_BattleCursorScrollState) {
        case 1:
            if (g_BattleCursorScrollY >= (j + 1) * 16) {
                j++;
                i--;
                g_BattleCursorScrollY = j * 16;
                g_BattleCursorScrollState = 0;
            }
            break;
        case 2:
            if (g_BattleCursorScrollY < (j - 1) * 16) {
                j--;
                i++;
                g_BattleCursorScrollY = j * 16;
                g_BattleCursorScrollState = 0;
            }
            break;
        case 3:
            if (g_BattleCursorScrollY >= (j + 1) * 16) {
                j++;
                g_BattleCursorScrollY = j * 16;
                g_BattleCursorScrollState = 0;
            }
            break;
        case 4:
            if (g_BattleCursorScrollY < (j - 1) * 16) {
                j--;
                g_BattleCursorScrollY = (j - 1 + 1) * 16;
                g_BattleCursorScrollState = 0;
            }
            break;
        }
    } while (result == 2);
    BattleClearPrimFlagsFourSlots(result);
    BattleFreePacketPrimitives();
    BattleClearFlagC6();
    BattleFreeSlotBuffers(0);
    BattleFreeSlotBuffers(1);
    BattleFreeSlotBuffers(2);
    BattleFreeSlotBuffers(3);
    return result;
}
*/

void BattleActivateSelectedCommand(u8 arg0) {
    s32 off;
    s32 sel;
    u16 mask;
    s32 i;
    u8 value;
    BattleCommandCellState *pCell;
    u8 *pUses;
    u8 *pIds;

    g_BattleSoundEnabled = 0;
    BattleApplySlotMaskToEffects(0);
    value = BATTLE_COMMAND_NONE;
    for (off = 0x8B8; off >= 0; off -= 0x48) {
        D_800C402F[off] = value;
    }
    func_800B89FC(1, arg0, g_BattleCommandQueue->slots[arg0].targetSlot, BattleFindMinValueOtherSlot(arg0));
    sel = g_BattleCommandEntries[g_BattleCommandQueue->commandId].flags & BATTLE_COMMAND_TARGET_MODE_MASK;
    if (sel != 0) {
        mask = g_BattleTargetSlotMask;
    } else {
        mask = BattleSlotBitMask(g_BattleTargetSlot);
    }
    BattleQueueCommandF3WithSlotMask(mask, arg0);
    BattleClearSlotCurveEntry();
    BattleBeginAction(arg0, mask, g_BattleCommandQueue->commandId);
    pCell = &g_BattleCommandCellFlags;
    if ((pCell->flags & 0x8000) == 0) {
        pUses = pCell->uses;
        if (--pUses[g_BattleCommandCellRow * 2 + g_BattleCommandCellColumn] == 0) {
            pIds = pCell->ids;
            pIds[g_BattleCommandCellRow * 2 + g_BattleCommandCellColumn] = 0;
        }
    }
    g_BattleSlotCurveRunning = 1;
    BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
    g_BattleSlotCurveRunning = 0;
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xF5;
    D_800C4022[g_BattleCommandQueue->count * 36] = g_BattleActionParam;
    D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
    *(u16 *)(g_BattleCommandRecords + g_BattleCommandQueue->count * 72) = g_BattleActionSlotMask;
    for (i = MAX_PARTY_MEMBERS; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (BattleMaskHasSlotBit(g_BattleActionSlotMask, i) != 0) {
            func_80079840(arg0, i);
        }
    }
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    BattleQueueCommandFE(arg0);
    while (g_BattleCommandQueue->commandFinished == 0) {
        BattleUpdateFrame();
    }
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (BattleMaskHasSlotBit(mask, i) != 0) {
            g_BattleDisplayState->slotMarks[i] = 1;
        }
    }
}
void BattleClearPrimFlagsTwoSlots(u8 arg0) {
    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 0;
    g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = 0;
    g_BattleDisplayState->primState = 0;
    if (arg0 == 0) {
        g_BattleDisplayState->primListsPending = 1;
    }
}
void BattleSetPrimFlagsTwoSlots(u8 arg0) {
    u16 value;

    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 1;
    g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = 1;
    g_BattleDisplayState->primState = 2;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot);
    BattleBeginAnimTask(value);
    BattleApplySlotMaskToEffects(BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot));
}
s32 BattleActivateTwoSlotCommand(u8 arg0, u8 arg1, u8 arg2, u8 arg3) {
    u8 value;
    u16 entry;
    s32 result;

    result = 0;
    entry = g_BattleCommandEntries[g_BattleCommandGridIds[arg2 * 2 + arg1]].flags;
    if (arg3 == 0) {
        value = g_BattleCommandGridIds[arg2 * 2 + arg1];
    } else {
        value = g_BattleCommandGridAltIds[arg2 * 2 + arg1];
    }
    if (value != 0) {
        if (arg3 == 0) {
            BattleClearPrimFlagsTwoSlots(1);
            g_BattleDisplayState->pulseTriangleActive = 1;
            if (BattleRunTargetSlotSelection(entry, arg0, 0) != 0) {
                result = 1;
            } else {
                g_BattleDisplayState->pulseTriangleActive = 0;
                BattleSetPrimFlagsTwoSlots(arg0);
            }
        } else if (BattleIsCharacterInParty(value - 0x32) != 0) {
            func_8009A854(value);
            value = g_BattleCommandGridAltUses[arg2 * 2 + arg1] - 1;
            g_BattleCommandGridAltUses[arg2 * 2 + arg1] = value;
            if (value == 0) {
                g_BattleCommandGridAltIds[arg2 * 2 + arg1] = 0;
            }
            result = 2;
        }
    } else {
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
    }
    return result;
}
extern void func_8008FE18(u8 arg0, u8 arg1, u8 arg2);
extern void BattleDrawMenuCell(u8 arg0, u8 arg1, u8 arg2);
extern void func_8009023C(s32 arg0);
extern void func_8009093C(s32 arg0);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BED8);
/* Best: 99.42% (1156 bytes vs the original's 1160). ONE instruction short and
   the whole rest is identical instruction for instruction (jump table at
   .rodata+0x18, both switches' case order 5,4,0,2,1,3 and 1,2,3,4, the 0x70
   frame, every delay slot). What is missing is a `li s0,1` for the 7th
   argument of the two BattleInitSlotGraphics calls: retail materialises a fresh 1 (s0)
   even though `pagePending` is already 1 in s4 at that point, and gcc 2.6
   reuses s4 (`sw s4,0x18(sp)` and no li), so `sw s0,0x48(sp)` moves up by one
   slot too. Tried: moving the pagePending/page initialisation after
   BattleInitPulseTrianglePrims -- that DOES give the fresh li, but the
   sw/li save pair for s4/s5 then moves from the top of the frame (positions
   12-15) to just before the calls, which costs 4 mismatched instructions
   (97.17%); an s32 local for the 7th argument, and a `u8 sel` vs s32 sel
   (the u8 local is what gives retail's andi on the call result).
   Permuter: 439 iterations, base=178 best=178.
   The state arms are stores-before-increments: writing
   `g_BattleCursorScrollY = (j + 1) * 13; j++; i--;` is what reproduces
   retail's per-arm `andi v1,(u8)j; addiu v1,v1,1` plus the -1 shared by
   states 2 and 4 (writing `j++; ... scrollY = j * 13;` gives the
   rematerialised `andi v1,(u8)j` at the shared tail and is 5 insns short). */
/*
u8 func_8008BED8(u8 arg0) {
    u8 i;
    u8 j;
    u8 prevI;
    u8 prevJ;
    s32 prevY;
    u8 result;
    s32 pagePending;
    s32 page;
    u8 redraw;
    s32 primIndex;
    u8 primCount;
    u32 iv;
    u8 sel;

    i = 0;
    j = 0;
    prevI = 0xFF;
    prevJ = 0xFF;
    prevY = 0xFFFF;
    result = 2;
    pagePending = 1;
    page = 0;
    redraw = 1;
    primIndex = 4;
    primCount = 0;
    g_BattleCursorScrollY = 0;
    g_BattleCursorScrollState = 0;
    g_BattleDisplayState->primListsPending = 0;
    BattleInitSlotGraphics(0, 0x20, 0x58, 0x118, 0x78, 0, 1);
    BattleInitSlotGraphics(1, 0x10, 0x2C, 0x128, 0x28, 0, 1);
    BattleInitPulseTrianglePrims();
    do {
        if (pagePending != 0) {
            func_8008FE18(arg0, pagePending - 1, redraw);
            page = pagePending - 1;
            pagePending = 0;
            redraw = 0;
        }
        if (i != prevI || j != prevJ) {
            BattleDrawMenuCell(i, j, page);
            prevI = i;
            prevJ = j;
        }
        if (g_BattleCursorScrollY != prevY) {
            func_8009023C(g_BattleCursorScrollY / 13 * 2);
            func_8009093C(g_BattleCursorScrollY);
            prevY = g_BattleCursorScrollY;
        }
        iv = i;
        BattleStepAnimatedCursorPrim((iv & 1) * 128 | ((iv & 1) * 2 + 0x2A), (iv >> 1) * 13 + 0x63,
                                     &primIndex, &primCount);
        BattleUpdateFrame();
        switch (g_BattleMenuEvent) {
        case 5:
            result = 0;
            break;
        case 4:
            sel = BattleActivateTwoSlotCommand(arg0, i, j, page);
            switch (sel) {
            case 1:
                g_BattleCommandQueue->commandId = g_BattleCommandGridIds[j * 2 + i];
                g_BattleCommandCellColumn = i;
                g_BattleCommandCellRow = j;
                result = 1;
                break;
            case 2:
                pagePending = 2;
                break;
            }
            break;
        case 0:
            if (j * 2 + i != 0x2F) {
                if (i + 1 == 0x10) {
                    g_BattleCursorScrollState = 1;
                } else {
                    i++;
                }
            }
            break;
        case 2:
            if (j * 2 + i > 0) {
                if (i == 0 && j != 0) {
                    g_BattleCursorScrollState = 2;
                } else {
                    i--;
                }
            }
            break;
        case 1:
            if (j * 2 + i < 0x2E) {
                if (i + 2 >= 0x10) {
                    g_BattleCursorScrollState = 3;
                } else {
                    i += 2;
                }
            }
            break;
        case 3:
            if (j * 2 + i >= 2) {
                if (i < 2 && j != 0) {
                    g_BattleCursorScrollState = 4;
                } else {
                    i -= 2;
                }
            }
            break;
        }
        switch (g_BattleCursorScrollState) {
        case 1:
            g_BattleCursorScrollY = (j + 1) * 13;
            j++;
            i--;
            g_BattleCursorScrollState = 0;
            break;
        case 2:
            g_BattleCursorScrollY = (j - 1) * 13;
            j--;
            i++;
            g_BattleCursorScrollState = 0;
            break;
        case 3:
            g_BattleCursorScrollY = (j + 1) * 13;
            j++;
            g_BattleCursorScrollState = 0;
            break;
        case 4:
            g_BattleCursorScrollY = (j - 1) * 13;
            j--;
            g_BattleCursorScrollState = 0;
            break;
        }
    } while (result == 2);
    BattleClearPrimFlagsTwoSlots(result);
    BattleFreePacketPrimitives();
    BattleClearFlagC6();
    BattleFreeSlotBuffers(0);
    BattleFreeSlotBuffers(1);
    return result;
}
*/
void BattleFreeSlotPrims(u8 arg0) {
    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 0;
    g_BattleDisplayState->primState = 0;
    if (arg0 != 0) {
        g_BattleDisplayState->pulseTriangleActive = 0;
        BattleFreeSlotBuffers(0);
        BattleFreeSlotBuffers(1);
        BattleUpdateFrame();
        BattleFreePacketPrimitives();
        BattleClearFlagC6();
    } else {
        g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = 0;
    }
}
void BattleSetPrimFlagsTwoSlotsMode3(u8 arg0) {
    u16 value;

    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 1;
    g_BattleDisplayState->primState = 3;
    g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = 1;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot);
    BattleBeginAnimTask(value);
    BattleApplySlotMaskToEffects(BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot));
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C4A8);
/* Best: 88.52% (932 bytes vs the original's 884). Same calls, same branch tree,
   same instruction multiset in every block except two shapes:
   1. loop-invariant base hoisting. Retail re-forms each of the three command
      table bases inside the loop (`lui at,%hi(D_800C402F); addu at,at,v1;
      sb %lo(D_800C402F)(at)`); gcc 2.6 hoists all three symbols into s3/s4/s5 in
      the loop preheader (L8008C66C) and reaches them with `addu v0,v0,sN`,
      which also grows the frame from 0x28 to 0x38. The same expression outside
      the loop compiles to retail's at-form, so the source spelling is right and
      it is the `loop` pass doing it. Tried: a `do { } while (0);` block around
      the three stores (no change), three separate index locals (loads of
      g_BattleCommandQueue->count drop from three to one), `int slotStride = 0x40;`
      (fixes the addu operand order of item 2 but makes the tail index a real
      `mult` and costs 4 more registers), pointer locals.
   2. addu operand order at three sites: retail `addu v0,v0,s0` (base first),
      gcc `addu v0,s0,v0`. `(g_BattleCommandQueue + arg0 * slotStride)[0x3C]` with a
      variable stride gives retail's order but breaks item 1 further (see above);
      with the 0x40 literal the order is wrong at exactly the three sites where
      the pointer lands in $v0 rather than $a0. Both `g_BattleCommandQueue[arg0 * 0x40 +
      0x3C]` and `(g_BattleCommandQueue + arg0 * 0x40)[0x3C]` measured identical.
   Everything else matches, including the shared epilogue, `g_BattleDisplayState->primListsPending =
   result` / `D_800C4928 = result` both storing $s3 (the source has to store the
   `result` local there, not the literal 1), the `sb` in BattleFreeSlotPrims's
   jal delay slot, the `addiu v0,v0,0xFF` (255, not -1) form of `x - 1`, and the
   `slti/bnez` for-loop tail. `make check` was NOT run on the parked body.
   Round 124: g_BattleCommandQueue and g_BattleDisplayState are typed now (BattleCommandQueue /
   BattleDisplayState at the top of this file), so the body below compiles as
   written; the typed slot spelling `g_BattleCommandQueue->slots[arg0].targetSlot` is proven
   byte-exact by the four *PrimFlags*Slots setters that use it.
   NEXT: permute this body. */
/*
s32 func_8008C4A8(u8 arg0) {
    s32 result;
    s32 i;

    result = 1;
    if ((func_8008C81C(arg0) & 0xFF) != 0) {
        BattleNoOpCallback(arg0, 0x100);
        BattleFreeSlotPrims(1);
        g_BattleDisplayState->primListsPending = result;
        result = 0;
    } else {
        BattleNoOpCallback(arg0, 0x80);
        g_BattleDisplayState->unkAF = 0;
        BattleFreeSlotPrims(1);
        g_BattleSoundEnabled = 0;
        BattleApplySlotMaskToEffects(0);
        g_BattleCommandQueue->slots[arg0].targetSlot = g_BattleTargetSlot;
        func_800877E0(arg0, g_BattleCommandQueue->slots[arg0].targetSlot);
        D_800C4928 = result;
        BattleSetupActorSlotDisplay(arg0);
        if ((*(u16 *)(D_800CCD1C + g_BattleCommandQueue->slots[arg0].targetSlot * 0x170) & 0x800) != 0) {
            D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
            D_800C402F[g_BattleCommandQueue->count * 72] = 0xF3;
            D_800C4022[g_BattleCommandQueue->count * 36] = BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot);
            g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
        }
        for (i = 0; i < 7; i++) {
            if (g_BattleCommandQueue->unk2C4[i] != 0xFF) {
                g_BattleCommandQueue->unk2DC = g_BattleCommandQueue->unk2C4[i] + 8;
                BattleBeginActorAction(arg0, BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot), g_BattleCommandQueue->unk2DC - 1);
                BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
                D_800C402F[g_BattleCommandQueue->count * 72] = g_BattleCommandQueue->unk2DC - 1;
                D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
                *(u16 *)(g_BattleCommandRecords + g_BattleCommandQueue->count * 72) = g_BattleActionSlotMask;
                g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
            }
        }
        BattleQueueCommandFE(arg0);
        func_80087EDC(arg0, g_BattleCommandQueue->slots[arg0].targetSlot);
        while (g_BattleCommandQueue->commandFinished == 0) {
            BattleUpdateFrame();
        }
    }
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C81C);
void BattleClearPrimFlagsThreeSlots(u8 arg0) {
    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 0;
    g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = g_BattleDisplayState->primFlags[2] = 0;
    g_BattleDisplayState->primState = 0;
    if (arg0 == 0) {
        g_BattleDisplayState->primListsPending = 1;
    }
}
void BattleSetPrimFlagsThreeSlots(u8 arg0) {
    u16 value;

    g_BattleDisplayState->spriteColumnPending = g_BattleDisplayState->menuColumnPending = g_BattleDisplayState->cursorPending = 1;
    g_BattleDisplayState->primFlags[0] = g_BattleDisplayState->primFlags[1] = g_BattleDisplayState->primFlags[2] = 1;
    g_BattleDisplayState->primState = 4;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot);
    BattleBeginAnimTask(value);
    BattleApplySlotMaskToEffects(BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot));
}

