#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

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
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
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
extern u8 D_800D2C34;
extern u8 func_80084854(u8 arg0, u8 arg1);
extern void func_80093B08(u8 arg0);
extern u16 BattleSlotBitMask(u8 arg0);
extern void BattleBeginAnimTask(u16 arg0);
extern void func_800BCD98(u16 arg0);
extern void func_80088490(u8 arg0);
extern void func_8009AEFC(u8 arg0);
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
extern void func_800B89FC(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern void func_80087AF0(u8 arg0, u8 arg1);
extern void func_8008ADD0(u8 arg0);
extern void BattleSetSlotFlag15A1(u8 arg0);
extern u8 g_BattleSlotDisplayInitialized;
extern u8 g_BattleTargetSlot;
extern u8 D_800C48EA;
extern u8 D_800CCD38[];
extern u8 D_800CDD53[];
extern u8 g_BattleMenuEvent;
extern u8 D_800C3E29;
extern u8 func_8008B478(u8 arg0);
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern void BattleEnsureTargetSlotInCandidateList(u8 arg0);
extern void BattleInitPulseTrianglePrims(void);
extern void func_800826CC(u8 arg0);
extern u8 func_8008BED8(u8 arg0);
extern void BattleActivateSelectedCommand(u8 arg0);

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
    /* 0x3C */ u8 unk3C;
    /* 0x3D */ u8 unk3D[0x3];
} BattleActorSlot; // Size: 0x40

// 0x2F8-byte block of actor slots plus the slot menu state that follows them.
typedef struct {
    /* 0x000 */ BattleActorSlot slot[0xB];
    /* 0x2C0 */ u8 unk2C0[0x1D];
    /* 0x2DD */ u8 unk2DD;
    /* 0x2DE */ u8 unk2DE;
    /* 0x2DF */ u8 unk2DF[0xB];
    /* 0x2EA */ u8 unk2EA;
    /* 0x2EB */ u8 unk2EB[0xB];
    /* 0x2F6 */ u8 unk2F6;
    /* 0x2F7 */ u8 unk2F7;
} BattleActorSlots; // Size: 0x2F8

/* 82.95%: best of 6 shapes. The loop body matches instruction for
   instruction (four reloads of the volatile counter, the two separate
   `lbu 0(a1)` of pSrc[i], the incrementing pSrc pointer in the branch delay
   slot) and so do the prologue and the tail. What is left is loop.c's
   invariant hoisting: retail hoists `lw a3,g_BattleGraphicsWork` and `li t2,0xC` into
   the preheader and leaves `li v1,0x12` in the body, while our gcc hoists
   neither (it keeps the base load and both constants in the body, 1 insn
   shorter) and puts the `li 0xC` in the body. Writing pSrc as a pre-loop
   local (74.02) puts its init before the loop test, which retail has after;
   declaring it inside the loop body (this shape) fixes the order and the
   incrementing pointer. A `u8 *pBase = g_BattleGraphicsWork;` pre-loop local does hoist
   the load but reorders the preheader (71.44). -O3 / -G0 / cdk change nothing
   material. */
