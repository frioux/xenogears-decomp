#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

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
extern u8 BattleRunFourSlotCommandMenu(u8 arg0);
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern void BattleEnsureTargetSlotInCandidateList(u8 arg0);
extern void BattleInitPulseTrianglePrims(void);
extern void BattleMarkSlotReleased(u8 arg0); 
extern u8 func_8008BED8(u8 arg0);
extern u8 func_8008C4A8(u8 arg0);
extern void BattleActivateSelectedCommand(u8 arg0);
extern u8 g_BattleTaskListBusy;
extern u8 g_BattleMenuEventHistory;
extern u8 g_BattleSlotDisplayValue[];
extern u8 g_BattleSlotEntries[];
extern u8 g_BattleSlotStatusRows[];
extern u16 g_BattleSlotDisplayValues[];
extern u16 D_800C3234[];
extern u8 D_800D32A4[];
extern u8 D_800C4928;
extern void BattleClearCommandSlots(void);
extern void BattleNoOpCallback(s32 arg0, s32 arg1);
extern s32 func_8008D598(u8 arg0, u8 arg1, u8 arg2);
extern s32 BattleAdvanceSlotMenuStateEventCommand(u8 arg0);
extern u32 func_80083948(u8 arg0);
extern void BattleAdvanceSlotMenuStateEventDisplay(u8 arg0);
extern void BattleAdvanceSlotMenuStateEventActivate(u8 arg0);
extern void BattleAdvanceSlotMenuStateEventExit(u8 arg0);
extern void BattleAdvanceSlotMenuStateEventFourSlot(u8 arg0);
extern void BattleAdvanceSlotMenuStateEventQueue(u8 arg0);
extern void BattleAdvanceSlotMenuStateEventSubMode(u8 arg0);
extern void BattleAdvanceSlotMenuStateEventRandom(u8 arg0);
extern void BattleAdvanceSlotMenuState(u8 arg0);
extern void BattleSetActorSlotDisplayMode(u8 arg0);
extern void BattleSetActorSlotMenuMode(u8 arg0);
extern void BattleAdvanceSlotMenuStateCounter(u8 arg0);
extern void BattleAdvanceSlotMenuStateAE(u8 arg0);
extern void BattleAdvanceSlotMenuStateSlotFlags(u8 arg0);
extern void BattleAdvanceSlotMenuStateSubMode(u8 arg0);
extern void BattleAdvanceSlotMenuStateRandom(u8 arg0);
extern void BattleAdvanceSlotMenuStateText(u8 arg0);

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

