#include "common.h"
#include "psyq/libgpu.h"
#include "system/model.h"
#include "libgear/main.h"
#include "battle/actor.h"
#include "psyq/inline_c.h"

typedef struct {
    /* 0x0 */ u8* pBlocks;
    /* 0x4 */ u16 usedCount;
    /* 0x6 */ u16 capacity;
} BattleBlockPool; // Size: 0x8

typedef struct {
    /* 0x0 */ u8* pBlocks;
    /* 0x4 */ s16 count;
    /* 0x6 */ u16 unk6;
} BattleSpritePool; // Size: 0x8

typedef struct {
    /* 0x0 */ ModelPart** pParts;
    /* 0x4 */ u32 count;
} BattleModelPartList; // Size: 0x8

typedef struct {
    /* 0x0 */ u16 partIndex;
    /* 0x2 */ u16 parentIndex;
} BattleSkeletonEntry; // Size: 0x4

typedef struct {
    /* 0x00 */ u8 unk00[0x4];
    /* 0x04 */ u16 flags;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ u8 unk08[0x4];
    /* 0x0C */ u16 countA;
    /* 0x0E */ u16 countB;
    /* 0x10 */ u8 unk10[0x8];
} BattleAnimData; // Size: 0x18

// One entry of the battle part-slot table (stride 0x14): the position the
// battle scene lays a part out at, and the divisor its scale is spread over.
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u16 z;
    /* 0x06 */ s16 inUse;
    /* 0x08 */ u8 unk08[0xC];
} BattlePartSlot; // Size: 0x14

// One 0x7C slot of the sprite vertices BattleAllocSpriteSlot hands out: four
// corner coordinates, the interpolation step and the two primitives.
typedef struct {
    /* 0x00 */ u16 x0;
    /* 0x02 */ u16 y0;
    /* 0x04 */ u16 z0;
    /* 0x06 */ u16 pad06;
    /* 0x08 */ u16 x1;
    /* 0x0A */ u16 y1;
    /* 0x0C */ u16 z1;
    /* 0x0E */ s16 is3D; // 0 = the four corners are screen coordinates, 1 = they were projected
    /* 0x10 */ u16 x2;
    /* 0x12 */ u16 y2;
    /* 0x14 */ u16 z2;
    /* 0x16 */ s16 age; // the step the slot is at; 0 = just allocated
    /* 0x18 */ u16 x3;
    /* 0x1A */ u16 y3;
    /* 0x1C */ u16 z3;
    /* 0x1E */ s16 lifetime;
    /* 0x20 */ u16 red;
    /* 0x22 */ u16 green;
    /* 0x24 */ u16 blue;
    /* 0x26 */ s16 redDelta;
    /* 0x28 */ s16 greenDelta;
    /* 0x2A */ s16 blueDelta;
    /* 0x2C */ POLY_FT4 polys[2];
} BattleSpriteSlot; // Size: 0x7C

// One queued sprite tween, stride 0x70 of BattleActor.pEntries: the two model
// positions to interpolate between (start/end), the per-step coordinate history
// the projection writes, and the colour ramp.
typedef struct {
    /* 0x00 */ s16 spriteIndex; // index into pActor->pSprites; negative = idle record
    /* 0x02 */ u8 project3D; // non-zero = start/end are projected to 3D, otherwise to screen
    /* 0x03 */ u8 semiTrans;
    /* 0x04 */ void* pSpritePool; // passed to BattleAllocSpriteSlot
    /* 0x08 */ BattleSpriteSlot* pSlot; // the slot BattleAllocSpriteSlot handed out
    /* 0x0C */ SVECTOR start;
    /* 0x14 */ SVECTOR end;
    /* 0x1C */ DVECTOR projectedStart[8]; // where `start` projects at each of the eight step slots
    /* 0x3C */ DVECTOR projectedEnd[8];   // where `end` projects at each of the eight step slots
    /* 0x5C */ s16 slot; // which of the eight step slots this record is on
    /* 0x5E */ s16 step; // advanced per frame, runs to stepCount
    /* 0x60 */ s16 stepCount;
    /* 0x62 */ u16 lifetime;
    /* 0x64 */ u16 red;
    /* 0x66 */ u16 green;
    /* 0x68 */ u16 blue;
    /* 0x6A */ s16 redDelta;
    /* 0x6C */ s16 greenDelta;
    /* 0x6E */ s16 blueDelta;
} BattleSpriteTween; // Size: 0x70

// One 0x10-byte point of a model list entry: the source position and where its
// projection lands in the packet beside it.
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ s16 spriteIndex; // which sprite's matrix projects this point
    /* 0x8 */ u16 x;
    /* 0xA */ u16 y;
    /* 0xC */ u16 z;
    /* 0xE */ u16 pad0E;
} BattleModelListPoint; // Size: 0x10

// One 0x24-byte entry of BattleActor.pModelListEntries.
typedef struct {
    /* 0x00 */ s16 spriteIndex; // negative = idle entry
    /* 0x02 */ u8 unk02[0x2];
    /* 0x04 */ s16 pointCount;
    /* 0x06 */ u8 unk06[0x4];
    /* 0x0A */ s16 recordCount;
    /* 0x0C */ u8 unk0C[0x2];
    /* 0x0E */ u8 unk0E[0x2];
    /* 0x10 */ u8 unk10[0x4];
    /* 0x14 */ SVECTOR* pPoints;
    /* 0x18 */ BattleModelListPoint* pRecords;
    /* 0x1C */ void** pPackets;
    /* 0x20 */ u8 unk20[0x4];
} BattleModelListEntry; // Size: 0x24

// The 0x28-byte screen quad of BattleActor.unkB8: the tag, then four corners.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 unk6;
} BattleWarpCorner; // Size: 0x8

typedef struct {
    /* 0x00 */ u32 tag;
    /* 0x04 */ u32 code;
    /* 0x08 */ BattleWarpCorner corner[4];
} BattleWarpQuad; // Size: 0x28

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern void HeapChangeCurrentUser(u_int userTag, char** pContentTypes);
extern void BattleResetBlockPool(BattleBlockPool* pPool);
extern void BattleResetSpritePool(BattleSpritePool* pPool);
extern s32 BattleFreeBlock(BattleBlockPool* pPool, BattleBlock* pBlock);
extern u32 ModelResolvePointers(void* pModel);
extern void ModelPartFreeLightData(void* pLightData);
extern void ModelRenderPacket(void* pPart, void* pPacket, u_long* pOT, int flags);
extern void ModelAllocatePackets(ModelPart* pPart, void** ppPacket, void** ppPacketCopy);
extern void ModelInitializePacket(ModelPart* pPart, void* pPacket, s32 packetIndex);
extern void ModelPacketSetBaseTexPage(s16 x, s16 y);
extern void ModelPacketSetBaseClut(s16 x, s16 y);
extern void func_800A3E98(void* pRecord, s32 arg1);
extern void func_800A7948(void* pEntry, SVECTOR* pVec, MATRIX* pMatrix, u_long* pOt, s32 arg4, s32 arg5,
                          s32 arg6);
extern s32 BattleAllocSpriteSlot(void* pPool, s16 arg1);
extern BattlePartSlot g_BattlePartSlots[];
extern u8 g_BattleExtraMatrixFlag;
extern s32 g_GfxOTDepthShift;
extern s16 D_800D39E8;

