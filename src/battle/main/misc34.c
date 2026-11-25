#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgte.h"
#include "system/sound.h"

extern s32 g_NumFlaggedTimerWorkListEntries;
extern s16 g_BattleActorListCount;
extern s32 g_BattleActorList[];
extern s32 g_BattleActorModelIds[];
extern u16 g_BattleDelayCounter;
extern u16 g_BattleActorDelayCounter;
extern void *g_BattleState;
extern s32 g_BattlePendingCommand;
extern s32 g_BattleWaitFrameCounter;
extern s32 g_BattleTimerTask;
extern u8 g_FlagTimerWorkListAdds;
extern SpriteAnimPackage g_SpriteAnimPackageDirs5;

extern void func_800A9FF0(s32);
s32 BattleIsTimerTaskActive(void);
extern void BattleUpdateCommandRows(void);
extern void *BattleFindAnimationTask(SpriteData *pSprite, s32 arg1, s32 mode);
extern void *g_TimerWorkList;
extern s32 BattleSpriteDistanceToTarget(SpriteData *pSpriteData);
extern void BattleSetActorTargetFromPathPosition(BattleSpriteState *pActor);
extern void BattleActorSnapToTargetAndFace(SpriteData *pSprite, SpriteData *pTarget);
extern void BattleFaceTargetAndMeasureDistance(BattleSpriteState *pActor, s32 arg1);
extern void BattleSetStateStep(s32 arg0);
extern u8 *TimerWorkListAllocateTask(void *pList, s32 size);
extern SpriteData *func_80023B84(SpriteData *pActor, u8 *pEntry, SpriteAnimPackage *pPackage);
extern void TimerWorkListSetTaskCallback(void *pTask, void *pCallback);
extern u16 g_BattleSlotBitMask;
extern u16 g_BattleActorSlotDoneMask;
extern u16 g_BattleConditionAnimIds[];
extern s32 BattleGetActorConditionCode(s32 slot);
extern void func_800C0564(void);
extern s32 func_800C0314(void);
extern void BattleActorSetFacingFromSideFlag(s32 index);
extern u16 FieldScriptClearAnimationOverrideIfFlag16(s32 slot);

// An absolute difference, spelled with the subtraction duplicated so gcc folds the
// negate into a reversed subu rather than emitting negu on a named difference.
#define BATTLE_ABS_DIFF(x) ((x) < 0 ? -(x) : (x))

// A sprite's battle slot number: the top two bits of flags_0xA8 are its low bits and
// the two low bits of flags_0xAC.flags its high bits.
#define BATTLE_SLOT_INDEX(pSprite)                                            \
    (((u32)(pSprite)->flags_0xA8 >> 30) | (((pSprite)->flags_0xAC.flags & 3) << 2))

/* One 0x1C-byte slot of the battle actor table at g_BattleStateWork. */
typedef struct {
    /* 0x00 */ u8 unk0[0x6];
    /* 0x06 */ u8 actorId; // which actor the slot holds, BATTLE_SLOT_CHARACTER_NONE when empty (g_BattleSlotActorIds)
    /* 0x07 */ u8 flags; // only masked and tested: bit 7 by BattleActorSlotHasFlag80, 0 means idle
    /* 0x08 */ u8 inUse; // set to 1 when the slot is claimed; every reader tests it against 0
    /* 0x09 */ u8 unk9;
    /* 0x0A */ u8 sideFlag; // non-zero faces the slot's sprite 0x800 (half a turn), the angle BattleActorSetFacingFromSideFlag passes to SpriteSetDirection
    /* 0x0B */ u8 unkB[0x3];
    /* 0x0E */ u16 x; // whole x the slot's sprite takes its 16.16 position from (misc29 BattleActorInitSpriteFromSlot)
    /* 0x10 */ u16 z;
    /* 0x12 */ u16 y; // the slot's height, published with x and z as a position triple by func_800AEF68
    /* 0x14 */ u8 unk14[0x8];
} BattleActorSlot; // Size: 0x1C

extern BattleActorSlot g_BattleStateWork[];
extern SpriteData *D_800C3E1C;
extern u8 g_BattleActorSlotMode;
extern s32 g_BattlePendingActorTaskCount;
extern void *g_BattleWdsEntry;
extern u8 g_BattleSoundArchiveLoaded;
extern u8 g_BattleActorArchiveReady;
extern u16 g_BattleActorSlotMask;
extern void BattleFreeActorArchiveAndSoundSeds(void);
extern void BattleQueueActorSlotTask(s32 index);
extern void BattleFreeWdsEntry(void);
extern void *SoundLoadWdsFile(void *pBuffer, s32 arg1);
extern s16 SoundGetPendingSpuCommandType(s32 arg0);

