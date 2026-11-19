#include "common.h"
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "system/animation.h"

// The archive-load task func_800BB760 queues (TimerWorkListAllocateTask payload 4):
// a timer work-list entry followed by the animation archive index it loads.
typedef struct {
    /* 0x00 */ WorkListEntry entry;
    /* 0x1C */ s32 archiveIndex;
} BattleArchiveLoadTask; // Size: 0x20

// The sprite colour pulse task (g_BattleSpriteColorTask): the work-list entry, the
// sprite it tints, and the angle it steps each frame.
typedef struct {
    /* 0x00 */ WorkListEntry entry;
    /* 0x1C */ SpriteData *pSpriteData;
    /* 0x20 */ s32 angle;
    /* 0x24 */ u8 unk24[0x4];
} BattleSpriteColorTask; // Size: 0x28

extern s32 g_BattlePendingAnimState;
extern s16 D_800C3740;
extern u8 g_BattleAnimCommandsSuppressed;

extern void func_800BC2F0(s32);
extern void HeapFree(void *pMem);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void func_8001F6B0(u8 *pEntry);
extern s32 rcos(s32 a);
extern s32 rsin(s32 a);
extern void SpriteSetIVec3(IVEC3 *pVector, s32 x, s32 y, s32 z);
extern s32 BattleGetActorHeight(s32 arg0);
extern s32 g_GfxOTDepthShift;
extern MATRIX g_BattleViewMatrix;
extern void TimerWorkListSetTaskCallback(void *pEntry, void *pCallback);
extern void func_800A9540(s32 arg0);
extern void func_800BB620(u8 *pEntry);
extern void func_800BB6E0(u8 *pEntry);
extern u8 *TimerWorkListAllocateTask(void *pData, s32 dataSize);
extern u8 g_FlagTimerWorkListAdds;
extern u8 g_TaskHeapAllocFlags;
extern s32 g_BattlePendingActorTaskCount;
extern u8 g_BattleAnimTaskAllocCount;
extern u32 D_800C3674;
extern s32 D_800C3678;
extern u8 g_BattleActiveAnimTaskCount;
extern s32 D_800C3CBC;
extern BattleSpriteColorTask *g_BattleSpriteColorTask;
// The anim tasks the two owner groups run; AnimTask (system/animation.h) is the
// record: entry, the entry the owner list links, then the sprite it animates.
extern AnimTask *g_BattleAnimOwnerTaskGroupA;
extern AnimTask *g_BattleAnimOwnerTaskGroupB;
extern u16 g_BattleAnimSavedGroupStates[];
extern u16 D_800C3CD4[];
extern u16 g_BattleAnimGroupStates[];
extern u16 D_800D30A8[];
extern void WorkListsRemoveEntriesForData(void *pTask);
extern void AnimScriptTick(SpriteData *pSprite);
extern void SpriteApplyMovementStep(SpriteData *pSprite);
extern void WorkListTaskSetOnFreeCallback(void *pEntry, void *pCallback);
extern u16 D_800C3CDC;
extern u16 D_80059454;
extern IVEC3 g_BattleAnimPositionGroupA;
extern IVEC3 g_BattleAnimPositionGroupB;
extern void func_800BC460(s32 arg0);
extern s32 D_800C3CC0;
extern void func_800BB350(s32 arg0);
extern void func_800A979C(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4);
extern void func_800A4654(MATRIX *pMatrix, SVECTOR *pA, SVECTOR *pB, s32 depth,
                          s32 primIndex, SVECTOR *pC, SVECTOR *pD, s32 scale);
extern SVECTOR g_BattleSlotVectors;
extern SVECTOR D_800D335C;
extern SVECTOR D_800C3730;
extern SVECTOR D_800C354C;
extern u8 D_800C372C;
extern u8 g_BattleStateWork[];

// The three actor-slot spawn positions the slot allocator hands out, one pair each.
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
} BattleActorSlotPos; // Size: 0x4

