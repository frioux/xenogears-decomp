#include "common.h"
#include "field/actor.h"
#include "psyq/libgte.h"

// Battle state pointed to by D_800C3610. Only the fields this unit touches are recovered.
typedef struct {
    /* 0x00 */ u8 pad00[0x4];
    /* 0x04 */ SpriteData* pCurSprite;
    /* 0x08 */ u8 pad08[0x14];
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ u8 pad20[0x14];
    /* 0x34 */ u32 unk34;
    /* 0x38 */ u8 pad38[0x10];
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u8 unk49;
} BattleState;

extern BattleState* D_800C3610;
extern u8 D_800C3664;
extern u8 D_800C3EB0[];
extern s32 D_800C360C;
extern u16 D_800C3608;
extern u16 D_800C3614;
extern u16 D_800C3DF0;
extern u8 D_800C3623;

// The per-slot vectors retail copies from 0x800D30A0 to 0x800D3354.
extern u16 D_800D30A0[];
extern u16 D_800D3354[];
extern u16 D_800D335C[];
extern u8* D_800D3368[];
extern s32 D_80050100;
extern MATRIX D_800D30BC;
extern u_long* g_GfxCurOT;

// WorkListEntry/AnimTask of include/system/animation.h; overlay units cannot include
// that header (it holds tentative definitions of main-exe globals), so the layout is
// repeated. Its bitfields at 0x10/0x14 are whole words to this unit.
typedef struct {
    /* 0x00 */ void* unk0;
    /* 0x04 */ void* unk4; // pSpriteData, at least in some cases
    /* 0x08 */ void* onTriggerCallback;
    /* 0x0C */ void* onFreeCallback;
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ struct WorkListEntry* pNext;
} WorkListEntry; // Size: 0x1C

typedef struct {
    /* 0x00 */ WorkListEntry task1;
    /* 0x1C */ WorkListEntry task2;
    /* 0x38 */ SpriteData spriteData;
} AnimTask; // Size: 0x19C

extern void SpriteSetScale(SpriteData* pSprite, s16 scale);
extern void SpriteInitFromAnimFileWithClut(SpriteData* pSprite, void* pAnimFile, s16 arg2, s16 arg3,
                                           s16 arg4, s16 arg5, s16 arg6, s32 arg7);
extern AnimTask* WorkListsAddTasks(u32 allocSize, void* pData, void* pTimerCallback,
                                  void* pWorkCallback, void* pOnFreeCallback);