// WorkListEntry of include/system/animation.h; an overlay unit cannot include that
// header (it holds tentative definitions of main-exe globals), so the layout is
// repeated. The distance task's block starts with one of these.
typedef struct WorkListEntry {
    /* 0x00 */ void *unk0;
    /* 0x04 */ void *unk4;
    /* 0x08 */ void *onTriggerCallback;
    /* 0x0C */ void (*onFreeCallback)(void *);
    /* 0x10 */ u32 unk10: 29; // address of the task this entry belongs to, without its tag bits
    /* 0x10 */ u32 unk10_1: 1;
    /* 0x10 */ u32 unk10_2: 1;
    /* 0x10 */ u32 unk10_3: 1;
    /* 0x14 */ u32 unk14: 29;
    /* 0x14 */ u32 unk14_1: 1;
    /* 0x14 */ u32 unk14_2: 1;
    /* 0x14 */ u32 unk14_3: 1;
    /* 0x18 */ struct WorkListEntry *pNext;
} WorkListEntry; // Size: 0x1C

// The block BattleCreateActorDistanceTask adds after its timer entry: the actor to
// watch, the animation it plays while waiting, the distance measured last frame,
// the range that ends the wait and the callback run when the range is left.
typedef struct {
    /* 0x00 */ WorkListEntry entry;
    /* 0x1C */ SpriteData *pSpriteData;
    /* 0x20 */ s32 animId;
    /* 0x24 */ s32 distance;
    /* 0x28 */ s32 minDistance;
    /* 0x2C */ void (*pOnDistanceOutOfRange)(SpriteData *);
} BattleDistanceTask; // Size: 0x30
STATIC_ASSERT_SIZEOF(BattleDistanceTask, 0x30);

// One 6-byte point of the target-position path the battle state walks, in whole
// coordinates; 0xFFFF in y ends the path.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 z;
} BattlePathPoint; // Size: 0x6

// The target-position path as it sits in the battle state work block. pathIndex in
// the battle state is the cursor into it.
typedef struct {
    /* 0x0000 */ u8 unk0000[0xA3C];
    /* 0x0A3C */ BattlePathPoint points[1];
} BattlePathTable;

// The battle state g_BattleState points at; only the field this unit reads.
typedef struct {
    /* 0x00 */ u8 unk00[0x2C];
    /* 0x2C */ s32 pathIndex;
} BattleStatePathCursor; // Size: 0x30

// Walks the actor one frame along the battle state's target-position path: while the
// path still has a point at the cursor the path step owns the actor, and once it ends
// the actor is put on its target position and turned toward the target.
void BattleStepActorAlongPath(BattleSpriteState *pActor, BattleSpriteState *pTarget) {
    BattlePathTable *pPath;
    BattleStatePathCursor *pState;
    s32 targetX;
    s32 targetZ;

    pState = (BattleStatePathCursor *)g_BattleState;
    pPath = (BattlePathTable *)g_BattleStateWork;
    if (pPath->points[pState->pathIndex].x == 0xFFFF &&
        pPath->points[pState->pathIndex].y == 0xFFFF) {
        *(s32 *)&pActor->position = pActor->targetPosition.x << 16;
        *(s32 *)&pActor->position.zFrac = pActor->targetPosition.z << 16;
        targetX = pTarget->position.xWhole;
        if (pActor->position.xWhole < targetX) {
            pActor->targetPosition.x = targetX - 0x50;
        } else {
            pActor->targetPosition.x = targetX + 0x50;
        }
        targetZ = pTarget->position.zWhole;
        pActor->targetPosition.z = targetZ;
        pActor->targetPosition.y = 0;
        if (pActor->targetPosition.x == pActor->position.xWhole) {
            if (pActor->targetPosition.z == pActor->position.zWhole) {
                BattleActorSnapToTargetAndFace((SpriteData *)pActor, (SpriteData *)pTarget);
                return;
            }
        }
        BattleFaceTargetAndMeasureDistance(pActor, 3);
        BattleSetStateStep(2);
    } else {
        BattleSetActorTargetFromPathPosition(pActor);
    }
}

void BattleIncFrameCounter(void) {
    g_BattleWaitFrameCounter++;
}

extern void BattleLoadStageActorArchive(s32);
extern void SpriteSetCallback(SpriteData *pSpriteData, void *pCallback);
extern void SpritePlayAnimation(SpriteData *pSpriteData, s32 animId);
extern void func_800BE790(void);

// The animation the actor is playing is byte 3 of its 0xAC flag word.
void BattleActorWaitFramesWithCallback(s32 arg0, SpriteData *pSpriteData) {
    if (pSpriteData->pDefaultAnimFile == NULL) {
        BattleLoadStageActorArchive(arg0);
        return;
    }
    g_BattleWaitFrameCounter = 0;
    if ((s8)pSpriteData->flags_0xAC.bytes[3] != 0) {
        SpriteSetCallback(pSpriteData, BattleIncFrameCounter);
        SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xAC.bytes[3]);
    }
    BattleLoadStageActorArchive(arg0);
    if ((s8)pSpriteData->flags_0xAC.bytes[3] != 0) {
        while (g_BattleWaitFrameCounter == 0) {
            func_800BE790();
        }
        SpriteSetCallback(pSpriteData, 0);
    }
}

