#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

// Candidate scan order for target selection.
typedef struct {
    /* 0x0 */ u8 cursor;     // next entry of order[] to consider
    /* 0x1 */ u8 order[0xB]; // slot ids, in candidate order
} BattleTargetOrder; // Size: 0xC

extern u8 *g_BattleCommandQueue;

extern u8 *D_800C3DE8;
extern u8 *g_BattleDisplayState;
extern u8 *g_BattleSpriteColumnWork;
extern void *D_800D367C;
extern u8 *g_BattlePrimPool;
extern u8 D_800C400B[];
extern u8 D_800C402F[];
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
extern u8 D_800C4022[];
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
extern void func_8009BAC4(u8 arg0, u8 *pInfo, u8 *pField);
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);
extern void BattleInitSlotRowsForActor(s32 arg0, s32 arg1);
extern s32 func_800877E0(s32 arg0, s32 arg1);
extern void BattleInitActorSlotAnimation(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 func_80087AF0(u8 arg0, u8 arg1);
extern void BattlePlaySoundWithBaseId(u8 arg0);
extern void BattleActorInitSpriteFromSlot(void);
extern void func_80085D34(void);
extern void func_800861D0(u8 arg0, u8 arg1);
extern s32 BattleCheckCommandKeyStatusBit(u8 arg0, u8 arg1);
extern u16 g_BattleSlotCountdownTimers[];
extern u8 D_800C31D4[][8];
extern void func_8008ADD0(u8 arg0);
extern void BattleSetSlotFlag15A1(u8 arg0);
extern u8 g_BattleSlotDisplayInitialized;
extern u8 g_BattleTargetSlot;
extern u8 D_800C48EA;
extern u8 D_800CCD38[];
extern u8 D_800CDD53[];
extern u8 g_BattleMenuEvent;
extern u8 g_BattleLastMenuEvent;
extern u8 func_8008B478(u8 arg0);
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern void BattleEnsureTargetSlotInCandidateList(u8 arg0);
extern void BattleInitPulseTrianglePrims(void);
extern void func_800826CC(u8 arg0); 
extern u8 func_8008BED8(u8 arg0);
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
    /* 0x2D4 */ u8 gaugeValue; // the reading the menu spends, the second index of the D_800C31D4[max * 8 + value] pair
    /* 0x2D5 */ u8 gaugeMax; // its value at menu entry, the first index of that pair and the row func_8007FBE0 draws
    /* 0x2D6 */ u8 commandKeyLength; // commandKey bytes filled, 0 for no command
    /* 0x2D7 */ u8 unk2D7[0x3];
    /* 0x2DA */ u8 count; // queued commands, the row the command tables are indexed by
    /* 0x2DB */ u8 commandFinished; // 0 while a queued command runs, 1 once it has; BattleWaitWhileStateIs8 and func_80079778 spin on it
    /* 0x2DC */ u8 unk2DC;
    /* 0x2DD */ u8 menuState; // the state each menu event handler switches to
    /* 0x2DE */ u8 eventHandled; // handlers set it to release the update wait loop
    /* 0x2DF */ u8 unk2DF;
    /* 0x2E0 */ u8 colorShift; // subtracted from each colour byte of the slot's prims after << 4
    /* 0x2E1 */ u8 commandQueued; // set when a handler queued its command, so its next call returns at once
    /* 0x2E2 */ u8 lastMenuState; // the menuState the current frame was built for
    /* 0x2E3 */ u8 unk2E3;
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

/* 99.39%: two 4-minute permuter runs (base 245 -> 50 -> 40) closed everything
   except ONE register swap. Retail count=$t0, limit=$t1, color=$t2; this body
   gives color=$t0, limit=$t1, count=$t2. Their two finds are both load-bearing:
   - a real `s32 limit;` local assigned in the preheader (`limit = arg0 - 1;`)
     instead of `i < arg0 - 1` in the test: that is retail's `move t1,v0`, and it
     took the body from 96.29 to 99.24;
   - the preheader pair wrapped in `do { limit = arg0 - 1; pSrc = D_800C3202; }
     while (0);` (schedule as one cluster), 99.24 -> 99.39.
   What is left is not an allocator accident: `color` is the unit's only
   block-local quantity (its def and its use are in the do-while's single block),
   so local_alloc takes $t0 for it before global_alloc hands $t0 to the
   cross-block `count`. Retail has color LAST, i.e. cross-block, which happens
   when `color = 0xC` is written INSIDE the loop and loop.c hoists it into its own
   preheader block - measured: that spelling fixes count/limit ($t0/$t1) but then
   the 0xC is rematerialised in the loop (96.82, one instruction later per
   iteration) because the same variable is set twice there (`(color = 0x12)`,
   which is what keeps retail's in-loop `ori v1,zero,0x12`: as a plain literal
   loop.c hoists it too, 91.82 with count correct). So the two requirements -
   color cross-block, and 0xC hoisted while 0x12 is not - cannot be met by one
   variable spelled from C here. Also measured dead: `shade`/fresh variable for
   the 0x12 (hoisted, 91.82), `(color = 0x12)` as a separate statement. */
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
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FBE0);