BattleModelPartList* BattleInitializeMesh(u8* pModel, BattleModelPartList* pList) {
    u32 count;
    u32 i;

    HeapChangeCurrentUser(4, NULL);
    count = ModelResolvePointers(pModel);
    pList->pParts = HeapAlloc(count * 4, 0);
    pList->count = count;
    if (pList->pParts != NULL) {
        for (i = 0; i < count; i++) {
            pList->pParts[i] = (ModelPart*)(pModel + (0x10 + i * 0x38));
        }
    }
    return pList;
}
BattleSprite* BattleCreateModelSkeleton(BattleModelPartList* pMesh, BattleSkeletonEntry* pEntries, s32 packetIndex,
                            s32 hasTexture, s16 texX, s16 texY, s16 clutX, s16 clutY) {
    int i;
    int numSprites;
    u16 partIndex;
    u16 parentIndex;
    BattleSkeletonEntry* pEntry;
    BattleSprite* pSprites;
    BattleSprite* pSprite;

    HeapChangeCurrentUser(4, NULL);

    pEntry = pEntries;
    partIndex = pEntry->partIndex;
    numSprites = 0;
    while (partIndex < pMesh->count || partIndex == 0xFFFF) {
        pEntry++;
        partIndex = pEntry->partIndex;
        numSprites++;
    }

    if (numSprites == 0) {
        return NULL;
    }

    // Always account for the root sprite
    numSprites++;

    pSprites = HeapAlloc(numSprites * sizeof(BattleSprite), 0x0);
    if (pSprites == NULL) {
        return NULL;
    }

    pEntry = pEntries;
    pSprite = pSprites + 1;
    i = 1;
    partIndex = pEntry->partIndex;
    parentIndex = pEntry->parentIndex;

    // Root sprite
    pSprites[0].doTransform = 1;
    pSprites[0].doRotate = 1;
    pSprites[0].rotDirection = 1;
    pSprites[0].scale.vx = 0x1000;
    pSprites[0].scale.vy = 0x1000;
    pSprites[0].scale.vz = 0x1000;
    pSprites[0].pParent = NULL;
    pSprites[0].isVisible = 0;
    pSprites[0].jointIndex = 0xFFFF;
    pSprites[0].count = numSprites;
    pSprites[0].pModelPacketBuffer = NULL;
    pSprites[0].pCurModelPacket = NULL;
    pSprites[0].rot[0] = 0;
    pSprites[0].rot[1] = 0;
    pSprites[0].rot[2] = 0;
    pSprites[0].pos[0] = 0;
    pSprites[0].pos[1] = 0;
    pSprites[0].pos[2] = 0;
    pSprites[0].p70 = NULL;
    pSprites[0].p74 = NULL;
    pSprites[0].p78 = NULL;

    while (partIndex < pMesh->count || partIndex == 0xFFFF) {
        if (parentIndex == 0xFFFF) {
            pSprite->pParent = NULL;
        } else {
            pSprite->pParent = &pSprites[parentIndex] + 1;
        }

        pSprite->count = i++;
        pSprite->doTransform = 1;
        pSprite->doRotate = 1;
        pSprite->isVisible = 1;
        pSprite->scale.vx = 0x1000;
        pSprite->scale.vy = 0x1000;
        pSprite->scale.vz = 0x1000;
        pSprite->rotDirection = 0;
        pSprite->scale.pad = 0;
        pSprite->jointIndex = partIndex;

        if (partIndex != 0xFFFF) {
            ModelAllocatePackets(pMesh->pParts[partIndex], &pSprite->pModelPacketBuffer, &pSprite->pCurModelPacket);
            if (pSprite->pModelPacketBuffer == NULL) {
                BattleFreeModelSkeleton(pSprites);
                return NULL;
            }

            if (hasTexture) {
                ModelPacketSetBaseTexPage(texX, texY);
                ModelPacketSetBaseClut(clutX, clutY);
            }

            ModelInitializePacket(pMesh->pParts[partIndex], pSprite->pModelPacketBuffer, packetIndex);
            memcpy(pSprite->pCurModelPacket, pSprite->pModelPacketBuffer, pMesh->pParts[partIndex]->unk20[1].modelPacketSize);
        } else {
            pSprite->pModelPacketBuffer = NULL;
            pSprite->pCurModelPacket = NULL;
        }

        pSprite->rot[0] = 0;
        pSprite->rot[1] = 0;
        pSprite->rot[2] = 0;
        pSprite->pos[0] = 0;
        pSprite->pos[1] = 0;
        pSprite->pos[2] = 0;
        pSprite->p70 = NULL;
        pSprite->p74 = NULL;
        pSprite->p78 = NULL;

        pSprite++;
        pEntry++;

        partIndex = pEntry->partIndex;
        parentIndex = pEntry->parentIndex;
    }

    return pSprites;
}
s32 BattleUpdateSpriteTransforms(BattleSprite* pSprites, s32 scale) {
    MATRIX* pScaleMatrix;
    BattleSprite* pSprite;
    u32 count;
    u32 i;

    pSprite = pSprites;
    pScaleMatrix = (MATRIX*)PSX_SCRATCH;
    pSprite->globalTransform.t[0] = pSprite->pos[0];
    pSprite->globalTransform.t[1] = pSprite->pos[1];
    pSprite->globalTransform.t[2] = pSprite->pos[2];
    count = pSprite->count;
    if (pSprite->rotDirection != 0) {
        RotMatrixYXZ((SVECTOR*)pSprite->rot, &pSprite->globalTransform);
    } else {
        RotMatrix((SVECTOR*)pSprite->rot, &pSprite->globalTransform);
    }
    pScaleMatrix->m[0][0] = (scale * pSprites->scale.vx) >> 12;
    pScaleMatrix->m[0][1] = 0;
    pScaleMatrix->m[0][2] = 0;
    pScaleMatrix->m[1][0] = 0;
    pScaleMatrix->m[1][1] = (scale * pSprites->scale.vy) >> 12;
    pScaleMatrix->m[1][2] = 0;
    pScaleMatrix->m[2][0] = 0;
    pScaleMatrix->m[2][1] = 0;
    pScaleMatrix->m[2][2] = (scale * pSprites->scale.vz) >> 12;
    MulMatrix0(&pSprites->globalTransform, pScaleMatrix, &pSprites->localTransform);
    pSprites->localTransform.t[0] = pSprites->globalTransform.t[0];
    pSprites->localTransform.t[1] = pSprites->globalTransform.t[1];
    pSprites->localTransform.t[2] = pSprites->globalTransform.t[2];
    for (i = 1; i < count; i++) {
        pSprites++;
        if (pSprites->doRotate != 0) {
            if (pSprites->rotDirection != 0) {
                RotMatrixYXZ((SVECTOR*)pSprites->rot, &pSprites->localTransform);
            } else {
                RotMatrix((SVECTOR*)pSprites->rot, &pSprites->localTransform);
            }
            pSprites->doRotate = 0;
        }
        if (pSprites->pParent != NULL && *((u8*)pSprites->pParent + 4) == 1) {
            pSprites->doTransform = 1;
        }
        if (pSprites->doTransform != 0) {
            pSprites->localTransform.t[0] = pSprites->pos[0];
            pSprites->localTransform.t[1] = pSprites->pos[1];
            pSprites->localTransform.t[2] = pSprites->pos[2];
            if (pSprites->pParent != NULL) {
                CompMatrix((MATRIX*)((u8*)pSprites->pParent + 0x2C), &pSprites->localTransform,
                           &pSprites->globalTransform);
            } else {
                pSprites->globalTransform = pSprites->localTransform;
            }
        }
    }
    for (i = 1; i < count; i++) {
        pSprite++;
        pSprite->doTransform = 0;
    }
    return count;
}INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F1C4);
/* (992 bytes vs retail's 992; identical instruction multiset except the
   constant-1 pseudo). The structural read below is CONFIRMED - every call, branch
   and store matches retail; what is left is ONE allocator tie and the register
   permutation it drags along:
   - retail keeps the two flag constants (`li a3,1` before `bne a3` and again for
     the `sb`) in the CALLER-saved $a3, rematerialised at each of the four uses.
     We get ONE pseudo hoisted into the callee-saved $s8 before the loop, which
     costs the tenth callee-saved register and makes gcc spill $a0 (pSprites) to
     the frame: prologue `sw a0,16(sp); lw s1,16(sp)`, frame 0x40 where retail has
     0x38, and every later register shifted (ours pSprites=$s1/walker=$s17,
     retail walker=$s3/pSprites=$s6).
   - PROVEN: writing both compares as `if (x)` instead of `== 1` removes the
     constant pseudo and the result is EXACTLY retail's frame (0x38, no spill,
     identical epilogue) at 91.86%, but `!= 0` emits `beqz` where retail emits
     `li/bne`, so it can never reach byte equality - the `== 1` spelling is
     retail's and must stay.
   - Both permuter runs (base 2654 -> 1202 -> 1076/1116) only produced dead
     branches and an extra matrix pointer; the tie is an allocno-numbering
     question, i.e. the ORDER the pseudos appear in the source, which no shape
     tried so far moves.
   Facts worth keeping (each cost a round):
   - the scale matrix base is PSX_SCRATCH itself (retail's `lui $s0,0x1F80` with
     m[2][2] at +0x10), not PSX_SCRATCH+0x10.
   - the scale-matrix rows must be read through the WALKER pointer (`pSprite->scale.vx`),
     not pSprites: retail's `lh v0,76(s3)` uses the walker $s3 while the pos /
     globalTransform stores above use the pSprites copy $s6.
   - pParent is `void*` in include/battle/actor.h, so every parent access needs the
     `((BattleSprite*)pSprite->pParent)` cast; retail RELOADS pParent before each
     check (`lw v0,0(s3)` twice), so the source must not keep a pParent local.
   - the second loop clears doRotate BEFORE doTransform (`sb 0,1` then `sb 0,0`).
   - the inverse-scale matrix is `0x1000000 / pParent->scale.v*` (signed div; the
     unit's --expand-div turns it into the break 6/7 sequence retail has).
   - the whole body reuses one scratch MATRIX for all three matrices; retail
     rematerialises `lui $s1,0x1F80` inside the loop because the prologue's copy
     died (it is reused as the globalTransform walker).
   Round 185 (park repair, un-park + re-measure against today's headers): the body
   still COMPILES, but it now gives 87.12% / 952 bytes, not this note's 992 - ten
   instructions short. So the delta since round 118 is a missing statement block
   (start of the first loop: retail loads the three scale words, stores them at
   0x40/0x44/0x48 and does THREE mult/div matrix fills; ours diverges before the
   loop tail), not the constant-1 allocator tie described above. Re-derive from the
   .s before spending time on the tie.
   Round 195 (type-width sweep, un-parked and re-measured: still 87.12% / 952 bytes
   vs retail's 992 = 248 insns vs our 238). CONFIRMED it is a missing statement
   block, not a width: retail's pParent block materialises the flag constant 1 with
   a FRESH `ori a3,zero,1` at each of FOUR sites (before each `bne`, and again in
   the delay slot before each `sb`), all in the CALLER-saved $a3, while our compile
   CSEs all four into ONE pseudo and parks it in the CALLEE-saved $s7 - which costs
   the s8 save/restore pair (retail frames 9 callee-saved regs, we use 8) and two
   instructions at each of the two parent tests. Retail also re-materialises
   `lui s1,0x1F80` (scale-matrix base) INSIDE the loop where ours keeps it. Widths
   checked and all already retail's: count `lhu 0xA`, rotDirection `lbu 0x6`,
   scale `lh 0x4C/0x4E/0x50` in the pre-loop but `lhu 0x20/0x22/0x24` in the loop
   (two different access spellings of the same field), pParent `lw 0`,
   parent->doRotate/doTransform `lbu 5/4`, rot `lh 0x54`, pos `lw 0x5C/0x60/0x64`,
   matrix base is PSX_SCRATCH itself with m[2][2] at +0x10 (round 118 was right).
   Everything else in the two loops is instruction-for-instruction retail's; what is
   left is one constant-1 CSE plus the allocator permutation it drags. Not in scope
   for a width sweep - needs a shape that keeps four independent constant-1 pseudos.
   Parked body compiles (lint clean) at 87.12%.

s32 func_8009F1C4(BattleSprite* pSprites, s32 scale) {
    MATRIX* pScaleMatrix;
    BattleSprite* pSprite;
    u32 count;
    u32 i;

    pSprite = pSprites;
    pScaleMatrix = (MATRIX*)PSX_SCRATCH;
    pSprites->globalTransform.t[0] = pSprites->pos[0];
    pSprites->globalTransform.t[1] = pSprites->pos[1];
    pSprites->globalTransform.t[2] = pSprites->pos[2];
    count = pSprites->count;
    if (pSprites->rotDirection != 0) {
        RotMatrixYXZ((SVECTOR*)pSprites->rot, &pSprites->globalTransform);
    } else {
        RotMatrix((SVECTOR*)pSprites->rot, &pSprites->globalTransform);
    }
    pScaleMatrix->m[0][0] = (scale * pSprites->scale.vx) >> 12;
    pScaleMatrix->m[0][1] = 0;
    pScaleMatrix->m[0][2] = 0;
    pScaleMatrix->m[1][0] = 0;
    pScaleMatrix->m[1][1] = (scale * pSprites->scale.vy) >> 12;
    pScaleMatrix->m[1][2] = 0;
    pScaleMatrix->m[2][0] = 0;
    pScaleMatrix->m[2][1] = 0;
    pScaleMatrix->m[2][2] = (scale * pSprites->scale.vz) >> 12;
    MulMatrix0(&pSprites->globalTransform, pScaleMatrix, &pSprites->localTransform);
    pSprites->localTransform.t[0] = pSprites->globalTransform.t[0];
    pSprites->localTransform.t[1] = pSprites->globalTransform.t[1];
    pSprites->localTransform.t[2] = pSprites->globalTransform.t[2];
    for (i = 1; i < count; i++) {
        pSprites++;
        if (pSprite->pParent != NULL) {
            if (((BattleSprite*)pSprite->pParent)->doRotate == 1) {
                pSprite->doRotate = 1;
            }
            if (((BattleSprite*)pSprite->pParent)->doTransform == 1) {
                pSprite->doTransform = 1;
            }
        }
        if (pSprite->doRotate != 0) {
            if (pSprite->rotDirection != 0) {
                RotMatrixYXZ((SVECTOR*)pSprite->rot, &pSprite->localTransform);
            } else {
                RotMatrix((SVECTOR*)pSprite->rot, &pSprite->localTransform);
            }
            pScaleMatrix->m[0][0] = pSprite->scale.vx;
            pScaleMatrix->m[0][1] = 0;
            pScaleMatrix->m[0][2] = 0;
            pScaleMatrix->m[1][0] = 0;
            pScaleMatrix->m[1][1] = pSprite->scale.vy;
            pScaleMatrix->m[1][2] = 0;
            pScaleMatrix->m[2][0] = 0;
            pScaleMatrix->m[2][1] = 0;
            pScaleMatrix->m[2][2] = pSprite->scale.vz;
            MulMatrix0(&pSprite->localTransform, pScaleMatrix, &pSprite->localTransform);
            if (pSprite->pParent != NULL) {
                pScaleMatrix->m[0][0] = 0x1000000 / ((BattleSprite*)pSprite->pParent)->scale.vx;
                pScaleMatrix->m[0][1] = 0;
                pScaleMatrix->m[0][2] = 0;
                pScaleMatrix->m[1][0] = 0;
                pScaleMatrix->m[1][1] = 0x1000000 / ((BattleSprite*)pSprite->pParent)->scale.vy;
                pScaleMatrix->m[1][2] = 0;
                pScaleMatrix->m[2][0] = 0;
                pScaleMatrix->m[2][1] = 0;
                pScaleMatrix->m[2][2] = 0x1000000 / ((BattleSprite*)pSprite->pParent)->scale.vz;
                MulMatrix0(pScaleMatrix, &pSprite->localTransform, &pSprite->localTransform);
            }
        }
        if (pSprite->doTransform != 0) {
            pSprite->localTransform.t[0] = pSprite->pos[0];
            pSprite->localTransform.t[1] = pSprite->pos[1];
            pSprite->localTransform.t[2] = pSprite->pos[2];
            if (pSprite->pParent != NULL) {
                CompMatrix(&((BattleSprite*)pSprite->pParent)->globalTransform, &pSprite->localTransform,
                           &pSprite->globalTransform);
            } else {
                pSprite->globalTransform = pSprite->localTransform;
            }
        }
    }
    pSprite = pSprites;
    for (i = 1; i < count; i++) {
        pSprite++;
        pSprite->doRotate = 0;
        pSprite->doTransform = 0;
    }
    return count;
}
*/
void BattleNoOp(void) {
}
void BattleRenderModel(BattleModelPartList* pMesh, BattleSprite* pSkeleton, MATRIX* pLightMatrix,
                   MATRIX* pViewMatrix, s32 arg4, s32 arg5, s32 packetIndex) {
    MATRIX* pRotMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x20);
    MATRIX* pTransMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x40);
    MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
    u_int numChildren;
    u_int i;

    MulMatrix0(pViewMatrix, &pSkeleton->globalTransform, pRotMatrix);
    CompMatrix(pLightMatrix, &pSkeleton->localTransform, pTransMatrix);

    numChildren = pSkeleton->count;
    pSkeleton++;

    for (i = 1; i < numChildren; i++) {
        if (pSkeleton->jointIndex != 0xFFFF) {
            void* pPart;
            void* pPacket;

            MulMatrix0(pRotMatrix, &pSkeleton->globalTransform, pMatrix);
            SetLightMatrix(pMatrix);
            CompMatrix(pTransMatrix, &pSkeleton->globalTransform, pMatrix);
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);

            pPart = pMesh->pParts[pSkeleton->jointIndex];
            pPacket = ((void**)&pSkeleton->pModelPacketBuffer)[packetIndex];
            ModelRenderPacket(pPart, pPacket, (u_long*)arg5, arg4);
        }
        pSkeleton++;
    }
}
void BattleFreeModelSkeleton(BattleSprite* pSprites) {
    BattleSprite* pSprite;
    s32 i;

    if (pSprites != NULL) {
        pSprite = pSprites;
        for (i = 0; i < pSprites->count; i++, pSprite++) {
            if (pSprite->pModelPacketBuffer != NULL) {
                HeapFree(pSprite->pModelPacketBuffer);
                pSprite->pModelPacketBuffer = NULL;
                pSprite->pCurModelPacket = NULL;
            }
        }
        pSprites->count = 0;
        HeapFree(pSprites);
    }
}
void BattleFreeModelMesh(BattleModelPartList* pList, s32 freeLight) {
    u32 i;

    if (pList != NULL) {
        for (i = 0; i < pList->count; i++) {
            if (pList->pParts != NULL && pList->pParts[i] != NULL && freeLight != 0) {
                ModelPartFreeLightData(pList->pParts[i]);
            }
        }
        if (pList->pParts != NULL) {
            HeapFree(pList->pParts);
            pList->pParts = NULL;
        }
    }
}
/* The stack layout is the function's shape: the memory locals are declared in
   first-use order, and macPoint/tweenVec sit in a nested block so that depth and
   sz (whose address gte_stszotz takes) land below them - without the block every
   sp slot from 0x48 up shifts and ~20 instructions differ.
   Load-bearing spellings, each measured: the tween loop is
   `if (pTween->spriteIndex >= 0) { ... } pTween++;`, so the `++step == 0` continue
   skips the bump exactly as retail does; each tween arm carries its own
   allocate:/done: labels; the 2D arm is the THEN arm and the 3D arm the else;
   pSprite and pSprites are two live copies (walker at sp+0x90, root at sp+0x98)
   and the sprite loop bumps the walker in memory; `(k + pTween->projectedStart)`
   is the only spelling that gives retail's index-first addu.
   Residual: the slot scale is read through BattlePartSlot
   (g_BattlePartSlots[i].x), where retail's object names the field addresses
   D_800D3306/3308/330A, so objdiff reads 99.96%; the linked bytes are identical
   (tools/scripts/check_bin.sh battle: OK). */
