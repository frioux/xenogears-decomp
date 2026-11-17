#include "common.h"

// The sound played when the battle menu refuses an input (battle/main/misc11 and
// misc16 call it with the same id; kept here because this unit does not include actor.h).
#define BATTLE_SOUND_MENU_DENIED 0x4F

// The slot id that means "no slot": an empty candidate list entry, a slot that
// targets nothing.
#define BATTLE_SLOT_NONE 0xFF
// The mode BattleClearSlotCurveState and BattleClearSlotCurveEntry store to mean
// that no curve runs on the slot; BattleApplySlotCurve's switch never matches it.
#define BATTLE_CURVE_MODE_NONE 0xFF
// Slot flags bits that make a slot untargetable: BattlePairTargetable and
// BattleActorTargetableWithOverride call it targetable only while they are clear.
#define BATTLE_SLOT_FLAGS_UNTARGETABLE 0xC001

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
extern u8 g_BattleSlotRecords[][0x1C];
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
    /* 0x008 */ u16 unk8;
    /* 0x00A */ u8 padA[0x56];
    /* 0x060 */ u16 counter; // steps by g_BattleSlotCounterStep up to counterLimit
    /* 0x062 */ u16 counterLimit;
    /* 0x064 */ u8 pad64[0x40];
    /* 0x0A4 */ u16 stateFlags; // the +0xA4 state block's copy, tested like flags
    /* 0x0A6 */ u8 padA6[0x2];
    /* 0x0A8 */ u16 unkA8;
    /* 0x0AA */ u8 padAA[0xC6];
} BattleUnk170; // Size: 0x170

// The same entry seen from g_BattleSlotEntriesBase (+0x4C of it): only the word the
// candidate sorts below minimise is named.
typedef struct {
    /* 0x000 */ u16 rank;
    /* 0x002 */ u8 unk2[0x16E];
} BattleSlotBaseEntry; // Size: 0x170

extern u8 g_BattleSlotEnabledFlags[];
extern u8 D_800C3EB7[][0x1C];
extern u8 g_BattleSlotFlags[];
extern u8 *g_BattleSlotParamTable;
extern BattleUnk170 g_BattleSlotEntries[];
// The same slot table seen from +0xA4, so its `flags` is the entry's stateFlags.
extern BattleUnk170 D_800CCE08[];
extern BattleSlotBaseEntry g_BattleSlotEntriesBase[];

typedef struct {
    /* 0x00 */ u8 unk0[0x24];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u16 unk26;
    /* 0x28 */ u16 unk28;
    /* 0x2A */ u16 unk2A;
    /* 0x2C */ u16 unk2C;
    /* 0x2E */ u16 unk2E;
    /* 0x30 */ u16 unk30;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u16 unk34;
    /* 0x36 */ u16 unk36;
    /* 0x38 */ u8 unk38[0x4];
    /* 0x3C */ u8 targetSlot; // the slot this one targets, 0xFF for none
    /* 0x3D */ u8 unk3D[0x3];
} BattleActorSlot; // Size: 0x40

// 0x2F8-byte block of actor slots plus the slot menu state that follows them.
typedef struct {
    /* 0x000 */ BattleActorSlot slot[0xB];
    /* 0x2C0 */ u8 unk2C0[0x13];
    /* 0x2D3 */ u8 actorIndex; // the slot the queued command is being built for
    /* 0x2D4 */ u8 unk2D4[0x9];
    /* 0x2DD */ u8 menuState; // the state each menu event handler switches to
    /* 0x2DE */ u8 eventHandled; // handlers set it to release the update wait loop
    /* 0x2DF */ u8 unk2DF[0x9];
    /* 0x2E8 */ u8 targetSlot; // the slot being selected, the menu's copy of slot[].targetSlot
    /* 0x2E9 */ u8 unk2E9;
    /* 0x2EA */ u8 unk2EA;
    /* 0x2EB */ u8 charRedrawFlags[0xB];
    /* 0x2F6 */ u8 repeatPending; // the last event was this one, so take it as a repeat
    /* 0x2F7 */ u8 unk2F7;
} BattleActorSlots; // Size: 0x2F8

