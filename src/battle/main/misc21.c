#include "common.h"
#include "psyq/libgpu.h"
#include "system/model.h"
#include "libgear/main.h"
#include "battle/actor.h"

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

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern void HeapChangeCurrentUser(u_int userTag, char** pContentTypes);
extern void BattleResetBlockPool(BattleBlockPool* pPool);
extern void func_800A2D5C(BattleSpritePool* pPool);
extern s32 BattleFreeBlock(BattleBlockPool* pPool, BattleBlock* pBlock);
extern u32 ModelResolvePointers(void* pModel);
extern void ModelPartFreeLightData(void* pLightData);
extern void ModelRenderPacket(void* pPart, void* pPacket, u_long* pOT, int flags);
extern void ModelAllocatePackets(ModelPart* pPart, void** ppPacket, void** ppPacketCopy);
extern void ModelInitializePacket(ModelPart* pPart, void* pPacket, s32 packetIndex);
extern void ModelPacketSetBaseTexPage(s16 x, s16 y);
extern void ModelPacketSetBaseClut(s16 x, s16 y);

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
BattleSprite* func_8009EC4C(BattleModelPartList* pMesh, BattleSkeletonEntry* pEntries, s32 packetIndex,
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
    pSprites[0].unk7 = 0;
    pSprites[0].jointIndex = 0xFFFF;
    pSprites[0].count = numSprites;
    pSprites[0].p68 = NULL;
    pSprites[0].p6C = NULL;
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
        pSprite->unk7 = 1;
        pSprite->scale.vx = 0x1000;
        pSprite->scale.vy = 0x1000;
        pSprite->scale.vz = 0x1000;
        pSprite->rotDirection = 0;
        pSprite->scale.pad = 0;
        pSprite->jointIndex = partIndex;

        if (partIndex != 0xFFFF) {
            ModelAllocatePackets(pMesh->pParts[partIndex], &pSprite->p68, &pSprite->p6C);
            if (pSprite->p68 == NULL) {
                BattleFreeModelSkeleton(pSprites);
                return NULL;
            }

            if (hasTexture) {
                ModelPacketSetBaseTexPage(texX, texY);
                ModelPacketSetBaseClut(clutX, clutY);
            }

            ModelInitializePacket(pMesh->pParts[partIndex], pSprite->p68, packetIndex);
            memcpy(pSprite->p6C, pSprite->p68, pMesh->pParts[partIndex]->unk20[1].modelPacketSize);
        } else {
            pSprite->p68 = NULL;
            pSprite->p6C = NULL;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009EF3C);
/* 94.94% (648 vs 624 bytes). Everything is recovered: struct offsets (pos[3] s32 at
   0x5C, rot[3] s16 at 0x54, scale[3] s16 at 0x4C, pParent void* at 0x0, dirty flags
   u8 at 0x4/0x5, rot-order flag u8 at 0x6), the three calls and their exact argument
   order, and both loops. Facts worth keeping:
   - the scratch matrix at PSX_SCRATCH is a SCALE matrix (m[0][0], m[1][1], m[2][2]
     = (scale * scale[i]) >> 12, other six elements 0); the pointer must be assigned
     BEFORE the RotMatrix if/else or gcc rematerialises lui at,0x1f80 per store.
   - prologue: RotMatrix(YXZ) writes pSprites->globalTransform (0x2C), MulMatrix0
     (globalTransform, scaleMatrix, localTransform), then localTransform.t =
     globalTransform.t. The loop instead rotates into localTransform (0x0C) and does
     CompMatrix(pParent + 0x2C, &localTransform, &globalTransform) - i.e. the third
     argument is the DESTINATION; the no-parent branch is globalTransform =
     localTransform (a 32-byte copy).
   - both loops are shape `for (i = 1; i < count; i++) { p++; ... }` with the pointer
     pre-increment at the TOP of the body - that is what reproduces retail's
     increments at the loop head and the store in the back-branch delay slot.
   - signature is (BattleSprite* pSprites, s32 scale): misc24's BattleStepActorAnimation calls it
     as func_8009EF3C(*(u32 *)(arg0 + 4), *(s16 *)(arg0 + 0x1C)), and the sibling that
     takes the same two arguments is func_8009F1C4 (rot-order flag 0x37 there).
   Remaining deltas (6 excess instructions, so not purely a tie): retail keeps the sprite walker in $s2, the
   base in $s4, i in $s1, count in $s6 and the constant 1 in $s7 (frame 0x38); this
   version gets walker $s4/$s2, i $s2, count $s5, 1 in $s6 (frame 0x30), and derives
   one of the three loop pointers with an extra addiu instead of retail's
   `move s0,s5 / addiu s0,s0,0x7C`. Six declaration orders and the two-pointer vs
   one-pointer walker shapes all land on the same tie.
s32 func_8009EF3C(BattleSprite* pSprites, s32 scale) {

    MATRIX* pScaleMatrix;
    BattleSprite* pSprite;
    BattleSprite* pWalk;
    u32 count;
    u32 i;

    pSprite = pSprites;
    pScaleMatrix = (MATRIX*)PSX_SCRATCH;
    pSprite->globalTransform.t[0] = pSprite->pos[0];
    pSprite->globalTransform.t[1] = pSprite->pos[1];
    pSprite->globalTransform.t[2] = pSprite->pos[2];
    count = pSprite->count;
    if (((u8*)pSprite)[6] != 0) {
        RotMatrixYXZ((SVECTOR*)pSprite->rot, &pSprite->globalTransform);
    } else {
        RotMatrix((SVECTOR*)pSprite->rot, &pSprite->globalTransform);
    }
    pScaleMatrix->m[0][0] = (scale * ((s16*)pSprite->unk4C)[0]) >> 12;
    pScaleMatrix->m[0][1] = 0;
    pScaleMatrix->m[0][2] = 0;
    pScaleMatrix->m[1][0] = 0;
    pScaleMatrix->m[1][1] = (scale * ((s16*)pSprite->unk4C)[1]) >> 12;
    pScaleMatrix->m[1][2] = 0;
    pScaleMatrix->m[2][0] = 0;
    pScaleMatrix->m[2][1] = 0;
    pScaleMatrix->m[2][2] = (scale * ((s16*)pSprite->unk4C)[2]) >> 12;
    MulMatrix0(&pSprite->globalTransform, pScaleMatrix, &pSprite->localTransform);
    pSprite->localTransform.t[0] = pSprite->globalTransform.t[0];
    pSprite->localTransform.t[1] = pSprite->globalTransform.t[1];
    pSprite->localTransform.t[2] = pSprite->globalTransform.t[2];
    pWalk = pSprite;
    for (i = 1; i < count; i++) {
        pWalk++;
        if (((u8*)pWalk)[5] != 0) {
            if (((u8*)pWalk)[6] != 0) {
                RotMatrixYXZ((SVECTOR*)pWalk->rot, &pWalk->localTransform);
            } else {
                RotMatrix((SVECTOR*)pWalk->rot, &pWalk->localTransform);
            }
            ((u8*)pWalk)[5] = 0;
        }
        if (((void**)pWalk)[0] != NULL && *((u8*)((void**)pWalk)[0] + 4) == 1) {
            ((u8*)pWalk)[4] = 1;
        }
        if (((u8*)pWalk)[4] != 0) {
            pWalk->localTransform.t[0] = pWalk->pos[0];
            pWalk->localTransform.t[1] = pWalk->pos[1];
            pWalk->localTransform.t[2] = pWalk->pos[2];
            if (((void**)pWalk)[0] != NULL) {
                CompMatrix((MATRIX*)((u8*)((void**)pWalk)[0] + 0x2C), &pWalk->localTransform,
                           &pWalk->globalTransform);
            } else {
                pWalk->globalTransform = pWalk->localTransform;
            }
        }
    }
    pSprite = pSprites;
    for (i = 1; i < count; i++) {
        pSprite++;
        ((u8*)pSprite)[4] = 0;
    }
    return count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F1C4);
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
            pPacket = ((void**)&pSkeleton->p68)[packetIndex];
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
            if (pSprite->p68 != NULL) {
                HeapFree(pSprite->p68);
                pSprite->p68 = NULL;
                pSprite->p6C = NULL;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F844);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A0838);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1B50);
/* 97.90% (420 vs 421 bytes in the body; the loop-head `nop` vs retail's dead
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
                if (pBlock == NULL || pBlock->unk1[2] != 0xFF) {
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
                if (pBlock == NULL || pBlock->unk1[2] != 0xFF) {
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
            pBlock->unk0 = 0;
            pBlock++;
        }
    }
}
BattleBlock* BattleAllocBlock(BattleBlockPool* pPool) {
    BattleBlock* pBlock;

    if (pPool->usedCount < pPool->capacity) {
        pBlock = &((BattleBlock*)pPool->pBlocks)[pPool->usedCount];
        if (pBlock->unk0 == 0) {
            pPool->usedCount++;
            while (pPool->usedCount < pPool->capacity &&
                   ((BattleBlock*)pPool->pBlocks)[pPool->usedCount].unk0 != 0) {
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
    pBlock->unk0 = 0;
    return index;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2434);
/* 83.63% (740 vs 720 bytes). Everything is recovered except two shapes:
   1. the 5th parameter (tag, u8 at 0x58(sp)) is reloaded by retail in each
      init block (`lbu t0,0x58(sp)`) while this version copies it once into a
      frame slot, which costs 8 bytes of frame (0x50 vs retail's 0x48) - that
      alone ends every diff; 2. retail stores `limit` (u16 at 0x10(sp)) twice,
      once before the branch, and tests it with blez, this version stores it
      only in the taken path and uses beqz. Struct view used below: block 0x00
      flag, 0x01, 0x02, 0x03 tag, 0x04/0x08 pointers, 0x10/0x12 s16; data 0x02
      u16, 0x04 flags, 0x06 u16, 0x0C countA, 0x0E countB, 0x18 frame records
      of 6 bytes (u16 idxA, u16 idxB, u8 byteA, u8 byteB). Note BattleFreeAllSpriteBlocks
      and BattleFreeSpriteBlocksByTag match ONLY with `unk1[2]` - naming that byte (e.g.
      `tag`) changes their induction variable and drops them to 91%.
      Round 63, from the sibling func_800A2704: there the same block-init code
      reloads the tag from the caller frame (`lbu 0x88(sp)`, twice) and hands it
      straight to `sb ...,3(a0)`, and countA/countB/limit/unk12 live in u16 stack
      slots (`sh` then `lhu` back, 0x20/0x28/0x30/0x38(sp)): so `pB[2] = tag` with
      a plain parameter use is retail's shape and the 0x50-vs-0x48 frame is stack
      pressure from the other locals, not the tag.
s32 func_800A2434(BattleBlockPool* pPool, BattleSprite* pSprites, BattleAnimData* pData, u32 flags,
                   u8 tag) {
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
            } else if (pBlock->unk1[2] == 0xFF) {
                pBlock = NULL;
            }
            if (pBlock != NULL) {
                pB = pBlock->unk1;
                pBlock->flag = 1;
                pB[0] = flagA;
                pB[2] = tag;
                pB[1] = pBytes[0];
                *(s16*)(pB + 0xF) = 0;
                *(s16*)(pB + 0x11) = unk12;
                *(u8**)(pB + 3) = *(u8**)(pB + 7) = pTable + *pIdx;
                pSprites->p70 = pBlock;
            }
        } else {
            pBlock = pSprites->p70;
            if (pBlock != NULL && i != 0 && pBlock->unk1[2] != 0xFF) {
                BattleFreeBlock(pPool, pBlock);
                pSprites->p70 = NULL;
            }
        }
        pIdx++;
        if (*pIdx != 0xFFFF) {
            pBlock = pSprites->p74;
            if (pBlock == NULL) {
                pBlock = BattleAllocBlock(pPool);
            } else if (pBlock->unk1[2] == 0xFF) {
                pBlock = NULL;
            }
            if (pBlock != NULL) {
                pB = pBlock->unk1;
                pBlock->flag = 1;
                pB[0] = flagA;
                pB[2] = tag;
                pB[1] = pBytes[1];
                *(s16*)(pB + 0xF) = 0;
                *(s16*)(pB + 0x11) = unk12;
                *(u8**)(pB + 3) = *(u8**)(pB + 7) = pTable + *pIdx;
                pSprites->p74 = pBlock;
            }
        } else {
            pBlock = pSprites->p74;
            if (pBlock != NULL && i != 0 && pBlock->unk1[2] != 0xFF) {
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
/*s32 func_800A2704(BattleBlockPool* pPool, BattleSprite* pSprites, BattleAnimData* pData, u32 flags,
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
            } else if (pBlock->unk1[2] == 0xFF) {
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
                pBlock->flag = 1;
                pBlock->unk1[0] = flagA;
                pBlock->unk1[2] = tag;
                pBlock->unk1[1] = pBytes[0];
                *(u16*)(pBlock->unk1 + 0xF) = 0;
                *(u16*)(pBlock->unk1 + 0x11) = unk12;
                *(u8**)(pBlock->unk1 + 3) = pTable + *pFrame;
                *(u8**)(pBlock->unk1 + 7) = pTable + *pFrame;
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
            } else if (pBlock->unk1[2] == 0xFF) {
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
                pBlock->flag = 1;
                pBlock->unk1[0] = flagA;
                pBlock->unk1[2] = tag;
                pBlock->unk1[1] = pBytes[1];
                *(u16*)(pBlock->unk1 + 0xF) = 0;
                *(u16*)(pBlock->unk1 + 0x11) = unk12;
                *(u8**)(pBlock->unk1 + 7) = (*(u8**)(pBlock->unk1 + 3) = pTable + *pFrame);
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
        if (pBlock != NULL && pBlock->unk3 != 0xFF) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->unk3 != 0xFF) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->unk3 != 0xFF) {
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
        if (pBlock != NULL && pBlock->unk3 == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->unk3 == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->unk3 == tag) {
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
        func_800A2D5C(pPool);
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
void func_800A2D5C(BattleSpritePool* pPool) {
    int i;
    int j;
    Vertex* pVertex;

    pVertex = (Vertex*)pPool->pBlocks;
    for (i = 0; i < pPool->count + 1; i++) {
        pVertex->unk16 = -1;
        pVertex->unk1E = 0;

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
