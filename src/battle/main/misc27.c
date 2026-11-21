#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"

extern void TimerWorkListSetTaskCallback();
extern void WorkListSetTaskCallback();
extern void func_8001E148();
extern void AnimScriptTick(void* pSprite);
extern s32 SpriteScaleValue(void* pSprite, s32 value);
extern void BattleEmitTrailSegments();
extern void func_800B3CD4();
extern void func_800B51B0();
extern void func_800B50D4();
extern void func_800B5588();
extern u8 D_800C356C[];
extern u32 D_800D2FD8[];
extern u16 D_800C3E9C[];
extern u16 D_800C3D4C[];
extern u16 D_800D3334[];
extern void BattleIntegrateSpriteVelocityTask();
extern void BattleDrawActorLinePrim();
extern void BattleDrawLinkedActorLinePrim();
extern void BattleGetActorJointOffset();
extern void BattleUpdateDistanceWaitTask();
extern u8* TimerWorkListAllocateTask();

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ s8 unk1;
    /* 0x2 */ s8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
} BattleScriptOp;

// WorkListEntry of include/system/animation.h; an overlay unit cannot include that header
// (it holds tentative definitions of main-exe globals), so the layout is repeated. Both
// entries WorkListsAddTasks fills in point unk4 back at the block they were allocated in.
typedef struct BattleDirectionTask BattleDirectionTask;

typedef struct WorkListEntry {
    /* 0x00 */ void* unk0;
    /* 0x04 */ BattleDirectionTask* pTask;
    /* 0x08 */ void* onTriggerCallback;
    /* 0x0C */ void (*onFreeCallback)(void*);
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ struct WorkListEntry* pNext;
} WorkListEntry; // Size: 0x1C

// The 0xB8-byte block WorkListsAddTasks allocates for the direction-transform task: the
// timer and work entries, then the task's own fields.
struct BattleDirectionTask {
    /* 0x00 */ WorkListEntry timerTask;
    /* 0x1C */ WorkListEntry workTask;
    /* 0x38 */ SpriteData* pSpriteData;
    /* 0x3C */ u32 curSpriteFrame;
    /* 0x40 */ s32 animId;
    /* 0x44 */ u32 semiTrans;
    /* 0x48 */ s32 radius;
    /* 0x4C */ u8* pArgs;
    /* 0x50 */ SVECTOR targetPos[5];
    /* 0x78 */ SVECTOR curPos[5];
    /* 0xA0 */ u8 unkA0[0x18];
}; // Size: 0xB8

// One entry of the direction-transform table SpriteData.pBase points at
// (SpriteDirectionTransforms in include/field/actor.h): retail loads the translation as
// s8 and the rotations as u16, so this unit views the table with that signedness instead
// of changing the shared header, which other units depend on.
typedef struct {
    /* 0x0 */ s8 translationX;
    /* 0x1 */ s8 translationY;
    /* 0x2 */ u16 rotationX;
    /* 0x4 */ u16 rotationY;
    /* 0x6 */ u16 rotationZ;
} BattleDirectionTransform; // Size: 0x8

extern BattleDirectionTask* WorkListsAddTasks(s32 allocSize, void* pData, void* pTimerCallback,
                                              void* pWorkCallback, void* pOnFreeCallback);

// Split out of main/misc27 (see config/battle.yaml): compiled with gcc 2.7.2-cdk,
// which the callback cluster in this function range needs.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B3F04);

// Uploads the current frame's texture and CLUT into the sprite's own slots: the
// 8x8 tile at the frame descriptor's texture-page rectangle goes to (0x3F0,0x1F0)
// and its CLUT to (0x3F0,0x1EE).
void BattleScriptUploadSpriteFrame(SpriteData* pSpriteData) {
    RECT rect;
    SpriteFrameData* pFrame;
    s16 texPage;
    s16 clut;

    pFrame = pSpriteData->pBase->pCurRenderFramesData;
    texPage = pFrame->texPage;
    rect.x = (pFrame->texX >> 2) + ((texPage & 0xF) << 6);
    rect.y = pFrame->texY + ((texPage << 4) & 0x100);
    rect.w = 8;
    rect.h = 8;
    MoveImage(&rect, 0x3F0, 0x1F0);
    clut = pFrame->clutID;
    rect.w = 0x10;
    rect.h = 1;
    rect.x = clut & 0x3F;
    rect.y = (clut >> 6) & 0x1FF;
    MoveImage(&rect, 0x3F0, 0x1EE);
}