extern u8 g_BattleMenuEvent;
extern void BattlePlaySoundIfEnabled(u8);
extern u8 func_8008BED8(u8);
extern void BattleActivateSelectedCommand(u8);
extern u8 g_BattleLastMenuEvent;
extern void func_800826CC(u8);
void BattleAdvanceSlotMenuState(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        func_800826CC(a);
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk26 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk32 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 8;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2C == 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk30 == 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk24 != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk36 == 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk24 != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
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
        ((BattleActorSlots *)g_BattleCommandQueue)->unk2EA = 0;
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk24 != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2C == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x17;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
extern void BattleLoadStringTableAE(u8);
extern u8 func_8008B478(u8);
extern void func_8008ADD0(u8);
void BattleAdvanceSlotMenuStateAE(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTableAE(a);
        if (func_8008B478(a) != 0) {
            func_8008ADD0(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk24 == 0) {
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
        if ((g_BattleSlotEntries[a].unkA8 & 0x8000) != 0) {
            g_BattleSlotEntries[a].unkA8 &= 0x7FFF;
            g_BattleSlotEntries[a].unk8 &= 0x7FFF;
        } else {
            g_BattleSlotEntries[a].stateFlags &= 0xFFDF;
            g_BattleSlotEntries[a].flags &= 0xEFFF;
            g_BattleSlotEntries[a].unkA8 |= 0x8000;
            g_BattleSlotEntries[a].unk8 |= 0x8000;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2C == 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk24 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk28 != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E == 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk36 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x15;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2C == 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk24 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk28 != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
extern u8 D_800C48EA;
extern s32 func_8009A9D0(u8);
void BattleAdvanceSlotMenuStateRandom(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if (func_8009A9D0(((BattleActorSlots *)g_BattleCommandQueue)->actorIndex) != 0) {
            D_800C48EA = 0x40;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk36 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x15;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk24 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk28 != 0) {
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
extern u8 func_8008CFB8(u8);
void BattleAdvanceSlotMenuStateText(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTable96(a);
        if (func_8008CFB8(a) != 0) {
            func_8008ADD0(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x18;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk36 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x15;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E != 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2C == 0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x13;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083948);
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
    u8 list[12];
    u8 sameRow[12];
    u8 *p;

    g_BattleTargetCandidateCount = 0;
    for (i = 0; i < 12; i++) {
        list[i] = BATTLE_SLOT_NONE;
        g_BattleTargetCandidates[i] = BATTLE_SLOT_NONE;
        sameRow[i] = 0;
    }
    if (arg0 < 3) {
        i = 3;
        count = 0;
        p = list;
        for (; i < 0xB; i++) {
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
        for (; i < 3; i++) {
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
                && g_BattleSlotEntriesBase[g_BattleTargetCandidates[0]].rank > g_BattleSlotEntriesBase[g_BattleTargetCandidates[i]].rank) {
                tmp = g_BattleTargetCandidates[0];
                g_BattleTargetCandidates[0] = g_BattleTargetCandidates[i];
                g_BattleTargetCandidates[i] = tmp;
            }
        }
    } else {
        for (i = 1; i < count; i++) {
            if (g_BattleSlotEntriesBase[g_BattleTargetCandidates[0]].rank > g_BattleSlotEntriesBase[g_BattleTargetCandidates[i]].rank) {
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
    i = 0xB;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    switch (arg0) {
    case 0:
        start1 = 3;
        count1 = 8;
        break;
    case 1:
        start1 = 0;
        count1 = 3;
        break;
    case 2:
        if (arg2 == 0) {
            start1 = 3;
            count1 = 8;
            start2 = 0;
            count2 = 3;
        } else {
            start1 = 0;
            count1 = 3;
            start2 = 3;
            count2 = 8;
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

    k = 8;
    val = BATTLE_SLOT_NONE;
    i = 11;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    n = 0;
    g_BattleTargetCandidateCount = 0;
    g_BattleTargetSlotMask = 0;
    for (i = 3; --k >= 0; i++) {
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
/* 84.85%: 536 bytes against retail's 548 (3 instructions short). The saved-register
   map now agrees with retail (s0 flag, s1 candidate pointer, s2 mode, s3 arg0*0x1C,
   s4 arg0&0xFF, s5 result, s6 best) after three fixes: (a) BattleUnk1C's 0x2 field
   typed u16 unk2 - D_800C3EC0 is D_800C3EBE+2 read as a u16 on a 0x1C stride, so
   D_800C3EC0[i * 14] and D_800C3EBE[i].y are the same access; (b) flag = 0 moved
   ABOVE the ratan2 call (retail sets it in the beq delay slot before the call, and
   that is what keeps it live across the call in s0); (c) best = 0xFFFFFF written
   first and declared s32 (retail compares with slt, not sltu). What is left:
   retail's absolute difference is the REVERSED subtraction ('subu v0,v0,a0; mult
   v0,v0') where 'if (d < 0) d = -d;' makes gcc emit 'negu v1,v1; mult v1,v1'; the
   loop test is 'slt' (signed) where 'p < D_800C3E9B' gives 'sltu'; and we are 3
   instructions short (we emit an extra mflo/nop pair retail does not have). Permuter
   job, not a hand job.
*/
/*
u8 func_80084854(u8 arg0, u8 arg1) {
    u8 *p;
    u8 result;
    s32 mode;
    s32 angle;
    s32 flag;
    s32 dx;
    s32 dy;
    s32 dist;
    s32 best;

    best = 0xFFFFFF;
    result = arg0;
    mode = arg1;
    p = g_BattleTargetCandidates;
    do {
        if (*p != 0xFF && *p != arg0) {
            flag = 0;
            angle = ratan2(D_800C3EBE[*p].y - D_800C3EBE[arg0].y,
                           D_800C3EBE[*p].x - D_800C3EBE[arg0].x);
            switch (mode) {
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
extern void func_800BCD98(u16);
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
            func_800BCD98(BattleSlotBitMask(m));
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
            func_800BCD98(BattleSlotBitMask(((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].targetSlot));
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
    for (i = 0; i < 0xB; i++) {
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
        flags = 0x3000;
        break;
    case 2:
        flags = 0x2001;
        break;
    }
    if ((flags & 0x8000) != 0) {
        hi = 1;
    }
    if ((flags & 0x1000) != 0) {
        first = 0;
        grp = 0;
        mask = 0xFFF8;
    } else {
        first = 1;
        grp = 1;
        mask = 7;
    }
    if ((flags & 0x2000) != 0) {
        grp = 2;
        mask = 0xFFFF;
    }
    if (arg4 != 0) {
        g_BattleTargetSlot = BattleBuildTargetCandidateList(arg2);
    } else {
        g_BattleTargetSlot = BattleBuildTargetCandidateListByGroup(grp, hi, first);
    }
    g_BattleTargetSlotMask &= mask;
    if ((flags & 0x8000) != 0) {
        n = 0;
        g_BattleTargetSlot = BATTLE_SLOT_NONE;
        for (i = 0, off = 0, off8 = 0; i < 0xB; i++, off8 += 8) {
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
            off += 0x170;
        }
        while (n < 0xB) {
            g_BattleTargetCandidates[n] = BATTLE_SLOT_NONE;
            n++;
        }
    } else if ((flags & 0x4000) != 0) {
        g_BattleTargetSlot = arg2;
        g_BattleTargetSlotMask = BattleSlotBitMask(arg2);
        val = BATTLE_SLOT_NONE;
        n = 10;
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

    last = 0xFE;
    BattleSelectTargetCandidates(arg0, arg0, arg1, 0, arg2);
    mask = g_BattleTargetSlotMask;
    state = g_BattleTargetSlot == BATTLE_SLOT_NONE;
    while (state == 0) {
        BattleUpdateFrame();
        switch (arg0 & 0xF) {
        case 0:
            func_800BCD98(BattleSlotBitMask(g_BattleTargetSlot));
            BattleBeginAnimTask(BattleSlotBitMask(g_BattleTargetSlot));
            break;
        case 1:
            func_800BCD98(g_BattleTargetSlotMask);
            BattleBeginAnimTask(g_BattleTargetSlotMask);
            break;
        case 2:
            mask = BattleGetSameRowSlotMask();
            func_800BCD98(mask);
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
            for (i = 0; i < 4; i++) {
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

    for (i = 0; i < 11; i++) {
        g_BattleCurveModes[i] = BATTLE_CURVE_MODE_NONE;
        g_BattleCurveValues[i] = 0;
    }
}
void BattleClearSlotCurveEntry(void) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
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

    for (i = 0; i < 0xB; i++) {
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