/* 99.39% (264 bytes, 66 insns - retail's exact size): the residual is ONE
   register pair swapped. Retail count=$t0, limit=$t1, color=$t2 with the
   in-loop 0x12 in the caller-saved $v1; this body gives color=$t0 (0xC and
   0x12 both through it), limit=$t1, count=$t2. cc1 -dg shows color is the FIRST
   allocno global_alloc processes and count the LAST (processing order
   "78 79 76"), i.e. colour is weighted as live inside the loop while count is
   referenced only in the entry block and after the loop - no C spelling moves
   count's live range. Measured dead: color as s16 (99.39, same swap), as
   u32/s32/u8 (97.88, +4 bytes), colour declared or assigned first (99.39 /
   96.44), *pSrc with pSrc++ (99.09, also moves the pSrc base into $a1), 0xC
   written inside the loop (96.82, li stays in-loop in $v1 and limit takes
   $t0), 0x12 as a literal or as 0xC+6 (94.39, GCSE hoists both constants into
   $t0), dropping new_var for a plain "if (arg0 > 1)" guard (different
   allocation, both colour values in $t0), and "u8 count = arg1;" as an
   initialised declaration instead of an assignment (99.39, identical bytes).
   Two 4-minute permuter runs (base 245 -> 50 -> 40) plus two 3-minute runs
   (PERMUTER_NONE, base 60 best 60 / 1368 iterations; and base 40 best 40 /
   1495 iterations, round 405) found nothing. Retail's 0xC in $t2 and 0x12 in
   $v1 are two different registers, so the original was not one variable; a
   plain 0x12 literal is hoisted by GCSE, so that spelling is out too. The two
   requirements - one hard register for the 0xC and a re-materialised 0x12 -
   are not reachable from one variable.
   The -dl dump of this body proves the swap is unreachable rather than
   untested: global_alloc processes allocnos by
   floor_log2(refs) * refs / live_length * mode_size, and this body gives
   count 4 refs / 49 insns / 1 byte = 0.16, limit 4 refs / 36 insns / 4 bytes =
   0.89, color 7 refs / 28 insns / 2 bytes = 1.00, so the order is color,
   limit, count and the three loop-live temporaries take $t0/$t1/$t2 in that
   order. Count's terms are pinned by the machine code: 4 RTL refs are exactly
   the move, the masked compare, the branch-set and the final store, its
   49-insn live range is entry to the last store, and it must be QI because
   retail's copy from arg1 is a bare "move" with no zero-extend (an s32 count
   would need an andi there, i.e. 67 insns). No C spelling moves it above
   limit's 4-byte 0.89. A third 3-minute permuter run (round 418, base 40 best
   40, 1474 iterations) found nothing, and tools/rounds/levers.md's margin rule
   settles it: a 6x quotient gap is a different function, not a missing
   spelling. One reading note kept for the next attempt: retail's 0xC is
   materialised in the PREHEADER and 0x12 re-materialised inside the loop, so
   0x910 gets 0xC on every iteration; "(color = 0x12)" is the only spelling
   whose shape (invariant held, 0x12 in-loop) matches, and it is the
   first-iteration/next-iteration reading of the same bytes - "color + 6" for
   the 0x914 store instead lets GCSE hoist both constants (94.39, still 66
   insns / 264 bytes, 26 differing lines). try_flags: gcc-2.6.0-psx 99.39,
   gcc-2.7.2-cdk-psx 85.14, so the preset is not the way out. */
