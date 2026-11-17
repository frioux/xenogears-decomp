#include "common.h"
#include "system/menu.h"
#include "system/animation.h"

// SpriteData's 0x3C flag word. The shared header only splits bits 20-31 into
// fields (bits 3-7 sit inside its 20-bit field_0x3C_0), so the bits this unit
// sets get names here; the word member is for the places that change two bits
// with one read-modify-write.
typedef union {
    /* 0x0 */ u32 flags;
    struct {
        /* 0x0 */ u32 unk0: 3;
        /* 0x3 */ u32 flipX: 1;
        /* 0x4 */ u32 flipY: 1;
        /* 0x5 */ u32 direction: 3;
        /* 0x8 */ u32 unk8: 24;
    } fields;
} SpriteFlagsWord;

typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ SpriteFlagsWord flags_0x3C;
} SpriteDataFlagsView;

#define g_SpriteFlags0x3C(pSpriteData) \
    (((SpriteDataFlagsView*)(pSpriteData))->flags_0x3C)

// Sprite / Animation functions

// .sbss
s32 g_SpritePendingClut;
s32 g_AnimTimerTickCount;

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022B2C);
/* Best attempt: 96.30% (376 bytes; the original is 380). Instruction stream and control flow
   agree. Two related deltas: (1) reorg steals the `move a0,s0` from the head of the else block
   (.L80022C68) into the `bnez` delay slot, so this C is one instruction short -- the original
   emits `lw a1,16(s0)` first in that block and keeps the move second; (2) in the bounce block the
   original keeps |step.y| in v1 and stores before the `bgez`, this C uses a1 and the store lands
   in the `bgez` delay slot. Tried: gcc 2.6.0/2.7.2/-O1..-O3/-G4..-G8 (try_flags sweep, best is the
   current preset at 96.30), and hoisting the unshifted `stepY = pSpriteData->step.y` into a local
   before the else-branch call (still 96.30, the load is not emitted first). Local `speed` must not
   be reused for the bounce term below or gcc keeps it in a callee-saved register. The else block's
   emission order is the whole problem: gcc emits `move a0,s0` BEFORE `lw a1,16(s0)` and reorg then
   steals the move into the bnez delay slot (a `lw` is not delay-slot eligible, so the original's
   block starts with the load and reorg stops there). Hoisting the shift into a local
   (s32 step = step.y >> 4) is folded away and does not change the order.

extern s32 SpriteScaleValue(SpriteData* pSpriteData, s32 value);
extern void BattleActorUpdateGroundHeightAndTriangle(void);

void func_80022B2C(SpriteData* pSpriteData) {
    s32 speed;
    s32 gravity;

    if (((*(u32*)((u8*)pSpriteData + 0x3C) >> 26) & 1) == 0) {
        BattleActorUpdateGroundHeightAndTriangle();
        speed = pSpriteData->step.y;
        if (speed > 0 && (s32)pSpriteData->gravity > 0) {
            if (*(s16*)((u8*)pSpriteData + 0x6) != pSpriteData->field_0x84) {
                pSpriteData->position.y += SpriteScaleValue(pSpriteData, speed >> 4) << 4;
                if ((pSpriteData->position.y >> 16) < pSpriteData->field_0x84) {
                    pSpriteData->step.y += pSpriteData->gravity;
                    return;
                }
                pSpriteData->position.y = pSpriteData->field_0x84 << 16;
                speed = -pSpriteData->step.y * (s32)(((u32)pSpriteData->flags_0xA8 >> 1) & 0x3FF) / 256;
                pSpriteData->step.y = speed;
                gravity = (s32)pSpriteData->gravity;
                if (speed < 0) {
                    speed = -speed;
                }
                if (gravity < 0) {
                    gravity = -gravity;
                }
                if (speed < gravity) {
                    pSpriteData->step.y = 0;
                }
                return;
            }
            return;
        }
        pSpriteData->position.y += SpriteScaleValue(pSpriteData, speed >> 4) << 4;
        if ((pSpriteData->position.y >> 16) >= pSpriteData->field_0x84) {
            pSpriteData->position.y = pSpriteData->field_0x84 << 16;
        }
        pSpriteData->step.y += pSpriteData->gravity;
    } else {
        pSpriteData->position.y += SpriteScaleValue(pSpriteData, pSpriteData->step.y >> 4) << 4;
        pSpriteData->step.y += pSpriteData->gravity;
    }
}
*/

s32 SpriteScaleValue(SpriteData* pSpriteData, s32 value) {
    s32 scale = pSpriteData->field_0x3A;
    s32 result = value;

    if (scale) {
        // split so gcc keeps the sign-correction temp of the signed / 1024
        result = result * scale;
        result = result / 1024;
    }
    return result;
}