void func_8009F844(BattleActor* pActor, MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 flags,
                   s32 texParam, u_long* pOt, s32 quadIndex) {
    u32 count;
    s32 i;
    s32 j;
    s32 k;
    BattleModelPartList* pMesh;
    BattleSprite* pSprite;
    BattleSprite* pSprites;
    Temp8* pTexture;
    u16 scale16;
    MATRIX* pRotMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x20);
    MATRIX* pTransMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x40);
    s32 scale;
    MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
    BattleModelListEntry* pModelList;
    BattleModelListPoint* pPoint;
    BattleSpriteTween* pTween;
    SVECTOR vec;
    s32 macStart[4];
    s32 macEnd[4];
    s32 depth;
    s32 sz;
    s32 shade;

    if (pActor->doUpdate == 0) {
        return;
    }
    pMesh = pActor->pMesh;
    scale16 = pActor->scale;
    pSprite = pActor->pSprites;
    scale = scale16;
    pSprites = pSprite;
    count = pSprites->count;
    for (i = 0; i < 2; i++) {
        if (g_BattlePartSlots[i].inUse != 0) {
            s16 dx = g_BattlePartSlots[i].x - ((u16*)&pSprite->pos[0])[0];
            s16 dy = g_BattlePartSlots[i].y -
                     ((pSprite->pos[1] - pActor->height * (scale * pSprite->scale.vy >> 12)) >> 13);
            s16 dz = g_BattlePartSlots[i].z - ((u16*)&pSprite->pos[0])[4];
            s32 denominator = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;

            pViewMatrix->m[i + 1][0] =
                (dx << 12) / denominator * g_BattlePartSlots[i].inUse / (g_BattlePartSlots[i].inUse + denominator);
            pViewMatrix->m[i + 1][1] =
                (dy << 12) / denominator * g_BattlePartSlots[i].inUse / (g_BattlePartSlots[i].inUse + denominator);
            pViewMatrix->m[i + 1][2] =
                (dz << 12) / denominator * g_BattlePartSlots[i].inUse / (g_BattlePartSlots[i].inUse + denominator);
        } else {
            pViewMatrix->m[i + 1][0] = 0;
            pViewMatrix->m[i + 1][1] = 0;
            pViewMatrix->m[i + 1][2] = 0;
        }
    }
    CompMatrix(pLightMatrix, &pSprite->localTransform, pTransMatrix);
    if ((pActor->flags4A & 1) == 0 && g_BattleExtraMatrixFlag != 0) {
        CompMatrix(&pSprite->localTransform, &pActor->pSprites[1].globalTransform, pMatrix);
        SetRotMatrix(pMatrix);
        SetTransMatrix(pMatrix);
        vec.vx = 0;
        vec.vy = 0;
        vec.vz = 0x1000;
        gte_ldv0(&vec);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(macStart);
        vec.vx = 0;
        vec.vy = 0;
        vec.vz = 0;
        gte_ldv0(&vec);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(macEnd);
        vec.vy = -ratan2(macStart[2] - macEnd[2], macStart[0] - macEnd[0]);
        vec.vx = 0;
        vec.vz = 0;
        RotMatrix(&vec, pRotMatrix);
        pRotMatrix->t[0] = macEnd[0];
        pRotMatrix->t[1] = pActor->groundY;
        pRotMatrix->t[2] = macEnd[2];
        CompMatrix(pLightMatrix, pRotMatrix, pRotMatrix);
        shade = pActor->scale - (pActor->groundY - pActor->pSprites->pos[1]) / 4;
        if (shade < 0) {
            shade = 0;
        }
        pMatrix->m[0][0] = shade;
        pMatrix->m[0][1] = 0;
        pMatrix->m[0][2] = 0;
        pMatrix->m[1][0] = 0;
        pMatrix->m[1][1] = shade;
        pMatrix->m[1][2] = 0;
        pMatrix->m[2][0] = 0;
        pMatrix->m[2][1] = 0;
        pMatrix->m[2][2] = shade;
        MulMatrix(pRotMatrix, pMatrix);
        SetRotMatrix(pRotMatrix);
        SetTransMatrix(pRotMatrix);
        vec.vx = pActor->width;
        vec.vy = 0;
        vec.vz = pActor->depth;
        gte_ldv0(&vec);
        gte_rtps();
        gte_stsxy2(&((BattleWarpQuad*)pActor->unkB8)[quadIndex].corner[0]);
        gte_stszotz(&depth);
        vec.vx = -pActor->width;
        gte_ldv0(&vec);
        gte_rtps();
        gte_stsxy2(&((BattleWarpQuad*)pActor->unkB8)[quadIndex].corner[1]);
        gte_stszotz(&sz);
        if (sz < depth) {
            depth = sz;
        }
        vec.vx = pActor->width;
        vec.vz = -pActor->depth;
        gte_ldv0(&vec);
        gte_rtps();
        gte_stsxy2(&((BattleWarpQuad*)pActor->unkB8)[quadIndex].corner[2]);
        gte_stszotz(&sz);
        if (sz < depth) {
            depth = sz;
        }
        vec.vx = -pActor->width;
        gte_ldv0(&vec);
        gte_rtps();
        gte_stsxy2(&((BattleWarpQuad*)pActor->unkB8)[quadIndex].corner[3]);
        gte_stszotz(&sz);
        if (sz < depth) {
            depth = sz;
        }
        depth >>= g_GfxOTDepthShift;
        addPrim(&pOt[depth], &((BattleWarpQuad*)pActor->unkB8)[quadIndex]);
        if (depth > 0x2D8) {
            depth = 0x2D8;
        }
        pActor->unk39 = 0x6B - depth / 8;
    }
    MulMatrix0(pViewMatrix, &pSprite->globalTransform, pRotMatrix);
    pSprite++;
    for (i = 1; i < count; i++, pSprite++) {
        if (pSprite->jointIndex != 0xFFFF && pSprite->isVisible != 0) {
            MulMatrix0(pRotMatrix, &pSprite->globalTransform, pMatrix);
            SetLightMatrix(pMatrix);
            CompMatrix(pTransMatrix, &pSprite->globalTransform, pMatrix);
            if (pSprite->scale.pad > 0) {
                pMatrix->m[0][0] = *(u16*)&pActor->scale;
                pMatrix->m[0][2] = 0;
                pMatrix->m[1][0] = 0;
                pMatrix->m[1][2] = 0;
                pMatrix->m[2][0] = 0;
                pMatrix->m[2][2] = *(u16*)&pActor->scale;
                if (pSprite->scale.pad == 1) {
                    pMatrix->m[0][1] = pTransMatrix->m[0][1];
                    pMatrix->m[1][1] = pTransMatrix->m[1][1];
                    pMatrix->m[2][1] = pTransMatrix->m[2][1];
                } else {
                    pMatrix->m[0][1] = 0;
                    pMatrix->m[1][1] = *(u16*)&pActor->scale;
                    pMatrix->m[2][1] = 0;
                }
            }
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            ModelRenderPacket(pMesh->pParts[pSprite->jointIndex], ((void**)&pSprite->pModelPacketBuffer)[quadIndex],
                              pOt, flags);
        }
    }
    {
        s32 macPoint[4];
        SVECTOR tweenVec;

        pModelList = (BattleModelListEntry*)pActor->pModelListEntries;
        for (i = 0; i < pActor->numModelListEntries; pModelList++, i++) {
            if (pModelList->spriteIndex < 0) {
                continue;
            }
            tweenVec.vx = -D_800D39E8 * rsin(pSprites->rot[1] + 0x400) / 0x1000;
            tweenVec.vz = D_800D39E8 * rcos(pSprites->rot[1] + 0x400) / 0x1000;
            tweenVec.vy = pActor->unk3E;
            CompMatrix(&pSprites->localTransform, &pSprites[pModelList->spriteIndex].globalTransform, pMatrix);
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            for (j = 0; j < pModelList->pointCount; j++) {
                gte_ldv0(&pModelList->pPoints[j]);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(macPoint);
                ((u16*)((void**)pModelList->pPackets)[j])[2] = ((u16*)macPoint)[0];
                ((u16*)((void**)pModelList->pPackets)[j])[3] = ((u16*)macPoint)[2];
                ((u16*)((void**)pModelList->pPackets)[j])[4] = ((u16*)macPoint)[4];
            }
            pPoint = pModelList->pRecords;
            for (j = 0; j < pModelList->recordCount; j++, pPoint++) {
                CompMatrix(&pSprites->localTransform, &pSprites[pPoint->spriteIndex].globalTransform, pMatrix);
                SetRotMatrix(pMatrix);
                SetTransMatrix(pMatrix);
                gte_ldv0((SVECTOR*)&pPoint->vx);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(macPoint);
                pPoint->x = ((u16*)macPoint)[0];
                pPoint->y = ((u16*)macPoint)[2];
                pPoint->z = ((u16*)macPoint)[4];
            }
            func_800A7948(pModelList, &tweenVec, pLightMatrix, pOt, quadIndex, scale16, pActor->groundY);
        }
    }
    pTexture = pActor->pTextures;
    for (i = 0; i < pActor->numTextures; i++, pTexture++) {
        func_800A3E98(pTexture, texParam);
    }
    pTween = (BattleSpriteTween*)pActor->pEntries;
    for (i = 0; i < pActor->numEntries; i++) {
        if (pTween->spriteIndex >= 0) {
            pTween->slot = (pTween->slot - 1) & 7;
            if (pTween->project3D == 0) {
                CompMatrix(pTransMatrix, &pSprites[pTween->spriteIndex].globalTransform, pMatrix);
                SetRotMatrix(pMatrix);
                SetTransMatrix(pMatrix);
                gte_ldv0(&pTween->start);
                gte_rtps();
                gte_stsxy2(&pTween->projectedStart[pTween->slot]);
                gte_ldv0(&pTween->end);
                gte_rtps();
                gte_stsxy2(&pTween->projectedEnd[pTween->slot]);
                if (++pTween->step == 0) {
                    continue;
                }
                if (pTween->step > pTween->stepCount) {
                    goto allocate;
                }
                if (pTween->pSlot != NULL) {
                    goto done;
                }
            allocate:
                pTween->step = 1;
                pTween->pSlot = (BattleSpriteSlot*)BattleAllocSpriteSlot(pTween->pSpritePool, pTween->semiTrans);
                pTween->pSlot->is3D = 0;
                pTween->pSlot->age = 0;
                pTween->pSlot->lifetime = pTween->lifetime;
                pTween->pSlot->red = pTween->red;
                pTween->pSlot->green = pTween->green;
                pTween->pSlot->blue = pTween->blue;
                pTween->pSlot->redDelta = pTween->redDelta;
                pTween->pSlot->greenDelta = pTween->greenDelta;
                pTween->pSlot->blueDelta = pTween->blueDelta;
                k = pTween->slot;
                k += pTween->step;
                k &= 7;
                pTween->pSlot->x0 = (k + pTween->projectedStart)->vx;
                pTween->pSlot->y0 = (k + pTween->projectedStart)->vy;
                pTween->pSlot->x2 = (k + pTween->projectedEnd)->vx;
                pTween->pSlot->y2 = (k + pTween->projectedEnd)->vy;
            done:
                pTween->pSlot->x1 = (pTween->slot + pTween->projectedStart)->vx;
                pTween->pSlot->y1 = (pTween->slot + pTween->projectedStart)->vy;
                pTween->pSlot->x3 = (pTween->slot + pTween->projectedEnd)->vx;
                pTween->pSlot->y3 = (pTween->slot + pTween->projectedEnd)->vy;
            } else {
            CompMatrix(&pSprites->localTransform, &pSprites[pTween->spriteIndex].globalTransform, pMatrix);
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            {
                s32 side = pTween->slot & 1;
                s32* pStart = (s32*)&pTween->projectedStart[side * 4];
                s32* pEnd = (s32*)&pTween->projectedEnd[side * 4];

                gte_ldv0(&pTween->start);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(pStart);
                gte_ldv0(&pTween->end);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(pEnd);
                if (++pTween->step == 0) {
                    continue;
                }
                if (pTween->step > pTween->stepCount) {
                    goto allocate3D;
                }
                if (pTween->pSlot != NULL) {
                    goto done3D;
                }
            allocate3D:
                pTween->step = 1;
                pTween->pSlot = (BattleSpriteSlot*)BattleAllocSpriteSlot(pTween->pSpritePool, pTween->semiTrans);
                pTween->pSlot->is3D = 1;
                pTween->pSlot->age = 0;
                pTween->pSlot->lifetime = pTween->lifetime;
                pTween->pSlot->red = pTween->red;
                pTween->pSlot->green = pTween->green;
                pTween->pSlot->blue = pTween->blue;
                pTween->pSlot->redDelta = pTween->redDelta;
                pTween->pSlot->greenDelta = pTween->greenDelta;
                pTween->pSlot->blueDelta = pTween->blueDelta;
                {
                    s32 other = 1 - side;
                    s32* pFrom = (s32*)&pTween->projectedStart[other * 4];
                    s32* pTo = (s32*)&pTween->projectedEnd[other * 4];

                    pTween->pSlot->x0 = ((u16*)pFrom)[0];
                    pTween->pSlot->y0 = ((u16*)pFrom)[2];
                    pTween->pSlot->z0 = ((u16*)pFrom)[4];
                    pTween->pSlot->x2 = ((u16*)pTo)[0];
                    pTween->pSlot->y2 = ((u16*)pTo)[2];
                    pTween->pSlot->z2 = ((u16*)pTo)[4];
                }
            done3D:
                pTween->pSlot->x1 = ((u16*)pStart)[0];
                pTween->pSlot->y1 = ((u16*)pStart)[2];
                pTween->pSlot->z1 = ((u16*)pStart)[4];
                pTween->pSlot->x3 = ((u16*)pEnd)[0];
                pTween->pSlot->y3 = ((u16*)pEnd)[2];
                pTween->pSlot->z3 = ((u16*)pEnd)[4];
            }
            }
        }
        pTween++;
    }
}
/* func_800A0838: 1238 insns, 8 calls (BattleFreeBlock x3, SquareRoot0 x2,
   ratan2 x2, ApplyMatrix x1), 127 internal labels and 166 memory writes - a
   state machine over the actor's queued blocks (include/battle/actor.h names
   BattleActor.blockTag 0x3C as "func_800A0838 compares it with each queued
   block's tag and sets that block's result bits"). It OWNS both of this unit's
   .rodata tables, which start at the unit's rodata offset 0xA24:
   jtbl_80070514 (9 case words + a trailing zero word, which is the .align 3 pad
   of the NEXT table, so it is reproducible) and jtbl_8007053C at +0x28 (6
   words). Convert it as C with real switch statements and judge with
   check_bin.sh battle, not diff_func. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A0838);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1B50);
/*
   `andi` is the only non-multiset insn). The remaining delta is register
   identity: pFrame sits in $v1 where retail coalesces it into $a1 (the pData
   parameter register, `addiu a1,a1,0x18` in the bnez delay slot), which drags
   the third rot temp and the unk06 temp along with it.
   Shapes tried: every statement order of the four field reads (with
   and without a unk06 local), countA inline vs a local, and both pointer/walker
   forms. Facts this cost a round to establish, keep them:
   - `pFrame += (countA + 1) * 3` MUST go through a temp: the temp is what puts
     `move t1,zero` back at the top of the prologue (94.95 -> 96.05).
   - the sprite pointer MUST be pre-incremented at the TOP of the body
     (`pSprite = pSprites; for (...) { pSprite++; ... }`): that is what makes
     retail's 0x7C increment sit in the loop-head branch delay slot and the
     andi/move stay in the tail's.
   - there MUST be no `if (limit <= 0) return;`: retail's `andi; blez; move
     t0,zero` entry test IS the `for` loop's own entry test.
   - records are read with `*pFrame++` (stride 2), and the sprite base is
     rebased to sprite+4 (doTransform) by gcc, as retail's 0x50/0x58/0x6C
     displacements show.
   permuter: 2 runs / 1364 iterations, base 555 best 555 - it cannot break the
   load-order tie either.
   Round 63: both bodies are 420 bytes (the note's "424" is stale) and the real
   delta is register identity, not just order:
   - retail walks the frame in $a1 (the *pData parameter register itself*:
     `addiu a1,a1,24` in the bnez delay slot) and uses $a3/$a2/$v1 as the three
     rot temps; ours gives pFrame=$v1 and the third temp=$a1 - a straight swap.
     So the tie is pFrame-vs-third-temp allocation, and the `lh t0,6(a1)` order
     follows from it.
   - retail's `bnez v0,loop` delay slot repeats a DEAD `andi v0,t3,0xffff`
     (gcc emits `nop` there in ours); that is the only non-multiset insn left.
   - moving the `pFrame = (s16*)(pData + 1)` statement after the four field
     loads changes nothing (same 96.05%), so statement order is not the lever.
   Nothing hand-editable is left; this is a -O2 allocator/scheduler shape. Next
   round should spend the permuter budget (`--minutes 4 -j 2` twice did nothing)
   or bias the allocator with a differently-shaped frame walker, not text order.
   Round 64: a `s32 skipFirst = (unk06 == 0);` local assigned right after the
   unk06 load and tested by the if took it from 96.05 to 97.90 (the body below).
   That is now the only delta left: pFrame still lands in $v1 where retail keeps
   it in $a1 (the parameter register, coalesced), and the unk06 temp follows it
   ($v0 vs retail's $t0), plus retail's dead `andi v0,t3,0xffff` in the loop-head
   bnez delay slot where we emit nop. Permuter on top of the 97.90 body: base 360
   best 360 (646 iterations, 3 min).
   Round 118: the parked body was never compiling (it spelled the block tag
   `pBlock->tag`, and the header's BattleBlock has `flags` as a scalar). Fixed to
   `pBlock->tag` -> 97.90%, still 420 bytes = retail's 420 and 105 insns each, so the
   delta is exactly: retail's dead `andi v0,t3,0xffff` copied into the loop-head bnez
   delay slot (we emit `nop`) and the prologue register permutation
   (retail pFrame=$a1 / 3rd rot temp=$v1 / unk06=$t0, ours $v1/$a1/$v0). The
   permutation is source-invisible: gcc only coalesces the +24 into $a1 when the
   walker's add keeps the copy preference, and nothing in the body changes it.
   Same 97.90% under gcc-2.7.2 -O2 -G8 (try_flags), so it is not a preset question.
   Round 164: still 97.90% / 420 bytes = retail's, and three more spellings of the
   frame pointer (`pData++` then `pFrame = (s16*)pData`, and `pFrame = (s16*)pData;
   pFrame += 12;`) all leave pFrame in $v1 - the copy/add is folded straight into
   one `addiu v1,a1,24` before any coalescing can tie it to $a1. This is the
   permuter's job, not a spelling.
   Round 185 (park repair): still compiles, still 97.90% / 420 bytes = retail's 420.
   The two deltas above are the whole remainder; unchanged.
   Round 195 (type-width sweep, per the round-194 rule): every local whose width
   feeds a load/store was re-measured against retail's own load widths -
   s16 v0/v1/v2 -> 73.31% / 476 bytes (they MUST be s32: the pos half stores `sw`),
   u32 numA/numB -> 95.90% / 412 (retail's `andi 0xffff / sltu` needs u16),
   u16 unk06 -> 95.88% / 420 (retail loads it `lh`, so s16),
   s32 countA/countB -> 96.76% / 420 (retail loads them `lhu`).
   All four are worse than the 97.90% body, so the widths are already retail's and
   the remainder is the allocno tie + the dead andi alone - not a width question.
   Third permuter run deliberately skipped: two runs (555/555 and 360/360 base==best)
   already plateaued on this exact body.

void func_800A1B50(BattleSprite* pSprites, BattleAnimData* pData) {
    s16* pFrame;
    BattleSprite* pSprite;
    BattleBlock* pBlock;
    u16 numA;
    u16 numB;
    u16 countA;
    u16 countB;
    u16 animFlags;
    u16 numSprites;
    s16 unk06;
    s32 skipFirst;
    s32 i;
    s32 v0;
    s32 v1;
    s32 v2;

    numA = 0;
    numB = 0;
    unk06 = pData->unk06;
    skipFirst = unk06 == 0;
    countA = pData->countA;
    countB = pData->countB;
    animFlags = pData->flags;
    pFrame = (s16*)(pData + 1);
    if (skipFirst) {
        v2 = countA + 1;
        pFrame += v2 * 3;
    }
    numSprites = pSprites->count - 1;
    pSprite = pSprites;
    for (i = 0; i < numSprites; i++) {
        pSprite++;
        if (!(animFlags & 1) && numA < countA) {
            v0 = *pFrame++;
            v1 = *pFrame++;
            v2 = *pFrame++;
            numA++;
            if (pSprite->rot[0] != v0 || pSprite->rot[1] != v1 || pSprite->rot[2] != v2) {
                pBlock = pSprite->p70;
                if (pBlock == NULL || pBlock->tag != 0xFF) {
                    pSprite->rot[0] = v0;
                    pSprite->rot[1] = v1;
                    pSprite->rot[2] = v2;
                    pSprite->doTransform = 1;
                    pSprite->doRotate = 1;
                }
            }
        }
        if (!(animFlags & 2) && numB < countB) {
            v0 = *pFrame++;
            v1 = *pFrame++;
            v2 = *pFrame++;
            numB++;
            if (pSprite->pos[0] != v0 || pSprite->pos[1] != v1 || pSprite->pos[2] != v2) {
                pBlock = pSprite->p74;
                if (pBlock == NULL || pBlock->tag != 0xFF) {
                    pSprite->pos[0] = v0;
                    pSprite->pos[1] = v1;
                    pSprite->pos[2] = v2;
                    pSprite->doTransform = 1;
                }
            }
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1CF4);
/* 86.05% (1136 bytes vs retail's 1144; the structure below is CONFIRMED - every call,
   branch and store of retail has a counterpart, and every branch target in our compile is
   one of retail's own targets). What is left is two things: an allocno permutation and a
   2-instruction skeleton difference.
   FUNCTION (decoded, keep it): walks sprites 1..count-1 and queues one rot tween block
   (p70) and one pos tween block (p74) per sprite from a 6-byte-per-sprite frame record
   (three s16 per half). Half rot: skips when its anim flag bit is set, when its counter
   reached countA/countB, or when all three frame values already equal the sprite's; else it
   takes/reuses the sprite's block (alloc if the slot is empty, skip entirely when the queued
   block's tag is the 0xFF marker), fills inUse/flags/opcode/tag, unk4/6/8 = the sprite's
   current rot (or pos), unkA/C/E = the wrapped delta (pos half: delta, or the target itself
   when the "absolute" flag is set), unk10 = 0, unk12 = frames; otherwise it FREES the queued
   block. That free tail is the same block reached from all four of retail's edges.
   Deltas left, in order of how many instructions they cost:
   1. ALLOCNO PERMUTATION (most of the remaining score). Retail: pPool=$s7, pFrame=$s1,
      v0/v1/v2=$s2/$s3/$s4, absolute=$s5, i=$s6, 0xFF=$s8. Ours: pPool=$s5, pFrame=$s4,
      v0/v1/v2=$s1/$s2/$s3, absolute=$s6, i=$s7, 0xFF=$s8 (walker $s0 in both). So retail
      ranks pFrame ABOVE the three temps and pPool above everything except $s8; ours ranks the
      temps above pFrame. Every source shape tried (permuter included) keeps that order.
   2. Our reloads of the three stack arguments are WORD loads where retail narrows them:
      retail `lbu 0x8C(sp)` / `lbu 0x90(sp)` / `lhu 0x10(sp)` at both init sites and both
      unk12 stores, ours `lw` (6 opcodes, same instruction count). Declaring blockFlags/tag
      u8 instead of u32 costs a 16-byte frame (gcc makes a register copy of the stack arg),
      so u32 is right - the narrow RELOAD is what we cannot reproduce.
   3. Two instructions short: retail's block selection is `lw v0,-4(s0); beqz v0,ALLOC;
      addu a0,v0,zero` (the copy in the delay slot) then `lbu v0,3(a0); beq v0,fp,DONE` then
      `j INIT`, and the shared INIT starts `beqz a0,FREE` - i.e. the free tail is the ELSE of
      the init test, so an alloc failure goes to it. Written as one
      `if (pBlock != NULL && pBlock->tag != 0xFF)` we lose that edge and the delay-slot copy.
   Facts this round established (each cost time, keep them):
   - gcc 2.6.0 assigns STACK SLOTS IN DECLARATION ORDER: retail's slots are frames 0x10,
     countA 0x18, numA 0x20, countB 0x28, numB 0x30, numSprites 0x38, animFlags 0x40, so the
     source declared countA, numA, countB, numB, numSprites, animFlags in exactly that order.
     With that order our frame is retail's 0x78 and the slot offsets all line up.
   - The free tail must be written TWICE in the source (inner else + outer else); gcc merges
     the two identical copies into retail's one block. A `changed`/`active` flag does NOT
     work - it leaves a `move a0,zero` in the delay slot of retail's own branch, and a
     `pBlock = NULL` fallback does the same.
   - `frames` is a WORD parameter with `if (frames == 0) frames = 1;` - a ternary gives a
     branch, and u16 gives `sh`/`lhu` at entry instead of retail's two `sw`.
   - the rot half wraps its delta (`& 0xFFF`, `if (>= 0x800) -= 0x1000`) and then adds the
     absolute angle back when the flag is set (i.e. the wrapped-delta store happens first,
     unconditionally); the pos half has a plain if/else instead and no wrap. pSprite->pos[]
     is compared with `lw` but subtracted from with `lhu` - that narrowing is what the natural
     `pBlock->targetX = v0 - pSprite->pos[0];` gives.
   - the whole loop is `pSprite++` at the TOP of the body off one `pSprite = pSprites;`
     (`addiu s0,s0,0x7C` lands in the first test's delay slot) and `for (i = 0; i < n; i++)`
     with n a u16 local gives retail's `lhu/blez/move` entry test and `slt` tail.
   permuter: 1653 iterations / 4 min, base 4756 -> 4450; the only "improvement" it found
   deletes the tag-0xFF guard (semantically wrong - do not port) plus copying `frames` into
   a temp before the pos-half unk12 store.
   Round 185 (park repair): still compiles, 86.05% / 1136 bytes vs retail's 1144 -
   the note is accurate as written.
s32 func_800A1CF4(BattleBlockPool* pPool, BattleSprite* pSprites, BattleAnimData* pData, u32 frames,
                  u32 absolute, u32 blockFlags, u32 tag) {
    BattleBlock* pBlock;
    BattleSprite* pSprite;
    s16* pFrame;
    u16 countA;
    u16 numA;
    u16 countB;
    u16 numB;
    u16 numSprites;
    u16 animFlags;
    s16 unk06;
    s32 v0;
    s32 v1;
    s32 v2;
    s32 i;

    if (frames == 0) {
        frames = 1;
    }
    numA = 0;
    numB = 0;
    absolute &= 1;
    blockFlags &= 1;
    countA = pData->countA;
    countB = pData->countB;
    animFlags = pData->flags;
    unk06 = pData->unk06;
    pFrame = (s16*)(pData + 1);
    if (unk06 == 0) {
        pFrame += (countA + 1) * 3;
    }
    numSprites = pSprites->count - 1;
    pSprite = pSprites;
    for (i = 0; i < numSprites; i++) {
        pSprite++;
        if (!(animFlags & 1) && numA < countA) {
            v0 = *pFrame++;
            v1 = *pFrame++;
            v2 = *pFrame++;
            numA++;
            if (pSprite->rot[0] != v0 || pSprite->rot[1] != v1 || pSprite->rot[2] != v2) {
                pBlock = pSprite->p70;
                if (pBlock == NULL) {
                    pBlock = BattleAllocBlock(pPool);
                }
                if (pBlock != NULL && pBlock->tag != 0xFF) {
                    pBlock->inUse = 1;
                    pBlock->opcode = absolute + 3;
                    pBlock->flags = blockFlags;
                    pBlock->tag = tag;
                    pBlock->startX = pSprite->rot[0];
                    pBlock->startY = pSprite->rot[1];
                    pBlock->startZ = pSprite->rot[2];
                    v0 = (v0 - pSprite->rot[0]) & 0xFFF;
                    if (v0 >= 0x800) {
                        v0 -= 0x1000;
                    }
                    pBlock->targetX = v0;
                    v1 = (v1 - pSprite->rot[1]) & 0xFFF;
                    if (v1 >= 0x800) {
                        v1 -= 0x1000;
                    }
                    pBlock->targetY = v1;
                    v2 = (v2 - pSprite->rot[2]) & 0xFFF;
                    if (v2 >= 0x800) {
                        v2 -= 0x1000;
                    }
                    pBlock->targetZ = v2;
                    if (absolute != 0) {
                        pBlock->targetX += pSprite->rot[0];
                        pBlock->targetY += pSprite->rot[1];
                        pBlock->targetZ += pSprite->rot[2];
                    }
                    pBlock->curFrame = 0;
                    pBlock->endFrame = frames;
                    pSprite->p70 = pBlock;
                }
            } else {
                pBlock = pSprite->p70;
                if (pBlock != NULL && pBlock->tag != 0xFF) {
                    BattleFreeBlock(pPool, pBlock);
                    pSprite->p70 = NULL;
                }
            }
        } else {
            pBlock = pSprite->p70;
            if (pBlock != NULL && pBlock->tag != 0xFF) {
                BattleFreeBlock(pPool, pBlock);
                pSprite->p70 = NULL;
            }
        }
        if (!(animFlags & 2) && numB < countB) {
            v0 = *pFrame++;
            v1 = *pFrame++;
            v2 = *pFrame++;
            numB++;
            if (pSprite->pos[0] != v0 || pSprite->pos[1] != v1 || pSprite->pos[2] != v2) {
                pBlock = pSprite->p74;
                if (pBlock == NULL) {
                    pBlock = BattleAllocBlock(pPool);
                }
                if (pBlock != NULL && pBlock->tag != 0xFF) {
                    pBlock->inUse = 1;
                    pBlock->opcode = absolute + 3;
                    pBlock->flags = blockFlags;
                    pBlock->tag = tag;
                    pBlock->startX = pSprite->pos[0];
                    pBlock->startY = pSprite->pos[1];
                    pBlock->startZ = pSprite->pos[2];
                    if (absolute != 0) {
                        pBlock->targetX = v0;
                        pBlock->targetY = v1;
                        pBlock->targetZ = v2;
                    } else {
                        pBlock->targetX = v0 - pSprite->pos[0];
                        pBlock->targetY = v1 - pSprite->pos[1];
                        pBlock->targetZ = v2 - pSprite->pos[2];
                    }
                    pBlock->curFrame = 0;
                    pBlock->endFrame = frames;
                    pSprite->p74 = pBlock;
                }
            } else {
                pBlock = pSprite->p74;
                if (pBlock != NULL && pBlock->tag != 0xFF) {
                    BattleFreeBlock(pPool, pBlock);
                    pSprite->p74 = NULL;
                }
            }
        } else {
            pBlock = pSprite->p74;
            if (pBlock != NULL && pBlock->tag != 0xFF) {
                BattleFreeBlock(pPool, pBlock);
                pSprite->p74 = NULL;
            }
        }
    }
    return numSprites;
}
*/
void BattleFreeSpriteBlocksByFlags(BattleBlockPool* pPool, BattleSprite* pSprites, s32 index, u32 flags) {
    BattleSprite* pSprite;

    pSprite = pSprites;
    if (index < pSprite->count) {
        pSprite += index;
        if (pSprite->p70 != NULL && (flags & 1)) {
            BattleFreeBlock(pPool, pSprite->p70);
            pSprite->p70 = NULL;
        }
        if (pSprite->p74 != NULL && (flags & 2)) {
            BattleFreeBlock(pPool, pSprite->p74);
            pSprite->p74 = NULL;
        }
        if (pSprite->p78 != NULL && (flags & 4)) {
            BattleFreeBlock(pPool, pSprite->p78);
            pSprite->p78 = NULL;
        }
    }
}
BattleBlockPool* BattleCreateBlockPool(BattleBlockPool* pPool, s32 count) {
    if (count <= 0) {
        return NULL;
    }
    pPool->capacity = count;
    HeapChangeCurrentUser(4, NULL);
    pPool->pBlocks = HeapAlloc(count * 0x14, 0);
    if (pPool->pBlocks != NULL) {
        BattleResetBlockPool(pPool);
        return pPool;
    }
    return NULL;
}
void BattleFreeBlockPool(BattleBlockPool* pPool) {
    u8* pBlocks = pPool->pBlocks;

    pPool->usedCount = 0;
    if (pBlocks != NULL) {
        HeapFree(pBlocks);
    }
    pPool->pBlocks = NULL;
}
void BattleResetBlockPool(BattleBlockPool* pPool) {
    BattleBlock* pBlock;
    s32 i;

    if (pPool->pBlocks != NULL) {
        pBlock = (BattleBlock*)pPool->pBlocks;
        pPool->usedCount = 0;
        for (i = 0; i < pPool->capacity; i++) {
            pBlock->inUse = 0;
            pBlock++;
        }
    }
}
BattleBlock* BattleAllocBlock(BattleBlockPool* pPool) {
    BattleBlock* pBlock;

    if (pPool->usedCount < pPool->capacity) {
        pBlock = &((BattleBlock*)pPool->pBlocks)[pPool->usedCount];
        if (pBlock->inUse == 0) {
            pPool->usedCount++;
            while (pPool->usedCount < pPool->capacity &&
                   ((BattleBlock*)pPool->pBlocks)[pPool->usedCount].inUse != 0) {
                pPool->usedCount++;
            }
            return pBlock;
        }
    }
    return NULL;
}
s32 BattleFreeBlock(BattleBlockPool* pPool, BattleBlock* pBlock) {
    s32 index;

    if (pBlock == NULL) {
        return -1;
    }
    index = (u32)((u8*)pBlock - pPool->pBlocks) / 0x14;
    if (index < pPool->usedCount) {
        pPool->usedCount = index;
    }
    pBlock->inUse = 0;
    return index;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2434);