void BattleUpdateActorSlotsIfStateExists(void) {
    if (g_BattleState != 0) {
        BattleUpdateCommandRows();
    }
}

s32 BattleGetAdjustedWaitCountdown(void) {
    return g_NumFlaggedTimerWorkListEntries - BattleIsTimerTaskActive();
}

s32 BattleIsTimerTaskActive(void) {
    return g_BattleTimerTask != 0;
}

void BattleSetPendingCommand(s32 arg0) {
    g_BattlePendingCommand = arg0;
}

void BattleUpdateDistanceTask(BattleDistanceTask *pTask) {
    BattleDistanceTask *pSelf;
    s32 prev;
    s32 dist;

    // Retail keeps this second name of the same block in its own register; using
    // pTask in the branch below as well drops the match to 91%.
    pSelf = pTask;
    prev = pTask->distance;
    dist = BattleSpriteDistanceToTarget(pTask->pSpriteData);
    pTask->distance = dist;
    if (prev < dist || dist < pTask->minDistance) {
        pSelf->pOnDistanceOutOfRange(pSelf->pSpriteData);
        pSelf->entry.onFreeCallback(pSelf);
    }
}

void BattleCreateActorDistanceTask(SpriteData *pSpriteData, s32 minDistance,
                                  void (*pOnDistanceOutOfRange)(SpriteData *)) {
    BattleDistanceTask *pTask;

    pTask = (BattleDistanceTask *)TimerWorkListAllocateTask(
        pSpriteData->field_0x6C, sizeof(BattleDistanceTask) - sizeof(WorkListEntry));
    TimerWorkListSetTaskCallback(pTask, BattleUpdateDistanceTask);
    pTask->pOnDistanceOutOfRange = pOnDistanceOutOfRange;
    pTask->pSpriteData = pSpriteData;
    pTask->animId = (s8)pSpriteData->flags_0xAC.bytes[3];
    pTask->distance = BattleSpriteDistanceToTarget(pSpriteData);
    pTask->minDistance = minDistance;
    pSpriteData->flags_0xAC.flags |= 0x20;
}
// A view of g_BattleStateWork that reaches the per-slot sprite pointer table at
// 0x8C8C: each entry is the sprite the spawner made for that slot.
typedef struct {
    /* 0x0000 */ u8 unk0000[0x8C8C];
    /* 0x8C8C */ SpriteData *sprites[BATTLE_ACTOR_SLOT_COUNT];
} BattleSpriteSlots;

// Makes the actor of slot arg0 the current battle sprite and makes the actor of slot
// arg1 its target, which is also the only entry the model id table keeps.
void BattleSetCurrentSpriteAndTarget(s32 arg0, s32 arg1) {
    BattleSpriteSlots *pSlots;
    SpriteData **pTargetSlot;
    SpriteData *pSprite;

    pSlots = (BattleSpriteSlots *)g_BattleStateWork;
    pSprite = pSlots->sprites[arg0];
    if (pSprite == NULL) {
        return;
    }
    // The byte arithmetic (rather than &pSlots->sprites[arg1]) is what makes gcc add
    // the base before 0x8C8C, as retail does; the struct form comes out 99.64%.
    pTargetSlot = (SpriteData **)((u8 *)pSlots + (arg1 * 4) + 0x8C8C);
    D_800C3E1C = pSprite;
    pSprite->pTargetEntitySprite = *pTargetSlot;
    g_BattleActorSlotMask = 1 << arg1;
    g_BattleActorList[0] = (s32)*pTargetSlot;
    g_BattleActorList[1] = 0;
}
void BattleRemapActorModelId(SpriteData *pActor) {
    s32 i = 0;
    s32 *pEntry;

    if (g_BattleActorListCount != 0) {
        pEntry = g_BattleActorList;
        for (i = 0; i != g_BattleActorListCount; i++) {
            if (pEntry[i] == (s32)pActor->pTargetEntitySprite) {
                break;
            }
        }
    }
    if (i >= g_BattleActorListCount) {
        pActor->pTargetEntitySprite = (void *)g_BattleActorList[0];
    } else {
        pActor->pTargetEntitySprite = (void *)g_BattleActorModelIds[i];
    }
}
s32 BattleFindTableEntryIndex(s32 arg0) {
    s16 count = g_BattleActorListCount;
    s32 i = 0;

    if (count != 0) {
        s32 *pEntry = g_BattleActorList;

        for (i = 0; i != count; i++) {
            if (pEntry[i] == arg0) {
                break;
            }
        }
    }
    return i;
}
void BattleTickCountersAndFreeUnitSlot(void) {
    s16 first = g_BattleDelayCounter + 1;
    s16 second = g_BattleActorDelayCounter + 1;

    g_BattleDelayCounter = first;
    g_BattleActorDelayCounter = second;
    if (first == 2) {
        func_800A9FF0(0xB);
    }
}
extern u8 g_BattleActorArchiveLoadPending;
extern void BattleWaitArchiveSync(void);
extern int ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void func_8002DDE4(void *p0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6);

