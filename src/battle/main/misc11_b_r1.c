#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

// Played when the pressed menu event has no command bound to it.
#define BATTLE_SOUND_MENU_DENIED 0x4F

// A slot's targetSlot when the command being built has no target yet.
#define BATTLE_SLOT_NONE 0xFF

// Candidate scan order for target selection.
typedef struct {
    /* 0x0 */ u8 cursor;     // next entry of order[] to consider
    /* 0x1 */ u8 order[0xB]; // slot ids, in candidate order
} BattleTargetOrder; // Size: 0xC

extern u8 *g_BattleCommandQueue;

extern u8 *g_BattleEntry3StringTable;
extern u8 *g_BattleDisplayState;
extern u8 *g_BattleSpriteColumnWork;
extern void *g_BattleSlotStringTable;
extern u8 *g_BattlePrimPool;
extern u8 g_BattleCommandCodes[];
extern u8 g_BattleCommandSlotIds[];
extern u8 g_BattleSoundEnabled;
extern u8 g_BattleSlotFlags[];
extern BattleTargetOrder g_BattleTargetScan;
extern u8 D_800C204C;
extern u8 D_800C3202[];
extern u8 *g_BattleGraphicsWork;
extern volatile s32 g_BattlePrimStartIndex;

extern s32 BattleAllocHeapBlock(s32, s32);
extern void BattleSetArchive20Entry3(void);
extern void BattleReleaseEffectSlotIfChar4(u8);
extern void BattleWaitForArchiveSync(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);
extern u8 *g_BattlePulseTrianglePrims;
extern u8 g_BattleCommandCharIndex;
extern u8 func_80084854(u8 arg0, u8 arg1);
extern void func_80093B08(u8 arg0);
extern u16 BattleSlotBitMask(u8 arg0);
extern void BattleBeginAnimTask(u16 arg0);
extern void BattleApplySlotMaskToEffects(u16 arg0);
extern void BattleAllocCharSlot(u8 arg0);
extern void BattleResetSlotEffectWords(u8 arg0);
extern void BattleFinishActorAnimationAndReleaseSlot(u8 arg0);
extern void BattleReleaseCharSlot(u8 arg0);
extern u8 g_BattleSlotConditions[];
extern u8 g_BattleSlotReleasedFlags[];
extern u8 g_BattleActorCharIds[];
extern u8 g_FieldPlayerWalkmeshMaterial;
extern u8 g_BattleCommandSlotMasks[];
extern u8 g_BattleCommandRecords[];
extern void BattleClearFlagC6(void);
extern void BattleClearSlotFlagsIfChar4(u8 arg0, u8 arg1);
extern void BattleAppendCommandRecord(u8 arg0);
extern u8 D_800D2C0C[];
extern void func_8009A2D4(u8 arg0);
extern void BattleBuildSpritePrimLists(u8 arg0);
extern void func_800898F0(u8 arg0);
extern void BattleUpdateFrame(void);
extern u8 BattlePairTargetable(u8 arg0, u8 arg1);
extern void BattleClearSlotFlag15A1(u8 arg0);
extern void BattleRollActorActionCondition(u8 arg0, u8 *pInfo, u8 *pField);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);
extern void BattleInitSlotRowsForActor(s32 arg0, s32 arg1);
extern s32 func_800877E0(s32 arg0, s32 arg1);
extern void BattleInitActorSlotAnimation(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 BattleAdvanceCommandStep(u8 arg0, u8 arg1);
extern void BattlePlaySoundWithBaseId(u8 arg0);
extern void BattleActorInitSpriteFromSlot(void);
extern void BattleDrawCommandRowSprites(void);
extern void BattleDrawCommandSlotRows(u8 arg0, u8 arg1);
extern s32 BattleCheckCommandKeyStatusBit(u8 arg0, u8 arg1);
extern u16 g_BattleSlotCountdownTimers[];
extern u8 g_BattleGaugeCountdownTable[][8];
extern void BattleQueueOneActorCommand(u8 arg0);
extern void BattleSetSlotFlag15A1(u8 arg0);
extern u8 g_BattleSlotDisplayInitialized;
extern u8 g_BattleTargetSlot;
extern u8 D_800C48EA;
extern u8 D_800CCD38[];
extern u8 g_BattleCharacterRowValues[];
extern u8 g_BattleMenuEvent;
extern u8 g_BattleLastMenuEvent;
extern void BattleLoadStringTableAE(u8 arg0);
extern u8 BattleRunFourSlotCommandMenu(u8 arg0);
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern void BattleEnsureTargetSlotInCandidateList(u8 arg0);
extern void BattleInitPulseTrianglePrims(void);
extern void BattleMarkSlotReleased(u8 arg0); 
extern u8 func_8008BED8(u8 arg0);
extern u8 BattleRunQueuedSlotCommands(u8 arg0);
extern void BattleActivateSelectedCommand(u8 arg0);

typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u8 statDigits[3][4];
    // One word per command: the character's command bit for it, so nonzero means
    // the slot may use it. The menu handlers test one word per menu event.
    /* 0x1C */ u16 commandFlags[16];
    /* 0x3C */ u8 targetSlot; // the slot this one targets, 0xFF for none
    /* 0x3D */ u8 unk3D[0x3];
} BattleActorSlot; // Size: 0x40