/*
    Round 164 park repair: the body did not compile (it spelled the block tag
    `pBlock->unk1[2]` and `pBlock->flag`, which the header now names `tag` and
    `inUse`). Repaired to the header's names + `((u32*)pBlock)[1]/[2]` for the
    two 4-byte stores at +4/+8, and the 5th parameter is now `u32 tag` - that is
    what makes the frame retail's 0x48 (a `u8 tag` spilled the stack argument
    and gave 0x50). Re-measured: 72.45%, 732 bytes vs retail's 720. The tag is
    still not reloaded from the caller frame per init block and pSprites is not
    kept in $s0 (retail s0=a1 pSprites, s1=a2 pData; ours s0=a2 only).
    Round 185 (park repair): still compiles, 72.45% / 732 bytes - confirmed.
s32 func_800A2434(BattleBlockPool* pPool, BattleSprite* pSprites, BattleAnimData* pData, u32 flags,
                   u32 tag) {
    BattleBlock* pBlock;
    u16* pIdx;
    u8* pBytes;
    u8* pTable;
    u16 numSprites;
    u16 countA;
    u16 countB;
    u16 limit;
    u16 animFlags;
    s16 unk12;
    u32 flagA;
    u8* pB;
    s32 i;

    if (pData->unk06 != 0) {
        BattleFreeAllSpriteBlocks(pPool, pSprites);
        func_800A1B50(pSprites, pData);
        return 1;
    }
    countA = pData->countA;
    numSprites = pSprites->count;
    limit = numSprites;
    if (countA + 1 < numSprites) {
        limit = countA + 1;
    }
    countB = pData->countB;
    flagA = flags & 1;
    unk12 = *(u16*)(pData->unk00 + 2);
    if (flagA == 0) {
        unk12--;
    }
    animFlags = pData->flags;
    pIdx = (u16*)(pData + 1);
    pTable = (u8*)pIdx + (countA + 1) * 6;
    if (!(animFlags & 1)) {
        pTable += countA * 6;
    }
    if (!(animFlags & 2)) {
        pTable += countB * 6;
    }
    if (limit <= 0) {
        return 0;
    }
    for (i = 0; i < limit; pIdx += 2, i++, pSprites++) {
        pBytes = (u8*)(pIdx + 2);
        if (*pIdx != 0xFFFF) {
            pBlock = pSprites->p70;
            if (pBlock == NULL) {
                pBlock = BattleAllocBlock(pPool);
            } else if (pBlock->tag == 0xFF) {
                pBlock = NULL;
            }
            if (pBlock != NULL) {
                pB = (u8*)pBlock;
                pBlock->inUse = 1;
                pBlock->flags = flagA;
                pBlock->tag = tag;
                pBlock->opcode = pBytes[0];
                pBlock->curFrame = 0;
                pBlock->endFrame = unk12;
                *(u32*)(pB + 4) = *(u32*)(pB + 8) = (u32)(pTable + *pIdx);
                pSprites->p70 = pBlock;
            }
        } else {
            pBlock = pSprites->p70;
            if (pBlock != NULL && i != 0 && pBlock->tag != 0xFF) {
                BattleFreeBlock(pPool, pBlock);
                pSprites->p70 = NULL;
            }
        }
        pIdx++;
        if (*pIdx != 0xFFFF) {
            pBlock = pSprites->p74;
            if (pBlock == NULL) {
                pBlock = BattleAllocBlock(pPool);
            } else if (pBlock->tag == 0xFF) {
                pBlock = NULL;
            }
            if (pBlock != NULL) {
                pB = (u8*)pBlock;
                pBlock->inUse = 1;
                pBlock->flags = flagA;
                pBlock->tag = tag;
                pBlock->opcode = pBytes[1];
                pBlock->curFrame = 0;
                pBlock->endFrame = unk12;
                *(u32*)(pB + 4) = *(u32*)(pB + 8) = (u32)(pTable + *pIdx);
                pSprites->p74 = pBlock;
            }
        } else {
            pBlock = pSprites->p74;
            if (pBlock != NULL && i != 0 && pBlock->tag != 0xFF) {
                BattleFreeBlock(pPool, pBlock);
                pSprites->p74 = NULL;
            }
        }
    }
    return 0;
}
*/