void BattleLoadArchiveEntryFile(void) {
    void *pBuffer;

    if (g_BattleActorArchiveLoadPending != 0) {
        BattleWaitArchiveSync();
        ArchiveSetIndex(0x2C, 0);
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(1), 0);
        ArchiveReadFileToBuffer(1, pBuffer, 0, 0x80);
        BattleWaitArchiveSync();
        func_8002DDE4(pBuffer, 0, 0, 0, 0, 0, 0);
        func_800BE790();
        HeapFree(pBuffer);
        g_BattleActorArchiveLoadPending = 0;
    }
}
// Hands every live actor slot whose actorId is below 0x11 to the per-actor animation
// task, except the slot the current battle state owns (state mode 2). The 4-bit slot
// mode packs bits 30-31 of flags_0xA8 with bits 0-1 of flags_0xAC, so it is assembled
// from two pieces.
void BattleReleaseAllActorSlots(void) {
    s32 i;
    s32 mode;
    s32 high;
    s32 low;
    SpriteData *pSprite;

    if (g_BattleActorSlotMode == 0) {
        return;
    }
    BattleFreeActorArchiveAndSoundSeds();
    func_800BE790();
    func_800BE790();
    pSprite = D_800C3E1C;
    high = (u32)pSprite->flags_0xA8 >> 30;
    low = (pSprite->flags_0xAC.flags & 3) << 2;
    mode = low | high;
    for (i = 0; i != 3; i++) {
        if (g_BattleStateWork[i].inUse != 0) {
            if (g_BattleActorSlotMode != 2 || i != mode) {
                if (g_BattleStateWork[i].actorId < 0x11) {
                    BattleQueueActorSlotTask(i);
                }
            }
        }
    }
    while (g_BattlePendingActorTaskCount != 0) {
        func_800BE790();
    }
    g_BattleActorSlotMode = 0;
}

// Loads the battle's sound archive file (directory 0x2C, entry 5) and hands it to
// the WDS player, unless the WDS table already has its id.
void BattleLoadSoundWdsFile(void) {
    SoundWDSEntry *pWds;

    if (g_BattleSoundArchiveLoaded != 0) {
        return;
    }
    BattleWaitArchiveSync();
    ArchiveSetIndex(0x2C, 0);
    pWds = HeapAlloc(ArchiveDecodeAlignedSize(5), 0);
    ArchiveReadFileToBuffer(5, pWds, 0, 0x80);
    BattleWaitArchiveSync();
    if (SoundFindWdsEntry(pWds->id) == NULL) {
        BattleFreeWdsEntry();
        g_BattleWdsEntry = SoundLoadWdsFile(pWds, 0);
        while (SoundGetPendingSpuCommandType(0) != 0) {
            func_800BE790();
        }
        g_BattleSoundArchiveLoaded = 1;
        g_BattleActorArchiveReady = 0;
    }
    HeapFree(pWds);
}

// Finds the timer task of the animation the sprite at pSprite->field_0x6C plays from
// the fifth animation package. mode 0 only reports it (returns the sprite the task
// draws, or NULL), mode 1 also runs the task's callback when the animation id matches
// and mode 2 runs it whatever the animation id is.
void *BattleFindAnimationTask(SpriteData *pSprite, s32 arg1, s32 mode) {
    WorkListEntry *pEntry;
    WorkListEntry *pOwner;
    WorkListEntry *pTask;
    SpriteData *pTaskSprite;

    pOwner = (WorkListEntry *)pSprite->field_0x6C;
    for (pEntry = g_TimerWorkList; pEntry != NULL; pEntry = pEntry->pNext) {
        if (pEntry->unk0 != pOwner) {
            continue;
        }
        if (pEntry->unk14 != pOwner->unk10 || (pEntry->unk14_1 & 1) == 0) {
            continue;
        }
        pTaskSprite = (SpriteData *)pEntry->unk4;
        if (pTaskSprite->pVramData != &g_SpriteAnimPackageDirs5) {
            continue;
        }
        if (mode != 2) {
            if ((s8)pTaskSprite->flags_0xAC.bytes[3] != arg1) {
                continue;
            }
            if (mode == 0) {
                return pTaskSprite;
            }
        }
        pTask = (WorkListEntry *)pTaskSprite->field_0x6C;
        pTask->onFreeCallback(pTask);
    }
    return NULL;
}

void BattleInvokeSpriteCallbackForActor(void *arg0, s32 arg1) {
    BattleFindAnimationTask(arg0, arg1, 1);
}