void BattleFreeStringTableAE(void) {
    if (g_BattleDisplayState[0xAE] != 0) {
        HeapFree(D_800D367C);
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
    D_800D367C = (void *)BattleAllocHeapBlock(ArchiveDecodeAlignedSize(type), 0);
    ArchiveReadFileToBuffer(type, D_800D367C, 0, 0x80);
    BattleWaitForArchiveSync();
    g_BattleDisplayState[0xAE] = 1;
}

void BattleFreeStringTable96(void) {
    if (g_BattleDisplayState[0x96] != 0) {
        HeapFree(D_800C3DE8);
        g_BattleDisplayState[0x96] = 0;
    }
}

void BattleLoadStringTable96(void) {
    s32 pBuffer;

    if (g_BattleDisplayState[0x96] == 0) {
        BattleWaitForArchiveSync();
        BattleSetArchive20Entry3();
        pBuffer = BattleAllocHeapBlock(ArchiveDecodeAlignedSize(3), 0);
        D_800C3DE8 = (u8 *)pBuffer;
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
    D_800C402F[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFE;
    D_800C400B[g_BattleCommandQueue[0x2DA] * 0x48] = color;
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
    func_8009BAC4(arg0, info, g_BattleCommandQueue + arg0 * 0x40 + 0x2A);
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
            D_800C400B[g_BattleCommandQueue[0x2DA] * 0x48] = arg0;
            D_800C402F[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFD;
            *(u16 *)(D_800C4022 + g_BattleCommandQueue[0x2DA] * 0x48) = 0;
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
        func_80087AF0(arg0, info[1] + 1);
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
            func_8008ADD0(arg0);
        }
        break;
    case 4:
        func_800826CC(arg0);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8008115C);
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
}*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081318);
/* 85.02% and JTBL_LAYOUT - and the solver of the whole-family theory: the C
   jump table cannot reach retail's address. This unit's eight tables tile its
   .rodata 0x38C..0x600 exactly (func_80080160's 101-word table at +0x194, then
   seven 0x20-byte tables), so jtbl_80070030 sits at +0x1B4 and jtbl_80070050 at
   +0x1D4 - both 4-mod-8 - while cc1 emits `.align 3` before every jump table. A
   converted switch therefore lands at +0x1B8 and 4 pad bytes appear where retail
   has the table's first word (measured at battle.bin 0x540: retail 0x80081388,
   ours 0x00000000). Converting the whole family cannot fix it either: the first
   table's 0x194 length is not a multiple of 8, so from the second table on the
   pad is forced and the unit's .rodata ends at 0x278 instead of 0x274.
   The rest of the body is instruction-identical; what is left is (a) gcc
   cross-jumps the three `sb v0,0x2DD(v1)` case tails into one shared block that
   retail keeps inline, which also flips case 0's and case 1's branch arms, and
   (b) the two `repeatPending = 1; BattlePlaySoundIfEnabled(0x4F);` sites lose retail's
   early `li a0,0x4F` to the jal delay slot. Writing the plain stores through a
   local `BattleActorSlots *p` (as the func_800816F8 park does) is worse here:
   81.27% / 440 bytes. */
/*
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
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
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
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].commandFlags[11] != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 8;
        }
        break;
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081504);
/* 86.26% and JTBL_LAYOUT for the same reason as func_80081318 above:
   jtbl_80070050 is at .rodata +0x1D4, 4-mod-8, and cc1 aligns jump tables to 8,
   so no switch of this unit can be converted on its own or with the rest of the
   family. Same remaining code differences too - the three `sb v0,0x2DD(v1)`
   tails are cross-jumped into one shared block retail keeps inline (which flips
   the case arms) and the `li a0,0x4F` sites lose their early materialisation.
   The local-pointer store form scores 78.69%. */
/*
void func_80081504(u8 arg0) {
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleSetSlotFlag15A1(arg0);
        ((BattleActorSlots *)g_BattleCommandQueue)->menuRunning = 0;
        ((BattleActorSlots *)g_BattleCommandQueue)->eventHandled = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[9] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 1;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[5] != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 7;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[11] != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 2;
        }
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[10] == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 4;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->repeatPending != 0 && g_BattleLastMenuEvent == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[7] != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 0xA;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->repeatPending = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].commandFlags[8] != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->menuState = 9;
        }
        break;
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800816F8);
/* 99.90% and JTBL_LAYOUT: with the switch converted, the C jump table is
   emitted with `.align 3` at object offset 0x1F4, so cc1 inserts a 4-byte pad
   word and the unit's .rodata becomes 0x278 instead of 0x274 - check_bin.sh
   battle then reports MISMATCH (build/out/battle.bin is 4 bytes long) while
   every instruction of the function is byte-identical. The table cannot be
   moved to an 8-aligned offset: the three 0x20-byte asm tables before it
   (func_8008115C/80081318/80081504) plus func_80080160's 0x194 table put it at
   0x1F4, and the backend hardcodes the table alignment. Only convertible
   together with a change that makes the offset 8-aligned. Best body kept.
   Re-measured: converting this one alone gives check_bin.sh battle MISMATCH
   with build/out/battle.bin 343940 bytes against disc's 343936 - exactly the
   one pad word - while the function is 420 bytes, retail's. The unit's .rodata
   starts at 0x8006FE7C (file 0x38C), 4 mod 8, and retail's seven switch tables
   sit at .rodata-relative 0x194, 0x1B4, ... 0x254, i.e. all 4 mod 8 (they are
   8-aligned in vram, which is the alignment cc1's `.align 3` does NOT use). So
   every one of them pads and only the 101-word table at +0 can be aligned. */
/*
void func_800816F8(u8 arg0) {
    u8 a;
    u8 *p;
    s32 off;

    a = arg0;
    switch (g_BattleMenuEvent) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTableAE(a);
        if (func_8008B478(a) != 0) {
            func_8008ADD0(a);
        } else {
            g_BattleCommandQueue[0x2DD] = 4;
        }
        break;
    case 0:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2E) == 0) {
            p[0x2DD] = 1;
            break;
        }
        if (p[0x2F6] != 0 && g_BattleLastMenuEvent == 0) {
            if (*(u16 *)(p + off + 0x26) != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                g_BattleCommandQueue[0x2DD] = 7;
            }
            g_BattleCommandQueue[0x2F6] = 0;
        } else {
            g_BattleCommandQueue[0x2F6] = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 1:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x32) == 0) {
            p[0x2DD] = 2;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        g_BattleCommandQueue[0x2DD] = 3;
        break;
    case 3:
        p = g_BattleCommandQueue;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2A) == 0) {
            p[0x2DD] = 0xA;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}*/

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
        D_800C400B[g_BattleCommandQueue[0x2DA] * 0x48] = arg0;
        D_800C402F[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFD;
        pTarget = g_BattleCommandQueue + idx;
        *(u16 *)(D_800C4022 + g_BattleCommandQueue[0x2DA] * 0x48) = 0;
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
   (e) D_800C31D4 is declared [][8] and read with the 2D subscript; the flat
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
   CLOSED THIS ROUND (341): case 4's arm ends with its OWN unk2DF++, so the source has
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
        retail materialises li v0,1 (the unk2E3 store's constant) BETWEEN the
        division's `sra` and the timers `sh`, and that register is then taken. A
        `remain = 1;` statement before the timer store does hoist the constant into
        the middle of the sequence but lands it before `mfhi` in $a0 (98.47) and also
        moves case 4's `addiu` to $a0; moving unk2E3 above the timers store and a
        local for the quotient were measured in round 341 at 1368 / 97.04. A
        do-while(0) around the two stores instead hoists `sll s0,s0,0x1` and
        `lui a2,0x9249` out of the table sequence (1360, 97.27).
   (iii) the g_BattleDisplayState[0xAF] = 1 store takes the base in $v1 and the 1 in
        $v0 where retail has them the other way round.
   The preset is right: a 4-minute permuter run on THIS body found nothing
   (base 213 = best 213, 1572 iterations); round 341's run on the 98.23 body was
   444 = 444. cdk is 95.31, -G0 is the same, and 2.6.0 does not compile the unit. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081B58);
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
    ((BattleActorSlots *)g_BattleCommandQueue)->unk2DF = 0;
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
        ((BattleActorSlots *)g_BattleCommandQueue)->unk2DF++;
    case 7:
        ((BattleActorSlots *)g_BattleCommandQueue)->unk2DF++;
    case 6:
        ((BattleActorSlots *)g_BattleCommandQueue)->unk2DF++;
        if (((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue
                - ((BattleActorSlots *)g_BattleCommandQueue)->unk2DF >= 0) {
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
                - ((BattleActorSlots *)g_BattleCommandQueue)->unk2DF;
            func_80085D34();
            who = arg0;
            func_800861D0(g_BattleMenuEvent, who);
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E5 =
                func_80087AF0(who, ((BattleActorSlots *)g_BattleCommandQueue)->unk2DF);
            g_BattleSlotCountdownTimers[who] =
                (D_800C31D4[((BattleActorSlots *)g_BattleCommandQueue)->gaugeMax]
                            [((BattleActorSlots *)g_BattleCommandQueue)->gaugeValue] * 100) / 56;
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2E3 = 1;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80082504);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800826CC);
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
