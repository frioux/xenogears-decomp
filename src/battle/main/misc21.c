#include "common.h"
#include "psyq/libgpu.h"
#include "system/model.h"
#include "libgear/main.h"
#include "battle/actor.h"
#include "psyq/inline_c.h"

#define BATTLE_ACTOR_DEPTH_MAX 0x2D8
#define BATTLE_ACTOR_SHADE_MAX 0x6B

typedef struct {
    /* 0x0 */ BattleBlock* pBlocks;
    /* 0x4 */ u16 usedCount;
    /* 0x6 */ u16 capacity;
} BattleBlockPool; // Size: 0x8

typedef struct {
    /* 0x0 */ Vertex* pBlocks;
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

// The three MATRIX slots the battle renderer stages its GTE work in, at the
// start of the PSX scratchpad: the working matrix, the composed rotation and
// the composed light/translation matrix.
typedef struct {
    /* 0x00 */ MATRIX work;
    /* 0x20 */ MATRIX rot;
    /* 0x40 */ MATRIX trans;
} BattleScratchMatrices; // Size: 0x60

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
extern s16 g_BattleTweenVectorScale;

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
        if (pSprites->pParent != NULL && ((BattleSprite*)pSprites->pParent)->doTransform == 1) {
            pSprites->doTransform = 1;
        }
        if (pSprites->doTransform != 0) {
            pSprites->localTransform.t[0] = pSprites->pos[0];
            pSprites->localTransform.t[1] = pSprites->pos[1];
            pSprites->localTransform.t[2] = pSprites->pos[2];
            if (pSprites->pParent != NULL) {
                CompMatrix(&((BattleSprite*)pSprites->pParent)->globalTransform, &pSprites->localTransform,
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
}
s32 BattleUpdateScaledSpriteTransforms(BattleSprite* pSprites, s32 scale) {
    BattleSprite* pSprite = pSprites;
    MATRIX* pScaleMatrix = (MATRIX*)PSX_SCRATCH;
    u32 i;
    u32 count;

    pSprites->globalTransform.t[0] = pSprites->pos[0];
    pSprites->globalTransform.t[1] = pSprites->pos[1];
    pSprites->globalTransform.t[2] = pSprites->pos[2];

    count = pSprites->count;

    if (pSprites->rotDirection != 0) {
        RotMatrixYXZ((SVECTOR*)pSprites->rot, &pSprites->globalTransform);
    } else {
        RotMatrix((SVECTOR*)pSprites->rot, &pSprites->globalTransform);
    }

    pScaleMatrix->m[0][0] = (scale * pSprites->scale.vx) >> 0xC;
    pScaleMatrix->m[0][1] = 0;
    pScaleMatrix->m[0][2] = 0;
    pScaleMatrix->m[1][0] = 0;
    pScaleMatrix->m[1][1] = (scale * pSprites->scale.vy) >> 0xC;
    pScaleMatrix->m[1][2] = 0;
    pScaleMatrix->m[2][0] = 0;
    pScaleMatrix->m[2][1] = 0;
    pScaleMatrix->m[2][2] = (scale * pSprites->scale.vz) >> 0xC;

    MulMatrix0(&pSprites->globalTransform, pScaleMatrix, &pSprites->localTransform);

    pSprites->localTransform.t[0] = pSprites->globalTransform.t[0];
    pSprites->localTransform.t[1] = pSprites->globalTransform.t[1];
    pSprites->localTransform.t[2] = pSprites->globalTransform.t[2];

    for (i = 1; i < count; i++) {
        pSprites++;

        if (pSprites->pParent != NULL) {
            if (((BattleSprite*)pSprites->pParent)->doRotate == TRUE) {
                pSprites->doRotate = TRUE;
            }
            if (((BattleSprite*)pSprites->pParent)->doTransform == TRUE) {
                pSprites->doTransform = TRUE;
            }
        }

        if (pSprites->doRotate) {
            MATRIX* pScratch = (MATRIX*)PSX_SCRATCH;

            if (pSprites->rotDirection != 0) {
                RotMatrixYXZ((SVECTOR*)pSprites->rot, &pSprites->localTransform);
            } else {
                RotMatrix((SVECTOR*)pSprites->rot, &pSprites->localTransform);
            }

            pScratch->m[0][0] = pSprites->scale.vx;
            pScratch->m[0][1] = 0;
            pScratch->m[0][2] = 0;
            pScratch->m[1][0] = 0;
            pScratch->m[1][1] = pSprites->scale.vy;
            pScratch->m[1][2] = 0;
            pScratch->m[2][0] = 0;
            pScratch->m[2][1] = 0;
            pScratch->m[2][2] = pSprites->scale.vz;

            MulMatrix0(&pSprites->localTransform, pScratch, &pSprites->localTransform);

            if (pSprites->pParent != NULL) {
                pScratch->m[0][0] = 0x1000000 / ((BattleSprite*)pSprites->pParent)->scale.vx;
                pScratch->m[0][1] = 0;
                pScratch->m[0][2] = 0;
                pScratch->m[1][0] = 0;
                pScratch->m[1][1] = 0x1000000 / ((BattleSprite*)pSprites->pParent)->scale.vy;
                pScratch->m[1][2] = 0;
                pScratch->m[2][0] = 0;
                pScratch->m[2][1] = 0;
                pScratch->m[2][2] = 0x1000000 / ((BattleSprite*)pSprites->pParent)->scale.vz;

                MulMatrix0(pScratch, &pSprites->localTransform, &pSprites->localTransform);
            }
        }

        if (pSprites->doTransform) {
            pSprites->localTransform.t[0] = pSprites->pos[0];
            pSprites->localTransform.t[1] = pSprites->pos[1];
            pSprites->localTransform.t[2] = pSprites->pos[2];

            if (pSprites->pParent != NULL) {
                CompMatrix(&((BattleSprite*)pSprites->pParent)->globalTransform, &pSprites->localTransform,
                           &pSprites->globalTransform);
            } else {
                pSprites->globalTransform = pSprites->localTransform;
            }
        }
    }

    for (i = 1; i < count; i++) {
        pSprite++;
        pSprite->doRotate = FALSE;
        pSprite->doTransform = FALSE;
    }

    return count;
}
void BattleNoOp(void) {
}
void BattleRenderModel(BattleModelPartList* pMesh, BattleSprite* pSkeleton, MATRIX* pLightMatrix,
                   MATRIX* pViewMatrix, s32 arg4, s32 arg5, s32 packetIndex) {
    BattleScratchMatrices* pScratch = (BattleScratchMatrices*)PSX_SCRATCH;
    MATRIX* pRotMatrix = &pScratch->rot;
    MATRIX* pTransMatrix = &pScratch->trans;
    MATRIX* pMatrix = &pScratch->work;
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
   D_800D3306/3308/330A (g_BattlePartSlots is 0x800D3304, so those are its +2/+4/+6
   fields), so objdiff reads 99.97%; the linked bytes are identical
   (tools/scripts/check_bin.sh battle: OK). The actor scale is read as `*(u16*)&`
   because actor.h types it `s16` and retail loads it `lhu`. */
void BattleDrawActor(BattleActor* pActor, MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 flags,
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
    BattleScratchMatrices* pScratch = (BattleScratchMatrices*)PSX_SCRATCH;
    MATRIX* pRotMatrix = &pScratch->rot;
    MATRIX* pTransMatrix = &pScratch->trans;
    s32 scale;
    MATRIX* pMatrix = &pScratch->work;
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
            s16 dx = g_BattlePartSlots[i].x - (u16)pSprite->pos[0];
            s16 dy = g_BattlePartSlots[i].y -
                     ((pSprite->pos[1] - pActor->height * (scale * pSprite->scale.vy >> 12)) >> 13);
            s16 dz = g_BattlePartSlots[i].z - (u16)pSprite->pos[2];
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
        gte_stsxy2(&pActor->polys[quadIndex].x0);
        gte_stszotz(&depth);
        vec.vx = -pActor->width;
        gte_ldv0(&vec);
        gte_rtps();
        gte_stsxy2(&pActor->polys[quadIndex].x1);
        gte_stszotz(&sz);
        if (sz < depth) {
            depth = sz;
        }
        vec.vx = pActor->width;
        vec.vz = -pActor->depth;
        gte_ldv0(&vec);
        gte_rtps();
        gte_stsxy2(&pActor->polys[quadIndex].x2);
        gte_stszotz(&sz);
        if (sz < depth) {
            depth = sz;
        }
        vec.vx = -pActor->width;
        gte_ldv0(&vec);
        gte_rtps();
        gte_stsxy2(&pActor->polys[quadIndex].x3);
        gte_stszotz(&sz);
        if (sz < depth) {
            depth = sz;
        }
        depth >>= g_GfxOTDepthShift;
        addPrim(&pOt[depth], &pActor->polys[quadIndex]);
        if (depth > BATTLE_ACTOR_DEPTH_MAX) {
            depth = BATTLE_ACTOR_DEPTH_MAX;
        }
        pActor->depthShade = BATTLE_ACTOR_SHADE_MAX - depth / 8;
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
            tweenVec.vx = -g_BattleTweenVectorScale * rsin(pSprites->rot[1] + 0x400) / 0x1000;
            tweenVec.vz = g_BattleTweenVectorScale * rcos(pSprites->rot[1] + 0x400) / 0x1000;
            tweenVec.vy = pActor->unk3E;
            CompMatrix(&pSprites->localTransform, &pSprites[pModelList->spriteIndex].globalTransform, pMatrix);
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            for (j = 0; j < pModelList->pointCount; j++) {
                gte_ldv0(&pModelList->pPoints[j]);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(macPoint);
                ((u16*)((void**)pModelList->pPackets)[j])[2] = (u16)macPoint[0];
                ((u16*)((void**)pModelList->pPackets)[j])[3] = (u16)macPoint[1];
                ((u16*)((void**)pModelList->pPackets)[j])[4] = (u16)macPoint[2];
            }
            pPoint = pModelList->pRecords;
            for (j = 0; j < pModelList->recordCount; j++, pPoint++) {
                CompMatrix(&pSprites->localTransform, &pSprites[pPoint->spriteIndex].globalTransform, pMatrix);
                SetRotMatrix(pMatrix);
                SetTransMatrix(pMatrix);
                gte_ldv0((SVECTOR*)&pPoint->vx);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(macPoint);
                pPoint->x = (u16)macPoint[0];
                pPoint->y = (u16)macPoint[1];
                pPoint->z = (u16)macPoint[2];
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
                DVECTOR* pStart = &pTween->projectedStart[side * 4];
                DVECTOR* pEnd = &pTween->projectedEnd[side * 4];

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
                    DVECTOR* pFrom = &pTween->projectedStart[other * 4];
                    DVECTOR* pTo = &pTween->projectedEnd[other * 4];

                    pTween->pSlot->x0 = pFrom[0].vx;
                    pTween->pSlot->y0 = pFrom[1].vx;
                    pTween->pSlot->z0 = pFrom[2].vx;
                    pTween->pSlot->x2 = pTo[0].vx;
                    pTween->pSlot->y2 = pTo[1].vx;
                    pTween->pSlot->z2 = pTo[2].vx;
                }
            done3D:
                pTween->pSlot->x1 = pStart[0].vx;
                pTween->pSlot->y1 = pStart[1].vx;
                pTween->pSlot->z1 = pStart[2].vx;
                pTween->pSlot->x3 = pEnd[0].vx;
                pTween->pSlot->y3 = pEnd[1].vx;
                pTween->pSlot->z3 = pEnd[2].vx;
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
// Parked body: 82.84% / 5092 bytes / 1273 instructions against retail's 4888 / 1249.
// The whole body is present, both owned jump tables render, and the prologue, the
// loop tail and the ApplyMatrix site now carry retail's instructions apart from
// register names. This is the SAME function as lib_gear/main/main's func_801DDBF8
// (twin_for.py ratio 1.000, 1222 insns; that unit parks it at 79.62%), so what is
// learned here ports there and back. Measurements on the way here: the twin's body
// 79.62% / 4892 bytes (one instruction over), per-channel pObject->value.step.pCursor
// stores 80.68%, `*(s8*)...pCursor++` in the RELATIVE arms 82.52%, both together
// 82.92%, and the single-walker form below 83.57% / 5096 bytes (the highest objdiff
// score measured; same body, `u16 numChildren`).
// Findings from this pass, each measured alone:
//  - `for (i = 0; i < numChildren; i++, pJoint++)` with no second walker pointer
//    (82.92% -> 83.57%). gcc makes retail's shape from it: the register copy biased
//    +4 (retail `addiu s1,a1,4`, hence its 0x6C/0x48/0x58(s1) field offsets) plus the
//    +0x7C step on the spilled home as well. A `pJoint = pMesh;` body walks the same
//    but allocates worse.
//  - `u32 numChildren`, because retail stores it with `sw` and tests it at the entry
//    with a bare `beqz count`; u16 gives `sh` plus an `sltu`/`beqz` pair. This body
//    keeps the u32 form (1273 insns) although objdiff scores it 0.7 lower than the
//    u16 one (1274 insns): two known instruction differences are worth more than the
//    fuzzy number, which is not monotone for a register delta.
//  - declaring `u32 i;` BEFORE `u32 numChildren;` puts the slots at retail's offsets:
//    i at 0x48 (`sw zero,72(sp)` at the entry) and count at 0x50.
//  - pJoint's home shares the pSkeleton parameter slot in both compiles (mine 0x40,
//    retail 0x30); the offset differs only because this body spills a2/a3 before a1
//    where retail spills a0..a3 in argument order.
//  - the translation ADD arm's ApplyMatrix site is retail's: $sp+0x10 for vec, the
//    VECTOR out at $sp+0x18, pJoint+0x2C for globalTransform, and the `sh` of vec.vz
//    in the jal slot (mine emits addu where retail emits addiu).
// Residual: `.frame vars` is 112 against retail's 128 - 0x10 of extra spilled
// temporaries in retail, dead stack in both bodies (retail's 0x54..0x8F is unused
// too) - and with it the whole s0..s7 assignment: retail keeps `result` in s5 and `i`
// in memory, this body keeps `result` in s7 and `i` in memory. 24 instructions over,
// spread over the arms: retail hoists `lw a0,0x8(s0)` above each channel's NO_X test
// where this body reloads the cursor inside the arm. Declaration order of
// result/frame/dx/dy/dz/dist/angle/maxStep was tried and changes nothing; u16 vs u32
// numChildren and the single walker are the only knobs that moved the score at all.
// Permuter not run: the residual is 24 instructions of arm shape, not a register tie.
// The LIBGEAR_* constants and the JointObject view below duplicate
// lib_gear/main/main.c's file-local spellings; they belong in include/libgear/main.h.
// Temp3::opcode & this mask is the command the queued-object update dispatches on,
// and the update's result bits below. The spellings are lib_gear/main/main.c's, where
// this function also lives; they belong in include/libgear/main.h and are repeated
// here only because that unit defines them file-locally.
#define LIBGEAR_JOINT_OPCODE_MASK 0xF
#define LIBGEAR_JOINT_OBJECT_SET 0
#define LIBGEAR_JOINT_OBJECT_RELATIVE 1
#define LIBGEAR_JOINT_OBJECT_ADD 2
#define LIBGEAR_JOINT_OBJECT_STEP 4
#define LIBGEAR_JOINT_OBJECT_ACCUMULATE 5
#define LIBGEAR_JOINT_OBJECT_NO_X 0x10
#define LIBGEAR_JOINT_OBJECT_NO_Y 0x20
#define LIBGEAR_JOINT_OBJECT_NO_Z 0x40
#define LIBGEAR_SCRIPT_LITERAL -0x80
#define LIBGEAR_JOINT_UPDATE_ADVANCED 0x1
#define LIBGEAR_JOINT_UPDATE_FINISHED 0x2
#define LIBGEAR_JOINT_UPDATE_LOOPED 0x4
#define LIBGEAR_JOINT_UPDATE_ADVANCED_ANY 0x100
#define LIBGEAR_JOINT_UPDATE_FINISHED_ANY 0x200
#define LIBGEAR_JOINT_UPDATE_LOOPED_ANY 0x400
#define LIBGEAR_JOINT_FRAME_ACCUMULATE_MAX 0x7D00
#define LIBGEAR_ANGLE_MASK 0xFFF
#define LIBGEAR_ANGLE_HALF_TURN 0x800
#define LIBGEAR_ANGLE_FULL_TURN 0x1000

// A joint's queued object (Temp3) as the update reads it: bytes 0x4..0xE are a byte
// script while opcode & 0xF is below LIBGEAR_JOINT_OPCODE_INTERPOLATE and an
// interpolation from there up, so they share one union.
typedef struct {
    /* 0x0 */ u8* pScript;
    /* 0x4 */ u8* pCursor;
} JointObjectStep;
typedef struct {
    /* 0x0 */ s16 startX;
    /* 0x2 */ s16 startY;
    /* 0x4 */ s16 startZ;
    /* 0x6 */ s16 targetX;
    /* 0x8 */ s16 targetY;
    /* 0xA */ s16 targetZ;
} JointObjectLerp;
typedef union {
    JointObjectStep step;
    JointObjectLerp lerp;
} JointObjectValue;
typedef struct {
    /* 0x0  */ u8 inUse;
    /* 0x1  */ u8 loops;
    /* 0x2  */ u8 opcode;
    /* 0x3  */ u8 type;
    /* 0x4  */ JointObjectValue value;
    /* 0x10 */ s16 curFrame;
    /* 0x12 */ s16 endFrame;
} JointObject; // Size: 0x14

/*
s32 func_800A0838(BattleBlockPool* pPool, ModelJoint* pSkeleton, u32 objectType, s32 scale) {
    s32 result;
    JointObject* pObject;
    ModelJoint* pJoint;
    u32 i;
    u32 numChildren;
    s16 frame;
    u8* pCursor;
    u8 opcode;
    s8 delta;
    SVECTOR vec;
    VECTOR out;
    s16 dx;
    s16 dy;
    s16 dz;
    s32 dist;
    s16 angle;
    s16 maxStep;

    result = 0;
    numChildren = pSkeleton->numChildren;
    pJoint = pSkeleton;

    for (i = 0; i < numChildren; i++, pJoint++) {

        // Rotation channel
        pObject = (JointObject*)pJoint->pRotationObject;
        if (pObject != NULL) {
            opcode = pObject->opcode;

            switch (opcode & LIBGEAR_JOINT_OPCODE_MASK) {
            case LIBGEAR_JOINT_OBJECT_SET:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    pJoint->rotation.vx = *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    pJoint->rotation.vy = *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    pJoint->rotation.vz = *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_RELATIVE:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->rotation.vx += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        pJoint->rotation.vx = pCursor[0] | (pCursor[1] << 8);
                        pObject->value.step.pCursor++;
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->rotation.vy += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        pJoint->rotation.vy = pCursor[0] | (pCursor[1] << 8);
                        pObject->value.step.pCursor++;
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->rotation.vz += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        pJoint->rotation.vz = pCursor[0] | (pCursor[1] << 8);
                        pObject->value.step.pCursor++;
                    }
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ADD:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    pJoint->rotation.vx += *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    pJoint->rotation.vy += *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    pJoint->rotation.vz += *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                break;

            case LIBGEAR_JOINT_OPCODE_INTERPOLATE:
                frame = pObject->curFrame + 1;
                pJoint->rotation.vx = pObject->value.lerp.startX +
                    (pObject->value.lerp.targetX * frame) / pObject->endFrame;
                pJoint->rotation.vy = pObject->value.lerp.startY +
                    (pObject->value.lerp.targetY * frame) / pObject->endFrame;
                pJoint->rotation.vz = pObject->value.lerp.startZ +
                    (pObject->value.lerp.targetZ * frame) / pObject->endFrame;
                break;

            case LIBGEAR_JOINT_OBJECT_STEP:
                dx = (pObject->value.lerp.targetX - pJoint->rotation.vx) / pObject->endFrame;
                dy = (pObject->value.lerp.targetY - pJoint->rotation.vy) / pObject->endFrame;
                dz = (pObject->value.lerp.targetZ - pJoint->rotation.vz) / pObject->endFrame;
                if ((dx == 0) && (dy == 0) && (dz == 0)) {
                    pObject->curFrame = pObject->endFrame;
                    pJoint->rotation.vx = pObject->value.lerp.targetX;
                    pJoint->rotation.vy = pObject->value.lerp.targetY;
                    pJoint->rotation.vz = pObject->value.lerp.targetZ;
                } else {
                    pJoint->rotation.vx += dx;
                    pJoint->rotation.vy += dy;
                    pJoint->rotation.vz += dz;
                    pObject->curFrame = 0;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ACCUMULATE:
                pObject->value.lerp.startX += pObject->value.lerp.targetX;
                pJoint->rotation.vx += pObject->value.lerp.startX;
                pObject->value.lerp.startY += pObject->value.lerp.targetY;
                pJoint->rotation.vy += pObject->value.lerp.startY;
                pObject->value.lerp.startZ += pObject->value.lerp.targetZ;
                pJoint->rotation.vz += pObject->value.lerp.startZ;
                break;

            case LIBGEAR_JOINT_OPCODE_MOVE_BASE:
            case LIBGEAR_JOINT_OPCODE_MOVE_BASE + 1:
                dx = pObject->value.lerp.targetX - pJoint->translation.x;
                dy = pObject->value.lerp.targetY - pJoint->translation.y;
                dz = pObject->value.lerp.targetZ - pJoint->translation.z;
                dist = SquareRoot0((dx * dx) + (dy * dy) + (dz * dz)) + 1;
                maxStep = pObject->value.lerp.startY +
                    ((dist + pObject->curFrame) * pObject->value.lerp.startZ) /
                        pObject->value.lerp.startX;
                angle = (ratan2(-dx, -dz) - pJoint->rotation.vy) & LIBGEAR_ANGLE_MASK;
                if (angle >= LIBGEAR_ANGLE_HALF_TURN) {
                    angle -= LIBGEAR_ANGLE_FULL_TURN;
                }
                if ((angle < 0 ? -angle : angle) >= maxStep) {
                    angle = (angle < 0) ? -maxStep : maxStep;
                }
                pJoint->rotation.vy += angle;
                if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) == LIBGEAR_JOINT_OPCODE_MOVE_BASE) {
                    angle = (ratan2(dy, SquareRoot0((dx * dx) + (dz * dz))) -
                                pJoint->rotation.vx) &
                        LIBGEAR_ANGLE_MASK;
                    if (angle >= LIBGEAR_ANGLE_HALF_TURN) {
                        angle -= LIBGEAR_ANGLE_FULL_TURN;
                    }
                    if ((angle < 0 ? -angle : angle) >= maxStep) {
                        angle = (angle < 0) ? -maxStep : maxStep;
                    }
                    pJoint->rotation.vx += angle;
                }
                if (pObject->curFrame < LIBGEAR_JOINT_FRAME_ACCUMULATE_MAX) {
                    pObject->curFrame += pObject->endFrame;
                }
                break;
            }

            pObject->curFrame++;
            if (pObject->curFrame >= pObject->endFrame) {
                if (pObject->loops == 0) {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_FINISHED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_FINISHED_ANY;
                    BattleFreeBlock(pPool, (BattleBlock*)pObject);
                    pJoint->pRotationObject = NULL;
                } else {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_LOOPED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_LOOPED_ANY;
                    if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) < LIBGEAR_JOINT_OPCODE_INTERPOLATE) {
                        pObject->curFrame = 0;
                        pObject->value.step.pCursor = pObject->value.step.pScript;
                    } else {
                        pObject->curFrame = -1;
                        if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) == LIBGEAR_JOINT_OBJECT_ACCUMULATE) {
                            pObject->value.lerp.targetX = 0;
                            pObject->value.lerp.targetY = 0;
                            pObject->value.lerp.targetZ = 0;
                        }
                    }
                }
            } else {
                if (pObject->type == objectType) {
                    result |= LIBGEAR_JOINT_UPDATE_ADVANCED;
                }
                result |= LIBGEAR_JOINT_UPDATE_ADVANCED_ANY;
            }

            pJoint->doRotate = TRUE;
            pJoint->doTransform = TRUE;
        }

        // Translation channel
        pObject = (JointObject*)pJoint->pTranslationObject;
        if (pObject != NULL) {
            opcode = pObject->opcode;

            switch (opcode & LIBGEAR_JOINT_OPCODE_MASK) {
            case LIBGEAR_JOINT_OBJECT_SET:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    pJoint->translation.x = *(s16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    pJoint->translation.y = *(s16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    pJoint->translation.z = *(s16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_RELATIVE:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->translation.x += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        pJoint->translation.x = pCursor[0] | (pCursor[1] << 8);
                        pObject->value.step.pCursor++;
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->translation.y += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        pJoint->translation.y = pCursor[0] | (pCursor[1] << 8);
                        pObject->value.step.pCursor++;
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->translation.z += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        pJoint->translation.z = pCursor[0] | (pCursor[1] << 8);
                        pObject->value.step.pCursor++;
                    }
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ADD:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    vec.vx = *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                } else {
                    vec.vx = 0;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    vec.vy = *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                } else {
                    vec.vy = 0;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    vec.vz = *(u16*)pObject->value.step.pCursor;
                    pObject->value.step.pCursor += 2;
                } else {
                    vec.vz = 0;
                }
                vec.vx = (vec.vx * pJoint->scale.vx) >> 12;
                vec.vy = (vec.vy * pJoint->scale.vy) >> 12;
                vec.vz = (vec.vz * pJoint->scale.vz) >> 12;
                ApplyMatrix(&pJoint->globalTransform, &vec, &out);
                pJoint->translation.x += (scale * out.vx) >> 12;
                pJoint->translation.y += (scale * out.vy) >> 12;
                pJoint->translation.z += (scale * out.vz) >> 12;
                break;

            case LIBGEAR_JOINT_OPCODE_INTERPOLATE:
                frame = pObject->curFrame + 1;
                pJoint->translation.x = pObject->value.lerp.startX +
                    (pObject->value.lerp.targetX * frame) / pObject->endFrame;
                pJoint->translation.y = pObject->value.lerp.startY +
                    (pObject->value.lerp.targetY * frame) / pObject->endFrame;
                pJoint->translation.z = pObject->value.lerp.startZ +
                    (pObject->value.lerp.targetZ * frame) / pObject->endFrame;
                break;

            case LIBGEAR_JOINT_OBJECT_STEP:
                dx = (pObject->value.lerp.targetX - pJoint->translation.x) / pObject->endFrame;
                dy = (pObject->value.lerp.targetY - pJoint->translation.y) / pObject->endFrame;
                dz = (pObject->value.lerp.targetZ - pJoint->translation.z) / pObject->endFrame;
                if ((dx == 0) && (dy == 0) && (dz == 0)) {
                    pObject->curFrame = pObject->endFrame;
                    pJoint->translation.x = pObject->value.lerp.targetX;
                    pJoint->translation.y = pObject->value.lerp.targetY;
                    pJoint->translation.z = pObject->value.lerp.targetZ;
                } else {
                    pJoint->translation.x += dx;
                    pJoint->translation.y += dy;
                    pJoint->translation.z += dz;
                    pObject->curFrame = 0;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ACCUMULATE:
                pObject->value.lerp.startX += pObject->value.lerp.targetX;
                pJoint->translation.x += pObject->value.lerp.startX;
                pObject->value.lerp.startY += pObject->value.lerp.targetY;
                pJoint->translation.y += pObject->value.lerp.startY;
                pObject->value.lerp.startZ += pObject->value.lerp.targetZ;
                pJoint->translation.z += pObject->value.lerp.startZ;
                break;
            }

            pObject->curFrame++;
            if (pObject->curFrame >= pObject->endFrame) {
                if (pObject->loops == 0) {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_FINISHED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_FINISHED_ANY;
                    BattleFreeBlock(pPool, (BattleBlock*)pObject);
                    pJoint->pTranslationObject = NULL;
                } else {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_LOOPED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_LOOPED_ANY;
                    if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) < LIBGEAR_JOINT_OPCODE_INTERPOLATE) {
                        pObject->curFrame = 0;
                        pObject->value.step.pCursor = pObject->value.step.pScript;
                    } else {
                        pObject->curFrame = -1;
                        if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) == LIBGEAR_JOINT_OBJECT_ACCUMULATE) {
                            pObject->value.lerp.targetX = 0;
                            pObject->value.lerp.targetY = 0;
                            pObject->value.lerp.targetZ = 0;
                        }
                    }
                }
            } else {
                if (pObject->type == objectType) {
                    result |= LIBGEAR_JOINT_UPDATE_ADVANCED;
                }
                result |= LIBGEAR_JOINT_UPDATE_ADVANCED_ANY;
            }

            pJoint->doRotate = TRUE;
            pJoint->doTransform = TRUE;
        }

        // Scale channel; it has no script form, so its opcode starts at the interpolation
        pObject = (JointObject*)pJoint->pScaleObject;
        if (pObject != NULL) {
            switch (pObject->opcode & LIBGEAR_JOINT_OPCODE_MASK) {
            case LIBGEAR_JOINT_OPCODE_INTERPOLATE:
                frame = pObject->curFrame + 1;
                pJoint->scale.vx = pObject->value.lerp.startX +
                    (pObject->value.lerp.targetX * frame) / pObject->endFrame;
                pJoint->scale.vy = pObject->value.lerp.startY +
                    (pObject->value.lerp.targetY * frame) / pObject->endFrame;
                pJoint->scale.vz = pObject->value.lerp.startZ +
                    (pObject->value.lerp.targetZ * frame) / pObject->endFrame;
                break;

            case LIBGEAR_JOINT_OBJECT_STEP:
                dx = (pObject->value.lerp.targetX - pObject->value.lerp.startX) /
                    pObject->endFrame;
                dy = (pObject->value.lerp.targetY - pObject->value.lerp.startY) /
                    pObject->endFrame;
                dz = (pObject->value.lerp.targetZ - pObject->value.lerp.startZ) /
                    pObject->endFrame;
                if ((dx == 0) && (dy == 0) && (dz == 0)) {
                    pObject->curFrame = pObject->endFrame;
                    pJoint->scale.vx = pObject->value.lerp.targetX;
                    pJoint->scale.vy = pObject->value.lerp.targetY;
                    pJoint->scale.vz = pObject->value.lerp.targetZ;
                } else {
                    pObject->value.lerp.startX += dx;
                    pObject->value.lerp.startY += dy;
                    pObject->value.lerp.startZ += dz;
                    pJoint->scale.vx = pObject->value.lerp.startX;
                    pJoint->scale.vy = pObject->value.lerp.startY;
                    pJoint->scale.vz = pObject->value.lerp.startZ;
                    pObject->curFrame = 0;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ACCUMULATE:
                pObject->value.lerp.startX += pObject->value.lerp.targetX;
                pJoint->scale.vx += pObject->value.lerp.startX;
                pObject->value.lerp.startY += pObject->value.lerp.targetY;
                pJoint->scale.vy += pObject->value.lerp.startY;
                pObject->value.lerp.startZ += pObject->value.lerp.targetZ;
                pJoint->scale.vz += pObject->value.lerp.startZ;
                break;
            }

            pObject->curFrame++;
            if (pObject->curFrame >= pObject->endFrame) {
                if (pObject->loops == 0) {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_FINISHED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_FINISHED_ANY;
                    BattleFreeBlock(pPool, (BattleBlock*)pObject);
                    pJoint->pScaleObject = NULL;
                } else {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_LOOPED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_LOOPED_ANY;
                    pObject->curFrame = -1;
                    if ((pObject->opcode & LIBGEAR_JOINT_OPCODE_MASK) ==
                        LIBGEAR_JOINT_OBJECT_ACCUMULATE) {
                        pObject->value.lerp.targetX = 0;
                        pObject->value.lerp.targetY = 0;
                        pObject->value.lerp.targetZ = 0;
                    }
                }
            } else {
                if (pObject->type == objectType) {
                    result |= LIBGEAR_JOINT_UPDATE_ADVANCED;
                }
                result |= LIBGEAR_JOINT_UPDATE_ADVANCED_ANY;
            }

            pJoint->doRotate = TRUE;
            pJoint->doTransform = TRUE;
        }
    }

    return result;
}
*/
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
   Round 446 (-da/-dl allocno dump, gcc-2.6.0 cc1 on the .i with -O2 -G8): the swap is
   a global-alloc hand-out, not a local tie. The contested pair is pFrame (pseudo 73,
   POINTER, 39 refs / 78 insns -> 10.0) against the third rot temp (pseudo 87, 2 bytes,
   15 refs / 35 insns -> 2.57); the processing order is `75 150 73 87 86 85 148 84 74 77
   71 80 76 81 126 78 125 129 103 79`, so pFrame is handled THIRD and $v0/$a0 are
   already taken by pseudos 75 (the (countA+1)*3 address offset) and 150, which hands it
   $v1 - the register retail gives the third rot temp. Retail's $a1 is the parameter
   register coalesced in place (`addiu a1,a1,24`), and $a1 carries pData (pseudo 72, 6
   refs / 7 insns, block 0) whose local-allocno penalty in global-alloc's find_reg is what
   pushes pFrame off it. New spellings measured this round, all exactly 97.90% / 420 bytes
   and instruction-identical to the old body: `pData = (BattleAnimData*)((s16*)pData + 12);
   pFrame = (s16*)pData;` (gcc folds the copy+add straight back into `addiu v1,a1,24`), and
   try_flags screens 2.7.2-cdk 95.38 / -O3 97.9 / -G0 97.9 - no compiler harvests it.

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
// Round 459: re-measured, still 97.90 percent / 420 bytes = retail's 420 / 105 insns. Residual is exactly two things: (a) the pFrame-vs-countA+1-temp register swap - retail addiu a1,a1,24 in the bnez delay slot (pFrame coalesced onto the pData parameter register) with the skip temp in v1; ours puts pFrame in v1 and the temp in a1, an exact swap of the same two registers, all other insns equal; (b) retail's dead duplicated 'andi v0,t3,0xffff' in the loop-head bnez delay slot where we emit nop. Rounds 63-446 already ruled out every statement order, pointer spelling, width (s16/u16/s32 sweep), pData++ forms and two permuter runs (555/555, 360/360); the round-446 allocno dump shows pFrame handled third after pseudos 75/150 and handed v1. Nothing hand-editable is left; this needs a different allocator, not a spelling.
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
    pPool->pBlocks = HeapAlloc(count * sizeof(BattleBlock), 0);
    if (pPool->pBlocks != NULL) {
        BattleResetBlockPool(pPool);
        return pPool;
    }
    return NULL;
}
void BattleFreeBlockPool(BattleBlockPool* pPool) {
    BattleBlock* pBlocks = pPool->pBlocks;

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
        pBlock = pPool->pBlocks;
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
        pBlock = &pPool->pBlocks[pPool->usedCount];
        if (pBlock->inUse == 0) {
            pPool->usedCount++;
            while (pPool->usedCount < pPool->capacity &&
                   pPool->pBlocks[pPool->usedCount].inUse != 0) {
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
    index = (u32)((u8*)pBlock - (u8*)pPool->pBlocks) / sizeof(BattleBlock);
    if (index < pPool->usedCount) {
        pPool->usedCount = index;
    }
    pBlock->inUse = 0;
    return index;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2434);
/* 75.10%, 720 bytes = retail's 720 / 180 insns. Structure is retail's: the goto
   labels ARE retail's edges (the tag-0xFF case and the 0xFFFF case land on
   different tails: 0xFF jumps past the free block, 0xFFFF runs it), the
   `*(u32*)&pBlock->startX = *(u32*)&pBlock->startZ = pTable + pFrame[n]` chained
   store keeps retail's two word stores, and the loop is one `pFrame += 3` +
   `pSprites++` a pass with no early `if (limit <= 0) return 0` (retail's
   `blez 0x10(sp)` IS the for's entry test).
   Two deltas left, both measured to the end:
   1. the 5th parameter. Declared `u32 tag` (this body) the frame is retail's
      0x48 and every stack slot lines up, but retail reloads the argument with
      `lbu 0x58(sp)` per init block where we emit `lw`; declared `u8 tag` we get
      retail's `lbu` but gcc copies the argument once (`lbu 0x60(sp); sb 16(sp)`)
      and the frame becomes 0x50 -> 724 bytes / 181 insns (75.28%, wrong size).
      So the narrow RELOAD of a stack argument is the unreachable part.
   2. a three-register rotation among {s6,s7,fp}: retail flagA=s6, 0xFF=s7,
      endFrame=fp; we get flagA=fp, endFrame=s7, 0xFF=s6, and with it the
      `lhu`/`sh` order of the limit init block (we load limit first and store it
      before countA's load; retail loads countA first and stores limit after the
      slt). Moving `countB = pData->countB` after the `if` (retail's load order)
      measured 74.60%; `u16 flagA` (the different-signedness copy lever) grew the
      body to 183 insns / 732 bytes at 70.24%.
   The lib_gear twin (func_801DF7F4, identical asm) is parked at 70.90% - this
   body is the best measured for the function in either binary.
   The two 4-byte stores at +4/+8 both hold pTable + index for this opcode, which
   is why they are written as one u32 through the s16 startX/startZ fields. */
/* Round 427 (redo): re-measured the 75.10% body, then tried (a) the chained
   startX/startZ store replaced by a pValue temp with two statements, which does
   give retail's sw 4 / sw 8 order, (b) opcode-before-tag source order, (c) the
   `if (pBlock == NULL) {alloc; if (NULL) goto}` shape, which makes gcc invert the
   null test (bnez) and lose retail's delay-slot copy. (b)+(c) measured 74.68,
   (a) alone 75.12% / 720 bytes = retail's, which is the body below. u8 tag on
   this structure: 75.28% but 724 bytes / 181 insns (the frame grows to 0x50), so
   it cannot link - the narrow RELOAD of the stack argument stays unreachable and
   the rest is the {s6,s7,fp} rotation. */

/*
s32 func_800A2434(BattleBlockPool* pPool, BattleSprite* pSprites, BattleAnimData* pData, u32 flags,
                  u32 tag) {
    BattleBlock* pBlock;
    u16 countA;
    u16 limit;
    u16 countB;
    u16 animFlags;
    u16 endFrame;
    u16* pFrame;
    u8* pTable;
    u8* pBytes;
    u32 flagA;
    mem_addr pValue;
    s32 i;

    if (pData->unk06 != 0) {
        BattleFreeAllSpriteBlocks(pPool, pSprites);
        func_800A1B50(pSprites, pData);
        return 1;
    }
    flagA = flags & 1;
    countA = pData->countA;
    limit = pSprites->count;
    countB = pData->countB;
    if (countA + 1 < limit) {
        limit = countA + 1;
    }
    endFrame = *(u16*)((u8*)pData + 2);
    if (flagA == 0) {
        endFrame--;
    }
    animFlags = pData->flags;
    pFrame = (u16*)((u8*)pData + 0x18);
    pTable = (u8*)pFrame + (countA + 1) * 6;
    if (!(animFlags & 1)) {
        pTable += countA * 6;
    }
    if (!(animFlags & 2)) {
        pTable += countB * 6;
    }
    for (i = 0; i < limit; i++) {
        pBytes = (u8*)pFrame + 4;
        if (pFrame[0] != 0xFFFF) {
            pBlock = pSprites->p70;
            if (pBlock == NULL) {
                pBlock = BattleAllocBlock(pPool);
            } else if (pBlock->tag == 0xFF) {
                goto next1;
            }
            if (pBlock != NULL) {
                pBlock->inUse = 1;
                pBlock->flags = flagA;
                pBlock->tag = tag;
                pBlock->opcode = pBytes[0];
                pBlock->curFrame = 0;
                pBlock->endFrame = endFrame;
                pValue = (mem_addr)(pTable + pFrame[0]);
                *(u32*)&pBlock->startX = pValue;
                *(u32*)&pBlock->startZ = pValue;
                pSprites->p70 = pBlock;
            }
        } else {
            pBlock = pSprites->p70;
            if (pBlock != NULL && i != 0 && pBlock->tag != 0xFF) {
                BattleFreeBlock(pPool, pBlock);
                pSprites->p70 = NULL;
            }
        }
    next1:
        pFrame++;
        if (pFrame[0] != 0xFFFF) {
            pBlock = pSprites->p74;
            if (pBlock == NULL) {
                pBlock = BattleAllocBlock(pPool);
            } else if (pBlock->tag == 0xFF) {
                goto next2;
            }
            if (pBlock != NULL) {
                pBlock->inUse = 1;
                pBlock->flags = flagA;
                pBlock->tag = tag;
                pBlock->opcode = pBytes[1];
                pBlock->curFrame = 0;
                pBlock->endFrame = endFrame;
                pValue = (mem_addr)(pTable + pFrame[0]);
                *(u32*)&pBlock->startX = pValue;
                *(u32*)&pBlock->startZ = pValue;
                pSprites->p74 = pBlock;
            }
        } else {
            pBlock = pSprites->p74;
            if (pBlock != NULL && i != 0 && pBlock->tag != 0xFF) {
                BattleFreeBlock(pPool, pBlock);
                pSprites->p74 = NULL;
            }
        }
    next2:
        pFrame += 3;
        pSprites++;
    }
    return 0;
}
*/


/* The frame record is 6 bytes per sprite (u16 indexA, u16 indexB, u8 byteA,
   u8 byteB); indexA drives pSprites->p70 (rot at 0x54, loaded lhu, plus
   doTransform and doRotate), indexB drives p74 (pos at 0x5C, loaded lh and
   stored sw, plus doTransform only). Block init order in retail: inUse, flags,
   opcode, tag, curFrame, endFrame, then the same pointer value to +4 and +8,
   then the sprite's block field. */
/* 77.19%, 1036 bytes vs retail's 968 / 242 insns (was 69.38% at 1024 before
   this round). Body ported from the lib_gear twin func_801DFAC4 (identical asm,
   parked at 79.40%), translated to this unit's names; the twin's `goto` labels
   ARE retail's edges - the tag-0xFF case re-enters the 0xFFFF arm's own guard
   test (`skipRot`), so it still advances pValues/numA, and the rotation copy
   sits BEFORE the `if (pBlock != NULL)` fill test, as retail's .L800A288C ->
   .L800A28E0 order shows.
   Frame is retail's 0x78 and the stack slot map is retail's (pPool 0x10, countA
   0x20, countB 0x28, limit 0x30, endFrame 0x38).
   Deltas left:
   1. flagA (u32) is spilled to 0x40(sp) and reloaded for its own test
      (`sw a3,0x40(sp) ... lw t0,0x40(sp); bnez`) where retail keeps it in $fp;
      retail spends its 9 callee-saved registers on pValues/pSprites/pFrame/i/
      numB/numA/animFlags/pTable/flagA and spills pBytes and the hoisted 1, we
      spend them the same way but still lose one to flagA. `u16 flagA` was the
      lever tried for this on func_800A2434 and cost instructions there.
   2. the pSprites/pData permutation: retail s1=pSprites, s2=pData(->pFrame);
      ours s1=pData, s7=pSprites. Same inversion as func_800A2434 and as the
      lib_gear twin's own note - pData's short init-block live range outranks the
      walker.
   3. `u8 tag` gave retail's `lbu` reload but the parameter copy grew the frame
      to 0x80 -> 1040 bytes / 260 insns, 74.76%. `u32 tag` is parked.
   `pBytes = (u8*)pFrame + 4` as a loop local and the inline
   `((u8*)&pFrame[2])[n]` spelling compile identically (same 259 insns / 1036
   bytes); the local is kept for readability. */

INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2704);
/* Round 427 (redo): re-measured 77.19% / 1036 bytes / 259 insns against
   retail's 968 / 242, then found the surplus was a SPILL CASCADE, not the
   allocno permutation the old note blamed:
   - making the sprite walker the pSprites parameter itself (`pSprites++`, no
     pSprite local) removed the second sprite register and took it to
     80.86% / 996 bytes / 249 insns (the body below). pSprites was dead after
     the preheader but gcc kept a copy in $s7 only to hand it to the cursor in
     $s1 later; with one value the cursor coalesces, as retail's `addiu
     s1,s1,116` then `-4(s1)`/`0(s1)` shows.
   - assigning `flagA = flags & 1` AFTER the limit block (retail's own order)
     was worth 77.19 -> 79.84 / 1012 / 253 by itself.
   What is left is 7 instructions and one register short: retail spends its 9
   callee-saved registers on pValues/sprite-cursor/pFrame/i/numB/numA/animFlags/
   pTable/flagA, keeps endFrame in $fp and the hoisted constant 1 in a
   caller-saved $a1 with a `sw/lw 0x48(sp)` pair around each BattleAllocBlock
   call. Ours spends $s7 on the 1, so flagA is spilled to 0x38(sp) and reloaded
   for its own test and for both `sb`s, and endFrame lives at 0x30(sp). Every
   use of the 1 is a literal (inUse/doTransform/doRotate), so there is nothing
   source-visible to demote it with.
   Tried and measured on top of the body below: `s32 flagA` (same 79.84 at the
   time), `u8 flagA` (80.44 / 1000 / 250), and the old `pSprite` local
   (79.84). `u8 tag` is still the frame-grower (74.76) and the tag reload stays
   `lw 0x88(sp)` where retail has `lbu`. */
/*
s32 func_800A2704(BattleBlockPool* pPool, BattleSprite* pSprites, BattleAnimData* pData, u32 flags,
                  u32 tag) {
    BattleBlock* pBlock;
    u16 countA;
    u16 numA;
    u16 countB;
    u16 numB;
    u16 limit;
    u16 animFlags;
    u16 endFrame;
    u16* pFrame;
    u16* pValues;
    u8* pTable;
    u8* pBytes;
    u32 flagA;
    mem_addr pValue;
    s32 i;

    if (pData->unk06 != 0) {
        BattleFreeAllSpriteBlocks(pPool, pSprites);
        func_800A1B50(pSprites, pData);
        return 1;
    }
    countA = pData->countA;
    numA = 0;
    limit = pSprites->count;
    if (countA + 1 < limit) {
        limit = countA + 1;
    }
    countB = pData->countB;
    numB = 0;
    flagA = flags & 1;
    endFrame = *(u16*)((u8*)pData + 2);
    if (flagA == 0) {
        endFrame--;
    }
    animFlags = pData->flags;
    pFrame = (u16*)((u8*)pData + 0x18);
    pValues = pFrame + (countA + 1) * 3;
    pTable = (u8*)pValues;
    if (!(animFlags & 1)) {
        pTable += countA * 6;
    }
    if (!(animFlags & 2)) {
        pTable += countB * 6;
    }
    for (i = 0; i < limit; i++) {
        pBytes = (u8*)pFrame + 4;
        if (pFrame[0] != 0xFFFF) {
            pBlock = pSprites->p70;
            if (pBlock != NULL) {
                if (pBlock->tag == 0xFF) {
                    goto skipRot;
                }
            } else {
                pBlock = BattleAllocBlock(pPool);
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
                pBlock->endFrame = endFrame;
                pValue = (mem_addr)(pTable + pFrame[0]);
                *(u32*)&pBlock->startX = pValue;
                *(u32*)&pBlock->startZ = pValue;
                pSprites->p70 = pBlock;
            }
        } else {
        skipRot:
            if (!(animFlags & 1) && i != 0 && numA < countA) {
                pValues += 3;
                numA++;
            }
        }
        pFrame++;
        if (pFrame[0] != 0xFFFF) {
            pBlock = pSprites->p74;
            if (pBlock != NULL) {
                if (pBlock->tag == 0xFF) {
                    goto skipTrans;
                }
            } else {
                pBlock = BattleAllocBlock(pPool);
            }
            if (!(animFlags & 2) && i != 0 && numB < countB) {
                pSprites->pos[0] = *(s16*)pValues++;
                pSprites->pos[1] = *(s16*)pValues++;
                pSprites->pos[2] = *(s16*)pValues++;
                numB++;
                pSprites->doTransform = 1;
            }
            if (pBlock != NULL) {
                pBlock->inUse = 1;
                pBlock->flags = flagA;
                pBlock->tag = tag;
                pBlock->opcode = pBytes[1];
                pBlock->curFrame = 0;
                pBlock->endFrame = endFrame;
                pValue = (mem_addr)(pTable + pFrame[0]);
                *(u32*)&pBlock->startX = pValue;
                *(u32*)&pBlock->startZ = pValue;
                pSprites->p74 = pBlock;
            }
        } else {
        skipTrans:
            if (!(animFlags & 2) && i != 0 && numB < countB) {
                pValues += 3;
                numB++;
            }
        }
        pFrame += 2;
        pSprites++;
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
    BattleBlock* pBlocks = pPool->pBlocks;

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

    pVertex = pPool->pBlocks;
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
