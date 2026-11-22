#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

// Played when the pressed menu event has no command bound to it.
#define BATTLE_SOUND_MENU_DENIED 0x4F

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
extern s32 func_80087AF0(u8 arg0, u8 arg1);
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
    /* 0x2DF */ u8 gaugeCost; // gauge points the confirmed command spends; func_80087AF0 also uses it as the step index into the action-id table
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
// The slot-menu handlers from func_8008115C on (rodata 0x520-0x600). cc1 emits
// `.align 3` before every jump table, so their tables have to come from a TU
// whose .rodata starts 8-aligned (abs 0x80070010); main/misc11 keeps
// func_80080160, whose 101-word table is the 4-mod-8 first table of the old range.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11_b", func_8008115C);
/* 88.35%: every case body is instruction-identical to retail except two
   block-placement differences. (a) gcc cross-jumps all four plain
   `g_BattleCommandQueue[0x2DD] = N;` case tails into one shared store block at the end of
   the function; retail keeps each store inline and only shares the
   BattlePlaySoundIfEnabled call. (b) that shared call block: retail is
   `jal; nop` with `li a0,0x4F` sitting in each predecessor's branch delay slot,
   ours is `jal; li a0,0x4F` (reorg always steals the constant into the jal slot
   when it is the last insn before the call). Same two shapes, unresolved, in
   func_800816F8 (99.90%, JTBL-only there). Tried: literal vs. local argument,
   both if/else orders, p/off locals vs. inline g_BattleCommandQueue indexing. */
/*
void func_8008115C(u8 arg0) {
    u8 a;
    u8 *p;
    s32 off;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (p[off + 0x3C] == 0xFF) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            g_BattleSoundEnabled = 0;
            BattleSetupActorSlotDisplay(a);
            BattleEnsureTargetSlotInCandidateList(a);
            BattleInitPulseTrianglePrims();
            g_BattleCommandQueue[0x2DD] = 5;
        }
        break;
    case 1:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x32) != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            g_BattleCommandQueue[0x2DD] = 2;
        }
        break;
    case 2:
        g_BattleCommandQueue[0x2DD] = 3;
        break;
    case 3:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x30) == 0) {
            g_BattleCommandQueue[0x2DD] = 4;
            break;
        }
        if (p[0x2F6] != 0 && g_BattleLastMenuEvent == 3) {
            if (*(u16 *)(p + off + 0x2A) != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                g_BattleCommandQueue[0x2DD] = 0xA;
            }
            g_BattleCommandQueue[0x2F6] = 0;
        } else {
            g_BattleCommandQueue[0x2F6] = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 0:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x26) != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            g_BattleCommandQueue[0x2DD] = 7;
        }
        break;
    }
   Re-measured round 389 with the typed-struct spelling the main/misc12 twins
   use ("((BattleActorSlots *)g_BattleCommandQueue)->menuState = N;" and
   "->slot[a].commandFlags[N]"): 87.59% with the store arms written as early
   exits, 85.11% with plain if/else, against this body's 88.35 - the raw
   g_BattleCommandQueue[0x2DD] spelling is the best measured, keep it.
   NOT a preset: gcc-2.6.0 gives the identical 87.59 (and the unit cannot
   switch anyway: BattleSetActorFlag34 is 97.00 under 2.6.0).
   JTBL_LAYOUT no longer applies: the split into main/misc11_b puts this table
   at .rodata+0x0 of a range that starts 8-aligned (see the header comment), so
   only the two block-placement differences above remain. Re-measured after the
   split 88.35% at 440 bytes against retail's 444; the typed-struct twin port the
   main/misc12 neighbours use measures 87.59, so this raw spelling stays. */

