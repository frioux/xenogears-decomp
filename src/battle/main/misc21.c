#include "common.h"
#include "psyq/libgpu.h"

typedef struct {
    /* 0x0 */ u8* pBlocks;
    /* 0x4 */ u16 usedCount;
    /* 0x6 */ u16 capacity;
} BattleBlockPool; // Size: 0x8

typedef struct {
    /* 0x0 */ u8 flag;
    /* 0x1 */ u8 unk1[0x13];
} BattleBlock; // Size: 0x14

typedef struct {
    /* 0x0 */ u8* pBlocks;
    /* 0x4 */ s16 count;
    /* 0x6 */ u16 unk6;
} BattleSpritePool; // Size: 0x8

typedef struct {
    /* 0x0 */ void** pParts;
    /* 0x4 */ u32 count;
} BattleModelPartList; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0xA];
    /* 0x0A */ u16 count;
    /* 0x0C */ MATRIX localTransform;
    /* 0x2C */ MATRIX globalTransform;
    /* 0x4C */ u8 unk4C[0x8];
    /* 0x54 */ s16 rot[3];
    /* 0x5A */ u16 pad5A;
    /* 0x5C */ s32 pos[3];
    /* 0x68 */ void* p68;
    /* 0x6C */ void* p6C;
    /* 0x70 */ BattleBlock* p70;
    /* 0x74 */ BattleBlock* p74;
    /* 0x78 */ BattleBlock* p78;
} BattleSprite; // Size: 0x7C