extern void AnimScriptTick(SpriteData* pSprite);
extern void SpriteApplyMovementStep(SpriteData* pSprite);
extern void SpritePlayAnimation(SpriteData* pSprite, s32 arg1);
void BattleActorTickAnimScript(u8* pTask);
void BattleActorDrawProjected(u8* pTask);
void BattleFreeActorAnimTask(AnimTask* pTask);
extern void SpriteSetDirection(SpriteData* pSprite, s16 arg1);
extern void func_80022B2C(SpriteData* pSprite);
extern s32 BattleAngleToPoint(SpriteData* p0, SpriteData* p1);
extern void func_800223B0(SpriteData* pSprite, s16 arg1);
extern void BattleSetStateStep(s32 arg0);
extern void BattleWaitForOtherActorAnimations(s32 arg0);
extern void BattleFaceActorsTowardEachOther(SpriteData* p0, SpriteData* p1);
extern void WorkListsRemoveEntriesForData(void* pTask);
extern void SpriteAnimFrameListRemove(void* pPayload);
extern void WorkListRemoveTask(void* pTask);
extern void TimerWorkListRemoveTask(void* pTask);
extern s32 BattleFindGroundTriangle(s16* pVec, s32 arg1, s32 arg2);
extern s32 BattleFindTriangleContainingPoint(s16* pVec);
extern void BattleProjectPointOntoIndexedTriangle(s16* pVec, s32 arg1, u8* pOut);
extern void SpriteDrawFrame(SpriteData* pSprite, u_long* pOT);
extern void* func_800BFC80(void* pArg0, s32 arg1, s32 arg2);
extern s32 D_80059464;
extern u8 D_800591AC;
extern void func_800B136C(void);
extern void func_800B8D7C(void);
extern void func_800BFBA0(void);
extern void func_800BC454(s32 arg0);
extern void BattleLoadActorArchiveFile(SpriteData* pSprite);
extern void func_800BEFF4(s32 actorIndex);
extern void func_800BF3E8(SpriteData* pSprite);
extern void func_800BC460(u16 mask);
extern void BattleLoadArchiveEntryFile(void);
extern void func_800B9508(SpriteData* pSprite);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B905C);
/* 86.54%: 516 bytes against retail's 512, so one instruction too many.  The head
   (the 0x48-stride index, the 0x15B dispatch, the two 0x40 flag stores and both
   BattleFaceActorsTowardEachOther calls) is exact, and so is the four-insn index
   recovery for pSlot.  Left over: (1) retail loads the sprite's and the target's
   flag words A8 then AC (A8 into $v0), ours AC then A8, because the mask is one
   expression `(1 << i) | (1 << j)` -- the `li a1,1` it hoists is right; (2) the
   x/+-0x50 pair: retail copies x into $a1 (`move a1,a2`) and re-issues
   `slt v0,a1,a3` in both branch delay slots, ours re-loads x with `lhu` and
   sign-extends it, and the two tests come out inverted (bne/beq) with a
   destructively-merged tail.  (3) the six u16 copies at the end: retail
   materialises the source base into $v0 and the destination into $v1, ours the
   other way round (D_800D30A0/D_800D3354/... spellings are right).
   Tried: if/else-if vs ternary for the +-0x50 pair, x reloaded vs reused,
   pTarget assigned twice (needed - retail loads 0x74 again after the call) vs
   once, mask as `mask = 1<<i; mask |= 1<<j` (gives the li late) vs one
   expression (li a1,1 hoisted, retail's shape).  A permuter job.
void func_800B905C(void) {
    s32 index;
    s32 slotIndex;
    s32 mask;
    s16 x;
    SpriteData* pSprite;
    SpriteData* pTarget;
    u8* pBase;
    u8* pSlot;

    pBase = D_800C3EB0;
    func_800BEFF4(pBase[D_800C360C * 0x48 + 0x15B]);
    pSprite = D_800C3610->pCurSprite;
    func_800BF3E8(pSprite);
    if (pBase[0xA78] != 0) {
        pTarget = pSprite->pTargetEntitySprite;
        pTarget->flags_0xAC |= 0x40;
        pSprite->flags_0xAC |= 0x40;
        BattleFaceActorsTowardEachOther(pSprite, pTarget);
        pTarget = pSprite->pTargetEntitySprite;
        mask = (1 << (((*(u32*)((u8*)pSprite + 0xAC) & 3) << 2) | (*(u32*)((u8*)pSprite + 0xA8) >> 30))) |
               (1 << (((*(u32*)((u8*)pTarget + 0xAC) & 3) << 2) | (*(u32*)((u8*)pTarget + 0xA8) >> 30)));
        func_800BC460(mask);
        index = *(u32*)((u8*)pTarget + 0xA8) >> 30;
        slotIndex = ((*(u32*)((u8*)pTarget + 0xAC) & 3) << 2) | index;
        pSlot = pBase + slotIndex * 0x1C;
        x = *(s16*)((u8*)pTarget + 0x2);
        if (x == *(u16*)(pSlot + 0xE) && *(s16*)((u8*)pTarget + 0xA) == *(u16*)(pSlot + 0x10)) {
            if (*(s16*)((u8*)pSprite + 0x2) < x) {
                *(s16*)((u8*)pSprite + 0xA0) = x - 0x50;
            } else {
                *(s16*)((u8*)pSprite + 0xA0) = x + 0x50;
            }
        } else if (x < *(u16*)(pSlot + 0xE)) {
            *(s16*)((u8*)pSprite + 0xA0) = x - 0x50;
        } else {
            *(s16*)((u8*)pSprite + 0xA0) = x + 0x50;
        }
        *(s16*)((u8*)pSprite + 0xA2) = 0;
        *(s16*)((u8*)pSprite + 0xA4) = *(s16*)((u8*)pTarget + 0xA);
        func_800BC460(mask);
        D_800D3354[0] = D_800D30A0[0];
        D_800D3354[1] = D_800D30A0[1];
        D_800D3354[2] = D_800D30A0[2];
        D_800D335C[0] = D_800D30A0[4];
        D_800D335C[1] = D_800D30A0[5];
        D_800D335C[2] = D_800D30A0[6];
        BattleFaceActorsTowardEachOther(pSprite, pTarget);
    }
    BattleLoadArchiveEntryFile();
    func_800B9508(pSprite);
}
*/

