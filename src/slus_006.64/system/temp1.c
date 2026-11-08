#include "common.h"
#include "system/menu.h"
#include "system/animation.h"

// Sprite / Animation functions

// .sbss
s32 D_800591B8;
s32 D_800592EC;

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022B2C);
/* Best attempt: 96.30% (376 bytes; the original is 380). Instruction stream and control flow
   agree; the only difference is a `move a0,s0` that the original repeats at the head of the
   last block and this C omits because gcc carries a0 across the blez delay slot. Local `speed`
   must not be reused for the bounce term below or gcc keeps it in a callee-saved register.
extern s32 func_80022CAC(SpriteData* pSpriteData, s32 value);
extern void func_800BA8F4(void);

void func_80022B2C(SpriteData* pSpriteData) {
    s32 speed;
    s32 gravity;

    if (((*(u32*)((u8*)pSpriteData + 0x3C) >> 26) & 1) == 0) {
        func_800BA8F4();
        speed = pSpriteData->step.y;
        if (speed > 0 && (s32)pSpriteData->gravity > 0) {
            if (*(s16*)((u8*)pSpriteData + 0x6) != pSpriteData->field_0x84) {
                pSpriteData->position.y += func_80022CAC(pSpriteData, speed >> 4) << 4;
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
        pSpriteData->position.y += func_80022CAC(pSpriteData, speed >> 4) << 4;
        if ((pSpriteData->position.y >> 16) >= pSpriteData->field_0x84) {
            pSpriteData->position.y = pSpriteData->field_0x84 << 16;
        }
        pSpriteData->step.y += pSpriteData->gravity;
    } else {
        pSpriteData->position.y += func_80022CAC(pSpriteData, pSpriteData->step.y >> 4) << 4;
        pSpriteData->step.y += pSpriteData->gravity;
    }
}
*/

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

// non-prototype on purpose: func_80023170 below calls it without the frame index
extern void func_8001D2B0();

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

extern void func_80025180(void* pData);
extern void func_8001CE74(WorkListEntry* pTask);
extern void func_8001D034(WorkListEntry* pTask);

void func_80022EB8(AnimTask* pAnimTask) {
    WorkListEntry* pTask = &pAnimTask->task1;
    SpriteData* pSpriteData = pTask->unk4;

    if (pSpriteData->pBase) {
        if (pSpriteData->pBase->pFramesData) {
            func_80025180(pSpriteData->pBase->pFramesData);
        }
    }
    if ((*(u32*)((u8*)pSpriteData + 0x3C) & 3) == 1) {
        if (pSpriteData->pBase->pDirTransforms) {
            HeapFree(pSpriteData->pBase->pDirTransforms);
        }
    }
    if (((u32)pSpriteData->flags_0xAC >> 5) & 1) {
        func_8001CE74(pTask);
    }
    if (((u32)pSpriteData->flags_0xB0 >> 11) & 1) {
        func_8001D034(pTask);
    }
    if ((*(u32*)((u8*)pSpriteData + 0x3C) & 3) == 1) {
        func_8001D3F4(pSpriteData);
    }
    TimerWorkListRemoveTask(pTask);
    WorkListRemoveTask(pTask + 1);
    HeapFree(pAnimTask);
}

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
/* Best attempt: 79.47%. Param is an 8-byte struct of two s16 pairs (x1,y1,x2,y2); ratan2 is
   already declared long/long in psyq/libgte.h, so do not redeclare it (conflicting types).
   Code and registers agree; the original's frame is 0x10 bigger because it also stores both
   differences to dead stack slots at 0x10 and 0x18 (8 bytes apart) around the ratan2 call.
   Same result under gcc-2.6.0 (67.95) and gcc-2.7.2 (79.47), so it is source-level, not preset.
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
/* Best attempt: 98.57%. Only the register holding the 0xFFF1FFFF mask differs - the original
   keeps both masks in t1/t0 (a1 unavailable), this C lets the allocator use a1 for the second.
   Swapping the two `&=` statements makes the li order follow source, so the allocation order is
   source-driven; gcc picks t0 then a1/t1 here, never t1 then t0, whatever the statement order.
void func_80023170(SpriteData* pSpriteData, s32 arg1, s32 arg2, s32 arg3) {
    u32 flags = *(u32*)((u8*)pSpriteData + 0x3C);

    flags &= ~0x10;
    flags |= (arg3 & 1) << 4;
    flags &= ~8;
    flags |= (arg2 & 1) << 3;
    *(u32*)((u8*)pSpriteData + 0x3C) = flags;
    pSpriteData->animScriptWaitTimer = 0;
    pSpriteData->flags_0xA8 &= ~0x300000;
    pSpriteData->flags_0xA8 &= ~0xE0000;
    func_8001D2B0(pSpriteData);
}
*/

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

