#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/animation.h"

// The screen radius the slot projection keeps the group inside: under it the frame uses
// the projection's own distance, over it the distance is scaled by radius / this.
#define BATTLE_ANIM_RADIUS_MAX 120

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
extern u8 D_800C3688;

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
extern u32 g_BattleGteDepthCueValue;
extern s32 g_BattleAnimStateId;
extern u8 g_BattleActiveAnimTaskCount;
extern s32 g_BattleAnimStateFlags;
extern BattleSpriteColorTask *g_BattleSpriteColorTask;
// The anim tasks the two owner groups run; AnimTask (system/animation.h) is the
// record: entry, the entry the owner list links, then the sprite it animates.
extern AnimTask *g_BattleAnimOwnerTaskGroupA;
extern AnimTask *g_BattleAnimOwnerTaskGroupB;
extern u16 g_BattleAnimSavedGroupStates[];
extern u16 g_BattleAnimSavedGroupStatesB[];
extern s16 g_BattleAnimGroupStates[];
extern u16 g_BattleAnimGroupStatesB[];
extern void WorkListsRemoveEntriesForData(void *pTask);
extern void AnimScriptTick(SpriteData *pSprite);
extern void SpriteApplyMovementStep(SpriteData *pSprite);
extern void WorkListTaskSetOnFreeCallback(void *pEntry, void *pCallback);
extern u16 D_800C3CDC;
extern u16 D_80059454;
extern IVEC3 g_BattleAnimPositionGroupA;
extern IVEC3 g_BattleAnimPositionGroupB;
extern void func_800BC460(u32 actorMask);
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
extern SVECTOR g_BattleSlotVectorsB;
extern SVECTOR D_800C3730;
extern SVECTOR g_BattleDrawScaleX;
extern u8 g_BattleSlotDrawSuppressed;
extern u8 g_BattleStateWork[];

extern IVEC3 *g_BattleAnimSweepPos;
extern s32 g_BattleAnimSweepRadius;
extern s16 g_BattleAnimSweepAngle;

// The battle animation frame: the two group reference vectors the slot vectors are
// cued towards (g_BattleAnimGroupStates is that pair), the rotation built from their
// difference and the distance between them.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ SVECTOR groupA;
    /* 0x0C */ SVECTOR groupB;
    /* 0x14 */ SVEC rotation;
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ s32 distance;
} BattleAnimFrame; // Size: 0x20

