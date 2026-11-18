#include "common.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "system/archive.h"

/*
 * This unit's retail object materialises absolute symbol addresses into allocated
 * registers (lui $v0 / $v1 / $s0 ...) for every access, and it was assembled with
 * li left unexpanded (the XenoBattleTail preset's --dont-expand-li). At -G8 our
 * assembler expands a one-shot absolute store's address through $at; at -G0 gcc
 * splits it itself, which is what retail does: BattleClearFlagD2FDC, BattleSetSpriteTemplate and
 * BattleFreeActorArchiveAndSoundSeds are 100.00% under gcc 2.7.2-cdk -G0 and 45/45/81.63% at -G8.
 */

extern int ArchiveDataSync(void);
extern u8 D_800D2FDC;
extern void *D_800C3E1C;
extern s32 g_NumFlaggedTimerWorkListEntries;
extern DISPENV D_800C4A7C;
extern void *g_BattleActorArchiveBuffer;
extern void func_800BE790(void);
extern void SpritePlayAnimation(void *, s32);
extern void SpriteSetCallback(void *, void *);
extern void ArchiveStopTransfer(s32);

/* The work buffer carved out at battle init. Both of its self-pointers address
   the buffer itself; only these two fields are known in this unit. */
typedef struct {
    /* 0x00000 */ u8 unk0[0x4];
    /* 0x00004 */ void *unk4;
    /* 0x00008 */ u8 unk8[0x18];
    /* 0x00020 */ void *unk20;
    /* 0x00024 */ u8 unk24[0x10F58];
} BattleWorkBuffer; // Size: 0x10F7C

void BattleAllocWorkBuffer(void) {
    BattleWorkBuffer *pWork = HeapAlloc(sizeof(BattleWorkBuffer), 1);

    pWork->unk4 = pWork;
    pWork->unk20 = pWork;
    func_800B7424(pWork);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7424);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7870);
void BattleClearFlagD2FDC(void) {
    D_800D2FDC = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C34);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7E94);
void BattleSetSpriteTemplate(void *pArg) {
    D_800C3E1C = pArg;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8054);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8068);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B81BC);

/* One of the battle's two frame environments: the DRAWENV a frame draws with,
   that frame's DISPENV 0x5C bytes later, then the frame's ordering table (the 0x1000
   tags func_800B88C4 clears with ClearOTagR at DRAWENV + 0x70). */
typedef struct {
    /* 0x0000 */ DRAWENV draw;
    /* 0x005C */ DISPENV disp;
    /* 0x0070 */ u8 otBuffer[0x4000];
} BattleFrameEnv; // Size: 0x4070

/* The battle work block at g_BattleStateWork seen from its frame environments on. The
   0xB70 bytes before them are the actor table and the rest of the block. */
typedef struct {
    /* 0x0000 */ u8 unk0[0xB70];
    /* 0x0B70 */ BattleFrameEnv frames[2];
} BattleEnvBlock; // Size: 0x8C50

// This unit is a cdk unit (XenoBattleTailCdk, per-file): retail rebases the tail
// once (addiu $s0,$s0,-0xBCC off the DISPENV base = g_BattleStateWork) and then does eight
// sh off that register, where plain 2.7.2 folds base+offset into one absolute
// address per store and maspsx expands each through $at (16 insns instead of 11).
// The pBase assignment must stay after the SetGeomOffset call: that is what puts
// the three prologue saves in retail's order.
void BattleInitDispEnvAndDrawEnv(void) {
    u8 *pBase;
    BattleEnvBlock *pEnv;

    SetGeomOffset(0xA0, 0xA4);
    pBase = (u8 *)&D_800C4A7C;
    SetDefDispEnv((DISPENV *)pBase, 0, 0xE0, 0x140, 0xE0);
    SetDefDispEnv((DISPENV *)(pBase + 0x4070), 0, 0, 0x140, 0xE0);
    SetDefDrawEnv((DRAWENV *)(pBase - 0x5C), 0, 0, 0x140, 0xE0);
    SetDefDrawEnv((DRAWENV *)(pBase + 0x4014), 0, 0xE0, 0x140, 0xE0);

    pEnv = (BattleEnvBlock *)(pBase - 0xBCC);
    pEnv->frames[1].disp.screen.y = 0xA;
    pEnv->frames[0].disp.screen.y = 0xA;
    pEnv->frames[1].disp.screen.w = 0x100;
    pEnv->frames[0].disp.screen.w = 0x100;
    pEnv->frames[1].disp.screen.x = 0;
    pEnv->frames[0].disp.screen.x = 0;
    pEnv->frames[1].disp.screen.h = 0xD8;
    pEnv->frames[0].disp.screen.h = 0xD8;
}
void BattleWaitArchiveSync(void) {
    while (ArchiveDataSync() != 0) {
        func_800BE790();
    }
}