// PARKED 99.39% (264 bytes, 66 insns - retail's exact size): count/color swapped.
/*
void func_8007FBE0(u8 arg0, u8 arg1) {
    u8 count;
    s32 new_var;
    u16 color;
    s32 limit;
    u8 *pBase;
    u8 *pSrc;
    s32 i;

    count = arg1;
    if (arg0 == arg1) {
        count = arg1 - 1;
    }
    new_var = 0;
    i = 0;
    if (arg0 - 1 > new_var) {
        pBase = g_BattleGraphicsWork;
        color = 0xC;
        do { limit = arg0 - 1; pSrc = D_800C3202; } while (0);
        pSrc += arg0 * 6;
        do {
            *(u16 *)(pBase + (i * 2 + g_BattlePrimStartIndex) * 0x10 + 0x910) = color;
            *(u16 *)(pBase + (i * 2 + g_BattlePrimStartIndex) * 0x10 + 0x912) = pSrc[i] + 0x5E;
            *(u16 *)(pBase + (i * 2 + g_BattlePrimStartIndex) * 0x10 + 0x914) = (color = 0x12);
            *(u16 *)(pBase + (i * 2 + g_BattlePrimStartIndex) * 0x10 + 0x916) = pSrc[i] + 0x5E;
            i++;
        } while (i < limit);
    }
    g_BattleDisplayState[0x97] = count;
    g_BattleDisplayState[0x98] = *(u8 *)&g_BattlePrimStartIndex;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FBE0);

void BattleFreeStringTableAE(void) {
    if (g_BattleDisplayState[0xAE] != 0) {
        HeapFree(g_BattleSlotStringTable);
        g_BattleDisplayState[0xAE] = 0;
    }
}

void BattleLoadStringTableAE(u8 arg0) {
    s32 type;

    if (g_BattleDisplayState[0xAE] != 0) {
        return;
    }
    BattleWaitForArchiveSync();
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        type = 1;
    } else {
        type = 2;
    }
    BattleSetArchive20Entry3();
    g_BattleSlotStringTable = (void *)BattleAllocHeapBlock(ArchiveDecodeAlignedSize(type), 0);
    ArchiveReadFileToBuffer(type, g_BattleSlotStringTable, 0, 0x80);
    BattleWaitForArchiveSync();
    g_BattleDisplayState[0xAE] = 1;
}

void BattleFreeStringTable96(void) {
    if (g_BattleDisplayState[0x96] != 0) {
        HeapFree(g_BattleEntry3StringTable);
        g_BattleDisplayState[0x96] = 0;
    }
}

void BattleLoadStringTable96(void) {
    s32 pBuffer;

    if (g_BattleDisplayState[0x96] == 0) {
        BattleWaitForArchiveSync();
        BattleSetArchive20Entry3();
        pBuffer = BattleAllocHeapBlock(ArchiveDecodeAlignedSize(3), 0);
        g_BattleEntry3StringTable = (u8 *)pBuffer;
        ArchiveReadFileToBuffer(3, (void *)pBuffer, 0, 0x80);
        BattleWaitForArchiveSync();
        g_BattleDisplayState[0x96] = 1;
    }
}

void BattleAllocDrawBuffer(void) {
    u8 *pBuffer;

    pBuffer = (u8 *)BattleAllocHeapBlock(0x5DA4, 0);
    g_BattleSpriteColumnWork = pBuffer;
    bzero(pBuffer, 0x5DA4);
    *(volatile u16 *)(g_BattleSpriteColumnWork + 0x5D9C) = 0xA0;
    *(volatile u16 *)(g_BattleSpriteColumnWork + 0x5D9E) = 0x64;
}


/* 91.69%: retail's exact 472 bytes, same calls, same branches, same address
   computations; only the loop's registers rotate by one (retail v=$a1,
   offset=$a2, quotient=$a3, digits=$a0, mfhi=$v0; ours v=$a2, offset=$a3,
   quotient=$v0, digits=$a1, and mfhi needs a third scratch $t3). Measured on top
   of this body: an explicit running byte offset (`offset = 8; offset += 4;
   pSrc += 2`) instead of `i * 4 + 8` is 85.22% at the same size (it loses the
   offset-first `addu` and the folded +8 displacement), `s32 v` is 89.62% at 476
   bytes, and two 4-minute permuter runs found only a no-op if/else (948 vs 971).
   cdk gives 93.74, so it is not a preset question.
   Round 306 measured the address SPIKE on the same body: retail's per-store
   address is `lw queue; addu v0,v0,s0; addu v0,a2,v0` + `sb K(v0)` - i.e.
   `(queue + arg0*0x40) + off` with the +0x40 as an `off`-side sum, and the base
   register FIRST. Writing `(g_BattleCommandQueue + arg0 * 0x40)[off + 8]` (the
   natural row-subscript form, which is also the only one that reloads the global
   per store the way retail does) gives the commuted `addu v0,s0,v0` and 87.12%
   at 472 bytes, so the second half is right but the operand order needs the
   embedded-assignment/base-in-a-statement forms, not the subscript. */

