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

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern void HeapChangeCurrentUser(u_int userTag, char** pContentTypes);
extern void func_800A22E8(BattleBlockPool* pPool);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009EBA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009EC4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009EF3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F1C4);
void func_8009F5B0(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F5B8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F708);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F794);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_8009F844);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A0838);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1B50);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A1CF4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A216C);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2ACC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2BB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc21", func_800A2CA4);
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
