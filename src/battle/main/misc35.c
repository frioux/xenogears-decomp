#include "common.h"
#include "psyq/libgte.h"
#include "system/memory.h"
#include "system/kernel.h"
#include "system/sound.h"
#include "psyq/libsn.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/inline_c.h"

extern void SoundFreeWdsEntry(SoundWDSEntry *pWds);
extern void SoundRemoveSedsEntry(void *pSoundFile);
extern SoundWDSEntry *g_BattleWdsEntry;
extern SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *pWdsFile, s32 arg1);
extern void SoundAddSedsEntry(SoundFile *pSoundFile);
extern s16 SoundGetPendingSpuCommandType(s32 arg0);
extern void SpriteSetupAnimPackage(SpriteAnimPackage *pAnimPackage, SpriteAnimPackageFileHeader *pAnimPackageFile,
                                   SVEC2 tex, SVEC2 clut, u32 field_0x3C_1);
extern SpriteAnimPackage g_SpriteAnimPackageDirs6;
extern u8 g_BattleSoundArchiveLoaded;
extern u8 g_BattleActorArchiveReady;

// A packed battle archive buffer: the word at 0 counts the three header words that
// follow it plus one offset word per chunk, so a directory walk runs count - 3
// times over the offsets at 0x10. Each offset is relative to the buffer start and
// the chunk it points at begins with a signature word saying what the chunk holds.
typedef struct {
    /* 0x00 */ s32 dirWordCount;
    /* 0x04 */ u8 unk4[0xC];
    /* 0x10 */ s32 chunkOffsets[1];
} SoundArchiveBuffer;

void BattleCalcLookAtRotation(SVECTOR *pFrom, SVECTOR *pTo, u16 *pRot) {
    MATRIX mtx; // unused: reproduces retail's 0x20-byte stack slot
    VECTOR diff;
    VECTOR sq;
    s32 len;

    diff.vx = pFrom->vx - pTo->vx;
    diff.vy = pFrom->vy - pTo->vy;
    diff.vz = pFrom->vz - pTo->vz;
    Square0(&diff, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    pRot[1] = ratan2(diff.vz, diff.vx);
    pRot[2] = ratan2(diff.vy, len);
    pRot[0] = 0;
}
typedef void (*BattleVectorCallback)(VECTOR *pFrom, VECTOR *pTo);

extern void BattleAverageRotatedVectors(s32 group, s32 angle, SVECTOR *pPos, VECTOR *pOut);
extern s32 D_800D2FCC;

// Emits the trail segments for a direction-transform task: for each of the five
// rotation groups it averages the four source positions under the group's eight
// rotation matrices and hands the result to pCallback together with the previous
// one. partCount below 5 pads the source array up to five entries first.
void BattleEmitTrailSegments(s32 partCount, SVECTOR *pVectors, BattleVectorCallback pCallback) {
    VECTOR vecA;
    VECTOR vecB;
    SVECTOR vecC; // unused: reproduces retail's 0x60 frame
    s32 i;
    s32 n;

    D_800D2FCC = 0;
    if (partCount < 5) {
        if (partCount < 3) {
            if (partCount < 2) {
                return;
            }
            vecA.vx = pVectors[0].vx;
            vecA.vy = pVectors[0].vy;
            vecA.vz = pVectors[0].vz;
            vecB.vx = pVectors[1].vx;
            vecB.vy = pVectors[1].vy;
            vecB.vz = pVectors[1].vz;
            pCallback(&vecA, &vecB);
            return;
        }
        switch (partCount) {
        case 3:
            pVectors[3].vx = pVectors[2].vx;
            pVectors[3].vy = pVectors[2].vy;
            pVectors[3].vz = pVectors[2].vz;
            pVectors[4].vx = pVectors[2].vx;
            pVectors[4].vy = pVectors[2].vy;
            pVectors[4].vz = pVectors[2].vz;
            break;
        case 4:
            pVectors[4].vx = pVectors[3].vx;
            pVectors[4].vy = pVectors[3].vy;
            pVectors[4].vz = pVectors[3].vz;
            break;
        }
        partCount = 5;
    }
    BattleAverageRotatedVectors(0, 0, pVectors, &vecA);
    n = 0;
    for (i = 1; i != 8; i++) {
        if (i & 1) {
            BattleAverageRotatedVectors(0, i, &pVectors[n], &vecB);
            pCallback(&vecA, &vecB);
        } else {
            BattleAverageRotatedVectors(0, i, &pVectors[n], &vecA);
            pCallback(&vecB, &vecA);
        }
    }
    if (partCount >= 6) {
        n = 1;
        BattleAverageRotatedVectors(1, 0, &pVectors[n], &vecA);
        pCallback(&vecB, &vecA);
        for (i = 1; i != 8; i++) {
            if (i & 1) {
                BattleAverageRotatedVectors(1, i, &pVectors[n], &vecB);
                pCallback(&vecA, &vecB);
            } else {
                BattleAverageRotatedVectors(1, i, &pVectors[n], &vecA);
                pCallback(&vecB, &vecA);
            }
        }
    }
    for (n = 2; n < partCount - 5; n++) {
        BattleAverageRotatedVectors(2, 0, &pVectors[n], &vecA);
        pCallback(&vecB, &vecA);
        for (i = 1; i != 8; i++) {
            if (i & 1) {
                BattleAverageRotatedVectors(2, i, &pVectors[n], &vecB);
                pCallback(&vecA, &vecB);
            } else {
                BattleAverageRotatedVectors(2, i, &pVectors[n], &vecA);
                pCallback(&vecB, &vecA);
            }
        }
    }
    if (partCount >= 7) {
        n = partCount - 5;
        BattleAverageRotatedVectors(3, 0, &pVectors[n], &vecA);
        pCallback(&vecB, &vecA);
        for (i = 1; i != 8; i++) {
            if (i & 1) {
                BattleAverageRotatedVectors(3, i, &pVectors[n], &vecB);
                pCallback(&vecA, &vecB);
            } else {
                BattleAverageRotatedVectors(3, i, &pVectors[n], &vecA);
                pCallback(&vecB, &vecA);
            }
        }
    }
    n = partCount - 4;
    BattleAverageRotatedVectors(4, 0, &pVectors[n], &vecA);
    pCallback(&vecB, &vecA);
    for (i = 1; i != 8; i++) {
        if (i & 1) {
            BattleAverageRotatedVectors(4, i, &pVectors[n], &vecB);
            pCallback(&vecA, &vecB);
        } else {
            BattleAverageRotatedVectors(4, i, &pVectors[n], &vecA);
            pCallback(&vecB, &vecA);
        }
    }
    if (partCount >= 5) {
        vecA.vx = pVectors[partCount - 1].vx;
        vecA.vy = pVectors[partCount - 1].vy;
        vecA.vz = pVectors[partCount - 1].vz;
        pCallback(&vecB, &vecA);
    }
}
// One 16-byte row of the per-group direction matrix table: the four IR0 factors a
// rotation group applies to its four source vectors in turn.
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
    /* 0xC */ s32 unkC;
} BattleDirectionMatrixRow; // Size: 0x10