typedef struct {
    /* 0x00 */ u8 unk00[0x4];
    /* 0x04 */ u16 flags;
    /* 0x06 */ s16 unk06;
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

BattleModelPartList* BattleInitializeMesh(u8* pModel, BattleModelPartList* pList) {
    u32 count;
    u32 i;

    HeapChangeCurrentUser(4, NULL);
    count = ModelResolvePointers(pModel);
    pList->pParts = HeapAlloc(count * 4, 0);
    pList->count = count;
    if (pList->pParts != NULL) {
        for (i = 0; i < count; i++) {
            pList->pParts[i] = pModel + (0x10 + i * 0x38);
        }
    }
    return pList;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009EC4C);
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
        if (*(u16*)(pSkeleton->unk00 + 8) != 0xFFFF) {
            void* pPart;
            void* pPacket;

            MulMatrix0(pRotMatrix, &pSkeleton->globalTransform, pMatrix);
            SetLightMatrix(pMatrix);
            CompMatrix(pTransMatrix, &pSkeleton->globalTransform, pMatrix);
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);

            pPart = pMesh->pParts[*(u16*)(pSkeleton->unk00 + 8)];
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
/* 90.08% (424 vs 416 bytes), s32 rot f2f3 values + u16 counters. Offsets
   confirmed: rot[3] s16 at 0x54, pos[3] s32 at 0x5C, p70/p74 at 0x70/0x74,
   set-bytes at 0x4/0x5, frame records 3 s16 each; the guard is `andi
   count-1,0xFFFF` then `blez` (this version emits beqz) and retail keeps the
   sprite pointer rebased to sprite+4 with the 0x7C increment in the loop's
   first branch delay slot (increment at the top of the body) while every shape
   tried here folds the +0x80 into the entry. Remaining deltas: guard beqz vs
   blez, the fold, and two register assignments. BLOCK_ROTATION-shaped.
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
    s32 i;
    s32 v0;
    s32 v1;
    s32 v2;

    numA = 0;
    numB = 0;
    pFrame = (s16*)((u8*)pData + 0x18);
    if (pData->unk06 == 0) {
        pFrame += (pData->countA + 1) * 3;
    }
    countA = pData->countA;
    countB = pData->countB;
    animFlags = pData->flags;
    numSprites = pSprites->count - 1;
    if (numSprites < 1) {
        return;
    }
    pSprite = pSprites + 1;
    for (i = 0; i < numSprites; i++, pSprite++) {
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
                    pSprite->unk00[4] = 1;
                    pSprite->unk00[5] = 1;
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
                    pSprite->unk00[4] = 1;
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
            pBlock->flag = 0;
            pBlock++;
        }
    }
}
BattleBlock* BattleAllocBlock(BattleBlockPool* pPool) {
    BattleBlock* pBlock;

    if (pPool->usedCount < pPool->capacity) {
        pBlock = &((BattleBlock*)pPool->pBlocks)[pPool->usedCount];
        if (pBlock->flag == 0) {
            pPool->usedCount++;
            while (pPool->usedCount < pPool->capacity &&
                   ((BattleBlock*)pPool->pBlocks)[pPool->usedCount].flag != 0) {
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
    pBlock->flag = 0;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2704);
void BattleFreeAllSpriteBlocks(BattleBlockPool* pPool, BattleSprite* pSprites) {
    BattleBlock* pBlock;
    BattleSprite* pSprite;
    s32 i;
    s32 count;

    count = pSprites->count;
    pSprite = pSprites;
    for (i = 0; i < count; i++, pSprite++) {
        pBlock = pSprite->p70;
        if (pBlock != NULL && pBlock->unk1[2] != 0xFF) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->unk1[2] != 0xFF) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->unk1[2] != 0xFF) {
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
        if (pBlock != NULL && pBlock->unk1[2] == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->unk1[2] == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->unk1[2] == tag) {
            BattleFreeBlock(pPool, pBlock);
            pSprite->p78 = NULL;
        }
    }
}
BattleSpritePool* BattleCreateSpritePool(BattleSpritePool* pPool, s32 count) {
    HeapChangeCurrentUser(4, NULL);
    pPool->count = count;
    pPool->unk6 = 0;
    pPool->pBlocks = HeapAlloc((count + 1) * 0x7C, 0);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2D5C);
/* 65.07% (288 vs 284 bytes): structure and all offsets are recovered, but gcc
   writes the poly fields through the poly base (0xC,0xD,0x14...) while retail
   uses a sprite-based pointer (0x38,0x39,0x40...) walking 0x28, and retail's
   outer loop test is `lh count; i++; count+1; slt`. Symbol map for the next
   attempt: sprite+0x16 (s16) = -1, sprite+0x1E (s16) = 0, then two POLY_FT4 at
   sprite+0x2C and sprite+0x54: clut = GetClut(0, 0x1CD),
   tpage = GetTPage(0, 1, 0x380, 0), u0 = 0, v0 = 0xC1, u1 = 0, v1 = 0xC1,
   u2 = 0xF, v2 = 0xC1, u3 = 0xF, v3 = 0xC1; stride 0x7C, count+1 sprites.
void func_800A2D5C(BattleSpritePool* pPool) {
    BattleSprite* pSprite;
    POLY_FT4* pPoly;
    s32 i;
    s32 j;

    pSprite = (BattleSprite*)pPool->pBlocks;
    for (i = 0; i <= pPool->count; i++, pSprite++) {
        *(s16*)&pSprite->unk0C[0xA] = -1;
        *(s16*)&pSprite->unk0C[0x12] = 0;
        for (j = 0; j < 2; j++) {
            pPoly = (POLY_FT4*)((u8*)pSprite + 0x2C + j * 0x28);
            SetPolyFT4(pPoly);
            SetSemiTrans(pPoly, 1);
            pPoly->clut = GetClut(0, 0x1CD);
            pPoly->tpage = GetTPage(0, 1, 0x380, 0);
            pPoly->u2 = 0xF;
            pPoly->u0 = 0;
            pPoly->v0 = 0xC1;
            pPoly->u1 = 0;
            pPoly->v1 = 0xC1;
            pPoly->v2 = 0xC1;
            pPoly->u3 = 0xF;
            pPoly->v3 = 0xC1;
        }
    }
}
*/
