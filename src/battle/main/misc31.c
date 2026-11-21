#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgpu.h"
#include "system/animation.h"

// The 0x20-byte timer work-list task BattleQueueActorSlotTask and BattleAnimTaskAllocArchiveEntry
// queue (TimerWorkListAllocateTask payload 4): a work-list entry followed by the
// 4-byte payload they carry - a pointer for one, an archive index for the other.
typedef struct {
    /* 0x00 */ WorkListEntry entry;
    /* 0x1C */ u32 payload;
} BattleAnimWorkTask; // Size: 0x20

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

extern void BattleSetAnimState(s32);
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
extern s32 g_BattleAnimStateFlags;
extern BattleSpriteColorTask *g_BattleSpriteColorTask;
// The anim tasks the two owner groups run; AnimTask (system/animation.h) is the
// record: entry, the entry the owner list links, then the sprite it animates.
extern AnimTask *g_BattleAnimOwnerTaskGroupA;
extern AnimTask *g_BattleAnimOwnerTaskGroupB;
extern u16 g_BattleAnimSavedGroupStates[];
extern u16 D_800C3CD4[];
extern s16 g_BattleAnimGroupStates[];
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
extern s32 g_BattleAnimState;
extern void func_800BB350(u32 slot);
extern void BattleActorSyncPositionFromModel(AnimTask *pTask);
extern AnimTask *WorkListsAddTasks(u32 allocSize, void *pData, void *pTimerCallback,
                                  void *pWorkCallback, void *pOnFreeCallback);
extern void func_80023804(SpriteData *pSprite);
extern void SpriteInitBaseWithInternalStorage(SpriteData *pSprite);
extern void SpriteSetScale(SpriteData *pSprite, s16 scale);
extern u16 g_GfxWorkListCapacity;
extern SpriteAnimPackage g_SpriteAnimPackageDirs5;
extern void func_800A979C(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4);
extern void func_800A4654(MATRIX *pMatrix, SVECTOR *pA, SVECTOR *pB, s32 depth,
                          s32 primIndex, SVECTOR *pC, SVECTOR *pD, s32 scale);
extern SVECTOR g_BattleSlotVectors;
extern SVECTOR D_800D335C;
extern SVECTOR D_800C3730;
extern SVECTOR g_BattleDrawScaleX;
extern u8 D_800C372C;
extern u8 g_BattleStateWork[];

// The three actor-slot spawn positions the slot allocator hands out, one pair each.
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
} BattleActorSlotPos; // Size: 0x4

extern BattleActorSlotPos g_BattleActorSlotSpawnPos[3];
extern u16 g_BattleActorSlotUsedMask;
extern u8 g_BattleAllActorTasksDone;


// Battle-side view of a battle actor's SpriteData (include/field/actor.h); the same
// record src/battle/main/misc30.c views. Its 0x2E/0x30/0x36/0x38 fields are unnamed
// spans in BattleSpriteState, which other battle units' matches depend on.
typedef struct {
    /* 0x00 */ BattleFixed16Vector position;
    /* 0x0C */ u8 unkC[0x22];
    /* 0x2E */ s16 otDepth; // projected depth + field_0x30, 0 when the projection clips
    /* 0x30 */ s16 field_0x30;
    /* 0x32 */ u8 unk32[0x4];
    /* 0x36 */ volatile u16 height; // volatile: the original reloads it after the store
    /* 0x38 */ u16 halfHeight;
    /* 0x3A */ u8 unk3A[0x6E];
    /* 0xA8 */ u32 flags_0xA8;
    /* 0xAC */ u32 flags_0xAC;
} BattleActorSprite; // Size: 0xB0
// The 0x1C-byte per-slot records at the head of g_BattleStateWork, as this unit
// reads them: the slot's spawn point in whole screen units.
typedef struct {
    /* 0x00 */ u8 unk0[0xE];
    /* 0x0E */ u16 x;
    /* 0x10 */ u16 z;
    /* 0x12 */ u8 unk12[0xA];
} BattleSlotWorkRow; // Size: 0x1C