void BattleSetActorDirectionMatrix(SpriteData* pSpriteData, s32 index, MATRIX* pMatrix) {
    BattleDirectionTransform* pTransform;
    SVECTOR rot;
    s32 x;
    s32 y;
    s16 scale;

    if (pSpriteData->pBase == NULL) {
        return;
    }
    pTransform = (BattleDirectionTransform*)(index * sizeof(BattleDirectionTransform) +
                                             (mem_addr)pSpriteData->pBase->pDirTransforms);
    y = pTransform->translationY;
    x = pTransform->translationX;
    if (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1) {
        x = -x;
    }
    scale = pSpriteData->scale;
    y = (y * scale) / 0x1000;
    x = (x * scale) / 0x1000;
    rot.vx = pTransform->rotationX;
    rot.vy = ((BattleDirectionTransform*)pSpriteData->pBase->pDirTransforms)[index].rotationY;
    rot.vz = ((BattleDirectionTransform*)pSpriteData->pBase->pDirTransforms)[index].rotationZ;
    RotMatrix(&rot, pMatrix);
    pMatrix->t[0] = pSpriteData->pBase->transformMatrix.t[0] + x;
    pMatrix->t[1] = pSpriteData->pBase->transformMatrix.t[1] + y;
    pMatrix->t[2] = pSpriteData->pBase->transformMatrix.t[2];
    SetMulMatrix(pMatrix, &pSpriteData->pBase->transformMatrix);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B50D4);