// Builds the animation task of slot arg1 from the fifth battle animation package and
// makes the new sprite the actor's target for it.
void BattleCreateSlotAnimationTask(SpriteData *pActor, s32 arg1) {
    SpriteAnimPackage *pPackage;
    u16 *pAnims;
    s32 offset;
    u8 *pEntry;
    SpriteData *pNew;
    u8 saved;
    u32 flags;

    if (pActor->pDefaultAnimFile == NULL) {
        return;
    }
    if (BattleFindAnimationTask(pActor, arg1, 0) != 0) {
        return;
    }
    pPackage = &g_SpriteAnimPackageDirs5;
    pAnims = pPackage->pAnimations;
    // The sum has to be spelled offset-first as integers: the pointer form
    // ((u8 *)pAnims + offset) makes gcc add the base first, which is 99.75%.
    offset = pAnims[arg1 + 1];
    pEntry = (u8 *)(offset + (s32)pAnims);
    saved = g_FlagTimerWorkListAdds;
    g_FlagTimerWorkListAdds = 0;
    pNew = func_80023B84(pActor, pEntry, pPackage);
    flags = pNew->flags_0xB0.flags;
    pNew->flags_0xAC.bytes[3] = arg1;
    pNew->pTargetEntitySprite = pActor;
    g_FlagTimerWorkListAdds = saved;
    pNew->flags_0xB0.flags = flags | 0x100;
}
// Clears the slot bit mask of every idle actor whose animation is one of the
// condition animations, re-seats the ones standing away from their slot, and then
// per live slot plays the condition's animation and starts/stops the extra
// animations the animation-override word's bits 13-15 select.
// 85.96% (1248 bytes vs retail 1228; 312 insns vs 307) under the unit preset
// XenoBattleTailCdk0. REDO-LANE rewrite from the assembly: structure is right (four slot loops with
// the same three jump tables and the same call order) and the old body's phase-4 continues that
// skipped the bit loops are gone - the animation-override bit loops now run for every live slot as
// retail does, which is what took it from 74.18 to 85.96. LOAD-BEARING spellings: the phase-1 switch
// index must be (s8) - a bare u8 index drops the sll 24/sra 24 pair retail has (measured with
// cc1_probe); BATTLE_ABS_DIFF (the duplicated subtraction) is what produces retail's reversed subu in
// the phase-2 distance test; the done-mask test must be ORed into the anim != 0x15 test, and the
// bit-13/14/15 switches need case labels 5..12 (8 empty arms) or gcc emits a beq tree instead of the
// 11-word tables retail has (both measured with cc1_probe).
// RESIDUALS: (a) in all three sprite-table walks gcc folds 0x8C8C into 0x8000 + lw 0xC8C(v0) where
// retail materialises the whole constant (ori 0x8C8C; addu; lw 0(v0)) - tried the base as a u8*
// local, as (u8*)g_BattleStateWork, and a s32 offset local (the last was worse), none forces it;
// (b) phase 2: gcc picks the 4-byte sprite pointer as the base and reads the slot as base+16 with
// lbu -8 / lhu -2 / lhu 0, where retail reads lbu 8 / lhu 14 / lhu 16 off its own slot pointer - tried
// initialising the sprite pointer from the slot pointer and from the symbol, both the same; the
// pPosition local that keeps pSprite in s1 also costs the extra move a1 and one j;
// (c) phase 4: gcc rematerialises g_BattleStateWork inside the loop instead of keeping it in fp like
// retail, and derives the slot pointer as base+7; (d) prologue/register rotation: pSlot and the
// sprite pointer take s3/s2 swapped against retail's s2/s3.
// A 4-minute permuter run on the 74.18% body found nothing; not re-run on this body.
/*
void func_800BFE48(void) {
    BattleActorSlot *pSlot;
    BattleFixed16Vector *pPosition;
    SpriteData *pSprite;
    SpriteDataI3 *pAnimState;
    u8 *pSpriteSlots;
    u8 *pSlotBytes;
    s32 anim;
    s32 i;
    s32 bit;
    u16 oldBits;
    u16 newBits;
    u16 addedBits;
    u16 clearedBits;

    pSlot = g_BattleStateWork;
    pSpriteSlots = (u8 *)g_BattleStateWork;
    for (i = 0; i != BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (pSlot->inUse == 0) {
            pSprite = *(SpriteData **)(pSpriteSlots + 0x8C8C);
            if (pSprite != NULL && BattleGetActorConditionCode(i) != 8) {
                switch ((s8)pSprite->flags_0xAC.bytes[3]) {
                case 5:
                case 7:
                case 14:
                case 15:
                case 21:
                    anim = (s8)pSprite->flags_0xAC.bytes[3];
                    if (anim != g_BattleConditionAnimIds[BattleGetActorConditionCode(i)]) {
                        SpritePlayAnimation(pSprite, 0x10);
                        g_BattleSlotBitMask &= ~(1 << BATTLE_SLOT_INDEX(pSprite));
                    }
                    break;
                }
            }
        }
        pSlot++;
        pSpriteSlots += 4;
    }
    func_800C0564();
    pSlot = g_BattleStateWork;
    pSpriteSlots = (u8 *)g_BattleStateWork;
    for (i = 0; i != BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (pSlot->inUse == 0) {
            pSprite = *(SpriteData **)(pSpriteSlots + 0x8C8C);
            if (pSprite != NULL && BattleGetActorConditionCode(i) != 8 &&
                (s8)pSprite->flags_0xAC.bytes[3] != 0x15) {
                pPosition = (BattleFixed16Vector *)&pSprite->position;
                if (BATTLE_ABS_DIFF(pPosition->xWhole - pSlot->x) >= 9 ||
                    BATTLE_ABS_DIFF(pPosition->zWhole - pSlot->z) >= 9) {
                    pSprite->flags_0xAC.flags |= 0x40;
                    pSprite->field_0xA0 = pSlot->x;
                    pSprite->field_0xA2 = 0;
                    pSprite->field_0xA4 = pSlot->z;
                    SpritePlayAnimation(pSprite, 3);
                }
            }
        }
        pSlot++;
        pSpriteSlots += 4;
    }
    func_800C0564();
    pSpriteSlots = (u8 *)g_BattleStateWork;
    for (i = 0; i != BATTLE_ACTOR_SLOT_COUNT; i++) {
        pSprite = *(SpriteData **)(pSpriteSlots + 0x8C8C);
        if (pSprite != NULL) {
            pSprite->flags_0xAC.flags &= ~0x40;
        }
        pSpriteSlots += 4;
    }
    pSlotBytes = (u8 *)g_BattleStateWork;
    pSlot = g_BattleStateWork;
    for (i = 0; i != BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (pSlot->inUse == 0) {
            pSprite = *(SpriteData **)(pSlotBytes + i * 4 + 0x8C8C);
            if (pSprite != NULL) {
                if ((s8)pSprite->flags_0xAC.bytes[3] != 0x15) {
                    BattleActorSetFacingFromSideFlag(i);
                }
                func_800C0314();
                if (BattleGetActorConditionCode(i) != 8) {
                    anim = g_BattleConditionAnimIds[BattleGetActorConditionCode(i)];
                    if (pSlot->inUse == 0 &&
                        (anim != 0x15 ||
                         ((g_BattleActorSlotDoneMask >> BATTLE_SLOT_INDEX(pSprite)) & 1) != 0)) {
                        if (anim == 1) {
                            anim = (s8)pSprite->flags_0xB0.bytes[0];
                        }
                        if (pSlot->flags == 0 && (s8)pSprite->flags_0xAC.bytes[3] != anim) {
                            SpritePlayAnimation(pSprite, anim);
                        }
                    }
                }
                newBits = FieldScriptClearAnimationOverrideIfFlag16(i);
                pAnimState = (SpriteDataI3 *)pSprite->field_0x7C;
                oldBits = pAnimState->field_0xC;
                pAnimState->field_0xC = newBits;
                clearedBits = oldBits & ~newBits;
                addedBits = newBits & ~oldBits;
                for (bit = 0; bit != 16; bit++) {
                    if (addedBits & 1) {
                        switch (bit) {
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                            break;
                        case 13:
                            BattleCreateSlotAnimationTask(pSprite, 9);
                            break;
                        case 14:
                            BattleCreateSlotAnimationTask(pSprite, 10);
                            break;
                        case 15:
                            BattleCreateSlotAnimationTask(pSprite, 11);
                            break;
                        }
                    }
                    addedBits >>= 1;
                }
                for (bit = 0; bit != 16; bit++) {
                    if (clearedBits & 1) {
                        switch (bit) {
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                            break;
                        case 13:
                            BattleInvokeSpriteCallbackForActor(pSprite, 9);
                            break;
                        case 14:
                            BattleInvokeSpriteCallbackForActor(pSprite, 10);
                            break;
                        case 15:
                            BattleInvokeSpriteCallbackForActor(pSprite, 11);
                            break;
                        }
                    }
                    clearedBits >>= 1;
                }
            }
        }
        pSlot++;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFE48);


// The battle state work block as the slot pass reads it: the actor slot rows at the
// start and the 0x8000 flag word the pass clears its bit from.
typedef struct {
    /* 0x0000 */ BattleActorSlot slots[BATTLE_ACTOR_SLOT_COUNT];
    /* 0x0134 */ u8 unk0134[0x904];
    /* 0x0A38 */ u16 slotFlag8000Mask;
} BattleSlotWorkView;