/* Archive entry table at D_800C35DC: the archive file index to load at +0 (it is what
   ArchiveDecodeAlignedSize and func_80029AFC take) and the base sound id at +2. */
typedef struct {
    /* 0x0 */ u16 fileIndex;
    /* 0x2 */ s16 soundId;
} BattleArchiveEntry; // Size: 0x4

/* Archive read request filled in on the stack and handed to func_80029AFC: the archive
   file index to load and the buffer it is decoded into, twice, then the third entry
   zeroed because func_80029AFC walks 8-byte entries until one's fileIndex is 0. */
typedef struct {
    /* 0x0  */ u16 fileIndex1;
    /* 0x2  */ u16 pad2;
    /* 0x4  */ void *pBuffer1;
    /* 0x8  */ u16 fileIndex2;
    /* 0xA  */ u16 padA;
    /* 0xC  */ void *pBuffer2;
    /* 0x10 */ u16 fileIndex3; // zero: ends the entry list func_80029AFC walks
    /* 0x12 */ u16 pad12;
    /* 0x14 */ s32 buffer3;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 pad1A;
    /* 0x1C */ s32 unk1C;
} BattleArchiveRequest; // Size: 0x20

extern BattleArchiveEntry D_800C35DC[];
extern void func_800C0F70(void);
extern void func_80029AFC(void *pRequest, s32 arg1, s32 arg2);
extern void SoundAddSedsEntry(void *pEntry);
extern void *SoundLoadWdsFile(void *pBuffer, s32 arg1);
extern void SoundFreeWdsEntry(void *pWds);
extern void SoundRemoveSedsEntry(void *pBuffer);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern s32 SoundGetActiveElementMask(s32 arg0);
extern s16 SoundGetPendingSpuCommandType(s32 arg0);

// Best 94.91% under the unit preset (gcc 2.7.2-cdk, -G8 and -G0 alike; 93.69
// under gcc 2.6.0). Structure is complete and in order; what is left:
// (1) register roles - retail has s0=pEntry, s1=pTable, s2=pBuffer1, s3=pWds,
// s4=arg0, s5=arg1, while arg0 takes $s0 here and pEntry stays live across the two
// poll loops in $s3 (retail re-forms &D_800C35DC[arg0] after the loops with a fresh
// lui/addiu, so pEntry is dead there and $s3 is free for pWds);
// (2) the two store-then-reload pairs retail keeps ("sh v0,16(sp)" + "lhu a0,16(sp)",
// likewise at 0x18(sp)) - gcc forwards the value from the register here.
// Tried and worse: request accessed through a pointer local (88.0, needs $s6),
// pTable[arg0] spelled inline instead of the pEntry local (88.0).
/*
void func_800B838C(s32 arg0, s32 arg1) {
    BattleArchiveRequest request;
    BattleArchiveEntry *pEntry;
    BattleArchiveEntry *pTable;
    void *pBuffer1;
    void *pBuffer2;
    void *pWds;
    s32 id;

    func_800C0F70();
    BattleWaitArchiveSync();
    ArchiveSetIndex(0x2C, 1);

    pTable = D_800C35DC;
    pEntry = &pTable[arg0];
    request.fileIndex1 = pEntry->fileIndex;
    pBuffer1 = HeapAlloc(ArchiveDecodeAlignedSize(request.fileIndex1), 0);
    request.pBuffer1 = pBuffer1;
    if (arg0 == 8) {
        request.fileIndex2 = pTable[8].fileIndex + 1;
    } else {
        request.fileIndex2 = pEntry->fileIndex + 1 + arg1;
    }
    pBuffer2 = HeapAlloc(ArchiveDecodeAlignedSize(request.fileIndex2), 0);
    request.pBuffer2 = pBuffer2;
    request.buffer3 = 0;
    request.fileIndex3 = 0;
    func_80029AFC(&request, 0, 0);

    BattleWaitArchiveSync();
    SoundAddSedsEntry(pBuffer1);
    pWds = SoundLoadWdsFile(pBuffer2, 0);
    while (SoundGetPendingSpuCommandType(0) != 0) {
        func_800BE790();
    }
    id = pEntry->soundId + arg1 + (*(u16 *)((u8 *)pBuffer1 + 0x14) << 16);
    SoundPlaySoundEffectOnFreePair(id);
    while (SoundGetActiveElementMask(id) != 0) {
        func_800BE790();
    }
    SoundRemoveSedsEntry(pBuffer1);
    SoundFreeWdsEntry(pWds);
    HeapFree(pBuffer2);
    HeapFree(pBuffer1);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B838C);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B853C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8774);
/* One 0x1C-byte slot of the battle actor table at g_BattleStateWork. */
typedef struct {
    /* 0x00 */ u8 unk0[0x6];
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 flags; // only masked and tested: bit 7 by BattleActorSlotHasFlag80, 0 means idle
    /* 0x08 */ u8 inUse; // set to 1 when the slot is claimed; every reader tests it against 0
    /* 0x09 */ u8 unk9[0x5];
    /* 0x0E */ u16 x;
    /* 0x10 */ u16 z;
    /* 0x12 */ u8 unk12[0xA];
} BattleActorSlot; // Size: 0x1C

