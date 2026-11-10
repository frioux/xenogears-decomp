#include "common.h"
#include "system/model.h"

typedef struct {
    /* 0x0 */ u8* pBlocks;
    /* 0x4 */ u16 usedCount;
    /* 0x6 */ u16 capacity;
} BattleBlockPool; // Size: 0x8

typedef struct {
    /* 0x0 */ u8 flag;
    /* 0x1 */ u8 unk1[0x13];
} BattleBlock; // Size: 0x14

// Header of the 0x7C-stride entry pool allocated by func_800A2CA4.
typedef struct {
    /* 0x0 */ void* pEntries;
    /* 0x4 */ u16 capacity;
    /* 0x6 */ u16 usedCount;
} BattleEntryPool; // Size: 0x8

// Battle actor whose byte 3 marks it as released (0xFF).
typedef struct {
    /* 0x0 */ u8 pad0[0x3];
    /* 0x3 */ u8 unk3;
} BattleSlot; // Size: 0x4

typedef struct {
    /* 0x0  */ void* pUnk0;
    /* 0x4  */ void* pUnk4;
    /* 0x8  */ BattleSlot* pUnk8;
    /* 0xC  */ BattleSlot* pUnkC;
    /* 0x10 */ BattleSlot* pUnk10;
    /* 0x14 */ u8 pad14[0x68];
} BattleModelSlot; // Size: 0x7C

typedef struct {
    /* 0x0  */ u8 pad0[0xA];
    /* 0xA  */ u16 count;
    /* 0xC  */ u8 padC[0x5C];
    /* 0x68 */ BattleModelSlot slots[1]; // count sized
} BattleModelSlotTable; // Size: 0x68

// Model part array handed to the renderer, one entry per model part.
typedef struct {
    /* 0x0 */ ModelPart** pParts;
    /* 0x4 */ u32 numParts;
} BattleModelPartList; // Size: 0x8

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern void HeapChangeCurrentUser(u_int userTag, char** pContentTypes);
extern void func_800A22E8(BattleBlockPool* pPool);
extern s32 func_800A23E8(BattleBlockPool* pPool, void* pBlock);
extern void func_800A2D5C(BattleEntryPool* pPool);
extern void ModelPartFreeLightData(ModelPart* pModelPart);
BattleModelPartList* func_8009EBA8(ModelFileHeader* pModelFile, BattleModelPartList* pList) {
    u32 numParts;
    s32 i;

    HeapChangeCurrentUser(4, NULL);
    numParts = ModelResolvePointers(pModelFile);
    pList->pParts = HeapAlloc(numParts * 4, 0);
    pList->numParts = numParts;
    if (pList->pParts != NULL) {
        for (i = 0; i < numParts; i++) {
            pList->pParts[i] = &pModelFile->modelParts[i];
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
void func_8009F708(BattleModelSlotTable* pModel) {
    s32 i;

    if (pModel == NULL) {
        return;
    }
    for (i = 0; i < pModel->count; i++) {
        if (pModel->slots[i].pUnk0 != NULL) {
            HeapFree(pModel->slots[i].pUnk0);
            pModel->slots[i].pUnk0 = NULL;
            pModel->slots[i].pUnk4 = NULL;
        }
    }
    pModel->count = 0;
    HeapFree(pModel);
}

void func_8009F794(BattleModelPartList* pList, s32 freeLightData) {
    u32 i;

    if (pList == NULL) {
        return;
    }
    for (i = 0; i < pList->numParts; i++) {
        if (pList->pParts != NULL && pList->pParts[i] != NULL && freeLightData != 0) {
            ModelPartFreeLightData(pList->pParts[i]);
        }
    }
    if (pList->pParts != NULL) {
        HeapFree(pList->pParts);
        pList->pParts = NULL;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F844);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A0838);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1B50);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1CF4);
void func_800A216C(BattleBlockPool* pPool, BattleModelSlotTable* pModel, s32 index, u32 flags) {
    if (index >= pModel->count) {
        return;
    }
    if (pModel->slots[index].pUnk8 != NULL && (flags & 1)) {
        func_800A23E8(pPool, pModel->slots[index].pUnk8);
        pModel->slots[index].pUnk8 = NULL;
    }
    if (pModel->slots[index].pUnkC != NULL && (flags & 2)) {
        func_800A23E8(pPool, pModel->slots[index].pUnkC);
        pModel->slots[index].pUnkC = NULL;
    }
    if (pModel->slots[index].pUnk10 != NULL && (flags & 4)) {
        func_800A23E8(pPool, pModel->slots[index].pUnk10);
        pModel->slots[index].pUnk10 = NULL;
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
/* 80.53% (72 bytes): same insns, but gcc lays out the return -1 block last
   (beqz to the end) where retail branches to the body and puts the return -1
   block in the middle (bnez a1 + j epilogue). Also mfhi lands in a2, retail v1.
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
void func_800A2ACC(BattleBlockPool* pPool, BattleModelSlotTable* pModel) {
    BattleModelSlot* pSlot;
    s32 i;
    u16 count;

    count = pModel->count;
    for (i = 0; i < count; i++) {
        pSlot = &pModel->slots[i];
        if (pSlot->pUnk8 != NULL && pSlot->pUnk8->unk3 != 0xFF) {
            func_800A23E8(pPool, pSlot->pUnk8);
            pSlot->pUnk8 = NULL;
        }
        if (pSlot->pUnkC != NULL && pSlot->pUnkC->unk3 != 0xFF) {
            func_800A23E8(pPool, pSlot->pUnkC);
            pSlot->pUnkC = NULL;
        }
        if (pSlot->pUnk10 != NULL && pSlot->pUnk10->unk3 != 0xFF) {
            func_800A23E8(pPool, pSlot->pUnk10);
            pSlot->pUnk10 = NULL;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2BB8);
BattleEntryPool* func_800A2CA4(BattleEntryPool* pPool, s32 count) {
    HeapChangeCurrentUser(4, NULL);
    pPool->capacity = count;
    pPool->usedCount = 0;
    pPool->pEntries = HeapAlloc((count + 1) * 0x7C, 0);
    if (pPool->pEntries != NULL) {
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