// The three position halves go through TWO temporaries: the first load lands in $v1
// because $v0 still holds the height, and a single temp shared by all three costs the
// register split (99.61%).
void BattleUpdateActorSpriteDepth(u8 *pTask) {
    BattleActorSprite *pSprite;
    SVECTOR vec;
    long sxy;
    long flag;
    s32 nOTOffset;
    s32 mode;
    s32 whole;
    s32 whole2;

    pSprite = *(BattleActorSprite **)(pTask + 4);
    mode = pSprite->flags_0xA8 >> 30;
    pSprite->height = BattleGetActorHeight(((pSprite->flags_0xAC & 3) << 2) | mode);
    pSprite->halfHeight = pSprite->height >> 1;
    whole = pSprite->position.xWhole;
    vec.vx = whole;
    whole2 = pSprite->position.yWhole;
    vec.vy = whole2;
    whole2 = pSprite->position.zWhole;
    vec.vz = whole2;
    SetRotMatrix(&g_BattleViewMatrix);
    SetTransMatrix(&g_BattleViewMatrix);
    nOTOffset = (RotTransPers(&vec, &sxy, &sxy, &flag) >> g_GfxOTDepthShift) +
                pSprite->field_0x30;
    if (flag & 0x8000) {
        nOTOffset = 0;
    }
    pSprite->otDepth = nOTOffset;
}

