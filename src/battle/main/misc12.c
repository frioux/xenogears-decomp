#include "common.h"
#include "battle/battle_work.h"

// The sound played when the battle menu refuses an input (battle/main/misc11 and
// misc16 call it with the same id; kept here because this unit does not include actor.h).
#define BATTLE_SOUND_MENU_DENIED 0x4F

// The battle actor slots: the slot[] and charRedrawFlags[] arrays of
// BattleActorSlots and every loop that walks the per-slot g_BattleCurve* tables or
// g_BattleSlotEntries. Same value as include/battle/actor.h's
// BATTLE_ACTOR_SLOT_COUNT; duplicated here because this unit does not include that
// header.
// owned by include/battle/actor.h; this unit carries its own externs and cannot include it
#define BATTLE_ACTOR_SLOT_STRIDE 0x1C

#define BATTLE_ACTOR_SLOT_COUNT 11

// One entry per battle slot, the twelfth included: the length of the candidate list
// g_BattleTargetCandidates and of the buffers BattleBuildTargetCandidateListByRow
// fills. Same value as include/battle/actor.h's BATTLE_SLOT_COUNT.
#define BATTLE_SLOT_COUNT 12

// The two slot groups the target search walks: the party's slots 0-2 and the other
// actors' slots 3-10, entered from whichever group the acting slot is not in. Same
// values as include/battle/actor.h's slot groups (BATTLE_PARTY_SLOT_MASK is those
// 3 bits).
#define BATTLE_PARTY_SLOT_COUNT 3
#define BATTLE_PARTY_SLOT_MASK 0x7
// The other group's slots, BATTLE_PARTY_SLOT_COUNT through BATTLE_ACTOR_SLOT_COUNT - 1:
// the count each target search walks once the party group is done.
#define BATTLE_OTHER_SLOT_COUNT (BATTLE_ACTOR_SLOT_COUNT - BATTLE_PARTY_SLOT_COUNT)

// The slot id that means "no slot": an empty candidate list entry, a slot that
// targets nothing.
#define BATTLE_SLOT_NONE 0xFF
// The mode BattleClearSlotCurveState and BattleClearSlotCurveEntry store to mean
// that no curve runs on the slot; BattleApplySlotCurve's switch never matches it.
#define BATTLE_CURVE_MODE_NONE 0xFF
// Slot flags bits that make a slot untargetable: BattlePairTargetable and
// BattleActorTargetableWithOverride call it targetable only while they are clear.
#define BATTLE_SLOT_FLAGS_UNTARGETABLE 0xC001

// The target-selection flags word BattleSelectTargetCandidates is given: where the
// target may come from, and how many slots it may hold.
#define BATTLE_TARGET_FLAGS_OTHER_SLOTS 0x1000 // the other actors' slots only
#define BATTLE_TARGET_FLAGS_BOTH_GROUPS 0x2000 // no group narrowing
#define BATTLE_TARGET_FLAGS_GIVEN_SLOT 0x4000 // arg2's slot and nothing else
#define BATTLE_TARGET_FLAGS_SINGLE_TARGET 0x8000 // one slot out of the eligible ones
// The directions a slot menu moves in: func_80084854's mode, one pulse-triangle
// indicator byte per direction in BattleRunTargetSlotSelection.
#define BATTLE_TARGET_DIRECTION_COUNT 4
// No event at all: the value g_BattleMenuEvent never holds. BattleRunTargetSlotSelection
// starts its "the event changed" check from it so the first frame draws the indicators.
#define BATTLE_MENU_EVENT_NONE 0xFE

extern u8 g_BattleCurveModes[];
extern u16 g_BattleCurveValues[];
// One 0x1C-byte row of the per-slot position table at D_800C3EBE: the slot's x and
// y, which the target-selection code compares and feeds to ratan2 as (y, x).
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u8 unk4[0x18];
} BattleUnk1C; // Size: 0x1C

extern BattleUnk1C D_800C3EBE[];
extern u8 g_BattleSlotRecords[][BATTLE_ACTOR_SLOT_STRIDE];
extern u16 g_BattleTargetSlotMask;
extern u8 g_BattleTargetSlot;
extern u16 BattleMaskHasSlotBit(u16, u8);
extern u16 BattleSlotBitMask(u8);
extern u8 BattlePairTargetable(u8, u8);
extern u8 *g_BattleCommandQueue;
extern u8 g_BattleTargetCandidates[];
extern u8 D_800C3E9B[];
extern u16 D_800C3EC0[];
extern long ratan2(long y, long x);
extern u8 D_800C3E9B[];
extern u8 g_BattleTargetCandidateCount;
extern u8 BattleBuildTargetCandidateListByRow(u8);

// One command row: per slot, the curve BattleApplySlotCurve copies out of the four
// g_BattleCurve* globals (the target it moves to and the value/mode it holds now).
typedef struct {
    /* 0x00 */ u16 curveTargets[12];
    /* 0x18 */ u8 curveTargetModes[12];
    /* 0x24 */ u16 curveValues[12];
    /* 0x3C */ u8 curveModes[12];
} BattleUnk48; // Size: 0x48

extern BattleUnk48 g_BattleCommandRows[];

// One 0x170-byte battle slot entry as the slot menu reads it: the targetable flags
// word, the two-word counter BattleAdvanceSlotMenuStateCounter ramps, and the state
// block at +0xA4 with its own copy of that flags word.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 pad2[0x6];
    /* 0x008 */ u16 activeEffectFlags3; // the entry's word at +0x84: this view starts 0x7C into it (actor.h BattleTableEntry)
    /* 0x00A */ u8 padA[0x56];
    /* 0x060 */ u16 counter; // steps by g_BattleSlotCounterStep up to counterLimit
    /* 0x062 */ u16 counterLimit;
    /* 0x064 */ u8 pad64[0x40];
    /* 0x0A4 */ u16 stateFlags; // the +0xA4 state block's copy, tested like flags
    /* 0x0A6 */ u8 padA6[0x2];
    /* 0x0A8 */ u16 stateActiveEffectFlags2; // the +0xA4 state block's activeEffectFlags2 (entry +0x124): its 0x8000 bit moves with activeEffectFlags3's
    /* 0x0AA */ u8 padAA[0xC6];
} BattleUnk170; // Size: 0x170

