#include "common.h"
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
extern void BattleIntegrateSpriteVelocityTask();
extern void func_800B5DF4();
extern void func_800B6004();
extern void BattleUpdateDistanceWaitTask();
extern u8* TimerWorkListAllocateTask();
extern s32 BattleSpriteDistanceToTarget(SpriteData* pSpriteData);
extern u8* g_GfxCurWorkBuffer;
extern u8* g_GfxCurWorkBufferEnd;
extern u_long* g_GfxCurOT;
extern s32 g_GfxOTDepthShift;
extern MATRIX g_GfxBaseMatrix;

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ s8 unk1;
    /* 0x2 */ s8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
} BattleScriptOp;

typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
} BattlePoint;

// WorkListEntry of include/system/animation.h; an overlay unit cannot include that
// header (it holds tentative definitions of main-exe globals), so the layout is
// repeated. unk4 is the SpriteData the entry's callback works on.
typedef struct WorkListEntry {
    /* 0x00 */ void* unk0;
    /* 0x04 */ SpriteData* pSpriteData;
    /* 0x08 */ void* onTriggerCallback;
    /* 0x0C */ void (*onFreeCallback)(void*);
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ struct WorkListEntry* pNext;
} WorkListEntry; // Size: 0x1C

// What SpriteData.field_0x6C points at (AnimTask in include/system/animation.h):
// the timer entry the motion callbacks hang on and, at 0x1C, the entry the sprite
// draw pass hangs its callback on.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry drawEntry;
} BattleSpriteTask; // Size: 0x38

// The 0x18-byte block BattleCreateActorDistanceWaitTask adds after its timer
// entry: the actor to watch, the animation it plays while the task waits, the
// distance measured last frame and the bound that ends the wait.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ SpriteData* pSpriteData;
    /* 0x20 */ u8 unk20[0x4];
    /* 0x24 */ s32 animId;
    /* 0x28 */ s32 distance;
    /* 0x2C */ s32 minDistance;
    /* 0x30 */ void* pSpriteBytecode;
} BattleDistanceWaitTask; // Size: 0x34

// The 0x18-byte block BattleCreateSpriteFollowTask adds after its timer entry: the
// actor whose joint leads, the sprite it drags along, the two direction-transform
// slots the script byte at 0x24/0x28 packs (one nibble each), and the frame and
// animation the follow started with.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ SpriteData* pSpriteData;
    /* 0x20 */ SpriteData* pTargetSprite;
    /* 0x24 */ s32 sourceJoint;
    /* 0x28 */ s32 targetJoint;
    /* 0x2C */ s32 curSpriteFrame;
    /* 0x30 */ s32 animId;
} BattleSpriteFollowTask; // Size: 0x34


// Split out of main/misc27 (see config/battle.yaml): compiled with gcc 2.7.2-cdk.
BattleDistanceWaitTask* BattleCreateActorDistanceWaitTask(SpriteData* pSpriteData, s32 minDistance,
                                                              void* pSpriteBytecode) {
    BattleDistanceWaitTask* pTask;

    pTask = (BattleDistanceWaitTask*)TimerWorkListAllocateTask(
        pSpriteData->field_0x6C, sizeof(BattleDistanceWaitTask) - sizeof(WorkListEntry));
    TimerWorkListSetTaskCallback(pTask, BattleUpdateDistanceWaitTask);
    pTask->pSpriteData = pSpriteData;
    pTask->distance = BattleSpriteDistanceToTarget(pSpriteData);
    pTask->minDistance = minDistance;
    pTask->pSpriteBytecode = pSpriteBytecode;
    pTask->animId = (s8)pSpriteData->flags_0xAC.bytes[3];
    pSpriteData->flags_0xAC.flags |= 0x20;
    return pTask;
}
BattlePoint* BattleGetActorJointOffset(BattlePoint* pOut, SpriteData* pSpriteData, s32 index) {
    DVECTOR rot;

    if (pSpriteData->pBase == NULL) {
        return pOut;
    }
    if ((pSpriteData->field_0x3C_0 & 3) != 1) {
        return pOut;
    }
    if (pSpriteData->pBase->pDirTransforms == NULL) {
        return pOut;
    }
    rot.vy = (s8)(pSpriteData->pBase->pDirTransforms->dirTransforms + index)->translationY;
    rot.vx = (s8)(pSpriteData->pBase->pDirTransforms->dirTransforms + index)->translationX;
    if ((((u32)pSpriteData->flags_0xAC.flags) >> 2) & 1) {
        rot.vx = -rot.vx;
    }
    rot.vy = rot.vy * pSpriteData->scale / 4096;
    rot.vx = rot.vx * pSpriteData->scale / 4096;
    *(DVECTOR*)pOut = rot;
    return pOut;
}
BattlePoint* BattleGetActorJointPoint(BattlePoint* pOut, SpriteData* pSpriteData, s32 index) {
    BattlePoint point;
    int ofs;

    // The block is load-bearing: without it gcc hoists the second joint offset above the
    // first store and this function compiles 8 bytes short of retail.
    do {
        BattleGetActorJointOffset(&point, pSpriteData, index);
        ofs = ((BattleFixed16Vector*)&pSpriteData->position)->xWhole;
        point.x = point.x + ofs;
        ofs = ((BattleFixed16Vector*)&pSpriteData->position)->yWhole;
    } while (0);
    point.y = point.y + ofs;
    *pOut = point;
    return pOut;
}