extern BattleAnimFrame g_BattleAnimFrame;

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
    /* 0x00 */ u8 unk0[0x7];
    /* 0x07 */ u8 unk7; // nonzero: the slot is skipped by the frame fit
    /* 0x08 */ u8 unk8; // nonzero: the slot's height is projected as well
    /* 0x09 */ u8 unk9[0x5];
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
// R396 re-measured 98.10 at 496 bytes: the whole difference is one register - retail
// holds the flags_0xA8 word in $a0 and the slot<<30 temp in $v0, this body the other
// way round, and the sunk store follows from it ($v0 is needed again by the 0xAC
// block's srl). Also measured: the shift in a local computed above the pVramData
// store (96.49, and the mask's ori moves).
// R419 re-measured 98.10 at 496 bytes and measured three more spellings, all no better:
// a `u32 slotBits = slot << 30;` statement above the flags store (96.49) and the same
// with the flags word also in a local (96.49) - the shift's pseudo is created before the
// load's in both, yet the load still takes $v0 and the shift $a0, so creation order is
// not what local-alloc is sorting on here; `(slot << 30) | (flags & 0x3FFFFFFF)` 97.78.
// The mirror is what the fourth spelling below cannot reach: retail's order is
// shift->$v0, mask->$v1, load->$a0, ours load->$v0, mask->$v1, shift->$a0.
// R434 re-measured 98.10 at 496 bytes: the whole residual is the mirror this note names.
// One new spelling: the flipped operand order (slot << 30) | (flags & 0x3FFFFFFF) is
// 97.78, and it shows why the register splits rotate - the 0x3FFFFFFF mask's pseudo
// moves too (lui a0 instead of lui v1), so the very source order that gives the shift
// $v0 hands the mask $a0. Per levers a-flag-word-rmw-mask-can-never-take-v0,
// a-bitfield-store-is-the-same-rtl and a-single-use-constant-local-is-folded this is
// the flag-word RMW allocator family: the masked word and the shift result tie at
// priority 1.0 and the earlier-created pseudo (the load, here) takes $v0, so retail's
// mirror needs the shift's pseudo created before the load's, which no spelling that
// keeps the load first reaches.
// R447 re-measured 98.10 at 496 bytes and added two more: the in-place chain on a NAMED
// local (flags = pSprite->flags_0xA8; flags &= 0x3FFFFFFF; flags |= slot << 30; store) is
// BYTE-IDENTICAL to the one-expression body, so a-in-place-chain-on-a-named-local does
// not reach this one either; a do { } while (0) around only the flags_0xA8 store splits
// the block and costs an instruction (87.65, 500 bytes). Retail's `sll v0,s5,0x1e` sits
// in the $v0 whose address temp (s7+0x8CB8) dies two instructions earlier, so both bodies
// hand that one dying register to one of the two pseudos and the RTL creation order
// (load, then mask, then shift) decides which - retail's needs shift, mask, word, i.e.
// the mask forced into a register BEFORE the word load, which expand_binop's andsi3 can
// never do (its operand is the memory ref), so park here.
// R460 re-measured 98.10 at 496 bytes and added three results. (1) The in-place-chain
// lever's precondition (the load as its OWN statement, placed where retail's `lw a0,0xA8`
// sits, right after the 0x8CB8 store) is byte-identical to the one-expression body, so
// pinning the word's register first does not help here. (2) Creating the shift before the
// load (`slotBits = slot << 30;` above the two NULL stores, then `flags = slotBits |
// (flags_0xA8 & 0x3FFFFFFF)`) reaches 97.78 with a THREE-way rotation: mask $a0, word $v0,
// shift $v1 - the mask follows whatever the other two pick, so only the word-versus-shift
// pair decides and the mask can never be pinned independently. (3) Cross-check:
// system/temp1 func_80023B84 is a MATCHED function with the same one-expression statement
// and the same cdk compiler, and retail there holds the mask in $v0, the word in $a1 and
// the shift in $a0 - i.e. the mask keeps the low register only when its live range is long
// (created well above the load). So the residual is which of word/shift local-alloc sees
// first after sched1, not the source order, and every spelling that gets the pair right
// moves the mask. Park.
// R473 re-measured 98.10% at 496 bytes / 124 insns (un-parked, whole first output read) and
// named the MECHANISM behind the second diff row: the sunk `sw a0,0xA8(s0)` is NOT a separate
// residual. Ours reuses $v0 for `srl v0,s5,0x2`, which is the register the OR just wrote, so
// the WAW forces the store before that `srl`; retail's `srl` overwrites the shift's own dying
// $v0 and nothing pins the store, so sched2 sinks it past the 0xAC block's `lw`/`andi`. Fix the
// word/shift pair and the store position follows for free - do not spend a run trying to move
// the store alone (which is what the earlier `volatile` spelling did, in the wrong direction).
// With the priorities tied (word 3 refs / 3 insns, shift 2 refs / 2 insns, quotient 1.0 both)
// only a strictly greater shift priority or a strictly smaller word priority reorders them, and
// the one lever for the latter - lengthening the word's live range past the two NULL stores and
// the sb - is what every measured spelling already contains. Park again; closed.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB350);
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
}*/

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
    g_BattleGteDepthCueValue = 0x200;
    g_BattleAnimStateId = -1;
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
// Re-measured this round: the frame, the byte count and the instruction multiset are
// retail's; the FIRST difference is the work base, which retail computes into $v1 with
// the 0x8000 in $v0 (this body swaps them), and the load placement follows from the
// scheduler. Address-of locals for the two SVECTOR arguments (so the ads run before the
// loads) measure the identical 84.11. R396: the -dg dump shows why - insn 78/79 (the
// address of g_BattleStateWork) is a reg/v USER variable that takes $v0 and insn 82's
// 32768 takes $v1, so local-alloc hands primIndex the dead base register; retail has the
// constant in $v0 and the base in $v1 instead, which only the inline-load form reaches
// (and there gcc's right-to-left argument expansion loads 0xC84 before 0xC54, 83.44).
// R419: re-measured 84.11 at retail's exact 228 bytes with pWork as a u8* (the
// mem_addr form above scores the same). The whole residual is a two-register mirror:
// retail holds the work base in v1 and the 0x8000 constant in v0, this body the other
// way round, so primIndex takes v1 in both. The const pseudo has to be allocated FIRST
// for that, which an addsi3 constant split cannot do: `offset = 0x8000; pWork += offset`
// folds back to the immediate, and `0x8000 + base` folds the whole add into the symbol's
// addend (220 bytes).
// R434 re-measured 84.11% at retail's exact 228 bytes / 57 insns: the whole diff is the tail
// block, and it has three parts that are really one decision. (1) The base pair: retail
// has lui/addiu g_BattleStateWork into $v1 and li 0x8000 into $v0, this body the mirror
// - lever a-symbol-plus-a-big-immediate was measured on THIS function in round 396 and
// its -dg shows insn 78/79 is the reg/v USER address pseudo, which always takes the
// lower free register, so retail's pair is unreachable while the base is a source
// variable. (2) The base computation sits above the two argument moves (move a0,s0 /
// move a1,zero) here and below them in retail. (3) Retail's two loads straddle the two
// sw s2/s1 stack stores; ours are adjacent - sched1/sched2 placement that follows (1).
// One new spelling: the base as a block-scope declaration-with-initialiser
// (`u8 *pSlotWork = g_BattleStateWork; pSlotWork += 0x8000;`) compiles to the
// IDENTICAL body (84.11, 228 bytes), so a-declaration-with-initialiser-in-the-loop
// does not reach a base-vs-constant tie either.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB9D4);
/*
void func_800BB9D4(void) {
    s32 *pViewTrans;
    u8 *pWork;
    s32 depth;
    s32 primIndex;

    BattleBuildMatrixFromThreeVectors(&g_BattleViewMatrix, &g_BattleSlotVectors, &g_BattleSlotVectorsB,
                                      &D_800C3730);
    pViewTrans = (s32 *)((u8 *)&g_BattleViewMatrix - 0x20);
    pViewTrans[0xD] += g_BattleDrawScaleX.vx;
    pViewTrans[0xE] += g_BattleDrawScaleX.vy;
    pViewTrans[0xF] += g_BattleDrawScaleX.vz;
    if (g_BattleSlotDrawSuppressed == 0) {
        pWork = g_BattleStateWork;
        pWork += 0x8000;
        depth = *(s32 *)(pWork + 0xC54);
        primIndex = *(s32 *)(pWork + 0xC84);
        func_800A4654(&g_BattleViewMatrix, NULL, NULL, depth, primIndex, &g_BattleSlotVectors,
                      &g_BattleSlotVectorsB, 0x1000);
    }
}*/