void BattleIncrementStateTimer(void) {
    BattleState* pState = D_800C3610;

    if (pState != NULL) {
        ++pState->unk34;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9284);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9508);

void BattleApplyStateStep(void) {
    D_800C3610->unk48 = 1;
    D_800C3610->unk49 = ((u8*)D_800C3610)[0x1C];
}
void BattleFaceActorsTowardEachOther(SpriteData* p0, SpriteData* p1) {
    if (p0 == p1) {
        return;
    }
    func_800223B0(p0, BattleAngleToPoint(p0, p1));
    SpriteSetDirection(p0, BattleAngleToPoint(p0, p1));
    if (((s8*)p1)[0xAF] != 0x15) {
        func_800223B0(p1, BattleAngleToPoint(p1, p0));
        SpriteSetDirection(p1, BattleAngleToPoint(p1, p0));
    }
}
void BattleActorSnapToTargetAndFace(SpriteData* pSprite, s32 arg1) {
    pSprite->position.x = pSprite->field_0xA0 << 16;
    pSprite->position.z = pSprite->field_0xA4 << 16;
    D_800C3610->unk48 = 1;
    BattleSetStateStep(4);
    SpritePlayAnimation(pSprite, ((s8*)pSprite)[0xB0]);
    BattleFaceActorsTowardEachOther(pSprite, arg1);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9C78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9F78);