void BattleUpdateSpriteFollowTask(BattleSpriteFollowTask* pTask) {
    BattlePoint a;
    BattlePoint b;
    DVECTOR d;
    SpriteData* pSpriteData;
    s32 mode;

    pSpriteData = pTask->pSpriteData->pTargetEntitySprite;
    BattleGetActorJointPoint(&a, pTask->pSpriteData, pTask->sourceJoint);
    BattleGetActorJointPoint(&b, pSpriteData, pTask->targetJoint);
    d.vx = a.x - b.x;
    d.vy = a.y - b.y;
    pSpriteData->position.x += d.vx << 16;
    pSpriteData->position.y += d.vy << 16;
    if (pTask->animId == (s8)pTask->pSpriteData->flags_0xAC.bytes[3]) {
        mode = ((u32)pTask->pSpriteData->flags_0xA8 >> 28) & 3;
        if (mode != 0) {
            if (mode != 1) {
                return;
            }
        }
    }
    pTask->timerEntry.onFreeCallback(pTask);
}

BattleSpriteFollowTask* BattleCreateSpriteFollowTask(SpriteData* pSpriteData, u8* pArgs) {
    BattleSpriteFollowTask* pTask;

    pTask = (BattleSpriteFollowTask*)TimerWorkListAllocateTask(
        pSpriteData->field_0x6C, sizeof(BattleSpriteFollowTask) - sizeof(WorkListEntry));
    TimerWorkListSetTaskCallback(pTask, BattleUpdateSpriteFollowTask);
    pTask->pSpriteData = pSpriteData;
    pTask->pTargetSprite = pSpriteData->pTargetEntitySprite;
    // Read unsigned: the frame the follow starts on is a 16-bit value
    pTask->curSpriteFrame = *(u16*)&pSpriteData->curSpriteFrame;
    pTask->animId = (s8)pSpriteData->flags_0xAC.bytes[3];
    pTask->sourceJoint = pArgs[0] & 0xF;
    pTask->targetJoint = pArgs[0] >> 4;
    BattleUpdateSpriteFollowTask(pTask);
    return pTask;
}
void BattleIntegrateSpriteVelocityTask(WorkListEntry* pWork) {
    MATRIX m;
    SVECTOR rot;
    SVECTOR rot2;
    VECTOR v;
    SpriteData* pSpriteData;

    pSpriteData = pWork->pSpriteData;
    AnimScriptTick(pSpriteData);
    rot2.vy = pSpriteData->step.y >> 13;
    rot2.vz = pSpriteData->step.z >> 13;
    rot2.vx = 0;
    rot.vx = SpriteScaleValue(pSpriteData, pSpriteData->step.x >> 13);
    rot.vy = 0;
    rot.vz = 0;
    RotMatrix(&rot2, &m);
    ApplyMatrix(&m, &rot, &v);
    v.vx += pSpriteData->field_0xA0;
    v.vy += pSpriteData->field_0xA2;
    v.vz += pSpriteData->field_0xA4;
    pSpriteData->position.x = v.vx << 16;
    pSpriteData->position.y = v.vy << 16;
    pSpriteData->position.z = v.vz << 16;
    if (pSpriteData->pSpriteBytecode == NULL) {
        pWork->onFreeCallback(pWork);
    }
}
void BattleStartSpriteMotionTask(SpriteData* pSpriteData) {
    BattleSpriteTask* pTask;

    pSpriteData->curSpriteFrame = 1;
    pTask = pSpriteData->field_0x6C;
    TimerWorkListSetTaskCallback(&pTask->timerEntry, BattleIntegrateSpriteVelocityTask);
}
// The identity matrix a sprite carrying its own transform set draws its line
// through; the other choice is the shared g_GfxBaseMatrix.
extern MATRIX g_BattleSpriteOwnMatrix;