/*
void func_8007FBE0(u8 arg0, u8 arg1) {
    u8 count;
    s32 i;

    count = arg1;
    if (arg0 == arg1) {
        count = arg1 - 1;
    }
    for (i = 0; i < arg0 - 1; i++) {
        u8 *pSrc = D_800C3202 + arg0 * 6;

        *(u16 *)(g_BattleGraphicsWork + (g_BattlePrimStartIndex + i * 2) * 0x10 + 0x910) = 0xC;
        *(u16 *)(g_BattleGraphicsWork + (g_BattlePrimStartIndex + i * 2) * 0x10 + 0x912) = pSrc[i] + 0x5E;
        *(u16 *)(g_BattleGraphicsWork + (g_BattlePrimStartIndex + i * 2) * 0x10 + 0x914) = 0x12;
        *(u16 *)(g_BattleGraphicsWork + (g_BattlePrimStartIndex + i * 2) * 0x10 + 0x916) = pSrc[i] + 0x5E;
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


INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FF14);
/* 91.69%: only a register rotation left (retail v=$a1, digits=$a0, i*4=$a2,
   quotient=$a3, mfhi=$v0; ours shifts by one). 12 hand shapes tried plus a
   4-minute permuter run (best score 948 vs base 971, and its "fix" was a no-op
   if/else, so nothing to port). cdk gives 93.74, so not a preset question. */
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
}*/



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
    s32 i = D_800D2DD7[0];
    s32 sel;
    s32 idx;
    s16 *pSlots = (s16 *)(D_800D2DD7 + 0x2F);

    do {
        idx = D_800D2DD8[i];
        i++;
        if (*(pSlots + idx) < best && idx != arg0) {
            sel = idx;
            best = *(u8 *)(pSlots + idx);
        }
        if (i == 0xB) {
            i = 0;
        }
    } while (i != D_800D2DD7[0]);
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
        func_800B89FC(r, arg0, (g_BattleCommandQueue + idx)[0x3C], BattleFindMinValueOtherSlot(arg0));
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
        if (p[0x2F6] != 0 && D_800C3E29 == 3) {
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
   (b) the two `unk2F6 = 1; BattlePlaySoundIfEnabled(0x4F);` sites lose retail's
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
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 2;
        }
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2E == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 1;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 != 0 && D_800C3E29 == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk26 != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 7;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 2:
        ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 3;
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk30 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 4;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 != 0 && D_800C3E29 == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk2A != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 0xA;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[a].unk32 != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 8;
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
        ((BattleActorSlots *)g_BattleCommandQueue)->unk2EA = 0;
        ((BattleActorSlots *)g_BattleCommandQueue)->unk2DE = 1;
        break;
    case 0:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].unk2E == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 1;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 != 0 && D_800C3E29 == 0) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].unk26 != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 7;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 1:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].unk32 != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 2;
        }
        break;
    case 3:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].unk30 == 0) {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 4;
            break;
        }
        if (((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 != 0 && D_800C3E29 == 3) {
            if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].unk2A != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 0xA;
            }
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 0;
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2F6 = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 2:
        if (((BattleActorSlots *)g_BattleCommandQueue)->slot[arg0].unk2C != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            ((BattleActorSlots *)g_BattleCommandQueue)->unk2DD = 9;
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
   together with a change that makes the offset 8-aligned. Best body kept. */
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
        if (p[0x2F6] != 0 && D_800C3E29 == 0) {
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
    func_800BCD98(BattleSlotBitMask(g_BattleCommandQueue[0x2E8]));
    i = 0;
    do {
        if (func_80084854(g_BattleCommandQueue[0x2E8], i) != g_BattleCommandQueue[0x2E8]) {
            (g_BattlePulseTrianglePrims + i)[0xE6] = 1;
        } else {
            (g_BattlePulseTrianglePrims + i)[0xE6] = 0;
        }
        i++;
    } while (i < 4);
    if (D_800D2C34 != 4) {
        func_80093B08(arg0);
    }
}
/* 92.34%: `BattleSlotBitMask` returns u16, so a separate `orBits = f(a); orBits |=
   f(b);` masks both results; writing the two calls as one `|` expression inside
   the BattleBeginAnimTask call lets gcc fold them into the single `or` + final `andi
   0xffff` the original has. What is left: the original loads g_BattleCommandQueue once and
   shares the base between the `[0x2DA]` read and the `+arg0*0x40` pointer for
   the BattleSlotBitMask argument (we reload it), and the two `addu base,base,idx`
   come out with the index register as destination instead of the base
   (allocator tie, both values dead after). */
/*
void func_800819A4(u8 arg0) {
    s32 orBits;

    if (g_BattleCommandQueue[0x2E9] != 0) {
        return;
    }
    g_BattleCommandQueue[0x2E9] = 1;
    func_800BCD98(0);
    *(u8 *)((u32)g_BattleCommandQueue + arg0 * 0x40 + 0x3C) = g_BattleCommandQueue[0x2E8];
    BattleFreeStringTableAE();
    BattleFreeStringTable96();
    BattleClearFlagC6();
    orBits = BattleSlotBitMask(arg0) | BattleSlotBitMask(g_BattleCommandQueue[arg0 * 0x40 + 0x3C]);
    BattleBeginAnimTask(orBits);
    if (g_BattleActorCharIds[arg0] != 4) {
        D_800C400B[g_BattleCommandQueue[0x2DA] * 0x48] = arg0;
        D_800C402F[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFD;
        *(u16 *)(D_800C4022 + g_BattleCommandQueue[0x2DA] * 0x48) = 0;
        *(u16 *)(g_BattleCommandRecords + g_BattleCommandQueue[0x2DA] * 0x48) =
            BattleSlotBitMask(g_BattleCommandQueue[arg0 * 0x40 + 0x3C]);
        g_BattleCommandQueue[0x2DA]++;
    }
    BattleClearSlotFlagsIfChar4(arg0, 0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800819A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081B58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80082504);
/* 84.29%: every instruction after the flag update matches. The blocker is the
   flag update itself: retail has the gas `sym+reg` form twice
   (`lui at,%hi(g_BattleSlotConditions); addu at,at,v0; lbu/sb %lo(at)`, i.e. the base
   rematerialised into $at for each access, 4 more insns) while cse in our gcc
   makes one `la $3,g_BattleSlotConditions` and shares it (`addu v0,v0,v1`), 8 bytes
   shorter, so the size cannot match either. gcc does emit the `sym+reg` macro
   form for g_BattleSlotFlags/g_BattleActorCharIds in this same function; it registers an
   address only where it is used twice. Tried: plain `|=`, array-ref, pointer
   local, volatile. */
/*
void func_800826CC(u8 arg0) {
    s32 i;

    func_80088490(arg0);
    func_8009AEFC(arg0);
    BattleFinishActorAnimationAndReleaseSlot(arg0);
    g_BattleSlotConditions[arg0 * 0x170] |= 0x80;
    BattleReleaseCharSlot(arg0);
    g_BattleSlotFlags[arg0 * 8] = 2;
    if (g_BattleActorCharIds[arg0] != 7) {
        *(u8 *)(arg0 * 0x1E4 + (u32)g_BattleGraphicsWork + 0x853D) = 2;
    }
    D_800C3EB8[arg0 * 0x1C] = 1;
    g_BattleCommandQueue[arg0 + 0x2EB] = 1;
    for (i = 0; i < 3; i++) {
        if (g_GameState.partyMembers[i] == g_BattleActorCharIds[arg0] && g_FieldPlayerWalkmeshMaterial == 0) {
            g_GameState.gearRide[i] = 1;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800826CC);
