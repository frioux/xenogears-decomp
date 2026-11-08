#include "common.h"
#include "system/menu.h"
#include "system/animation.h"

// Sprite / Animation functions

// .sbss
s32 D_800591B8;
s32 D_800592EC;

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022B2C);

s32 func_80022CAC(SpriteData* pSpriteData, s32 value) {
    s32 scale = pSpriteData->field_0x3A;
    s32 result = value;

    if (scale) {
        // split so gcc keeps the sign-correction temp of the signed / 1024
        result = result * scale;
        result = result / 1024;
    }
    return result;
}

void func_80022CDC(SpriteData* pSpriteData) {
    pSpriteData->position.x += func_80022CAC(pSpriteData, pSpriteData->step.x >> 4) << 4;
    pSpriteData->position.z += func_80022CAC(pSpriteData, pSpriteData->step.z >> 4) << 4;
    func_80022B2C(pSpriteData);
}

extern void func_8001D2B0(SpriteData* pSpriteData, s32 frameIndex);

void func_80022D44(SpriteData* pSpriteData) {
    s32 animIndex;
    u16 animFlags;
    s32 frameIndex;

    // split so gcc keeps the shift result in the register of the masked index
    animIndex = (u32)pSpriteData->flags_0xA8 >> 11;
    animIndex &= 0x3F;
    if (animIndex < 0) {
        animIndex = 0;
    }
    animFlags = ((u16*)pSpriteData->field_0x54)[animIndex];
    frameIndex = animFlags & 0x1FF;
    if (animFlags & 0x200) {
        pSpriteData->flags_0xAC |= 8;
    } else {
        pSpriteData->flags_0xAC &= ~8;
    }
    *(u32*)((u8*)pSpriteData + 0x3C) = (*(u32*)((u8*)pSpriteData + 0x3C) & ~8) |
                                       (((((u32)pSpriteData->flags_0xAC >> 3) & 1) ^
                                         (((u32)pSpriteData->flags_0xAC >> 2) & 1))
                                        << 3);
    func_8001D2B0(pSpriteData, frameIndex);
}

extern void AnimScriptTick(SpriteData* pSpriteData);

// K&R definition: func_80022E8C below calls it with no arguments, as the original does.
void func_80022DF4(pEntry) WorkListEntry* pEntry;
{
    SpriteData* pSpriteData = pEntry->unk4;

    AnimScriptTick(pSpriteData);
    func_80022CDC(pSpriteData);
    if (pSpriteData->pSpriteBytecode == NULL) {
        pEntry->onFreeCallback(pEntry);
    } else if (((u32)pSpriteData->flags_0xAC >> 6) & 1) {
        AnimScriptTick(pSpriteData);
        func_80022CDC(pSpriteData);
        if (pSpriteData->pSpriteBytecode == NULL) {
            pEntry->onFreeCallback(pEntry);
        }
    }
}

void func_80022E8C(void) {
    D_800592EC++;
    func_80022DF4();
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022EB8);

void func_80022FC4(SpriteData* pSpriteData, s32 count, u_int allocFlags) {
    SpriteDataI1* pBase;
    void* pFramesData;

    if (pSpriteData->pBase->pFramesData) {
        HeapFree(pSpriteData->pBase->pFramesData);
    }
    pFramesData = HeapAlloc(count * sizeof(SpriteFrameData), allocFlags);
    pBase = pSpriteData->pBase;
    pBase->pFramesData = pFramesData;
    pBase->pCurRenderFramesData = pFramesData;
}