extern BattleActorSlotPos D_800C3668[3];
extern u16 D_800C3666;
extern u8 D_800C37CC;

// best 99.61% at retail's exact 204 bytes: everything matches but the three SSVECTOR
// loads. Retail puts the first in $v1 and the other two in $v0 (lh v1 / sh 0x10 /
// lh v0 / sh 0x12 / lh v0 / sh 0x14); one shared temp gives $v1 three times
// (three separate typed assignments give $v0 and 96.47%). The u16 store to field
// 0x36 must be VOLATILE or gcc forwards the register into the 'lhu 0x36' re-read
// as an 'andi 0xffff' (93.33%); retail really does reload it.
void func_800BB248(u8 *pTask) {
    u8 *pSpriteData;
    SVECTOR vec;
    long sxy;
    long flag;
    s32 nOTOffset;
    s32 mode;
    u16 *pField;
    u16 *pRead;
    s32 v;
    s32 v2;

    pSpriteData = *(u8 **)(pTask + 4);
    mode = *(u32 *)(pSpriteData + 0xA8) >> 30;
    pField = (u16 *)(pSpriteData + 0x36);
    *(volatile u16 *)pField = BattleGetActorHeight(((*(u32 *)(pSpriteData + 0xAC) & 3) << 2) | mode);
    pRead = (u16 *)(pSpriteData + 0x36);
    *(u16 *)(pSpriteData + 0x38) = *pRead >> 1;
    v = *(s16 *)(pSpriteData + 0x2);
    vec.vx = v;
    v2 = *(s16 *)(pSpriteData + 0x6);
    vec.vy = v2;
    v2 = *(s16 *)(pSpriteData + 0xA);
    vec.vz = v2;
    SetRotMatrix(&g_BattleViewMatrix);
    SetTransMatrix(&g_BattleViewMatrix);
    nOTOffset = (RotTransPers(&vec, &sxy, (u_long *)&sxy, &flag) >> g_GfxOTDepthShift) +
                *(s16 *)(pSpriteData + 0x30);
    if (flag & 0x8000) {
        nOTOffset = 0;
    }
    *(u16 *)(pSpriteData + 0x2E) = nOTOffset;
}

