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
    /* 0x38 */ u8 pad38[0xC];
    /* 0x44 */ s32 targetDistance; // measured by the state loop; a step acts only while it grows
    /* 0x48 */ u8 pendingAction; // 0x905C request: the frame loop runs it once when this is set
    /* 0x49 */ u8 pendingStep; // step the state loop dispatches once, then clears
    /* 0x4A */ u8 commandUnhandled; // set by the command switch's default arm; the next dispatch clears it
    /* 0x4B */ u8 pad4B;
    /* 0x4C */ struct BattleActorSprite* pTargetSprite; // the actor the moving steps drive
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
    /* 0x40 */ u8 unk40[0x8];
    /* 0x48 */ s32 unk48; // only tested against 0, to advance to step 7 when a command starts
    /* 0x4C */ u8 unk4C[0x4];
    /* 0x50 */ u32 pSedsFile; // the SEDS bank BattleLoadSoundSeds hands this sprite
    /* 0x54 */ u8 unk54[0x10];
    /* 0x64 */ void* pSpriteBytecode;
    /* 0x68 */ void* pCallback; // the slot SpriteSetCallback stores into
    /* 0x6C */ void* field_0x6C;
    /* 0x70 */ u8 unk70[0x4];
    /* 0x74 */ struct BattleActorSprite* pTargetEntitySprite;
    /* 0x78 */ u32 field_0x78;
    /* 0x7C */ SpriteDataI3* pAnimState; // the sprite's animation state record
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
    /* 0x00 */ u8 unk0[0x6];
    /* 0x06 */ u8 mode; // actor slot mode: indexes the per-mode animation frame tables
    /* 0x07 */ u8 flags; // tested against 0 only; while set, the 0xFF state arm replays no animation
    /* 0x08 */ u8 inUse; // non-zero while the slot is claimed; every reader here tests it against 0
    /* 0x09 */ u8 unk9;
    /* 0x0A */ u8 sideFlag; // non-zero faces the sprite 0x800 (half a turn) instead of 0
    /* 0x0B */ u8 unkB[0x3];
    /* 0x0E */ u16 x; // whole x, compared with the actor's own
    /* 0x10 */ u16 z; // whole z, compared with the actor's own
    /* 0x12 */ u8 unk12[0xA];
} BattleActorSlot; // Size: 0x1C


// The 0x48-byte per-command working rows of g_BattleStateWork: the command code
// decides which state arm the frame loop runs and the parameter word carries the
// slot, animation or flag that arm works with.
typedef struct {
    /* 0x00 */ u16 unk0; // anim task id a command hands BattleBeginAnimTask
    /* 0x02 */ u8 slotAnimIds[0x10]; // one animation id per actor slot, played when a command starts
    /* 0x12 */ u8 unk12[0x12];
    /* 0x24 */ u16 param; // the command's parameter word
    /* 0x26 */ u8 unk26[0xB];
    /* 0x31 */ u8 command; // the command code the frame loop dispatches on
    /* 0x32 */ u8 unk32[0x16];
} BattleCommandRow; // Size: 0x48

// The head of g_BattleStateWork: state, then the command rows.
typedef struct {
    /* 0x000 */ u8 unk000[0x14E];
    /* 0x14E */ BattleCommandRow rows[0x10]; // one row per actor slot
} BattleWorkRows;

#define BATTLE_COMMAND_ROW(i) (&((BattleWorkRows*)g_BattleStateWork)->rows[i])

// A command row reached from its own base inside g_BattleStateWork. The 0x14E pad
// is what keeps the row's fields in the load displacement (`lhu 0x172(base + i * 0x48)`
// after `addiu base,base,0`): a written-out `g_BattleStateWork + i * 0x48 + 0x14E`
// (or an address-of this member) makes cc1 fold the offset into the symbol's %lo
// (`addiu base,base,334` / `lhu 0`), and only a member lvalue holds the address
// shape every arm has.
typedef struct {
    /* 0x000 */ u8 unk000[0x14E];
    /* 0x14E */ BattleCommandRow row;
} BattleCommandRowBase;

#define BATTLE_COMMAND_ROW_AT(i) (((BattleCommandRowBase*)(g_BattleStateWork + (i) * 0x48))->row)

// The texture coordinate pair a sprite animation state carries at +0xE; retail
// copies both halves with one unaligned word move.
typedef struct {
    /* 0x0 */ s16 texX;
    /* 0x2 */ s16 texY;
} SpriteTexCoordPair; // Size: 0x4