// 0x2F8-byte block of actor slots plus the slot menu state that follows them.
typedef struct {
    /* 0x000 */ BattleActorSlot slot[0xB];
    /* 0x2C0 */ u8 unk2C0[0x4];
    /* 0x2C4 */ u8 queuedActionIds[0x7]; // 0xFF for no entry; BattleRunQueuedSlotCommands passes each to BattleBeginActorAction
    /* 0x2CB */ u8 unk2CB;
    /* 0x2CC */ u8 commandKey[7];
    /* 0x2D3 */ u8 actorIndex; // the slot the queued command is being built for
    /* 0x2D4 */ u8 gaugeValue; // the reading the menu spends, the second index of the g_BattleGaugeCountdownTable[max * 8 + value] pair
    /* 0x2D5 */ u8 gaugeMax; // its value at menu entry, the first index of that pair and the row func_8007FBE0 draws
    /* 0x2D6 */ u8 commandKeyLength; // commandKey bytes filled, 0 for no command
    /* 0x2D7 */ u8 unk2D7[0x3];
    /* 0x2DA */ u8 count; // queued commands, the row the command tables are indexed by
    /* 0x2DB */ u8 commandFinished; // 0 while a queued command runs, 1 once it has; BattleWaitWhileStateIs8 and func_80079778 spin on it
    /* 0x2DC */ u8 unk2DC;
    /* 0x2DD */ u8 menuState; // the state each menu event handler switches to
    /* 0x2DE */ u8 eventHandled; // handlers set it to release the update wait loop
    /* 0x2DF */ u8 gaugeCost; // gauge points the confirmed command spends; BattleAdvanceCommandStep also uses it as the step index into the action-id table
    /* 0x2E0 */ u8 colorShift; // subtracted from each colour byte of the slot's prims after << 4
    /* 0x2E1 */ u8 commandQueued; // set when a handler queued its command, so its next call returns at once
    /* 0x2E2 */ u8 lastMenuState; // the menuState the current frame was built for
    /* 0x2E3 */ u8 gaugeSpent; // set when a confirmed command spent gauge; func_80080160 then folds the leftover reading into the slot display value
    /* 0x2E4 */ u8 unk2E4;
    /* 0x2E5 */ u8 unk2E5;
    /* 0x2E6 */ u8 commandId; // the picked command, indexes the 0x10-byte command entry rows
    /* 0x2E7 */ u8 unk2E7;
    /* 0x2E8 */ u8 targetSlot; // the slot being selected, the menu's copy of slot[].targetSlot
    /* 0x2E9 */ u8 commandAppended; // set once BattleAppendCommandRecord has run; both append paths return at once
    /* 0x2EA */ u8 menuRunning; // 1 while a slot menu run is in progress; cleared together with eventHandled
    /* 0x2EB */ u8 charRedrawFlags[0xB];
    /* 0x2F6 */ u8 repeatPending; // the last event was this one, so take it as a repeat
    /* 0x2F7 */ u8 unk2F7;
} BattleActorSlots; // Size: 0x2F8
// The slot-menu handlers from BattleAdvanceSlotMenuStateEventDisplay on (rodata 0x520-0x600). cc1 emits
// `.align 3` before every jump table, so their tables have to come from a TU
// whose .rodata starts 8-aligned (abs 0x80070010); main/misc11 keeps
// func_80080160, whose 101-word table is the 4-mod-8 first table of the old range.
s32 BattleAdvanceSlotMenuStateEventCommand(u32 arg0) {
    u8 who;
    u8 slot;
    s32 remain;
    g_BattleSoundEnabled = 0;
    if (((BattleActorSlots *)g_BattleCommandQueue)->commandQueued != 0) {
        return;
    }
    slot = arg0;
    ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost = 0;
    BattleInitTargetSlotFlags(slot);
    switch (g_BattleMenuEvent) {
    case 5:
        if (((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue
            != ((BattleActorSlots *)g_BattleCommandQueue)->gaugeMax) {
            BattleQueueCommandFE(slot);
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
            break;
        }
        BattleActorInitSpriteFromSlot();
        g_BattleSoundEnabled = 1;
        g_BattleDisplayState[0x7B] = 0;
        g_BattleDisplayState[0xAF] = 0;
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
        BattleClearFlagC6();
        BattleClearSlotFlagsIfChar4(slot, 1);
        break;
    case 4:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[slot].commandFlags[2] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(BATTLE_SOUND_MENU_DENIED);
        }
        break;
    case 7:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[slot].commandFlags[1] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(BATTLE_SOUND_MENU_DENIED);
        }
        break;
    case 6:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[slot].commandFlags[0] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(BATTLE_SOUND_MENU_DENIED);
        }
    }
    if (((BattleActorSlots *)g_BattleCommandQueue)->commandAppended == 0) {
        switch (g_BattleMenuEvent) {
        case 0:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot =
                func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 0);
            BattleInitSlotRowsForActor(((BattleActorSlots *)g_BattleCommandQueue)->actorIndex,
                                      ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        case 1:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot =
                func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 1);
            BattleInitSlotRowsForActor(((BattleActorSlots *)g_BattleCommandQueue)->actorIndex,
                                      ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        case 2:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot =
                func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 2);
            BattleInitSlotRowsForActor(((BattleActorSlots *)g_BattleCommandQueue)->actorIndex,
                                      ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        case 3:
            ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot =
                func_80084854(((BattleActorSlots *)g_BattleCommandQueue)->targetSlot, 3);
            BattleInitSlotRowsForActor(((BattleActorSlots *)g_BattleCommandQueue)->actorIndex,
                                      ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot);
            BattlePlaySoundWithBaseId(0x4C);
            break;
        }
        func_800877E0((u8)arg0, ((BattleActorSlots *)g_BattleCommandQueue)->targetSlot);
    }
    g_BattleDisplayState[0xAF] = 1;
    switch (g_BattleMenuEvent) {
    case 5:
        break;
    case 4:
        if ((u8)BattleCheckCommandKeyStatusBit(4, arg0) != 0) {
            remain = ((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue - 3;
            if (remain >= 0) {
                ((BattleActorSlots *)g_BattleCommandQueue)->unk2E4++;
            }
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost++;
    case 7:
        ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost++;
    case 6:
        ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost++;
        if (((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue
                - ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost >= 0) {
            BattlePlaySoundWithBaseId(0x4D);
            BattleAppendCommandRecord(arg0);
            if (g_BattleDisplayState[0xCB] != 0) {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0x64;
                ((BattleActorSlots *)g_BattleCommandQueue)->lastMenuState = 0xFF;
                ((BattleActorSlots *)g_BattleCommandQueue)->colorShift = 1;
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 5;
                ((BattleActorSlots *)g_BattleCommandQueue)->lastMenuState = 5;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue =
                ((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue
                - ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost;
            BattleDrawCommandRowSprites();
            who = arg0;
            BattleDrawCommandSlotRows(g_BattleMenuEvent, who);
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E5 =
                BattleAdvanceCommandStep(who, ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost);
            g_BattleSlotCountdownTimers[who] =
                (g_BattleGaugeCountdownTable[((BattleActorSlots *)g_BattleCommandQueue)->gaugeMax]
                            [((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue] * 100) / 56;
            ((BattleActorSlots *)g_BattleCommandQueue)->gaugeSpent = 1;
        } else {
            BattlePlaySoundWithBaseId(BATTLE_SOUND_MENU_DENIED);
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue == 0) {
            BattleQueueCommandFE((u8)arg0);
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->unk2E5 != 0) {
            BattleQueueCommandFE((u8)arg0);
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
        }
        break;
    }
}