// objdiff stops at 99.41% because its table reference is `jtbl_80018664` and ours is `.rodata`;
// `tools/scripts/check_bin.sh slus_006.64` prints OK, so this definition is byte-identical.
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

void func_800234AC(SpriteData* pSpriteData) {
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

// SpriteData stuff
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023804);
/* Best attempt: 99.10%. Everything matches except a v0/v1 swap: the original keeps the
   flags_0xAC value in v1 and its 0xFFF8007F mask in v0, this C does the opposite (and
   so the flags_0xA8 value lands in v1 instead of v0). Function is 78 instructions.
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
    pSpriteData->flags_0xAC = (pSpriteData->flags_0xAC & 0xFFF8007F) | 0x8000;
    pSpriteData->flags_0xA8 &= 0xFFF1FFFF;
    cycles = D_80059198 + 1;
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
/* Best attempt: 97.22%. diff_func.sh needs 100.00%; the only differences are registers picked
   for the &D_8006BE10 / &D_8005A474 address loads in case 1 (the original materializes the
   %hi half in v0 and adds into s2, this C lets gcc load straight into s2, one insn shorter).
extern u8 D_8006BE10;
extern u8 D_8005A474;
extern s32 func_8001EE74(u16* pFlags);

AnimTask* func_80023A48(s32 arg0, s32 type, void* pAnimPackage, s32 size, void* pData) {
    AnimTask* pEntry;
    SpriteData* pSpriteData;
    s32 dataSize;

    switch (type) {
    case 0:
        dataSize = 0;
        pEntry = func_800233A4(pData, dataSize + size);
        func_80023950(&pEntry->spriteData);
        break;
    case 1:
        if (arg0 == 5) {
            pAnimPackage = &D_8006BE10;
        }
        if (arg0 == 6) {
            pAnimPackage = &D_8005A474;
        }
        dataSize = (func_8001EE74(*(u16**)pAnimPackage) - 1) * 24 + 0x58;
        pEntry = func_800233A4(pData, dataSize + size);
        func_800239F4(&pEntry->spriteData);
        break;
    case 2:
        dataSize = 0x54;
        pEntry = func_800233A4(pData, dataSize + size);
        func_80023958(&pEntry->spriteData);
        break;
    }
    pSpriteData = &pEntry->spriteData;
    pSpriteData->field_0x6C = pEntry;
    pSpriteData->allocatedDataSize = dataSize + 0xEC;
    pSpriteData->pVramData = pAnimPackage;
    return pEntry;
}
*/

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
/* Best attempt: 89.07%. Every operation, load, store and call is in place and in order; what is
   left is register allocation only. The original keeps pAnimFile in s6 for the tail (the 0x48
   store and the func_800222BC call) and a second copy in s1 for the 0x8 offset load, so it saves
   seven callee-saved registers; this C uses one register throughout. The 0x3C/0x40 flag block
   also lands in the wrong registers (mask in a0, D_800591AD in v1 instead of the reverse).
   Returns pSpriteData in v0, so the prototype above should be SpriteData* once defined.
extern void SpriteSetScale(SpriteData* pSpriteData, s16 scale);
extern s32 func_8001EE74(u16* pFlags);

SpriteData* func_8002435C(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4,
                          s16 arg5, s16 arg6) {
    SpriteDataI1* pBase;
    void* pFramesData;
    u8* pAnimations;
    u8* pAnimFileData;
    mem_addr pAnimHeader;
    u32 flags;

    func_80023804(pSpriteData);
    func_800239A0(pSpriteData);
    SpriteSetScale(pSpriteData, 0x1000);
    *(u32*)((u8*)pSpriteData + 0x3C) = (*(u32*)((u8*)pSpriteData + 0x3C) & ~3) | 1;
    pSpriteData->field_0x40 &= 0xFFFE1FFF;
    if (D_800591AD) {
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
    flags |= (D_800591B8 & 0xF) << 20;
    flags &= 0xFFF0FFFF;
    flags |= (D_800591B8 & 0xF) << 16;
    *(u32*)((u8*)pSpriteData + 0x3C) = flags;
    pAnimFileData = pAnimFile;
    pAnimHeader = *(u32*)(pAnimFileData + 8);
    pFramesData =
        HeapAlloc(func_8001EE74((u16*)(pAnimHeader + (mem_addr)pAnimFileData)) * sizeof(SpriteFrameData), 0);
    pBase = pSpriteData->pBase;
    pBase->pFramesData = pFramesData;
    pBase->pCurRenderFramesData = pFramesData;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.x = arg4;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.y = arg5;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->clut.x = arg2;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->clut.y = arg3;
    pSpriteData->pDefaultAnimFile = pAnimFile;
    func_800222BC(pSpriteData, pAnimFile);
    pAnimations = ((SpriteAnimPackage*)pSpriteData->pVramData)->pAnimations;
    pSpriteData->field_0x60 = (u32)pAnimations + ((*(u16*)pAnimations & 0x3F) + 1) * 2;
    func_800245D8(pSpriteData, 0);
    return pSpriteData;
}
*/