/* 69.38% (1024 vs retail's 968 bytes; ~6 excess instructions in the body). The structural
   read below is CONFIRMED against the .s, and three facts cost a round to establish:
   - the 5th parameter is a WORD, not a byte. Declared `u32 tag`, gcc uses the incoming
     stack slot in place and emits retail's two `lbu 0x88(sp)` (the `sb t0,0x3(a0)` narrows
     it). Declared `u8 tag`, gcc makes a register copy of the stack argument at entry,
     spills it, and the frame becomes 0x80 instead of retail's 0x78, shifting every stack
     slot by 8. That is the whole "0x50 vs 0x48 frame" mystery noted on func_800A2434.
   - `pData->unk06` is u16: retail loads it with `lhu` (the field is changed above).
   - there must be NO `if (limit <= 0) return 0;` - retail's `blez 0x30(sp)` entry test IS
     the `for` loop's own test (same as the func_800A1B50 note below).
   - the index-0xFFFF case and the block-tag-0xFF case SHARE one tail (`pValues += 3;
     numA++`): retail jumps to it from both tests, with `andi v0,s6,1` stolen into both
     delay slots. Writing the tag case as a plain `else if` on the NULL check converges it
     onto the update block instead (tried: 62.81%). A `u32 active` flag in the source is the
     only shape found that gives retail's two edges, and it did not improve the compile.
   Remaining delta is ONE allocator tie and the branch layout it drags along: retail keeps
   flagA in $fp and spills pBytes to 0x18(sp) (retail slots: 0x18 pBytes, 0x20 countA,
   0x28 countB, 0x30 limit, 0x38 unk12, 0x48 the constant 1 held in $a1 and spilled around
   BattleAllocBlock); this version keeps pBytes in $fp and spills flagA to 0x38, which
   renumbers the loop registers (retail pValues=$s0 pTable=$s7 animFlags=$s6 numA=$s5
   numB=$s4 flagA=$fp; ours $s0/$s4/$s7/$s6/$s5) and inverts the availability test (mine
   inlines the BattleAllocBlock call, retail puts it out of line at .L800A2874).
   permuter: 1465 iterations / 4 min, base 7767 best 6517, and all it found were the merged
   double store and the `countB > numB` operand swap (both ported here: 66.4 -> 69.4%).
   Next: a longer permuter run, or the source lever that makes pBytes the spilled value.
   Original structural read (still accurate): 5 parameters (BattleBlockPool* pPool,
   BattleSprite* pSprites, BattleAnimData* pData, u32 flags, tag on the stack at 0x88(sp)).
     if (pData->unk06 != 0) { BattleFreeAllSpriteBlocks(pPool, pSprites);
                              func_800A1B50(pSprites, pData); return 1; }
     flagA = flags & 1; countA = pData->countA; numA = 0; limit = pSprites->count;
     countB = pData->countB; numB = 0; if (countA + 1 < limit) limit = countA + 1;
     unk12 = *(u16*)(pData->unk00 + 2); if (!flagA) unk12--;
     animFlags = pData->flags; pFrame = (u16*)(pData + 1);
     pValues = (s16*)((u8*)pFrame + (countA + 1) * 6); pTable = (u8*)pValues;
     if (!(animFlags & 1)) pTable += countA * 6;  if (!(animFlags & 2)) pTable += countB * 6;
     for (i = 0; i < limit; i++) with TWO mirrored halves per sprite: frame record is
     6 bytes (u16 indexA, u16 indexB, u8 byteA, u8 byteB), indexA drives pSprites->p70
     (rot at 0x54, lhu, plus doTransform and doRotate), indexB drives p74 (pos at 0x5C,
     lh/sw, plus doTransform only); each half: block from the sprite field, else
     BattleAllocBlock(pPool); while the half is enabled (!(animFlags & n) && i != 0 &&
     counter < countN) the three frame shorts are copied and the counter advances, else
     pValues += 3; block init: unk0 = 1, unk1[0] = flagA, unk1[1] = byteA/byteB,
     unk1[2] = tag, unk1[0xF] = 0 (sh), unk1[0x11] = unk12 (sh), unk1[3] and unk1[7] =
     pTable + index, then the sprite block field = pBlock. Retail's registers: s1 =
     pSprites + 0x74 (sprite fields as negative displacements), s6 = animFlags,
     fp = flagA, s3 = i, s5 = rot counter, s4 = pos counter, s0 = frame walker, s7 = pTable.
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2704);
/*
    Round 164 park repair: did not compile for the same reason (header field
    renames). Repaired; re-measured 69.38%, 1024 bytes vs retail's 968, i.e. the
    note above is still accurate - the remaining work is the pBytes-spill
    allocator tie and the out-of-line BattleAllocBlock call.
    Round 185 (park repair): still compiles, 69.38% / 1024 bytes - confirmed.
s32 func_800A2704(BattleBlockPool* pPool, BattleSprite* pSprites, BattleAnimData* pData, u32 flags,
                  u32 tag) {
    BattleBlock* pBlock;
    u16* pFrame;
    s16* pValues;
    u8* pTable;
    u8* pBytes;
    u16 countA;
    u16 countB;
    u16 limit;
    u16 unk12;
    u16 animFlags;
    u16 numA;
    u16 numB;
    u32 flagA;
    s32 i;

    if (pData->unk06 != 0) {
        BattleFreeAllSpriteBlocks(pPool, pSprites);
        func_800A1B50(pSprites, pData);
        return 1;
    }
    flagA = flags & 1;
    countA = pData->countA;
    numA = 0;
    limit = pSprites->count;
    countB = pData->countB;
    numB = 0;
    if (countA + 1 < limit) {
        limit = countA + 1;
    }
    unk12 = *(u16*)(pData->unk00 + 2);
    if (flagA == 0) {
        unk12--;
    }
    animFlags = pData->flags;
    pFrame = (u16*)(pData + 1);
    pValues = (s16*)((u8*)pFrame + (countA + 1) * 6);
    pTable = (u8*)pValues;
    if (!(animFlags & 1)) {
        pTable += countA * 6;
    }
    if (!(animFlags & 2)) {
        pTable += countB * 6;
    }
    for (i = 0; i < limit; i++) {
        pBytes = (u8*)(pFrame + 2);
        if (*pFrame != 0xFFFF) {
            pBlock = pSprites->p70;
            if (pBlock == NULL) {
                pBlock = BattleAllocBlock(pPool);
            } else if (pBlock->tag == 0xFF) {
                pBlock = NULL;
            }
            if (!(animFlags & 1) && i != 0 && numA < countA) {
                pSprites->rot[0] = *pValues++;
                pSprites->rot[1] = *pValues++;
                pSprites->rot[2] = *pValues++;
                numA++;
                pSprites->doTransform = 1;
                pSprites->doRotate = 1;
            }
            if (pBlock != NULL) {
                pBlock->inUse = 1;
                pBlock->flags = flagA;
                pBlock->tag = tag;
                pBlock->opcode = pBytes[0];
                pBlock->curFrame = 0;
                pBlock->endFrame = unk12;
                *(u32*)((u8*)pBlock + 4) = (u32)(pTable + *pFrame);
                *(u32*)((u8*)pBlock + 8) = (u32)(pTable + *pFrame);
                pSprites->p70 = pBlock;
            }
        } else if (!(animFlags & 1) && i != 0 && numA < countA) {
            pValues += 3;
            numA++;
        }
        pFrame++;
        if (*pFrame != 0xFFFF) {
            pBlock = pSprites->p74;
            if (pBlock == NULL) {
                pBlock = BattleAllocBlock(pPool);
            } else if (pBlock->tag == 0xFF) {
                pBlock = NULL;
            }
            if (!(animFlags & 2) && i != 0 && numB < countB) {
                pSprites->pos[0] = *pValues++;
                pSprites->pos[1] = *pValues++;
                pSprites->pos[2] = *pValues++;
                numB++;
                pSprites->doTransform = 1;
            }
            if (pBlock != NULL) {
                pBlock->inUse = 1;
                pBlock->flags = flagA;
                pBlock->tag = tag;
                pBlock->opcode = pBytes[1];
                pBlock->curFrame = 0;
                pBlock->endFrame = unk12;
                *(u32*)((u8*)pBlock + 8) = (*(u32*)((u8*)pBlock + 4) = (u32)(pTable + *pFrame));
                pSprites->p74 = pBlock;
            }
        } else if (!(animFlags & 2) && i != 0 && countB > numB) {
            pValues += 3;
            numB++;
        }
        pSprites++;
        pFrame += 3;
    }
    return 0;
}
*/

