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
extern void func_80088490(u8 arg0);
extern void BattleResetSlotEffectWords(u8 arg0);
extern void BattleFinishActorAnimationAndReleaseSlot(u8 arg0);
extern void BattleReleaseCharSlot(u8 arg0);
extern u8 g_BattleSlotConditions[];
extern u8 D_800C3EB8[];
extern u8 g_BattleActorCharIds[];
extern u8 g_FieldPlayerWalkmeshMaterial;
extern u8 g_BattleCommandSlotMasks[];
extern u8 g_BattleCommandRecords[];
extern void BattleClearFlagC6(void);
extern void BattleClearSlotFlagsIfChar4(u8 arg0, u8 arg1);
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
extern void func_800861D0(u8 arg0, u8 arg1);
extern s32 BattleCheckCommandKeyStatusBit(u8 arg0, u8 arg1);
extern u16 g_BattleSlotCountdownTimers[];
extern u8 g_BattleGaugeCountdownTable[][8];
extern void BattleQueueOneActorCommand(u8 arg0);
extern void BattleSetSlotFlag15A1(u8 arg0);
extern u8 g_BattleSlotDisplayInitialized;
extern u8 g_BattleTargetSlot;
extern u8 D_800C48EA;
extern u8 D_800CCD38[];
extern u8 D_800CDD53[];
extern u8 g_BattleMenuEvent;
extern u8 g_BattleLastMenuEvent;
extern void BattleLoadStringTableAE(u8 arg0);
extern u8 BattleRunFourSlotCommandMenu(u8 arg0);
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern void BattleEnsureTargetSlotInCandidateList(u8 arg0);
extern void BattleInitPulseTrianglePrims(void);
extern void func_800826CC(u8 arg0); 
extern u8 func_8008BED8(u8 arg0);
extern u8 func_8008C4A8(u8 arg0);
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
    /* 0x2C4 */ u8 queuedActionIds[0x7]; // 0xFF for no entry; func_8008C4A8 passes each to BattleBeginActorAction
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
void BattleAdvanceSlotMenuStateEventDisplay(u8 arg0) {
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
        g_BattleSoundEnabled = 0;
        BattleSetupActorSlotDisplay(a);
        BattleEnsureTargetSlotInCandidateList(a);
        BattleInitPulseTrianglePrims();
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 5;
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[11] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 2;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 3;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[10] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0
            && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[7] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[5] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}