/* Battle state pointed to by g_BattleState; only the fields this unit reads. */
typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ SpriteData *pSprite;
    /* 0x08 */ u8 unk8[0x18];
    /* 0x20 */ s32 initialActorIndex; // the actor index the init hands to BattleSetCurrentActorSprite
    /* 0x24 */ s32 actorIndex;
    /* 0x28 */ u8 unk28[0x18];
    /* 0x40 */ void *unk40; // an index, not a pointer: func_800B8EBC passes it to BattleLoadActorSlotAnimation
} BattleUnitState; // Size: 0x44

extern BattleUnitState *g_BattleState;
extern BattleActorSlot g_BattleStateWork[];
extern void BattleSetCurrentActorSprite(s32 actorIndex);
extern void BattleActorUpdateGroundHeightAndTriangle(SpriteData *pSprite);

extern u8 g_IsBattle;
extern u8 g_FlagTimerWorkListAdds;
extern s32 g_GfxWorkListCapacity;
extern s32 g_ModelRenderTestFlags;
extern void BattleResetStateGlobals(void);
extern void BattleResetTimerTaskGlobals(void);
extern void WorkListsReset(void);
extern void BattleResetAnimState(void);
extern void GfxAllocateWorkBuffers(s32 workBufferSize, s32 allocFlag);
extern void BattleClearActorMask(void);
extern void BattleEmptyHook(void);

void BattleInitStateWorkBuffers(void) {
    g_IsBattle = 1;
    g_NumFlaggedTimerWorkListEntries = 0;
    g_FlagTimerWorkListAdds = 0;
    g_GfxWorkListCapacity = 0x2000;
    BattleResetStateGlobals();
    BattleResetTimerTaskGlobals();
    WorkListsReset();
    BattleResetAnimState();
    GfxAllocateWorkBuffers(0x5000, 0);
    BattleClearActorMask();
    BattleClearFlagD2FDC();
    BattleEmptyHook();
    g_ModelRenderTestFlags = 0;
}

