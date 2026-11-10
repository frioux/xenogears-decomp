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
    /* 0x0C */ u8 unk0C[0x5C];
    /* 0x68 */ void* p68;
    /* 0x6C */ void* p6C;
    /* 0x70 */ BattleBlock* p70;
    /* 0x74 */ BattleBlock* p74;
    /* 0x78 */ BattleBlock* p78;
} BattleSprite; // Size: 0x7C

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern void HeapChangeCurrentUser(u_int userTag, char** pContentTypes);
extern void func_800A22E8(BattleBlockPool* pPool);
extern void func_800A2D5C(BattleSpritePool* pPool);
extern s32 func_800A23E8(BattleBlockPool* pPool, BattleBlock* pBlock);
extern u32 ModelResolvePointers(void* pModel);
extern void ModelPartFreeLightData(void* pLightData);

BattleModelPartList* func_8009EBA8(u8* pModel, BattleModelPartList* pList) {
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F1C4);
void func_8009F5B0(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F5B8);
void func_8009F708(BattleSprite* pSprites) {
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
void func_8009F794(BattleModelPartList* pList, s32 freeLight) {
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1CF4);
void func_800A216C(BattleBlockPool* pPool, BattleSprite* pSprites, s32 index, u32 flags) {
    BattleSprite* pSprite;

    pSprite = pSprites;
    if (index < pSprite->count) {
        pSprite += index;
        if (pSprite->p70 != NULL && (flags & 1)) {
            func_800A23E8(pPool, pSprite->p70);
            pSprite->p70 = NULL;
        }
        if (pSprite->p74 != NULL && (flags & 2)) {
            func_800A23E8(pPool, pSprite->p74);
            pSprite->p74 = NULL;
        }
        if (pSprite->p78 != NULL && (flags & 4)) {
            func_800A23E8(pPool, pSprite->p78);
            pSprite->p78 = NULL;
        }
    }
}
BattleBlockPool* func_800A2234(BattleBlockPool* pPool, s32 count) {
    if (count <= 0) {
        return NULL;
    }
    pPool->capacity = count;
    HeapChangeCurrentUser(4, NULL);
    pPool->pBlocks = HeapAlloc(count * 0x14, 0);
    if (pPool->pBlocks != NULL) {
        func_800A22E8(pPool);
        return pPool;
    }
    return NULL;
}
void func_800A22A8(BattleBlockPool* pPool) {
    u8* pBlocks = pPool->pBlocks;

    pPool->usedCount = 0;
    if (pBlocks != NULL) {
        HeapFree(pBlocks);
    }
    pPool->pBlocks = NULL;
}
void func_800A22E8(BattleBlockPool* pPool) {
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
BattleBlock* func_800A2330(BattleBlockPool* pPool) {
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
s32 func_800A23E8(BattleBlockPool* pPool, BattleBlock* pBlock) {
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2704);
void func_800A2ACC(BattleBlockPool* pPool, BattleSprite* pSprites) {
    BattleBlock* pBlock;
    BattleSprite* pSprite;
    s32 i;
    s32 count;

    count = pSprites->count;
    pSprite = pSprites;
    for (i = 0; i < count; i++, pSprite++) {
        pBlock = pSprite->p70;
        if (pBlock != NULL && pBlock->unk1[2] != 0xFF) {
            func_800A23E8(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->unk1[2] != 0xFF) {
            func_800A23E8(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->unk1[2] != 0xFF) {
            func_800A23E8(pPool, pBlock);
            pSprite->p78 = NULL;
        }
    }
}
void func_800A2BB8(BattleBlockPool* pPool, BattleSprite* pSprites, u8 tag) {
    BattleBlock* pBlock;
    BattleSprite* pSprite;
    s32 i;
    s32 count;

    count = pSprites->count;
    pSprite = pSprites;
    for (i = 0; i < count; i++, pSprite++) {
        pBlock = pSprite->p70;
        if (pBlock != NULL && pBlock->unk1[2] == tag) {
            func_800A23E8(pPool, pBlock);
            pSprite->p70 = NULL;
        }
        pBlock = pSprite->p74;
        if (pBlock != NULL && pBlock->unk1[2] == tag) {
            func_800A23E8(pPool, pBlock);
            pSprite->p74 = NULL;
        }
        pBlock = pSprite->p78;
        if (pBlock != NULL && pBlock->unk1[2] == tag) {
            func_800A23E8(pPool, pBlock);
            pSprite->p78 = NULL;
        }
    }
}
BattleSpritePool* func_800A2CA4(BattleSpritePool* pPool, s32 count) {
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
void func_800A2D1C(BattleBlockPool* pPool) {
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
