#include "common.h"
#include "field/actor.h"
#include "psyq/libgte.h"
#include "system/sound.h"

extern s32 D_80059464;
extern s16 D_800D3678;
extern s32 D_800D363C[];
extern s32 D_800D3640[];
extern u16 D_800D2D4C;
extern u16 D_800D36BC;
extern s32 D_800C3610;
extern s32 D_800C3628;
extern s32 D_800C3CE8;
extern s32 D_800D2D68;
extern u8 D_800591AC;
extern u8 D_8006BE10[];

extern void func_800A9FF0(s32);
s32 BattleIsTimerTaskActive(void);
extern void func_800BD2E4(void);
extern void *func_800BFC80(void *, s32, s32);
extern s32 func_800B57E4(SpriteData *pSpriteData);
extern u8 *TimerWorkListAllocateTask(void *pList, s32 size);
extern u8 *func_80023B84(u8 *pActor, u8 *pEntry);
extern void TimerWorkListSetTaskCallback(void *pTask, void *pCallback);

/* One 0x1C-byte slot of the battle actor table at D_800C3EB0. */
typedef struct {
    /* 0x00 */ u8 unk0[0x6];
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ u8 unk8;
    /* 0x09 */ u8 unk9[0x5];
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u8 unk12[0xA];
} BattleActorSlot; // Size: 0x1C

extern BattleActorSlot D_800C3EB0[];
extern SpriteData *D_800C3E1C;
extern u8 D_800C362C;
extern s32 D_800C35D8;
extern void *D_800C3A6C;
extern u8 D_800C3620;
extern u8 D_800C3622;
extern u16 D_800D3634;
extern void BattleFreeActorArchiveAndSoundSeds(void);
extern void func_800BB760(s32 index);
extern void func_800C0F70(void);
extern void *SoundLoadWdsFile(void *pBuffer, s32 arg1);
extern s16 func_8003BDFC(s32 arg0);

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
    D_800C3CE8++;
}

extern void func_800B7C34(s32);
extern void SpriteSetCallback(SpriteData *pSpriteData, void *pCallback);
extern void SpritePlayAnimation(SpriteData *pSpriteData, s32 animId);
extern void func_800BE790(void);

// Byte 3 of the actor's 0xAC flag word is the animation it is playing, so it is
// read through the byte view of that word rather than as a named field.
void BattleActorWaitFramesWithCallback(s32 arg0, SpriteData *pSpriteData) {
    if (pSpriteData->pDefaultAnimFile == NULL) {
        func_800B7C34(arg0);
        return;
    }
    D_800C3CE8 = 0;
    if (*(s8 *)((u8 *)pSpriteData + 0xAF) != 0) {
        SpriteSetCallback(pSpriteData, BattleIncFrameCounter);
        SpritePlayAnimation(pSpriteData, *(s8 *)((u8 *)pSpriteData + 0xAF));
    }
    func_800B7C34(arg0);
    if (*(s8 *)((u8 *)pSpriteData + 0xAF) != 0) {
        while (D_800C3CE8 == 0) {
            func_800BE790();
        }
        SpriteSetCallback(pSpriteData, 0);
    }
}

void BattleUpdateActorSlotsIfStateExists(void) {
    if (D_800C3610 != 0) {
        func_800BD2E4();
    }
}

s32 BattleGetAdjustedWaitCountdown(void) {
    return D_80059464 - BattleIsTimerTaskActive();
}

s32 BattleIsTimerTaskActive(void) {
    return D_800D2D68 != 0;
}

void BattleSetPendingCommand(s32 arg0) {
    D_800C3628 = arg0;
}

void BattleUpdateDistanceTask(BattleDistanceTask *pTask) {
    BattleDistanceTask *pSelf;
    s32 prev;
    s32 dist;

    // Retail keeps this second name of the same block in its own register; using
    // pTask in the branch below as well drops the match to 91%.
    pSelf = pTask;
    prev = pTask->distance;
    dist = func_800B57E4(pTask->pSpriteData);
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
    pTask->animId = *(s8 *)((u8 *)pSpriteData + 0xAF);
    pTask->distance = func_800B57E4(pSpriteData);
    pTask->minDistance = minDistance;
    pSpriteData->flags_0xAC.flags |= 0x20;
}
// best 73.04% (71.61% in the variant below, 108 bytes = one instruction short):
// retail's first instruction is a `move t0,a1` copy of the second parameter (its
// a1 is then reused for the 0x8C8C constant), and its reload of the target slot
// happens before the D_800D363C lui; every instruction after that is present, only
// the allocator's register roles differ. 0x8C8C is a byte offset into the
// D_800C3EB0 block that does not fit a 16-bit displacement, so retail assembles
// base+index and +0x8C8C in two addu's - that part the C below does reproduce.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF85C);
/*
// Makes the actor of slot arg0 the current battle sprite and points it at the actor
// of slot arg1, which is also recorded as the model id table's only entry.
void func_800BF85C(s32 arg0, s32 arg1) {
    mem_addr base;
    s32 off;
    s32 slot;
    s32 target;
    SpriteData *pSprite;
    u8 *pTargetSlot;

    base = (mem_addr)D_800C3EB0;
    off = 0x8C8C;
    slot = arg1;
    pSprite = *(SpriteData **)(base + (arg0 << 2) + off);
    if (pSprite == NULL) {
        return;
    }
    pTargetSlot = (u8 *)(base + (slot << 2) + off);
    D_800C3E1C = pSprite;
    target = *(s32 *)pTargetSlot;
    D_800D3634 = 1 << slot;
    *(s32 *)((u8 *)pSprite + 0x74) = target;
    target = *(s32 *)pTargetSlot;
    D_800D363C[1] = 0;
    D_800D363C[0] = target;
}
*/
void BattleRemapActorModelId(u8 *pActor) {
    s32 i = 0;
    s32 *pEntry;

    if (D_800D3678 != 0) {
        pEntry = D_800D363C;
        for (i = 0; i != D_800D3678; i++) {
            if (pEntry[i] == *(s32*)(pActor + 0x74)) {
                break;
            }
        }
    }
    if (i >= D_800D3678) {
        *(s32*)(pActor + 0x74) = D_800D363C[0];
    } else {
        *(s32*)(pActor + 0x74) = D_800D3640[i];
    }
}
s32 BattleFindTableEntryIndex(s32 arg0) {
    s16 count = D_800D3678;
    s32 i = 0;

    if (count != 0) {
        s32 *pEntry = D_800D363C;

        for (i = 0; i != count; i++) {
            if (pEntry[i] == arg0) {
                break;
            }
        }
    }
    return i;
}
void BattleTickCountersAndFreeUnitSlot(void) {
    s16 first = D_800D2D4C + 1;
    s16 second = D_800D36BC + 1;

    D_800D2D4C = first;
    D_800D36BC = second;
    if (first == 2) {
        func_800A9FF0(0xB);
    }
}
extern u8 D_800C3621;
extern void BattleWaitArchiveSync(void);
extern int ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void func_8002DDE4(void *p0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6);