/*
void func_8007FF14(u8 arg0) {
    s32 i;
    u8 v;
    s32 n;
    s32 digits;

    func_8009A2D4(arg0);
    for (i = 2; i < 5; i++) {
        v = D_800D2C0C[(i - 2) * 2];
        *(u8 *)(g_BattleCommandQueue + (arg0 << 6) + i * 4 + 8) = 0xFF;
        *(u8 *)(g_BattleCommandQueue + (arg0 << 6) + i * 4 + 9) = 0xFF;
        *(u8 *)(g_BattleCommandQueue + (arg0 << 6) + i * 4 + 0xA) = 0xFF;
        n = v / 100;
        digits = 0;
        if (n != 0) {
            v -= n * 100;
            *(u8 *)(g_BattleCommandQueue + (arg0 << 6) + i * 4 + 8) = n + 0x83;
            digits = 1;
        }
        n = v / 10;
        if (n != 0 || digits != 0) {
            v -= n * 10;
            *(u8 *)(g_BattleCommandQueue + (arg0 << 6) + (i * 4 + digits) + 8) = n + 0x83;
            digits++;
        }
        *(u8 *)(g_BattleCommandQueue + (arg0 << 6) + (i * 4 + digits) + 8) = v + 0x83;
    }
    BattleAllocDrawBuffer();
    if (g_BattleActorCharIds[arg0] != 7) {
        BattleBuildSpritePrimLists(arg0);
        BattleUpdateFrame();
        func_800898F0(arg0);
        BattleUpdateFrame();
        g_BattleDisplayState[0xAD] = 1;
        g_BattleDisplayState[0xC7] = 1;
    }
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FF14);
void BattleResetActorDrawState(u8 arg0) {
    s32 index;

    g_BattleDisplayState[0xAD] = 0;
    g_BattleDisplayState[0xC7] = 0;
    g_BattleDisplayState[0xA8] = 0;
    index = arg0 * 8;
    if (g_BattleSlotFlags[index] == 2) {
        g_BattleSlotFlags[index] = 1;
    }
    HeapFree(g_BattleSpriteColumnWork);
}

/* PARKED 92.56% (2440 bytes against retail's 2436, +4 bytes): whole 610-
   instruction body written from the assembly; every block, call, branch and the
   101-word jump table are in retail's order, and the function is one instruction
   longer than retail. 348 differing lines, all register/order residuals:
   (a) `addu` operand order on the displayState row addresses - retail is
       base-first (`addu v0,v0,a1`, `addu v0,v0,s0`), every spelling tried gives
       index-first (`addu v0,a1,v0`): `g_BattleDisplayState[arg0 + 0x90]` and
       `*(u8 *)(g_BattleDisplayState + (arg0 + 0x90))` measure the same 92.56%
       (see levers.md a-addu-index-first-is-the-legitimiser: the legitimiser
       decides it for a plain `u8 *` global).
   (b) retail loads g_BattleCommandQueue once and stores BOTH 0x2D5 and 0x2D4
       through that one base; two global subscripts reload it, because a char
       store through the pointer can alias the pointer variable itself. Worth
       ~3 instructions at that pair; a pointer local only there is untested.
   (c) the two per-slot 0x1C/0x20 mask stores and the commandFlags fill loop
       share retail's single `sll a0,a0,6`; this body re-forms `arg0 * 0x40`.
   Round status: 5 scored runs, best 92.56% on the first body; no new best after
   the addu respelling, so parked under the STOP rule. The register set, the
   frame (0x28) and the saved set ($ra,$s0-$s3) already match retail's.
*/
/*
void func_80080160(u8 arg0) {
    u8 gauge;
    u8 hasWeapon;
    s32 i;
    u8 v;

    g_BattleCommandQueue[0x2EA] = 1;
    g_BattleCommandQueue[0x2E9] = 0;
    g_BattleCommandQueue[0x2DE] = 0;
    g_BattleCommandQueue[0x2E0] = 0;
    g_BattleCommandQueue[0x2E1] = 0;
    g_BattleCommandQueue[0x2E2] = 0xFF;
    g_BattleCommandQueue[0x2E3] = 0;
    g_BattleCommandQueue[0x2E4] = 0;
    g_BattleCommandQueue[0x2DC] = 0;
    gauge = D_800D32A4[arg0 * 8];
    g_BattleCommandQueue[0x2D5] = gauge;
    g_BattleCommandQueue[0x2D4] = gauge;
    *(u8 *)(g_BattleDisplayState + (arg0 + 0x90)) = 3;
    g_BattleCommandQueue[0x2E7] = 0;
    g_BattleLastMenuEvent = 0xFF;
    g_BattleTaskListBusy = 0;
    while (*(u8 *)(g_BattleDisplayState + (arg0 + 0x90)) != 1) {
        BattleUpdateFrame();
    }
    g_BattleSoundEnabled = 1;
    BattlePlaySoundIfEnabled(0x5A);
    g_BattleSoundEnabled = 0;
    for (i = 0; i < 0x10; i++) {
        ((u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x1C))[i] =
            *(u16 *)(g_BattleSlotStatusRows + arg0 * 0x170) & D_800C3234[i];
    }
    if (g_BattleActorCharIds[arg0] == 7 && g_BattleSlotFlags[arg0 * 8] != 0) {
        *(u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x36) = D_800C3234[13];
        *(u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x24) = D_800C3234[4];
    }
    if (g_BattleActorCharIds[arg0] == 4) {
        if (g_BattleSlotFlags[arg0 * 8] == 0) {
            if (*(u8 *)((u8 *)&g_GameState + 0x2286 +
                    g_GameState.characters[4].extraWeaponIds[0]) == 0) {
                *(u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x1C) = D_800C3234[0];
            }
            hasWeapon = *(u8 *)((u8 *)&g_GameState + 0x2286 +
                g_GameState.characters[4].extraWeaponIds[3]);
        } else {
            if (*(u8 *)((u8 *)&g_GameState + 0x22B6 +
                    g_GameState.gears[g_GameState.characters[4].gearId].extraWeaponIds[0]) == 0) {
                *(u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x1C) = D_800C3234[0];
            }
            hasWeapon = *(u8 *)((u8 *)&g_GameState + 0x22B6 +
                g_GameState.gears[g_GameState.characters[g_BattleActorCharIds[arg0]].gearId]
                    .extraWeaponIds[3]);
        }
        if (hasWeapon == 0) {
            *(u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x20) = D_800C3234[2];
        }
    }
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        if (*(u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x2E) == 0) {
            g_BattleCommandQueue[0x2DD] = 1;
            g_BattleMenuEventHistory = 0;
        } else {
            g_BattleCommandQueue[0x2DD] = 4;
            g_BattleMenuEventHistory = 3;
        }
        if (g_BattleSlotEntries[arg0 * 0x170] & 2) {
            g_BattleCommandQueue[0x2DD] = 2;
            g_BattleMenuEventHistory = 1;
        }
        BattleAllocDrawBuffer();
    } else {
        if (*(u16 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x2E) == 0) {
            g_BattleCommandQueue[0x2DD] = 0x10;
            g_BattleMenuEventHistory = 0;
        } else {
            g_BattleCommandQueue[0x2DD] = 0x13;
            g_BattleMenuEventHistory = 3;
        }
        func_8007FF14(arg0);
    }
    BattleClearCommandSlots();
    BattleClearSlotFlag15A1(arg0);
    BattleBeginAnimTask(BattleSlotBitMask(arg0) |
        BattleSlotBitMask(*(u8 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x3C)));
    v = *(u8 *)(g_BattleCommandQueue + arg0 * 0x40 + 0x3C);
    if (v == 0xFF) {
        v = 0;
    }
    BattleApplySlotMaskToEffects(BattleSlotBitMask(v));
    while (g_BattleCommandQueue[0x2DE] == 0 && D_800C48EA == 0 &&
           g_BattleCommandQueue[0x2DB] == 0) {
        if (g_BattleSlotFlags[arg0 * 8] == 0) {
            func_8007FBE0(g_BattleCommandQueue[0x2D5], g_BattleCommandQueue[0x2D4]);
        }
        if (g_BattleCommandQueue[0x2E2] != g_BattleCommandQueue[0x2DD]) {
            g_BattleCommandQueue[0x2F6] = 0;
            g_BattleCommandQueue[0x2E2] = g_BattleCommandQueue[0x2DD];
            g_BattleDisplayState[0xCB] = 0;
            g_BattleDisplayState[0xA3] = func_8008D598(arg0,
                g_BattleCommandQueue[0x2DD], g_BattleCommandQueue[0x2E0]);
            g_BattleDisplayState[0xCB] = 1;
        }
        if (g_BattleMenuEvent != 0xFF) {
            BattleUpdateFrame();
            if (g_BattleCommandQueue[0x2DB] == 0) {
                if (g_BattleCommandQueue[0x2E0] == 0) {
                    g_BattleSoundEnabled = 1;
                }
                switch (g_BattleCommandQueue[0x2DD]) {
                case 0x1:
                    BattleAdvanceSlotMenuStateEventDisplay(arg0);
                    break;
                case 0x2:
                    BattleAdvanceSlotMenuStateEventActivate(arg0);
                    break;
                case 0x3:
                    BattleAdvanceSlotMenuStateEventExit(arg0);
                    break;
                case 0x4:
                    BattleAdvanceSlotMenuStateEventFourSlot(arg0);
                    break;
                case 0x64:
                    g_BattleCommandQueue[0x2E2] = 0xFF;
                    // fallthrough
                case 0x5:
                    BattleAdvanceSlotMenuStateEventCommand(arg0);
                    break;
                case 0x7:
                    BattleAdvanceSlotMenuStateEventQueue(arg0);
                    break;
                case 0x8:
                    BattleAdvanceSlotMenuStateEventSubMode(arg0);
                    break;
                case 0x9:
                    BattleAdvanceSlotMenuStateEventRandom(arg0);
                    break;
                case 0xA:
                    BattleAdvanceSlotMenuState(arg0);
                    break;
                case 0x10:
                    BattleSetActorSlotDisplayMode(arg0);
                    break;
                case 0x11:
                    BattleSetActorSlotMenuMode(arg0);
                    break;
                case 0x12:
                    BattleAdvanceSlotMenuStateCounter(arg0);
                    break;
                case 0x13:
                    BattleAdvanceSlotMenuStateAE(arg0);
                    break;
                case 0x15:
                    BattleAdvanceSlotMenuStateSlotFlags(arg0);
                    break;
                case 0x16:
                    BattleAdvanceSlotMenuStateSubMode(arg0);
                    break;
                case 0x17:
                    BattleAdvanceSlotMenuStateRandom(arg0);
                    break;
                case 0x18:
                    BattleAdvanceSlotMenuStateText(arg0);
                    break;
                case 0x65:
                    g_BattleCommandQueue[0x2E2] = 0xFF;
                    // fallthrough
                case 0x19:
                    func_80083948(arg0);
                    break;
                default:
                    break;
                }
            }
        }
    }
    g_BattleDisplayState[0xCB] = 0;
    g_BattleDisplayState[0xAF] = 0;
    *(u8 *)(g_BattleDisplayState + (arg0 + 0x90)) = 0;
    *(u8 *)(g_BattleDisplayState + (arg0 + 0x7F)) = 0;
    if (g_BattleCommandQueue[0x2E3] != 0) {
        g_BattleSlotDisplayValue[arg0 * 8] += g_BattleCommandQueue[0x2D4];
        if (g_BattleSlotDisplayValue[arg0 * 8] >= 0x1D) {
            g_BattleSlotDisplayValue[arg0 * 8] = 0x1C;
        }
    }
    g_BattleSoundEnabled = 0;
    BattleNoOpCallback(arg0, 0x100);
    if (g_BattleCommandQueue[0x2DE] != 0 || D_800C48EA != 0) {
        BattleFreeStringTableAE();
        BattleFreeStringTable96();
        BattleResetActorDrawState(arg0);
        BattleReleaseEffectSlotIfChar4(arg0);
    }
    g_BattleSlotCountdownTimers[arg0] = g_BattleSlotDisplayValues[arg0];
    *(u8 *)(g_BattleDisplayState + (arg0 + 0x7C)) = 1;
    D_800C4928 = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080160);
s32 BattleFindMinValueOtherSlot(u8 arg0) {
    s32 best = 0xFF;
    s32 i = g_BattleTargetScan.cursor;
    s32 sel;
    s32 idx;
    // g_BattleSlotCountdownTimers is 0x2F bytes past the ring; retail forms the
    // table off the ring base (addiu t3,v0,0x2F), so naming the symbol costs 4 bytes.
    s16 *pTimers = (s16 *)((u8 *)&g_BattleTargetScan + 0x2F);

    do {
        idx = g_BattleTargetScan.order[i];
        i++;
        if (pTimers[idx] < best && idx != arg0) {
            sel = idx;
            best = *(u8 *)&pTimers[idx];
        }
        if (i == sizeof(g_BattleTargetScan.order)) {
            i = 0;
        }
    } while (i != g_BattleTargetScan.cursor);
    return sel;
}

void BattleQueueCommandFE(u8 color) {
    g_BattleCommandSlotIds[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFE;
    g_BattleCommandCodes[g_BattleCommandQueue[0x2DA] * 0x48] = color;
    g_BattleSoundEnabled = 0;
}

void BattleSetFlag2DBAndReleaseAssets(void) {
    g_BattleCommandQueue[0x2DB] = 1;
    if (g_BattleCommandQueue[0x2D3] < 3 && D_800C204C == 0) {
        BattleFreeStringTableAE();
        BattleFreeStringTable96();
        BattleResetActorDrawState(g_BattleCommandQueue[0x2D3]);
        BattleReleaseEffectSlotIfChar4(g_BattleCommandQueue[0x2D3]);
    }
}

void BattleSetActorFlag34(u8 index) {
    u8 *pEntry = g_BattlePrimPool + index * 0x38;

    pEntry[0x34] = 1;
}
/* 88.65%: second full reconstruction; every block, call and branch is in
   retail's order and the function is 1244 bytes against retail's 1220. What the
   shape work settled since the 86.16 park:
   (a) the function is `void`, not u8 - retail's epilogue has no copy of the
       `ret` flag into $v0 (it is only ever tested), which removes one move.
   (b) the BattlePairTargetable fill loop must READ AND BUMP in one statement:
       `while (i < 0xB) { slot = i++; ... }` gives retail's `move s1,s0` plus
       `sb s1` (a plain `slot = i;` is copy-propagated away and the loop stores
       the counter directly). Only the `addiu s0,s0,1` position is still off:
       retail has it in the beqz delay slot, ours hoists it above the call and
       reorg fills the slot with `slti`.
   (c) the random-target loop is a do/do whose outer test reads the GLOBAL
       (`while (g_BattleTargetSlot == arg0)`), which is what makes retail reload it.
   (d) case 2's cost index must go through a local first (`i = info[1] + 0x16;`)
       or gcc distributes the multiply and folds 0x16*0x28 into the symbol as
       `D_800CDD53+880`; and the D_800CCD38 offset must be a local too
       (`i = arg0 * 0x170;`) or gcc materialises the address once with `la` and
       shares it instead of re-forming retail's two sym+reg operands. Both from
       one 4-minute permuter run (base 5054 -> 3469) plus a hand improvement.
   Still open: (i) case 1's command-queue block - retail loads g_BattleCommandQueue twice
   for the last two statements and reuses each load across a store
   (lbu 0x2DA + lbu 0x3C share one; lbu 0x2DA, the g_BattleCommandRecords index and the
   `g_BattleCommandQueue[0x2DA]++` share the other), we reload once per statement, +2
   loads; a pointer local covers them but keeps the value live across
   BattleSlotBitMask, which needs a sixth callee-saved register.
   (ii) arg0 sits in s2, retail has it in s3 (s0..s4 are used in both).
   (iii) the tail `while (a && b && c)` block is laid out with the third test
   hoisted above the body, costing one instruction.
   Tried and no help: -O3, -G0, cdk (preset is not the issue), `li` order,
   pointer locals for the queue base, s32/u8 slot, do/while(0) around the
   queue block. */
