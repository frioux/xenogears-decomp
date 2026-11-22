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
// The bytes each actor owns in the two per-actor action tables (include/battle/actor.h's
// BattleRow, which this unit reads as the partial view below).
#define BATTLE_CELL_ACTION_BLOCK_SIZE 0x5F0
#define BATTLE_ACTION_BLOCK_SIZE 0x690

// One row of either action table: the flags word at +0, whose low bits pick the row's target
// mode and whose 0x1000/0x4000 bits pick the icon the menu draws.
typedef struct {
    /* 0x00 */ u16 flags;
    /* 0x02 */ u8 unk2[0x26];
} BattleActionRow; // Size: 0x28

// The word at +0 of one 0x170-byte slot row (D_800CCD1C): bit 0x800 keeps the slot's bit in the
// mask BattleQueueCommandF3WithSlotMask queues. src/battle/main/misc13.c declares the same row.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk2[0x16E];
} BattleCommandRow; // Size: 0x170

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
extern u8 g_BattleCommandCodes[];
extern u16 g_BattleCommandSlotMasks[];
extern u8 g_BattleCommandSlotIds[];
extern u8 D_800CCD1C[];
extern u8 g_BattleCommandGridIds[];
typedef struct {
    /* 0x0 */ u16 flags; // & 7 picks the target mode, 0xF and 0x1000/0x4000 pick the drawn icon
    /* 0x2 */ u8 pad2[0xE];
} BattleCommandEntry; // Size: 0x10

extern BattleCommandEntry g_BattleCommandEntries[];
extern u8 g_BattleCommandGridAltUses[];
extern u8 g_BattleCommandGridAltIds[];
// One 0x48-byte row of the command record table g_BattleCommandRecords: the slot mask the
// queued command covers at +0. src/battle/main/misc13 declares the rest of the same record
// (the actor slot at +0xD, the argument word at +0x24 and the command byte at +0x31, which
// this unit reaches through g_BattleCommandCodes / g_BattleCommandSlotIds).
typedef struct {
    /* 0x00 */ u16 slotMask;
    /* 0x02 */ u8 unk2[0x46];
} BattleCommandRecord; // Size: 0x48