// The same entry seen from g_BattleSlotEntriesBase (+0x4C of it): only the hp the
// candidate sort minimises is named here. BattleTableEntry's hp/maxHp sit at +0x4C/+0x4E
// in include/battle/actor.h, and src/battle/main/misc8.c declares this block in full.
typedef struct {
    /* 0x000 */ u16 hp;
    /* 0x002 */ u8 unk2[0x16E];
} BattleSlotBaseEntry; // Size: 0x170

extern u8 g_BattleSlotEnabledFlags[];
extern u8 D_800C3EB7[][BATTLE_ACTOR_SLOT_STRIDE];
extern u8 g_BattleSlotFlags[];
extern u8 *g_BattleSlotParamTable;
extern BattleUnk170 g_BattleSlotEntries[];
// The same slot table seen from +0xA4, so its `flags` is the entry's stateFlags.
extern BattleUnk170 D_800CCE08[];
extern BattleSlotBaseEntry g_BattleSlotEntriesBase[];

typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u8 statDigits[3][4]; // per stat, three font digit codes written left to right
    // One word per command: the character's command bit for it (D_800C3234 holds the 16
    // single-bit masks the slot setup ANDs into these), so nonzero means the slot may use it.
    /* 0x1C */ u16 commandFlags[16];
    /* 0x3C */ u8 targetSlot; // the slot this one targets, 0xFF for none
    /* 0x3D */ u8 unk3D[0x3];
} BattleActorSlot; // Size: 0x40

// 0x2F8-byte block of actor slots plus the slot menu state that follows them.
typedef struct {
    /* 0x000 */ BattleActorSlot slot[BATTLE_ACTOR_SLOT_COUNT];
    /* 0x2C0 */ u8 unk2C0[0x4];
    /* 0x2C4 */ u8 queuedActionIds[0x7]; // 0xFF for no entry; func_8008C4A8 passes each to BattleBeginActorAction
    /* 0x2CB */ u8 unk2CB;
    /* 0x2CC */ u8 commandKey[7]; // matched against the 13 7-byte rows BattleCheckCommandKeyStatusBit walks
    /* 0x2D3 */ u8 actorIndex; // the slot the queued command is being built for
    /* 0x2D4 */ u8 gaugeValue; // the reading the menu spends, the second index of the g_BattleGaugeCountdownTable[max * 8 + value] pair
    /* 0x2D5 */ u8 gaugeMax; // its value at menu entry, the first index of that pair and the row func_8007FBE0 draws
    /* 0x2D6 */ u8 commandKeyLength; // commandKey bytes filled, 0 for no command
    /* 0x2D7 */ u8 unk2D7[0x3];
    /* 0x2DA */ u8 count; // queued commands, the row BattleClearSlotCurveEntry indexes
    /* 0x2DB */ u8 commandFinished; // 0 while a queued command runs, 1 once it has; BattleWaitWhileStateIs8 and func_80079778 spin on it
    /* 0x2DC */ u8 commandCostIndex; // one-based index into g_BattleCommandCost of the command just queued, so the confirm handler charges that row
    /* 0x2DD */ u8 menuState; // the state each menu event handler switches to
    /* 0x2DE */ u8 eventHandled; // handlers set it to release the update wait loop
    /* 0x2DF */ u8 gaugeCost; // gauge points the confirmed command spends; BattleAdvanceCommandStep also uses it as the step index into the action-id table
    /* 0x2E0 */ u8 colorShift; // subtracted from each colour byte of the slot's prims after << 4
    /* 0x2E1 */ u8 commandQueued; // set when a handler queued its command, so its next call returns at once
    /* 0x2E2 */ u8 lastMenuState; // the menuState the current frame was built for; the dispatcher rebuilds when it differs
    /* 0x2E3 */ u8 gaugeSpent; // set when a confirmed command spent gauge; func_80080160 then folds the leftover reading into the slot display value
    /* 0x2E4 */ u8 unk2E4;
    /* 0x2E5 */ u8 unk2E5;
    /* 0x2E6 */ u8 commandId; // the picked command: indexes the 0x10-byte g_BattleCommandEntries rows and is BattleBeginAction's third argument
    /* 0x2E7 */ u8 unk2E7;
    /* 0x2E8 */ u8 targetSlot; // the slot being selected, the menu's copy of slot[].targetSlot
    /* 0x2E9 */ u8 commandAppended; // set once BattleAppendCommandRecord has run; both append paths return at once
    /* 0x2EA */ u8 menuRunning; // 1 while a slot menu run is in progress; cleared together with eventHandled
    /* 0x2EB */ u8 charRedrawFlags[BATTLE_ACTOR_SLOT_COUNT];
    /* 0x2F6 */ u8 repeatPending; // the last event was this one, so take it as a repeat
    /* 0x2F7 */ u8 unk2F7;
} BattleActorSlots; // Size: 0x2F8