/*
void func_80080C94(u8 arg0) {
    u8 info[8];
    u8 buf[0xB];
    u8 *pBuf;
    u8 cost;
    u8 ret = 1;
    s32 idx;
    s32 r;
    s32 i;
    s32 slot;

    BattleUpdateFrame();
    for (i = 0xA; i >= 0; i--) {
        buf[i] = 0xFF;
    }
    g_BattleCommandQueue[0x2EA] = 1;
    g_BattleCommandQueue[0x2DE] = 0;
    BattleClearSlotFlag15A1(arg0);
    BattleRollActorActionCondition(arg0, info, g_BattleCommandQueue + arg0 * 0x40 + 0x2A);
    if (info[0] == 1 || info[0] == 2) {
        pBuf = buf;
        i = 0;
        while (i < 0xB) {
            slot = i++;
            if (BattlePairTargetable(arg0, slot) != 0) {
                *pBuf++ = slot;
            }
        }
        do {
            do {
                g_BattleTargetSlot = buf[GameRandomByteRange(0, 0xA)];
            } while (g_BattleTargetSlot == 0xFF);
        } while (g_BattleTargetSlot == arg0);
    }
    BattleInitSlotRowsForActor(arg0, g_BattleTargetSlot);
    idx = arg0 * 0x40;
    (g_BattleCommandQueue + idx)[0x3C] = g_BattleTargetSlot;
    BattleBeginAnimTask(BattleSlotBitMask(arg0) | BattleSlotBitMask((g_BattleCommandQueue + idx)[0x3C]));
    BattleUpdateFrame();
    D_800C204C = 1;
    switch (info[0]) {
    case 1:
        g_BattleSlotDisplayInitialized = 0;
        if (g_BattleActorCharIds[arg0] != 4) {
            g_BattleCommandCodes[g_BattleCommandQueue[0x2DA] * 0x48] = arg0;
            g_BattleCommandSlotIds[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFD;
            *(u16 *)(g_BattleCommandSlotMasks + g_BattleCommandQueue[0x2DA] * 0x48) = 0;
            *(u16 *)(g_BattleCommandRecords + g_BattleCommandQueue[0x2DA] * 0x48) =
                BattleSlotBitMask((g_BattleCommandQueue + idx)[0x3C]);
            g_BattleCommandQueue[0x2DA]++;
        }
        if (g_BattleSlotFlags[arg0 * 8] == 0) {
            g_BattleCommandQueue[0x2DC] = 0;
        } else {
            g_BattleCommandQueue[0x2DC] = info[1];
        }
        ret = 0;
        r = func_800877E0(arg0, (g_BattleCommandQueue + idx)[0x3C]);
        BattleInitActorSlotAnimation(r, arg0, (g_BattleCommandQueue + idx)[0x3C], BattleFindMinValueOtherSlot(arg0));
        BattleAdvanceCommandStep(arg0, info[1] + 1);
        BattleQueueCommandFE(arg0);
        break;
    case 2:
        i = info[1] + 0x16;
        cost = D_800CDD53[i * 0x28 + arg0 * 0x5F0];
        i = arg0 * 0x170;
        if (*(u16 *)(D_800CCD38 + i) >= cost) {
            *(u16 *)(D_800CCD38 + i) -= cost;
            g_BattleCommandQueue[0x2E6] = info[1];
            ret = 0;
            BattleQueueOneActorCommand(arg0);
        }
        break;
    case 4:
        BattleMarkSlotReleased(arg0);
        ret = 0;
        g_BattleCommandQueue[0x2DE] = 1;
        break;
    default:
        break;
    }
    if (ret != 0) {
        BattleSetSlotFlag15A1(arg0);
        g_BattleCommandQueue[0x2EA] = 0;
        g_BattleCommandQueue[0x2DE] = 1;
    }
    while (g_BattleCommandQueue[0x2DE] == 0 && D_800C48EA == 0 && g_BattleCommandQueue[0x2DB] != 0) {
        BattleUpdateFrame();
    }
    D_800C204C = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080C94);