void BattleAdvanceSlotMenuStateEventActivate(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if (func_8008BED8(a) != 0) {
            BattleActivateSelectedCommand(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 2;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[5] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 3;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[10] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[7] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
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
    }
}

void BattleAdvanceSlotMenuStateEventExit(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleSetSlotFlag15A1(a);
        ((BattleActorSlots *)g_BattleCommandQueue)->menuRunning = 0;
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[5] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[11] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 2;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[10] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[7] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[8] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 9;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}

void BattleAdvanceSlotMenuStateEventFourSlot(u8 arg0) {
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
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[5] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[11] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 2;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 3;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[7] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}

void BattleInitTargetSlotFlags(s32 arg0) {
    s32 i;

    if (g_BattleCommandQueue[0x2E9] != 0) {
        return;
    }
    BattleBeginAnimTask(BattleSlotBitMask(g_BattleCommandQueue[0x2E8]));
    BattleApplySlotMaskToEffects(BattleSlotBitMask(g_BattleCommandQueue[0x2E8]));
    i = 0;
    do {
        if (func_80084854(g_BattleCommandQueue[0x2E8], i) != g_BattleCommandQueue[0x2E8]) {
            (g_BattlePulseTrianglePrims + i)[0xE6] = 1;
        } else {
            (g_BattlePulseTrianglePrims + i)[0xE6] = 0;
        }
        i++;
    } while (i < 4);
    if (g_BattleCommandCharIndex != 4) {
        func_80093B08(arg0);
    }
}
void BattleAppendCommandRecord(u8 arg0) {
    u16 orBits;
    s32 idx;

    if (g_BattleCommandQueue[0x2E9] != 0) {
        return;
    }
    g_BattleCommandQueue[0x2E9] = 1;
    BattleApplySlotMaskToEffects(0);
    idx = arg0 * 0x40;
    *(g_BattleCommandQueue + idx + 0x3C) = g_BattleCommandQueue[0x2E8];
    BattleFreeStringTableAE();
    BattleFreeStringTable96();
    BattleClearFlagC6();
    orBits = BattleSlotBitMask(arg0) | BattleSlotBitMask(*(g_BattleCommandQueue + idx + 0x3C));
    BattleBeginAnimTask(orBits);
    if (g_BattleActorCharIds[arg0] != 4) {
        u8 *pTarget;
        u8 *pCmd;
        u16 mask;
        g_BattleCommandCodes[g_BattleCommandQueue[0x2DA] * 0x48] = arg0;
        g_BattleCommandSlotIds[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFD;
        pTarget = g_BattleCommandQueue + idx;
        *(u16 *)(g_BattleCommandSlotMasks + g_BattleCommandQueue[0x2DA] * 0x48) = 0;
        mask = BattleSlotBitMask(pTarget[0x3C]);
        pCmd = g_BattleCommandQueue;
        *(u16 *)(g_BattleCommandRecords + pCmd[0x2DA] * 0x48) = mask;
        pCmd[0x2DA]++;
    }
    BattleClearSlotFlagsIfChar4(arg0, 0);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11_b", func_80081B58);
// 99.32% under this unit's preset, but 100.00 UNDER gcc-2.6.0 - this function needs the
// XenoLibrary compiler and the unit needs a SPLIT (see the end of this note).
// Load-bearing, each measured:
//  (a) THE TWO TRAILING IFs BELONG INSIDE case 6, not after the switch. Retail's case-5
//      `beq` and the dispatch default jump to the EPILOGUE (0x534), i.e. they skip both
//      `if (gaugeValue == 0)` and `if (unk2E5 != 0)`; leaving those two after the switch
//      makes every path run them and every branch target is off: 98.51% at 1352 bytes.
//  (b) THE RETURN TYPE IS s32, NOT void (the value is never used). `void` lets reorg steal
//      the dispatch's `slti v0,v1,6` into the case-5 `beq` delay slot where retail has a
//      nop: 99.03% and 4 bytes short. This is levers.md's c-stolen-delay-slot-retail
//      entry; do not "tidy" it back to void.
//  (c) Still load-bearing from earlier rounds: the last switch's source case order is
//      5, 4, 7, 6 with an empty `case 5: break;` (that is what makes retail's li v0,5 /
//      beq / slti 6 four-node tree); the masked slot is TWO locals, `slot` from the entry
//      and `who` masked again just before func_800861D0 and used as the timer index (pass
//      arg0 to BattleAdvanceCommandStep and it re-masks from $s1); the gauge scale is /56 (magic
//      0x92492493 with addu + sra 5, the table's full-scale row byte 0x38); case 5's four
//      stores keep menuState last (it lands in the BattleClearFlagC6 jal delay slot);
//      g_BattleGaugeCountdownTable is read with its 2D subscript (the flat
//      [max * 8 + value] form folds into maspsx's lui at / addu at,at,idx / lbu);
//      func_800877E0 is INSIDE `if (commandAppended == 0)` because retail's bnez 0x2E9
//      targets the block after that call while the switch's default targets the call;
//      and that call's first argument is cast, func_800877E0((u8)arg0, ...), because its
//      parameter is u8, so the call site masks (andi a0,s1,0xff).
// THE RESIDUAL IS A PRESET, NOT A SPELLING. This body compiles to retail's exact 1356
// bytes under gcc 2.6.0, every instruction equal:
//   tools/scripts/try_flags.sh battle/main/misc11_b func_80081B58 --gcc gcc-2.6.0-psx
//   -> gcc-2.6.0 -O2 -G8 -mcpu=3000   100.0
// and to 99.32 / 1356 under the unit's 2.7.2, where the only difference is the gaugeSpent
// constant's live range: retail is sra v1,v1,5 / li v0,1 / lui at,%hi(timer) / addu
// at,at,s0 / sh v1 / j / sb v0,0x2E3(a1), ours is sra v0 / lui at / addu at / sh v0 /
// li v0,1 / j / sb v0. cc1 2.7.2's own dumps say why: pre-reload the constant is a real
// pseudo (lreg insn 753 `(set (reg:QI 283) (const_int 1))`) but it is placed AFTER the
// store insn 748 and greg then hands 283 and the quotient pseudo the same $v0 because
// their live ranges do not overlap; retail's 283 is live across 748, which needs a
// statement between the division and the timer store whose store comes after it - and
// cse1 folds every such spelling back in: a `u8 spent` local assigned between them is
// (i) below, and `q->gaugeSpent = 1;` before the store is 97.32 / 1368 (the sb leaves the
// j delay slot and the queue pointer is reloaded). A 4-minute permuter run in an earlier
// round and a fresh 3-minute one both scored base=90 best=90 (1426 iterations).
// WHAT TO DO: do NOT switch the unit. try_flags --all --gcc gcc-2.6.0-psx scores
// BattleInitTargetSlotFlags 100.00 but BattleAppendCommandRecord 99.91 (it is 100.00
// under 2.7.2), so the unit is mixed; give this function its own TU (one c range in
// config/battle.yaml, its own INCLUDE_ASM path, a per-file gears.toml key on gcc-2.6.0)
// and un-park this body there. The eight slot-menu functions stay at 99.90-99.93 under
// both compilers (jtbl operand only), so they do not constrain the split.
/*
s32 func_80081B58(u32 arg0) {
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
            BattlePlaySoundWithBaseId(0x4F);
        }
        break;
    case 7:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[slot].commandFlags[1] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(0x4F);
        }
        break;
    case 6:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[slot].commandFlags[0] != 0) {
            g_BattleMenuEvent = 5;
            BattlePlaySoundWithBaseId(0x4F);
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
            func_800861D0(g_BattleMenuEvent, who);
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E5 =
                BattleAdvanceCommandStep(who, ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost);
            g_BattleSlotCountdownTimers[who] =
                (g_BattleGaugeCountdownTable[((BattleActorSlots *)g_BattleCommandQueue)->gaugeMax]
                            [((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue] * 100) / 56;
            ((BattleActorSlots *)g_BattleCommandQueue)->gaugeSpent = 1;
        } else {
            BattlePlaySoundWithBaseId(0x4F);
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
*/
void BattleAdvanceSlotMenuStateEventQueue(u8 arg0) {
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
        if (func_8008C4A8(a) != 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
        ((BattleActorSlots *)g_BattleCommandQueue)->lastMenuState = 0xFF;
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0
            && g_BattleLastMenuEvent == 2) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 3;
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[7] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0
            && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[10] != 0) {
                BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            break;
        }
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
        break;
    }
}

/* 99.93% JTBL: every instruction is retail's (576 bytes, retail's 0x240), the
   only difference is the switch's table operand (retail names jtbl_800700B0,
   ours .rodata) and the local label names. It now links: the split into this TU
   puts the table at .rodata+0x214 of a range that starts 8-aligned, where cc1's
   `.align 3` is a no-op. Ported from the strict-1.000 twin
   BattleAdvanceSlotMenuStateSubMode (main/misc12.c:511); only the per-arm
   constants differ. */
void BattleAdvanceSlotMenuStateEventSubMode(u8 arg0) {
    u8 a;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        if (func_8008BED8(a) != 0) {
            BattleActivateSelectedCommand(a);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 8;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[5] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(0x4F);
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
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[7] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[10] != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[11] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 2;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}

/* 99.91% JTBL: every instruction is retail's at retail's 0x1C8 bytes, the diff
   is the jtbl operand and label names only. Links via the same split as
   BattleAdvanceSlotMenuStateEventSubMode. Ported from the strict-1.000 twin
   BattleAdvanceSlotMenuStateRandom (main/misc12.c:583); only the per-arm
   constants differ. */
void BattleAdvanceSlotMenuStateEventRandom(u8 arg0) {
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
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[5] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[9] != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[11] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 8;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[7] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[10] != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
            break;
        }
        ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 3;
        break;
    }
}

