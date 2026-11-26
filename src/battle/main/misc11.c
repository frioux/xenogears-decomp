#include "common.h"
#include "battle/battle_work.h"
#include "main/game.h"
#include "psyq/libgpu.h"

// One slot's command row (g_BattleCommandQueue + slot * 0x40): the 16 status
// words a slot's commands fill in, and the slot it is linked to.
typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ u16 statusWords[0x10]; // the slot's status rows: the 16 masks the setup loop ANDs the slot's conditionFlags with, individual entries overwritten per command
    /* 0x3C */ u8 targetSlotId; // slot id whose bitmask is applied with this slot's; 0xFF = none
    /* 0x3D */ u8 unk3D[0x3];
} BattleCommandRow; // Size: 0x40

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
// array view of the same byte: the outer target loop must re-read it, which the
// scalar spelling's store-to-load fold deletes
extern u8 g_BattleTargetSlotRead[] __asm__("g_BattleTargetSlot");
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
extern u8 BattleRunQueuedSlotCommands(u8 arg0);
extern void BattleActivateSelectedCommand(u8 arg0);
extern u8 g_BattleTaskListBusy;
extern u8 g_BattleMenuEventHistory;
extern u8 g_BattleSlotDisplayValue[];
extern u8 g_BattleSlotStatusRows[];
extern u16 g_BattleSlotEntries[];
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


/* 91.24% (468 bytes / 117 insns - retail's own instruction count, and every
   loop register retail's): the quotient is a u8, which is what the "andi
   v1,a3,0xff before the branch but the raw quotient in the addiu" pair was
   saying. With `s32 n` the body is 118 insns / 472 bytes and rotates the whole
   caller-saved set by one (v=a2, offset=a3, n=v0, digits=a1, mfhi=t3); with
   `u8 n` v=a1, offset=a2, digits=a0, n=a3, both masked copies in v1, the 0xFF in
   t2, i in t1, D_800D2C0C in t0 - retail exactly - because a QI pseudo may keep
   the unmasked srl result in its register (the store's addiu reuses it) and is
   masked at each SI use, which gcc cse's into retail's single andi.
   SOURCE ORDER IS IRRELEVANT HERE: writing `n = v / 100; digits = 0;` before the
   three 0xFF stores instead of after them measures the identical 91.24 / 468 /
   117 (sched1 puts the division back where retail has it either way).
   What is LEFT is one allocator artifact, the mfhi scratch: retail
   `mfhi v0 / srl a3,v0,5` at both divisions, ours `mfhi t3 / srl a3,t3,5`, so
   the third 0xFF store's address stays live across it in v0 (retail has
   `sb t2,10(v0)` BEFORE the mfhi and `move a0,zero` in the beqz delay slot, ours
   has them the other way round). Everything still differing downstream is the
   consequence: retail's three tail address temps take v1 and the
   g_BattleCommandQueue base loads v0, ours mirror those two.
   cc1 -da/-dl on this body: the 15 global allocnos are processed
   115 138 136 77 75 179 166 84 134 86 89 180 74 96 72 and land 115/138 in v1,
   77 in a0, 75 in a1, 179 in a2, 84/134 in a3, 166 in s0, 72 in s1, 74 in t1,
   96 in t2 - retail's hand-out - while NO allocno holds t3: the mfhi
   destination is not in the disposition list and global_alloc prints
   "Spilling reg 11" twice, i.e. t3 is a spill slot for the mfhi qty, not a
   priority contest a spelling can re-rank. Treat the residual as parked.
   Measured dead on this body or the s32-n one: an explicit running byte offset
   (`offset = 8; offset += 4; pSrc += 2`) 85.22, `s32 v` 89.62 at 476 bytes,
   declaration order, and two 4-minute permuter runs (base 245 -> 50 -> 40). cdk
   gives 93.74, so it is not a preset question. Round 306's address-spike note
   still holds: the per-store address is base-first `(queue + arg0*0x40) + off`,
   and `(g_BattleCommandQueue + arg0 * 0x40)[off + 8]` commutes the addu (87.12).
   objdiff's percentage is not monotone for this pair - the 118-insn body scores
   91.69, the 117-insn one 91.24 - judge by the instruction count. */