// Best attempt: 99.82%, 456 bytes (retail's exact size), 113 of 114 instructions.
// The only difference is the %hi scratch register of the two symbol addresses in
// the matrix selection: retail is "lui s0,%hi(sym) / j / addiu s0,s0,%lo(sym)",
// this body gives "lui v0,%hi(sym) / j / addiu s0,v0,%lo(sym)" - reload put the
// HIGH pseudo in a scratch register instead of tying it to the destination.
// Measured and rejected: ternary vs if/else vs default+override (same shape),
// four local declaration orders, "register" on the pointers, -O3, and this unit's
// own functions at gcc-2.7.2-psx / 2.6.0-psx (86.80 / 86.62). Two 4-minute
// permuter runs from different bodies (base score 20 and 135) found nothing.
// The pBuf variable is load-bearing: it must stay live past the pLine assignment,
// otherwise gcc merges the two and the "move s2,a1" that fills the flag test's
// delay slot disappears (452 bytes, 98.86%).
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B5DF4);
/*
void func_800B5DF4(WorkListEntry* pWork) {
    s32 ofs;
    s32 flag;
    s32 otz;
    s32 shift;
    u8* pBuf;
    SpriteData* pSpriteData;
    LINE_F2* pLine;
    DR_TPAGE* pTpage;
    SVECTOR vec;
    MATRIX* pMatrix;
    u32 abr;

    pSpriteData = pWork->pSpriteData;
    if (*(u16*)&pSpriteData->curSpriteFrame != 0) {
        return;
    }
    pBuf = g_GfxCurWorkBuffer;
    if (pBuf + 0x10 >= g_GfxCurWorkBufferEnd) {
        return;
    }
    pLine = (LINE_F2*)pBuf;
    ofs = ((BattleFixed16Vector*)&pSpriteData->position)->xWhole;
    vec.vx = ofs;
    ofs = ((BattleFixed16Vector*)&pSpriteData->position)->yWhole;
    vec.vy = ofs;
    ofs = ((BattleFixed16Vector*)&pSpriteData->position)->zWhole;
    vec.vz = ofs;
    g_GfxCurWorkBuffer = pBuf + 0x10;
    if (pSpriteData->field_0x3C_2 & 1) {
        pMatrix = &g_BattleSpriteOwnMatrix;
    } else {
        pMatrix = &g_GfxBaseMatrix;
    }
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    otz = RotTransPers(&vec, (s32*)&pLine->x0, &flag, &flag) >> g_GfxOTDepthShift;
    pSpriteData->field_0x2e = otz;
    shift = (u16)pSpriteData->field_0x36 + 8;
    vec.vx -= pSpriteData->step.x >> shift;
    vec.vy -= pSpriteData->step.y >> shift;
    vec.vz -= pSpriteData->step.z >> shift;
    RotTransPers(&vec, (s32*)&pLine->x1, &flag, &flag);
    setlen(pLine, 3);
    *(u32*)&pLine->r0 = *(u32*)&pSpriteData->red;
    AddPrim(&g_GfxCurOT[otz], pLine);
    pTpage = (DR_TPAGE*)g_GfxCurWorkBuffer;
    if ((u8*)(pTpage + 1) >= g_GfxCurWorkBufferEnd) {
        return;
    }
    abr = *(u8*)((u8*)pSpriteData + 0x3C) >> 5;
    if (abr == 0) {
        return;
    }
    g_GfxCurWorkBuffer = (u8*)(pTpage + 1);
    setDrawTPage(pTpage, 0, 0, getTPage(0, abr - 1, 0, 0));
    AddPrim(&g_GfxCurOT[otz], pTpage);
}
*/
void BattleSetSpriteDrawWorkCallback(SpriteData* pSpriteData) {
    BattleSpriteTask* pTask;

    pSpriteData->curSpriteFrame = 1;
    pTask = pSpriteData->field_0x6C;
    WorkListSetTaskCallback(&pTask->drawEntry, func_800B5DF4);
    pSpriteData->prim = 0x40;
}
void func_800B6004(WorkListEntry* pWork) {
    s32 coord;
    s32 ofs;
    SpriteData* pSpriteData;
    SpriteData* pLinked;
    LINE_F2* pLine;
    DR_TPAGE* pTpage;
    SVECTOR vec;
    s32 p;
    s32 flag;
    s32 otz;
    u32 abr;

    pSpriteData = pWork->pSpriteData;
    if (*(u16*)&pSpriteData->curSpriteFrame != 0) {
        return;
    }
    pLine = (LINE_F2*)g_GfxCurWorkBuffer;
    pLinked = pSpriteData->field_0x70;
    if ((u8*)(pLine + 1) >= g_GfxCurWorkBufferEnd) {
        return;
    }
    coord = ((BattleFixed16Vector*)&pSpriteData->position)->xWhole;
    vec.vx = coord;
    coord = ((BattleFixed16Vector*)&pSpriteData->position)->yWhole;
    vec.vy = coord;
    coord = ((BattleFixed16Vector*)&pSpriteData->position)->zWhole;
    vec.vz = coord;
    g_GfxCurWorkBuffer = (u8*)(pLine + 1);
    SetRotMatrix(&g_GfxBaseMatrix);
    SetTransMatrix(&g_GfxBaseMatrix);
    otz = RotTransPers(&vec, (s32*)&pLine->x0, &p, &p) >> g_GfxOTDepthShift;
    pSpriteData->field_0x2e = otz;
    // Retail refills the same vertex from the sprite before overwriting it with
    // the linked sprite's, dead stores included. The first read keeps its own
    // local because retail still holds the projected depth in $v0 there.
    ofs = ((BattleFixed16Vector*)&pSpriteData->position)->xWhole;
    vec.vx = ofs;
    coord = ((BattleFixed16Vector*)&pSpriteData->position)->yWhole;
    vec.vy = coord;
    coord = ((BattleFixed16Vector*)&pSpriteData->position)->zWhole;
    vec.vz = coord;
    coord = ((BattleFixed16Vector*)&pLinked->position)->xWhole;
    vec.vx = coord;
    coord = ((BattleFixed16Vector*)&pLinked->position)->yWhole;
    vec.vy = coord;
    coord = ((BattleFixed16Vector*)&pLinked->position)->zWhole;
    vec.vz = coord;
    RotTransPers(&vec, (s32*)&pLine->x1, &p, &p);
    setlen(pLine, 3);
    *(u32*)&pLine->r0 = *(u32*)&pSpriteData->red;
    AddPrim(&g_GfxCurOT[otz], pLine);
    pTpage = (DR_TPAGE*)g_GfxCurWorkBuffer;
    if ((u8*)(pTpage + 1) >= g_GfxCurWorkBufferEnd) {
        return;
    }
    abr = *(u8*)((u8*)pSpriteData + 0x3C) >> 5;
    if (abr == 0) {
        return;
    }
    g_GfxCurWorkBuffer = (u8*)(pTpage + 1);
    setDrawTPage(pTpage, 0, 0, getTPage(0, abr - 1, 0, 0));
    AddPrim(&g_GfxCurOT[otz], pTpage);
}
void BattleSetLinkedSpriteDrawWorkCallback(SpriteData* pSpriteData) {
    BattleSpriteTask* pTask;

    pSpriteData->curSpriteFrame = 1;
    pTask = pSpriteData->field_0x6C;
    WorkListSetTaskCallback(&pTask->drawEntry, func_800B6004);
    pSpriteData->prim = 0x40;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B61F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B626C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B62C8);
void BattleScriptApplyRectParams(u8* pActor, s8* pScript) {
    BattleScriptOp* pOp;

    pOp = (BattleScriptOp*)(pScript + ((pScript[1] << 8) | (u8)pScript[0]));
    func_800B3CD4(pOp->unk5, pOp->unk3, pOp->unk4, (s8)pOp->unk0, pOp->unk1, pOp->unk2);
}