void func_8002303C(SpriteData* pSpriteData, s32 count, u_int allocFlags) {
    ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18 = HeapAlloc(count * 4, allocFlags);
    ((RECT*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)->y = ((RECT*)pSpriteData->pVramData)->h;
    ((RECT*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)->x = ((RECT*)pSpriteData->pVramData)->w;
}

extern void func_8001D3F4(SpriteData* pSpriteData);

void func_800230A8(SpriteData* pSpriteData) {
    switch (pSpriteData->flags_0xA8 & 1) {
    case 1:
        if (((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18) {
            HeapFree(((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18);
        }
        break;
    }
    func_8001D3F4(pSpriteData);
    HeapFree(pSpriteData->pBase->pFramesData);
    HeapFree(pSpriteData);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023124);
/* Best attempt: 79.47%. Param is an 8-byte struct of two s16 pairs (x1,y1,x2,y2). The original's
   frame is 0x10 bigger: it also spills both differences to 0x10/0x18 (dead stores) around the
   ratan2 call, which this C does not reproduce.
typedef struct {
    0x0 s16 x1;
    0x2 s16 y1;
    0x4 s16 x2;
    0x6 s16 y2;
} SVEC2Pair;

s32 func_80023124(SVEC2Pair line) {
    s32 dx = line.x1 - line.x2;
    s32 dy = line.y1 - line.y2;

    return -ratan2(dy, dx) & 0xFFF;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023170);

// Blocks whose data table sits at a self-relative byte offset stored at 0xC
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ u32 unk8;
    /* 0xC */ u32 dataOffset;
} AnimScriptLoopBlock;

u32 func_800231E0(AnimScriptLoopBlock* pBlock) {
    mem_addr offset = pBlock->dataOffset;

    return *(u32*)(offset + (mem_addr)pBlock);
}

u32 func_800231F8(AnimScriptLoopBlock* pBlock) {
    mem_addr offset = pBlock->dataOffset;

    return *(u32*)(offset + (mem_addr)pBlock + 4) + 1;
}

extern int D_80059198;
extern void func_800248D4(SpriteData* pSpriteData);

void AnimScriptTick(SpriteData* pSpriteData) {
    int i;

    for (i = 0; i != D_80059198 + 1; i++) {
        if (pSpriteData->animScriptWaitTimer) {
            pSpriteData->animScriptWaitTimer--;
            if (pSpriteData->animScriptWaitTimer == 0) {
                func_800248D4(pSpriteData);
            }
        }
    }
}

void func_80023290(SpriteData* pSpriteData, s32 direction) {
    s32 animType;
    s32 dir;

    *(u32*)((u8*)pSpriteData + 0x3C) =
        (*(u32*)((u8*)pSpriteData + 0x3C) & ~0xE0) | ((direction & 7) << 5);
    if (direction & 7) {
        pSpriteData->prim |= 2;
    } else {
        pSpriteData->prim &= ~2;
    }
    animType = (pSpriteData->field_0x40 >> 13) & 0xF;
    if (animType == 8 || animType == 9) {
        dir = (*(u32*)((u8*)pSpriteData + 0x3C) >> 5) & 7;
        if (dir) {
            *(u32*)((u8*)pSpriteData + 0x3C) =
                (*(u32*)((u8*)pSpriteData + 0x3C) & ~0xE0) | (((dir - 1) & 7) << 5);
        }
    } else {
        func_8001F6B0(pSpriteData);
    }
}

void func_80023340(SpriteData* pSpriteData, s32 count) {
    SpriteDataI1* pBase;
    void* pFramesData;

    HeapFree(pSpriteData->pBase->pFramesData);
    pFramesData = HeapAlloc(count * sizeof(SpriteFrameData), 0);
    pBase = pSpriteData->pBase;
    pBase->pCurRenderFramesData = pFramesData;
    pBase->pFramesData = pFramesData;
}

// Allocate struct stuff
AnimTask* func_800233A4(void* pData, int dataSize) {
    AnimTask* pEntry;
    WorkListEntry* pTask1;
    WorkListEntry* pTask2;

    pEntry = HeapAlloc(dataSize + 0xEC, D_800591AF);
    pTask1 = &pEntry->task1;
    pTask2 = &pEntry->task2;
    TimerWorkListAddTask(pData, pTask1);
    WorkListAddTask(pEntry, pTask2);
    func_80023804(&pEntry->spriteData);
    pTask1->unk4 = &pEntry->spriteData;
    pTask2->unk4 = &pEntry->spriteData;
    TimerWorkListSetTaskCallback(pEntry, &func_80022DF4);
    WorkListTaskSetOnFreeCallback(pEntry, &func_80022EB8);
    return pEntry;
}

s32 func_80023440(u16* pFlags) {
    u32 flags = *pFlags;
    s32 numDirections = (flags >> 8) & 0x7;

    if ((flags >> 14) & 0x1) {
        numDirections += 8;
    }
    return numDirections;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023468);
/* Best attempt: 99.41%. Code and jump-table contents agree with the original; diff_func.sh and
   objdiff stop at the .L case labels of the original, so 100.00% is not reachable through them.
s32 func_80023468(u_int index, s32 value) {
    s32 result = value;

    switch (index) {
    case 0:
    case 5:
    case 6:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        result = 1;
        break;
    case 1:
    case 4:
    case 8:
    case 9:
        result = 0;
        break;
    case 2:
    case 7:
    case 15:
        result = 2;
        break;
    }
    return result;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800234AC);
/* Best attempt: 98.29%, only the last store's address add has swapped v0/v1.
void func_800234AC(SpriteData* pSpriteData) {
    s32 i;

    for (i = 0; i != NUM_8DIR_MOVEMENT_DIRECTIONS; i++) {
        pSpriteData->pBase->pDirTransforms->dirTransforms[i].translationX = 0;
        pSpriteData->pBase->pDirTransforms->dirTransforms[i].translationY = 0;
        pSpriteData->pBase->pDirTransforms->dirTransforms[i].rotationX = 0;
        pSpriteData->pBase->pDirTransforms->dirTransforms[i].rotationY = 0;
        pSpriteData->pBase->pDirTransforms->dirTransforms[i].rotationZ = 0;
    }
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023538);

// SpriteData stuff
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023804);



void func_8002393C(SpriteDataI1* pBase) {
    pBase->rotation.x = 0;
    pBase->rotation.y = 0;
    pBase->rotation.z = 0;
    pBase->pFramesData = NULL;
}

void func_80023950(SpriteData* pSpriteData) {
    pSpriteData->pBase = NULL;
}

void func_80023958(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    func_8002393C(&pSpriteData->inner1);
    pSpriteData->pBase->pDirTransforms = NULL;
    // the store 0x40 into pBase lands on the first word of inner3
    *(u32*)((u8*)pSpriteData->pBase + 0x40) = 0;
}

void func_800239A0(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    func_8002393C(&pSpriteData->inner1);
    pSpriteData->field_0x7C = &pSpriteData->inner3;
    pSpriteData->pBase->pDirTransforms = &pSpriteData->directionTransforms;
    pSpriteData->pVramData = &pSpriteData->i2CurPackage;
    pSpriteData->pBase->pNextSpriteData = NULL;
}

void func_800239F4(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    func_8002393C(&pSpriteData->inner1);
    pSpriteData->pBase->pCurRenderFramesData = &pSpriteData->inner3;
    pSpriteData->pBase->pDirTransforms = NULL;
    pSpriteData->pBase->pNextSpriteData = NULL;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023A48);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023B84);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023FD8);

extern void func_80024524(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);

void func_80024294(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6) {
    D_800591B8 = arg6;
    func_80024524(arg0, arg1, arg2, arg3, arg4, arg5);
    D_800591B8 = 0;
}

extern void func_8002435C(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4, s16 arg5,
                          s16 arg6);

void func_800242F4(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4, s16 arg5,
                   s16 arg6, s32 arg7) {
    D_800591B8 = arg7;
    func_8002435C(pSpriteData, pAnimFile, arg2, arg3, arg4, arg5, arg6);
    D_800591B8 = 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_8002435C);

void func_80024524(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5) {
    SpriteData* pSpriteData;

    pSpriteData = HeapAlloc(sizeof(SpriteData), 0);
    pSpriteData->allocatedDataSize = sizeof(SpriteData);
    func_8002435C(pSpriteData, (void*)arg0, arg1, arg2, arg3, arg4, arg5);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800245D8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80024730);