// Round 490: re-measured 91.24% / 468 bytes / 117 insns (unchanged). One new datum: `u8 digits` measures 90.09% / 476 / 119, so the digits counter stays s32. The named blocker is unchanged: the mfhi result is spilled to t3 (global_alloc "Spilling reg 11") where retail has v0, because retail completes the third 0xFF store before the mfhi.
// Round 505: re-measured 91.24% / 468 bytes / 117 insns == retail's count. New datum: moving 'n = v / 100' ABOVE the three 0xFF stores gives byte-identical output (cc1 normalises the order), so the split 'retail sb t2,10(v0) before mfhi' cannot be bought with source order - it is sched2/reorg choosing ours: fill the lw delay slot with 'move a0,zero' and put the sb in the beqz slot. Parked.
/*
void func_8007FF14(u8 arg0) {
    s32 i;
    u8 v;
    u8 n;
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
}*/
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

// PARKED 95.01% (2424 bytes / 606 insns against retail's 2436 / 609): the whole
// 610-instruction body, three instructions SHORT of retail. Round 475 moved it from
// 92.56 to 95.01 with four measured edits:
//   (a) the per-slot row is now a typed struct access -
//       `((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[K]`
//       (double negation, lever c-double-negation-on-a-slot-stride-from-a-pointer-global)
//       - which gives retail's base-first `addu v1,v1,v0` at the 0x1C/0x20/0x24/0x2E
//       stores; the plain row subscript is index-first there. It does NOT flip the two
//       0x3C (`targetSlotId`) sites, which stay mirrored (`addu v1,s1,v1` against
//       retail's `addu v1,v1,s1`), and the negation there costs a `negu` + `subu`,
//       so those two keep the raw `((u8 *)(g_BattleCommandQueue + arg0 * 0x40))[0x3C]`.
//   (b) `((BattleWork *)g_BattleDisplayState)->panelMode[arg0]` / ->panelActive[arg0] /
//       ->slotMarks[arg0] instead of `*(u8 *)(state + (arg0 + 0x90))`: the struct
//       displacement gives retail's base-first `addu v0,v0,a1`. Same lever, three sites
//       plus the loop guard.
//   (c) the status-row loop reads `*(u16 *)(g_BattleSlotStatusRows + rowOffset)` with
//       rowOffset (`arg0 * 0x170`) a hoisted local - the typed
//       `g_BattleWorkBlock[arg0].conditionFlags` re-forms the base per iteration the way
//       retail does but leaves the 0x7A in the `lhu` displacement, which is a different
//       instruction word. Keeping the symbol's own address is the only matchable spelling.
//   (d) a `u8 *pQueue = g_BattleCommandQueue` local assigned BEFORE the gauge read makes
//       retail's single load feed both 0x2D5/0x2D4 stores (the two subscripts reload), and
//       `i = 0; rowOffset = ...; for (; i < 0x10; i++)` puts retail's `move a2,zero` in
//       its preheader slot.
// Open residuals, 264 side-by-side lines of which only ~30 are not branch-target
// cascades: (i) the two targetSlotId `addu` mirrors; (ii) retail's single row pointer
// shared by the 0x36/0x24 stores in the char-7 arm (a pointer local there took the body
// to 93.97 and then lost instructions elsewhere); (iii) mine re-truncates arg0
// (`andi v0,s2,0xff` + `sll v0,v0,0x6`) where retail does `sll v0,a1,0x6`, and an
// `andi a1,s2,0xff` sits two instructions late; (iv) g_BattleSlotFlags' base is hoisted
// into $s3 across the tail loop where retail re-forms it through $at. The body is now
// THREE instructions shorter than retail, so the next step is finding which of retail's
// three extra instructions (`andi`s and the row pointer) the spellings dropped.
// Tried and no help: typed BattleWorkBlock tables (disp), plain row subscript, cast-first
// sum, pQueue per statement.
// Round 488 re-parked at the same 95.01 / 2424 / 606 after measuring:
//   (preset leg, closed) try_flags --all: gcc-2.6.0 gives 96.34 for this function but
//       drops two already-matched neighbours (BattleFindMinValueOtherSlot 99.69,
//       BattleSetActorFlag34 97.00), and gcc-2.7.2-cdk gives 85.02 and breaks four
//       (BattleLoadStringTableAE 92.78, BattleResetActorDrawState 88.67,
//       BattleFindMinValueOtherSlot 79.22, BattleQueueCommandFE 73.89). The unit stays
//       Default. -G0 is byte-identical (95.01), -O1 is 82.54, -O3 will not compile it.
//   (targetSlotId typed, NEUTRAL) `((BattleCommandRow *)(g_BattleCommandQueue +
//       arg0 * 0x40))->targetSlotId` at the two 0x3C sites (the struct-member-displacement
//       route of lever c-struct-member-displacement-flips-a-reloaded-global-base) is
//       byte-identical to the raw `[0x3C]` form: the `addu` stays index-first. Kept
//       because it is the readable spelling at the same 95.01 / 2424 / 606.
//   (the tail is the 7-instruction hole) retail's `if (q[0x2E3])` block is 23 insns
//       against this body's 16, and the difference is BOTH a redundant reload and a
//       per-access address form: retail forms `lui $at,%hi` + `addu $at,$at,a0` +
//       `lbu/sb %lo($at)` for each of its four refs to g_BattleSlotDisplayValue (a0 =
//       arg0 * 8) while every plain spelling here hoists ONE `lui/addiu` base + `addu`
//       into a register and reuses it. Probed with cc1_probe on this unit: `+=`,
//       `A[i] = A[i] + x`, four refs and two refs all give the hoisted `la` + `andi
//       0xff`; `extern volatile u8` gives retail's extra reload but STILL hoists the
//       `la`. No probed spelling gives both, so the hole is a named blocker, not an
//       untried spelling. Also evidence against retyping 0x800D32A0 as one 8-byte-row
//       struct: g_BattleSlotDisplayValue/Flags/gauge are bytes 0/1/4 of one 8-byte
//       row, yet retail addresses each through its own symbol with its own $at.
/*
void func_80080160(u8 arg0) {
    u8 gauge;
    u8 hasWeapon;
    s32 i;
    s32 rowOffset;
    u8 *pQueue;
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
    pQueue = g_BattleCommandQueue;
    gauge = D_800D32A4[arg0 * 8];
    pQueue[0x2D5] = gauge;
    pQueue[0x2D4] = gauge;
    ((BattleWork *)g_BattleDisplayState)->panelMode[arg0] = 3;
    g_BattleCommandQueue[0x2E7] = 0;
    g_BattleLastMenuEvent = 0xFF;
    g_BattleTaskListBusy = 0;
    while (((BattleWork *)g_BattleDisplayState)->panelMode[arg0] != 1) {
        BattleUpdateFrame();
    }
    g_BattleSoundEnabled = 1;
    BattlePlaySoundIfEnabled(0x5A);
    g_BattleSoundEnabled = 0;
    i = 0;
    rowOffset = arg0 * 0x170;
    for (; i < 0x10; i++) {
        ((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[i] =
            *(u16 *)(g_BattleSlotStatusRows + rowOffset) & D_800C3234[i];
    }
    if (g_BattleActorCharIds[arg0] == 7 && g_BattleSlotFlags[arg0 * 8] != 0) {
        ((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[0xD] = D_800C3234[13];
        ((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[4] = D_800C3234[4];
    }
    if (g_BattleActorCharIds[arg0] == 4) {
        if (g_BattleSlotFlags[arg0 * 8] == 0) {
            if (*(u8 *)((u8 *)&g_GameState + 0x2286 +
                    g_GameState.characters[4].extraWeaponIds[0]) == 0) {
                ((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[0] = D_800C3234[0];
            }
            hasWeapon = *(u8 *)((u8 *)&g_GameState + 0x2286 +
                g_GameState.characters[4].extraWeaponIds[3]);
        } else {
            if (*(u8 *)((u8 *)&g_GameState + 0x22B6 +
                    g_GameState.gears[g_GameState.characters[4].gearId].extraWeaponIds[0]) == 0) {
                ((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[0] = D_800C3234[0];
            }
            hasWeapon = *(u8 *)((u8 *)&g_GameState + 0x22B6 +
                g_GameState.gears[g_GameState.characters[g_BattleActorCharIds[arg0]].gearId]
                    .extraWeaponIds[3]);
        }
        if (hasWeapon == 0) {
            ((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[2] = D_800C3234[2];
        }
    }
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        if (((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[9] == 0) {
            g_BattleCommandQueue[0x2DD] = 1;
            g_BattleMenuEventHistory = 0;
        } else {
            g_BattleCommandQueue[0x2DD] = 4;
            g_BattleMenuEventHistory = 3;
        }
        if (g_BattleSlotEntries[arg0 * 0xB8] & 2) {
            g_BattleCommandQueue[0x2DD] = 2;
            g_BattleMenuEventHistory = 1;
        }
        BattleAllocDrawBuffer();
    } else {
        if (((BattleCommandRow *)(g_BattleCommandQueue - -(arg0 * 0x40)))->statusWords[9] == 0) {
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
        BattleSlotBitMask(((BattleCommandRow *)(g_BattleCommandQueue + arg0 * 0x40))->targetSlotId));
    v = ((BattleCommandRow *)(g_BattleCommandQueue + arg0 * 0x40))->targetSlotId;
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
    ((BattleWork *)g_BattleDisplayState)->panelMode[arg0] = 0;
    ((BattleWork *)g_BattleDisplayState)->panelActive[arg0] = 0;
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
    ((BattleWork *)g_BattleDisplayState)->slotMarks[arg0] = 1;
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
/* 88.70% (1220 bytes - retail's EXACT size - 305 insns against the 306 the
   original object dumps): every block, call and branch is in retail's order.
   Round 477 closed the +24 bytes the 88.65 park carried: case 1's command block
   re-read g_BattleCommandQueue once per statement (`g_BattleCommandRecords +
   g_BattleCommandQueue[0x2DA] * 0x48` and the `g_BattleCommandQueue[0x2DA]++`)
   where retail loads the base ONCE after the BattleSlotBitMask call and reuses
   it for the 0x2DA load, the records store and the increment. Assigning a
   pointer local at that point (`pQueue = g_BattleCommandQueue;` immediately
   before the records store) puts the base in a caller-saved register exactly as
   retail does - it is NOT live across the call, because the assignment sits
   after it - and takes the body from 311 insns / 1244 bytes to 305 / 1220.
   What the earlier parks' shape work settled, still true:
   (a) the function is void, not u8 - retail's epilogue has no copy of the ret
       flag into v0 (it is only ever tested).
   (b) the BattlePairTargetable fill loop must READ AND BUMP in one statement:
       `while (i < 0xB) { slot = i++; ... }`, or the loop stores the counter.
   (c) the random-target loop is a do/do whose outer test reads the GLOBAL.
   (d) case 2's cost index must go through a local first (`i = info[1] + 0x16;`)
       or gcc folds 0x16*0x28 into the symbol as D_800CDD53+880, and the
       D_800CCD38 offset must be a local too (`i = arg0 * 0x170;`).
   Open residuals: (i) the remaining 143 differing lines are register rotation -
   arg0 is s2 here, s3 in retail - plus ONE instruction: the random-target
   loop's outer test is short because gcc forwards the stored value, while retail
   re-reads the global (lui/lw/lbu g_BattleTargetSlot, nop, beq); that reload is
   the whole 305-vs-306 gap. (ii) the fill loop's `addiu s0,s0,1` sits before the
   BattlePairTargetable call instead of in its beqz delay slot (reorg).
   Tried and no help: -O3, -G0, cdk (preset is not the issue), li order, s32/u8
   slot, do/while(0) around the queue block, one 4-minute permuter run
   (base 5054 -> 3469). */