extern u8 g_BattleMenuEvent;
extern void BattlePlaySoundIfEnabled(u8);
extern u8 func_8008BED8(u8);
extern void BattleActivateSelectedCommand(u8);
extern u8 g_BattleLastMenuEvent;
extern void BattleMarkSlotReleased(u8);
void BattleAdvanceSlotMenuState(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleMarkSlotReleased(a);
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[5] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[11] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 8;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[8] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 9;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 2) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 3;
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[10] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
extern void BattleSetupActorSlotDisplay(u8);
extern void BattleEnsureTargetSlotInCandidateList(u8);
extern void BattleInitPulseTrianglePrims(void);
extern u8 g_BattleSoundEnabled;
void BattleSetActorSlotDisplayMode(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].targetSlot == BATTLE_SLOT_NONE) {
            BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            break;
        }
        BattleSetupActorSlotDisplay(a);
        BattleEnsureTargetSlotInCandidateList(a);
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x19;
        BattleInitPulseTrianglePrims();
        g_BattleSoundEnabled = 0;
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[6] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[4] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[13] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x15;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
void BattleSetActorSlotMenuMode(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if (func_8008BED8(a) != 0) {
            BattleActivateSelectedCommand(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[6] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[4] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
extern void BattleSetSlotFlag15A1(u8);
extern u16 g_BattleSlotCounterStep;
void BattleAdvanceSlotMenuStateCounter(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleSetSlotFlag15A1(a);
        g_BattleSlotEntries[a].counter = g_BattleSlotEntries[a].counter + g_BattleSlotCounterStep;
        if (g_BattleSlotEntries[a].counter > g_BattleSlotEntries[a].counterLimit) {
            g_BattleSlotEntries[a].counter = g_BattleSlotEntries[a].counterLimit;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->charRedrawFlags[a] = 1;
        ((BattleActorSlots *)g_BattleCommandQueue)->menuRunning = 0;
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[6] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[4] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[8] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x17;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
extern void BattleLoadStringTableAE(u8);
extern u8 BattleRunFourSlotCommandMenu(u8);
extern void BattleQueueOneActorCommand(u8);
void BattleAdvanceSlotMenuStateAE(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTableAE(a);
        if (BattleRunFourSlotCommandMenu(a) != 0) {
            BattleQueueOneActorCommand(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[4] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}

void BattleAdvanceSlotMenuStateSlotFlags(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if ((g_BattleSlotEntries[a].stateActiveEffectFlags2 & 0x8000) != 0) {
            g_BattleSlotEntries[a].stateActiveEffectFlags2 &= 0x7FFF;
            g_BattleSlotEntries[a].activeEffectFlags3 &= 0x7FFF;
        } else {
            g_BattleSlotEntries[a].stateFlags &= 0xFFDF;
            g_BattleSlotEntries[a].flags &= 0xEFFF;
            g_BattleSlotEntries[a].stateActiveEffectFlags2 |= 0x8000;
            g_BattleSlotEntries[a].activeEffectFlags3 |= 0x8000;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[8] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x17;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 2) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[4] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[6] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
void BattleAdvanceSlotMenuStateSubMode(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if (func_8008BED8(a) != 0) {
            BattleActivateSelectedCommand(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[13] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x15;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[8] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x17;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 2) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[4] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[6] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
extern u8 D_800C48EA;
extern s32 BattleResetRowIndexAndMaybeRefreshStats(u8);
void BattleAdvanceSlotMenuStateRandom(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if (BattleResetRowIndexAndMaybeRefreshStats(((BattleActorSlots *)g_BattleCommandQueue)->actorIndex) != 0) {
            D_800C48EA = 0x40;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[13] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x15;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[4] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[6] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
        break;
    }
}
extern void BattleLoadStringTable96(u8);
extern u8 BattleRunSlotCommandMenu(u8);
void BattleAdvanceSlotMenuStateText(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTable96(a);
        if (BattleRunSlotCommandMenu(a) != 0) {
            BattleQueueOneActorCommand(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[13] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x15;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[12] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[8] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x17;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 2) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[6] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
extern BattleWork *g_BattleDisplayState;
extern u8 g_BattleTaskListBusy;
extern u16 g_BattleSlotFlag8000Mask;
extern u8 g_BattleCommandCharIndex;
// The per-command cost/limit table read by the confirm handler: one u16 row per
// character, three rows of the table before the scalar at g_BattleCommandCharIndex.
extern u16 g_BattleCommandCost[];
extern void BattleInitTargetSlotFlags(u8);
extern void BattleQueueCommandFE(u8);
extern void BattleActorInitSpriteFromSlot(void);
extern void BattleClearFlagC6(void);
extern void BattleClearSlotFlagsIfChar4(u8, u8);
extern void BattlePlaySoundWithBaseId(u8);
extern int func_80086F98(u8, u8);
extern u32 BattleCheckActorCommandAvailable(u8, u8);
extern void BattleAppendCommandRecord(u8);
extern void BattleAdvanceCommandStep(u8, u8);
extern void func_800898F0(u8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083948);
/* 87.92% (1696 bytes against retail's 1708: 3 instructions short). The whole event
   dispatch, the two switches, the cost table and the command queue tail are in place
   and the instruction stream agrees with retail except for delay slots and these
   known gaps:
   (a) retail has 59 nops where our reorg fills the slot (e.g. `j .L80083B34` with a
       nop, and `beq v1,v0,.L80083FCC` / nop / slti). Declaring the function u32 instead
       of void bought 4 bytes of that back and is kept in the parked body; the rest did
       not move with any return type tried.
   (b) the case-4/6/7 fall-through chain that builds the command index (case 4 runs three
       `index++`, case 7 two, case 6 one) is emitted one instruction longer here because
       reorg steals D08's increment into the case-4 `beq` delay slot, which then splits
       the chain retail shares. Source shape matches retail's block order otherwise.
   (c) the two g_BattleCommandCharIndex-based table reads need the base register retail uses, so the
       body spells them `((u16 *)(pMode - 0x15))[0xC + index]` with pMode = &g_BattleCommandCharIndex;
       the plain `g_BattleCommandCost[...]` form makes gcc fold the constant into the symbol
       (%hi(D_800D2C22) through $at) and cannot link. That spelling is ROUGH - the
       original almost certainly indexed one table.
   Table facts for the next round: g_BattleCommandCost is a u16 table; the three reads are
   [index], [0xC + index] and [3*commandKey[0] + 3 + index], so the last two are one row-major
   table indexed 3*character + 3 + command and the g_BattleCommandCharIndex==4 case is that row with
   character 3. s2 (the command index + 1) is built by the case 4/7/6 fall-through.
*/
/*
// The unused return type is load-bearing: with `void` gcc reorg fills delay
// slots retail leaves as nop, which costs 4 bytes per slot.
u32 func_80083948(u8 arg0) {
    u8 ok;
    u8 first;
    s32 index;
    s32 remain;
    u16 counter;

    counter = g_BattleSlotEntries[arg0].counter;
    g_BattleSoundEnabled = 0;
    if (((BattleActorSlots *)g_BattleCommandQueue)->commandQueued != 0) {
        return;
    }
    ok = 0;
    index = 0;
    BattleInitTargetSlotFlags(arg0);
    switch (g_BattleMenuEvent) {
    case 5:
        first = 1;
        if (((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 != 0 && g_BattleCommandCharIndex != 4) {
            BattleQueueCommandFE(arg0);
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
            first = 0;
        }
        if (first != 0) {
            g_BattleDisplayState->spriteWorkListsPending = 0;
            BattleActorInitSpriteFromSlot();
            g_BattleSoundEnabled = 1;
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            BattleClearFlagC6();
            if (g_BattleCommandCharIndex != 4) {
                BattleClearSlotFlagsIfChar4(arg0, 1);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 = 0;
                ((BattleActorSlots *)g_BattleCommandQueue)->commandKeyLength = 0;
            }
        }
        break;
    case 4:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[2] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(BATTLE_SOUND_MENU_DENIED);
        }
        break;
    case 7:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[1] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(BATTLE_SOUND_MENU_DENIED);
        }
        break;
    case 6:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[0] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(BATTLE_SOUND_MENU_DENIED);
        }
        break;
    }
    if (((BattleActorSlots *)g_BattleCommandQueue)->commandAppended == 0) {
        switch (g_BattleMenuEvent) {
        case 0:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot = func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 0);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        case 1:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot = func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 1);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        case 2:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot = func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 2);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        case 3:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot = func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 3);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        }
    }
    g_BattleDisplayState->slotMarks[arg0] = 1;
    if (((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 != 0) {
        if (g_BattleCommandCharIndex != 4
            && (g_BattleCommandCharIndex == 0 || g_BattleCommandCharIndex - 1 < ((BattleActorSlots *)g_BattleCommandQueue)->commandKey[0])) {
            BattleQueueCommandFE(arg0);
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
            return;
        }
        if (BattleMaskHasSlotBit(g_BattleSlotFlag8000Mask,
                                 ((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].targetSlot) != 0) {
            BattleQueueCommandFE(arg0);
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
            return;
        }
    }
    switch (g_BattleMenuEvent) {
    case 4:
        index++;
    case 7:
        index++;
    case 6:
        index++;
        if (g_BattleCommandCharIndex == 4 && ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 = ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 + 1;
            func_80086F98(0xFF, arg0);
            break;
        }
        if ((u8)BattleCheckActorCommandAvailable(index - 1, arg0) == 0) {
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->commandKeyLength == 0) {
            remain = counter - g_BattleCommandCost[index];
        } else {
            u8 *pMode;

            pMode = &g_BattleCommandCharIndex;
            if (*pMode == 4) {
                remain = counter - ((u16 *)(pMode - 0x15))[0xC + index];
            } else {
                remain = counter - ((u16 *)(pMode - 0x12))[
                                     3 * ((BattleActorSlots *)g_BattleCommandQueue)->commandKey[0] + index];
            }
        }
        if (remain >= 0) {
            ok = 1;
        }
        if (ok != 0) {
            BattlePlaySoundWithBaseId(0x4D);
            g_BattleTaskListBusy = 1;
            BattleAppendCommandRecord(arg0);
            if (((BattleActorSlots *)g_BattleCommandQueue)->commandKeyLength != 0) {
                ((BattleActorSlots *)g_BattleCommandQueue)->unk2E4 = ((BattleActorSlots *)g_BattleCommandQueue)->unk2E4 + 1;
            }
            if (g_BattleDisplayState->primListsPending != 0) {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x65;
                ((BattleActorSlots *)g_BattleCommandQueue)->lastMenuState = BATTLE_SLOT_NONE;
                ((BattleActorSlots *)g_BattleCommandQueue)->colorShift = 1;
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x19;
                ((BattleActorSlots *)g_BattleCommandQueue)->lastMenuState = 0x19;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E4 =
                (u8)func_80086F98(index - 1, arg0) == 0;
            BattleAdvanceCommandStep(arg0, index);
            g_BattleSlotEntries[arg0].counter =
                g_BattleSlotEntries[arg0].counter - g_BattleCommandCost[((BattleActorSlots *)g_BattleCommandQueue)->commandCostIndex];
            func_800898F0(arg0);
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 = ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 + 1;
        } else {
            g_BattleSoundEnabled = 1;
            BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            g_BattleSoundEnabled = 0;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->unk2E4 != 0) {
            BattleQueueCommandFE(arg0);
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
        }
        break;
    case 5:
        break;
    default:
        if (g_BattleCommandCharIndex == 4 && ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 = ((BattleActorSlots *)g_BattleCommandQueue)->unk2E7 + 1;
            func_80086F98(0xFF, arg0);
        }
        break;
    }
}
*/
u8 BattlePairTargetable(u8 arg0, u8 arg1) {
    u8 ret;

    ret = 0;
    if (g_BattleSlotEnabledFlags[arg1] != 0) {
        if (D_800C3EB7[arg1][0] == 0) {
            if (g_BattleSlotFlags[arg1 * 8] == 0) {
                if (*(g_BattleSlotParamTable + (g_BattleSlotRecords[arg0][0] * 0x40 +
                                               g_BattleSlotRecords[arg1][0] * 8) + 0x140) == 0) {
                    ret = (g_BattleSlotEntries[arg1].flags & BATTLE_SLOT_FLAGS_UNTARGETABLE) == 0;
                }
            } else if ((D_800CCE08[arg1].flags & BATTLE_SLOT_FLAGS_UNTARGETABLE) == 0) {
                ret = 1;
            }
        }
    }
    return ret;
}
u8 BattleActorTargetableWithOverride(u8 arg0, u8 arg1) {
    u8 ret;

    ret = 0;
    if (g_BattleSlotEnabledFlags[arg0] != 0) {
        if (D_800C3EB7[arg0][0] == 0) {
            if (g_BattleSlotFlags[arg0 * 8] == 0) {
                if (arg1 == 0) {
                    ret = (g_BattleSlotEntries[arg0].flags & BATTLE_SLOT_FLAGS_UNTARGETABLE) == 0;
                } else {
                    ret = 1;
                }
            } else {
                if (arg1 == 0) {
                    if ((D_800CCE08[arg0].flags & BATTLE_SLOT_FLAGS_UNTARGETABLE) == 0) {
                        ret = 1;
                    }
                } else {
                    ret = 1;
                }
            }
        }
    }
    return ret;
}
u8 BattleBuildTargetCandidateListByRow(u8 arg0) {
    s32 i;
    s32 n;
    s32 count;
    u8 tmp;
    u8 list[BATTLE_SLOT_COUNT];
    u8 sameRow[BATTLE_SLOT_COUNT];
    u8 *p;

    g_BattleTargetCandidateCount = 0;
    for (i = 0; i < BATTLE_SLOT_COUNT; i++) {
        list[i] = BATTLE_SLOT_NONE;
        g_BattleTargetCandidates[i] = BATTLE_SLOT_NONE;
        sameRow[i] = 0;
    }
    if (arg0 < BATTLE_PARTY_SLOT_COUNT) {
        i = BATTLE_PARTY_SLOT_COUNT;
        count = 0;
        p = list;
        for (; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
            if (BattlePairTargetable(arg0, i) != 0) {
                *p = i;
                p++;
                count++;
                g_BattleTargetCandidateCount++;
            }
        }
    } else {
        i = 0;
        count = 0;
        p = list;
        for (; i < BATTLE_PARTY_SLOT_COUNT; i++) {
            if (BattlePairTargetable(arg0, i) != 0) {
                *p = i;
                p++;
                count++;
                g_BattleTargetCandidateCount++;
            }
        }
    }
    n = 0;
    for (i = 0; i < count; i++) {
        if (g_BattleSlotRecords[arg0][0] == g_BattleSlotRecords[list[i]][0]) {
            g_BattleTargetCandidates[n] = list[i];
            list[i] = BATTLE_SLOT_NONE;
            sameRow[n] = 1;
            n++;
        }
    }
    for (i = 0; i < count; i++) {
        if (list[i] != BATTLE_SLOT_NONE) {
            g_BattleTargetCandidates[n] = list[i];
            n++;
        }
    }
    if (sameRow[0] != 0) {
        for (i = 1; i < count; i++) {
            if (sameRow[i] != 0
                && g_BattleSlotEntriesBase[g_BattleTargetCandidates[0]].hp > g_BattleSlotEntriesBase[g_BattleTargetCandidates[i]].hp) {
                tmp = g_BattleTargetCandidates[0];
                g_BattleTargetCandidates[0] = g_BattleTargetCandidates[i];
                g_BattleTargetCandidates[i] = tmp;
            }
        }
    } else {
        for (i = 1; i < count; i++) {
            if (g_BattleSlotEntriesBase[g_BattleTargetCandidates[0]].hp > g_BattleSlotEntriesBase[g_BattleTargetCandidates[i]].hp) {
                tmp = g_BattleTargetCandidates[0];
                g_BattleTargetCandidates[0] = g_BattleTargetCandidates[i];
                g_BattleTargetCandidates[i] = tmp;
            }
        }
    }
    return g_BattleTargetCandidates[0];
}

u8 BattleBuildTargetCandidateListByGroup(u8 arg0, u8 arg1, u8 arg2) {
    s32 i;
    s32 j;
    s32 n;
    s32 count1;
    u8 start1;
    s32 count2 = 0;
    u8 start2;
    u8 *p;
    u8 val;

    val = BATTLE_SLOT_NONE;
    i = BATTLE_SLOT_COUNT - 1;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    switch (arg0) {
    case 0:
        start1 = BATTLE_PARTY_SLOT_COUNT;
        count1 = BATTLE_OTHER_SLOT_COUNT;
        break;
    case 1:
        start1 = 0;
        count1 = BATTLE_PARTY_SLOT_COUNT;
        break;
    case 2:
        if (arg2 == 0) {
            start1 = BATTLE_PARTY_SLOT_COUNT;
            count1 = BATTLE_OTHER_SLOT_COUNT;
            start2 = 0;
            count2 = BATTLE_PARTY_SLOT_COUNT;
        } else {
            start1 = 0;
            count1 = BATTLE_PARTY_SLOT_COUNT;
            start2 = BATTLE_PARTY_SLOT_COUNT;
            count2 = BATTLE_OTHER_SLOT_COUNT;
        }
        break;
    }
    n = 0;
    g_BattleTargetCandidateCount = 0;
    g_BattleTargetSlotMask = 0;
    for (i = start1; --count1 >= 0; i++) {
        j = i;
        if (BattleActorTargetableWithOverride(j, arg1) != 0) {
            g_BattleTargetCandidates[n] = j;
            g_BattleTargetSlotMask |= BattleSlotBitMask(j);
            n++;
            g_BattleTargetCandidateCount++;
        }
    }
    for (i = start2; --count2 >= 0; i++) {
        j = i;
        if (BattleActorTargetableWithOverride(j, arg1) != 0) {
            g_BattleTargetCandidates[n] = j;
            g_BattleTargetSlotMask |= BattleSlotBitMask(j);
            n++;
            g_BattleTargetCandidateCount++;
        }
    }
    return g_BattleTargetCandidates[0];
}
u8 BattleBuildTargetCandidateList(u8 arg0) {
    s32 i;
    s32 j;
    s32 n;
    s32 k;
    u8 *p;
    u8 val;

    k = BATTLE_OTHER_SLOT_COUNT;
    val = BATTLE_SLOT_NONE;
    i = BATTLE_SLOT_COUNT - 1;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    n = 0;
    g_BattleTargetCandidateCount = 0;
    g_BattleTargetSlotMask = 0;
    for (i = BATTLE_PARTY_SLOT_COUNT; --k >= 0; i++) {
        j = i;
        if (BattlePairTargetable(arg0, j) != 0) {
            g_BattleTargetCandidates[n] = j;
            g_BattleTargetSlotMask |= BattleSlotBitMask(j);
            n++;
            g_BattleTargetCandidateCount++;
        }
    }
    return g_BattleTargetCandidates[0];
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084854);
/* 87.82%: 536 bytes against retail's 548. The saved-register map, the switch
   tree, the ratan2 call, the case-body sharing and the loop tail all agree with
   retail (s0 flag, s1 candidate pointer, s2 arg1, s3 arg0 * 0x1C, s4 arg0, s5
   result, s6 best; `switch (arg1)` rather than a `mode` local - the permuter found
   that, 84.85 -> 87.82). What is left, all four measured:
   1. The second axis. Retail RELOADS *p and recomputes the 0x1C index for the x
      difference (lbu, nop, sll, subu, sll = 5 instructions); every spelling tried
      here makes gcc 2.6.0 CSE the index across the first axis' branch join and
      reuse the register (tools/scripts/probe_twoaxes.c reproduces the CSE in 20
      lines, so the body shape is not the cause). The permuter's way of forcing the
      reload was `D_800C3EBE[result = *p].x`, which stores *p into result on every
      candidate instead of only on a new best one - wrong code, do not port it.
   2. Prologue order. We emit the arg0-derived invariants (s4, s3) after the
      `la s1` group; retail has them before s2 and s1, i.e. its entry-block
      statements put the loop pointer last, after the hoisted ones.
   3. The loop test is `slt` in retail (signed); `p < D_800C3E9B` gives `sltu`.
   4. Retail keeps a nop after each of the two `sltiu` range tests inside case 2
      where we steal the `flag = 1` or into the branch delay slot.
   The absolute difference is NOT a wall any more: `if (d < 0) d = -d;` already
   gives retail's two multiplies with one shared mflo (the old note here claimed
   otherwise); only the negate form differs, negu against retail's reversed
   `subu v0,v0,a0`, which is the same instruction count. */
/*
u8 func_80084854(u8 arg0, u8 arg1) {
    u8 *p;
    u8 result;
    s32 angle;
    s32 flag;
    s32 dx;
    s32 dy;
    s32 dist;
    s32 best;

    best = 0xFFFFFF;
    result = arg0;
    p = g_BattleTargetCandidates;
    do {
        if (*p != BATTLE_SLOT_NONE && *p != arg0) {
            flag = 0;
            angle = ratan2(D_800C3EBE[*p].y - D_800C3EBE[arg0].y,
                           D_800C3EBE[*p].x - D_800C3EBE[arg0].x);
            switch (arg1) {
            case 0:
                if ((u16)(angle + 0x200) < 0x400) {
                    flag = 1;
                }
                break;
            case 1:
                if ((u16)(angle + 0x600) < 0x400) {
                    flag = 1;
                }
                break;
            case 2:
                if ((u16)(angle + 0x800) < 0x200) {
                    flag = 1;
                }
                if ((u16)(angle - 0x600) < 0x201) {
                    flag = 1;
                }
                break;
            case 3:
                if ((u16)(angle - 0x200) < 0x400) {
                    flag = 1;
                }
                break;
            }
            if (flag != 0) {
                dx = D_800C3EBE[*p].y - D_800C3EBE[arg0].y;
                if (dx < 0) {
                    dx = -dx;
                }
                dist = dx * dx;
                dy = D_800C3EBE[*p].x - D_800C3EBE[arg0].x;
                if (dy < 0) {
                    dy = -dy;
                }
                dist += dy * dy;
                if (dist < best) {
                    best = dist;
                    result = *p;
                }
            }
        }
    } while (++p < D_800C3E9B);
    return result;
}
*/

void BattleEnsureTargetSlotInCandidateList(u8 arg0) {
    s32 off;
    s32 found;
    s32 i;

    off = arg0 * 0x40;
    g_BattleCommandQueue[0x2E8] = (g_BattleCommandQueue + off)[0x3C];
    found = 0;
    BattleBuildTargetCandidateListByRow(arg0);
    for (i = 0; i < g_BattleTargetCandidateCount; i++) {
        if (g_BattleTargetCandidates[i] == (g_BattleCommandQueue + off)[0x3C]) {
            found++;
            break;
        }
    }
    if (found == 0) {
        g_BattleCommandQueue[0x2E8] = g_BattleTargetCandidates[0];
    }
}
extern void BattleBeginAnimTask(u16);
extern void BattleApplySlotMaskToEffects(u16);
extern void BattleUpdateFrame(void);
extern u8 func_80084854(u8, u8);
s32 BattleSelectTargetSlotLoop(u8 arg0) {
    u8 b;
    u8 m;
    u8 state;

    b = ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot;
    state = 2;
    g_BattleMenuEvent = 8;
    while (1) {
        while (g_BattleMenuEvent == 8) {
            m = b;
            BattleBeginAnimTask(BattleSlotBitMask(arg0) | BattleSlotBitMask(m));
            BattleApplySlotMaskToEffects(BattleSlotBitMask(m));
            BattleUpdateFrame();
        }
        switch (g_BattleMenuEvent) {
        case 4:
        case 6:
        case 7:
            state = 1;
            ((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].targetSlot = b;
            break;
        case 5:
            BattleBeginAnimTask(BattleSlotBitMask(arg0) |
                          BattleSlotBitMask(((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].targetSlot));
            state = 0;
            BattleApplySlotMaskToEffects(BattleSlotBitMask(((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].targetSlot));
            break;
        case 0:
            b = func_80084854(b, 0);
            break;
        case 1:
            b = func_80084854(b, 1);
            break;
        case 2:
            b = func_80084854(b, 2);
            break;
        case 3:
            b = func_80084854(b, 3);
            break;
        }
        g_BattleMenuEvent = 8;
        if (state != 2) {
            break;
        }
    }
    return state;
}

u16 BattleGetSameRowSlotMask(void) {
    s32 i;
    u16 ret;

    ret = 0;
    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (BattleMaskHasSlotBit(g_BattleTargetSlotMask, i) != 0) {
            if (g_BattleSlotRecords[g_BattleTargetSlot][0] == g_BattleSlotRecords[i][0]) {
                ret |= BattleSlotBitMask(i);
            }
        }
    }
    return ret;
}
extern u16 BattleSlotBitMaskInverted(u8);
extern u8 D_800C3E9A[];
// arg0 IS the flags word (it is reassigned per arg3 below): keeping it a
// parameter, rather than copying it into a local, is what makes gcc home it
// into $s6 before arg2's copy in the entry block.
u16 BattleSelectTargetCandidates(u16 flags, u16 arg1, u8 arg2, u8 arg3, u8 arg4) {
    s32 i;
    s32 n;
    s32 hi;
    s32 off;
    s32 off8;
    u16 mask;
    u8 grp;
    u8 first;
    u8 *p;
    u8 val;

    hi = 0;
    switch (arg3) {
    case 0:
        flags = arg1;
        break;
    case 1:
        flags = BATTLE_TARGET_FLAGS_OTHER_SLOTS | BATTLE_TARGET_FLAGS_BOTH_GROUPS;
        break;
    case 2:
        flags = 0x2001;
        break;
    }
    if ((flags & BATTLE_TARGET_FLAGS_SINGLE_TARGET) != 0) {
        hi = 1;
    }
    if ((flags & BATTLE_TARGET_FLAGS_OTHER_SLOTS) != 0) {
        first = 0;
        grp = 0;
        mask = 0xFFF8;
    } else {
        first = 1;
        grp = 1;
        mask = BATTLE_PARTY_SLOT_MASK;
    }
    if ((flags & BATTLE_TARGET_FLAGS_BOTH_GROUPS) != 0) {
        grp = 2;
        mask = 0xFFFF;
    }
    if (arg4 != 0) {
        g_BattleTargetSlot = BattleBuildTargetCandidateList(arg2);
    } else {
        g_BattleTargetSlot = BattleBuildTargetCandidateListByGroup(grp, hi, first);
    }
    g_BattleTargetSlotMask &= mask;
    if ((flags & BATTLE_TARGET_FLAGS_SINGLE_TARGET) != 0) {
        n = 0;
        g_BattleTargetSlot = BATTLE_SLOT_NONE;
        for (i = 0, off = 0, off8 = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++, off8 += 8) {
            if (BattleMaskHasSlotBit(g_BattleTargetSlotMask, i) != 0) {
                if (g_BattleSlotFlags[off8] == 0) {
                    if ((*(u16 *)((u8 *)g_BattleSlotEntries + off) & 0x8000) != 0) {
                        g_BattleTargetSlot = i;
                        g_BattleTargetCandidates[n] = i;
                        n++;
                    } else {
                        g_BattleTargetSlotMask &= BattleSlotBitMaskInverted(i);
                    }
                } else {
                    if ((*(u16 *)((u8 *)D_800CCE08 + off) & 0x8000) != 0) {
                        g_BattleTargetSlot = i;
                        g_BattleTargetCandidates[n] = i;
                        n++;
                    } else {
                        g_BattleTargetSlotMask &= BattleSlotBitMaskInverted(i);
                    }
                }
            }
            off += sizeof(BattleUnk170);
        }
        while (n < BATTLE_ACTOR_SLOT_COUNT) {
            g_BattleTargetCandidates[n] = BATTLE_SLOT_NONE;
            n++;
        }
    } else if ((flags & BATTLE_TARGET_FLAGS_GIVEN_SLOT) != 0) {
        g_BattleTargetSlot = arg2;
        g_BattleTargetSlotMask = BattleSlotBitMask(arg2);
        val = BATTLE_SLOT_NONE;
        n = BATTLE_ACTOR_SLOT_COUNT - 1;
        p = D_800C3E9A;
        for (; n >= 0; n--) {
            *p-- = val;
        }
    }
    return flags & 0xFFFF;
}
extern u8 func_80084854(u8, u8);
extern u8 *g_BattlePulseTrianglePrims;
u8 BattleRunTargetSlotSelection(u16 arg0, u8 arg1, u8 arg2) {
    s32 i;
    u16 mask;
    u8 state;
    u8 last;
    u8 best;

    last = BATTLE_MENU_EVENT_NONE;
    BattleSelectTargetCandidates(arg0, arg0, arg1, 0, arg2);
    mask = g_BattleTargetSlotMask;
    state = g_BattleTargetSlot == BATTLE_SLOT_NONE;
    while (state == 0) {
        BattleUpdateFrame();
        switch (arg0 & 0xF) {
        case 0:
            BattleApplySlotMaskToEffects(BattleSlotBitMask(g_BattleTargetSlot));
            BattleBeginAnimTask(BattleSlotBitMask(g_BattleTargetSlot));
            break;
        case 1:
            BattleApplySlotMaskToEffects(g_BattleTargetSlotMask);
            BattleBeginAnimTask(g_BattleTargetSlotMask);
            break;
        case 2:
            mask = BattleGetSameRowSlotMask();
            BattleApplySlotMaskToEffects(mask);
            BattleBeginAnimTask(mask);
            break;
        }
        switch (g_BattleMenuEvent) {
        case 5:
            state = 1;
            break;
        case 4:
            g_BattleTargetSlotMask = mask;
            state = 2;
            break;
        case 0:
            best = func_80084854(g_BattleTargetSlot, 0);
            if (BattleMaskHasSlotBit(g_BattleTargetSlotMask, best) != 0) {
                g_BattleTargetSlot = best;
            }
            break;
        case 1:
            best = func_80084854(g_BattleTargetSlot, 1);
            if (BattleMaskHasSlotBit(g_BattleTargetSlotMask, best) != 0) {
                g_BattleTargetSlot = best;
            }
            break;
        case 2:
            best = func_80084854(g_BattleTargetSlot, 2);
            if (BattleMaskHasSlotBit(g_BattleTargetSlotMask, best) != 0) {
                g_BattleTargetSlot = best;
            }
            break;
        case 3:
            best = func_80084854(g_BattleTargetSlot, 3);
            if (BattleMaskHasSlotBit(g_BattleTargetSlotMask, best) != 0) {
                g_BattleTargetSlot = best;
            }
            break;
        case 6:
        case 7:
            break;
        }
        if (g_BattleMenuEvent != last) {
            last = g_BattleMenuEvent;
            for (i = 0; i < BATTLE_TARGET_DIRECTION_COUNT; i++) {
                if (func_80084854(g_BattleTargetSlot, i) != g_BattleTargetSlot) {
                    *(g_BattlePulseTrianglePrims + i + 0xE6) = 1;
                } else {
                    *(g_BattlePulseTrianglePrims + i + 0xE6) = 0;
                }
            }
        }
    }
    return state - 1;
}
u32 BattleActorPosXGreaterThan(u8 arg0, u8 arg1) {
    return D_800C3EBE[arg0].x > D_800C3EBE[arg1].x;
}
void BattleClearSlotCurveState(void) {
    s32 i;

    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        g_BattleCurveModes[i] = BATTLE_CURVE_MODE_NONE;
        g_BattleCurveValues[i] = 0;
    }
}
void BattleClearSlotCurveEntry(void) {
    s32 i;

    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        g_BattleCommandRows[g_BattleCommandQueue[0x2DA]].curveTargets[i] = 0;
        g_BattleCommandRows[g_BattleCommandQueue[0x2DA]].curveTargetModes[i] = BATTLE_CURVE_MODE_NONE;
        g_BattleCommandRows[g_BattleCommandQueue[0x2DA]].curveValues[i] = 0;
        g_BattleCommandRows[g_BattleCommandQueue[0x2DA]].curveModes[i] = BATTLE_CURVE_MODE_NONE;
    }
}
extern u16 g_BattleCurveTargets[][2];
extern u8 g_BattleCurveTargetModes[];
extern u8 g_BattleCurveModes[];
extern u16 g_BattleCurveValues[];
void BattleApplySlotCurve(u8 arg0) {
    s32 i;

    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        g_BattleCommandRows[arg0].curveTargets[i] = g_BattleCurveTargets[i][0];
        g_BattleCommandRows[arg0].curveTargetModes[i] = g_BattleCurveTargetModes[i];
        switch (g_BattleCurveTargetModes[i]) {
        case 0:
        case 5:
            if (g_BattleCurveModes[i] == 0 || g_BattleCurveModes[i] == 5) {
                g_BattleCurveValues[i] = g_BattleCurveValues[i] + g_BattleCurveTargets[i][0];
            } else if (g_BattleCurveModes[i] == 2) {
                if ((s16)g_BattleCurveTargets[i][0] - (s16)g_BattleCurveValues[i] >= 0) {
                    g_BattleCurveValues[i] = (s16)g_BattleCurveTargets[i][0] - (s16)g_BattleCurveValues[i];
                    g_BattleCurveModes[i] = 0;
                } else {
                    g_BattleCurveValues[i] = (s16)g_BattleCurveValues[i] - (s16)g_BattleCurveTargets[i][0];
                }
            } else {
                g_BattleCurveValues[i] = g_BattleCurveTargets[i][0];
                g_BattleCurveModes[i] = g_BattleCurveTargetModes[i];
            }
            break;
        case 2:
            if (g_BattleCurveModes[i] == 2) {
                g_BattleCurveValues[i] = g_BattleCurveValues[i] + g_BattleCurveTargets[i][0];
            } else if (g_BattleCurveModes[i] == 0 || g_BattleCurveModes[i] == 5) {
                if ((s16)g_BattleCurveTargets[i][0] - (s16)g_BattleCurveValues[i] < 0) {
                    g_BattleCurveValues[i] = (s16)g_BattleCurveValues[i] - (s16)g_BattleCurveTargets[i][0];
                } else {
                    g_BattleCurveValues[i] = (s16)g_BattleCurveTargets[i][0] - (s16)g_BattleCurveValues[i];
                    g_BattleCurveModes[i] = 2;
                }
            } else {
                g_BattleCurveValues[i] = g_BattleCurveTargets[i][0];
                g_BattleCurveModes[i] = g_BattleCurveTargetModes[i];
            }
            break;
        }
        g_BattleCommandRows[arg0].curveValues[i] = g_BattleCurveValues[i];
        g_BattleCommandRows[arg0].curveModes[i] = g_BattleCurveModes[i];
    }
}