extern BattleDirectionMatrixRow *g_BattleDirectionMatrix;

// Rotates one direction group's four source vectors, sums them into pOut and
// quarters the sum - the average a trail segment is built from. Group numbers
// eight matrix rows each, index picks the row inside the group.
void BattleAverageRotatedVectors(s32 group, s32 index, SVECTOR *pPos, VECTOR *pOut) {
    s32 vec[3];

    gte_lddp(g_BattleDirectionMatrix[group * 8 + index].unk0);
    vec[0] = pPos[0].vx;
    vec[1] = pPos[0].vy;
    vec[2] = pPos[0].vz;
    gte_ldlvl(vec);
    gte_gpf12();
    gte_stlvl((s32 *)pOut);

    gte_lddp(g_BattleDirectionMatrix[group * 8 + index].unk4);
    vec[0] = pPos[1].vx;
    vec[1] = pPos[1].vy;
    vec[2] = pPos[1].vz;
    gte_ldlvl(vec);
    gte_gpf12();
    gte_stlvl(vec);
    pOut->vx += vec[0];
    pOut->vy += vec[1];
    pOut->vz += vec[2];

    gte_lddp(g_BattleDirectionMatrix[group * 8 + index].unk8);
    vec[0] = pPos[2].vx;
    vec[1] = pPos[2].vy;
    vec[2] = pPos[2].vz;
    gte_ldlvl(vec);
    gte_gpf12();
    gte_stlvl(vec);
    pOut->vx += vec[0];
    pOut->vy += vec[1];
    pOut->vz += vec[2];

    gte_lddp(g_BattleDirectionMatrix[group * 8 + index].unkC);
    vec[0] = pPos[3].vx;
    vec[1] = pPos[3].vy;
    vec[2] = pPos[3].vz;
    gte_ldlvl(vec);
    gte_gpf12();
    gte_stlvl(vec);
    pOut->vx += vec[0];
    pOut->vy += vec[1];
    pOut->vz += vec[2];
    pOut->vx >>= 2;
    pOut->vy >>= 2;
    pOut->vz >>= 2;
}

void BattleFreeWdsEntry(void) {
    if (g_BattleWdsEntry != 0) {
        SoundFreeWdsEntry(g_BattleWdsEntry);
    }
    g_BattleWdsEntry = 0;
}