// Round 490: 88.70 -> 90.08% / 1224 bytes / 306 insns. NOTE the old note above is wrong on one number: retail is 306 insns / 1224 bytes (0x80080C94..0x8008115C), so the old 305-insn / 1220-byte body was 4 bytes SHORT of retail and could never have linked. Two edits, both kept:
//   (a) THE OUTER RANDOM-TARGET TEST NEEDS A RELOAD, AND AN ARRAY ALIAS WITH AN __asm__ LABEL SUPPLIES IT: `extern u8 g_BattleTargetSlotRead[] __asm__("g_BattleTargetSlot");` and `} while (g_BattleTargetSlotRead[0] == arg0);`. Reading the scalar after storing to it makes cse forward the stored byte, so only 305 insns; a second gcc object with a different name at the same address keeps retail`s `lui %hi / lbu %lo / nop / beq` reload (lever a-label-alias-array-read-beats-volatile, round 444). 88.70 -> 89.97 alone.
//   (b) THE MASK THE INNER TEST COMPARES AGAINST IS AN `s32` LOCAL MASKED ONCE IN THE PREHEADER: `s32 target;` + `target = arg0 & 0xFF;` before the do/do gives retail`s `andi s0,s3,0xff` in the preheader and `beq v0,s0`; a `u8 target` is coalesced to arg0 and re-masks at the use (`andi v0,s2,0xff` inside the loop, 305 insns), and no local at all 305. 89.97 -> 90.08 with the insn count 306 = retail.
// Open residual, 118 differing lines, all allocation/scheduling. Read it as ONE tie, not many: the random-loop preheader has three loop-live values (&buf, the 0xFF digit, the masked target) and retail hands them s1/s2/s0 while ours hands them s0/s1/s2 - a cyclic 3-rotation of pseudos that are each referenced once, so the quotient `floor_log2(refs)*refs/live_length` ties and the hand-out falls to processing order; the s0/s1/s2 sites all follow from it. Second residual is the trailing wait loop, where ours hoists the 0x2DB test into an entry test and jumps back to the body from the D_800C48EA test while retail keeps one three-test block at the bottom. Smaller leftovers measured: `li v1,0xff` / `li s0,0xa` order in the fill-loop setup, `move s0,zero` out of the beqz delay slot, and the BattleRollActorActionCondition argument chain (retail loads the queue into v0, ours into a2).
// Round 505: re-measured 90.08% / 1224 bytes / 306 insns (== retail's count, unchanged). Three new measurements, all worse - do not repeat: (1) dropping the pQueue local for plain g_BattleCommandQueue[...] gives 312 insns / 90.04% (six extra lui/lw base reloads), so pQueue is load-bearing; (2) the fill loop as for(i=0;i<0xB;i++){slot=i;...} loses retail's move s1,s0 copy and its addiu in the beqz slot - 89.63%; (3) the tail wait loop as for(;;)+three breaks gives 309 / 88.77%, so the && chain is the right spelling. Named residual unchanged: the queue base load is hoisted across the BattleSlotBitMask call (retail reloads after it and uses the pre-call base for the 60(v0) read).
/*
void func_80080C94(u8 arg0) {
    u8 info[8];
    u8 buf[0xB];
    u8 *pBuf;
    u8 *pQueue;
    u8 cost;
    u8 ret = 1;
    s32 idx;
    s32 r;
    s32 i;
    s32 slot;
    s32 target;

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
        target = arg0 & 0xFF;
        do {
            do {
                g_BattleTargetSlot = buf[GameRandomByteRange(0, 0xA)];
            } while (g_BattleTargetSlot == 0xFF);
        } while (g_BattleTargetSlotRead[0] == target);
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
            pQueue = g_BattleCommandQueue;
            *(u16 *)(g_BattleCommandRecords + pQueue[0x2DA] * 0x48) =
                BattleSlotBitMask((g_BattleCommandQueue + idx)[0x3C]);
            pQueue[0x2DA]++;
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
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080C94);


