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
extern void func_800C08CC();
extern void func_800B3CD4();
extern void func_800B51B0();
extern void func_800B50D4();
extern void func_800B5588();
extern u8 D_800C356C[];
extern void BattleIntegrateSpriteVelocityTask();
extern void func_800B5DF4();
extern void func_800B6004();
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
    /* 0x0C */ void* onFreeCallback;
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
// 90.55% under gcc 2.7.2-cdk -G8 and -G0 and every swept -O/-G/-mcpu, 73.07 under
// 2.6.0 (re-screened by the -G0 park sweep): no preset fixes this. The two combined
// induction bases match and only the register assignment differs (retail keeps pActor in
// $t1, the frame index in $a3). Permuter (4 min, -j 2) reached base=667 -> best=562 by
// writing x through a fixed ((s16*)pArg)[0], which is not equivalent, so nothing there is
// portable. The shape to reproduce is retail's two bases: x through 0(t0), y/z through
// a base 4 bytes further on (-2(a1)/0(a1)), both advanced by 8.
/*
void func_800B50D4(u8* pActor, u8* pArg) {
    s32 x;
    s32 y;
    s32 i;
    s16 scale;
    u8* pFrame;
    s16* pOut;

    if ((*(u32*)(pActor + 0x3C) & 3) != 1) {
        return;
    }
    if (*(u8**)(pActor + 0x20) == NULL) {
        return;
    }
    if (*(u8**)(*(u8**)(pActor + 0x20) + 0x34) == NULL) {
        return;
    }
    pOut = (s16*)pArg;
    for (i = 0; i != 5; i++) {
        pFrame = *(u8**)(*(u8**)(pActor + 0x20) + 0x34) + (D_800C356C[i] << 3);
        x = *(s8*)pFrame;
        y = *(s8*)(pFrame + 1);
        if ((*(u32*)(pActor + 0xAC) >> 2) & 1) {
            x = -x;
        }
        scale = *(s16*)(pActor + 0x2C);
        x = (x * scale) / 0x2000;
        y = (y * scale) / 0x2000;
        pOut[0] = x;
        pOut[1] = y;
        pOut[2] = 0;
        pOut += 4;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B50D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B51B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5588);
void BattleProcessDirectionTransformTask(WorkListEntry* pWork) {
    BattleDirectionTask* pTask;

    pTask = pWork->pTask;
    func_8001E148(pTask->pSpriteData);
    func_800C08CC(5, pTask->curPos, func_800B51B0);
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