/* 96.00%, and the instruction stream is identical to retail's EXCEPT one block
   position (see the end). Size is retail's 340 bytes. Four levers found, all of
   them load-bearing:
   (a) THE g_BattleSlotConditions DOUBLE-$at. Retail re-forms the base for each of
       the two references (`lui at,%hi; addu at,at,v0; lbu/sb %lo(at)`); with one
       `|=` our gcc cse's the address into a register (`la v1,sym; addu v0,v0,v1`)
       and is 8 bytes short. Two references only get separate reloads when their
       RTL differs, and the RMW gives that only if the index is a NAMED LOCAL and
       the two spellings put the symbol on opposite sides: `*(u8*)(off + (u32)sym)`
       vs `*(u8*)((u32)sym + off)`. Inline index expressions canonicalise and
       merge; array-vs-cast forms too; volatile and pointer locals do not help.
       This also explains misc19.c's 94.23% park of the same access.
   (b) `i = 0;` must be hoisted before the `if (g_BattleActorCharIds[arg0] != 7)`
       and the loop written `for (; i < 3; i++)`: that is what puts retail's
       `move a1,zero` in the ActorCharIds `beq` delay slot.
   (c) the graphics-work store wants a pointer local (`pWork = g_BattleGraphicsWork
       + arg0 * 0x1E4; pWork[0x853D] = 2;`) for retail's base-first
       `addu v1,v1,v0`; the plain cast sum gives index-first.
   (d) `*(g_BattleCommandQueue + arg0 + 0x2EB) = 1;` (not `[arg0 + 0x2EB]`) for
       retail's `addu v0,v0,a0` instead of `addu v0,a0,v0`.
   REMAINING: retail emits the `g_BattleSlotFlags[arg0 * 8] = 2` block BEFORE the
   g_BattleActorCharIds load; ours sinks it after the lbu, which flips the tail's
   registers and the beq target offsets. Forcing the order with `do { } while (0)`
   around or after the store, or a volatile store, gives the right block position
   but then `li a0,0x2` displaces arg0 from $s0 and an extra `move a0,s0` appears
   (344 bytes, 98.35%). A 4-minute permuter run (base 505 -> 149) proposed reusing
   the loop local for the index (`i = arg0 * 8; g_BattleSlotFlags[i] = 2;`), which
   is the same 344-byte/no-link shape. Re-measured this round, all at 340 bytes
   except where noted: `*(g_BattleSlotFlags + arg0 * 8) = 2;` (cast form) 96.00,
   a shared `u8 state = 2` local for both `2` stores 96.00, a pointer local for
   the flags index 96.00, `i = 0;` before the store 96.00, the load hoisted into
   a `charId` local 91.65 (344), the store made volatile 91.47 (344), the index
   taken through an `s64` local 95.41 (352), and `do { } while (0)` 98.35 (344).
   A second 3-minute permuter run from this body went 470 -> 394 and its only
   find was the `s64` index local, i.e. nothing portable. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11_b", func_800826CC);
/*
void func_800826CC(u8 arg0) {
    s32 i;
    s32 cond;
    s32 off;
    u8 *pWork;

    func_80088490(arg0);
    BattleResetSlotEffectWords(arg0);
    BattleFinishActorAnimationAndReleaseSlot(arg0);
    off = arg0 * 0x170;
    cond = *(u8 *)(off + (u32)g_BattleSlotConditions);
    *(u8 *)((u32)g_BattleSlotConditions + off) = cond | 0x80;
    BattleReleaseCharSlot(arg0);
    g_BattleSlotFlags[arg0 * 8] = 2;
    i = 0;
    if (g_BattleActorCharIds[arg0] != 7) {
        pWork = g_BattleGraphicsWork + arg0 * 0x1E4;
        *(u8 *)(pWork + 0x853D) = 2;
    }
    D_800C3EB8[arg0 * 0x1C] = 1;
    *(g_BattleCommandQueue + arg0 + 0x2EB) = 1;
    for (; i < 3; i++) {
        if (g_GameState.partyMembers[i] == g_BattleActorCharIds[arg0] && g_FieldPlayerWalkmeshMaterial == 0) {
            g_GameState.gearRide[i] = 1;
        }
    }
}
*/