void BattleLoadActorSlotAnimation(s32 index) {
    SpriteData* pSprite;
    u8* pBase;
    s32 slotOffset;

    D_80059464 = 0;
    D_800591AC = 0;
    func_800B136C();
    pBase = D_800C3EB0;
    slotOffset = 0x8C8C;
    if (pBase[index * 0x1C + 8] != 0) {
        func_800B8D7C();
        func_800BFBA0();
    } else if (index < 3) {
        pSprite = *(SpriteData**)(pBase + index * 4 + slotOffset);
        if (pSprite != NULL) {
            BattleLoadActorArchiveFile(pSprite);
        }
    } else {
        func_800B8D7C();
    }
    func_800BC454(0xC0);
}
void BattleActorSetStepFromAngle(SpriteData* pSprite, s16 angle) {
    s32 scale = pSprite->moveSpeed >> 3;

    pSprite->direction = angle;
    pSprite->step.x = ((rsin(angle) >> 1) * scale) >> 8;
    pSprite->step.z = -(((rcos(pSprite->direction) >> 1) * scale)) >> 8;
}
void BattleActorComputeFallDirection(SpriteData* pSprite) {
    VECTOR diff;
    SVECTOR vec;
    u8 out[0x10];
    s32 index;
    s32 len;
    s32 dy;
    s32 angle;

    SpriteSetSVec(&vec, pSprite->field_0xA0, pSprite->field_0xA2, pSprite->field_0xA4);
    index = BattleFindGroundTriangle((s16*)&vec, pSprite->field_0x78, 4);
    if (index < 0) {
        index = BattleFindTriangleContainingPoint((s16*)&vec);
    }
    BattleProjectPointOntoIndexedTriangle((s16*)&vec, index, out);
    if (vec.vy > pSprite->field_0xA2) {
        *(u16*)&vec.vy = *(u16*)&pSprite->field_0xA2;
    }
    dy = ((vec.vy << 16) - *(s32*)((u8*)pSprite + 0x4)) >> 16;
    diff.vx = pSprite->field_0xA0 - *(s16*)((u8*)pSprite + 0x2);
    diff.vz = pSprite->field_0xA4 - *(s16*)((u8*)pSprite + 0xA);
    angle = -ratan2(diff.vz, diff.vx);
    Square0(&diff, &diff);
    len = SquareRoot0(diff.vx + diff.vz);
    pSprite->step.y = ((-(s32)pSprite->gravity * len) << 4) / (pSprite->moveSpeed >> 11) +
                      (pSprite->moveSpeed * dy) / len;
    BattleActorSetStepFromAngle(pSprite, angle);
}
void BattleActorUpdateFallMotion(SpriteData* pSprite) {
    VECTOR diff;
    SVECTOR vec;
    u8 out[0x10];
    s32 index;
    s32 divisor;
    s32 angle;
    s32 len;
    s32 num;

    divisor = -((pSprite->step.y << 1) / (s32)pSprite->gravity);
    pSprite->position.x &= 0xFFFF0000;
    pSprite->position.y &= 0xFFFF0000;
    pSprite->position.z &= 0xFFFF0000;
    diff.vx = pSprite->field_0xA0 - *(s16*)((u8*)pSprite + 0x2);
    diff.vz = pSprite->field_0xA4 - *(s16*)((u8*)pSprite + 0xA);
    diff.vy = 0;
    angle = -ratan2(diff.vz, diff.vx);
    Square0(&diff, &diff);
    len = SquareRoot0(diff.vx + diff.vz);
    if (divisor != 0) {
        pSprite->moveSpeed = (len << 16) / divisor;
    } else {
        pSprite->moveSpeed = 0;
    }
    SpriteSetSVec(&vec, pSprite->field_0xA0, pSprite->field_0xA2, pSprite->field_0xA4);
    index = BattleFindGroundTriangle((s16*)&vec, pSprite->field_0x78, 4);
    if (index < 0) {
        index = BattleFindTriangleContainingPoint((s16*)&vec);
    }
    BattleProjectPointOntoIndexedTriangle((s16*)&vec, index, out);
    if (vec.vy > pSprite->field_0xA2) {
        *(u16*)&vec.vy = *(u16*)&pSprite->field_0xA2;
    }
    num = (vec.vy << 16) - *(s32*)((u8*)pSprite + 0x4);
    if (divisor != 0) {
        pSprite->step.y += num / divisor;
    }
    BattleActorSetStepFromAngle(pSprite, angle);
    func_80022B2C(pSprite);
}
void BattleActorUpdateGroundHeightAndTriangle(u8* pSprite) {
    s16 vec[3];
    u8 out[0x10];
    s32 coord;
    s32 index;

    coord = *(s16*)(pSprite + 0x2);
    vec[0] = coord;
    coord = *(s16*)(pSprite + 0x6);
    vec[1] = coord;
    coord = *(s16*)(pSprite + 0xA);
    vec[2] = coord;
    index = BattleFindGroundTriangle(vec, *(s32*)(pSprite + 0x78), 4);
    if (index < 0) {
        index = BattleFindTriangleContainingPoint(vec);
    }
    BattleProjectPointOntoIndexedTriangle(vec, index, out);
    *(u16*)(pSprite + 0x84) = vec[1];
    *(s32*)(pSprite + 0x78) = index;
}
// The two unused slots keep the caller's argument area at retail's offsets.
AnimTask* BattleCreateAnimTask(void* pAnimFile, s16 clutX, s16 clutY, s16 texX, s16 texY, s16 unk5, s16 posX,
                        s16 posY, s16 posZ, s16 animId, u16 direction, s32 unk84, s32 unk88, s32 clut) {
    AnimTask* pTask;
    SpriteData* pSprite;

    pTask = WorkListsAddTasks(sizeof(AnimTask), NULL, BattleActorTickAnimScript,
                              BattleActorDrawProjected, BattleFreeActorAnimTask);
    pSprite = &pTask->spriteData;
    pTask->task1.unk4 = pSprite;
    pTask->task2.unk4 = pSprite;
    pTask->task2.unk0 = NULL;
    SpriteInitFromAnimFileWithClut(pSprite, pAnimFile, clutX, clutY, texX, texY, unk5, clut);
    pSprite->field_0x6C = pTask;
    pSprite->position.x = posX << 16;
    pSprite->position.y = posY << 16;
    pSprite->position.z = posZ << 16;
    *(u8*)((u8*)pSprite + 0xB0) = animId;
    *(u32*)((u8*)pSprite + 0x3C) |= 4;
    pSprite->direction = direction;
    SpriteSetScale(pSprite, 0x2000);
    pSprite->field_0x82 = 0x2000;
    pSprite->field_0x78 = 0;
    SpritePlayAnimation(pSprite, animId);
    return pTask;
}
// RotTransPers output block; the tail is unused but reproduces retail's 0x40 frame.
typedef struct {
    s32 sxy;
    s32 flag;
    s32 pad[4];
} RotTransPersOut;