// Best 75.16%: the body is right apart from the loop's slot pointer, which gcc
// folds into the symbol (%lo(g_BattleStateWork + 0x54)) where retail materialises
// g_BattleStateWork and then does a separate `addiu v1,v0,0x54`, so the two lbu
// displacements come out -2/0 instead of 6/8. Tried: &g_BattleStateWork[3], (u8*)cast
// + 0x54, a typed pointer local, a pBase local shared with the tail - all fold.
extern s32 D_800C3D58;
extern s32 g_AnimScriptExtraTicks;
extern s32 g_GfxOTDepthShift;
/*
void func_800B88C4(void) {
    BattleActorSlot *pSlot;
    u8 *pBase;
    u8 *pWork;
    u8 *pOT;
    u8 *pBuf;
    s32 value;
    s32 n;
    s32 i;

    D_800C3D58 = 0;
    pSlot = (BattleActorSlot *)((u8 *)g_BattleStateWork + 0x54);
    for (i = 3; i != 0xB; i++) {
        if (pSlot->unk6 < 0x11 && pSlot->inUse != 0) {
            D_800C3D58++;
        }
        pSlot++;
    }
    n = D_800C3D58 / 2 - 1;
    g_AnimScriptExtraTicks = n;
    if (n < 0) {
        g_AnimScriptExtraTicks = 0;
    }
    pBase = (u8 *)g_BattleStateWork;
    pWork = pBase + 0x8000;
    value = g_AnimScriptExtraTicks;
    g_AnimScriptExtraTicks = 0;
    pOT = *(u8 **)(pWork + 0xC50);
    g_GfxOTDepthShift = 2;
    if (pOT == pBase + 0xB70) {
        pBuf = pBase + 0x4BE0;
    } else {
        *(s32 *)(pWork + 0xDAC) = value;
        pBuf = pBase + 0xB70;
    }
    *(u8 **)(pWork + 0xC50) = pBuf;
    *(u8 **)(pWork + 0xC54) = pBuf + 0x70;
    ClearOTagR((u_long *)(pBuf + 0x70), 0x1000);
    *(s32 *)(pWork + 0xC84) = 1;
    *(u8 **)(pWork + 0xC50) = pBase + 0x4BE0;
    *(u8 *)(pWork + 0xDA8) = 0;
    pBase[0x4BF8] = pBase[0xB88];
    pBase[0x4BF9] = pBase[0xB89];
    pBase[0x4BFA] = pBase[0xB8A];
    pBase[0x4BFB] = pBase[0xB8B];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B88C4);

void BattleEmptyHook(void) {
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B89FC);
void BattleFreeActorArchiveAndSoundSeds(void) {
    BattleWaitArchiveSync();
    BattleLoadArchiveEntryFile();
    while (g_NumFlaggedTimerWorkListEntries != BattleIsTimerTaskActive()) {
        func_800BE790();
    }
    BattleFreeSoundSeds();
    if (g_BattleActorArchiveBuffer != NULL) {
        HeapFree(g_BattleActorArchiveBuffer);
        g_BattleActorArchiveBuffer = NULL;
    }
}

void BattleUnloadActorArchive(void) {
    ArchiveStopTransfer(0);
    BattleFreeActorArchiveAndSoundSeds();
}

extern void BattleDestroyState(void);
extern void func_800BEE2C(u16 arg0, u16 arg1, s32 arg2);

void BattleActorInitSpriteFromSlot(void) {
    BattleActorSlot *pTable;
    SpriteData *pSprite;
    u32 slotHigh;
    s32 index;

    if (g_BattleState == NULL) {
        return;
    }
    BattleSetCurrentActorSprite(g_BattleState->initialActorIndex);
    pTable = g_BattleStateWork;
    pSprite = g_BattleState->pSprite;
    slotHigh = (u32)pSprite->flags_0xA8 >> 30;
    index = ((pSprite->flags_0xAC.flags & 3) << 2) | slotHigh;
    if (pTable[index].inUse != 0) {
        func_800BEE2C(index, 0, 0x1F);
    } else {
        pSprite->position.x = pTable[g_BattleState->actorIndex].x << 16;
        pSprite->position.z = pTable[g_BattleState->actorIndex].z << 16;
        BattleActorUpdateGroundHeightAndTriangle(pSprite);
        pSprite->position.y = pSprite->field_0x84 << 16;
        SpritePlayAnimation(pSprite, (s8)pSprite->flags_0xB0.bytes[0]);
        SpriteSetCallback(pSprite, NULL);
    }
    BattleDestroyState();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8EBC);

void BattleSpritePlayStoredAnimation(SpriteData *pSprite) {
    SpritePlayAnimation(pSprite, (s8)pSprite->flags_0xB0.bytes[0]);
    SpriteSetCallback(pSprite, NULL);
}