void BattleFreeAllSpriteBlocks(BattleBlockPool* pPool, BattleSprite* pSprites) {
    BattleBlock* pBlock;
    BattleSprite* pSprite;
    s32 i;
    s32 count;

    count = pSprites->count;
    pSprite = pSprites;
    for (i = 0; i < count; i++, pSprite++) {
        pBlock = pSprite->p70;
        if (pBlock != NULL && pBlock->tag != 0xFF) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->tag != 0xFF) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->tag != 0xFF) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p78 = NULL;
        }
    }
}
void BattleFreeSpriteBlocksByTag(BattleBlockPool* pPool, BattleSprite* pSprites, u8 tag) {
    BattleBlock* pBlock;
    BattleSprite* pSprite;
    s32 i;
    s32 count;

    count = pSprites->count;
    pSprite = pSprites;
    for (i = 0; i < count; i++, pSprite++) {
        pBlock = pSprite->p70;
        if (pBlock != NULL && pBlock->tag == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->tag == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->tag == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p78 = NULL;
        }
    }
}
BattleSpritePool* BattleCreateSpritePool(BattleSpritePool* pPool, s32 count) {
    HeapChangeCurrentUser(4, NULL);
    pPool->count = count;
    pPool->unk6 = 0;
    pPool->pBlocks = HeapAlloc((count + 1) * sizeof(Vertex), 0);
    if (pPool->pBlocks != NULL) {
        BattleResetSpritePool(pPool);
        return pPool;
    }
    return NULL;
}
void BattleFreeSpritePool(BattleBlockPool* pPool) {
    u8* pBlocks = pPool->pBlocks;

    pPool->usedCount = 0;
    pPool->capacity = 0;
    if (pBlocks != NULL) {
        HeapFree(pBlocks);
    }
    pPool->pBlocks = NULL;
}
void BattleResetSpritePool(BattleSpritePool* pPool) {
    int i;
    int j;
    Vertex* pVertex;

    pVertex = (Vertex*)pPool->pBlocks;
    for (i = 0; i < pPool->count + 1; i++) {
        pVertex->age = -1;
        pVertex->lifetime = 0;

        for (j = 0; j < 2; j++) {
            SetPolyFT4(&pVertex->polys[j]);
            SetSemiTrans(&pVertex->polys[j], 1);
            pVertex->polys[j].clut = GetClut(0, 0x1CD);
            pVertex->polys[j].tpage = GetTPage(0, 1, 0x380, 0x0);
            setUV4(&pVertex->polys[j], 0x0, 0xC1, 0x0, 0xC1, 0xF, 0xC1, 0xF, 0xC1);
        }

        pVertex++;
    }
}