void SpriteApplyMovementStep(SpriteData* pSpriteData) {
    pSpriteData->position.x += SpriteScaleValue(pSpriteData, pSpriteData->step.x >> 4) << 4;
    pSpriteData->position.z += SpriteScaleValue(pSpriteData, pSpriteData->step.z >> 4) << 4;
    func_80022B2C(pSpriteData);
}

// non-prototype on purpose: the second argument is a frame index, not in the header's prototype
extern void func_8001D2B0();

void SpriteSetFrameFromAnimIndex(SpriteData* pSpriteData) {
    u16* pAnimFlags;
    s32 animIndex;
    u16 animFlags;
    s32 frameIndex;

    // split so gcc keeps the shift result in the register of the masked index
    animIndex = (u32)pSpriteData->flags_0xA8 >> 11;
    animIndex &= 0x3F;
    if (animIndex < 0) {
        animIndex = 0;
    }
    // field_0x54 holds a raw address (animation_scripts.c stores one) to u16 frame entries
    pAnimFlags = (u16*)(mem_addr)pSpriteData->field_0x54;
    animFlags = pAnimFlags[animIndex];
    frameIndex = animFlags & 0x1FF;
    if (animFlags & 0x200) {
        pSpriteData->flags_0xAC.flags |= 8;
    } else {
        pSpriteData->flags_0xAC.flags &= ~8;
    }
    // the direction bit is set from the pair of animation flags, so this stays a
    // single read-modify-write of the word's flip field
    g_SpriteFlags0x3C(pSpriteData).fields.flipX = (((u32)pSpriteData->flags_0xAC.flags >> 3) & 1) ^
                                                 (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1);
    func_8001D2B0(pSpriteData, frameIndex);
}

extern void AnimScriptTick(SpriteData* pSpriteData);

// K&R definition: AnimTaskCountedTimerTick below calls it with no arguments, as the original does.
void func_80022DF4(pEntry) WorkListEntry* pEntry;
{
    SpriteData* pSpriteData = pEntry->unk4;

    AnimScriptTick(pSpriteData);
    SpriteApplyMovementStep(pSpriteData);
    if (pSpriteData->pSpriteBytecode == NULL) {
        pEntry->onFreeCallback(pEntry);
    } else if (((u32)pSpriteData->flags_0xAC.flags >> 6) & 1) {
        AnimScriptTick(pSpriteData);
        SpriteApplyMovementStep(pSpriteData);
        if (pSpriteData->pSpriteBytecode == NULL) {
            pEntry->onFreeCallback(pEntry);
        }
    }
}

void AnimTaskCountedTimerTick(void) {
    g_AnimTimerTickCount++;
    func_80022DF4();
}

extern void func_80025180(void* pData);
extern void WorkListsRemoveEntriesForData(WorkListEntry* pTask);
extern void TimerWorkListClearDataField70(WorkListEntry* pTask);

void AnimTaskFree(AnimTask* pAnimTask) {
    WorkListEntry* pTask = &pAnimTask->task1;
    SpriteData* pSpriteData = pTask->unk4;

    if (pSpriteData->pBase) {
        if (pSpriteData->pBase->pFramesData) {
            func_80025180(pSpriteData->pBase->pFramesData);
        }
    }
    if ((pSpriteData->field_0x3C_0 & 3) == 1) {
        if (pSpriteData->pBase->pDirTransforms) {
            HeapFree(pSpriteData->pBase->pDirTransforms);
        }
    }
    if (((u32)pSpriteData->flags_0xAC.flags >> 5) & 1) {
        WorkListsRemoveEntriesForData(pTask);
    }
    if (((u32)pSpriteData->flags_0xB0.flags >> 11) & 1) {
        TimerWorkListClearDataField70(pTask);
    }
    if ((pSpriteData->field_0x3C_0 & 3) == 1) {
        SpriteAnimFrameListRemove(pSpriteData);
    }
    TimerWorkListRemoveTask(pTask);
    WorkListRemoveTask(pTask + 1);
    HeapFree(pAnimTask);
}