void func_80081318(u8 arg0) {
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

void func_80081504(u8 arg0) {
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

void func_800816F8(u8 arg0) {
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

/* 98.51%, 1356 bytes (retail's exact size); the 9 remaining differing instructions
   are two delay-slot fills and one register choice, so no size difference (see
   REMAINING). Levers that are load-bearing:
   (a) THE CASE ORDER IS THE TREE. Switch 1 lists 5, 4, 7, 6 (case 4 =
       commandFlags[2], case 7 = [1], case 6 = [0]: retail's lhu 0x20/0x1E/0x1C), and
       the third switch lists 5 (empty body), 4, 7, 6: that empty "case 5: break;" is
       what makes cc1 emit retail's li v0,5 / beq / slti 6 four-node tree, not a chain.
   (b) THE MASKED SLOT IS TWO LOCALS, not one: `slot` (entry, the switch arms, the
       slot[] index) and `who` (masked again just before func_800861D0, then passed to
       func_80087AF0 and used as the timer index). Passing arg0 to func_80087AF0
       instead of `who` re-masks from $s1 where retail moves $s0.
   (c) THE GAUGE SCALE IS /56, not /100: retail's magic is 0x92492493 with addu + sra 5,
       i.e. (value * 100) / 56; the table's full-scale row byte is 0x38.
   (d) case 5's stores are FOUR separate statements with menuState stored LAST (it
       lands in the BattleClearFlagC6 jal delay slot).
   (e) g_BattleGaugeCountdownTable is declared [][8] and read with the 2D subscript; the flat
       [max * 8 + value] form, a mem_addr base and a u8 *pCost local all fold into
       maspsx's lui at / addu at,at,idx / lbu %lo(at), and the 2D form also decides
       the tail's registers (queue in $a1, /56 magic in $a2).
   (f) func_800877E0 IS INSIDE `if (commandAppended == 0)`, after the switch. Retail's
       bnez 0x2E9 targets the block AFTER that call (0x80081DB8) while the switch's
       default jumps to the call (0x80081DA0), i.e. two different targets; with the
       call outside the if both branches share one target and the bnez offsets differ
       (98.23 -> 98.33).
   (g) THE CALL ARGUMENT IS A CAST, `func_800877E0((u8)arg0, ...)`. func_800877E0's
       first parameter is u8 (misc4.c and misc15.c both declare it that way), so the
       call site masks; the bare `arg0` gives `move a0,s1` where retail has
       `andi a0,s1,0xff` (98.33 -> 98.51). Round 341 measured the same cast as a
       regression to 97.26, but that was on the body WITHOUT (f): the extra andi it
       reported was the bnez-target damage, not the cast.
   CLOSED THIS ROUND (341): case 4's arm ends with its OWN gaugeCost++, so the source has
   three separate read-modify-writes of 0x2DF (case 4, case 7, case 6) - that is
   retail's third copy at 0x80081E60, and adding it took the honest body 1328 -> 1356,
   retail's exact size. With it round 340's two permuter duplicate-arm blocks (case 6's
   "if (arg0) break; else break;" and the case-4 increment wrapped in
   "if (g_BattleCommandQueue) ... else ...") are BOTH unnecessary and measure
   identically without them. The old 96.22% note was partly a dead arm: gcc folded its
   re-read of the global to a null base (lbu v0,740(zero) / sb v0,740(zero)).
   REMAINING (three clusters, each compensating so the size stays 1356):
   (i)  the third switch's case-5 `beq` has a nop in its delay slot in retail where we
        put `slti v0,v1,6`, and retail's `li a0,4` sits in the case-4 beq's slot where
        we keep it at the head of the arm plus a nop in the slot. Pure reorg choices
        on an otherwise identical tree; a nop-return-type / statement reorder test is
        the only lever left and it rewrites the whole function.
   (ii) the gauge quotient lands in $v0 with mfhi $a3 where retail has $v1, because
        retail materialises li v0,1 (the gaugeSpent store's constant) BETWEEN the
        division's `sra` and the timers `sh`, and that register is then taken. A
        `remain = 1;` statement before the timer store does hoist the constant into
        the middle of the sequence but lands it before `mfhi` in $a0 (98.47) and also
        moves case 4's `addiu` to $a0; moving gaugeSpent above the timers store and a
        local for the quotient were measured in round 341 at 1368 / 97.04. A
        do-while(0) around the two stores instead hoists `sll s0,s0,0x1` and
        `lui a2,0x9249` out of the table sequence (1360, 97.27).
   (iii) the g_BattleDisplayState[0xAF] = 1 store takes the base in $v1 and the 1 in
        $v0 where retail has them the other way round.
   The preset is right: a 4-minute permuter run on THIS body found nothing
   (base 213 = best 213, 1572 iterations); round 341's run on the 98.23 body was
   444 = 444. cdk is 95.31, -G0 is the same, and 2.6.0 does not compile the unit. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11_b", func_80081B58);
/*
void func_80081B58(u32 arg0) {
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
                func_80087AF0(who, ((BattleActorSlots *)g_BattleCommandQueue)->gaugeCost);
            g_BattleSlotCountdownTimers[who] =
                (g_BattleGaugeCountdownTable[((BattleActorSlots *)g_BattleCommandQueue)->gaugeMax]
                            [((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue] * 100) / 56;
            ((BattleActorSlots *)g_BattleCommandQueue)->gaugeSpent = 1;
        } else {
            BattlePlaySoundWithBaseId(0x4F);
        }
        break;
    }
    if (((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue == 0) {
        BattleQueueCommandFE((u8)arg0);
        ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
    }
    if (((BattleActorSlots *)g_BattleCommandQueue)->unk2E5 != 0) {
        BattleQueueCommandFE((u8)arg0);
        ((BattleActorSlots *)g_BattleCommandQueue)->commandQueued = 1;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11_b", func_800820A4);
/* 96.88% (552 bytes, retail 544) - parked on an 8-byte code difference, NOT on
   layout: the JTBL_LAYOUT paragraph this park used to carry is stale. The unit's
   .rodata range starts 8-aligned at 0x80070010, and cc1 emits each switch table
   at its function's position in the file, so converting an in-family stub in
   address order keeps retail's table offsets - func_80081318/80081504/800816F8
   converted this way and check_bin.sh battle prints OK. The real residue is the
   cross-jumped `menuState = 3; repeatPending = 0;` tail (case 2 reaches it with
   `j` + delay slot where retail branches to it conditionally) plus the queue
   base landing in $5 where retail uses v1/a0/a1, i.e. 8 extra bytes.
   Shape ported from the strict-0.807 twin BattleAdvanceSlotMenuStateSubMode
   (main/misc12.c:511); the per-arm constants are this function's own. Note for
   whoever converts it: retail masks func_8008C4A8's return to a byte
   (`andi v0,v0,0xff` before the test), so the caller-side prototype is
   `u8 func_8008C4A8(u8)` even though misc15.c's park declares it s32.
   Remaining code differences: case 2 reaches the cross-jumped
   `menuState = 3; repeatPending = 0;` tail with `j` + delay slot where retail
   branches conditionally straight to it (that pair is most of the 8 extra
   bytes), and the queue base lands in $5 in every case where retail uses v1
   (cases 1 and 0), a0 (case 2) and a1 (case 3's inner test).
   Tried and measured worse: the `||` guard-clause spelling of cases 2 and 3
   (`if (p[0x2F6] == 0 || g_BattleLastMenuEvent != 2) { ...; break; }`) 88.85% /
   496 bytes; routing the state stores through the local base pointer
   (`p[0x2DD]`, `p[0x2F6]`) 84.77% / 504 bytes. Also fixed along the way: case 2
   stores 3 to menuState, not 2 (retail's `li v0,3` sits in the beq's delay
   slot), and the s32 return type on func_8008C4A8 cost the `andi`. */
/*
void func_800820A4(u8 arg0) {
    u8 a;
    u8 *p;
    s32 off;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (p[off + 0x3C] == 0xFF) {
            BattlePlaySoundIfEnabled(0x4F);
            break;
        }
        if (func_8008C4A8(a) != 0) {
            g_BattleCommandQueue[0x2E1] = 1;
        } else {
            g_BattleCommandQueue[0x2DD] = 7;
            g_BattleCommandQueue[0x2E2] = 0xFF;
        }
        break;
    case 1:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x32) == 0) {
            p[0x2DD] = 8;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2C) == 0) {
            p[0x2DD] = 9;
            break;
        }
        if (p[0x2F6] != 0 && g_BattleLastMenuEvent == 2) {
            g_BattleCommandQueue[0x2DD] = 3;
            g_BattleCommandQueue[0x2F6] = 0;
        } else {
            g_BattleCommandQueue[0x2F6] = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 3:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2A) == 0) {
            p[0x2DD] = 0xA;
            break;
        }
        if (p[0x2F6] != 0 && g_BattleLastMenuEvent == 3) {
            if (*(u16 *)(p + off + 0x30) != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                g_BattleCommandQueue[0x2DD] = 4;
            }
            g_BattleCommandQueue[0x2F6] = 0;
        } else {
            g_BattleCommandQueue[0x2F6] = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 0:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2E) == 0) {
            p[0x2DD] = 1;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}
*/

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