void BattleActorDrawProjected(u8* pTask) {
    SpriteData* pSprite;
    SVECTOR vec;
    s32 coord;
    s32 otz;
    RotTransPersOut out;

    if (D_800C3664 != 0) {
        return;
    }
    pSprite = *(SpriteData**)(pTask + 0x4);
    coord = *(s16*)((u8*)pSprite + 0x2);
    vec.vx = coord;
    coord = *(s16*)((u8*)pSprite + 0x6);
    vec.vy = coord;
    coord = *(s16*)((u8*)pSprite + 0xA);
    vec.vz = coord;
    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    otz = (RotTransPers(&vec, &out.sxy, (u_long*)&out.sxy, &out.flag) >> D_80050100) +
          *(s16*)((u8*)pSprite + 0x30);
    if (out.flag & 0x8000) {
        otz = 0;
    }
    *(s16*)((u8*)pSprite + 0x2E) = otz;
    if ((u32)(otz - 1) < 0xFFF) {
        SpriteDrawFrame(pSprite, g_GfxCurOT + otz);
    }
}

void BattleFreeActorAnimTask(AnimTask* pTask) {
    AnimTask* pSelf;
    SpriteData* pSprite;
    SpriteData* pSpriteCopy;
    SpriteData* pLoadSprite;
    WorkListEntry* pTaskEntry2;
    void* pBlock;

    pSelf = pTask;
    pSprite = &pSelf->spriteData;
    pSpriteCopy = pSprite;
    pLoadSprite = pSpriteCopy;
    pBlock = pLoadSprite->pBase->pFramesData;
    if (pBlock != NULL) {
        HeapFree(pBlock);
    }
    pTaskEntry2 = &pSelf->task2;
    WorkListsRemoveEntriesForData(pSelf);
    SpriteAnimFrameListRemove(pLoadSprite);
    WorkListRemoveTask(pTaskEntry2);
    TimerWorkListRemoveTask(pSelf);
    HeapFree(pSelf);
}