extern BattleCommandRecord g_BattleCommandRecords[];
extern u8 BattleRunTargetSlotSelection(u16 arg0, u8 arg1, s32 arg2);
extern s32 BattleIsCharacterInParty(s32 arg0);
extern s32 BattleMaskHasStatusBit(s32 mask, u8 idx);
extern void BattleFillPartySlotWeaponRecord(u8 arg0);
extern u16 g_BattleActionSlotMask;
extern u16 g_BattleActionParam;
extern u8 g_BattleSlotFlags[];
extern u16 g_BattleTargetSlotMask;
extern u8 g_BattleTargetSlot;
extern u8 g_BattleCellActionRows[];
extern u8 D_800CCD38[];
extern u8 D_800CCD36[];
extern u8 D_800CCD1C[];
extern u8 g_BattleActorCharIds[];
extern u8 g_BattleActionRows[];
extern void BattleClearSlotCurveEntry(void);
extern void BattleSetSlotRegistersFromActor(u8 arg0, u8 arg1);
extern void BattleQueueCommandFE(u8 arg0);
extern void BattleApplySlotCurveAndStep(u8 arg0);
extern void BattleBeginActorAction(u8 arg0, u16 arg1, u16 arg2);
extern s32 BattleFindMinValueOtherSlot(u8 arg0);
extern void BattleInitActorSlotAnimation(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
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
void BattleQueueCommandF3WithSlotMask(u16 slotMask, u8 actorSlot) {
    u16 mask;
    s32 i;
    s32 off;
    u8 idx;

    mask = slotMask & ~BATTLE_PARTY_SLOT_MASK;
    for (i = 0, off = MAX_PARTY_MEMBERS * sizeof(BattleCommandRow); i < BATTLE_OTHER_SLOT_COUNT; i++) {
        idx = i + MAX_PARTY_MEMBERS;
        if (BattleMaskHasSlotBit(mask, idx) != 0) {
            if ((((BattleCommandRow *)(D_800CCD1C + off))->flags & 0x800) == 0) {
                mask &= BattleSlotBitMaskInverted(idx);
            }
        }
        off += sizeof(BattleCommandRow);
    }
    if (mask == 0) {
        return;
    }
    g_BattleCommandCodes[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = actorSlot;
    g_BattleCommandSlotIds[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = 0xF3;
    g_BattleCommandSlotMasks[g_BattleCommandQueue->count * 36] = mask;
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
}

// Fills the four command-record pages with BATTLE_COMMAND_NONE, then queues the
// command the cursor picked (commandId picks the row, actorSlot the acting actor slot) and
// spins until the queued command has finished.
// `fillOff` and `entryOff` are separate from `off` on purpose: one local shared by
// the fill loop and the two arms couples the arm sums' `addu` destination with the
// fill loop's constant register pair, and no spelling of the shared form gets both
// (99.83%); with the three locals split it is 100.00 at retail's 824 bytes.
void BattleQueueOneActorCommand(u8 actorSlot) {
    s32 off;
    s32 i;
    u16 mask;
    u16 sel;
    u8 value;
    u16 *pActionMask;
    u16 *pMask;
    BattleActionRow *pRow;
    s32 fillOff;

    g_BattleSoundEnabled = 0;
    BattleApplySlotMaskToEffects(0);
    value = 0xFF;
    for (fillOff = 0x8B8; fillOff >= 0; fillOff -= 0x48) {
        do {
            g_BattleCommandSlotIds[fillOff] = value;
        } while (0);
    }
    if (g_BattleSlotFlags[actorSlot * 8] == 0) {
        g_BattleCommandQueue->unk2DC = g_BattleCommandQueue->commandId + 0x17;
        off = (g_BattleCommandQueue->commandId + 0x16) * sizeof(BattleActionRow) +
              actorSlot * BATTLE_CELL_ACTION_BLOCK_SIZE;
        pRow = (BattleActionRow *)(g_BattleCellActionRows + off);
        sel = pRow->flags & BATTLE_COMMAND_TARGET_MODE_MASK;
    } else {
        g_BattleCommandQueue->unk2DC = g_BattleCommandQueue->commandId + 0x16;
        {
            s32 entryOff;

            entryOff = (g_BattleCommandQueue->commandId + 0x15) * sizeof(BattleActionRow) +
                       actorSlot * BATTLE_ACTION_BLOCK_SIZE;
            pRow = (BattleActionRow *)(g_BattleActionRows + entryOff);
            sel = pRow->flags & BATTLE_COMMAND_TARGET_MODE_MASK;
        }
    }
    pActionMask = &g_BattleActionSlotMask;
    if (sel != 0) {
        pMask = pActionMask;
        if (1) {
            mask = g_BattleTargetSlotMask;
        }
    } else {
        mask = BattleSlotBitMask(g_BattleTargetSlot);
    }
    pMask = pActionMask;
    BattleQueueCommandF3WithSlotMask(mask, actorSlot);
    BattleClearSlotCurveEntry();
    BattleBeginActorAction(actorSlot, mask, g_BattleCommandQueue->unk2DC - 1);
    BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
    mask = BattleSlotBitMask(actorSlot);
    BattleBeginAnimTask(*pMask | mask);
    BattleInitActorSlotAnimation(1, actorSlot, g_BattleCommandQueue->slots[actorSlot].targetSlot, BattleFindMinValueOtherSlot(actorSlot));
    g_BattleCommandSlotIds[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = g_BattleActionParam;
    g_BattleCommandCodes[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = actorSlot;
    g_BattleCommandRecords[g_BattleCommandQueue->count].slotMask = *pMask;
    for (i = MAX_PARTY_MEMBERS; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (BattleMaskHasSlotBit(*(volatile u16 *)&g_BattleActionSlotMask, i) != 0) {
            BattleSetSlotRegistersFromActor(actorSlot, i);
        }
    }
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    BattleQueueCommandFE(actorSlot);
    while (g_BattleCommandQueue->commandFinished == 0) {
        BattleUpdateFrame();
    }
}

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
/* Best: 96.54% at retail's exact 596 bytes -- every block has retail's
   instruction count and the multiset matches; what is left is register
   assignment and statement order in the two g_BattleSlotFlags arms.
   What took this from 89.19 to here:
   1. the D_800CCD38 sites. gcc merges a repeated (symbol + reg) address into one
      register (`lui/addiu` + `addu`, 3 instructions short of retail), so each of
      the three accesses needs its OWN symbol spelling to stay in retail's
      `lui at,%hi; addu at,at,reg; lhu/sh %lo(at)` form: the compare reads
      D_800CCD38, and the `-=` is a load from D_800CCD1C + 0x1C and a store to
      D_800CCD36 + 2 (all three spell the same address, so the linked bytes are
      identical -- check_bin.sh proves it).
   2. the second arm needs the bit index in a local assigned BEFORE its
      BattleMaskHasStatusBit call; without it gcc keeps a copy of the raw arg1
      (`move t0,a1` at entry) and does the sum at the call, which is retail's
      first-arm shape only.
   Left: the first arm wants that early sum (`addu a1,a1,v0` destroying the
   incoming a1) while the second wants retail's masked recomputation, and giving
   BOTH arms the local makes gcc CSE the masked form away (584 bytes). So the two
   arms need opposite spellings of the same source line. Two 3-minute permuter
   runs (from this body and from the 89.19 one) only returned variants that read
   the local before assigning it. Measured dead: inline `D_800CCD38 + arg0*0x170`
   at all three sites, an s32/u32 off local, one-symbol two-statement `-=`
   (596 bytes but a formed base register).
   Round 297 re-measured: the bit local in BOTH arms makes cse merge the two sums
   into one (584 bytes), and hoisting the mask call to the first statement of an
   arm re-orders the loads and costs a saved register. The two retail sums differ
   only in operand order, so the open question is which sum gcc emits first.
   Round 396 re-measured, 596 bytes / 96.54 confirmed: arm 1 does emit both sums
   but places the raw one after the entry/count loads (so raw arg1 has to survive
   in t0, hence the prologue `move t0,a1` and a delay slot reading t0); arm 2 CSEs
   the two sums into one, which is the single instruction this body is short.
   Also measured dead: no bit local at all in arm 2 (600 bytes, 95.94), the bit
   local as the FIRST statement of arm 1 (588, 92.09), the bit local after the
   loads in arm 1 (588, 95.38). The two count loads also want a symbol at
   g_BattleCellActionRows+0x13 / g_BattleActionRows+0x13 (retail names
   D_800CDD53 / D_800CEF23) where the typed spelling folds the 0x13 into the LO16
   addend -- same linked bytes, but objdiff cannot fold it. */
/*
u8 func_8008B224(u8 arg0, u8 arg1, u8 arg2) {
    u8 result;
    u8 canAct;
    u8 inParty;
    u16 entry;
    u8 count;
    s32 off;
    s32 bit;

    result = 0;
    canAct = 1;
    inParty = 0;
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        off = (arg2 * 2 + arg1 + 0x16) * 40 + arg0 * 1520;
        entry = *(u16 *)(g_BattleCellActionRows + off);
        count = *(u8 *)(g_BattleCellActionRows + off + 0x13);
        inParty = (u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].characterConditionMask, arg1 + arg2 * 2) != 0;
    } else {
        off = (arg2 * 2 + arg1 + 0x15) * 40 + arg0 * 1680;
        entry = *(u16 *)(g_BattleActionRows + off);
        count = *(u8 *)(g_BattleActionRows + off + 0x13);
        bit = arg1 + arg2 * 2;
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].gearConditionMask, bit) != 0) {
            inParty = 1;
        }
    }
    if (*(u16 *)(D_800CCD38 + arg0 * 0x170) >= count && inParty != 0) {
        BattleClearPrimFlagsFourSlots(1);
        g_BattleDisplayState->pulseTriangleActive = 1;
        if (BattleRunTargetSlotSelection(entry, arg0, 0) != 0) {
            *(u16 *)(D_800CCD36 + arg0 * 0x170 + 2) =
                *(u16 *)(D_800CCD1C + arg0 * 0x170 + 0x1C) - count;
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
extern void BattleDrawSlotSpriteQuads(s32 arg0);
extern void BattleStepAnimatedCursorPrim(s32 arg0, s32 arg1, s32 *pIndex, u8 *pCount);

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
            BattleDrawSlotSpriteQuads(g_BattleCursorScrollY);
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
        // Each arm stores the same expression it compares against, so the scroll
        // target is computed once and reaches the store in the compare's register.
        switch (g_BattleCursorScrollState) {
        case 1:
            if (g_BattleCursorScrollY >= (j + 1) * 16) {
                g_BattleCursorScrollY = (j + 1) * 16;
                j++;
                i--;
                g_BattleCursorScrollState = 0;
            }
            break;
        case 2:
            if (g_BattleCursorScrollY < (j - 1) * 16) {
                g_BattleCursorScrollY = (j - 1) * 16;
                j--;
                i++;
                g_BattleCursorScrollState = 0;
            }
            break;
        case 3:
            if (g_BattleCursorScrollY >= (j + 1) * 16) {
                g_BattleCursorScrollY = (j + 1) * 16;
                j++;
                g_BattleCursorScrollState = 0;
            }
            break;
        case 4:
            if (g_BattleCursorScrollY < (j - 1) * 16) {
                g_BattleCursorScrollY = (j - 1) * 16;
                j--;
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
void BattleActivateSelectedCommand(u8 actorSlot) {
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
        g_BattleCommandSlotIds[off] = value;
    }
    BattleInitActorSlotAnimation(1, actorSlot, g_BattleCommandQueue->slots[actorSlot].targetSlot, BattleFindMinValueOtherSlot(actorSlot));
    sel = g_BattleCommandEntries[g_BattleCommandQueue->commandId].flags & BATTLE_COMMAND_TARGET_MODE_MASK;
    if (sel != 0) {
        mask = g_BattleTargetSlotMask;
    } else {
        mask = BattleSlotBitMask(g_BattleTargetSlot);
    }
    BattleQueueCommandF3WithSlotMask(mask, actorSlot);
    BattleClearSlotCurveEntry();
    BattleBeginAction(actorSlot, mask, g_BattleCommandQueue->commandId);
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
    g_BattleCommandSlotIds[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = 0xF5;
    g_BattleCommandSlotMasks[g_BattleCommandQueue->count * 36] = g_BattleActionParam;
    g_BattleCommandCodes[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = actorSlot;
    g_BattleCommandRecords[g_BattleCommandQueue->count].slotMask = g_BattleActionSlotMask;
    for (i = MAX_PARTY_MEMBERS; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (BattleMaskHasSlotBit(g_BattleActionSlotMask, i) != 0) {
            BattleSetSlotRegistersFromActor(actorSlot, i);
        }
    }
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    BattleQueueCommandFE(actorSlot);
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
            BattleFillPartySlotWeaponRecord(value);
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
extern void BattleDrawColumnSpriteCells(s32 arg0);
extern void func_8009093C(s32 arg0);

/* Best: 99.42% (1156 bytes vs the original 1160). ONE instruction short and
   identical instruction for instruction elsewhere (jump table at .rodata+0x18,
   both switches 5,4,0,2,1,3 and 1,2,3,4, the 0x70 frame, every delay slot).
   MISSING: `li s0,1` for the 7th argument of the two BattleInitSlotGraphics
   calls. Retail materialises a fresh 1 into s0 even though pagePending already
   holds 1 in s4, and gcc 2.6 reuses s4 (`sw s4,0x18(sp)`, no li), which also
   moves the `sw s0,0x48(sp)` save three slots later and leaves the (u8)result
   scratch in s1 where retail uses s0.
   ROUND 392 re-measured: same body, same 1156 bytes, 99.42%.
   The tie is SImode CONSTANT CSE, proved by mode rather than by spelling:
   declaring pagePending `u8` makes the arg constant stop matching s4, and the
   prologue then reproduces retail exactly (sw s0,72(sp) + li s0,1, sw s0,24(sp)
   at both calls) - but a QI pagePending costs 2 instructions in the loop head
   (`andi s0,s4,0xFF; beqz s0` for the `!= 0` test, and `page` recomputed as
   `addiu s5,s0,-1`) = 1164 bytes. The same mode split is already visible in the
   matched part: the byte store `g_BattleCursorScrollState = 2` gets its own
   register (s7) next to result in s6. So the answer is NOT a spelling of the
   literal: every s32 spelling of the value 1 (literal, u8 local, u16 local, an
   s32 local declared before the calls, pagePending itself) measures 1156.
   Presets screened on this body: 2.7.2-psx 99.39, 2.6.0 -O1 -G0 69.09,
   2.6.0 -O3 -G0 93.16 (both -G8 variants fail to build through try_flags);
   2.6.0 -O2 -G8 stays the best, so this is not a preset question.
   Permuter (1323 iterations, -j2): base=178 best=163, and the only find hoists
   `pagePending - 1` into a preheader local and reads it in the loop - WRONG
   CODE: pagePending becomes 2 in the command arm, so that value is not loop
   invariant. Rejected; the earlier run (439 iterations, base=178) found nothing.
   Dead spellings measured earlier and not to be retried: moving the
   pagePending/page initialisation after BattleInitPulseTrianglePrims (gives the
   fresh li but moves the s4/s5 save pair out of the frame top, 97.17%), an s32
   local for the 7th argument, `u8 sel` vs s32 sel.
   The state arms are stores-before-increments: `g_BattleCursorScrollY = (j + 1) * 13;`
   then `j++` is what reproduces retail per-arm `andi v1,(u8)j; addiu v1,v1,1`
   plus the -1 shared by states 2 and 4. *//*
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
            BattleDrawColumnSpriteCells(g_BattleCursorScrollY / 13 * 2);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BED8);

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
      table bases inside the loop (`lui at,%hi(g_BattleCommandSlotIds); addu at,at,v1;
      sb %lo(g_BattleCommandSlotIds)(at)`); gcc 2.6 hoists all three symbols into s3/s4/s5 in
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
            g_BattleCommandCodes[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = arg0;
            g_BattleCommandSlotIds[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = 0xF3;
            g_BattleCommandSlotMasks[g_BattleCommandQueue->count * 36] = BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot);
            g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
        }
        for (i = 0; i < 7; i++) {
            if (g_BattleCommandQueue->unk2C4[i] != 0xFF) {
                g_BattleCommandQueue->unk2DC = g_BattleCommandQueue->unk2C4[i] + 8;
                BattleBeginActorAction(arg0, BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot), g_BattleCommandQueue->unk2DC - 1);
                BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
                g_BattleCommandSlotIds[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = g_BattleCommandQueue->unk2DC - 1;
                g_BattleCommandCodes[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = arg0;
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