// One pointer of the per-slot table at g_BattleStateWork + 0x8CB8: an object
// whose +0xC entry retail calls with the object itself while the slot is being
// released, right before the work lists drop the same pointer.
typedef struct BattleSlotResource {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ void (*pOnFree)(struct BattleSlotResource* pSelf);
} BattleSlotResource; // Size: 0x10

// The two per-slot pointer tables at the tail of g_BattleStateWork: the slot
// sprites at 0x8C8C and the slot resources at 0x8CB8, eleven entries each.
typedef struct {
    /* 0x0000 */ u8 unk0[0x8C8C];
    /* 0x8C8C */ SpriteData* slotSprites[BATTLE_ACTOR_SLOT_COUNT];
    /* 0x8CB8 */ BattleSlotResource* slotResources[BATTLE_ACTOR_SLOT_COUNT];
} BattleSlotTables;

extern BattleSlotRecord* g_BattleActors[];

// Retail's call sites hand this the sprite; the callback body ignores it.
void BattleApplyStateStep();
extern s32 func_800C0314(void);
extern void SpriteSetFieldB0(SpriteData* pSprite, u16 arg1);
extern void SpriteSetSpecialAnimFile(SpriteData* pSprite, void* pAnimFile);
extern s32 SpriteAnimPackageTestFlag8000(void* pFrames);
extern s32 BattleLoadSoundSeds(void);
extern void BattleSetActorTargetFromPathPosition(SpriteData* pSprite);
extern u8 g_BattleSedsLoaded;
extern void* g_BattleActorArchiveBuffer;
extern s16 D_800C3626;
extern s16 D_800C3630[]; // per actor slot mode: the first frame of its animation range
extern s16 D_800C3648[]; // and one past its last frame

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
extern SpriteData* BattleSetCurrentActorSprite(s32 actorIndex);
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
/* 97.48%, 508 bytes = retail's exact size, 127 instructions both sides.  Two
   finds this round, both driven by the source:
   1. the six u16 copies at the end are a POINTER WALK from element 4, the same
      idiom misc31's matched code uses (`pStates = g_BattleAnimGroupStates + 4;`
      then `pStates[0..2]`): that is retail's `addiu v0,v0,0x8` before the 0/2/4
      loads.  Spelling the three destinations as
      `g_BattleAnimGroupStates[4]/[5]/[6]` costs that instruction (96.61%, 512
      bytes, one too long).
   2. `less = x < pSlot->x;` hoisted ABOVE the `if (x != pSlot->x || z !=
      pSlot->z)` and used as the arm's test puts retail's `slt` in the `bne`
      delay slot and lands retail's exact 508 bytes.
   Left over, register names only, instruction for instruction:
   - retail keeps x in $a2 for the +-0x50 arithmetic and a COPY in $a1 for the
     compares (`move a1,a2`), and the slt result in $v0, so the slot-z load
     clobbers it and retail REFILLS the beq delay slot with a second `slt`; ours
     keeps the result in $a2 and emits a nop there;
   - in each of the three key blocks retail's `or` DESTINATION is the 0xAC word
     register (`or v1,v1,v0` / `or v0,v0,v1`) where ours is the 0xA8 word one,
     i.e. an in-place or over an AC-left expression -- that spelling puts the AC
     load first and measures 89.73%;
   - the x/0x50 block still has retail's layout (bne to the non-equal body, beq
     to the equal one) and the z store needs an s32 local (an s16 store value
     makes gcc use lhu, retail lh).
   Tried this round: reading the field twice (`less = ...xWhole < pSlot->x;` then
   `x = ...xWhole;`) to force retail's copy -- 94.28% / 516 bytes; AC-left keys at
   all three sites 89.73%; the mask through two key locals in retail's emission
   order 91.32%.  Earlier: s16 vs s32 for x and z, `&&` vs `||`, nested if vs
   else-if, one expression vs index locals, and a 4-minute permuter run
   (base=1388, best=1188) whose only usable find was the z hoist kept below.
   Round 452 re-measured 97.48 / 508 and closed three more axes: gcc-2.6.0 is
   76.91, so the preset question is shut for this function too; a `s32 slotX =
   pSlot->x` local used by both compares folds back byte-identically; and a
   second mirror local for x, narrow or wide, is deleted by copy propagation, so
   retail's `move a1,a2` needs the value in two pseudos and one load cannot give
   that.
   Round 465 re-measured 97.48 / 508 bytes / 127 insns and closed one more
   spelling: the target key hoisted into its own `s32 targetKey` local (retail's
   emission order, the sprite side left inline) measures 92.71%, so the fully
   inline nesting this body uses is the right shape.  A `-dl` dump of the live
   body shows the mask and x/slot-x temporaries are all block-local quantities of
   2 refs / 2 insns with one death each (`Register 95..98, 101, 105`), i.e. the
   mirrored pairs have no term of the priority quotient to separate them, so the
   a2-versus-a1/a0 hand-out of x and `less` is not a spelling - park on it.
/*
void func_800B905C(void) {
    BattleActorSprite* pSprite;
    BattleActorSprite* pTarget;
    BattleActorSlot* pSlot;
    u8* pBase;
    s32 index;
    s32 mask;
    s32 x;
    s32 z;
    u16* pStates;
    s32 less;

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
        less = x < pSlot->x;
        if (x != pSlot->x || pTarget->position.parts.zWhole != pSlot->z) {
            if (less) {
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
        pStates = g_BattleAnimGroupStates + 4;
        g_BattleSlotVectorsB[0] = pStates[0];
        g_BattleSlotVectorsB[1] = pStates[1];
        g_BattleSlotVectorsB[2] = pStates[2];
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
   arm and its `lhu` of the global precedes the slot `lhu`.
   Round 439 re-measured 96.61% (644 bytes, retail's size) and closed the preset
   axis with `try_flags.sh --sweep`: cdk -G4/-G8 (all -O levels) 89.09, 2.7.2-psx
   -G0 76.38, so -G0 cdk is right.  Why the pair cannot be spelled: local-alloc
   orders block-local quantities by deaths/refs and then shorter live_length
   first.  In this arm the base is 1 death / 3 refs (lui, addiu, the final addu)
   live over 8 insns and the slot index is 1 death / 3 refs live over 4, so the
   INDEX is processed first and takes $v1 in every body that emits these
   instructions -- retail's base-in-$v1 needs the base to outrank it, which needs
   either a 4th index use or a 2-ref base, and both change the instruction count.
   Round 452 re-measured 96.61 / 644 and closed two more axes: a `u16 id` local
   (the load is a lhu) is 95.99, so the s32 id stays; and the BATTLE_COMMAND_ROW
   form the neighbouring stubs use cannot say what this body says - retail reads
   the same +0x172 word as `lbu` in the 0xFA/0xF8 arms and as `lhu` in
   0xF7/0xF5/0xF4/0xF3, so the source went through a byte pointer (a u16 row
   field would make every one of those reads lhu).
   Round 465 re-measured 96.61 / 644 bytes / 161 insns and read the block-local
   `-dl` lines for an address-building arm: the base's `%hi` pseudo is `used 2
   times across 8 insns` and the row index `used 3 times across 4`, i.e. 1 death /
   2 refs against 1 death / 3 refs.  local-alloc's n_deaths/n_refs ranks the BASE
   first there while floor_log2(refs)*refs/live_length ranks the INDEX first, so
   the two readings of the priority disagree and neither makes the retail pairing
   provable from the source - the rank these eight instructions actually get is
   the one that measures the mirror, and that is the whole residual in all seven
   address-building arms (the 0xF3 arm's extra early `lui` is
   -fschedule-insns).
*/
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
/* 76.27%, 1568 bytes against retail's 1576, so TWO instructions short.  The head,
   the 13-entry dispatch and the 0xFE/F9/F3..FA arms are instruction-identical; the
   ROW TYPING is the lever that moved this body from 72.11 to 76.27: the command
   rows are 0x48-byte elements that start at +0x14E of g_BattleStateWork, so
   BATTLE_COMMAND_ROW(i) puts the +0x14E into the load DISPLACEMENT (retail's
   `lhu 0x172(base+idx*0x48)`) instead of folding it into the symbol
   (`addiu base,370` + `lhu 0`), and `((BattleActorSlot*)g_BattleStateWork)[i].x`
   gives retail's `lhu 0xE` where `g_BattleStateWork + i*0x1C + 0xE` folds.
   Residuals, in order of size:
   1. the 0xFF arm: retail keeps flags_0xA8 in $a1 across both tests and
      materialises g_BattleStateWork with `lui a0/addiu` for the slot-0x07 probe;
      ours reloads 0xAC after the tests and reaches the slot base through $s3.
   2. the default arm: retail loads pTarget (0x74(s2)) BEFORE D_800C3626 = 0 and
      keeps pSprite->pAnimState in $a0 across the slotAnimIds store; ours emits the
      two index computations in the other order and reads the slot mode through a
      different base (lbu -328(v0) against retail's lbu 6(base)).
   3. the 0xFD/0xFC/0xFB arms each re-materialise g_BattleStateWork into a
      caller-saved register (retail a1/v1/a0); with one pBase local gcc keeps it in
      $s3 for every arm, which is one instruction short per arm.
   Tried: u8* pBase assigned per arm (74.63%, keeps the base in $s3 and the arms
   lose retail's fresh lui/addiu); the struct indexed as a stride-0x180 array
   (76.41%, right displacements, wrong stride); pBase written in the arms instead
   of the global (69-74%); the 0xFB heap tail through one pSprite->pBase local
   (kept, it is what gives retail's single `lw a0,0x20(s2)` for both stores); and
   a SpriteTexCoordPair struct copy for the +0xE tex pair (kept, it is what gives
   retail's lwl/lwr/swl/swr instead of a misaligned lw/sw). */