// 95.27% at retail's exact 220 bytes: the instruction multiset, the frame and both
// induction bases match, and the ONLY differing instruction is the y/z pointer's
// def - retail copies the parameter in place of the first `bne` delay slot's nop
// (`move t0,a1`) and adds 4 to $a1 itself, while this body (pX = pYz - 2) makes
// pYz keep $a1 and derives pX with `addiu t0,a1,-4`. Swapping which pointer gets
// $a1 needs the allocator to beat the copy-coalescing of pX with the parameter: a
// plain `pX = (s16*)pOut` (either order, before or after the guards, as s16* or
// SVECTOR*, via mem_addr or &pOut->vx, or with `pYz = pOut; pX = pYz; pYz += 2`)
// is always 94.64% with pX coalesced into $a1, and the permuter cannot compile
// this body ("conversion to non-scalar type requested"). One instruction from 100.
/*
void func_800B50D4(SpriteData* pSpriteData, SVECTOR* pOut) {
    s16* pX;
    s16* pYz;
    s32 i;
    s32 x;
    s32 y;
    s16 scale;
    BattleDirectionTransform* pTransform;

    if ((pSpriteData->field_0x3C_0 & 3) != 1) {
        return;
    }
    if (pSpriteData->pBase == NULL) {
        return;
    }
    if (pSpriteData->pBase->pDirTransforms == NULL) {
        return;
    }
    pYz = (s16*)pOut;
    pX = pYz - 2;
    for (i = 0; i != 5;) {
        pTransform = (BattleDirectionTransform*)(D_800C356C[i] * sizeof(BattleDirectionTransform) + (mem_addr)pSpriteData->pBase->pDirTransforms);
        x = pTransform->translationX;
        y = pTransform->translationY;
        if (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1) {
            x = -x;
        }
        scale = pSpriteData->scale;
        x = (x * scale) / 0x2000;
        y = (y * scale) / 0x2000;
        i++;
        pX[0] = x;
        pYz[-1] = y;
        pYz[0] = 0;
        pYz += 4;
        pX += 4;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B51B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5588);
// 90.01% (348 bytes): the four global stores and the loop match; retail's position
// reads are unsigned at some sites (lhu) and signed at others (lh), which suggests the
// original curPos/targetPos view is u16 and the delta is read through an s16 view - not
// reproduced here. Two other measured findings:
//  - The four globals must be extern ARRAYS (`D_800C3E9C[0] = ...`): as plain scalars
//    they go through `lui $at` where retail uses `lui $v0,%hi` + the store off $v0
//    (the atfree signature; with the array spelling that part matches exactly).
//  - retail keeps only $s0 (0x18 frame); holding pWork across the tail call costs an
//    $s1, so the onFree callback must be reached through pTask, not pWork.
//  - The tail mode test is UNMERGED in retail (`beqz v1` / `li v0,1` / `bne v1,v0`)
//    where gcc 2.7.2 merges `mode != 0 && mode != 1` into `sltiu v0,v1,2`; that is
//    the 2.6.0/preset branch-shape class.
/*
void func_800B5588(WorkListEntry* pWork) {
    BattleDirectionTask* pTask;
    SpriteData* pSpriteData;
    s32 mode;
    s32 i;

    pTask = pWork->pTask;
    pSpriteData = pTask->pSpriteData;
    D_800D2FD8[0] = (u32)pTask->pArgs;
    D_800C3E9C[0] = pTask->radius;
    D_800C3D4C[0] = pTask->semiTrans;
    D_800D3334[0] = pSpriteData->field_0x2e;
    if (pSpriteData->curSpriteFrame != pTask->curSpriteFrame) {
        pTask->curSpriteFrame = pSpriteData->curSpriteFrame;
        func_800B50D4(pSpriteData, pTask->targetPos);
    }
    for (i = 1; i != 5; i++) {
        pTask->curPos[i].vx = pTask->curPos[i].vx + (pTask->targetPos[i].vx - pTask->curPos[i].vx) / 2;
        pTask->curPos[i].vy = pTask->curPos[i].vy + (pTask->targetPos[i].vy - pTask->curPos[i].vy) / 2;
        pTask->curPos[i].vz = pTask->curPos[i].vz + (pTask->targetPos[i].vz - pTask->curPos[i].vz) / 2;
    }
    pTask->curPos[0].vx = pTask->targetPos[0].vx;
    pTask->curPos[0].vy = pTask->targetPos[0].vy;
    pTask->curPos[0].vz = pTask->targetPos[0].vz;
    if (pTask->animId == (s8)pTask->pSpriteData->flags_0xAC.bytes[3]) {
        mode = ((u32)pTask->pSpriteData->flags_0xA8 >> 28) & 3;
        if (mode != 0 && mode != 1) {
            return;
        }
    }
    pTask->timerTask.onFreeCallback(pTask);
}
*/
void BattleProcessDirectionTransformTask(WorkListEntry* pWork) {
    BattleDirectionTask* pTask;

    pTask = pWork->pTask;
    func_8001E148(pTask->pSpriteData);
    BattleEmitTrailSegments(5, pTask->curPos, func_800B51B0);
}

void BattleCreateDirectionTransformTask(SpriteData* pSpriteData, u8* pArgs) {
    BattleDirectionTask* pTask;

    pTask = WorkListsAddTasks(sizeof(BattleDirectionTask), pSpriteData->field_0x6C, func_800B5588,
                              BattleProcessDirectionTransformTask, NULL);
    pTask->pSpriteData = pSpriteData;
    // Read as unsigned: curSpriteFrame is s16 in the struct but -1 is a valid frame
    pTask->curSpriteFrame = *(u16*)&pSpriteData->curSpriteFrame;
    pTask->animId = *(s8*)((u8*)pSpriteData + 0xAF);
    pTask->pArgs = pArgs;
    pTask->semiTrans = *(u8*)((u8*)pSpriteData + 0x3C) >> 5;
    if (pArgs[0] == 0) {
        pTask->radius = 4;
    } else {
        pTask->radius = pArgs[0];
    }
    func_800B50D4(pSpriteData, pTask->curPos);
    func_800B50D4(pSpriteData, pTask->targetPos);
}
long BattleSpriteDistanceToTarget(BattleSpriteState* pSprite) {
    VECTOR v;
    VECTOR out;

    v.vx = pSprite->targetPosition.x - pSprite->position.xWhole;
    v.vy = pSprite->targetPosition.y - pSprite->position.yWhole;
    v.vz = pSprite->targetPosition.z - pSprite->position.zWhole;
    Square0(&v, &out);
    return SquareRoot0(out.vx + out.vz + out.vy);
}