void BattleFreeActorSpriteTask(u8 *pEntry) {
    u32 unused[2];
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB350);
// 98.10% at the original's exact 496 bytes: all 124 instructions are present and in
// retail's order, and only the closing group differs - the flags_0xA8 word ends in $v0
// where retail keeps it in $a0, and its store lands after the 0xAC block's `andi`
// instead of between that `andi` and its `and`. Measured: this body (98.10), the shift
// operand first `(slot << 30) | (flags & 0x3FFFFFFF)` (97.78, which does get the store
// position right but rotates all four registers), the two-statement
// `&= 0x3FFFFFFF; |= slot << 30` split (98.10), the word in a u32 local read before the
// two NULL file stores (98.10), the shift in its own local just above the store (96.49,
// the 0x3FFFFFFF mask leaves $v1), the two flag statements swapped (88.44), the pair in a
// do/while(0) block (91.40, which also pulls the masks into $s2/$s3 and moves a save) and
// a VOLATILE lvalue on the flags_0xA8 store (98.10, and it is the wrong direction: the
// store then lands straight after the `or`, before the 0xAC load, with the word still in
// $v0 - retail's store sinks past the 0xAC load and its `andi`, so it has to stay a
// movable plain store). A 3-minute permuter run from this body (1518 iterations) found
// nothing, so the residual is where sched2 puts the store, not the C.
/*
void func_800BB350(u32 slot) {
    u8 *pBase;
    AnimTask *pTask;
    SpriteData *pSprite;
    u8 saved;
    s32 scale;

    pBase = g_BattleStateWork;
    if (*(SpriteData **)(pBase + slot * 4 + 0x8C8C) != NULL) {
        return;
    }
    saved = g_FlagTimerWorkListAdds;
    g_FlagTimerWorkListAdds = 0;
    pTask = WorkListsAddTasks(sizeof(AnimTask), NULL, BattleActorSyncPositionFromModel,
                              BattleUpdateActorSpriteDepth, BattleFreeActorSpriteTask);
    pSprite = &pTask->spriteData;
    pSprite->field_0x6C = pTask;
    pTask->task1.unk4 = pSprite;
    pTask->task2.unk4 = pSprite;
    func_80023804(pSprite);
    SpriteInitBaseWithInternalStorage(pSprite);
    pSprite->field_0x40 = (pSprite->field_0x40 & 0xFFFE1FFF) | 0x8000;
    *(u32 *)((u8 *)pSprite + 0x3C) &= ~3;
    pSprite->field_0x6C = pTask;
    pSprite->flags_0xA8 &= ~1;
    ((SpriteDataI3 *)pSprite->field_0x7C)->field_0x8 = 0;
    ((SpriteDataI3 *)pSprite->field_0x7C)->field_0xC = 0;
    // the original stores the work-list capacity here and overwrites it with the
    // sprite's own scale below; the first store is dead but is in the object
    scale = 0x2000;
    pSprite->field_0x82 = g_GfxWorkListCapacity;
    pSprite->position.x = ((BattleSlotWorkRow *)(pBase + slot * 0x1C))->x << 16;
    pSprite->position.z = ((BattleSlotWorkRow *)(pBase + slot * 0x1C))->z << 16;
    pSprite->position.y = 0;
    // volatile on both halves: the original stores the height and then reloads it
    *(volatile u16 *)&pSprite->field_0x36 = BattleGetActorHeight(slot);
    pSprite->field_0x38 = *(volatile u16 *)&pSprite->field_0x36 >> 1;
    pSprite->field_0x82 = scale;
    SpriteSetScale(pSprite, scale);
    pSprite->pVramData = &g_SpriteAnimPackageDirs5;
    *(SpriteData **)(pBase + slot * 4 + 0x8C8C) = pSprite;
    *(AnimTask **)(pBase + slot * 4 + 0x8CB8) = pTask;
    pSprite->pSpecialAnimFile = NULL;
    pSprite->pDefaultAnimFile = NULL;
    g_FlagTimerWorkListAdds = saved;
    // the slot index is carried across both flag words: its low two bits in the top
    // of flags_0xA8, its high two in the low bits of flags_0xAC
    pSprite->flags_0xA8 = (pSprite->flags_0xA8 & 0x3FFFFFFF) | (slot << 30);
    pSprite->flags_0xAC.flags = (pSprite->flags_0xAC.flags & ~3) | ((slot >> 2) & 3);
}
*/
void BattleSpawnActorSlot(BattleAnimWorkTask *pTask) {
    s32 i;
    u16 slotBit;
    u32 payload;
    BattleActorSlotPos *pSlotPosTable;
    BattleActorSlotPos *pSlotPos;

    for (i = 0, slotBit = 1; i != 3; i++, slotBit <<= 1) {
        if ((g_BattleActorSlotUsedMask & slotBit) == 0) {
            g_BattleActorSlotUsedMask |= slotBit;
            break;
        }
    }
    // the base and the sum live in two variables: retail forms the table address
    // before it scales the index
    pSlotPosTable = g_BattleActorSlotSpawnPos;
    pSlotPos = pSlotPosTable + i;
    // the payload is a word for the slot's owner and a halfword for its target angle
    func_800A979C(pTask->payload, pSlotPos->x, pSlotPos->y, 0,
                  (s16)(*(u16 *)&pTask->payload + 0x1C0));
    // read before the decrement so the count store fills the call's delay slot
    payload = pTask->payload;
    g_BattleAnimTaskAllocCount--;
    func_800BB350(payload);
    pTask->entry.onFreeCallback(pTask);
    g_BattlePendingActorTaskCount--;
    if (g_BattlePendingActorTaskCount == 0) {
        g_BattleAllActorTasksDone = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB620);
void BattleAnimTaskAllocArchiveEntry(BattleAnimWorkTask *pTask) {
    func_800A9540(pTask->payload);
    g_BattleAnimTaskAllocCount++;
    TimerWorkListSetTaskCallback(pTask, func_800BB620);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB6E0);
void BattleQueueActorSlotTask(u8 *pEntry) {
    u8 saved;
    BattleAnimWorkTask *pTask;
    s32 pendingCount;

    saved = g_FlagTimerWorkListAdds;
    g_FlagTimerWorkListAdds = 0;
    g_TaskHeapAllocFlags = 1;
    pTask = (BattleAnimWorkTask *)TimerWorkListAllocateTask(NULL, 4);
    TimerWorkListSetTaskCallback(pTask, func_800BB6E0);
    // the counter is read into a local first: retail puts the payload store in the
    // load-delay slot of that read, which a plain ++ cannot
    pendingCount = g_BattlePendingActorTaskCount;
    pTask->payload = (u32)pEntry;
    g_TaskHeapAllocFlags = 0;
    g_BattlePendingActorTaskCount = pendingCount + 1;
    g_FlagTimerWorkListAdds = saved;
}
void BattleResetAnimState(void) {
    D_800C3674 = 0x200;
    D_800C3678 = -1;
    g_BattleActiveAnimTaskCount = 0;
    g_BattleAnimStateFlags = 1;
    BattleSetAnimState(0);
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
// best 84.11% at retail's exact 228 bytes: the tail block's argument setup is the same
// instruction multiset in retail's order, but the two loads (depth 0xC54 into $a3,
// primIndex 0xC84 into $v1) sit between the three stack stores in retail and are hoisted
// above them here, and the work base lands in $v0 instead of $v1. Spellings measured:
// both loads in locals (84.11), both inline in the call with a mem_addr base (83.44 - the
// base does take $v1 there, but the loads reorder to primIndex-first and depth moves into
// the jal delay slot), depth local + primIndex inline (84.11), a dead store of the base
// (92.51, not a spelling to ship) and g_BattleStateWork + 0x8000 as one expression (220
// bytes: it folds into the address addend). Statement order cannot move it: this preset
// runs -fschedule-insns, and two 4-minute permuter runs from 84.11 and 92.51 found nothing.
/*
void func_800BB9D4(void) {
    s32 *pViewTrans;
    mem_addr pWork;
    s32 depth;
    s32 primIndex;

    BattleBuildMatrixFromThreeVectors(&g_BattleViewMatrix, &g_BattleSlotVectors, &D_800D335C,
                                      &D_800C3730);
    pViewTrans = (s32 *)((u8 *)&g_BattleViewMatrix - 0x20);
    pViewTrans[0xD] += g_BattleDrawScaleX.vx;
    pViewTrans[0xE] += g_BattleDrawScaleX.vy;
    pViewTrans[0xF] += g_BattleDrawScaleX.vz;
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
        BattleSetAnimState(g_BattlePendingAnimState);
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
    BattleSetAnimState(2);
}
// case 4 before case 2: this gcc emits switch bodies in source order.

void BattleSetAnimState(s32 state) {
    s16 *pGroupBStates;

    g_BattleAnimState = state;
    g_BattleAnimStateFlags = 1;
    switch (state) {
    case 4:
        g_BattleAnimStateFlags = 5;
        break;
    case 2:
        g_BattleAnimPositionGroupA.x = g_BattleAnimGroupStates[0] << 16;
        g_BattleAnimPositionGroupA.y = g_BattleAnimGroupStates[1] << 16;
        g_BattleAnimPositionGroupA.z = g_BattleAnimGroupStates[2] << 16;
        // the original walks a pointer for the second group: retail adds 8 to the
        // table base and reads the following angles off the new address
        pGroupBStates = g_BattleAnimGroupStates + 4;
        g_BattleAnimPositionGroupB.x = pGroupBStates[0] << 16;
        g_BattleAnimPositionGroupB.y = pGroupBStates[1] << 16;
        g_BattleAnimPositionGroupB.z = pGroupBStates[2] << 16;
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

void BattleSetAnimTaskDoneState(s32 value) {
    g_BattlePendingAnimState = value;
}
void BattleBeginAnimTask(u8 *pEntry) {
    if (g_BattleAnimCommandsSuppressed == 0) {
        BattleSetAnimState(1);
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
        BattleSetAnimState(4);
    }
}

void BattleSetAnimState1(void) {
    if (g_BattleAnimCommandsSuppressed == 0) {
        BattleSetAnimState(1);
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