void SpriteAllocFramesData(SpriteData* pSpriteData, s32 count, u_int allocFlags) {
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

void SpriteAllocClutRect(SpriteData* pSpriteData, s32 count, u_int allocFlags) {
    ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18 = HeapAlloc(count * 4, allocFlags);
    ((RECT*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)->y = ((RECT*)pSpriteData->pVramData)->h;
    ((RECT*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)->x = ((RECT*)pSpriteData->pVramData)->w;
}

extern void SpriteAnimFrameListRemove(SpriteData* pSpriteData);

void SpriteDestroy(SpriteData* pSpriteData) {
    switch (pSpriteData->flags_0xA8 & 1) {
    case 1:
        if (((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18) {
            HeapFree(((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18);
        }
        break;
    }
    SpriteAnimFrameListRemove(pSpriteData);
    HeapFree(pSpriteData->pBase->pFramesData);
    HeapFree(pSpriteData);
}

typedef struct {
    /* 0x0 */ s16 xStart;
    /* 0x2 */ s16 yStart;
    /* 0x4 */ s16 xEnd;
    /* 0x6 */ s16 yEnd;
} SLineEnds;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
} SDelta;

s32 SpriteComputeAngleFromPoints(SLineEnds line) {
    SDelta deltas[2];

    deltas[0].x = line.xStart - line.xEnd;
    deltas[1].x = line.yStart - line.yEnd;
    return -ratan2(deltas[1].x, deltas[0].x) & 0xFFF;
}

void SpriteSetFrameAndFlips(SpriteData* pSpriteData, s32 arg1, s32 arg2, s32 arg3) {
    // both flip bits change together, so the word is copied out, edited and stored back
    SpriteFlagsWord flags = g_SpriteFlags0x3C(pSpriteData);

    flags.fields.flipY = arg3 & 1;
    flags.fields.flipX = arg2 & 1;
    g_SpriteFlags0x3C(pSpriteData) = flags;
    pSpriteData->animScriptWaitTimer = 0;
    pSpriteData->flags_0xA8 &= ~0x300000;
    pSpriteData->flags_0xA8 &= ~0xE0000;
    // arg1 is passed through in a1, which is what keeps a1 out of the mask allocation above
    func_8001D2B0(pSpriteData, arg1);
}
// Blocks whose data table sits at a self-relative byte offset stored at 0xC
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ u32 unk8;
    /* 0xC */ u32 dataOffset;
} AnimScriptLoopBlock;

// The pair a block's 0xC offset points at, placed right after the block
typedef struct {
    /* 0x0 */ u32 target;
    /* 0x4 */ u32 count; // stored as count - 1
} AnimScriptLoopBlockData;

u32 AnimScriptGetBlockTarget(AnimScriptLoopBlock* pBlock) {
    AnimScriptLoopBlockData* pData = (AnimScriptLoopBlockData*)(pBlock->dataOffset + (mem_addr)pBlock);

    return pData->target;
}

u32 AnimScriptGetBlockCount(AnimScriptLoopBlock* pBlock) {
    AnimScriptLoopBlockData* pData = (AnimScriptLoopBlockData*)(pBlock->dataOffset + (mem_addr)pBlock);

    return pData->count + 1;
}

extern int g_AnimScriptExtraTicks;
extern void func_800248D4(SpriteData* pSpriteData);

void AnimScriptTick(SpriteData* pSpriteData) {
    int i;

    for (i = 0; i != g_AnimScriptExtraTicks + 1; i++) {
        if (pSpriteData->animScriptWaitTimer) {
            pSpriteData->animScriptWaitTimer--;
            if (pSpriteData->animScriptWaitTimer == 0) {
                func_800248D4(pSpriteData);
            }
        }
    }
}

void SpriteSetDirectionAndRefreshFrames(SpriteData* pSpriteData, s32 direction) {
    s32 animType;
    s32 dir;

    g_SpriteFlags0x3C(pSpriteData).fields.direction = direction & 7;
    if (direction & 7) {
        pSpriteData->prim |= 2;
    } else {
        pSpriteData->prim &= ~2;
    }
    animType = (pSpriteData->field_0x40 >> 13) & 0xF;
    if (animType == 8 || animType == 9) {
        dir = g_SpriteFlags0x3C(pSpriteData).fields.direction;
        if (dir) {
            g_SpriteFlags0x3C(pSpriteData).fields.direction = (dir - 1) & 7;
        }
    } else {
        func_8001F6B0(pSpriteData);
    }
}

void SpriteReallocFramesData(SpriteData* pSpriteData, s32 count) {
    SpriteDataI1* pBase;
    void* pFramesData;

    HeapFree(pSpriteData->pBase->pFramesData);
    pFramesData = HeapAlloc(count * sizeof(SpriteFrameData), 0);
    pBase = pSpriteData->pBase;
    pBase->pCurRenderFramesData = pFramesData;
    pBase->pFramesData = pFramesData;
}

// Allocate struct stuff
AnimTask* AnimTaskAlloc(void* pData, int dataSize) {
    AnimTask* pEntry;
    WorkListEntry* pTask1;
    WorkListEntry* pTask2;

    pEntry = HeapAlloc(dataSize + 0xEC, g_TaskHeapAllocFlags);
    pTask1 = &pEntry->task1;
    pTask2 = &pEntry->task2;
    TimerWorkListAddTask(pData, pTask1);
    WorkListAddTask(pEntry, pTask2);
    func_80023804(&pEntry->spriteData);
    pTask1->unk4 = &pEntry->spriteData;
    pTask2->unk4 = &pEntry->spriteData;
    TimerWorkListSetTaskCallback(pEntry, &func_80022DF4);
    WorkListTaskSetOnFreeCallback(pEntry, &AnimTaskFree);
    return pEntry;
}

s32 AnimEntryGetNumDirections(u16* pFlags) {
    u32 flags = *pFlags;
    s32 numDirections = (flags >> 8) & 0x7;

    if ((flags >> 14) & 0x1) {
        numDirections += 8;
    }
    return numDirections;
}

// objdiff stops at 99.41% because its table reference is `jtbl_80018664` and ours is `.rodata`;
// `tools/scripts/check_bin.sh slus_006.64` prints OK, so this definition is byte-identical.
// K&R style so func_80023FD8 can call it with the single argument retail passes.
s32 func_80023468(index, value) u_int index;
s32 value;
{
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

void SpriteClearDirTransforms(SpriteData* pSpriteData) {
    s32 i;

    for (i = 0; i != NUM_8DIR_MOVEMENT_DIRECTIONS; i++) {
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->translationX = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->translationY = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationX = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationY = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationZ = 0;
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023538);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023804);
/* Best attempt: 99.10% (312 bytes, every instruction identical except one register pair).
   Only the AC flag word differs: the original keeps the 0xFFF8007F mask in v0 and the loaded
   value in v1, gcc allocates the pair the other way round (and mirrors the 0xA8 word with it).
   The 0x8000 is anim index 0x100 in the bitfield at bit 7, so the original is probably a bitfield
   store, not this and/or. SpriteSetSpeedScale in animation_scripts.c compiles the same expression to
   mask=v1/value=v0 too, so the shape is right and the swap is context-driven allocation.
   Tried: statement swap (93.37%), A8 statement first (93.37%), constant-first operand order,
   mask in a declared local, `u32 maskAC = ...` initialised at the top of the function, `maskAC`
   assigned just before the load, loaded-word temp (96.67%), intermediate `ac = v | 0x8000`
   (93.78%), the `((0x100 & 0xFFF) << 7)` spelling, splitting into `&= mask;` then `|= 0x8000;`
   (gcc merges them, identical RTL), a `u32* pFlagsAC` local (82.27%), `pFlagsA8[1]`/`[0]`
   indexing (67.63%) and the (u8*)pSpriteData casts (70-67%). Every spelling that keeps the mask
   a constant leaves it in v1, so the choice is not source-order driven.

void func_80023804(SpriteData* pSpriteData) {
    u32* pFlagsA8;
    s32 cycles;

    *(u32*)((u8*)pSpriteData + 0x3C) = 0;
    pSpriteData->prim = 0x2D;
    pSpriteData->field_0x40 = 0;
    pSpriteData->field_0x3A = 0;
    pSpriteData->field_0x30 = 0;
    pSpriteData->direction = 0;
    pSpriteData->curSpriteFrame = 0;
    pSpriteData->flags_0xA8 = 0;
    *(u32*)((u8*)pSpriteData + 0x3C) &= ~4;
    *(u32*)((u8*)pSpriteData + 0x3C) &= ~8;
    *(u32*)((u8*)pSpriteData + 0x3C) &= ~0x10;
    *(u32*)((u8*)pSpriteData + 0x3C) &= 0xFF0FFFFF;
    *(u32*)((u8*)pSpriteData + 0x3C) &= 0xFEFFFFFF;
    *(u32*)((u8*)pSpriteData + 0x3C) &= 0xFFF0FFFF;
    *(u32*)((u8*)pSpriteData + 0x40) &= ~0xFC;
    *(u32*)((u8*)pSpriteData + 0x40) &= ~0x1F00;
    *(u32*)((u8*)pSpriteData + 0x40) &= 0xFFFE1FFF;
    pFlagsA8 = (u32*)&pSpriteData->flags_0xA8;
    pFlagsA8[1] = 0;
    pFlagsA8[2] = 0;
    *(u8*)((u8*)pSpriteData + 0xB0) = 0;
    *(u8*)((u8*)pSpriteData + 0xAF) = 0;
    pSpriteData->flags_0xAC.flags = (pSpriteData->flags_0xAC.flags & 0xFFF8007F) | ((0x100 & 0xFFF) << 7);
    pSpriteData->flags_0xA8 &= 0xFFF1FFFF;
    cycles = g_AnimScriptExtraTicks + 1;
    pSpriteData->gravity = (cycles * 0x4000) * cycles * pSpriteData->field_0x82 / 4096;
    pSpriteData->pSpriteBytecode = NULL;
    pSpriteData->field_0x70 = NULL;
    pSpriteData->pCurAnimFile = NULL;
    pSpriteData->field_0x68 = NULL;
    pSpriteData->field_0x80 = 0;
    pSpriteData->stackIndex = 0x10;
    pSpriteData->field_0x84 = 0;
    pSpriteData->field_0x6C = NULL;
    pSpriteData->field_0x50 = 0;
}
*/

void SpriteResetBase(SpriteDataI1* pBase) {
    pBase->rotation.x = 0;
    pBase->rotation.y = 0;
    pBase->rotation.z = 0;
    pBase->pFramesData = NULL;
}

void SpriteClearBase(SpriteData* pSpriteData) {
    pSpriteData->pBase = NULL;
}

void SpriteInitBaseAndClearInner3(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    SpriteResetBase(&pSpriteData->inner1);
    pSpriteData->pBase->pDirTransforms = NULL;
    // pBase points at inner1, so inner3 sits right after it
    ((SpriteDataI3*)(pSpriteData->pBase + 1))->field_0x0 = 0;
}

void SpriteInitBaseWithInternalStorage(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    SpriteResetBase(&pSpriteData->inner1);
    pSpriteData->field_0x7C = &pSpriteData->inner3;
    pSpriteData->pBase->pDirTransforms = &pSpriteData->directionTransforms;
    pSpriteData->pVramData = &pSpriteData->i2CurPackage;
    pSpriteData->pBase->pNextSpriteData = NULL;
}

void SpriteInitBaseWithRenderFrames(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    SpriteResetBase(&pSpriteData->inner1);
    pSpriteData->pBase->pCurRenderFramesData = &pSpriteData->inner3;
    pSpriteData->pBase->pDirTransforms = NULL;
    pSpriteData->pBase->pNextSpriteData = NULL;
}

extern u8 g_SpriteAnimPackageDirs5[];
extern u8 g_SpriteAnimPackageDirs6[];
extern s32 SpriteAnimPackageGetFrameCount(u16* pFlags);

AnimTask* AnimTaskCreate(s32 arg0, s32 type, void* pAnimPackage, s32 size, void* pData) {
    AnimTask* pEntry;
    SpriteData* pSpriteData;
    s32 dataSize;

    switch (type) {
    case 0:
        dataSize = 0;
        pEntry = AnimTaskAlloc(pData, dataSize + size);
        SpriteClearBase(&pEntry->spriteData);
        break;
    case 1:
        // indexed, not plain &g_SpriteAnimPackageDirs5: the index makes gcc build the address as
        // lui v0,%hi + addiu s2,v0,%lo (a temp for the high half) as the original does
        if (arg0 == 5) {
            pAnimPackage = &g_SpriteAnimPackageDirs5[0];
        }
        if (arg0 == 6) {
            pAnimPackage = &g_SpriteAnimPackageDirs6[0];
        }
        dataSize = (SpriteAnimPackageGetFrameCount(((SpriteAnimPackage*)pAnimPackage)->pFrames) - 1) *
                       sizeof(SpriteFrameData) + 0x58;
        pEntry = AnimTaskAlloc(pData, dataSize + size);
        SpriteInitBaseWithRenderFrames(&pEntry->spriteData);
        break;
    case 2:
        dataSize = 0x54;
        pEntry = AnimTaskAlloc(pData, dataSize + size);
        SpriteInitBaseAndClearInner3(&pEntry->spriteData);
        break;
    }
    pSpriteData = &pEntry->spriteData;
    pSpriteData->field_0x6C = pEntry;
    pSpriteData->allocatedDataSize = dataSize + 0xEC;
    pSpriteData->pVramData = pAnimPackage;
    return pEntry;
}


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023B84);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023FD8);
/* Best attempt: 85.83% (704 bytes; the original is 700). The body below is structurally right --
   every load, store, mask and call is in place and in order; what is left is register allocation.
   (1) The original gives pAnimFile s1 and jointTaskSize s0, this C the other way round, and that
   one swap renames s0/s1 in every later instruction. (2) The original keeps pSpriteData in t0 from
   `addiu t0,s0,0x38` to the end and reuses the dead s1 for pVramData (`lw s1,0x24(t0)`); this C
   uses a3 for the sprite pointer and t1 for pVramData. (3) The original materialises the 0x7FF80
   mask (lui a3,0x7 / ori a3,a3,0xff80) in the delay slot of the first beqz, this C loads it inline.
   Tried: indexed vs pointer table access, `(offset + base)` spelled as an int sum (this is what
   fixes the addu operand order, worth 77.05 -> 85.83), a `taskSize` local for the 4th argument.
   The call to func_80023468 passes ONE argument in retail (no `move a1,...` before the jal), which
   is why func_80023468 is defined K&R above; with a prototype the call costs an extra `move`.

extern u32 g_GfxWorkListCapacity;
extern void* D_800C3E1C;
extern u8 g_IsBattle;

AnimTask* func_80023FD8(s32 index, void* pAnimFile, ActorRotation* pOffset, s32 jointTaskSize) {
    u8* pAnimationOffsets;
    void* pAnimEntry;
    AnimTask* pAnimTask;
    SpriteData* pSpriteData;
    SpriteData* pTemplate;
    void* pVramData;
    s32 numDirections;
    s32 type;
    u32 flags;

    pAnimationOffsets = *(u8**)((u8*)pAnimFile + 0x10);
    pAnimEntry = (void*)(((u16*)pAnimationOffsets)[index + 1] + (mem_addr)pAnimationOffsets);
    numDirections = AnimEntryGetNumDirections((u16*)pAnimEntry);
    type = func_80023468(numDirections);
    pAnimTask = AnimTaskCreate(numDirections, type, pAnimFile, jointTaskSize, NULL);
    *(u32*)((u8*)pAnimTask + 0x14) |= 0x20000000;
    pSpriteData = &pAnimTask->spriteData;
    pVramData = pSpriteData->pVramData;
    pSpriteData->field_0x70 = NULL;
    pSpriteData->pTargetEntitySprite = NULL;
    if (g_IsBattle) {
        pTemplate = D_800C3E1C;
        if (pTemplate) {
            pSpriteData->pCurAnimFile = pTemplate->pCurAnimFile;
            pSpriteData->pDefaultAnimFile = pTemplate->pDefaultAnimFile;
            pSpriteData->pTargetEntitySprite = pTemplate->pTargetEntitySprite;
            pSpriteData->moveSpeed = pTemplate->moveSpeed;
            pSpriteData->direction = pTemplate->direction;
            pSpriteData->field_0x40 = (pSpriteData->field_0x40 & ~0x1F00) | (pTemplate->field_0x40 & 0x1F00);
            (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) & ~8) | ((*(u32*)((u8*)pTemplate + 0x3C)) & 8);
            (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) & ~0x10) | ((*(u32*)((u8*)pTemplate + 0x3C)) & 0x10);
            *(u8*)((u8*)pSpriteData + 0x3D) = *(u8*)((u8*)pTemplate + 0x3D);
            pSpriteData->scale = pTemplate->scale;
            (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) | 0x4000000) & ~4;
            pSpriteData->flags_0xAC.flags = (pSpriteData->flags_0xAC.flags & ~4) | (pTemplate->flags_0xAC.flags & 4);
            pSpriteData->flags_0xAC.flags =
                (pSpriteData->flags_0xAC.flags & 0xFFF8007F) | (pTemplate->flags_0xAC.flags & 0x7FF80);
            pSpriteData->field_0x7C = pTemplate->field_0x7C;
            pSpriteData->field_0x7C = pTemplate->field_0x7C;
            flags = ((u32)pTemplate->flags_0xA8 >> 30) | ((pTemplate->flags_0xAC.flags & 3) << 2);
            pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & 0x3FFFFFFF) | (flags << 30);
            pSpriteData->flags_0xAC.flags = (pSpriteData->flags_0xAC.flags & ~3) | (flags >> 2);
            pSpriteData->field_0x50 = pTemplate->field_0x50;
            *(u8*)((u8*)pSpriteData + 0x8D) = *(u8*)((u8*)pTemplate + 0xAF);
        }
    }
    pSpriteData->pCurAnimFile = NULL;
    pSpriteData->pDefaultAnimFile = NULL;
    pSpriteData->curSpriteFrame = 0;
    pSpriteData->pVramData = pVramData;
    pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFE1FFF) | ((numDirections & 0xF) << 13);
    (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) & ~3) | (type & 3);
    pSpriteData->field_0x82 = *(u16*)&g_GfxWorkListCapacity;
    pSpriteData->position.x = pOffset->vx << 16;
    pSpriteData->position.y = pOffset->vy << 16;
    pSpriteData->position.z = pOffset->vz << 16;
    func_80023538(pSpriteData, pAnimEntry);
    func_80024730(pAnimTask);
    return pAnimTask;
}
*/

extern void SpriteAllocFromAnimFile(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);

void SpriteAllocFromAnimFileWithClut(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6) {
    g_SpritePendingClut = arg6;
    SpriteAllocFromAnimFile(pAnimFile, arg1, arg2, arg3, arg4, arg5);
    g_SpritePendingClut = 0;
}

extern void func_8002435C(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4, s16 arg5,
                          s16 arg6);

void SpriteInitFromAnimFileWithClut(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4, s16 arg5,
                   s16 arg6, s32 arg7) {
    g_SpritePendingClut = arg7;
    func_8002435C(pSpriteData, pAnimFile, arg2, arg3, arg4, arg5, arg6);
    g_SpritePendingClut = 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_8002435C);
/* Best attempt: 99.19% (456 bytes, the original's size). Every instruction is present and in the
   original's order; the ONLY difference is one register pair: the original keeps the 0x3C flag
   word in v1 with the ~3 mask in v0, this C the other way round, and the 0x40 word follows it
   (lw v0,64 in the original, lw v1,64 here). Two source facts were needed to get this far and are
   load-bearing, keep them:
   (1) pAnimFileData = pAnimFile as the FIRST statement of the body. A plain local assigned where it
       is used is copy-propagated away, so the function saves six callee-saved registers instead of
       seven; with the copy first, gcc keeps pAnimFile in s6 for the tail (the 0x48 store and the
       SpriteSetAnimFile call) and a second copy in s1 for the 0x8 offset load, exactly as the
       original does. Worth 89.07 -> 92.17.
   (2) isBattle = g_IsBattle into a u8 local before the 0x3C read-modify-write. With the test read
       inline gcc emits that whole flag block as straight-line statement order; the local makes it
       interleave the way the original does (mask constants first, both loads, then the and/or/store
       pairs with the branch last). Worth 92.17 -> 99.19; found by the permuter.
   Tried for the last tie: the word in a local before the store (98.45, the interleave collapses),
   a u32 pointer local to 0x3C (99.19, folded away), a 2-bit bitfield store through the
   SpriteFlagsWord view (92.17, the interleave collapses again), the two statements swapped (91.73),
   s32 vs u8 for isBattle (99.19). Permuter: two 4-minute runs, the second (1582 iterations) found
   nothing beyond the isBattle local, so this is a GIV_TIE-shaped allocator tie.

extern void SpriteSetScale(SpriteData* pSpriteData, s16 scale);
extern s32 SpriteAnimPackageGetFrameCount(u16* pFlags);
extern u8 g_IsBattle;

SpriteData* func_8002435C(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4,
                          s16 arg5, s16 arg6) {
    SpriteDataI1* pBase;
    void* pFramesData;
    u8* pAnimations;
    u8* pAnimFileData;
    mem_addr pAnimHeader;
    u8 isBattle;
    u32 flags;

    pAnimFileData = pAnimFile;
    func_80023804(pSpriteData);
    SpriteInitBaseWithInternalStorage(pSpriteData);
    SpriteSetScale(pSpriteData, 0x1000);
    isBattle = g_IsBattle;
    *(u32*)((u8*)pSpriteData + 0x3C) = (*(u32*)((u8*)pSpriteData + 0x3C) & ~3) | 1;
    pSpriteData->field_0x40 &= 0xFFFE1FFF;
    if (isBattle) {
        pSpriteData->flags_0xA8 &= ~1;
        ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x8 = 0;
        ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0xC = 0;
    } else {
        pSpriteData->flags_0xA8 |= 1;
        ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18 = NULL;
    }
    flags = *(u32*)((u8*)pSpriteData + 0x3C);
    pSpriteData->field_0x6C = pSpriteData;
    flags &= 0xFF0FFFFF;
    flags |= (g_SpritePendingClut & 0xF) << 20;
    flags &= 0xFFF0FFFF;
    flags |= (g_SpritePendingClut & 0xF) << 16;
    *(u32*)((u8*)pSpriteData + 0x3C) = flags;
    pAnimHeader = *(u32*)(pAnimFileData + 8);
    pFramesData =
        HeapAlloc(SpriteAnimPackageGetFrameCount((u16*)(pAnimHeader + (mem_addr)pAnimFileData)) * sizeof(SpriteFrameData), 0);
    pBase = pSpriteData->pBase;
    pBase->pFramesData = pFramesData;
    pBase->pCurRenderFramesData = pFramesData;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.x = arg4;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.y = arg5;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->clut.x = arg2;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->clut.y = arg3;
    pSpriteData->pDefaultAnimFile = pAnimFile;
    SpriteSetAnimFile(pSpriteData, pAnimFile);
    pAnimations = ((SpriteAnimPackage*)pSpriteData->pVramData)->pAnimations;
    pSpriteData->field_0x60 = (u32)pAnimations + ((*(u16*)pAnimations & 0x3F) + 1) * 2;
    SpritePlayAnimation(pSpriteData, 0);
    return pSpriteData;
}
*/

void SpriteAllocFromAnimFile(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5) {
    SpriteData* pSpriteData;

    pSpriteData = HeapAlloc(sizeof(SpriteData), 0);
    pSpriteData->allocatedDataSize = sizeof(SpriteData);
    func_8002435C(pSpriteData, pAnimFile, arg1, arg2, arg3, arg4, arg5);
}

extern void SpriteSetAnimFile(SpriteData* pSpriteData, void* pAnimFile);
extern s32 SpriteAnimPackageTestFlag8000(void* pFrames);
extern void func_800223B0(SpriteData* pSpriteData, s16 arg1);
extern void func_80023538(SpriteData* pSpriteData, void* pAnimEntry);
extern u8 g_IsBattle;

void SpritePlayAnimation(SpriteData* pSpriteData, s32 animId) {
    u8* pAnimations;
    u16 animOffset;

    if (pSpriteData->pDefaultAnimFile == NULL) {
        pSpriteData->pSpriteBytecode = NULL;
        return;
    }
    if (pSpriteData->pCurAnimFile == pSpriteData->pDefaultAnimFile) {
        pSpriteData->flags_0xB0.flags &= ~0x400;
    } else {
        pSpriteData->flags_0xB0.flags |= 0x400;
    }
    if (animId < 0) {
        SpriteSetAnimFile(pSpriteData, pSpriteData->pSpecialAnimFile);
        if (g_IsBattle && SpriteAnimPackageTestFlag8000(((SpriteAnimPackage*)pSpriteData->pVramData)->pFrames) == 0) {
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.y = 0x100;
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.x = 0x300;
        }
    } else {
        SpriteSetAnimFile(pSpriteData, pSpriteData->pDefaultAnimFile);
        if (g_IsBattle) {
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex =
                *(SVEC2*)&((SpriteDataI3*)pSpriteData->field_0x7C)->texX;
        }
    }
    pSpriteData->flags_0xAC.bytes[3] = (u8)animId;
    if (animId < 0) {
        animId = ~animId;
    }
    pAnimations = ((SpriteAnimPackage*)pSpriteData->pVramData)->pAnimations;
    animOffset = ((u16*)pAnimations)[animId + 1];
    pSpriteData->field_0x40 |= 0x100000;
    pSpriteData->pCurAnimation = pAnimations + animOffset;
    func_80023538(pSpriteData, pSpriteData->pCurAnimation);
    func_800223B0(pSpriteData, pSpriteData->field_0x80);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80024730);
/* Best attempt: 53.46%. Shape facts recovered from the asm: the switch table covers 0..14
   (`sltiu $v0,$s1,0xF`), so cases 0-6 and 14 are explicit empty cases in the source, not a
   default; the bodies come out in source order 12, 10, 13, 11, 7, 8, 9 (12 and 13 are
   byte-identical duplicates that gcc did not cross-jump). `addiu $s2,$a0,0x1C` in the beqz
   delay slot means the source takes &pAnimTask->task2 into a local before the switch. Open
   gaps: the original copies the AnimTask argument to $a2 at entry (`addu $a2,$a0`) and passes
   it with `addu $a0,$a2`; a source-level extra local gets gcc to copy too but into $a1, and
   the position copies come out load-all-then-store-all instead of the original's interleaved
   `lw %lo/%lo+4/%lo+8; sw` with `addiu $v0,$v0,%lo` in the delay slot.
extern void BattleInitAnimTask(AnimTask* pAnimTask);
extern void SpriteSetTaskHandler(WorkListEntry* pTask, s32 animType);
extern IVEC3 g_BattleAnimPositionGroupA;
extern IVEC3 g_BattleAnimPositionGroupB;

void func_80024730(AnimTask* pAnimTask) {
    SpriteData* pSpriteData = &pAnimTask->spriteData;
    WorkListEntry* pTask = &pAnimTask->task2;
    s32 animType;

    animType = (pSpriteData->field_0x40 >> 13) & 0xF;
    switch (animType) {
    case 12:
        pSpriteData->curSpriteFrame = 1;
        animType = (animType - 2) & 0xF;
        pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFE1FFF) | (animType << 13);
        BattleInitAnimTask(pAnimTask);
        pSpriteData->position = g_BattleAnimPositionGroupA;
        break;
    case 10:
        BattleInitAnimTask(pAnimTask);
        pSpriteData->curSpriteFrame = 0;
        pSpriteData->position = g_BattleAnimPositionGroupA;
        break;
    case 13:
        pSpriteData->curSpriteFrame = 1;
        animType = (animType - 2) & 0xF;
        pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFE1FFF) | (animType << 13);
        BattleInitAnimTask(pAnimTask);
        pSpriteData->position = g_BattleAnimPositionGroupA;
        break;
    case 11:
        BattleInitAnimTask(pAnimTask);
        pSpriteData->curSpriteFrame = 0;
        pSpriteData->position = g_BattleAnimPositionGroupB;
        break;
    case 7:
        TimerWorkListSetTaskCallback(pAnimTask, &AnimTaskCountedTimerTick);
        break;
    case 8:
        pSpriteData->prim = 0x68;
        pSpriteData->curSpriteFrame = 1;
        break;
    case 9:
        pSpriteData->field_0x36 = 3;
        pSpriteData->prim = 0x60;
        pSpriteData->curSpriteFrame = 1;
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 14:
        break;
    }
    SpriteSetTaskHandler(pTask, animType);
}
*/
