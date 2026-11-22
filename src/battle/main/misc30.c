#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgte.h"

// Battle state pointed to by g_BattleState. Only the fields this unit touches are recovered.
typedef struct {
    /* 0x00 */ u8 pad00[0x4];
    /* 0x04 */ SpriteData* pCurSprite;
    /* 0x08 */ u8 pad08[0x14];
    /* 0x1C */ u8 stateStep; // state step, copied to 0x49 when the step changes
    /* 0x1D */ u8 pad1D[0x3];
    /* 0x20 */ u32 initialActorIndex; // actor index BattleSetCurrentActorSprite is handed at init
    /* 0x24 */ s32 actorIndex; // index of the acting slot's sprite in the 0x8C8C table
    /* 0x28 */ u8 pad28[0xC];
    /* 0x34 */ u32 timer; // counted down once a frame by the state loop
    /* 0x38 */ u8 pad38[0x10];
    /* 0x48 */ u8 pendingAction; // 0x905C request: the frame loop runs it once when this is set
    /* 0x49 */ u8 pendingStep; // step the state loop dispatches once, then clears
    /* 0x4A */ u8 commandUnhandled; // set by the command switch's default arm; the next dispatch clears it
    /* 0x4B */ u8 pad4B;
} BattleState;

extern BattleState* g_BattleState;
extern u8 D_800C3664;
extern u8 g_BattleStateWork[];
extern s32 g_BattleCommandRowIndex;
extern u16 g_BattleActorSlotDoneMask;
extern u16 D_800C3614;
extern u16 g_BattleActorAnimationIndex;
extern u8 D_800C3623;

// The per-slot vectors retail copies from 0x800D30A0 to 0x800D3354.
extern u16 g_BattleAnimGroupStates[];
extern u16 g_BattleSlotVectors[];
extern u16 g_BattleSlotVectorsB[];
extern s32 g_GfxOTDepthShift;
extern MATRIX g_BattleViewMatrix;
extern u_long* g_GfxCurOT;