extern BattleActor *g_BattleActors[];
extern void func_800BEE2C(s32 slot, s32 mask, s32 command);
extern void BattleWaitAndFreeActorBuffers(void);

// 91.91% under the unit's own preset (gcc-2.7.2-cdk-psx -G0) at 592 bytes, retail's exact 148
// instructions (re-measured this round, unchanged), so only registers and block layout differ.
// Preset confirmed: gcc-2.6.0 -G0 scores 56.72 and -G8 does not compile.
// What matches and is load-bearing: the collect mask must be spelled `m & (m ^ g_BattleSlotBitMask)`
// (xor+and, NOT `& ~mask` which gives nor+and); the loop guard `if (i != 0) { for (i--; i >= 0; i--) }`
// reproduces retail's redundant beqz/bltz pair around the preheader; the slot index must stay an
// inline MACRO at all four sites (a local collapses three recomputations and costs 18 instructions);
// func_800BEE2C takes the index TWICE (retail sets a1 = a0 in the jal's delay slot); and the wait
// loop's `pending` must be a u8 (both its tests are andi 0xFF).
// Residuals: (a) callee-saved rotation - retail s0=state work, s6=its loop copy, s4=bitmask base,
// s5=g_BattleActors, s3=flag, against mine s0=state work, s3=bitmask base, s4=flag, s6=g_BattleActors:
// the same multiset, so the two sw pairs and every site's register differ; (b) the index's `or`
// operand order (retail builds it 0xAC-part first; swapping the macro's operands makes the two loads
// AC-first and drops to 85.10; the preheader's `beqz` delay slot holds a nop here where retail has
// `addiu s1,s1,-1`); (c) the WAIT LOOP's block layout, which is the biggest remaining block: retail
// re-tests count before EACH of its two calls (`beqz s2,.L800C04A4` with `move a1,zero` in the delay
// slot, twice) and ends with an unconditional `j .L800C04A4` whose delay slot sets pending = 0 (14
// insns); this body has one `bnez s2,top` at the bottom and 10 insns there.
// Measured today: `pending = 0` hoisted above the loop and repeated at the body end gives 92.25% at
// 588 bytes / 147 insns - one instruction SHORT of retail's 592, so it can never link - and its tail
// is still `bnez s2,top` + `pending = 0`, i.e. that placement buys the number, not retail's shape.
// Tried in earlier rounds: `if (pending != 0) {calls}` 91.51; a pSlotTable pointer local 90.31; the
// pWork->slots[] view 90.28; a 4-minute permuter run (base=1341 best=1341, nothing).
/*
s32 func_800C0314(void) {
    SpriteData *slots[BATTLE_SLOT_COUNT];
    BattleSlotWorkView *pWork;
    SpriteData *pSprite;
    s32 i;
    s32 count;
    u8 flag;
    u8 pending;

    pWork = (BattleSlotWorkView *)g_BattleStateWork;
    count = 0;
    flag = 0;
    i = BattleCollectMaskedSlotActors(pWork->slotFlag8000Mask &
                                          (pWork->slotFlag8000Mask ^ g_BattleSlotBitMask),
                                      (void **)slots, (s32)D_800C3E1C);
    if (i != 0) {
        for (i--; i >= 0; i--) {
            pSprite = slots[i];
            if ((g_BattleActorSlotDoneMask >> BATTLE_SLOT_INDEX(pSprite)) & 1) {
                slots[i] = NULL;
                continue;
            }
            pSprite->animScriptWaitTimer = 0;
            if (g_BattleStateWork[BATTLE_SLOT_INDEX(pSprite)].inUse != 0) {
                flag = 1;
                g_BattleActors[BATTLE_SLOT_INDEX(pSprite)]->isBusy = 1;
                func_800BEE2C(BATTLE_SLOT_INDEX(pSprite), BATTLE_SLOT_INDEX(pSprite), 0x15);
            } else if ((s8)pSprite->flags_0xAC.bytes[3] != 0x15) {
                SpritePlayAnimation(pSprite, 0x15);
            }
            count++;
            g_BattleSlotBitMask |= 1 << BATTLE_SLOT_INDEX(pSprite);
        }
    }
    while (count != 0) {
        pending = 0;
        for (i = 0; i != count; i++) {
            pSprite = slots[i];
            if (pSprite != NULL && pSprite->pDefaultAnimFile != NULL &&
                pSprite->animScriptWaitTimer != 0) {
                pending = 1;
            }
        }
        if (pending == 0) {
            break;
        }
        BattleLoadArchiveEntryFile();
        func_800BE790();
    }
    if (flag != 0) {
        BattleWaitAndFreeActorBuffers();
    }
    return count;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0314);


// 86.15% (96 insns, retail 96 - size equal, so it is a register/placement tie only):
// every instruction is present and in retail's order; what is left is
//  (a) gcc hoists g_BattleConditionAnimIds's address out of the outer restart loop to the prologue
//      where retail re-forms it at the top of the loop (.L800C05A8),
//  (b) gcc picks pSpriteSlot as the base for the slot accesses, so pSlot's two byte
//      reads come out as lbu 1(s3)/lbu 0(s3) off a derived base+7 pointer instead of
//      retail's lbu 8(s2)/lbu 7(s2),
//  (c) the two jump-table/table bases land in the opposite callee-saved registers
//      (s6/s7 swapped) and i/pSlot swap s2/s3.
//  (d) the 0x8C8C sprite-table offset: retail materialises the WHOLE constant
//      (li 0x8C8C; addu; lw 0(v0)), gcc splits it (li 0x8000; addu; lw 0xC8C(v0));
//      the same split is func_800BFE48's park residual (b), so one spelling that
//      forces the whole constant into a register may move both.
// Tried: index form (no strength reduction: gcc re-derives i*0x1C with shifts and
// folds 0x8C8C into the symbol), `for (i = 0; ...)` instead of the do/while,
// pExpectedAnimIds pointer local, `s32 spritesOff = 0x8C8C` local (60.65%), both
// orders of the two pointer inits, u8* vs BattleActorSlot* slot pointer, declaration
// orders, and a 4-minute permuter run (base=1405 best=800, and its two best edits read
// new_var->inUse from a pointer that never advances - wrong code, not portable).
// 92.65% (384 bytes = retail's 96 insns) under the unit preset XenoBattleTailCdk0. NEW BEST: the
// rescan is a goto, not `while (1)`. With a real outer loop gcc hoists the inner loop's invariants
// TWO levels, into the prologue (86.15%); with the goto the outer loop does not exist, so they
// hoist ONE level, to the rescan label, which is where retail forms them (lui/addiu of
// g_BattleConditionAnimIds at .L800C05A8, then the two pointer copies). That is worth +6.5.
// Residuals, largest first: (a) the switch's jump-table address (s7) is the ONE invariant retail
// DID hoist the second level - it sits among the register saves in retail's prologue, at the
// rescan label in mine, so the entry block's schedule differs; (b) the pSlot walker is anchored at
// +7 (gcc derives it as spriteSlot + 7, so the two slot reads are lbu 1(s3)/lbu 0(s3)) where
// retail anchors it at +0 (lbu 8(s2)/lbu 7(s2)); (c) the 0x8C8C table offset is split by gcc into
// li 0x8000 + lw 0xC8C(v0) where retail materialises the whole constant and loads at 0; (d) i and
// pSlot land s2/s3 swapped (pSpriteSlot is s4 in both).
// Tried on this body today: s32 i declared first (identical), the increments in the other two
// orders (i last: identical; pSlot last, so its anchor should be +0: 93.55% but 388 bytes - one
// instruction too many, an extra move v0,s8), a typed SpriteData **pSpriteSlot (85.53), plus the
// earlier round's index form, for-loop, pointer-local, declaration and permuter attempts.
// Preset screened: gcc-2.7.2 83.33, cdk -O3 86.15 - the preset is right.
/*
void func_800C0564(void) {
    u8 *pBase;
    BattleActorSlot *pSlot;
    u8 *pSpriteSlot;
    SpriteData *pSprite;
    s32 i;
    u8 needsWait;

    pBase = (u8 *)g_BattleStateWork;
    needsWait = 0;
    i = 0;
rescan:
    pSlot = (BattleActorSlot *)pBase;
    pSpriteSlot = (u8 *)pSlot;
    do {
            pSprite = *(SpriteData **)(pSpriteSlot + 0x8C8C);
            if (pSprite != NULL) {
                if (pSlot->inUse != 0) {
                    if (pSprite->pSpriteBytecode != NULL) {
                        needsWait = 1;
                    }
                } else if (BattleGetActorConditionCode(i) != 8) {
                    switch ((s8)pSprite->flags_0xAC.bytes[3]) {
                    case 0:
                    case 1:
                    case 5:
                    case 7:
                    case 14:
                    case 15:
                    case 20:
                    case 21:
                    case 22:
                    case 24:
                        break;
                    default:
                        if (pSlot->flags == 0 &&
                            (s8)pSprite->flags_0xAC.bytes[3] !=
                                g_BattleConditionAnimIds[BattleGetActorConditionCode(i)] &&
                            (s8)pSprite->flags_0xAC.bytes[3] != (s8)pSprite->flags_0xB0.bytes[0]) {
                            needsWait = 1;
                        }
                    }
                }
            }
            pSlot++;
            i++;
            pSpriteSlot += 4;
    } while (i != 11);
    if (needsWait != 0) {
        needsWait = 0;
        func_800BE790();
        i = 0;
        goto rescan;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0564);

s32 BattleComputeVectorDistance(VECTOR *pVec0, VECTOR *pVec1) {
    VECTOR v;

    v.vx = pVec0->vx - pVec1->vx;
    v.vy = pVec0->vy - pVec1->vy;
    v.vz = pVec0->vz - pVec1->vz;
    Square0(&v, &v);
    return SquareRoot0(v.vy + v.vz + v.vx);
}

s32 BattleComputeShortVectorDistance(SVECTOR *pVec0, SVECTOR *pVec1) {
    VECTOR v;

    v.vx = pVec0->vx - pVec1->vx;
    v.vy = pVec0->vy - pVec1->vy;
    v.vz = pVec0->vz - pVec1->vz;
    Square0(&v, &v);
    return SquareRoot0(v.vy + v.vz + v.vx);
}

s32 BattleComputeVectorDistance2D(DVECTOR vec0, DVECTOR vec1) {
    VECTOR v;

    v.vx = vec0.vx - vec1.vx;
    v.vz = vec0.vy - vec1.vy;
    Square0(&v, &v);
    return SquareRoot0(v.vx + v.vz);
}