// The animation-task tick's state half: the switch fills the two group positions for
// the current state, the depth-cued block nudges the slot vectors along the gap, and the
// tail derives the frame's rotation and distance from that gap. Three things here are
// load-bearing rather than stylistic: `case 0: break;` because that case count is what
// makes cc1 emit retail's beq/slti chain instead of a pivot-2 tree, the IVEC3 word reads
// because gcc's store-high-part peephole turns `sh (sra (lw p) 16)` into retail's
// `lh 2/6/A`, and `gap[0] | gap[2] | gap[1]` because retail's OR tree tests
// the horizontal pair before y.
//
// Stack layout: retail's frame puts the case-3 SVECTOR pair AND the depth-cue vector in
// the same slot at 0x10, which is what block scoping both of them here does - this cc1
// shares a slot only between sibling blocks, and function-scope locals take the low
// slots first. The tail's VECTORs therefore sit in their own block behind two dead ones:
// retail's frame has 16-byte holes at 0x20, 0x30 and 0x50, and a declared-but-unused
// local of that size is the only thing that reproduces them.
void BattleUpdateAnimFrame(void) {
    IVEC3 *pSource;
    s16 *pStates;
    s32 horizLen;

    switch (g_BattleAnimState) {
    case 0:
        break;
    case 1:
        func_800BC460(g_BattleAnimStateId);
        break;
    case 2:
        g_BattleAnimGroupStates[0] = g_BattleAnimPositionGroupA.x >> 16;
        g_BattleAnimGroupStates[1] = g_BattleAnimPositionGroupA.y >> 16;
        g_BattleAnimGroupStates[2] = g_BattleAnimPositionGroupA.z >> 16;
        g_BattleAnimGroupStates[4] = g_BattleAnimPositionGroupB.x >> 16;
        pStates = g_BattleAnimGroupStates + 4;
        pStates[1] = g_BattleAnimPositionGroupB.y >> 16;
        pStates[2] = g_BattleAnimPositionGroupB.z >> 16;
        break;
    case 3:
        {
            SVECTOR vecA;
            SVECTOR vecB;

            pSource = g_BattleAnimSweepPos;
            vecA.vx = pSource->x >> 16;
            vecB.vx = vecA.vx;
            vecA.vy = pSource->y >> 16;
            vecA.vz = pSource->z >> 16;
            // the angle sweeps the second vector off the first along the y/z plane
            vecB.vz = vecA.vz - rsin(g_BattleAnimSweepAngle) * g_BattleAnimSweepRadius / 4096;
            vecB.vy = vecA.vy - rcos(g_BattleAnimSweepAngle) * g_BattleAnimSweepRadius / 4096;
            g_BattleAnimFrame.groupA = vecB;
            g_BattleAnimFrame.groupB = vecA;
        }
        break;
    }

    if (g_BattleAnimStateFlags == 1) {
        s32 gap[3];

        // the depth cue value scales the gap, so a small gap does not move the slot
        gte_lddp(g_BattleGteDepthCueValue);
        gap[0] = g_BattleAnimFrame.groupA.vx - g_BattleSlotVectors.vx;
        gap[1] = g_BattleAnimFrame.groupA.vy - g_BattleSlotVectors.vy;
        gap[2] = g_BattleAnimFrame.groupA.vz - g_BattleSlotVectors.vz;
        gte_ldlvl(gap);
        gte_gpf12();
        gte_stlvl(gap);
        if ((gap[0] | gap[2] | gap[1]) != 0) {
            g_BattleSlotVectors.vx += gap[0];
            g_BattleSlotVectors.vy += gap[1];
            g_BattleSlotVectors.vz += gap[2];
        } else {
            // the group pointer is what keeps retail's second and third field reads on
            // the subobject address rather than on the frame's base register
            SVECTOR *pGroupA;

            pGroupA = &g_BattleAnimFrame.groupA;
            g_BattleSlotVectors.vx = pGroupA->vx;
            g_BattleSlotVectors.vy = pGroupA->vy;
            g_BattleSlotVectors.vz = pGroupA->vz;
        }
        gap[0] = g_BattleAnimFrame.groupB.vx - g_BattleSlotVectorsB.vx;
        gap[1] = g_BattleAnimFrame.groupB.vy - g_BattleSlotVectorsB.vy;
        gap[2] = g_BattleAnimFrame.groupB.vz - g_BattleSlotVectorsB.vz;
        gte_ldlvl(gap);
        gte_gpf12();
        gte_stlvl(gap);
        if ((gap[0] | gap[2] | gap[1]) != 0) {
            g_BattleSlotVectorsB.vx += gap[0];
            g_BattleSlotVectorsB.vy += gap[1];
            g_BattleSlotVectorsB.vz += gap[2];
        } else {
            SVECTOR *pGroupB;

            pGroupB = &g_BattleAnimFrame.groupB;
            g_BattleSlotVectorsB.vx = pGroupB->vx;
            g_BattleSlotVectorsB.vy = pGroupB->vy;
            g_BattleSlotVectorsB.vz = pGroupB->vz;
        }
    }

    {
        // the three dead VECTORs are retail's frame holes (see above)
        VECTOR spareA;
        VECTOR spareB;
        VECTOR delta;
        VECTOR spareC;
        VECTOR square;

        // the frame's rotation and distance are built from the slot vectors' gap
        delta.vx = g_BattleSlotVectorsB.vx - g_BattleSlotVectors.vx;
        delta.vy = g_BattleSlotVectorsB.vy - g_BattleSlotVectors.vy;
        delta.vz = g_BattleSlotVectorsB.vz - g_BattleSlotVectors.vz;
        Square0(&delta, &square);
        horizLen = SquareRoot0(square.vx + square.vz);
        g_BattleAnimFrame.distance = SquareRoot0(square.vx + square.vy + square.vz);
        g_BattleAnimFrame.rotation.y = -ratan2(delta.vz, delta.vx);
        g_BattleAnimFrame.rotation.x = -ratan2(delta.vy, horizLen);
        g_BattleAnimFrame.rotation.z = 0;
    }
}

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
            g_BattleAnimGroupStatesB[0] = g_BattleAnimSavedGroupStatesB[0];
            g_BattleAnimGroupStatesB[1] = g_BattleAnimSavedGroupStatesB[1];
            g_BattleAnimGroupStatesB[2] = g_BattleAnimSavedGroupStatesB[2];
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
            g_BattleAnimSavedGroupStatesB[0] = g_BattleAnimGroupStatesB[0];
            g_BattleAnimSavedGroupStatesB[1] = g_BattleAnimGroupStatesB[1];
            g_BattleAnimSavedGroupStatesB[2] = g_BattleAnimGroupStatesB[2];
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
// PARK 78.89% at 1664 bytes against retail's 1604 (416 vs 401 insns). FRAME AND EVERY SLOT
// NOW MATCH RETAIL EXACTLY: vars=232, args=16, regs=10/0, 0x120. Two residuals left, both
// named below; the old 78.90 note's "24 bytes of unaccounted locals" is SOLVED.
// THE SLOT RULE, measured with cc1_probe in this unit's compiler (do not re-derive):
// gcc 2.7.2-cdk allocates an AGGREGATE local's slot at its DECLARATION, in declaration
// order, but an address-taken SCALAR's slot lazily, at its first address-of, i.e. after
// every aggregate slot handed out so far. An array or struct is an aggregate even at 4 or
// 8 bytes: `long x[2]` declared between two aggregates lands mid-frame, `long x` lands at
// the end. So retail's 0x78/0x7C are NOT two scalars: they are one 8-byte aggregate holding
// RotTransPers's p and flag, declared between zeroVec and nearMatrix. With `long gteOut[2]`
// in that spot and sxy as an 8-byte SVECTOR (it must be 8: with 4 the whole run below it
// moves down by 4) the layout comes out on retail's offsets: total 0x10, groupA 0x20,
// groupB 0x28, frameMatrix 0x30, projected 0x50, vertex 0x60, sxy 0x68, zeroVec 0x70,
// gteOut 0x78/0x7C, nearMatrix 0x80, nearScreen 0xA0, nearVec 0xB0, farMatrix 0xB8,
// farScreen 0xD8, farVec 0xE8, then the two reload spill slots (0xF0 holds &vertex) at
// 0xF0/0xF4 and the save area at 0xF8. Do not go back to `long sxy` + `long p, flag`.
// RESIDUAL 1, 15 instructions: each RotTransPers arm is 4 insns long. This body emits
// `lhu 0x68; addiu -160; sll 16; sra 16` then `sll 18; sra 16; mult`, i.e. it sign-extends
// the s16 field after the -= 160 and then truncates the *4 product; retail has only the
// second conversion (`lhu; addiu; sll 18; sra 16; mult`). So retail's source never
// materialises the s16 intermediate: the low 16 bits of `x - 160` are all either use needs.
// Resume with the two assignments fused/ordered differently, e.g. keep the subtracted value
// in an s32 local and write the SVECTOR fields from it, or `sxy.vx = (sxy.vx - 160) * 4`
// spelled so both stores stay. Retail stores dx then dx*4 to 0x68, so both statements exist.
// RESIDUAL 2, the far branch's output slot: retail writes and reads that ApplyMatrix result
// at 0xA0 (the nearScreen slot) and leaves the 0xD8 slot farScreen dead; this body uses
// farScreen for it. Using nearScreen instead measured 78.83 / 420 differing lines versus
// 78.89 / 370 here, so re-test both once residual 1 is gone - it is one line each.
// MEASURED, do not re-derive: the three RotMatrixZYX calls take (SVECTOR *)&D_800C3740 -
// retail's relocation names D_800C3740, not g_BattleAnimSweepAngle (0x800C373C).
// D_800C3CDC needs the (s32 *) store cast (the file's extern is u16 for BattleBeginAnimTask).
// The 0x8C8C sprite-table reach: retail `ori t0,0,0x8C8C; addu v0,a1,t0; lw 0(v0)`, this
// body `li $7,0x8000; addu; lw 0xC8C(v0)` - same address, one insn cheaper, not steerable
// from the source (it is *(SpriteData **)(pSlotWork + 0x8C8C) either way).
// Preset is right: 2.6.0 scores 74.63, -O3 57.54, -G8 does not compile.
/*
void func_800BC460(u32 actorMask) {
    VECTOR total;
    SVECTOR groupA;
    SVECTOR groupB;
    MATRIX frameMatrix;
    VECTOR projected;
    SVECTOR vertex;
    SVECTOR sxy;
    SVECTOR zeroVec;
    long gteOut[2];
    MATRIX nearMatrix;
    VECTOR nearScreen;
    SVECTOR nearVec;
    MATRIX farMatrix;
    VECTOR farScreen;
    SVECTOR farVec;
    u8 *pWork;
    u8 *pSlotWork;
    BattleSlotWorkRow *pRow;
    SpriteData *pSprite;
    BattleActorSprite *pActorSprite;
    s16 *pGroupStates;
    s32 i;
    u16 bit;
    s32 spriteCount;
    s32 maxX;
    s32 minX;
    s32 maxY;
    s32 minY;
    s32 maxZ;
    s32 minZ;
    s32 distance;
    s32 radius;
    s32 scale;

    memset(&total, 0, sizeof(total));
    distance = 0;
    g_BattleAnimStateId = actorMask;
    spriteCount = 0;
    pWork = g_BattleStateWork;
    pRow = (BattleSlotWorkRow *)pWork;
    pSlotWork = pWork;
    for (i = 0, bit = actorMask; i != BATTLE_ACTOR_SLOT_COUNT; i++, bit >>= 1) {
        if ((bit & 1) != 0 && pRow->unk7 == 0) {
            pSprite = *(SpriteData **)(pSlotWork + 0x8C8C);
            if (pSprite != NULL) {
                spriteCount++;
                total.vx += pSprite->position.x >> 1;
                total.vy += pSprite->position.y >> 1;
                total.vz += pSprite->position.z >> 1;
            }
        }
        pSlotWork += 4;
        pRow++;
    }
    if (spriteCount == 0) {
        return;
    }
    total.vx = total.vx / spriteCount * 2;
    total.vz = total.vz / spriteCount * 2;
    total.vy = total.vy / spriteCount * 2;
    minX = total.vx;
    maxX = total.vx;
    minZ = total.vz;
    maxZ = total.vz;
    minY = total.vy;
    maxY = total.vy;
    pRow = (BattleSlotWorkRow *)pWork;
    pSlotWork = pWork;
    for (i = 0, bit = actorMask; i != BATTLE_ACTOR_SLOT_COUNT; i++, bit >>= 1) {
        if ((bit & 1) != 0 && pRow->unk7 == 0) {
            pSprite = *(SpriteData **)(pSlotWork + 0x8C8C);
            if (pSprite != NULL) {
                if (maxX < pSprite->position.x) {
                    maxX = pSprite->position.x;
                }
                if (pSprite->position.x < minX) {
                    minX = pSprite->position.x;
                }
                if (maxZ < pSprite->position.z) {
                    maxZ = pSprite->position.z;
                }
                if (pSprite->position.z < minZ) {
                    minZ = pSprite->position.z;
                }
                if (maxY < pSprite->position.y) {
                    maxY = pSprite->position.y;
                }
                if (pSprite->position.y < minY) {
                    minY = pSprite->position.y;
                }
            }
        }
        pSlotWork += 4;
        pRow++;
    }
    total.vx = (minX + maxX) / 2;
    total.vy = (minY + maxY) / 2;
    total.vz = (minZ + maxZ) / 2;
    total.vx = (minX + maxX) >> 17;
    total.vy = (minY + maxY) >> 17;
    total.vz = (minZ + maxZ) >> 17;
    RotMatrixZYX((SVECTOR *)&D_800C3740, &frameMatrix);
    zeroVec.vx = 0;
    zeroVec.vy = 0;
    zeroVec.vz = ReadGeomScreen() * 8;
    ApplyMatrix(&frameMatrix, &zeroVec, &projected);
    groupA.vx = (u16)total.vx;
    groupA.vy = (u16)total.vy;
    groupA.vz = (u16)total.vz;
    groupB.vx = (u16)total.vx;
    groupB.vy = (u16)total.vy;
    groupB.vz = (u16)total.vz;
    groupA.vx -= (u16)projected.vx;
    groupA.vy += (u16)projected.vy;
    groupA.vz -= (u16)projected.vz;
    BattleBuildMatrixFromThreeVectors(&frameMatrix, &groupA, &groupB, &D_800C3730);
    SetRotMatrix(&frameMatrix);
    SetTransMatrix(&frameMatrix);
    pRow = (BattleSlotWorkRow *)pWork;
    pSlotWork = pWork;
    for (i = 0, bit = actorMask; i != BATTLE_ACTOR_SLOT_COUNT; i++, bit >>= 1) {
        if ((bit & 1) != 0 && pRow->unk7 == 0) {
            pActorSprite = *(BattleActorSprite **)(pSlotWork + 0x8C8C);
            if (pActorSprite != NULL) {
                vertex.vx = pActorSprite->position.xWhole;
                vertex.vy = pActorSprite->position.yWhole;
                vertex.vz = pActorSprite->position.zWhole;
                RotTransPers(&vertex, (long *)&sxy, &gteOut[0], &gteOut[1]);
                sxy.vx -= 160;
                sxy.vy -= 164;
                sxy.vx *= 4;
                sxy.vy *= 4;
                radius = sxy.vx * sxy.vx + sxy.vy * sxy.vy;
                if (distance < radius) {
                    distance = radius;
                }
                if (pRow->unk8 != 0 && D_800C3688 == 0) {
                    vertex.vy = pActorSprite->position.yWhole - pActorSprite->height;
                    RotTransPers(&vertex, (long *)&sxy, &gteOut[0], &gteOut[1]);
                    sxy.vx -= 160;
                    sxy.vy -= 164;
                    sxy.vx *= 4;
                    sxy.vy *= 4;
                    radius = sxy.vx * sxy.vx + sxy.vy * sxy.vy;
                    if (distance < radius) {
                        distance = radius;
                    }
                }
            }
        }
        pSlotWork += 4;
        pRow++;
    }
    distance = SquareRoot0(distance);
    if (distance < BATTLE_ANIM_RADIUS_MAX) {
        RotMatrixZYX((SVECTOR *)&D_800C3740, &nearMatrix);
        nearVec.vx = 0;
        nearVec.vy = 0;
        nearVec.vz = ReadGeomScreen() * 2;
        *(s32 *)&D_800C3CDC = ReadGeomScreen() * 2;
        ApplyMatrix(&nearMatrix, &nearVec, (VECTOR *)&vertex);
        groupA.vx = (u16)total.vx - (u16)((VECTOR *)&vertex)->vx;
        groupA.vy = (u16)total.vy + (u16)((VECTOR *)&vertex)->vy;
        groupA.vz = (u16)total.vz - (u16)((VECTOR *)&vertex)->vz;
    } else {
        scale = distance * 0x4000 / BATTLE_ANIM_RADIUS_MAX;
        scale = scale * 2 * ReadGeomScreen() / 0x4000;
        *(s32 *)&D_800C3CDC = scale;
        RotMatrixZYX((SVECTOR *)&D_800C3740, &farMatrix);
        farVec.vx = 0;
        farVec.vy = 0;
        farVec.vz = scale;
        ApplyMatrix(&farMatrix, &farVec, &farScreen);
        groupA.vx = (u16)total.vx - (u16)farScreen.vx;
        groupA.vy = (u16)total.vy + (u16)farScreen.vy;
        groupA.vz = (u16)total.vz - (u16)farScreen.vz;
    }
    groupB.vx = (u16)total.vx;
    groupB.vy = (u16)total.vy;
    groupB.vz = (u16)total.vz;
    pGroupStates = g_BattleAnimGroupStates;
    pGroupStates[0] = groupA.vx;
    g_BattleAnimGroupStates[1] = groupA.vy;
    g_BattleAnimGroupStates[2] = groupA.vz;
    pGroupStates = g_BattleAnimGroupStates + 4;
    pGroupStates[0] = groupB.vx;
    pGroupStates[1] = groupB.vy;
    pGroupStates[2] = groupB.vz;
}

*/
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