void BattleFreeActorSpriteTask(u8 *pEntry) {
    u32 unused[2];
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB350);
// best 88.12% at 232 bytes (retail 224): the slot-mask loop and everything up to the
// tail match. The gap is 'g_BattleAnimTaskAllocCount--' followed by
// 'func_800BB350(*(u32 *)(pTask + 0x1C))': retail reads the byte, then loads the
// argument, then subtracts, then calls with the store in the jal's delay slot
// (lbu / lw a0 / addiu / jal / sb). Swapping the two statements gives retail's exact
// 224 bytes but the call lands before the decrement (80.70%).
/*
void func_800BB540(u8 *pTask) {
    s32 i;
    u16 slotBit;
    BattleActorSlotPos *pSlotPos;

    for (i = 0, slotBit = 1; i != 3; i++, slotBit <<= 1) {
        if ((D_800C3666 & slotBit) == 0) {
            D_800C3666 |= slotBit;
            break;
        }
    }
    pSlotPos = &D_800C3668[i];
    func_800A979C(*(u32 *)(pTask + 0x1C), pSlotPos->x, pSlotPos->y, 0,
                  (s16)(*(u16 *)(pTask + 0x1C) + 0x1C0));
    g_BattleAnimTaskAllocCount--;
    func_800BB350(*(u32 *)(pTask + 0x1C));
    ((WorkListEntry *)pTask)->onFreeCallback(pTask);
    g_BattlePendingActorTaskCount--;
    if (g_BattlePendingActorTaskCount == 0) {
        D_800C37CC = 1;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB540);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB620);
void BattleAnimTaskAllocArchiveEntry(BattleArchiveLoadTask *pTask) {
    func_800A9540(pTask->archiveIndex);
    g_BattleAnimTaskAllocCount++;
    TimerWorkListSetTaskCallback(pTask, func_800BB620);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB6E0);
// best 88.71% under cdk -G0 (44.87% under 2.7.2 -G8): the frame is 40 and every
// instruction is present, only the global-base registers differ (we keep
// &g_FlagTimerWorkListAdds in s0 with the saved byte in s3, retail uses s1/s4), plus one
// load-delay nop after lw g_BattlePendingActorTaskCount that retail fills with the pEntry store.
/*
void func_800BB760(u8 *pEntry) {
    u8 saved;
    u8 *pTask;

    saved = g_FlagTimerWorkListAdds;
    g_FlagTimerWorkListAdds = 0;
    g_TaskHeapAllocFlags = 1;
    pTask = TimerWorkListAllocateTask(NULL, 4);
    TimerWorkListSetTaskCallback(pTask, func_800BB6E0);
    g_BattlePendingActorTaskCount++;
    *(u8 **)(pTask + 0x1C) = pEntry;
    g_TaskHeapAllocFlags = 0;
    g_FlagTimerWorkListAdds = saved;
}
*/
// best 94.74% at retail's exact 152 bytes: every store is in the right place except
// that retail fills the load-delay slot of 'lw v0,g_BattlePendingActorTaskCount'
// with the pEntry store ('lw / sw s3,0x1C(s0) / sb zero,g_TaskHeapAllocFlags').
// Ours hoists that store above the load; moving it after the increment instead
// leaves a nop in the delay slot and 156 bytes (88.71%), and a 4-minute permuter
// run from the 94.74% body found nothing (base=60 best=60).
/*
void func_800BB760(u8 *pEntry) {
    u8 saved;
    u8 *pTask;

    saved = g_FlagTimerWorkListAdds;
    g_FlagTimerWorkListAdds = 0;
    g_TaskHeapAllocFlags = 1;
    pTask = TimerWorkListAllocateTask(NULL, 4);
    TimerWorkListSetTaskCallback(pTask, func_800BB6E0);
    *(u8 **)(pTask + 0x1C) = pEntry;
    g_TaskHeapAllocFlags = 0;
    g_BattlePendingActorTaskCount++;
    g_FlagTimerWorkListAdds = saved;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB760);
void BattleResetAnimState(void) {
    D_800C3674 = 0x200;
    D_800C3678 = -1;
    g_BattleActiveAnimTaskCount = 0;
    D_800C3CBC = 1;
    func_800BC2F0(0);
}
void BattleBuildMatrixFromThreeVectors(MATRIX *pMatrix, SVECTOR *pA, SVECTOR *pB, SVECTOR *pC) {
    VECTOR vTmp;
    VECTOR vB;
    VECTOR vC;
    VECTOR vD;

    SpriteSetIVec3((IVEC3 *)&vTmp, pB->vx - pA->vx, pB->vy - pA->vy, pB->vz - pA->vz);
    vD.vx = pC->vx;
    vD.vy = pC->vy;
    vD.vz = pC->vz;
    VectorNormal(&vTmp, &vB);
    OuterProduct12(&vD, &vB, &vTmp);
    VectorNormal(&vTmp, &vC);
    OuterProduct12(&vB, &vC, &vTmp);
    VectorNormal(&vTmp, &vD);
    pMatrix->m[0][0] = vC.vx;
    pMatrix->m[0][1] = vC.vy;
    pMatrix->m[0][2] = vC.vz;
    pMatrix->m[1][0] = vD.vx;
    pMatrix->m[1][1] = vD.vy;
    pMatrix->m[1][2] = vD.vz;
    pMatrix->m[2][0] = vB.vx;
    pMatrix->m[2][1] = vB.vy;
    pMatrix->m[2][2] = vB.vz;
    PushMatrix();
    ApplyMatrix(pMatrix, pA, &vTmp);
    pMatrix->t[0] = -vTmp.vx;
    pMatrix->t[1] = -vTmp.vy;
    pMatrix->t[2] = -vTmp.vz;
    PopMatrix();
}
// best 84.11% at retail's exact 228 bytes: the instruction multiset is identical,
// only the argument-setup order differs. Retail interleaves the work-block pointer
// build with the two stack stores (lw a3, move a2, sw 0x14, sw 0x18, lw v1,
// li 4096, sw 0x1C, jal, sw 0x10) where we emit both loads before the stores, and
// with -fschedule-insns on statement order does not move it. Both spellings of the
// work pointer matter: 'g_BattleStateWork + 0x8000' as ONE expression folds into
// the address addend (220 bytes), so the base must be a mem_addr local that is
// assigned and then advanced. The three translation words are the matrix's t[]
// reached from 0x20 before it, which is why retail holds 'matrix - 0x20' in $a0
// and uses displacements 0x34/0x38/0x3C.
/*
void func_800BB9D4(void) {
    s32 *pViewTrans;
    mem_addr pWork;
    s32 depth;
    s32 primIndex;

    BattleBuildMatrixFromThreeVectors(&g_BattleViewMatrix, &g_BattleSlotVectors, &D_800D335C,
                                      &D_800C3730);
    pViewTrans = (s32 *)((u8 *)&g_BattleViewMatrix - 0x20);
    pViewTrans[0xD] += D_800C354C.vx;
    pViewTrans[0xE] += D_800C354C.vy;
    pViewTrans[0xF] += D_800C354C.vz;
    if (D_800C372C == 0) {
        pWork = (mem_addr)g_BattleStateWork;
        pWork += 0x8000;
        depth = *(s32 *)(pWork + 0xC54);
        primIndex = *(s32 *)(pWork + 0xC84);
        func_800A4654(&g_BattleViewMatrix, NULL, NULL, depth, primIndex, &g_BattleSlotVectors,
                      &D_800D335C, 0x1000);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB9D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBAB8);
void BattleFreeAnimTask(AnimTask *pTask) {
    SpriteData *pSprite;

    pSprite = pTask->task1.unk4;
    if ((pSprite->field_0x40 >> 13 & 0xF) == 0xA) {
        if (g_BattleAnimOwnerTaskGroupA == pTask) {
            g_BattleAnimOwnerTaskGroupA = NULL;
            if (g_BattleAnimCommandsSuppressed == 0) {
                g_BattleAnimGroupStates[0] = g_BattleAnimSavedGroupStates[0];
                g_BattleAnimGroupStates[1] = g_BattleAnimSavedGroupStates[1];
                g_BattleAnimGroupStates[2] = g_BattleAnimSavedGroupStates[2];
            }
        }
    } else if (g_BattleAnimOwnerTaskGroupB == pTask) {
        g_BattleAnimOwnerTaskGroupB = NULL;
        if (g_BattleAnimCommandsSuppressed == 0) {
            D_800D30A8[0] = D_800C3CD4[0];
            D_800D30A8[1] = D_800C3CD4[1];
            D_800D30A8[2] = D_800C3CD4[2];
        }
    }
    if (((u32)pSprite->flags_0xAC.flags >> 5) & 1) {
        WorkListsRemoveEntriesForData(pTask);
    }
    TimerWorkListRemoveTask(pTask);
    WorkListRemoveTask(&pTask->task2);
    HeapFree(pTask);
    g_BattleActiveAnimTaskCount--;
    if (g_BattleActiveAnimTaskCount == 0) {
        func_800BC2F0(g_BattlePendingAnimState);
    }
}
void BattleAnimTaskTick(AnimTask *pTask) {
    SpriteData *pSprite;

    pSprite = pTask->task1.unk4;
    AnimScriptTick(pSprite);
    SpriteApplyMovementStep(pSprite);
    if ((pSprite->field_0x40 >> 13 & 0xF) == 0xA) {
        g_BattleAnimPositionGroupA.x = pSprite->position.x;
        g_BattleAnimPositionGroupA.y = pSprite->position.y;
        g_BattleAnimPositionGroupA.z = pSprite->position.z;
    } else {
        g_BattleAnimPositionGroupB.x = pSprite->position.x;
        g_BattleAnimPositionGroupB.y = pSprite->position.y;
        g_BattleAnimPositionGroupB.z = pSprite->position.z;
    }
    if (pSprite->pSpriteBytecode == NULL) {
        pTask->task1.onFreeCallback(pTask);
        return;
    }
    if ((((u32)pSprite->flags_0xAC.flags >> 6) & 1) == 0) {
        return;
    }
    AnimScriptTick(pSprite);
    SpriteApplyMovementStep(pSprite);
    if ((pSprite->field_0x40 >> 13 & 0xF) == 0xA) {
        g_BattleAnimPositionGroupA.x = pSprite->position.x;
        g_BattleAnimPositionGroupA.y = pSprite->position.y;
        g_BattleAnimPositionGroupA.z = pSprite->position.z;
    } else {
        g_BattleAnimPositionGroupB.x = pSprite->position.x;
        g_BattleAnimPositionGroupB.y = pSprite->position.y;
        g_BattleAnimPositionGroupB.z = pSprite->position.z;
    }
    if (pSprite->pSpriteBytecode == NULL) {
        pTask->task1.onFreeCallback(pTask);
    }
}
void BattleInitAnimTask(AnimTask *pTask) {
    SpriteData *pSprite;
    AnimTask *pOwner;

    pSprite = &pTask->spriteData;
    if ((pSprite->field_0x40 >> 13 & 0xF) == 0xA) {
        pOwner = g_BattleAnimOwnerTaskGroupA;
        if (pOwner != NULL) {
            if ((u16)pOwner->spriteData.curSpriteFrame != 1 && (u16)pSprite->curSpriteFrame == 1) {
                pOwner->task1.onFreeCallback(pOwner);
                g_BattleAnimOwnerTaskGroupA = pTask;
            } else {
                pSprite->animScriptWaitTimer = 0;
                pSprite->pSpriteBytecode = NULL;
            }
        } else {
            g_BattleAnimOwnerTaskGroupA = pTask;
            g_BattleAnimSavedGroupStates[0] = g_BattleAnimGroupStates[0];
            g_BattleAnimSavedGroupStates[1] = g_BattleAnimGroupStates[1];
            g_BattleAnimSavedGroupStates[2] = g_BattleAnimGroupStates[2];
        }
    } else {
        pOwner = g_BattleAnimOwnerTaskGroupB;
        if (pOwner != NULL) {
            if ((u16)pOwner->spriteData.curSpriteFrame != 1 && (u16)pSprite->curSpriteFrame == 1) {
                pOwner->task1.onFreeCallback(pOwner);
                g_BattleAnimOwnerTaskGroupB = pTask;
            } else {
                pSprite->animScriptWaitTimer = 0;
                pSprite->pSpriteBytecode = NULL;
            }
        } else {
            g_BattleAnimOwnerTaskGroupB = pTask;
            D_800C3CD4[0] = D_800D30A8[0];
            D_800C3CD4[1] = D_800D30A8[1];
            D_800C3CD4[2] = D_800D30A8[2];
        }
    }
    g_BattleActiveAnimTaskCount++;
    if (((u32)pSprite->flags_0xAC.flags >> 2) & 1) {
        pSprite->direction = 0x800;
    } else {
        pSprite->direction = 0;
    }
    WorkListTaskSetOnFreeCallback(pTask, BattleFreeAnimTask);
    TimerWorkListSetTaskCallback(pTask, BattleAnimTaskTick);
    func_800BC2F0(2);
}
// best 97.55% at 260 bytes (retail 264): every instruction but one. Retail reaches
// the second position group with 'addu a0,a0,8' off the SAME register that holds
// &g_BattleAnimGroupStates; a compile-time-indexed spelling folds the +8 into the
// displacement (260 bytes), and a pointer-walk spelling ('pStates += 4') restores
// the addu but then the scheduler cannot fill the last lh's delay slot and the body
// grows to 268. Case-body order is source order: case 4 first, then case 2.
// NOTE g_BattleAnimGroupStates must be declared s16 to get retail's 'lh' here; that
// change is byte-neutral for BattleFreeAnimTask and BattleInitAnimTask (both stayed
// 100.00% at 312 and 408 bytes).
/*
void func_800BC2F0(s32 state) {
    D_800C3CC0 = state;
    D_800C3CBC = 1;
    switch (state) {
    case 4:
        D_800C3CBC = 5;
        break;
    case 2:
        g_BattleAnimPositionGroupA.x = g_BattleAnimGroupStates[0] << 16;
        g_BattleAnimPositionGroupA.y = g_BattleAnimGroupStates[1] << 16;
        g_BattleAnimPositionGroupA.z = g_BattleAnimGroupStates[2] << 16;
        g_BattleAnimPositionGroupB.x = g_BattleAnimGroupStates[4] << 16;
        g_BattleAnimPositionGroupB.y = g_BattleAnimGroupStates[5] << 16;
        g_BattleAnimPositionGroupB.z = g_BattleAnimGroupStates[6] << 16;
        break;
    default:
        if (g_BattleAnimOwnerTaskGroupA != NULL) {
            g_BattleAnimOwnerTaskGroupA->task1.onFreeCallback(g_BattleAnimOwnerTaskGroupA);
            g_BattleAnimOwnerTaskGroupA = NULL;
        }
        if (g_BattleAnimOwnerTaskGroupB != NULL) {
            g_BattleAnimOwnerTaskGroupB->task1.onFreeCallback(g_BattleAnimOwnerTaskGroupB);
            g_BattleAnimOwnerTaskGroupB = NULL;
        }
        break;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC2F0);
void BattleSetAnimTaskDoneState(s32 value) {
    g_BattlePendingAnimState = value;
}
void BattleBeginAnimTask(u8 *pEntry) {
    if (g_BattleAnimCommandsSuppressed == 0) {
        func_800BC2F0(1);
        func_800BC460((s32)pEntry);
    }
    D_80059454 = D_800C3CDC;
}
void BattleSetAnimTaskShort(s16 value) {
    D_800C3740 = value;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC460);

void BattleSetAnimState4(void) {
    if (g_BattleAnimCommandsSuppressed == 0) {
        func_800BC2F0(4);
    }
}

void BattleSetAnimState1(void) {
    if (g_BattleAnimCommandsSuppressed == 0) {
        func_800BC2F0(1);
    }
}

void BattleSetSpriteColorFromAngle(u8 *pEntry, s32 angle) {
    s32 color;

    color = rcos(angle << 6);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pEntry[0x28] = color;
    pEntry[0x29] = color;
    pEntry[0x2A] = color;
    func_8001F6B0(pEntry);
}
void BattleFreeSpriteColorTask(void) {
    BattleSpriteColorTask *pTask;

    pTask = g_BattleSpriteColorTask;
    if (pTask != NULL) {
        pTask->pSpriteData->prim |= 1;
        TimerWorkListRemoveTask(pTask);
        HeapFree(pTask);
        g_BattleSpriteColorTask = NULL;
    }
}

void BattleSpriteColorTaskTick(BattleSpriteColorTask *pTask) {
    s32 color;
    s32 angle;
    SpriteData *pSpriteData;
    /* the incoming $a0 only survives the angle load; the rest reads a saved copy */
    BattleSpriteColorTask *pSaved = pTask;

    angle = pTask->angle << 6;
    pSpriteData = pSaved->pSpriteData;
    color = rcos(angle);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pSpriteData->red = color;
    color = rsin(angle);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pSpriteData->green = color;
    pSpriteData->blue = color;
    func_8001F6B0(pSpriteData);
    pSaved->angle += 1;
}