// WorkListEntry/AnimTask of include/system/animation.h; overlay units cannot include
// that header (it holds tentative definitions of main-exe globals), so the layout is
// repeated. Its bitfields at 0x10/0x14 are whole words to this unit.
typedef struct {
    /* 0x00 */ void* unk0;
    /* 0x04 */ struct BattleActorSprite* pSpriteData; // the task's own spriteData in every entry this unit builds
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

// A battle actor's 16.16 position, read as whole words by the code that moves
// the actor and as integer halfwords by the code that projects it.
typedef union {
    /* 0x0 */ IVEC3 value;
    /* 0x0 */ BattleFixed16Vector parts;
} BattleActorPosition; // Size: 0xC

// The 0xAC flag word: its low two bits are the model slot index and its top
// byte is the animation id the callers compare against 0x15/0x16.
typedef union {
    /* 0x0 */ u32 flags;
    struct {
        /* 0x0 */ u8 unk0;
        /* 0x1 */ u8 unk1;
        /* 0x2 */ u8 unk2;
        /* 0x3 */ s8 animId;
    } bytes;
} BattleActorFlagWord; // Size: 0x4

// Battle-side view of a battle actor's SpriteData (include/field/actor.h).
// BattleSpriteState in include/battle/actor.h stops at 0xA8 and leaves +0x78 and
// +0x84 unnamed, which this unit reads, so the whole view lives here.
typedef struct BattleActorSprite {
    /* 0x00 */ BattleActorPosition position;
    /* 0x0C */ IVEC3 step;
    /* 0x18 */ s32 moveSpeed;
    /* 0x1C */ u32 gravity;
    /* 0x20 */ SpriteDataI1* pBase;
    /* 0x24 */ void* pVramData; // the SpriteAnimPackage whose pFrames the animation test reads
    /* 0x28 */ u8 unk28[0x6];
    /* 0x2E */ s16 field_0x2e;
    /* 0x30 */ s16 field_0x30;
    /* 0x32 */ s16 direction;
    /* 0x34 */ u8 unk34[0x8];
    /* 0x3C */ u32 flags; // 0x4: transform matrix dirty
    /* 0x40 */ u8 unk40[0x10];
    /* 0x50 */ u32 pSedsFile; // the SEDS bank BattleLoadSoundSeds hands this sprite
    /* 0x54 */ u8 unk54[0x10];
    /* 0x64 */ void* pSpriteBytecode;
    /* 0x68 */ void* pCallback; // the slot SpriteSetCallback stores into
    /* 0x6C */ void* field_0x6C;
    /* 0x70 */ u8 unk70[0x4];
    /* 0x74 */ struct BattleActorSprite* pTargetEntitySprite;
    /* 0x78 */ u32 field_0x78;
    /* 0x7C */ u8 unk7C[0x4];
    /* 0x80 */ s16 field_0x80;
    /* 0x82 */ s16 field_0x82;
    /* 0x84 */ s16 field_0x84;
    /* 0x86 */ u8 unk86[0x2];
    /* 0x88 */ u8 unk88[0x16];
    /* 0x9E */ s16 animScriptWaitTimer;
    /* 0xA0 */ SVEC targetPosition;
    /* 0xA6 */ u8 unkA6[0x2];
    /* 0xA8 */ u32 flags_0xA8;
    /* 0xAC */ BattleActorFlagWord flags_0xAC;
    /* 0xB0 */ s8 animId;
    /* 0xB1 */ u8 unkB1[0x3];
} BattleActorSprite; // Size: 0xB4

// The model position a per-slot record points at, copied into the actor's own
// position by BattleActorSyncPositionFromModel.
typedef struct {
    /* 0x00 */ u8 unk0[0x5C];
    /* 0x5C */ s32 x;
    /* 0x60 */ s32 y;
    /* 0x64 */ s32 z;
} BattleModelPosition; // Size: 0x68

typedef struct {
    /* 0x0 */ u8 unk0[0x4];
    /* 0x4 */ BattleModelPosition* pModelPosition;
} BattleSlotRecord; // Size: 0x8

// The 0x1C-byte per-slot records at the head of g_BattleStateWork.
typedef struct {
    /* 0x00 */ u8 unk0[0x7];
    /* 0x07 */ u8 flags; // tested against 0 only; while set, the 0xFF state arm replays no animation
    /* 0x08 */ u8 inUse; // non-zero while the slot is claimed; every reader here tests it against 0
    /* 0x09 */ u8 unk9;
    /* 0x0A */ u8 sideFlag; // non-zero faces the sprite 0x800 (half a turn) instead of 0
    /* 0x0B */ u8 unkB[0x3];
    /* 0x0E */ u16 x; // whole x, compared with the actor's own
    /* 0x10 */ u16 z; // whole z, compared with the actor's own
    /* 0x12 */ u8 unk12[0xA];
} BattleActorSlot; // Size: 0x1C

// One pointer of the per-slot table at g_BattleStateWork + 0x8CB8: an object
// whose +0xC entry retail calls with the object itself while the slot is being
// released, right before the work lists drop the same pointer.
typedef struct BattleSlotResource {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ void (*pOnFree)(struct BattleSlotResource* pSelf);
} BattleSlotResource; // Size: 0x10

extern BattleSlotRecord* g_BattleActors[];

extern void SpriteSetScale(BattleActorSprite* pSprite, s16 scale);
extern void SpriteInitFromAnimFileWithClut(BattleActorSprite* pSprite, void* pAnimFile, s16 arg2,
                                           s16 arg3, s16 arg4, s16 arg5, s16 arg6, s32 arg7);
extern AnimTask* WorkListsAddTasks(u32 allocSize, void* pData, void* pTimerCallback,
                                  void* pWorkCallback, void* pOnFreeCallback);
extern void AnimScriptTick(BattleActorSprite* pSprite);
extern void SpriteApplyMovementStep(BattleActorSprite* pSprite);
extern void SpritePlayAnimation(BattleActorSprite* pSprite, s32 arg1);
void BattleActorTickAnimScript(AnimTask* pTask);
void BattleActorDrawProjected(AnimTask* pTask);
void BattleFreeActorAnimTask(AnimTask* pTask);
extern void SpriteSetDirection(BattleActorSprite* pSprite, s16 arg1);
extern void func_80022B2C(BattleActorSprite* pSprite);
extern s32 BattleAngleToPoint(BattleActorSprite* p0, BattleActorSprite* p1);
extern void func_800223B0(BattleActorSprite* pSprite, s16 arg1);
extern void BattleSetStateStep(s32 arg0);
extern void BattleWaitForOtherActorAnimations(s32 arg0);
extern void BattleFaceActorsTowardEachOther(BattleActorSprite* p0, BattleActorSprite* p1);
extern void WorkListsRemoveEntriesForData(void* pTask);
extern void SpriteAnimFrameListRemove(void* pPayload);
extern void WorkListRemoveTask(void* pTask);
extern void TimerWorkListRemoveTask(void* pTask);
extern s32 BattleFindGroundTriangle(s16* pVec, s32 arg1, s32 arg2);
extern s32 BattleFindTriangleContainingPoint(s16* pVec);
extern void BattleProjectPointOntoIndexedTriangle(s16* pVec, s32 arg1, u8* pOut);
extern void SpriteDrawFrame(BattleActorSprite* pSprite, u_long* pOT);
extern void* BattleFindAnimationTask(void* pArg0, s32 arg1, s32 arg2);
extern s32 g_NumFlaggedTimerWorkListEntries;
extern u8 g_FlagTimerWorkListAdds;
extern void BattleWaitAndFreeActorBuffers(void);
extern void BattleUnloadActorArchive(void);
extern void BattleLoadSoundWdsFile(void);
extern void BattleSetAnimTaskShort(s32 arg0);
extern void BattleLoadActorArchiveFile(BattleActorSprite* pSprite);
extern void BattleSetCurrentActorSprite(s32 actorIndex);
extern void func_800BF3E8(BattleActorSprite* pSprite);
extern void func_800BC460(u16 mask);
extern void BattleLoadArchiveEntryFile(void);
extern void func_800B9508(BattleActorSprite* pSprite);
extern void SpriteSetCallback(BattleActorSprite* pSprite, void* pCallback);
extern void BattleSetSpriteTemplate(void* pArg);
extern void BattleActorWaitFramesWithCallback(s32 arg0, BattleActorSprite* pSprite);
extern void BattleSetPendingCommand(BattleActorSprite* pSprite);
extern void BattleBeginAnimTask(s32 arg0);
extern void BattleSetSlotFlagB4(s32 arg0);
extern void BattleClearSlotFlagB4(s32 arg0);
extern void BattleWaitArchiveSync(void);
extern void BattleSetActorPrimaryCommand(s32 arg0, u16 arg1);
extern void BattleSwapActorSlots(s32 arg0, s32 arg1);
extern void func_800BEE2C(s32 arg0, s32 arg1, s32 arg2);
extern void func_800B9284(SpriteData* pSprite, s32 code);
extern u16 D_800D39E4;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B905C);
/* 94.47%, 508 bytes against retail's 512, so ONE instruction short.  Everything
   the earlier 86.54% park was missing is now exact and driven by the source:
   the mask loads the 0xA8 word before the 0xAC one because each index is
   written `(A8 >> 30) | ((AC & 3) << 2)` (the AC-left spelling flips the `or`
   destination to the AC word and swaps the loads -- 85.60%); the x/0x50 block
   is laid out retail's way (bne to the non-equal body, beq to the equal one,
   the `slt` in the beq's delay slot) only when written as
   `if (x != pSlot->x || z != pSlot->z) { non-equal } else { equal }` --
   the `==`/`&&` form puts the equal body first with inverted branches; and the
   z store needs an s32 local (an s16 store value makes gcc use lhu, retail lh).
   Left over, both allocator ties: retail keeps x in $a2 for the +-0x50
   arithmetic and a COPY in $a1 for the compares (`move a1,a2`) and so re-issues
   `slt v0,a1,a3` in BOTH branch delay slots -- that duplicate is the missing
   instruction; and the six u16 copies at the end carry the source base in $v0
   and the destination in $v1 where ours are swapped.
   Tried: s16 vs s32 for x and z, `&&` vs `||`, nested if vs else-if, AC-left vs
   A8-left mask, one expression vs index locals (locals defer both `sllv` past
   the second index), and a 4-minute permuter run (base=1388, best=1188) whose
   only usable find was the z hoist kept below.
void func_800B905C(void) {
    BattleActorSprite* pSprite;
    BattleActorSprite* pTarget;
    BattleActorSlot* pSlot;
    u8* pBase;
    s32 index;
    s32 mask;
    s32 x;
    s32 z;

    pBase = g_BattleStateWork;
    BattleSetCurrentActorSprite(pBase[g_BattleCommandRowIndex * 0x48 + 0x15B]);
    pSprite = (BattleActorSprite*)g_BattleState->pCurSprite;
    func_800BF3E8(pSprite);
    if (pBase[0xA78] != 0) {
        pTarget = pSprite->pTargetEntitySprite;
        pTarget->flags_0xAC.flags |= 0x40;
        pSprite->flags_0xAC.flags |= 0x40;
        BattleFaceActorsTowardEachOther(pSprite, pTarget);
        pTarget = pSprite->pTargetEntitySprite;
        mask = (1 << ((pSprite->flags_0xA8 >> 30) | ((pSprite->flags_0xAC.flags & 3) << 2))) |
               (1 << ((pTarget->flags_0xA8 >> 30) | ((pTarget->flags_0xAC.flags & 3) << 2)));
        func_800BC460(mask);
        index = (pTarget->flags_0xA8 >> 30) | ((pTarget->flags_0xAC.flags & 3) << 2);
        pSlot = &((BattleActorSlot*)pBase)[index];
        x = pTarget->position.parts.xWhole;
        if (x != pSlot->x || pTarget->position.parts.zWhole != pSlot->z) {
            if (x < pSlot->x) {
                pSprite->targetPosition.x = x - 0x50;
            } else {
                pSprite->targetPosition.x = x + 0x50;
            }
        } else if (pSprite->position.parts.xWhole < x) {
            pSprite->targetPosition.x = x - 0x50;
        } else {
            pSprite->targetPosition.x = x + 0x50;
        }
        z = pTarget->position.parts.zWhole;
        pSprite->targetPosition.y = 0;
        pSprite->targetPosition.z = z;
        func_800BC460(mask);
        g_BattleSlotVectors[0] = g_BattleAnimGroupStates[0];
        g_BattleSlotVectors[1] = g_BattleAnimGroupStates[1];
        g_BattleSlotVectors[2] = g_BattleAnimGroupStates[2];
        g_BattleSlotVectorsB[0] = g_BattleAnimGroupStates[4];
        g_BattleSlotVectorsB[1] = g_BattleAnimGroupStates[5];
        g_BattleSlotVectorsB[2] = g_BattleAnimGroupStates[6];
        BattleFaceActorsTowardEachOther(pSprite, pTarget);
    }
    BattleLoadArchiveEntryFile();
    func_800B9508(pSprite);
}
*/

