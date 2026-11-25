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
// The command id the four-slot menu and the two-slot one queue with a slot's action
// (BattleQueueCommandF3WithSlotMask and BattleActivateSelectedCommand write the same byte).
#define BATTLE_COMMAND_ID_SLOT 0xF3
// The bit of a 0x170-byte slot row's flags word that marks the slot as carrying a queued command.
#define BATTLE_COMMAND_ROW_FLAG_QUEUED 0x800
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
// The eight cells the four-slot command menu moves its cursor over, and the seven of them that
// hold a queued command: cell BATTLE_COMBO_STEP_MAX is the one that confirms.
#define BATTLE_COMBO_CELL_COUNT 8
#define BATTLE_COMBO_STEP_MAX 7

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
    /* 0x0B7 */ u8 primState; // which prim set is up; BattleAddDisplayStatePrims switches five ways on it, 0 for none
    /* 0x0B8 */ u8 rectExpandActive[7]; // per slot: while set, the slot's rect grows toward its target size
    /* 0x0BF */ u8 rectExpandDone[7]; // per slot: set once w and h reached the target size
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
// The per-slot character id table the main exe holds (one byte per actor slot), which indexes
// g_GameState's condition rows: the battle overlay's g_BattleActorCharIds is a second copy.
extern u8 D_80059468[];
// One byte per command slot of the entry the four-slot menu queued (the cell it was picked at).
extern u8 D_800C3DE0[];
// One 8-byte row per actor slot: byte 0 is the slot's remaining command budget.
extern u8 g_BattleSlotDisplayValue[];
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
// The count byte at offset 0x13 of each action row, which retail reaches through its own label
// (the first row's, D_800CDD40 + 0x13 and D_800CEF10 + 0x13).
extern u8 D_800CDD53[];
extern u8 D_800CEF23[];
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
extern u8 func_8008C81C(u8 arg0);
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
// Charges actor slot arg0's action budget (a u16 per slot) for the cell at (arg1, arg2): the row
// comes from the party action table or, for a slot whose flags are set, the gear one, and it
// yields both the argument BattleRunTargetSlotSelection picks the target with and the cost. The
// cost is charged only when the character carries the row's status bit and the target selection
// ran; otherwise the menu-denied sound plays. Returns 1 when the cost was paid.
u8 BattleTrySpendSlotActionBudget(u8 arg0, u8 arg1, u8 arg2) {
    u8 result;
    u8 canAct;
    u8 inParty;
    u16 entry;
    u8 count;
    s32 off;
    u8 bitIndex;

    result = 0;
    canAct = 1;
    inParty = 0;
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        bitIndex = arg1 + arg2 * 2;
        off = (arg2 * 2 + arg1 + 0x16) * sizeof(BattleActionRow) + arg0 * BATTLE_CELL_ACTION_BLOCK_SIZE;
        entry = ((BattleActionRow *)(g_BattleCellActionRows + off))->flags;
        count = D_800CDD53[off];
        inParty = (u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].characterConditionMask, bitIndex) != 0;
    } else {
        bitIndex = arg1 + arg2 * 2;
        off = (arg2 * 2 + arg1 + 0x15) * sizeof(BattleActionRow) + arg0 * BATTLE_ACTION_BLOCK_SIZE;
        entry = ((BattleActionRow *)(g_BattleActionRows + off))->flags;
        count = D_800CEF23[off];
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].gearConditionMask, bitIndex) != 0) {
            inParty = 1;
        }
    }
    // The three accesses to this row's flags word spell the same address through different labels
    // (gcc merges a repeated symbol + index address into one register and drops three instructions).
    if (*(u16 *)(D_800CCD38 + arg0 * sizeof(BattleCommandRow)) >= count && inParty != 0) {
        BattleClearPrimFlagsFourSlots(1);
        g_BattleDisplayState->pulseTriangleActive = 1;
        if (BattleRunTargetSlotSelection(entry, arg0, 0) != 0) {
            *(u16 *)(D_800CCD36 + arg0 * sizeof(BattleCommandRow) + 2) =
                *(u16 *)(D_800CCD1C + arg0 * sizeof(BattleCommandRow) + 0x1C) - count;
            result = 1;
        } else {
            g_BattleDisplayState->pulseTriangleActive = 0;
            BattleSetPrimFlagsFourSlots(arg0);
        }
        canAct = 0;
    }
    if (canAct != 0) {
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
    }
    return result;
}