void func_80024524(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5) {
    SpriteData* pSpriteData;

    pSpriteData = HeapAlloc(sizeof(SpriteData), 0);
    pSpriteData->allocatedDataSize = sizeof(SpriteData);
    func_8002435C(pSpriteData, (void*)arg0, arg1, arg2, arg3, arg4, arg5);
}

extern void func_800222BC(SpriteData* pSpriteData, void* pAnimFile);
extern s32 func_8001EE68(void* pFrames);
extern void func_800223B0(SpriteData* pSpriteData, s16 arg1);
extern void func_80023538(SpriteData* pSpriteData, void* pAnimEntry);
extern u8 D_800591AD;

void func_800245D8(SpriteData* pSpriteData, s32 animId) {
    u8* pAnimations;
    u16 animOffset;

    if (pSpriteData->pDefaultAnimFile == NULL) {
        pSpriteData->pSpriteBytecode = NULL;
        return;
    }
    if (pSpriteData->pCurAnimFile == pSpriteData->pDefaultAnimFile) {
        pSpriteData->flags_0xB0 &= ~0x400;
    } else {
        pSpriteData->flags_0xB0 |= 0x400;
    }
    if (animId < 0) {
        func_800222BC(pSpriteData, pSpriteData->pSpecialAnimFile);
        if (D_800591AD && func_8001EE68(((SpriteAnimPackage*)pSpriteData->pVramData)->pFrames) == 0) {
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.y = 0x100;
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.x = 0x300;
        }
    } else {
        func_800222BC(pSpriteData, pSpriteData->pDefaultAnimFile);
        if (D_800591AD) {
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex = *(SVEC2*)((u8*)pSpriteData->field_0x7C + 0xE);
        }
    }
    *(u8*)((u8*)pSpriteData + 0xAF) = animId;
    if (animId < 0) {
        animId = ~animId;
    }
    pAnimations = ((SpriteAnimPackage*)pSpriteData->pVramData)->pAnimations;
    animOffset = *(u16*)(pAnimations + animId * 2 + 2);
    pSpriteData->field_0x40 |= 0x100000;
    pSpriteData->pCurAnimation = pAnimations + animOffset;
    func_80023538(pSpriteData, pSpriteData->pCurAnimation);
    func_800223B0(pSpriteData, pSpriteData->field_0x80);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80024730);