// 94.06% (392 bytes / 98 insns against retail's 404 / 101): the wait loop, its debug break,
// the switch, the frame and 8 of retail's 9 saved registers are exact; the whole residual is
// one hoisted invariant and one register count.
// THE 'break 0x400' DOES HAVE A C SPELLING: pollhost() (psyq/libsn.h: asm("break 1024"); controller_b.c
// ControllerUpdate and field/scripts/virtual_machine.c both ship it at 100%). Round 274's park
// claimed it had none, so the loop was parked with an empty if - with the statement in, the
// beq into the loop head, the break and the j back edge all match retail.
// THE LEFT OVER 3 INSTRUCTIONS = ONE HOISTED CONSTANT: retail hoists lui $s7,%hi(g_BattleWdsEntry)
// into the OUTER loop preheader and stores through it (sw $v0,%lo(...)($s7)), keeps -1 in $s6 and
// carries a DEAD lui $s5,0xFFFF0000 (saved and restored, never read); our compile hoists the
// INNER loop's lui %hi(g_DebugTable) into $s6 instead, so s6/s5 swap, the WDS store re-forms its
// %hi inline (2 insns where retail has 1) and we save 8 registers where retail saves 9
// (+2 save/restore, +1 dead lui, +1 in-loop debug lui, -1 in-loop WDS lui = the 12 bytes).
// So both invariant %hi halves are candidates to ONE of which gcse/loop.c moves, and retail moved
// the outer-loop one. Retail's dead 0xFFFF0000 (li -0x10000 / & 0xFFFF0000, the constant that
// pairs with a << 16) says its source had ONE MORE invariant constant than this body produces,
// and that extra movable is what tipped the choice; nothing in the emitted code shows where it
// came from.
// MEASURED ON THIS BODY: *(volatile s32 *)&g_DebugTable for the read - identical (the %hi half is
// hoisted whether or not the load is volatile); while ((SoundGetPendingSpuCommandType(0) << 16) != 0)
// - identical (the sll is already there). Prior rounds, not repeated: a second reference to
// g_BattleWdsEntry inside the loop 93.76/408 (both %hi hoisted, so retail's second ref must have
// been cse'd away), pointer-to-global local 400, local copy of the load result 392, do/while 384,
// 2.6.0 87.90, plain 2.7.2 87.95, -O3 91.39, if/else or continue guard 94.06. cc1 -dL on the
// sibling func_800C11CC showed a single-use invariant %hi has savings 0, which is why no spelling
// of this body moves it. All logs are 392 bytes: the body is honest (no UB park), it just cannot
// be linked.
//  94.34% (400 bytes / 100 insns against retail's 404 / 101): frame and saved set are now
//  retail's (9 saves, s0-s7 + ra), the wait loop, its break and the switch are exact, and
//  the WDS store has retail's shape (sw %lo(g_BattleWdsEntry)(s7), not an inline %hi).
//  THE RESIDUAL IS TWO hoisting decisions, and retail's 4 surplus bytes are BOTH of them:
//  1. retail's preheader holds only lui s7,%hi(g_BattleWdsEntry) and the store's offset is
//     %lo; the pointer-to-global local here makes gcc materialise the FULL address
//     (lui v0,%hi + addiu s7,v0,%lo, 1 instruction more than retail).
//  2. retail carries a DEAD lui s5,0xFFFF0000 there (saved and restored, never read); the
//     direct g_BattleWdsEntry spelling (94.06% / 392) has neither the dead constant nor the
//     %hi hoist, it re-forms %hi inline per iteration.
//  So retail's source had an invariant constant gcc hoisted and then deleted. MEASURED THIS
//  ROUND, do not repeat: spelling the wait test as ((SoundGetPendingSpuCommandType(0) << 16)
//  & 0xFFFF0000) != 0 DOES make gcc hoist lui 0xffff into a saved register, but cc1 2.7.2-cdk
//  does NOT fold the redundant and (it stays 'and v0,v0,s3'), so the constant is read after
//  all and that body scores 91.55 / 400 - and the extra invariant displaces the -1 hoist into
//  the wds arm (li s0,-1 per iteration). So the fold retail's compile made happens after
//  loop.c hoisted the constant, and no spelling of this loop test reaches it; the items the
//  note above already lists stay measured dead.
/*
// Feeds one battle actor archive buffer to the sound player: every SEDS chunk is
// added, every WDS chunk is (re)loaded after freeing the loaded one, and every
// other chunk sets up an animation package. Returns the last SEDS chunk added.
SoundFile *func_800C0FAC(SoundArchiveBuffer *pBuffer) {
    SoundFile *pSeds;
    SoundFile *pChunk;
    SoundWDSEntry **pWdsSlot;
    s32 *pEntry;
    SVEC2 tex;
    SVEC2 clut;
    s32 i;

    pSeds = NULL;
    pWdsSlot = &g_BattleWdsEntry;
    pEntry = (s32 *)pBuffer;
    i = pEntry[0] - 3;
    pEntry += 4;
    while (i > 0) {
        pChunk = (SoundFile *)(pEntry[0] + (s32)pBuffer);
        switch (pChunk->magic) {
        case FILE_SIGNATURE('s', 'e', 'd', 's'):
            pSeds = pChunk;
            SoundAddSedsEntry(pSeds);
            break;
        case FILE_SIGNATURE('w', 'd', 's', ' '):
            BattleFreeWdsEntry();
            g_BattleSoundArchiveLoaded = 0;
            g_BattleActorArchiveReady = 0;
            *pWdsSlot = SoundLoadWdsFile((SoundWDSEntry *)pChunk, 0);
            while ((SoundGetPendingSpuCommandType(0) << 16) != 0) {
                if (g_DebugTable != -1) {
                    pollhost();
                }
            }
            if (i == 1) {
                HeapInsertAlloc((HeapBlock *)pBuffer, pEntry[0]);
            }
            break;
        default:
            tex.x = 0x380;
            tex.y = 0x100;
            clut.x = 0;
            clut.y = 0x1F4;
            SpriteSetupAnimPackage(&g_SpriteAnimPackageDirs6, (SpriteAnimPackageFileHeader *)pChunk, tex, clut, 0);
            break;
        }
        i--;
        pEntry++;
    }
    return pSeds;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0FAC);


void BattleRemoveSedsEntries(s32 *pArg) {
    s32 *pEntry = pArg;
    s32 i = pEntry[0] - 3;

    pEntry += 4;
    for (; i > 0; i--) {
        s32 *pChunk = (s32 *)(pEntry[0] + (s32)pArg);

        if (*pChunk == 0x73646573) {
            SoundRemoveSedsEntry(pChunk);
        }
        pEntry++;
    }
}

extern u8 g_AnimScriptCommandLengths[]; // per-opcode instruction length, indexed by the opcode byte
extern u8 g_BattleStateWork[];
extern s32 g_BattleCommandRowIndex;
extern SpriteData *D_800C3E1C;
extern s16 g_BattleActorListCount;
extern s32 g_BattleActorList[];
extern u8 D_800C4928;
extern u8 g_BattleAnimScriptHalted;
extern s32 g_WorkListCurTimer;
extern SVECTOR g_BattleSlotVectors;
extern SVECTOR g_BattleSlotVectorsB;
extern SVEC D_800D30B0;
extern u16 D_800D30B8;
extern s16 D_80059454;
extern IVEC3 g_BattleAnimPositionGroupA;
extern IVEC3 g_BattleAnimPositionGroupB;

extern void SpriteAnimFrameListSetFrame(SpriteData *pSpriteData, s32 frame);
extern void SpriteSetFrameFromAnimIndex(SpriteData *pSpriteData);
extern void SpriteSetSVec(SVECTOR *pVector, s16 x, s16 y, s16 z);
extern void SpriteSetMoveSpeed(SpriteData *pSpriteData, s32 speed);
extern s32 SpriteComputeAngleFromPoints(s32 from, s32 to);
extern void SpriteSetDirection(SpriteData *pSpriteData, s16 direction);
extern void func_800223B0(SpriteData *pSpriteData, s16 direction);
extern void SpritePlayAnimation(SpriteData *pSpriteData, s32 animId);
extern void func_800B3F04(SpriteData *pSpriteData, s32 arg1, void *pValue);
extern s32 BattleGetActorFlag41(void);
extern void BattleStartScreenFade(s32 frames, s32 abr, u8 r, u8 g, u8 b);
extern void BattleCreateTimedScaleTask(SVECTOR *pTarget, s32 frames);
extern void func_80023538(SpriteData *pSpriteData, u8 *pScript);
extern void BattleCreateActorDistanceWaitTask(SpriteData *pSpriteData, s32 minDistance, void *pCallback);
extern void BattleActorUpdateFallMotion(SpriteData *pSpriteData);
extern void BattleActorComputeFallDirection(SpriteData *pSpriteData);
extern s32 ArchiveDataSync(void);
extern void BattleUpdateCommandRows(void);
extern s32 BattleFindTableEntryIndex(SpriteData *pSpriteData);
extern void BattleRemapActorModelId(SpriteData *pSpriteData);
extern void GfxQueueSpriteTileData(void *pData);
extern u8 *BattleGetFrame(u8 *pBase, s32 n);
extern s32 BattleGetPrimitiveBufferSize(void *pFrame);
extern void BattleModelScaleParts(void *pFrame, s32 shift);
extern void BattleExpandModelPrims(void *pFrame, u8 *pDst, s32 mode, u8 shadeTex);
extern void BattleQueueActorCommandPreservingSelection(s32 slot, u16 mask, s32 arg2);
extern void *AnimScriptResolveVariablePointer(SpriteData *pSpriteData, u8 *pIndex);
extern void AnimScriptStackPushU24(SpriteData *pSpriteData, s32 value);
extern s32 AnimScriptStackPopU24(SpriteData *pSpriteData);
extern u_char AnimScriptStackPopU8(SpriteData *pSpriteData);
extern void AnimScriptStackPushU8(SpriteData *pSpriteData, u8 value);
extern void func_8001FBE4(SpriteData *pSpriteData, s32 code, u8 *pArgs);

typedef void (*SpriteScriptCallback)(SpriteData *pSpriteData);


// What the animation id byte of the flag word means here: 0x3F asks for the
// default animation instead of the special one.
#define BATTLE_ANIM_ID_DEFAULT 0x3F
// Bits 28-29 of flags_0xA8 are the slot mode; 0x10000000 arms it.
#define BATTLE_ANIM_MODE_HIGH_MASK 0xCFFFFFFF
#define BATTLE_ANIM_MODE_HIGH_SET 0x10000000
// The two 6-bit frame counters it packs beside the mode: counter1 advances once per
// wait opcode and saturates at 0x3F, counter2 is stepped by the frame-step opcode.
#define BATTLE_ANIM_COUNTER1_MASK 0xF03FFFFF
#define BATTLE_ANIM_COUNTER1_SHIFT 22
#define BATTLE_ANIM_COUNTER2_MASK 0xFFFE07FF
#define BATTLE_ANIM_COUNTER2_SHIFT 11
#define BATTLE_ANIM_COUNTER_MASK 0x3F
// The 4-bit mode field of field_0x40 that picks which position group an angle
// opcode writes back to.
#define BATTLE_ANIM_GROUP_MASK 0xF
#define BATTLE_ANIM_GROUP_A 0xA
#define BATTLE_ANIM_GROUP_B 0xB
// The 12-bit speed field of flags_0xAC, and the 8-bit divisor a wait opcode scales
// its frame count by.
#define BATTLE_ANIM_SPEED_SHIFT 7
#define BATTLE_ANIM_SPEED_MASK 0xFFF
#define BATTLE_ANIM_SPEED_SCALE 256

// The jump displacement is the 16-bit word the opcode carries, sign-extended.
#define SCRIPT_JUMP_AT(p) ((s32)((s16)((p)[1] << 8) | (p)[0]))
// The block offsets the other opcodes carry are signed at the byte they come from.
#define SCRIPT_WORD_AT(p) (((s8 *)(p))[1] << 8 | (p)[0])
// The 24-bit block offset, signed from its top byte.
#define SCRIPT_U24_AT(p) (((s8)(p)[2] << 16) + ((p)[1] << 8) + (p)[0])
// The angle between two x/z points, as the direction opcodes hand it to
// SpriteSetDirection and func_800223B0.
#define SCRIPT_ANGLE(from, to) \
    SpriteComputeAngleFromPoints(((u16 *)(to))[1] << 16 | ((u16 *)(to))[0], \
                                 ((u16 *)(from))[1] << 16 | ((u16 *)(from))[0])



// 67.82% (3700 bytes against retail's 3708; frame 0x88 and every local slot
// exact: vec 0x18, mtx 0x20, out 0x40, pointA 0x50, pointB 0x58). The arm set is
// transcribed against the jump table's 124 case values and every call site
// matches. WHAT IS LEFT IS ONE THING: we save 9 registers (s0-s7 + ra), retail
// saves 8 (s0-s6 + ra), and the leftover is exactly the constant 1 that cse/loop
// keeps in $s7 (see below). pSpriteData is already in retail's $s2, code/frames
// are one register high, so the stream matches except for that pair.
//  - 0xF3: retail holds the COMBINED pointer pArgs + offset in one local and
//    passes it to all three BattleGetFrame calls (addu s4,v0,s1 then three
//    "addu a0,s4"); spelling pArgs + offset at each site keeps `offset` live
//    across those four calls instead. One pFrames local: 66.74 -> 67.13.
//  - 0xF8: retail reloads pSpriteData->pTargetEntitySprite per use (0x74($s2)
//    three times: the count word, case 7's argument, case 9's compare), so the
//    target's live range does not span the inner switch and cannot conflict with
//    `count`. The pTarget local in that arm cost the extra register: 67.13 ->
//    67.82. The other arms keep the local (0xE3 needs it, worth 0.5%).
//  - the surviving extra register is the constant 1: "li $s7,1" lands in the
//    loop head's beqz delay slot (retail has nop) and serves three sites - the
//    animScriptWaitTimer = 1 store in 0x97, the 1 << mask argument of
//    BattleQueueActorCommandPreservingSelection in 0xA4, and the (flags & 3) != 1
//    compare in 0xBE. Retail materialises each separately (addiu v0/a1/v1,zero,1).
//    Measured: writing all three as 2 keeps the sharing (the register just holds
//    2), removing any ONE use keeps it, and writing the 0xCFFFFFFF mask as a
//    literal instead of through the "mask" local is 66.27, so the mask local
//    stays. Breaking it needs one of the three to differ in RTL, and no honest
//    spelling of a constant does that.
//  - the frame: retail's saved set starts at 0x68 where ours starts at 0x60, i.e.
//    retail has 8 more bytes of locals than we do, so dropping to 8 saved
//    registers also owes an 8-byte dead local to keep the frame at 0x88.
//  - 0x9E: g_BattleStateWork + 0x8000 + 0xC58 folds to a %lo(+0x8C58) relocation;
//    assigning the base first and adding 0x8000 in a second statement
//    (pWork = (u8 *)g_BattleStateWork; pWork += 0x8000; *(u16 *)(pWork + 0xC58))
//    reproduces retail's lui/addiu + li 0x8000 + addu + lhu 0xC58.
//  - 0xBE: retail takes the == 1 arm (beq v0,v1) and stores curSpriteFrame in the
//    fall-through. Flipping our if/else to that polarity measured WORSE (65.54),
//    so the != 1 spelling and its arm order stay.
//  - 0x9D and 0x97 read the 16.16 position words: retail uses lh, our build lhu.
//    cc1_probe on a plain "s16 dst[0] = s->xWhole;" shows the same lhu, so this is
//    gcc narrowing a load that is only stored back again, not a type or header
//    bug. 0x9D also duplicates the three position stores in both branches of the
//    group test where ours shares one tail - that is 2 of the 8 bytes we are short.
//  - 0x9A: retail keeps &mtx in s0 across both the RotMatrix and ApplyMatrix calls
//    (addiu s0,sp,0x20 then two "addu ...s0"); ours re-forms sp+0x20 per call.
//  - curSpriteFrame is read unsigned: (u16)pSpriteData->curSpriteFrame gives
//    retail's lhu in both frame-step arms (misc31 uses the same cast).
//  - the SpriteAnimFrameListSetFrame +1/-1 call tails are cross-jumped as retail does
//    (j $L198 shared by the code<0x10 and code<0x30 arms) because both arms end
//    with frames = (code & 0xF) + 1; before the shared if (code < 0x40).
//  - the 0x9A/0x9C tails are ONE tail in source (goto animApplyRotation after each
//    RotMatrix), which is what retail has: it cross-jumps them at the RotMatrix
//    jal (0x9A jumps to 0x800C15A4, 0x9C falls in).
//  - a preset sweep over gcc 2.6.0 / 2.7.2 / 2.7.2-cdk x -O1..-O3 x -G4/-G8 x
//    -mcpu 3000/r8000 ranks the unit preset (cdk -O2 -G0) first at 66.74 (next
//    best 66.43). Permuter runs on the 66.74 and the 67.82 body (796 and 1016
//    iterations) found only wrong-code candidates: they delete the 0x81 arm's
//    second flags_0xA8 store, which retail has both of.
//  - round 435 RE-MEASURED the same body at 67.82% / 3700 bytes and read cc1's own
//    loop decision (`cc1 -dL`, dump in build/src/.../misc35.c.i.loop). Exactly ONE
//    movable in the whole function moves: "Insn 935: regno 399 (life 9),
//    move-insn savings 9 moved to 2336" against "Loop from 10 to 2321: 995 real
//    insns" - the constant 1, from THREE merged `set reg,const 1` loads (savings 3
//    each) whose uses are the 0x97 wait-timer store, the 0xA4 shift operand and the
//    0xBE compare. loop.c's test is threshold * savings * lifetime >= insn_count, so
//    the hoist clears it by 81 * threshold against 995, i.e. threshold >= 12.3 and
//    nothing else: no other movable in the function is even close. Two more
//    source-side negatives measured on this body: giving the 0xA4 shift its own
//    local (`bit = 1 << mode;` before the call, so its result is consumed
//    immediately) changes no byte and does not shorten `life`; and spelling the 0xBE
//    test as `(flags & 3) - 1 != 0`, which removes that const-1 load entirely, also
//    changes no byte and keeps the hoist. So the group's savings does not fall by
//    dropping a use. What the arms DO forbid is the other escape route: retail
//    materialises each of the function's const-1 uses separately (thirteen
//    `addiu ...,zero,0x1` sites in the stub's .s), so a body that makes one of the
//    three merged constants a different RTL constant would still leave twelve sites
//    diverging, and no spelling of a constant changes its mode. PARKED.
//  - round 461 un-parked and re-measured the same body: 67.82% / 3700 bytes / 925
//    insns against retail's 3708 / 927, and counted the constant-1 sites on both
//    sides to price the whole residual: retail materialises 13
//    ("addiu $x,$zero,0x1"), the compile 10 plus the one hoisted "li $s7,1" whose
//    only three uses are 0x97's wait-timer store (sh s7,0x9E($s2)), 0xA4's
//    "1 << ..." (sllv a1,s7,a1) and 0xBE's "(flags & 3) != 1" (beq v0,s7). So the
//    hoist IS the 2-instruction / 8-byte deficit and the 9th saved register at
//    once: breaking it pays the size, the saved set and (with retuned locals) the
//    frame. Nothing else in the diff is independent - it is the register cascade
//    the extra s7 causes. No new spelling was tried: the const-1 group was already
//    measured (one use written as 2, one use dropped, "bit = 1 << mode",
//    "(flags & 3) - 1"), and cc1 -dL names this as the only movable in the loop.
//    Ghidra agrees on the shape (size 0xe7c, frame 0x88, no return value, one
//    argument), so the prototype is not what costs the 8 bytes.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C11CC);
/*
void func_800C11CC(SpriteData *pSpriteData) {
    u8 *pArgs;
    u8 code;
    s32 frames;
    s32 count;
    s32 offset;
    SVECTOR vec;
    MATRIX mtx;
    VECTOR out;
    s16 pointA[2];
    s16 pointB[2];
    SpriteData *pTarget;
    u8 *pBuffer;
    u8 *pFrames;
    u8 *pWork;
    s32 size;
    u32 flags;
    u32 mask;

    while (pSpriteData->animScriptWaitTimer == 0) {
        code = *(u8 *)pSpriteData->pSpriteBytecode;
        pArgs = (u8 *)pSpriteData->pSpriteBytecode + 1;
        if (code < 0x80) {
            pSpriteData->pSpriteBytecode = pArgs;
            if (code < 0x10) {
                SpriteAnimFrameListSetFrame(pSpriteData, (u16)pSpriteData->curSpriteFrame + 1);
                frames = (code & 0xF) + 1;
            } else if (code < 0x20) {
                pSpriteData->flags_0xA8 =
                    (pSpriteData->flags_0xA8 & BATTLE_ANIM_COUNTER2_MASK) |
                    (((((u32)pSpriteData->flags_0xA8 >> BATTLE_ANIM_COUNTER2_SHIFT) & BATTLE_ANIM_COUNTER_MASK) + 1 &
                      BATTLE_ANIM_COUNTER_MASK)
                     << BATTLE_ANIM_COUNTER2_SHIFT);
                SpriteSetFrameFromAnimIndex(pSpriteData);
                frames = (code & 0xF) + 1;
            } else if (code < 0x30) {
                SpriteAnimFrameListSetFrame(pSpriteData, (u16)pSpriteData->curSpriteFrame - 1);
                frames = (code & 0xF) + 1;
            }
            if (code < 0x40) {
                frames = (code & 0xF) + 1;
            }
            frames = frames * (s32)(((u32)pSpriteData->flags_0xAC.flags >> BATTLE_ANIM_SPEED_SHIFT) & BATTLE_ANIM_SPEED_MASK) /
                     BATTLE_ANIM_SPEED_SCALE;
            if (frames == 0) {
                frames = 1;
            }
            count = ((((u32)pSpriteData->flags_0xA8 >> BATTLE_ANIM_COUNTER1_SHIFT) & BATTLE_ANIM_COUNTER_MASK) + 1) &
                    BATTLE_ANIM_COUNTER_MASK;
            pSpriteData->animScriptWaitTimer += frames;
            pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & BATTLE_ANIM_COUNTER1_MASK) |
                                      (count << BATTLE_ANIM_COUNTER1_SHIFT);
            if (count == 0) {
                pSpriteData->flags_0xA8 =
                    (pSpriteData->flags_0xA8 & BATTLE_ANIM_COUNTER1_MASK) |
                    (((((u32)pSpriteData->flags_0xA8 >> BATTLE_ANIM_COUNTER1_SHIFT) & BATTLE_ANIM_COUNTER_MASK) - 1 &
                      BATTLE_ANIM_COUNTER_MASK)
                     << BATTLE_ANIM_COUNTER1_SHIFT);
            }
            return;
        }
        switch (code) {
        case 0xE8:
            func_800B3F04(pSpriteData, pArgs[0], AnimScriptResolveVariablePointer(pSpriteData, pArgs + 1));
            break;
        case 0xCA:
            pBuffer = (u8 *)(pArgs + SCRIPT_WORD_AT(pArgs));
            if (pBuffer[4] != 0) {
                offset = pBuffer[4] - 1;
            } else {
                offset = BattleGetActorFlag41();
            }
            BattleStartScreenFade(pBuffer[3] >> 1, offset, pBuffer[0], pBuffer[1], pBuffer[2]);
            break;
        case 0xCB:
            pBuffer = (u8 *)(pArgs + SCRIPT_WORD_AT(pArgs));
            SpriteSetSVec(&vec, pBuffer[0], pBuffer[1], pBuffer[2]);
            BattleCreateTimedScaleTask(&vec, pBuffer[3]);
            break;
        case 0xE3:
            pTarget = pSpriteData->pTargetEntitySprite;
            pTarget->flags_0xAC.bytes[3] = BATTLE_ANIM_ID_DEFAULT;
            func_80023538(pTarget, (u8 *)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs));
            break;
        case 0xFB:
            BattleCreateActorDistanceWaitTask(pSpriteData, pArgs[2] << 1,
                                             (void *)((u8 *)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs)));
            break;
        case 0xC3:
        case 0xEC:
        case 0xF9:
            func_800B3F04(pSpriteData, pArgs[0], pArgs + 1);
            break;
        case 0x9D:
            if (((pSpriteData->field_0x40 >> 13) & BATTLE_ANIM_GROUP_MASK) == BATTLE_ANIM_GROUP_A) {
                pBuffer = (u8 *)&g_BattleSlotVectors;
            } else {
                pBuffer = (u8 *)&g_BattleSlotVectorsB;
            }
            ((s16 *)pBuffer)[0] = ((BattleSpriteState *)pSpriteData)->position.xWhole;
            ((s16 *)pBuffer)[1] = ((BattleSpriteState *)pSpriteData)->position.yWhole;
            ((s16 *)pBuffer)[2] = ((BattleSpriteState *)pSpriteData)->position.zWhole;
            break;
        case 0x99:
            pSpriteData->pBase->rotation.x = D_800D30B0.x;
            pSpriteData->pBase->rotation.y = D_800D30B0.y;
            pSpriteData->pBase->rotation.z = D_800D30B0.z;
            break;
        case 0x9A:
            vec.vx = D_800D30B8;
            vec.vy = 0;
            vec.vz = 0;
            RotMatrix((SVECTOR *)&pSpriteData->pBase->rotation, &mtx);
            goto animApplyRotation;
        case 0x9B:
            pSpriteData->scale = (D_80059454 << 12) / *(s32 *)&D_800D30B8;
            break;
        case 0x9C:
            vec.vx = (D_80059454 << 12) / pSpriteData->scale;
            vec.vy = 0;
            vec.vz = 0;
            RotMatrix((SVECTOR *)&D_800D30B0, &mtx);
        // Both rotation opcodes share this tail in retail, which is what keeps the
        // position-group addresses out of a loop-hoisted register.
        animApplyRotation:
            ApplyMatrix(&mtx, &vec, &out);
            if (((pSpriteData->field_0x40 >> 13) & BATTLE_ANIM_GROUP_MASK) == BATTLE_ANIM_GROUP_A) {
                pSpriteData->position.x = g_BattleAnimPositionGroupB.x - (out.vx << 16);
                pSpriteData->position.y = g_BattleAnimPositionGroupB.y - (out.vy << 16);
                pSpriteData->position.z = g_BattleAnimPositionGroupB.z - (out.vz << 16);
            }
            if (((pSpriteData->field_0x40 >> 13) & BATTLE_ANIM_GROUP_MASK) == BATTLE_ANIM_GROUP_B) {
                pSpriteData->position.x = g_BattleAnimPositionGroupA.x + (out.vx << 16);
                pSpriteData->position.y = g_BattleAnimPositionGroupA.y + (out.vy << 16);
                pSpriteData->position.z = g_BattleAnimPositionGroupA.z + (out.vz << 16);
            }
            break;
        case 0xC2:
            pTarget = pSpriteData->pTargetEntitySprite;
            pSpriteData->position.y = (pSpriteData->field_0x84 - 1) << 16;
            offset = (s8)pArgs[0] * pSpriteData->scale / 4096;
            if ((((u32)pSpriteData->flags_0xAC.flags >> 2) & 1) == 0) {
                offset = -offset;
            }
            ((BattleSpriteState *)pSpriteData)->targetPosition.x = ((BattleSpriteState *)pTarget)->position.xWhole + offset;
            ((BattleSpriteState *)pSpriteData)->targetPosition.y = 0;
            ((BattleSpriteState *)pSpriteData)->targetPosition.z = ((BattleSpriteState *)pTarget)->position.zWhole;
            BattleActorUpdateFallMotion(pSpriteData);
            break;
        case 0x97:
            pTarget = pSpriteData->pTargetEntitySprite;
            if (((BattleSpriteState *)pSpriteData)->position.yWhole < pSpriteData->field_0x84) {
                pSpriteData->animScriptWaitTimer = 1;
                return;
            }
            SpriteSetMoveSpeed(pSpriteData, 0);
            pointA[0] = ((BattleSpriteState *)pSpriteData)->position.xWhole;
            pointA[1] = ((BattleSpriteState *)pSpriteData)->position.zWhole;
            pointB[0] = ((BattleSpriteState *)pTarget)->position.xWhole;
            pointB[1] = ((BattleSpriteState *)pTarget)->position.zWhole;
            SpriteSetDirection(pSpriteData, SCRIPT_ANGLE(pointA, pointB));
            func_800223B0(pSpriteData, SCRIPT_ANGLE(pointA, pointB));
            SpriteSetDirection(pTarget, SCRIPT_ANGLE(pointB, pointA));
            func_800223B0(pTarget, SCRIPT_ANGLE(pointB, pointA));
            break;
        case 0xA4:
            pTarget = pSpriteData->pTargetEntitySprite;
            offset = (s8)pArgs[0];
            if (pTarget->pDefaultAnimFile == NULL) {
                BattleQueueActorCommandPreservingSelection(
                    (((u32)pTarget->flags_0xAC.flags & 3) << 2) | ((u32)pTarget->flags_0xA8 >> 30),
                    1 << ((((u32)pSpriteData->flags_0xAC.flags & 3) << 2) | ((u32)pSpriteData->flags_0xA8 >> 30)),
                    offset);
            } else {
                SpritePlayAnimation(pTarget, offset);
            }
            break;
        case 0x9E:
            pSpriteData->animScriptWaitTimer = 1;
            pWork = (u8 *)g_BattleStateWork;
            pWork += 0x8000;
            if ((*(u16 *)(pWork + 0xC58) & 0x100) == 0) {
                return;
            }
            break;
        case 0x95:
            pSpriteData->animScriptWaitTimer = 1;
            if (ArchiveDataSync() != 0) {
                return;
            }
            break;
        case 0x89:
            BattleActorUpdateFallMotion(pSpriteData);
            break;
        case 0x88:
            BattleActorComputeFallDirection(pSpriteData);
            break;
        case 0x8F:
            g_BattleAnimScriptHalted = 1;
            pSpriteData->pSpriteBytecode = NULL;
            mask = BATTLE_ANIM_MODE_HIGH_MASK;
            pSpriteData->flags_0xA8 &= mask;
            return;
        case 0xF8:
            count = (((u32)((SpriteData *)pSpriteData->pTargetEntitySprite)->flags_0xAC.flags & 3) << 2) |
                    ((u32)((SpriteData *)pSpriteData->pTargetEntitySprite)->flags_0xA8 >> 30);
            offset = g_BattleStateWork[(g_BattleCommandRowIndex - 1) * 0x48 + count + 0x150];
            switch (pArgs[2] & 0x7F) {
            case 0:
                count = offset == 0;
                break;
            case 1:
                count = offset == 1;
                break;
            case 2:
                count = offset == 2;
                break;
            case 3:
                count = offset == 3;
                break;
            case 4:
                count = offset == 4 || offset == 7;
                break;
            case 5:
                count = offset == 5;
                break;
            case 6:
                count = offset == 6;
                break;
            case 7:
                count = BattleFindTableEntryIndex((SpriteData *)pSpriteData->pTargetEntitySprite) + 1 <
                        g_BattleActorListCount;
                BattleRemapActorModelId(pSpriteData);
                break;
            case 8:
                count = D_800C4928 != 0;
                break;
            case 9:
                count = (SpriteData *)pSpriteData->pTargetEntitySprite == D_800C3E1C;
                break;
            default:
                count = 0;
                break;
            }
            if (pArgs[2] & 0x80) {
                count = count == 0;
            }
            if (count != 0) {
                pSpriteData->pSpriteBytecode =
                    (u8 *)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
                continue;
            }
            break;
        case 0xF3:
            if (pSpriteData->pBase->pFramesData != NULL) {
                GfxQueueSpriteTileData(pSpriteData->pBase->pFramesData);
            }
            offset = SCRIPT_U24_AT(pArgs);
            if (offset == 0) {
                pSpriteData->pBase->pFramesData = NULL;
                pSpriteData->pBase->pDirTransforms = NULL;
                break;
            }
            pFrames = pArgs + offset;
            size = BattleGetPrimitiveBufferSize(BattleGetFrame(pFrames, 0));
            pBuffer = HeapAlloc(size * 2, 0);
            if (pSpriteData->field_0x3A != 0) {
                BattleModelScaleParts(BattleGetFrame(pFrames, 0), 3);
            }
            BattleExpandModelPrims(BattleGetFrame(pFrames, 0), pBuffer, ((BattleSpriteState *)pSpriteData)->flags >> 29,
                                   pSpriteData->prim & 1);
            memcpy(pBuffer + size, pBuffer, size);
            pSpriteData->pBase->pFramesData = pBuffer;
            pSpriteData->pBase->pCurRenderFramesData = pBuffer + size;
            pSpriteData->pBase->pDirTransforms = (SpriteDirectionTransforms *)BattleGetFrame(pFrames, 0);
            break;
        case 0x8B:
            BattleUpdateCommandRows();
            break;
        case 0xBE:
            count = pArgs[0] | ((s8)pArgs[1] << 8);
            frames = ((count >> 11) & 0xF) + 1;
            frames = frames * (s32)(((u32)pSpriteData->flags_0xAC.flags >> BATTLE_ANIM_SPEED_SHIFT) & BATTLE_ANIM_SPEED_MASK) /
                     BATTLE_ANIM_SPEED_SCALE;
            if (frames == 0) {
                frames = 1;
            }
            offset = count & 0x1FF;
            if ((((BattleSpriteState *)pSpriteData)->flags & 3) != 1) {
                pSpriteData->curSpriteFrame = offset;
            } else {
                if (count < 0) {
                    if (offset != 0) {
                        offset = *(u8 *)(pSpriteData->field_0x60 + offset - 1);
                    }
                }
                pSpriteData->flags_0xAC.flags =
                    (pSpriteData->flags_0xAC.flags & ~8) | ((count >> 6) & 8);
                ((BattleSpriteState *)pSpriteData)->flags =
                    (((BattleSpriteState *)pSpriteData)->flags & ~8) |
                    ((((pSpriteData->flags_0xAC.flags >> 3) & 1) ^ ((pSpriteData->flags_0xAC.flags >> 2) & 1)) << 3);
                if ((count >> 10) & 1) {
                    ((BattleSpriteState *)pSpriteData)->flags |= 0x10;
                } else {
                    ((BattleSpriteState *)pSpriteData)->flags &= ~0x10;
                }
                SpriteAnimFrameListSetFrame(pSpriteData, offset);
            }
            pSpriteData->animScriptWaitTimer += frames;
            pSpriteData->pSpriteBytecode = (u8 *)pSpriteData->pSpriteBytecode + 3;
            return;
        case 0x8E:
            pSpriteData->pSpriteBytecode = NULL;
            return;
        case 0xE2:
            AnimScriptStackPushU24(pSpriteData, (s32)pSpriteData->pSpriteBytecode + 3);
            pSpriteData->pSpriteBytecode = (u8 *)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
            continue;
        case 0x85:
            pSpriteData->pSpriteBytecode = (void *)(((u32)pSpriteData->pSpriteBytecode & 0xFF000000) |
                                                    AnimScriptStackPopU24(pSpriteData));
            continue;
        case 0xFA:
            if (*(u8 *)AnimScriptResolveVariablePointer(pSpriteData, pArgs) != 0) {
                pSpriteData->pSpriteBytecode =
                    (u8 *)pSpriteData->pSpriteBytecode + ((s16)(pArgs[2] << 8) | pArgs[1]);
                continue;
            }
            break;
        case 0xD4:
            pSpriteData->pSpriteBytecode = (u8 *)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
            }
            continue;
        case 0x86:
            if (((BattleSpriteState *)pSpriteData)->step.y < 0) {
                pSpriteData->animScriptWaitTimer = 1;
                return;
            }
            break;
        case 0x87:
            if (((BattleSpriteState *)pSpriteData)->position.yWhole < pSpriteData->field_0x84) {
                pSpriteData->animScriptWaitTimer = 1;
                return;
            }
            break;
        case 0x80:
            mask = BATTLE_ANIM_MODE_HIGH_MASK;
            pSpriteData->flags_0xA8 &= mask;
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
                return;
            }
            if ((s8)pSpriteData->flags_0xB0.bytes[0] >= 0) {
                SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xB0.bytes[0]);
            }
            pSpriteData->flags_0xA8 &= mask;
            return;
        case 0x98:
            if (pSpriteData->field_0x70 == NULL) {
                break;
            }
            pSpriteData->animScriptWaitTimer = 1;
            if ((s8)((SpriteData *)pSpriteData->field_0x70)->flags_0xAC.bytes[3] != pSpriteData->field_0x8D) {
                break;
            }
            if ((((u32)((SpriteData *)pSpriteData->field_0x70)->flags_0xA8 >> 28) & 3) != 2) {
                break;
            }
            return;
        case 0x82:
            count = ((BattleSpriteState *)pSpriteData)->step.y;
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
            }
            SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xAC.bytes[3]);
            ((BattleSpriteState *)pSpriteData)->step.y = count;
            pSpriteData->animScriptWaitTimer = 0;
            func_800C11CC(pSpriteData);
            return;
        case 0x81:
            if ((s8)pSpriteData->flags_0xAC.bytes[3] == BATTLE_ANIM_ID_DEFAULT) {
                mask = BATTLE_ANIM_MODE_HIGH_MASK;
                pSpriteData->flags_0xA8 &= mask;
                if (pSpriteData->field_0x68 != NULL) {
                    ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
                    return;
                }
                if ((s8)pSpriteData->flags_0xB0.bytes[0] >= 0) {
                    SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xB0.bytes[0]);
                }
                pSpriteData->flags_0xA8 &= mask;
                return;
            }
            pSpriteData->animScriptWaitTimer = 0;
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
            }
            mask = BATTLE_ANIM_MODE_HIGH_MASK;
            pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & mask) | BATTLE_ANIM_MODE_HIGH_SET;
            return;
        case 0xE4:
            count = AnimScriptStackPopU8(pSpriteData);
            if (count != 0) {
                AnimScriptStackPushU8(pSpriteData, count - 1);
                pSpriteData->pSpriteBytecode =
                    (u8 *)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
                continue;
            }
            break;
        case 0xE1:
            pSpriteData->pSpriteBytecode = (u8 *)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
            continue;
        case 0xA7:
            pSpriteData->pSpriteBytecode = (u8 *)pSpriteData->pSpriteBytecode + g_AnimScriptCommandLengths[code];
            if (pArgs[0] & 0x80) {
                g_WorkListCurTimer = (pArgs[0] & 0x7F) + 1;
                pSpriteData->animScriptWaitTimer++;
                return;
            }
            count = (pArgs[0] + 2) *
                    (s32)(((u32)pSpriteData->flags_0xAC.flags >> BATTLE_ANIM_SPEED_SHIFT) & BATTLE_ANIM_SPEED_MASK) /
                    BATTLE_ANIM_SPEED_SCALE;
            if (count == 0) {
                count = 1;
            }
            pSpriteData->animScriptWaitTimer += count;
            return;
        case 0xC8:
            func_8001FBE4(pSpriteData, pArgs[0], AnimScriptResolveVariablePointer(pSpriteData, pArgs + 1));
            break;
        case 0x9F:
            break;
        default:
            func_8001FBE4(pSpriteData, code, pArgs);
            break;
        }
        pSpriteData->pSpriteBytecode = (u8 *)pSpriteData->pSpriteBytecode + g_AnimScriptCommandLengths[code];
    }
}
*/