extern u8 g_BattleMenuEvent;
extern u8 g_BattleCursorScrollState;
extern s32 g_BattleCursorScrollY;
extern u8 BattleTrySpendSlotActionBudget(u8 arg0, u8 arg1, u8 arg2);
extern void BattleInitSlotGraphics(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6);
extern void func_80091064(u8 arg0);
extern void BattleInitPulseTrianglePrims(void);
extern void BattleDrawCharStatusIfFlagged(u8 arg0, char arg1, u8 arg2);
extern void BattleBuildSpriteColumn8(s32 arg0);
extern void BattleDrawSlotSpriteQuads(s32 arg0);
extern void BattleStepAnimatedCursorPrim(s32 arg0, s32 arg1, s32 *pIndex, u8 *pCount);

u8 BattleRunFourSlotCommandMenu(u8 arg0) {
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
            if (BattleTrySpendSlotActionBudget(arg0, i, j) != 0) {
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
extern void BattleBuildCommandGridText(u8 arg0, u8 arg1, u8 arg2);
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
   ROUND 421 moved ONLY `pagePending = 1;` (not `page = 0;`) to just after
   BattleInitPulseTrianglePrims: 1160 bytes = retail's EXACT size (98.52 by
   objdiff, whose score drops on a displacement) and the ONLY residual left is
   4 lines - the pair `sw s4,88(sp); li s4,1` sitting after `li a3,0x118`
   instead of after `li s6,2`. Everything else, both `sw s0,24(sp)` stores, the
   `sw ra`/`sw s1` order and the `(u8)result` scratch in s0, is identical. So
   the byte count is reachable and the whole question is that pair's prologue
   slot: with the assignment at the top the slot is right and the CSE fires
   (1156), with it after the calls the CSE is broken and the slot is 4 late
   (1160). Prologue order follows statement order here, so what is needed is a
   const-1 that cse2 will not share with s4 while the statement stays after
   `result = 2`.
   Dead spellings measured earlier and not to be retried: moving the
   pagePending/page initialisation after BattleInitPulseTrianglePrims (gives the
   fresh li but moves the s4/s5 save pair out of the frame top, 97.17%), an s32
   local for the 7th argument, `u8 sel` vs s32 sel, `u32 pagePending` (the
   subtract stops CSEing: `addiu a1,s4,-1` + `addiu s5,s4,-1`, 98.77/1156).
   The state arms are stores-before-increments: `g_BattleCursorScrollY = (j + 1) * 13;`
   then `j++` is what reproduces retail per-arm `andi v1,(u8)j; addiu v1,v1,1`
   plus the -1 shared by states 2 and 4.
   ROUND 434 re-measured both ends of this and they are the ONLY two bodies:
   the one below is still 99.42% / 1156 bytes, and moving ONLY `pagePending = 1;`
   out of the init block to just after the second BattleInitSlotGraphics call
   (before BattleInitPulseTrianglePrims) gives retail's EXACT 1160 bytes - both
   calls' 7th arg then come from s0 with the `sw s0,72(sp)` / `li s0,1` pair and
   the (u8)result scratch in s0 exactly as retail has them - and its SOLE
   residual is the two-insn pair `sw s4,88(sp); li s4,1` landing after
   `li a3,0x118`, immediately before the loop-invariant `sw s7,100; li s7,2`
   pair, instead of after `li s6,2`: sched2 hoists it from its post-call RTL
   position and drops it 5 slots late. Placing the statement back in the init
   block tracks its source position exactly (permuter find this round: before
   `prevJ = 0xFF` lands 4 slots early, before `prevI = 0xFF` 2 early, right
   after `result = 2` is exact) but cse then substitutes s4 for every later SI
   constant 1, which is the 1156 body. So what is left is ONE scheduler slot,
   not the C: any spelling that keeps the assignment in the init block while
   stopping cse from recording `s4 == 1` closes it. Measured this round and all
   still 1156: `(u8)1` and `(u16)1` on pagePending, and a `u8 seekSlot = 1`
   copy passed as the 7th argument (both the cast and the QI copy fold back to
   SI const 1, and after the calls the same copy makes pagePending reuse it).
   A 4-minute permuter run from the 1160 body (base 415 best 223) found only the
   statement move above, i.e. the 1156 family again.
   ROUND 447 re-measured from an unpark: the init-block body below is still
   99.42% / 1156 bytes (one li s? short) and the pagePending-after-the-calls
   body is 1160 bytes with the single displaced `sw s4,88(sp); li s4,1` pair.
   NEW placement tried: `pagePending = 1;` between the two BattleInitSlotGraphics
   calls compiles to the SAME 1160 body as after both (the pair still sits after
   `li a3,0x118`, not after `li s6,2`), so every post-call position gives one
   body. lever.py on the residual returns the round-407 finding (c-two-loads-hoist:
   the pair's slot is the pre-reload scheduler's, unreachable from statement
   order), and a 3-minute permuter run from this init-block body went
   base=178 best=178 over 1472 iterations, i.e. it is a local optimum.
   ROUND 460 re-measured both ends from fresh unpark: the init-block body below
   is 99.42% / 1156 bytes (the missing `li s0,1` puts every later branch off by
   one), and the pagePending-after-BattleInitPulseTrianglePrims body is
   98.52% / 1160 bytes with the ONLY difference a 5-slot displacement of the
   `sw s4,88(sp); li s4,1` pair past the `li a3,0x118` of the first call's
   argument setup (every other line identical, jtbl operands aside). Nothing
   else measured: levers.md c-two-loads-hoist-the already records this exact
   residual from round 447 ("statement order is exhausted", do not re-permute),
   so this is the pre-reload scheduler's slot, not the C. */
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
            BattleBuildCommandGridText(arg0, pagePending - 1, redraw);
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
// The four-slot command menu's exit phase: func_8008C81C's return value picks the branch -- a
// cancelled or confirmed menu just clears the display, otherwise this walks the commands the
// player queued (g_BattleCommandQueue->unk2C4) and queues each one as a battle command, then
// waits for the queue to run dry. Returns 1 while the queued commands are being run, 0 when
// the menu ended without any.
s32 func_8008C4A8(u8 arg0) {
    s32 result;
    s32 i;
    s32 off;

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
        if ((((BattleCommandRow *)(D_800CCD1C + g_BattleCommandQueue->slots[arg0].targetSlot * sizeof(BattleCommandRow)))->flags & BATTLE_COMMAND_ROW_FLAG_QUEUED) != 0) {
            g_BattleCommandCodes[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = arg0;
            g_BattleCommandSlotIds[g_BattleCommandQueue->count * sizeof(BattleCommandRecord)] = BATTLE_COMMAND_ID_SLOT;
            g_BattleCommandSlotMasks[g_BattleCommandQueue->count * 36] = BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot);
            g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
        }
        for (i = 0; i < BATTLE_COMBO_STEP_MAX; i++) {
            if (g_BattleCommandQueue->unk2C4[i] != BATTLE_COMMAND_NONE) {
                g_BattleCommandQueue->unk2DC = g_BattleCommandQueue->unk2C4[i] + 8;
                BattleBeginActorAction(arg0, BattleSlotBitMask(g_BattleCommandQueue->slots[arg0].targetSlot), g_BattleCommandQueue->unk2DC - 1);
                BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
                // The queue count is re-read for each of the two records: the store between
                // them can alias the queue, so the second index is not the first one's value.
                off = g_BattleCommandQueue->count * sizeof(BattleCommandRecord);
                g_BattleCommandSlotIds[off] = g_BattleCommandQueue->unk2DC - 1;
                off = g_BattleCommandQueue->count * sizeof(BattleCommandRecord);
                g_BattleCommandCodes[off] = arg0;
                g_BattleCommandRecords[g_BattleCommandQueue->count].slotMask = g_BattleActionSlotMask;
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


INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C81C);
/* Best: 80.20% (1204 bytes against the original's 1200, 301 against 300 instructions).
   The whole shape is transcribed and every block is present in retail's order (5, 4, 0, 1,
   2, 3 then the 6/7/9/10 group, the switch table at .rodata+0, both call sites of
   BattleInitSlotGraphics, the wait on rectExpandDone[0]/[1], and `return result - 1`), so what
   is left is register assignment plus the frame:
   1. FRAME: retail's is 0x90, this body's 0x80. Retail's local area is 0x48 bytes and holds
      FIVE 8-byte arrays at 0x20/0x28/0x30/0x38/0x40, but only three of them are read (cmdIdx
      at 0x20 by func_80092784, cmdCost at 0x30 by func_80092784 and the pick arm, the backup
      array at 0x40 by the backspace arm) and only one is written-through (0x28, cleared by a
      pointer loop and never read again). Nothing writes 0x38 at all, so that slot is a gcc
      dead slot no source spelling reproduces: giving cellQueued 16 bytes instead of 8 moves
      cmdCost to 0x38 and measures 80.22, i.e. the offsets are not where the score is. Retail
      also has three 4-byte slots at 0x54/0x5C/0x64 that nothing in the object references.
   2. One extra saved register: this body hoists `sp+0x30` (the cmdCost base) into $s0 for the
      whole while loop, retail re-forms it with `addiu v0,sp,0x30` per arm. That shift moves
      arg0 to $s1 (retail $s0), remaining to $s5 ($s6), picked $s6 ($s7) and result $s7 (fp),
      and it is why every register in the tail differs.
   3. Structurally the case-4 arm costs one extra instruction: retail falls INTO the confirm
      block from `bne v0,s1` and jumps over the empty-queue `result = 2`, this body jumps to
      the result = 2 block instead (+4 bytes, 301 vs 300).
   The dead cellQueued clear loop is in the body because retail has it (a pointer loop
   `while (p < end) *p++ = 0;` over 8 bytes; a plain indexed loop compiles to gcc's downward
   clear instead). */
/*
// The four-slot command menu's entry phase: it reads the acting slot's action rows to build the
// list of commands the player may still use (and what each costs), then lets the player queue up
// to BATTLE_COMBO_STEP_MAX of them while the running total fits the slot's budget
// (g_BattleSlotDisplayValue). The last cell of the eight confirms, menu event 5 takes the last
// pick back. Returns 1 once the target selection ran, 0 if it was cancelled.
u8 func_8008C81C(u8 arg0) {
    u8 cmdIdx[8];
    u8 cellQueued[8];
    u8 cmdCost[8];
    u8 backup[8];
    s32 primIndex;
    u8 primCount;
    u8 dirty;
    s32 slotOff;
    u8 remaining;
    u8 cursor;
    u8 picked;
    s32 result;
    s32 i;
    s32 k;

    cursor = 0;
    picked = 0;
    result = 0;
    dirty = 1;
    remaining = g_BattleSlotDisplayValue[arg0 * 8];
    primIndex = 4;
    primCount = 0;
    g_BattleDisplayState->primListsPending = 0;
    for (i = 0; i < BATTLE_COMBO_STEP_MAX; i++) {
        cmdIdx[i] = BATTLE_COMMAND_NONE;
        cmdCost[i] = 0;
        g_BattleCommandQueue->unk2C4[i] = BATTLE_COMMAND_NONE;
        D_800C3DE0[i] = BATTLE_COMMAND_NONE;
    }
    // retail clears this array and never reads it again
    for (i = 0; i < BATTLE_COMBO_CELL_COUNT; i++) {
        cellQueued[i] = 0;
    }
    slotOff = arg0 * BATTLE_CELL_ACTION_BLOCK_SIZE;
    k = 0;
    for (i = 0; i < BATTLE_COMBO_STEP_MAX; i++) {
        if (BattleMaskHasStatusBit(
                *(u16 *)((u8 *)&g_GameState + 0x16C0 + D_80059468[arg0] * 0x20), i) != 0) {
            cmdIdx[k] = i;
            cmdCost[k] = g_BattleCellActionRows[slotOff + (i + BATTLE_COMBO_STEP_MAX) * 0x28 + 0x17];
            k++;
        }
    }
    BattleInitSlotGraphics(0, 0x16, 0x5C, 0x122, 0x4C, 1, 1);
    BattleInitSlotGraphics(1, 0x10, 0x2C, 0xE8, 0x2C, 1, 1);
    while (g_BattleDisplayState->rectExpandDone[0] == 0 ||
           g_BattleDisplayState->rectExpandDone[1] == 0) {
        BattleUpdateFrame();
    }
    func_80092784(arg0, cmdIdx, cmdCost);
    BattleInitPulseTrianglePrims();
    while (result == 0) {
        if (dirty != 0) {
            func_80092B74(arg0, remaining);
            dirty = 0;
        }
        BattleStepAnimatedCursorPrim((cursor & 1) * 0x88 + (cursor & 1) * 4 + 0x1E,
                                     (cursor >> 1) * 16 + 0x64, &primIndex, &primCount);
        BattleUpdateFrame();
        switch (g_BattleMenuEvent) {
        case 5:
            if (picked != 0) {
                picked--;
                dirty = 1;
                g_BattleCommandQueue->unk2C4[picked] = BATTLE_COMMAND_NONE;
                D_800C3DE0[picked] = BATTLE_COMMAND_NONE;
                remaining += backup[picked];
            } else {
                result = 2;
            }
            break;
        case 4:
            if (cursor == BATTLE_COMBO_STEP_MAX) {
                if (g_BattleCommandQueue->unk2C4[0] != BATTLE_COMMAND_NONE) {
                    BattleFreeSlotPrims(0);
                    g_BattleDisplayState->pulseTriangleActive = 1;
                    if (BattleRunTargetSlotSelection(0x1000, arg0, 1) != 0) {
                        g_BattleSlotDisplayValue[arg0 * 8] = remaining;
                        result = 1;
                    } else {
                        g_BattleDisplayState->pulseTriangleActive = 0;
                        BattleSetPrimFlagsTwoSlotsMode3(arg0);
                    }
                } else {
                    result = 2;
                }
            } else if (picked != BATTLE_COMBO_STEP_MAX) {
                if (cmdIdx[cursor] != BATTLE_COMMAND_NONE && remaining >= cmdCost[cursor]) {
                    g_BattleCommandQueue->unk2C4[picked] = cmdIdx[cursor];
                    D_800C3DE0[picked] = cursor;
                    backup[picked] = cmdCost[cursor];
                    picked++;
                    dirty = 1;
                    remaining -= cmdCost[cursor];
                }
            }
            break;
        case 0:
            if (++cursor >= BATTLE_COMBO_CELL_COUNT) {
                cursor--;
            }
            break;
        case 1:
            cursor += 2;
            if (cursor >= BATTLE_COMBO_CELL_COUNT) {
                cursor -= 2;
            }
            break;
        case 2:
            if (cursor != 0) {
                cursor--;
            }
            break;
        case 3:
            if (cursor >= 2) {
                cursor -= 2;
            }
            break;
        case 6:
        case 7:
        case 9:
        case 10:
            cursor = BATTLE_COMBO_STEP_MAX;
            break;
        }
    }
    return result - 1;
}
*/
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

