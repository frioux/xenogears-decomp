#include "common.h"

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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F708);
/* 96.57% (140 vs 144 bytes): only the loop increment is misplaced. Retail has
   `addiu s1,s1,1` stolen into the `beqz a0` delay slot (before the count reload),
   gcc emits `lhu count; addiu s1,s1,1; slt` instead; every other instruction,
   the frame (0x28) and the register assignment (s2 = pSprites, s1 = i, s0 =
   &pSprite->p6C) already agree. Tried: do/while vs for vs while, index form
   (pSprites[i].p68), pointer form, unused 8-byte local (needed for the frame in
   the do-while form only). Matches 96.37% under gcc-2.6.0 too.
void func_8009F708(BattleSprite* pSprites) {
    BattleSprite* pSprite;
    s32 i;

    if (pSprites != NULL) {
        i = 0;
        pSprite = pSprites;
        while (i < pSprites->count) {
            if (pSprite->p68 != NULL) {
                HeapFree(pSprite->p68);
                pSprite->p68 = NULL;
                pSprite->p6C = NULL;
            }
            pSprite++;
            i++;
        }
        pSprites->count = 0;
        HeapFree(pSprites);
    }
}
*/
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2330);
/* 73.89% (168 vs 164 bytes): retail reloads usedCount from memory after each
   increment (lhu with no andi mask) and keeps pBlocks[index] in a3; gcc keeps
   the incremented value in a register, masks it, and re-hoists the loop.
BattleBlock* func_800A2330(BattleBlockPool* pPool) {
    BattleBlock* pBlock;
    BattleBlock* pBlocks;

    if (pPool->usedCount < pPool->capacity) {
        pBlock = &((BattleBlock*)pPool->pBlocks)[pPool->usedCount];
        if (pBlock->flag == 0) {
            pPool->usedCount++;
            pBlocks = (BattleBlock*)pPool->pBlocks;
            while (pPool->usedCount < pPool->capacity && pBlocks[pPool->usedCount].flag != 0) {
                pPool->usedCount++;
            }
            return pBlock;
        }
        return NULL;
    }
    return NULL;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A23E8);
/* 100.0 under gcc-2.6.0 (tools/scripts/try_flags.sh battle/main/misc21
   func_800A23E8 --gcc gcc-2.6.0-psx), 80.53 under this unit's XenoBattleDiv
   (2.7.2) preset, so the C below IS the match once the unit gets a 2.6.0 preset;
   under 2.7.2 it emits 0x48 bytes vs retail 0x4c (the `return -1` placement and
   mfhi landing in a2 instead of v1).
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2434);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2704);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2ACC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2BB8);
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