void BattleIncrementStateTimer(void) {
    BattleState* pState = g_BattleState;

    if (pState != NULL) {
        ++pState->timer;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9284);
/* 96.61%, 644 bytes -- retail's exact size, 169 instructions both sides, and the
   instruction ORDER is now identical in all eight arms.  The only remaining
   difference, in each of the seven arms that build the slot address, is which of
   two registers gcc picks: retail materialises the base into $v1 and the slot
   index root into $a0 (`lui v1,%hi(g_BattleStateWork) / lui v0,%hi(g_BattleCommandRowIndex) /
   lw a0,%lo(g_BattleCommandRowIndex)(v0) / addiu v1,v1,%lo(g_BattleStateWork) / sll v0,a0,3 /
   addu v0,v0,a0 / sll v0,v0,3 / addu v0,v0,v1`), ours swaps those two ($a0 base,
   $v1 root) -- a GIV/allocator tie, not a shape error.  Two levers that DID move
   things and are kept here: the base in its own pointer local assigned per arm
   (`pBase = g_BattleStateWork;` then `pSlot = pBase + g_BattleCommandRowIndex * 0x48;`) fixes the
   emission ORDER (a plain one-statement form materialises the base after the
   index load and comes out 660 bytes / 79.10%), and the 0xF3 arm's operand load
   split into `id` (instead of `g_BattleActorSlotDoneMask |= *(u16 *)(pSlot + 0x172);` inline)
   fixes that arm's lhu order.  Tried and worse: one-statement `pSlot = g_BattleStateWork
   + slot * 0x48`, `pSlot = (u8 *)((mem_addr)g_BattleStateWork + g_BattleCommandRowIndex * 0x48)`
   (664 bytes), `pSlot +=`, `&pBase[idx]`, index-first operand order, declaration
   reorders, and three permuter runs (their closest candidates, which split
   `D_800C3623 = 1` through a temp, measure WORSE under objdiff than this body).
   Round 176 re-measured 96.61% and re-tried: an `s32 slot` index local (87.79%, it
   moves the index load above the base materialisation), the index-first sum
   `idx * 0x48 + pBase` (96.61%), declaring pBase before pSlot (96.61%) and the
   0xF3 arm's RMW written inline again (96.40%).  The 388-byte/0xF3 arm also
   differs in ORDER: retail's `lui a1,%hi(g_BattleActorSlotDoneMask)` is hoisted to the top of the
   arm and its `lhu` of the global precedes the slot `lhu`. */
/*
void func_800B9284(SpriteData* pSprite, s32 code) {
    u8 *pSlot;
    u8 *pBase;
    s32 id;

    switch (code) {
        case 0xFA:
            g_BattleState->pendingAction = 1;
            SpriteSetCallback(pSprite, 0);
            pBase = g_BattleStateWork;
            pSlot = pBase + g_BattleCommandRowIndex * 0x48;
            BattleSetSlotFlagB4(pSlot[0x172]);
            break;
        case 0xF8:
            g_BattleState->pendingAction = 1;
            SpriteSetCallback(pSprite, 0);
            pBase = g_BattleStateWork;
            pSlot = pBase + g_BattleCommandRowIndex * 0x48;
            BattleClearSlotFlagB4(pSlot[0x172]);
            break;
        case 0xF7:
            g_BattleState->pendingAction = 1;
            SpriteSetCallback(pSprite, 0);
            pBase = g_BattleStateWork;
            pSlot = pBase + g_BattleCommandRowIndex * 0x48;
            D_800C3614 = *(u16 *)(pSlot + 0x172);
            break;
        case 0xF5:
            BattleSetSpriteTemplate(pSprite);
            pBase = g_BattleStateWork;
            pSlot = pBase + g_BattleCommandRowIndex * 0x48;
            id = *(u16 *)(pSlot + 0x172);
            g_BattleActorAnimationIndex = (id >> 9) & 0x3F;
            ((u8 *)pSprite)[0xAF] = 0x11;
            BattleActorWaitFramesWithCallback(id & 0x1FF, pSprite);
            BattleSetPendingCommand(pSprite);
            SpritePlayAnimation(pSprite, 0x13);
            D_800C3623 = 1;
            break;
        case 0xF4:
            BattleSetSpriteTemplate(pSprite);
            pBase = g_BattleStateWork;
            pSlot = pBase + g_BattleCommandRowIndex * 0x48;
            id = *(u16 *)(pSlot + 0x172);
            g_BattleActorAnimationIndex = (id >> 9) & 0x3F;
            ((u8 *)pSprite)[0xAF] = 0x11;
            BattleActorWaitFramesWithCallback(id & 0x1FF, pSprite);
            break;
        case 0xF6:
            pBase = g_BattleStateWork;
            pSlot = pBase + g_BattleCommandRowIndex * 0x48;
            BattleBeginAnimTask(*(u16 *)(pSlot + 0x14E));
            break;
        case 0xF3:
            g_BattleState->pendingAction = 1;
            pBase = g_BattleStateWork;
            pSlot = pBase + g_BattleCommandRowIndex * 0x48;
            id = *(u16 *)(pSlot + 0x172);
            g_BattleActorSlotDoneMask |= id;
            break;
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9508);

void BattleApplyStateStep(void) {
    g_BattleState->pendingAction = 1;
    g_BattleState->pendingStep = g_BattleState->stateStep;
}
void BattleFaceActorsTowardEachOther(BattleActorSprite* p0, BattleActorSprite* p1) {
    if (p0 == p1) {
        return;
    }
    func_800223B0(p0, BattleAngleToPoint(p0, p1));
    SpriteSetDirection(p0, BattleAngleToPoint(p0, p1));
    if (p1->flags_0xAC.bytes.animId != 0x15) {
        func_800223B0(p1, BattleAngleToPoint(p1, p0));
        SpriteSetDirection(p1, BattleAngleToPoint(p1, p0));
    }
}
void BattleActorSnapToTargetAndFace(BattleActorSprite* pSprite, BattleActorSprite* pTarget) {
    pSprite->position.value.x = pSprite->targetPosition.x << 16;
    pSprite->position.value.z = pSprite->targetPosition.z << 16;
    g_BattleState->pendingAction = 1;
    BattleSetStateStep(4);
    SpritePlayAnimation(pSprite, pSprite->animId);
    BattleFaceActorsTowardEachOther(pSprite, pTarget);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9C78);
/* 76.54%, 800 bytes against retail's 768 (192 insns).  The head is exact -- the
   D_800C3614 lh/lhu pair (a signed test plus the unsigned decrement, hence the
   `*(s16 *)&D_800C3614` read), the 0x48-stride row index, the 0x8C8C sprite
   table with `offset = 0x8C8C` in a local, the 0x4A clear and the `sb` of 0xA73.
   The dispatch is retail's too (13-entry table 0xF3..0xFF, the 0xF9 hole taking
   the default label, bodies emitted in retail's order 0xFF, 0xFE, 0xFC, 0xFD,
   the five-way group, 0xFB, 0xF4, default).

   Two things are left, and both are known shapes:
   1. The arms need the row base in a REGISTER, not folded into the symbol:
      writing `*(u16 *)(g_BattleStateWork + g_BattleCommandRowIndex * 0x48 + 0x14E)` makes gcc fold
      0x14E into the lui/addiu pair (`addiu v1,v1,334` + `lhu a1,0(v0)`, 744
      bytes, 77.18%) where retail keeps `addiu v1,v1,%lo(g_BattleStateWork)` and loads
      `lhu a1,334(v0)`.  Tree fold reassociates sym+const, so the constant has to
      stay out of the address tree: two statements
      (`pRow = g_BattleStateWork; pRow += g_BattleCommandRowIndex * 0x48;`) reproduce retail's
      instruction multiset and displacement in every arm, at the cost of the
      register ties below.  A single assignment (`pRow = g_BattleStateWork + idx * 0x48`)
      already emits the base after the index load and leaves a `nop` in the load
      delay slot; using the function-wide pBase for it puts the base in $s2.
   2. The 0xFD and default arms keep separate copies: retail cross-jumps them into
      one tail (a0/a2 set before the shared block, +12 insns shared), mine emits
      `move a0,s3` inside the block at a different position in each arm, so
      jump.c finds nothing identical.  That is the +8 instructions.
   Register ties in every arm once (1) is right: retail keeps the base in $v1 and
   the sum in $v0, ours picks a1/v0 or a0/v0; and code/index land in $s3/$s2
   where retail has $s2/$s3.  A 4-minute permuter run (base=5831, best=3891) only
   found a semantically wrong rewrite of the 0xF4 increment, so no help there.
void func_800B9C78(void) {
    BattleActorSprite* pSprite;
    u8* pBase;
    u8* pRow;
    u8 code;
    s32 index;
    s32 offset;
    s32 id;

    if (*(s16 *)&D_800C3614 != 0) {
        D_800C3614 = D_800C3614 - 1;
        return;
    }
    BattleLoadArchiveEntryFile();
    pBase = g_BattleStateWork;
    BattleSetCurrentActorSprite(pBase[g_BattleCommandRowIndex * 0x48 + 0x15B]);
    func_800BF3E8((BattleActorSprite*)g_BattleState->pCurSprite);
    index = g_BattleState->actorIndex;
    offset = 0x8C8C;
    pSprite = *(BattleActorSprite**)(pBase + index * 4 + offset);
    code = pBase[g_BattleCommandRowIndex * 0x48 + 0x17F];
    BattleSetStateStep(8);
    if (g_BattleState->commandUnhandled != 0) {
        pBase[0xA73] = 0;
    }
    g_BattleState->commandUnhandled = 0;
    switch (code) {
        case 0xFF:
            BattleSetStateStep(9);
            break;
        case 0xFE:
            BattleWaitArchiveSync();
            pRow = g_BattleStateWork;
            pRow += g_BattleCommandRowIndex * 0x48;
            func_800BEE2C(g_BattleState->initialActorIndex, *(u16 *)(pRow + 0x14E), 4);
            BattleSetStateStep(0xA);
            break;
        case 0xFC:
            pRow = g_BattleStateWork;
            pRow += g_BattleCommandRowIndex * 0x48;
            BattleSetActorPrimaryCommand(index, *(u16 *)(pRow + 0x172));
            BattleSetStateStep(9);
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
        case 0xFD:
            pRow = g_BattleStateWork;
            pRow += g_BattleCommandRowIndex * 0x48;
            func_800BEE2C(index, *(u16 *)(pRow + 0x14E), 2);
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
        case 0xF3:
        case 0xF5:
        case 0xF6:
        case 0xF7:
        case 0xFA:
            func_800B9284((SpriteData*)pSprite, code);
            BattleSetStateStep(9);
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
        case 0xFB:
            BattleSetStateStep(9);
            pRow = g_BattleStateWork;
            pRow += g_BattleCommandRowIndex * 0x48;
            BattleSwapActorSlots(((pSprite->flags_0xAC.flags & 3) << 2) | (pSprite->flags_0xA8 >> 30),
                                 *(u16 *)(pRow + 0x172));
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
        case 0xF4:
            BattleSetStateStep(9);
            pRow = g_BattleStateWork;
            pRow += g_BattleCommandRowIndex * 0x48;
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            id = *(u16 *)(pRow + 0x172);
            g_BattleActorAnimationIndex = (id >> 9) & 0x3F;
            D_800D39E4 = id & 0x1FF;
            break;
        default:
            g_BattleState->commandUnhandled = 1;
            BattleSetSpriteTemplate(pSprite);
            pRow = g_BattleStateWork;
            pRow += g_BattleCommandRowIndex * 0x48;
            func_800BEE2C(index, *(u16 *)(pRow + 0x14E), code);
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9F78);
void BattleLoadActorSlotAnimation(s32 index) {
    SpriteData* pSprite;
    u8* pBase;
    s32 slotOffset;

    g_NumFlaggedTimerWorkListEntries = 0;
    g_FlagTimerWorkListAdds = 0;
    BattleWaitAndFreeActorBuffers();
    pBase = g_BattleStateWork;
    slotOffset = 0x8C8C;
    if (((BattleActorSlot*)pBase)[index].inUse != 0) {
        BattleUnloadActorArchive();
        BattleLoadSoundWdsFile();
    } else if (index < 3) {
        pSprite = *(SpriteData**)(pBase + index * 4 + slotOffset);
        if (pSprite != NULL) {
            BattleLoadActorArchiveFile(pSprite);
        }
    } else {
        BattleUnloadActorArchive();
    }
    BattleSetAnimTaskShort(0xC0);
}
void BattleActorSetStepFromAngle(BattleActorSprite* pSprite, s16 angle) {
    s32 scale = pSprite->moveSpeed >> 3;

    pSprite->direction = angle;
    pSprite->step.x = ((rsin(angle) >> 1) * scale) >> 8;
    pSprite->step.z = -(((rcos(pSprite->direction) >> 1) * scale)) >> 8;
}
void BattleActorComputeFallDirection(BattleActorSprite* pSprite) {
    VECTOR diff;
    SVECTOR vec;
    u8 out[0x10];
    s32 index;
    s32 len;
    s32 dy;
    s32 angle;

    SpriteSetSVec(&vec, pSprite->targetPosition.x, pSprite->targetPosition.y, pSprite->targetPosition.z);
    index = BattleFindGroundTriangle((s16*)&vec, pSprite->field_0x78, 4);
    if (index < 0) {
        index = BattleFindTriangleContainingPoint((s16*)&vec);
    }
    BattleProjectPointOntoIndexedTriangle((s16*)&vec, index, out);
    if (vec.vy > pSprite->targetPosition.y) {
        vec.vy = pSprite->targetPosition.y;
    }
    dy = ((vec.vy << 16) - pSprite->position.value.y) >> 16;
    diff.vx = pSprite->targetPosition.x - pSprite->position.parts.xWhole;
    diff.vz = pSprite->targetPosition.z - pSprite->position.parts.zWhole;
    angle = -ratan2(diff.vz, diff.vx);
    Square0(&diff, &diff);
    len = SquareRoot0(diff.vx + diff.vz);
    pSprite->step.y = ((-(s32)pSprite->gravity * len) << 4) / (pSprite->moveSpeed >> 11) +
                      (pSprite->moveSpeed * dy) / len;
    BattleActorSetStepFromAngle(pSprite, angle);
}
void BattleActorUpdateFallMotion(BattleActorSprite* pSprite) {
    VECTOR diff;
    SVECTOR vec;
    u8 out[0x10];
    s32 index;
    s32 divisor;
    s32 angle;
    s32 len;
    s32 num;

    divisor = -((pSprite->step.y << 1) / (s32)pSprite->gravity);
    pSprite->position.value.x &= 0xFFFF0000;
    pSprite->position.value.y &= 0xFFFF0000;
    pSprite->position.value.z &= 0xFFFF0000;
    diff.vx = pSprite->targetPosition.x - pSprite->position.parts.xWhole;
    diff.vz = pSprite->targetPosition.z - pSprite->position.parts.zWhole;
    diff.vy = 0;
    angle = -ratan2(diff.vz, diff.vx);
    Square0(&diff, &diff);
    len = SquareRoot0(diff.vx + diff.vz);
    if (divisor != 0) {
        pSprite->moveSpeed = (len << 16) / divisor;
    } else {
        pSprite->moveSpeed = 0;
    }
    SpriteSetSVec(&vec, pSprite->targetPosition.x, pSprite->targetPosition.y, pSprite->targetPosition.z);
    index = BattleFindGroundTriangle((s16*)&vec, pSprite->field_0x78, 4);
    if (index < 0) {
        index = BattleFindTriangleContainingPoint((s16*)&vec);
    }
    BattleProjectPointOntoIndexedTriangle((s16*)&vec, index, out);
    if (vec.vy > pSprite->targetPosition.y) {
        vec.vy = pSprite->targetPosition.y;
    }
    num = (vec.vy << 16) - pSprite->position.value.y;
    if (divisor != 0) {
        pSprite->step.y += num / divisor;
    }
    BattleActorSetStepFromAngle(pSprite, angle);
    func_80022B2C(pSprite);
}
void BattleActorUpdateGroundHeightAndTriangle(BattleActorSprite* pSprite) {
    s16 vec[3];
    u8 out[0x10];
    s32 coord;
    s32 index;

    coord = pSprite->position.parts.xWhole;
    vec[0] = coord;
    coord = pSprite->position.parts.yWhole;
    vec[1] = coord;
    coord = pSprite->position.parts.zWhole;
    vec[2] = coord;
    index = BattleFindGroundTriangle(vec, pSprite->field_0x78, 4);
    if (index < 0) {
        index = BattleFindTriangleContainingPoint(vec);
    }
    BattleProjectPointOntoIndexedTriangle(vec, index, out);
    pSprite->field_0x84 = vec[1];
    pSprite->field_0x78 = index;
}
// The two unused slots keep the caller's argument area at retail's offsets.
AnimTask* BattleCreateAnimTask(void* pAnimFile, s16 clutX, s16 clutY, s16 texX, s16 texY, s16 unk5, s16 posX,
                        s16 posY, s16 posZ, s16 animId, u16 direction, s32 unk84, s32 unk88, s32 clut) {
    AnimTask* pTask;
    BattleActorSprite* pSprite;

    pTask = WorkListsAddTasks(sizeof(AnimTask), NULL, BattleActorTickAnimScript,
                              BattleActorDrawProjected, BattleFreeActorAnimTask);
    pSprite = (BattleActorSprite*)&pTask->spriteData;
    pTask->task1.pSpriteData = pSprite;
    pTask->task2.pSpriteData = pSprite;
    pTask->task2.unk0 = NULL;
    SpriteInitFromAnimFileWithClut(pSprite, pAnimFile, clutX, clutY, texX, texY, unk5, clut);
    pSprite->field_0x6C = pTask;
    pSprite->position.value.x = posX << 16;
    pSprite->position.value.y = posY << 16;
    pSprite->position.value.z = posZ << 16;
    pSprite->animId = animId;
    pSprite->flags |= 4;
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

void BattleActorDrawProjected(AnimTask* pTask) {
    BattleActorSprite* pSprite;
    SVECTOR vec;
    s32 coord;
    s32 otz;
    RotTransPersOut out;

    if (D_800C3664 != 0) {
        return;
    }
    pSprite = pTask->task1.pSpriteData;
    coord = pSprite->position.parts.xWhole;
    vec.vx = coord;
    coord = pSprite->position.parts.yWhole;
    vec.vy = coord;
    coord = pSprite->position.parts.zWhole;
    vec.vz = coord;
    SetRotMatrix(&g_BattleViewMatrix);
    SetTransMatrix(&g_BattleViewMatrix);
    otz = (RotTransPers(&vec, &out.sxy, (u_long*)&out.sxy, &out.flag) >> g_GfxOTDepthShift) +
          pSprite->field_0x30;
    if (out.flag & 0x8000) {
        otz = 0;
    }
    pSprite->field_0x2e = otz;
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

void BattleActorTickAnimScript(AnimTask* pTask) {
    BattleActorSprite* pSprite = pTask->task1.pSpriteData;

    if (D_800C3664 == 0) {
        AnimScriptTick(pSprite);
        SpriteApplyMovementStep(pSprite);
        if ((pSprite->flags_0xAC.flags >> 6) & 1) {
            AnimScriptTick(pSprite);
            SpriteApplyMovementStep(pSprite);
        }
    }
}
void BattleSpriteProjectToScreen(s32 index, s16* p0, s16* p1, s16* p2, s16* p3, s16* p4, s16* p5) {
    u8* pBase = g_BattleStateWork;
    s32 vx;
    s32 vy;
    s32 vz;
    BattleActorSprite* pSprite;
    SVECTOR vec;
    u16 sxy[2];
    s32 flag;

    pSprite = *(BattleActorSprite**)(pBase + index * 4 + 0x8C8C);
    vx = pSprite->position.parts.xWhole;
    vec.vx = vx;
    vy = pSprite->position.parts.yWhole;
    vec.vy = vy;
    vz = pSprite->position.parts.zWhole;
    vec.vz = vz;
    PushMatrix();
    SetRotMatrix(&g_BattleViewMatrix);
    SetTransMatrix(&g_BattleViewMatrix);
    *p2 = RotTransPers(&vec, (long*)sxy, &flag, &flag) >> 4;
    *p0 = sxy[0];
    *p1 = sxy[1];
    *p3 = sxy[0] - 0x30;
    *p4 = 0x30;
    *p5 = sxy[0] - 0x18;
    PopMatrix();
}
void BattleReleaseActorSlot(s32 index) {
    u8* pBase = g_BattleStateWork;
    u8* pBlock;
    u8* pTail;
    BattleSlotResource* pResource;
    void** ppBuffer;
    void* pBuffer;
    u32* pSlot;
    u32* pSlot2;

    pSlot = (u32*)(pBase + index * 4 + 0x8CB8);
    pResource = *(BattleSlotResource**)pSlot;
    if (pResource == NULL) {
        return;
    }
    BattleFindAnimationTask(pResource, 0, 2);
    if (index < 3) {
        pBlock = pBase + index * 0xC + 0x8000;
        ppBuffer = (void**)(pBlock + 0xD24);
        pBuffer = *ppBuffer;
        if (pBuffer != NULL) {
            HeapFree(pBuffer);
        }
        *ppBuffer = NULL;
    }
    pResource->pOnFree(pResource);
    WorkListsRemoveEntriesForData(pResource);
    pTail = g_BattleStateWork;
    pSlot = (u32*)(pTail + index * 4 + 0x8C8C);
    *pSlot = 0;
    pSlot2 = (u32*)(pTail + index * 4 + 0x8CB8);
    *pSlot2 = 0;
}
/* 92.04%: the 0x8C8C constant-fold wall, second shape.  With pBase a local the
   tail's instruction stream is retail's exactly (`li v0,0x8C8C; addu; sw 0` and
   the same for 0x8CB8), but gcc then keeps pBase live in $s3 across the calls
   and saves it, where retail re-forms lui/addiu for g_BattleStateWork and reuses $s1
   for the if-block pointer.  Spelling the tail address with the global
   (`g_BattleStateWork + index*4 + 0x8C8C`) instead makes gcc fold the constant into
   the symbol (lui 0x1; addiu -29556; sw 0x2C) and reach 94.02%, and routing it
   through a cast `(mem_addr)` pointer gets the frame right but splits the add
   into li 0x8000 + displacement (92.75%).  Everything else, frame and the
   0x8000/D24 if-block included, is exact.
/*
void BattleReleaseActorSlot(s32 index) {
    u8* pBase = g_BattleStateWork;
    SpriteData* pSprite;
    u8* pSlot;
    void* pPayload;

    pSprite = *(SpriteData**)(pBase + index * 4 + 0x8CB8);
    if (pSprite == NULL) {
        return;
    }
    BattleFindAnimationTask(pSprite, 0, 2);
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
    u8* pBase = g_BattleStateWork;
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
    sfx = (((BattleActorSlot*)pCounter)->sideFlag != 0) << 11;
    func_800223B0(pSprite, sfx);
    SpriteSetDirection(pSprite, sfx);
}
/* 75.68%: the known 0x8C8C wall (same as misc28's BattleScriptPlaySpriteAnimation).  Retail keeps
   &g_BattleStateWork in $a1 and adds 0x8C8C with a separate li+addu; gcc always folds
   the constant into the symbolic address as lui at,0x1 / lw -29556(at).
   Tried: raw (u8*)g_BattleStateWork, base pointer local, base + separate offset local,
   gcc 2.6.0, 2.7.2-cdk (cc1 rejects), -O3.  The second half (index*0x1C + 0xA
   access, the lb 0xAF compare and both calls) matches exactly under the base
   pointer form, so only this one address is wrong.
void BattleActorSetFacingFromSideFlag(s32 index) {
    SpriteData* pSprite;
    s32 sfx;

    pSprite = *(SpriteData**)((u8*)g_BattleStateWork + index * 4 + 0x8C8C);
    if (((s8*)pSprite)[0xAF] == 0x15) {
        return;
    }
    sfx = (g_BattleStateWork[index * 0x1C + 0xA] != 0) << 11;
    func_800223B0(pSprite, sfx);
    SpriteSetDirection(pSprite, sfx);
}
*/
void BattleNoOpCallback(void) {
}
extern s32 g_NumTimerWorkListEntries;
extern s32 g_BattlePendingActorTaskCount;
extern void func_800BE790(void);
extern void BattleQueueActorSlotTask(s32 index);

void BattleFinishActorAnimationAndReleaseSlot(s32 index) {
    s32 count;
    s32 off;
    mem_addr base;
    BattleActorSprite* pSprite;
    u8* pEntry;

    BattleBeginAnimTask(1 << index);
    BattleBeginAnimTask(0);
    count = g_NumTimerWorkListEntries;
    base = (mem_addr)g_BattleStateWork;
    pEntry = (u8*)(base + (index << 2));
    off = 0x8C8C;
    pSprite = *(BattleActorSprite**)(pEntry + off);
    BattleUnloadActorArchive();
    SpritePlayAnimation(pSprite, 0x16);
    while (pSprite->animScriptWaitTimer != 0) {
        if (pSprite->flags_0xAC.bytes.animId != 0x16) {
            break;
        }
        func_800BE790();
    }
    while (g_NumTimerWorkListEntries != count) {
        func_800BE790();
    }
    BattleReleaseActorSlot(index);
    func_800BE790();
    func_800BE790();
    BattleQueueActorSlotTask(index);
    while (g_BattlePendingActorTaskCount != 0) {
        func_800BE790();
    }
}
/* 75.98%, 188 bytes = retail's exact size, 47 instructions both sides, and every
   branch target matches.  The C is a do-while whose two store addresses are
   recomputed from pEntry (no pSlot local): that is what makes gcc emit ONE base
   for both stores instead of the park's separate li 0x8CB8 + addu.
   Left over, three loop.c hoist/split decisions, none of them a shape error:
     1. retail hoists only 0x8C8C (li s4 in the preheader) and adds it in the
        body (addu s2,s0,s4); ours splits 0x8CB8 as 0x8000 + 0xCB8 so both
        stores address off a hoisted 0x8000 (sw zero,3212(s0) / 3256(s0)) --
        fewer body insns, one more saved register, frame 0x30 against 0x28.
     2. retail re-materialises the trip count (li v0,3 before the bne); ours
        hoists it (li s4,3).
     3. registers shifted accordingly (retail pEntry s0, ours s2).
   Tried this session, all under the unit's cdk -G0 preset: pSlot local + inline
   0x8CB8 store (63.36%, 51 insns), mem_addr( ) cast on the store address (no
   change), do-while with both addresses recomputed (75.98%, kept), pSlot local
   at top of body + mem_addr store (72.74%, 51 insns), a for loop with an off
   local (63.36%).  Preset is not the answer: gcc-2.7.2-psx -G0 scores this body
   76.91 but breaks the unit's matched functions (BattleIncrementStateTimer
   99.09, BattleApplyStateStep 87.22, BattleActorSnapToTargetAndFace 85.40), and
   gcc-2.6.0-psx -G0 does not compile the unit.
/*
void func_800BB080(s32 arg0) {
    s32 i;
    u8* pEntry;
    u8* pSlot;
    SpriteData* pSprite;
    void* pTask;

    i = 0;
    pEntry = g_BattleStateWork;
    do {
        pSlot = pEntry + 0x8C8C;
        if (i != arg0) {
            pSprite = *(SpriteData**)pSlot;
            if (pSprite != NULL && pSprite->pDefaultAnimFile == NULL) {
                pTask = pSprite->field_0x6C;
                (*(void (**)(void*))((u8*)pTask + 0xC))(pTask);
                *(u32*)pSlot = 0;
                *(u32*)((mem_addr)pEntry + 0x8CB8) = 0;
            }
        }
        i++;
        pEntry += 4;
    } while (i != 3);
    BattleWaitForOtherActorAnimations(arg0);
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BB080);


void BattleActorSyncPositionFromModel(AnimTask* pTask) {
    BattleActorSprite* pSprite;
    BattleSlotRecord* pRecord;
    s32 mode;
    s32 index;
    u32 unused[2]; // unused: reproduces retail's 0x28 frame

    pSprite = pTask->task1.pSpriteData;
    mode = pSprite->flags_0xA8 >> 30;
    index = ((pSprite->flags_0xAC.flags & 3) << 2) | mode;
    pRecord = g_BattleActors[index];
    if (pRecord == NULL) {
        return;
    }
    if (pSprite->animScriptWaitTimer == 0) {
        pSprite->pSpriteBytecode = NULL;
    }
    if (pSprite->pSpriteBytecode != NULL) {
        AnimScriptTick(pSprite);
        SpriteApplyMovementStep(pSprite);
        if ((pSprite->flags_0xAC.flags >> 6) & 1) {
            AnimScriptTick(pSprite);
            SpriteApplyMovementStep(pSprite);
        }
    }
    pSprite->position.value.x = pRecord->pModelPosition->x << 16;
    pSprite->position.value.y = pRecord->pModelPosition->y << 16;
    pSprite->position.value.z = pRecord->pModelPosition->z << 16;
}