void BattleLoadArchiveEntryFile(void) {
    void *pBuffer;

    if (D_800C3621 != 0) {
        BattleWaitArchiveSync();
        ArchiveSetIndex(0x2C, 0);
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(1), 0);
        ArchiveReadFileToBuffer(1, pBuffer, 0, 0x80);
        BattleWaitArchiveSync();
        func_8002DDE4(pBuffer, 0, 0, 0, 0, 0, 0);
        func_800BE790();
        HeapFree(pBuffer);
        D_800C3621 = 0;
    }
}
// Hands every live actor slot whose animation is neither idle nor one of the 0x10
// special ones to the per-actor animation task, except the slot the current battle
// state owns (state mode 2). The 4-bit slot mode packs bits 30-31 of flags_0xA8 with
// bits 0-1 of flags_0xAC, so it is assembled from two pieces.
void func_800BFA9C(void) {
    s32 i;
    s32 mode;
    s32 high;
    s32 low;
    SpriteData *pSprite;

    if (D_800C362C == 0) {
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
        if (D_800C3EB0[i].unk8 != 0) {
            if (D_800C362C != 2 || i != mode) {
                if (D_800C3EB0[i].unk6 < 0x11) {
                    func_800BB760(i);
                }
            }
        }
    }
    while (D_800C35D8 != 0) {
        func_800BE790();
    }
    D_800C362C = 0;
}

// Loads the battle's sound archive file (directory 0x2C, entry 5) and hands it to
// the WDS player, unless the WDS table already has its id.
void func_800BFBA0(void) {
    SoundWDSEntry *pWds;

    if (D_800C3620 != 0) {
        return;
    }
    BattleWaitArchiveSync();
    ArchiveSetIndex(0x2C, 0);
    pWds = HeapAlloc(ArchiveDecodeAlignedSize(5), 0);
    ArchiveReadFileToBuffer(5, pWds, 0, 0x80);
    BattleWaitArchiveSync();
    if (SoundFindWdsEntry(pWds->id) == NULL) {
        func_800C0F70();
        D_800C3A6C = SoundLoadWdsFile(pWds, 0);
        while (func_8003BDFC(0) != 0) {
            func_800BE790();
        }
        D_800C3620 = 1;
        D_800C3622 = 0;
    }
    HeapFree(pWds);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFC80);

void BattleInvokeSpriteCallbackForActor(void *arg0, s32 arg1) {
    func_800BFC80(arg0, arg1, 1);
}

// 95.75%: everything matches except the D_8006BE10+0x10 load. Retail's ASPSX emits
// lui/addiu/%lo(sym) then lw 16; gcc folds the address into the load
// (lui %hi + lw 16(%hi_reg)), one instruction shorter - no C shape materialises it.
// Hoisting the func_80023B84 argument into pEntry (permuter, score 177 -> 20) took it
// from 90.75 to 95.75. A permuter variant reached 99.50 by 'pActor = &D_8006BE10[0]'
// so the table address lands in s2 and the 0x74 store writes the table instead of the
// actor: same byte encodings, wrong code - do not un-park it as a match. Also tried:
// (s32)D_8006BE10 + 0x10 (folds the same way), decl order swaps.
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
    pTable = &D_8006BE10[0];
    pBase = *(u8 **)(pTable + 0x10);
    pEntry = (u8 *)((s32)(arg1 + (u16 *)pBase)[1] + (s32)pBase);
    saved = D_800591AC;
    D_800591AC = 0;
    pNew = func_80023B84(pActor, pEntry);
    flags = *(s32 *)(pNew + 0xB0);
    *(s8 *)(pNew + 0xAF) = arg1;
    *(u8 **)(pNew + 0x74) = pActor;
    D_800591AC = saved;
    flags |= 0x100;
    *(s32 *)(pNew + 0xB0) = flags;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFDA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFE48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0314);
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
