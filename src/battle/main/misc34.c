#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgte.h"
#include "system/sound.h"

extern s32 g_NumFlaggedTimerWorkListEntries;
extern s16 g_BattleActorListCount;
extern s32 g_BattleActorList[];
extern s32 D_800D3640[];
extern u16 g_BattleDelayCounter;
extern u16 g_BattleActorDelayCounter;
extern s32 g_BattleState;
extern s32 g_BattlePendingCommand;
extern s32 g_BattleWaitFrameCounter;
extern s32 g_BattleTimerTask;
extern u8 g_FlagTimerWorkListAdds;
extern u8 g_SpriteAnimPackageDirs5[];

extern void func_800A9FF0(s32);
s32 BattleIsTimerTaskActive(void);
extern void BattleUpdateCommandRows(void);
extern void *func_800BFC80(void *, s32, s32);
extern s32 BattleSpriteDistanceToTarget(SpriteData *pSpriteData);
extern u8 *TimerWorkListAllocateTask(void *pList, s32 size);
extern u8 *func_80023B84(u8 *pActor, u8 *pEntry);
extern void TimerWorkListSetTaskCallback(void *pTask, void *pCallback);

/* One 0x1C-byte slot of the battle actor table at g_BattleStateWork. */
typedef struct {
    /* 0x00 */ u8 unk0[0x6];
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 flags; // only masked and tested: bit 7 by BattleActorSlotHasFlag80, 0 means idle
    /* 0x08 */ u8 inUse; // set to 1 when the slot is claimed; every reader tests it against 0
    /* 0x09 */ u8 unk9[0x5];
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u8 unk12[0xA];
} BattleActorSlot; // Size: 0x1C

extern BattleActorSlot g_BattleStateWork[];
extern SpriteData *D_800C3E1C;
extern u8 g_BattleActorSlotMode;
extern s32 g_BattlePendingActorTaskCount;
extern void *g_BattleWdsEntry;
extern u8 g_BattleSoundArchiveLoaded;
extern u8 g_BattleActorArchiveReady;
extern u16 D_800D3634;
extern void BattleFreeActorArchiveAndSoundSeds(void);
extern void func_800BB760(s32 index);
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
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF4F0);

void BattleIncFrameCounter(void) {
    g_BattleWaitFrameCounter++;
}

extern void func_800B7C34(s32);
extern void SpriteSetCallback(SpriteData *pSpriteData, void *pCallback);
extern void SpritePlayAnimation(SpriteData *pSpriteData, s32 animId);
extern void func_800BE790(void);

// The animation the actor is playing is byte 3 of its 0xAC flag word.
void BattleActorWaitFramesWithCallback(s32 arg0, SpriteData *pSpriteData) {
    if (pSpriteData->pDefaultAnimFile == NULL) {
        func_800B7C34(arg0);
        return;
    }
    g_BattleWaitFrameCounter = 0;
    if ((s8)pSpriteData->flags_0xAC.bytes[3] != 0) {
        SpriteSetCallback(pSpriteData, BattleIncFrameCounter);
        SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xAC.bytes[3]);
    }
    func_800B7C34(arg0);
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
void func_800BF85C(s32 arg0, s32 arg1) {
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
    D_800D3634 = 1 << arg1;
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
        pActor->pTargetEntitySprite = (void *)D_800D3640[i];
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
// Hands every live actor slot whose animation is neither idle nor one of the 0x10
// special ones to the per-actor animation task, except the slot the current battle
// state owns (state mode 2). The 4-bit slot mode packs bits 30-31 of flags_0xA8 with
// bits 0-1 of flags_0xAC, so it is assembled from two pieces.
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
                if (g_BattleStateWork[i].unk6 < 0x11) {
                    func_800BB760(i);
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFC80);

void BattleInvokeSpriteCallbackForActor(void *arg0, s32 arg1) {
    func_800BFC80(arg0, arg1, 1);
}

// 95.75%: everything matches except the g_SpriteAnimPackageDirs5+0x10 load. Retail's ASPSX emits
// lui/addiu/%lo(sym) then lw 16; gcc folds the address into the load
// (lui %hi + lw 16(%hi_reg)), one instruction shorter - no C shape materialises it.
// Hoisting the func_80023B84 argument into pEntry (permuter, score 177 -> 20) took it
// from 90.75 to 95.75. A permuter variant reached 99.50 by 'pActor = &g_SpriteAnimPackageDirs5[0]'
// so the table address lands in s2 and the 0x74 store writes the table instead of the
// actor: same byte encodings, wrong code - do not un-park it as a match. Also tried:
// (s32)g_SpriteAnimPackageDirs5 + 0x10 (folds the same way), decl order swaps.
/*
void func_800BFDA8(u8 *pActor, s32 arg1) {
    u8 *pBase;
    u8 *pTable;
    u8 *pEntry;
    u8 saved;
    u8 *pNew;
    s32 flags;

    if (*(s32 *)(pActor + 0x48) == 0) {
        return;
    }
    if (func_800BFC80(pActor, arg1, 0) != 0) {
        return;
    }
    pTable = &g_SpriteAnimPackageDirs5[0];
    pBase = *(u8 **)(pTable + 0x10);
    pEntry = (u8 *)((s32)(arg1 + (u16 *)pBase)[1] + (s32)pBase);
    saved = g_FlagTimerWorkListAdds;
    g_FlagTimerWorkListAdds = 0;
    pNew = func_80023B84(pActor, pEntry);
    flags = *(s32 *)(pNew + 0xB0);
    *(s8 *)(pNew + 0xAF) = arg1;
    *(u8 **)(pNew + 0x74) = pActor;
    g_FlagTimerWorkListAdds = saved;
    flags |= 0x100;
    *(s32 *)(pNew + 0xB0) = flags;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFDA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFE48);
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
// Tried: index form (no strength reduction: gcc re-derives i*0x1C with shifts and
// folds 0x8C8C into the symbol), `for (i = 0; ...)` instead of the do/while,
// pExpectedAnimIds pointer local, `s32 spritesOff = 0x8C8C` local (60.65%), both
// orders of the two pointer inits, u8* vs BattleActorSlot* slot pointer, declaration
// orders, and a 4-minute permuter run (base=1405 best=800, and its two best edits read
// new_var->inUse from a pointer that never advances - wrong code, not portable).
// The outer loop must stay a loop (a `goto rescan` puts the jtbl address in the wrong
// block: retail forms it in the prologue, g_BattleConditionAnimIds inside the loop).
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
    while (1) {
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
        if (needsWait == 0) {
            break;
        }
        needsWait = 0;
        func_800BE790();
        i = 0;
    }
}
*/
extern u16 g_BattleConditionAnimIds[];
extern s32 BattleGetActorConditionCode(s32 slot);
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