/*
void func_800B9508(BattleActorSprite* pSprite) {
    BattleActorSprite* pTarget;
    SpriteData* pSlotSprite;
    SpriteDataI1* pSpriteBase;
    void* pBlock;
    s32 code;
    s32 index;
    s32 frames;
    s32 mode;

    if (*(s16*)&D_800C3614 != 0) {
        D_800C3614 = D_800C3614 - 1;
        return;
    }
    code = BATTLE_COMMAND_ROW(g_BattleCommandRowIndex)->command;
    switch (code) {
        case 0xF3:
        case 0xF4:
        case 0xF5:
        case 0xF6:
        case 0xF7:
        case 0xF8:
        case 0xFA:
            func_800B9284((SpriteData*)pSprite, code);
            g_BattleCommandRowIndex += 1;
            return;
        case 0xF9:
            SpritePlayAnimation(pSprite, 5);
            BattleBeginAnimTask(0);
            return;
        case 0xFF:
            SpriteSetCallback(pSprite, NULL);
            g_BattleState->pendingAction = 1;
            if (func_800C0314() == 0) {
                if (((pSprite->flags_0xA8 >> 28) & 3) != 0) {
                    return;
                }
                mode = ((pSprite->flags_0xAC.flags & 3) << 2) | (pSprite->flags_0xA8 >> 30);
                if (((BattleActorSlot*)g_BattleStateWork)[mode].flags != 0) {
                    return;
                }
                SpritePlayAnimation(pSprite, pSprite->animId);
                return;
            }
            // falls through to the 0xFE arm
        case 0xFE:
            g_BattleState->pendingAction = 0;
            pSprite = (BattleActorSprite*)BattleSetCurrentActorSprite(g_BattleState->initialActorIndex);
            func_800BF3E8(pSprite);
            SpriteSetCallback(pSprite, BattleApplyStateStep);
            BattleSetStateStep(5);
            if (pSprite->position.parts.xWhole ==
                    ((BattleActorSlot*)g_BattleStateWork)[g_BattleState->actorIndex].x &&
                pSprite->position.parts.zWhole ==
                    ((BattleActorSlot*)g_BattleStateWork)[g_BattleState->actorIndex].z) {
                BattleApplyStateStep(pSprite);
                return;
            }
            pSprite->targetPosition.x = ((BattleActorSlot*)g_BattleStateWork)[g_BattleState->actorIndex].x;
            pSprite->targetPosition.y = 0;
            pSprite->targetPosition.z = ((BattleActorSlot*)g_BattleStateWork)[g_BattleState->actorIndex].z;
            SpritePlayAnimation(pSprite, 4);
            return;
        case 0xFD:
            g_BattleState->pendingAction = 1;
            if (BATTLE_COMMAND_ROW(g_BattleCommandRowIndex)->param == 0) {
                g_BattleState->pendingAction = 0;
                BattleSetActorTargetFromPathPosition((SpriteData*)pSprite);
                SpriteSetCallback(pSprite, BattleApplyStateStep);
            }
            g_BattleCommandRowIndex += 1;
            return;
        case 0xFC:
            SpriteSetCallback(pSprite, NULL);
            g_BattleState->pendingAction = 1;
            SpriteSetFieldB0((SpriteData*)pSprite, BATTLE_COMMAND_ROW(g_BattleCommandRowIndex)->param);
            g_BattleCommandRowIndex += 1;
            return;
        case 0xFB:
            SpriteSetCallback(pSprite, NULL);
            g_BattleState->pendingAction = 1;
            index = BATTLE_COMMAND_ROW(g_BattleCommandRowIndex)->param;
            pSlotSprite = ((BattleSlotTables*)g_BattleStateWork)->slotSprites[index];
            pSprite->pVramData = pSlotSprite->pVramData;
            *(SpriteTexCoordPair*)&pSprite->pAnimState->texX =
                *(SpriteTexCoordPair*)&((SpriteDataI3*)pSlotSprite->field_0x7C)->texX;
            pSprite->flags |= 0x40000000;
            HeapFree(pSprite->pBase->pFramesData);
            pBlock = HeapAlloc(HeapGetNextBlockHeader(pSlotSprite->pBase->pFramesData), 0);
            pSpriteBase = pSprite->pBase;
            pSpriteBase->pFramesData = pBlock;
            pSpriteBase->pCurRenderFramesData = pBlock;
            g_BattleCommandRowIndex += 1;
            return;
        default:
            pTarget = pSprite->pTargetEntitySprite;
            D_800C3626 = 0;
            BattleSetSpriteTemplate(pSprite);
            SpriteSetCallback(pSprite, BattleApplyStateStep);
            index = ((pTarget->flags_0xAC.flags & 3) << 2) | (pTarget->flags_0xA8 >> 30);
            pSprite->pAnimState->field_0x8 = BATTLE_COMMAND_ROW(g_BattleCommandRowIndex)->slotAnimIds[index];
            if (SpriteAnimPackageTestFlag8000(((SpriteAnimPackage*)pSprite->pVramData)->pFrames) != 0) {
                SpritePlayAnimation(pSprite, code);
                index = ((pSprite->flags_0xAC.flags & 3) << 2) | (pSprite->flags_0xA8 >> 30);
                mode = ((pSprite->pTargetEntitySprite->flags_0xAC.flags & 3) << 2) |
                       (pSprite->pTargetEntitySprite->flags_0xA8 >> 30);
                func_800BC460((1 << index) | (1 << mode));
            } else if (code >= 0x10) {
                index = ((pSprite->flags_0xAC.flags & 3) << 2) | (pSprite->flags_0xA8 >> 30);
                mode = ((BattleActorSlot*)g_BattleStateWork)[index].mode;
                frames = (code - 0x10) + D_800C3630[mode];
                pSprite->flags_0xAC.bytes.animId = (frames < D_800C3648[mode]) ? 0x1C : 0x11;
                BattleActorWaitFramesWithCallback(frames, pSprite);
                BattleSetPendingCommand(pSprite);
            } else {
                if (g_BattleSedsLoaded == 0) {
                    if (g_BattleActorArchiveBuffer != NULL) {
                        BattleWaitArchiveSync();
                        pSprite->pSedsFile = BattleLoadSoundSeds();
                        SpriteSetSpecialAnimFile((SpriteData*)pSprite, g_BattleActorArchiveBuffer);
                    }
                }
                index = ((pSprite->flags_0xAC.flags & 3) << 2) | (pSprite->flags_0xA8 >> 30);
                if (g_BattleState->initialActorIndex == index) {
                    SpritePlayAnimation(pSprite, ~code);
                } else {
                    SpritePlayAnimation(pSprite, code);
                }
            }
            g_BattleState->pendingAction = 0;
            g_BattleCommandRowIndex += 1;
            return;
    }
}
*/

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
/* 98.93%, 768 bytes / 192 instructions -- retail's exact size, and the head,
   all thirteen dispatch arms and the frame match.  Three ties are left:
   1. s-register rotation: retail has pSprite=$s1, index=$s2, code=$s3, ours
      gives pSprite=$s1, code=$s2, index=$s3.  Declaration order is not it
      (declaring code before index measures the identical 98.93), so the pair is
      local-alloc's ordering.  Every statement order that creates `code` first
      also flips the head's load order (88.33%), which the note's author measured.
   2. gcc folds the row base into the symbol in the arms
      (%hi(g_BattleStateWork + 0x14E) with lhu 0x0) where retail materialises the
      plain symbol and reads lhu 0x14E; only a statement split stops the fold and
      both splits measure worse: a `u8 *pRow` local in the 0xFE arm alone 96.35
      (that arm's load schedule rotates, and the base then CSEs to $s4), the same
      local at all five sites 74.36 at 816 bytes (pRow becomes callee-saved).
   3. the default arm's caller-saved names (t0/a3 against retail's a3/v1) -- a
      consequence of the `g_BattleCommandRowIndex++` spelling retail needs for
      its store delay slot.
   The superseded body was missing `case 0xF8`: retail's jtbl_80070A80 entry 5 is
   .L800B9E24 (the func_800B9284 arm) and entry 6 is the default.  With that added
   the C emits 13 words against retail's 14-word `.size`; the trailing word looks
   like the `.align 3` pad of func_800B9F78's table, which starts 8-aligned at
   +0x90 -- not verified with check_bin.
   Kept, each measured: the one-expression row reads (retail's addu shape, +16%
   over the old pBase/pRow spelling); `rowIndex = g_BattleCommandRowIndex;
   g_BattleCommandRowIndex = rowIndex + 1;` in the 0xF4 arm (retail stores the
   increment before the row read and this is its exact 4-instruction chain,
   which the obvious `g_BattleCommandRowIndex++` in the expression does not
   give); and the 0xFB arm's `slot = pSprite->flags_0xA8 >> 30;` local, which a
   permuter run found (base 655 -> 403) and which fixed that arm's or-operand
   registers.
   Preset is not the answer here: the unit is already cdk -G0 and the matched
   neighbours break under 2.7.2-psx -G0.  Before this round the park read 76.54%
   with pBase/pRow arm spellings; that body is superseded.
*/
/* 99.51%, 768 bytes / 192 instructions -- retail's exact size, the frame and all
   thirteen dispatch arms instruction-identical.  One residual: the default arm's
   `g_BattleCommandRowIndex++` chain, where retail keeps the global's address in
   $a3, the loaded index in $v1 and the work base in $a1 (a1 doing double duty as
   the lhu destination once the base dies) and this body hands out $t0 / $a3 /
   $v1 for the same three quantities -- same instruction order, same ref counts,
   so it is local-alloc's block hand-out, not a shape error.
   What closed the old 98.93 body (all measured this round):
   1. the row reads.  Written as `g_BattleStateWork + row * 0x48 + FIELD` every
      arm folded the field into the symbol's %lo (`addiu base,base,334` /
      `lhu 0`) where retail keeps it in the load displacement
      (`addiu base,base,0` / `lhu 334(base)`).  BATTLE_COMMAND_ROW_AT -- a
      pad-then-row struct reached as a MEMBER LVALUE of a cast pointer -- keeps
      the displacement; the address-of spelling (&(...)->row) folds again, and
      BATTLE_COMMAND_ROW (the rows array of BattleWorkRows) folds because its
      0x14E is a sibling of the symbol.  Both probed with cc1_probe.sh.
   2. `index` also receives the head's 0x15B byte before being reassigned from
      g_BattleState->actorIndex, and `pCurSprite` names the 0xC field; that pair
      is a permuter find worth +0.47 (98.93 -> 99.40) -- it is what flips
      pSprite/index/code from $s1/$s3/$s2 to retail's $s1/$s2/$s3.
   3. the 0xFB arm materialises its own work-base local (`pWork`), +0.11.
   Tried on the residual, each identical or worse: a `u16 *` subscript for the
   same row word (identical), a per-arm base local in the default arm in place of
   pWork (identical), the row word hoisted into `id` with the increment as its
   own statement (87.63, 816 bytes -- the sw leaves the jal delay slot), and
   three 3-minute permuter runs (150 -> 105 -> 85 -> 85, no further finds).
   Preset is not the question: the unit is cdk -G0 and its matched neighbours
   break under 2.7.2-psx.  The switch's .rodata operand against retail's
   jtbl_80070A80 is expected in an owning unit.
*/
/*
void func_800B9C78(void) {
    BattleActorSprite* pSprite;
    u8* pBase;
    u8* pWork;
    u8* pRow;
    SpriteData* pCurSprite;
    s32 index;
    u8 code;
    s32 offset;
    s32 id;
    s32 rowIndex;
    s32 slot;

    if (*(s16 *)&D_800C3614 != 0) {
        D_800C3614 = D_800C3614 - 1;
        return;
    }
    BattleLoadArchiveEntryFile();
    pBase = g_BattleStateWork;
    index = pBase[g_BattleCommandRowIndex * 0x48 + 0x15B];
    BattleSetCurrentActorSprite(index);
    pCurSprite = g_BattleState->pCurSprite;
    func_800BF3E8((BattleActorSprite*)pCurSprite);
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
            func_800BEE2C(g_BattleState->initialActorIndex,
                          BATTLE_COMMAND_ROW_AT(g_BattleCommandRowIndex).unk0, 4);
            BattleSetStateStep(0xA);
            break;
        case 0xFC:
            BattleSetActorPrimaryCommand(index,
                                        BATTLE_COMMAND_ROW_AT(g_BattleCommandRowIndex).param);
            BattleSetStateStep(9);
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
        case 0xFD:
            func_800BEE2C(index,
                          BATTLE_COMMAND_ROW_AT(g_BattleCommandRowIndex++).unk0, 2);
            break;
        case 0xF3:
        case 0xF5:
        case 0xF6:
        case 0xF7:
        case 0xF8:
        case 0xFA:
            func_800B9284((SpriteData*)pSprite, code);
            BattleSetStateStep(9);
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
        case 0xFB:
            BattleSetStateStep(9);
            slot = pSprite->flags_0xA8 >> 30;
            pWork = g_BattleStateWork;
            BattleSwapActorSlots(((pSprite->flags_0xAC.flags & 3) << 2) | slot,
                                 ((BattleCommandRowBase*)(pWork + g_BattleCommandRowIndex * 0x48))->row.param);
            g_BattleCommandRowIndex = g_BattleCommandRowIndex + 1;
            break;
        case 0xF4:
            BattleSetStateStep(9);
            rowIndex = g_BattleCommandRowIndex;
            g_BattleCommandRowIndex = rowIndex + 1;
            id = BATTLE_COMMAND_ROW_AT(rowIndex).param;
            g_BattleActorAnimationIndex = (id >> 9) & 0x3F;
            D_800D39E4 = id & 0x1FF;
            break;
        default:
            g_BattleState->commandUnhandled = 1;
            BattleSetSpriteTemplate(pSprite);
            pRow = g_BattleStateWork;
            func_800BEE2C(index,
                          ((BattleCommandRowBase*)(pRow + g_BattleCommandRowIndex++ * 0x48))->row.unk0, code);
            break;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9C78);

extern s32 D_800C3660;
extern s32 g_BattlePendingCommand;
extern BattleActorSprite* g_BattleActorList;
extern u8 g_BattleActorSlotMode;
extern u8 g_BattleAnimScriptHalted;
extern s32 BattleStartStageActorAnimation(void);
extern s32 BattleComputeVectorDistance2D(DVECTOR vec0, DVECTOR vec1);
extern void BattleStepActorAlongPath(BattleActorSprite* pActor, BattleActorSprite* pTarget);
extern s32 BattleIsTimerTaskActive(void);
extern void BattleTearDownState(void);
extern void BattleReleaseAllActorSlots(void);
extern void BattleActorSetFacingFromSideFlag(s32 index);
extern void func_800BE790(void);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9F78);
/* 94.02%, 1368 bytes / 342 instructions against retail's 1384 / 346, with the
   head, the two dispatches and both distance arms otherwise exact.  The 4
   missing instructions are the frame: ours is 0x40 where retail's is 0x48, and
   the arm-6 distance measure keeps its second vector at 0x18(sp) (reusing arm
   2's dead slot) where retail uses 0x20(sp), so two more s32 locals were live
   in the original arm 6.  Also left, all measured:
   1. the pending-command key: retail ends `or v0,v0,v1` (the 0xAC word's
      register as destination) and `slti v0,v0,3`; ours parks the key in $a1
      (`or a1,v0,v1 / slti v0,a1,3`).  Same for the same key in the 4 and 5
      arms.  The signed `slti` needs the key in an s32 local, and the load
      order (0xA8 before 0xAC) needs the A8-left spelling -- both measured.
   2. `D_800C3623 = 0;` after SpritePlayAnimation re-forms `lui` in retail but
      is CSE'd into $s0 across the call in ours.
   3. the animation id choice: retail is `beqz`/`j` with `li 0x13` in the jump's
      delay slot and a separate `li 0x12` block (3 insns), ours fills the branch
      delay slot with `li 0x12` and drops the jump (2 insns).
   Case body order is retail's .text order, not ascending, and getting it right
   was worth 30 points: pendingStep 4,6,2,5 and stateStep 7,10,9,8,2,6,4.  The
   stateStep switch reads the step as a word while BattleApplyStateStep in this
   unit reads the same address as a byte, so it is spelled through a cast here.
   Tried: AC-left key (worse: swaps the two loads), ternary instead of the
   if/else `anim` local (same 2-insn shape), separate arm locals.
*/
/*
void func_800B9F78(BattleState* pState) {
    BattleActorSprite* pSprite;
    BattleActorSprite* pTarget;
    BattleActorSprite* pActor;
    DVECTOR vec0;
    DVECTOR vec1;
    s32 distance;
    s32 x;
    s32 z;
    s32 targetX;
    s32 targetZ;
    s32 key;
    s32 anim;

    if (D_800C3660 != 0) {
        return;
    }
    D_800C3660 = 1;
    pSprite = (BattleActorSprite*)pState->pCurSprite;
    g_BattleState = pState;
    pTarget = pState->pTargetSprite;
    if (g_BattlePendingCommand != 0) {
        g_BattlePendingCommand = 0;
        if ((BattleStartStageActorAnimation() & 0xFF) != 0) {
            SpriteSetCallback(pSprite, BattleApplyStateStep);
        } else {
            key = (pSprite->flags_0xA8 >> 30) | ((pSprite->flags_0xAC.flags & 3) << 2);
            if (key < 3) {
                if (D_800C3623 != 0) {
                    anim = 0x13;
                } else {
                    anim = 0x12;
                }
                SpritePlayAnimation(pSprite, anim);
                D_800C3623 = 0;
            }
            if (pSprite->unk48 != 0) {
                BattleSetStateStep(7);
            }
        }
    }
    if (g_BattleState->timer != 0) {
        func_800B9C78();
        --g_BattleState->timer;
    }
    if (g_BattleState->pendingStep != 0) {
        switch (g_BattleState->pendingStep) {
            case 4:
                pActor = g_BattleActorList;
                key = (pActor->flags_0xA8 >> 30) | ((pActor->flags_0xAC.flags & 3) << 2);
                if (key < 3 && ((BattleActorSlot*)g_BattleStateWork)[key].inUse == 0 &&
                    g_BattleStateWork[(g_BattleCommandRowIndex - 1) * 0x48 + 0x150 + key] == 7) {
                    BattleSetSpriteTemplate(pActor);
                    SpritePlayAnimation(pActor, 0x1B);
                    while (pActor->flags_0xAC.bytes.animId == 0x1B) {
                        func_800BE790();
                    }
                }
                break;
            case 6:
                BattleStepActorAlongPath(pSprite, pSprite->pTargetEntitySprite);
                break;
            case 2:
                BattleActorSnapToTargetAndFace(pSprite, pSprite->pTargetEntitySprite);
                break;
            case 5:
                key = (pSprite->flags_0xA8 >> 30) | ((pSprite->flags_0xAC.flags & 3) << 2);
                if (((BattleActorSlot*)g_BattleStateWork)[key].flags == 0) {
                    SpritePlayAnimation(pSprite, pSprite->animId);
                }
                BattleActorSetFacingFromSideFlag(g_BattleState->actorIndex);
                BattleSetStateStep(0xA);
                break;
        }
        g_BattleState->pendingStep = 0;
    }
    switch (*(s32*)&g_BattleState->stateStep) {
        case 7:
            if (g_NumFlaggedTimerWorkListEntries == BattleIsTimerTaskActive()) {
                BattleSetStateStep(4);
                g_BattleState->pendingAction = 1;
                BattleLoadArchiveEntryFile();
            }
            break;
        case 10:
            if (g_NumFlaggedTimerWorkListEntries == BattleIsTimerTaskActive()) {
                BattleTearDownState();
            }
            break;
        case 9:
            func_800B9C78();
            break;
        case 8:
            if (g_BattleActorSlotMode != 0 &&
                g_NumFlaggedTimerWorkListEntries == BattleIsTimerTaskActive()) {
                if (g_BattleAnimScriptHalted == 0) {
                    break;
                }
                if (g_BattleActorSlotMode == 1) {
                    ++g_BattleState->timer;
                }
                BattleReleaseAllActorSlots();
            }
            break;
        case 2:
            x = pSprite->position.parts.xWhole;
            vec0.vx = x;
            z = pSprite->position.parts.zWhole;
            vec0.vy = z;
            targetX = pSprite->targetPosition.x;
            vec1.vx = targetX;
            targetZ = pSprite->targetPosition.z;
            vec1.vy = targetZ;
            distance = BattleComputeVectorDistance2D(vec0, vec1);
            if (g_BattleState->targetDistance < distance) {
                BattleActorSnapToTargetAndFace(pSprite, pTarget);
            } else {
                g_BattleState->targetDistance = distance;
            }
            break;
        case 6:
            x = pSprite->position.parts.xWhole;
            vec0.vx = x;
            z = pSprite->position.parts.zWhole;
            vec0.vy = z;
            targetX = pSprite->targetPosition.x;
            vec1.vx = targetX;
            targetZ = pSprite->targetPosition.z;
            vec1.vy = targetZ;
            distance = BattleComputeVectorDistance2D(vec0, vec1);
            if (g_BattleState->targetDistance < distance) {
                BattleStepActorAlongPath(pSprite, pTarget);
            } else {
                g_BattleState->targetDistance = distance;
            }
            break;
        case 4:
            if (g_BattleState->pendingAction != 0) {
                func_800B905C();
            }
            break;
    }
    D_800C3660 = 0;
}
*/
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
   gcc-2.6.0-psx -G0 does not compile the unit. */
void BattleClearFinishedSlotSprites(s32 arg0) {
    BattleSlotTables* pWork;
    s32 i;
    SpriteData* pSprite;
    BattleSlotResource* pResource;

    i = 0;
    do {
        pWork = (BattleSlotTables*)g_BattleStateWork;
        if (i != arg0) {
            pSprite = pWork->slotSprites[i];
            if (pSprite != NULL && pSprite->pDefaultAnimFile == NULL) {
                pResource = pSprite->field_0x6C;
                pResource->pOnFree(pResource);
                pWork->slotSprites[i] = NULL;
                pWork->slotResources[i] = NULL;
            }
        }
        i++;
    } while (i != 3);
    BattleWaitForOtherActorAnimations(arg0);
}

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