void BattleActorTickAnimScript(u8* pTask) {
    SpriteData* pSprite = *(SpriteData**)(pTask + 0x4);

    if (D_800C3664 == 0) {
        AnimScriptTick(pSprite);
        SpriteApplyMovementStep(pSprite);
        if (((u32)pSprite->flags_0xAC >> 6) & 1) {
            AnimScriptTick(pSprite);
            SpriteApplyMovementStep(pSprite);
        }
    }
}
void BattleSpriteProjectToScreen(s32 index, s16* p0, s16* p1, s16* p2, s16* p3, s16* p4, s16* p5) {
    u8* pBase = D_800C3EB0;
    s32 vx;
    s32 vy;
    s32 vz;
    SpriteData* pSprite;
    SVECTOR vec;
    u16 sxy[2];
    s32 flag;

    pSprite = *(SpriteData**)(pBase + index * 4 + 0x8C8C);
    vx = *(s16*)((u8*)pSprite + 0x2);
    vec.vx = vx;
    vy = *(s16*)((u8*)pSprite + 0x6);
    vec.vy = vy;
    vz = *(s16*)((u8*)pSprite + 0xA);
    vec.vz = vz;
    PushMatrix();
    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    *p2 = RotTransPers(&vec, (long*)sxy, &flag, &flag) >> 4;
    *p0 = sxy[0];
    *p1 = sxy[1];
    *p3 = sxy[0] - 0x30;
    *p4 = 0x30;
    *p5 = sxy[0] - 0x18;
    PopMatrix();
}
void BattleReleaseActorSlot(s32 index) {
    u8* pBase = D_800C3EB0;
    u8* pBlock;
    u8* pTail;
    SpriteData* pSprite;
    void* pPayload;
    u32* pSlot;
    u32* pSlot2;

    pSlot = (u32*)(pBase + index * 4 + 0x8CB8);
    pSprite = *(SpriteData**)pSlot;
    if (pSprite == NULL) {
        return;
    }
    func_800BFC80(pSprite, 0, 2);
    if (index < 3) {
        pBlock = pBase + index * 0xC + 0x8000;
        pPayload = *(void**)(pBlock + 0xD24);
        if (pPayload != NULL) {
            HeapFree(pPayload);
        }
        *(u32*)(pBlock + 0xD24) = 0;
    }
    (*(void (**)(SpriteData*))((u8*)pSprite + 0xC))(pSprite);
    WorkListsRemoveEntriesForData(pSprite);
    pTail = D_800C3EB0;
    pSlot = (u32*)(pTail + index * 4 + 0x8C8C);
    *pSlot = 0;
    pSlot2 = (u32*)(pTail + index * 4 + 0x8CB8);
    *pSlot2 = 0;
}
/* 92.04%: the 0x8C8C constant-fold wall, second shape.  With pBase a local the
   tail's instruction stream is retail's exactly (`li v0,0x8C8C; addu; sw 0` and
   the same for 0x8CB8), but gcc then keeps pBase live in $s3 across the calls
   and saves it, where retail re-forms lui/addiu for D_800C3EB0 and reuses $s1
   for the if-block pointer.  Spelling the tail address with the global
   (`D_800C3EB0 + index*4 + 0x8C8C`) instead makes gcc fold the constant into
   the symbol (lui 0x1; addiu -29556; sw 0x2C) and reach 94.02%, and routing it
   through a cast `(mem_addr)` pointer gets the frame right but splits the add
   into li 0x8000 + displacement (92.75%).  Everything else, frame and the
   0x8000/D24 if-block included, is exact.
/*
void BattleReleaseActorSlot(s32 index) {
    u8* pBase = D_800C3EB0;
    SpriteData* pSprite;
    u8* pSlot;
    void* pPayload;

    pSprite = *(SpriteData**)(pBase + index * 4 + 0x8CB8);
    if (pSprite == NULL) {
        return;
    }
    func_800BFC80(pSprite, 0, 2);
    if (index < 3) {
        pSlot = pBase + index * 0xC + 0x8000;
        pPayload = *(void**)(pSlot + 0xD24);
        if (pPayload != NULL) {
            HeapFree(pPayload);
        }
        *(u32*)(pSlot + 0xD24) = 0;
    }
    (*(void (**)(SpriteData*))((u8*)pSprite + 0xC))(pSprite);
    WorkListsRemoveEntriesForData(pSprite);
    *(u32*)(pBase + index * 4 + 0x8C8C) = 0;
    *(u32*)(pBase + index * 4 + 0x8CB8) = 0;
}
*/
void BattleActorSetFacingFromSideFlag(s32 index) {
    u8* pBase = D_800C3EB0;
    u8* pEntry = pBase + index * 4;
    u8* pCounter;
    SpriteData** ppSlot;
    SpriteData* pSprite;
    s32 sfx;

    ppSlot = (SpriteData**)(pEntry + 0x8C8C);
    pSprite = *ppSlot;
    if (((s8*)pSprite)[0xAF] == 0x15) {
        return;
    }
    pCounter = pBase + index * 0x1C;
    sfx = (pCounter[0xA] != 0) << 11;
    func_800223B0(pSprite, sfx);
    SpriteSetDirection(pSprite, sfx);
}
/* 75.68%: the known 0x8C8C wall (same as misc28's func_800B64D4).  Retail keeps
   &D_800C3EB0 in $a1 and adds 0x8C8C with a separate li+addu; gcc always folds
   the constant into the symbolic address as lui at,0x1 / lw -29556(at).
   Tried: raw (u8*)D_800C3EB0, base pointer local, base + separate offset local,
   gcc 2.6.0, 2.7.2-cdk (cc1 rejects), -O3.  The second half (index*0x1C + 0xA
   access, the lb 0xAF compare and both calls) matches exactly under the base
   pointer form, so only this one address is wrong.
void BattleActorSetFacingFromSideFlag(s32 index) {
    SpriteData* pSprite;
    s32 sfx;

    pSprite = *(SpriteData**)((u8*)D_800C3EB0 + index * 4 + 0x8C8C);
    if (((s8*)pSprite)[0xAF] == 0x15) {
        return;
    }
    sfx = (D_800C3EB0[index * 0x1C + 0xA] != 0) << 11;
    func_800223B0(pSprite, sfx);
    SpriteSetDirection(pSprite, sfx);
}
*/
void BattleNoOpCallback(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BAF48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BB080);
/* 76.91%: same 0x8C8C fold as BattleActorSetFacingFromSideFlag.  Retail keeps the raw
   &D_800C3EB0 in $s0 and hoists 0x8C8C into $s4 (`li s4,0x8C8C; addu s2,s0,s4`),
   then adds 0x8CB8 with a second li+addu after the call; gcc folds 0x8CB8 into
   the symbol base (lui s0,0x1; addiu s0,-29512) and uses -0x2C for the other
   access, which also drags the loop-bound `3` into a register (li s3,3).
   The call through pSprite->field_0x6C+0xC, the three break tests and the
   BattleWaitForOtherActorAnimations tail are otherwise identical.
   Also tried: named pointer locals for the two zero stores (the ppSlot trick
   that closed BattleActorSetFacingFromSideFlag and BattleReleaseActorSlot here); loop.c then hoists BOTH
   0x8C8C and 0x8CB8 into callee-saved registers and the frame grows to 0x30.
void func_800BB080(s32 arg0) {
    u8* pEntry = D_800C3EB0;
    SpriteData* pSprite;
    void* pObj;
    s32 i;

    for (i = 0; i != 3; i++) {
        if (i == arg0) {
            break;
        }
        pSprite = *(SpriteData**)(pEntry + 0x8C8C);
        if (pSprite == NULL) {
            break;
        }
        if (pSprite->pDefaultAnimFile != NULL) {
            break;
        }
        pObj = *(void**)((u8*)pSprite + 0x6C);
        (*(void (**)(void*))((u8*)pObj + 0xC))(pObj);
        *(u32*)(pEntry + 0x8C8C) = 0;
        *(u32*)(pEntry + 0x8CB8) = 0;
        pEntry += 4;
    }
    BattleWaitForOtherActorAnimations(arg0);
}
*/
void BattleActorSyncPositionFromModel(u8* pTask) {
    SpriteData* pSprite;
    u8* pScript;
    s32 mode;
    s32 index;
    u32 unused[2]; // unused: reproduces retail's 0x28 frame

    pSprite = *(SpriteData**)(pTask + 0x4);
    mode = *(u32*)((u8*)pSprite + 0xA8) >> 30;
    index = ((*(u32*)((u8*)pSprite + 0xAC) & 3) << 2) | mode;
    pScript = D_800D3368[index];
    if (pScript == NULL) {
        return;
    }
    if (pSprite->animScriptWaitTimer == 0) {
        pSprite->pSpriteBytecode = NULL;
    }
    if (pSprite->pSpriteBytecode != NULL) {
        AnimScriptTick(pSprite);
        SpriteApplyMovementStep(pSprite);
        if (((u32)pSprite->flags_0xAC >> 6) & 1) {
            AnimScriptTick(pSprite);
            SpriteApplyMovementStep(pSprite);
        }
    }
    pSprite->position.x = *(u32*)(*(u32*)(pScript + 0x4) + 0x5C) << 16;
    pSprite->position.y = *(u32*)(*(u32*)(pScript + 0x4) + 0x60) << 16;
    pSprite->position.z = *(u32*)(*(u32*)(pScript + 0x4) + 0x64) << 16;
}

