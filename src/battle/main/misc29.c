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

// The battle's display region; the second of the two frame buffers starts one height down.
#define BATTLE_DISP_WIDTH 320
#define BATTLE_DISP_HEIGHT 224

// The per-battle actor table is 11 slots; the first three are not part of the
// group BattleInitFrameRenderState counts.
#define BATTLE_ACTOR_SLOT_COUNT 11
#define BATTLE_FIRST_ACTOR_SLOT 3

// Offset of the render/pad state inside the work block g_BattleStateWork starts.
#define BATTLE_RENDER_STATE_OFFSET 0x8000

// Ordering-table tags per frame buffer: the 0x1000-lentry tag list at frame + 0x70.
#define BATTLE_OT_TAG_COUNT 0x1000

extern int ArchiveDataSync(void);
extern u8 D_800D2FDC;
extern u16 g_BattleStageFileIndex; // stage file index the next stage load uses; func_800B8068 clears it
extern u8 g_BattleStageLoadQueued;  // set once the stage's archive reads are queued; func_800B8054 clears it
extern SpriteData *D_800C3E1C;
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
void func_800B7C34(s32 stage);
s32 func_800B7E94(void);

// The three actor spawn positions the stage loader blits into the framebuffer; the
// same table the slot allocator hands out.
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
} BattleActorSlotPos; // Size: 0x4

/* The actor stage archive as this unit reads it: the word at +4 is an offset from the
   buffer start to the file's chunk header, whose first u16 carries the stage variant
   in bits 12-13. */
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ u32 chunkOffset;
} BattleActorArchiveFile;

extern BattleActorSlotPos g_BattleActorSlotSpawnPos[3];
extern u16 g_BattleActorSlotUsedMask;
extern u8 g_BattleActorSlotMode;
extern u8 D_800C3CEC;
extern BattleActorArchiveFile *D_800594F0;
extern int *D_800594BC;
extern void func_800BB080(s32 arg0);
extern int ArchiveReadFileToBuffer(s32 fileIndex, void *pBuffer, u32 arg2, u32 arg3);
extern int ArchiveDecodeAlignedSize(s32 fileIndex);
extern void BattleUnloadActorArchive(void);
extern int *ArchiveAllocStreamFile(int numEntries, int allocMode);
extern void func_80029EB0(int, int *, int, int, short, short, short, short, short, short);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C34);
// 99.93% (608 bytes = retail's exact size). One instruction differs: retail's
// call-argument index ends `or a0,a0,v0` (the AC word is the or's destination),
// ours ends `or a0,v0,a0`. Everything else, including both 0xA8/0xAC load pairs
// (retail computes the index twice, once for the mask and once for the call) and
// the sh in the jal delay slot, is retail's.
// Measured spellings: AC-left at both sites 95.18 (swaps the loads), A8-left at
// both 93.93, mask A8-left + call AC-left 95.76, AC-left on the AC field without
// the (u32) cast 98.47, index in two locals 95.68 (one computation, 584 bytes).
// A 3-minute permuter run found nothing (base=10 best=10).
/*
// Loads the stage's actor archive and its stream file, then takes the slot state the
// archive's chunk header asks for: variant 1 frees every slot, 2 and 3 free all but
// the acting one. The actor archive buffer is kept for the teardown to free.
void func_800B7C34(s32 stage) {
    RECT rect;
    BattleActorSlotPos *pSpawnPos;
    BattleActorArchiveFile *pFile;
    SpriteData *pSprite;
    void *pBuffer;
    s32 mode;
    s32 fileIndex;
    s32 streamIndex;

    if (stage == 0xE3) {
        pSpawnPos = g_BattleActorSlotSpawnPos;
        rect.x = pSpawnPos[0].x;
        rect.y = pSpawnPos[0].y;
        rect.w = 0x40;
        rect.h = 0x100;
        MoveImage(&rect, 0x280, 0x100);
        rect.x = pSpawnPos[1].x;
        rect.y = pSpawnPos[1].y;
        rect.w = 0x40;
        rect.h = 0x100;
        MoveImage(&rect, 0x240, 0x100);
        rect.x = pSpawnPos[2].x;
        rect.y = pSpawnPos[2].y;
        rect.w = 0x40;
        rect.h = 0x100;
        MoveImage(&rect, 0x200, 0x100);
        DrawSync(0);
    }
    BattleUnloadActorArchive();
    ArchiveSetIndex(0xC, 2);
    D_800C3CEC = 1;
    D_800D2FDC = 1;
    fileIndex = stage * 2 + 0x22;
    streamIndex = stage * 2 + 0x23;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(fileIndex), 0);
    D_800594F0 = pBuffer;
    ArchiveReadFileToBuffer(fileIndex, pBuffer, 0, 0x80);
    BattleWaitArchiveSync();
    pFile = D_800594F0;
    mode = (*(u16 *)(pFile->chunkOffset + (s32)pFile) >> 12) & 3;
    if (mode != 0) {
        pSprite = D_800C3E1C;
        g_BattleActorSlotMode = mode;
        BattleBeginAnimTask(0);
        if (mode == 1) {
            func_800BB080(-1);
            g_BattleActorSlotUsedMask = 0;
        } else {
            g_BattleActorSlotUsedMask &=
                ~(1 << ((((u32)pSprite->flags_0xAC.flags & 3) << 2) | ((u32)pSprite->flags_0xA8 >> 30)));
            func_800BB080(((u32)pSprite->flags_0xA8 >> 30) | (((u32)pSprite->flags_0xAC.flags & 3) << 2));
        }
    }
    D_800594BC = ArchiveAllocStreamFile(8, 0);
    if (ArchiveDecodeAlignedSize(streamIndex) >= 0x11) {
        func_80029EB0(streamIndex, D_800594BC, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    BattleWaitArchiveSync();
    DrawSync(0);
    g_BattleActorArchiveBuffer = D_800594F0;
    HeapFree(D_800594BC);
}
*/
/* The work-list entry g_BattleStateWork's timer tasks are given; this unit only
   clears the trigger callback it adds the entry with. */
typedef struct {
    /* 0x00 */ void *unk0;
    /* 0x04 */ void *unk4;
    /* 0x08 */ void *onTriggerCallback;
    /* 0x0C */ void *onFreeCallback;
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ void *pNext;
} BattleTimerEntry; // Size: 0x1C

extern u8 D_800C3624;
extern s16 D_800C3DF0;
extern u8 g_BattleAudioFadeStarted;
extern s32 *g_BattleAudioManager;
extern void SoundSetAudioManagerVolumeWithFade(s32 *pManager, s32 arg1, s32 arg2);
extern SpriteAnimPackage g_SpriteAnimPackageDirs6;
extern u32 func_800C0FAC(void *pBuffer);
extern s32 SpriteAnimPackageTestFlag8000(void *pFrames);
extern void SpriteSetupAnimPackage(SpriteAnimPackage *pAnimPackage, void *pAnimPackageFile, SVEC2 tex, SVEC2 clut,
                                   u32 field_0x3C_1);
extern void SpriteSetSpecialAnimFile(void *pSprite, void *pAnimFile);
extern SpriteData *func_80023B84(SpriteData *pSprite, u8 *pAnimation, SpriteAnimPackage *pPackage);
extern void BattleReloadStageFile(void);
extern void TimerWorkListAddTask(void *pData, BattleTimerEntry *pEntry);
extern void TimerWorkListRemoveTask(BattleTimerEntry *pEntry);
extern u8 g_BattleSedsLoaded;

// Swaps the stage's actor animation in: rebuilds the shared animation package from the
// archive func_800B7C34 just loaded, then either starts the animation the current
// actor's package selects (variant 0) or attaches the special animation file (1), and
// hands the same frame environment to the outgoing sprite.
s32 func_800B7E94(void) {
    SVEC2 tex;
    SVEC2 clut;
    BattleTimerEntry timerEntry;
    SpriteAnimPackage packageCopy;
    SpriteAnimPackage *pPackage;
    SpriteData *pSprite;
    SpriteData *pOldSprite;
    u16 *pAnims;
    u32 pSedsFile;
    s32 offset;
    s32 result;

    pPackage = &g_SpriteAnimPackageDirs6;
    pOldSprite = D_800C3E1C;
    TimerWorkListAddTask(NULL, &timerEntry);
    timerEntry.onTriggerCallback = NULL;
    BattleWaitArchiveSync();
    tex.x = 0x380;
    tex.y = 0x100;
    clut.x = 0;
    clut.y = 0x1F4;
    D_800C3624 = 0;
    SpriteSetupAnimPackage(pPackage, D_800594F0, tex, clut, 0);
    BattleReloadStageFile();
    result = 0;
    if (SpriteAnimPackageTestFlag8000(pPackage->pFrames) != 0) {
        packageCopy = *(SpriteAnimPackage *)D_800C3E1C->pVramData;
        pAnims = pPackage->pAnimations;
        offset = pAnims[D_800C3DF0 + 1];
        pSprite = func_80023B84(D_800C3E1C, (u8 *)(offset + (s32)pAnims), pPackage);
    } else {
        result = 1;
        pSprite = D_800C3E1C;
        SpriteSetSpecialAnimFile(pSprite, D_800594F0);
        SpritePlayAnimation(pSprite, -1);
    }
    pSedsFile = func_800C0FAC(D_800594F0);
    pSprite->field_0x50 = pSedsFile;
    pOldSprite->field_0x50 = pSedsFile;
    g_BattleSedsLoaded = 1;
    g_BattleAudioFadeStarted = 1;
    SoundSetAudioManagerVolumeWithFade(g_BattleAudioManager, 0x60, 0x78);
    TimerWorkListRemoveTask(&timerEntry);
    return result;
}
void BattleSetSpriteTemplate(void *pArg) {
    D_800C3E1C = pArg;
}
void func_800B8054(s16 stage) {
    g_BattleStageFileIndex = stage;
    g_BattleStageLoadQueued = 0;
}

void func_800B8068(s32 stage) {
    func_800B7C34(stage);
    func_800B7E94();
    g_BattleStageLoadQueued = 1;
}
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

extern BattleActorSlot g_BattleStateWork[];

/* The streamed stage data behind g_BattleStageData: the two origins func_800B81BC
   publishes as the anim group and slot vector state. */
typedef struct {
    /* 0x000 */ u8 unk0[0x47C];
    /* 0x47C */ SVEC unk47C;
    /* 0x482 */ SVEC unk482;
} BattleStageData; // Size: 0x488

extern void func_800B7870(void);
extern void SoundLoadAndPlaySedsBank(void);
extern void func_801E8588(void);
extern void func_801E91E8(void);
extern void func_801E9594(void);
extern void func_801E893C(void);
extern void func_800A8B0C(void);
extern u8 func_801E7210(void *pData, void *pFileBuffer, void *pStreamBuffer, void *pTileSource,
                        void *pTileSourceTail, void *pColorSources);
extern void BattleSetColorSources(void *pColorSources, void *pDestination);
extern u8 g_BattleStageIndex;
extern s32 g_SystemStreamedFileData;
extern void *D_80059520;
extern void *g_SystemStreamedFileBuffer;
extern u8 D_800CCB94[];
extern u8 D_800C4A39[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8098);
// 99.86% (292 bytes = retail's size). Only the jump-table operand differs
// (retail names jtbl_80070A10, we say .rodata) and the label targets, i.e. the
// body is right. It cannot be converted alone: func_800B7870 still owns the
// table at the head of this unit's .rodata range (0xF0C, 0x14 bytes) and
// jtbl_80070A10 sits at +0x14, which is 4 mod 8 - converting only this function
// moves the table to the range start (or to an 8-aligned +0x18) and shifts the
// overlay. Convert func_800B7870's switch in the same change.
/*
void func_800B8098(s32 stage) {
    u8 *pBase;
    u8 colorFlag;

    g_BattleStageIndex = stage;
    BattleInitDispEnvAndDrawEnv();
    SoundLoadAndPlaySedsBank();
    switch (stage) {
    case 0:
        func_800B7870();
        break;
    case 1:
        ArchiveCdDataSync(0);
        func_801E8588();
        break;
    case 2:
        ArchiveCdDataSync(0);
        func_801E91E8();
        break;
    case 3:
        ArchiveCdDataSync(0);
        func_801E9594();
        break;
    case 4:
        ArchiveCdDataSync(0);
        func_801E893C();
        break;
    case 5:
        func_800B7870();
        break;
    default:
        func_800B7870();
        break;
    }
    ArchiveCdDataSync(0);
    func_800A8B0C();
    colorFlag = func_801E7210(&g_SystemStreamedFileData, D_80059520, g_SystemStreamedFileBuffer, D_800CCB94,
                              D_800CCB94 + 0x20, D_800C4A39);
    pBase = (u8 *)g_BattleStateWork;
    pBase[0xB88] = colorFlag;
    BattleSetColorSources(D_800C4A39, pBase + 0x4BF9);
}*/


/* The stage origins func_800B81BC publishes: g_BattleSlotVectors is the origin
   pair 8 bytes behind D_800D335C, so both globals are two of these. */
typedef struct {
    /* 0x0 */ SVEC vec;
    /* 0x6 */ u16 unk6;
} BattleStageOrigin; // Size: 0x8

extern BattleStageData *g_BattleStageData;
extern BattleStageOrigin g_BattleAnimGroupStates[2];
extern BattleStageOrigin g_BattleSlotVectors[2];
extern SVEC D_800D335C;
extern s32 g_GameWdsEntrySlot3;
extern void func_801E62E0(s32 arg0);
extern void BattleInitFrameRenderState(void);
extern void BattleInitStateWorkBuffers(void);

void func_800B81BC(s32 arg0) {
    BattleInitFrameRenderState();
    BattleInitStateWorkBuffers();
    func_801E62E0(arg0);
    SoundFreeWdsEntry((void *)g_GameWdsEntrySlot3);

    SpriteSetSVec(&g_BattleAnimGroupStates[0].vec, g_BattleStageData->unk482.x, g_BattleStageData->unk482.y,
                  g_BattleStageData->unk482.z);
    SpriteSetSVec(&g_BattleSlotVectors[0].vec, g_BattleStageData->unk482.x, g_BattleStageData->unk482.y,
                  g_BattleStageData->unk482.z);
    SpriteSetSVec(&g_BattleAnimGroupStates[1].vec, g_BattleStageData->unk47C.x, g_BattleStageData->unk47C.y,
                  g_BattleStageData->unk47C.z);
    SpriteSetSVec(&D_800D335C, g_BattleStageData->unk47C.x, g_BattleStageData->unk47C.y,
                  g_BattleStageData->unk47C.z);
    SetDispMask(1);
}

/* One of the battle's two frame environments: the DRAWENV a frame draws with,
   that frame's DISPENV 0x5C bytes later, then the frame's ordering table (the 0x1000
   tags BattleInitFrameRenderState clears with ClearOTagR at DRAWENV + 0x70). */
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
    SetDefDispEnv((DISPENV *)pBase, 0, BATTLE_DISP_HEIGHT, BATTLE_DISP_WIDTH, BATTLE_DISP_HEIGHT);
    SetDefDispEnv((DISPENV *)(pBase + 0x4070), 0, 0, BATTLE_DISP_WIDTH, BATTLE_DISP_HEIGHT);
    SetDefDrawEnv((DRAWENV *)(pBase - 0x5C), 0, 0, BATTLE_DISP_WIDTH, BATTLE_DISP_HEIGHT);
    SetDefDrawEnv((DRAWENV *)(pBase + 0x4014), 0, BATTLE_DISP_HEIGHT, BATTLE_DISP_WIDTH, BATTLE_DISP_HEIGHT);

    pEnv = (BattleEnvBlock *)(pBase - 0xBCC);
    pEnv->frames[1].disp.screen.y = 10;
    pEnv->frames[0].disp.screen.y = 10;
    pEnv->frames[1].disp.screen.w = 256;
    pEnv->frames[0].disp.screen.w = 256;
    pEnv->frames[1].disp.screen.x = 0;
    pEnv->frames[0].disp.screen.x = 0;
    pEnv->frames[1].disp.screen.h = 216;
    pEnv->frames[0].disp.screen.h = 216;
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
extern void BattleFreeWdsEntry(void);
extern void func_80029AFC(void *pRequest, s32 arg1, s32 arg2);
extern void SoundAddSedsEntry(void *pEntry);
extern void *SoundLoadWdsFile(void *pBuffer, s32 arg1);
extern void SoundFreeWdsEntry(void *pWds);
extern void SoundRemoveSedsEntry(void *pBuffer);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern s32 SoundGetActiveElementMask(s32 arg0);
extern s16 SoundGetPendingSpuCommandType(s32 arg0);

void BattleLoadSoundEffectFilesAndPlay(s32 arg0, s32 arg1) {
    BattleArchiveRequest request;
    BattleArchiveEntry *pEntry;
    BattleArchiveEntry *pTable;
    void *pBuffer1;
    void *pBuffer2;
    void *pWds;
    s32 fileIndex;
    s32 id;

    BattleFreeWdsEntry();
    BattleWaitArchiveSync();
    ArchiveSetIndex(ARCHIVE_DIR_BATTLE_SOUND, 1);

    // the table needs its own pointer local; spelling every site as D_800C35DC[i] moves a register.
    pTable = D_800C35DC;
    pEntry = &pTable[arg0];
    request.fileIndex1 = pEntry->fileIndex;
    pBuffer1 = HeapAlloc(ArchiveDecodeAlignedSize(request.fileIndex1), 0);
    request.pBuffer1 = pBuffer1;
    if (arg0 == 8) {
        request.fileIndex2 = pTable[8].fileIndex + 1;
    } else {
        fileIndex = pEntry->fileIndex + 1;
        request.fileIndex2 = fileIndex + arg1;
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
    id = D_800C35DC[arg0].soundId + arg1 + (*(u16 *)((u8 *)pBuffer1 + 0x14) << 16);
    SoundPlaySoundEffectOnFreePair(id);
    while (SoundGetActiveElementMask(id) != 0) {
        func_800BE790();
    }
    SoundRemoveSedsEntry(pBuffer1);
    SoundFreeWdsEntry(pWds);
    HeapFree(pBuffer2);
    HeapFree(pBuffer1);
}

/* The battle work block that g_BattleStateWork + 0x8000 starts: the render and
   pad state. Only the fields this unit touches are typed here. */
typedef struct {
    /* 0x000 */ u8 unk0[0xC50];
    /* 0xC50 */ u8 *pRenderEnv; // one of the two double-buffered frame env blocks
    /* 0xC54 */ u_long *pCurrentOT; // that block's ordering table at +0x70
    /* 0xC58 */ u8 unkC58[0x2C];
    /* 0xC84 */ s32 colorSetIndex; // selects the effect colour set; cleared at stage load, 1 after a change
    /* 0xC88 */ u8 unkC88[0x120];
    /* 0xDA8 */ u8 unkDA8;
    /* 0xDA9 */ u8 unkDA9[0x3];
    /* 0xDAC */ s32 unkDAC;
    /* 0xDB0 */ u8 unkDB0[0x4D0];
} BattleRenderState; // Size: 0x1280

/* The work block at g_BattleStateWork seen from its sprite pointers: the eleven
   sprites this function clears the 0x40 flag of on the way out. */
typedef struct {
    /* 0x0000 */ u8 unk0[0x8C8C];
    /* 0x8C8C */ SpriteData *pOwnedSprites[BATTLE_ACTOR_SLOT_COUNT];
} BattleOwnedSprites; // Size: 0x8CB8

extern void BattleReleaseActorSlot(s32 index);
extern void GfxFreeWorkBuffers(void);
extern void WorkListsFreeAllEntries(void);
extern void BattleFreeAllModelSlots(void);
extern void BattleFreeSceneResources(void);
extern void *g_BattleSoundBankMain;
extern void *g_BattleSceneResources;
extern u8 g_IsBattle;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B853C);
// 88.41% (576 bytes vs retail's 560). The switch tree, every call and both loops
// are retail's; what is left is register allocation: retail runs the case loops
// with only $s0-$s3 (it re-materialises `li v0,21` and `li v0,3` inside the loop)
// while gcc hoists both constants into $s4/$s3 here and needs two more saves and a
// 0x38 frame. The tail loop's 3/0xB are materialised straight into the tail block
// in retail, in the switch tree's delay slots here. Listing case 1 first is
// required for retail's test order (1, then the slti a0,2 branch, then 0, then 2).
/*
extern void BattleStartScreenFade(s32 arg0, u8 arg1, u8 arg2, u8 arg3, u8 arg4);
extern void func_800A9FF0(s32 arg0);
extern void BattleBeginAnimTask(s32 arg0);
extern void ArchiveReadFileToBuffer(s32 fileIndex, void *pBuffer, u32 arg2, u32 arg3);
extern void *D_800D39C8;

void func_800B853C(s32 mode) {
    BattleOwnedSprites *pOwned;
    BattleRenderState *pWork;
    SpriteData *pSprite;
    void *pBuffer;
    s32 i;

    switch (mode) {
    case 1:
        break;
    case 0:
        BattleBeginAnimTask(-1);
        pOwned = (BattleOwnedSprites *)g_BattleStateWork;
        for (i = 0; i != 3; i++) {
            pSprite = pOwned->pOwnedSprites[i];
            if (pSprite != NULL && (s8)pSprite->flags_0xAC.bytes[3] != 0x15) {
                BattleFindAnimationTask(pSprite, 0, 2);
                SpritePlayAnimation(pSprite, 0x18);
            }
        }
        break;
    case 2:
        BattleBeginAnimTask(0);
        BattleStartScreenFade(0x28, 2, 0xFF, 0xFF, 0xFF);
        pOwned = (BattleOwnedSprites *)g_BattleStateWork;
        for (i = 0; i != 3; i++) {
            pSprite = pOwned->pOwnedSprites[i];
            if (pSprite != NULL && (s8)pSprite->flags_0xAC.bytes[3] != 0x15) {
                SpritePlayAnimation(pSprite, 5);
            }
        }
        for (i = 0x28; i > 0; i--) {
            func_800BE790();
        }
        break;
    }
    for (i = BATTLE_FIRST_ACTOR_SLOT; i != BATTLE_ACTOR_SLOT_COUNT; i++) {
        func_800A9FF0(i);
        BattleReleaseActorSlot(i);
    }
    BattleFreeWdsEntry();
    BattleWaitArchiveSync();
    ArchiveSetIndex(0x2C, 0);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(5), 1);
    ArchiveReadFileToBuffer(5, pBuffer, 0, 0x80);
    BattleWaitArchiveSync();
    g_GameWdsEntrySlot3 = SoundLoadWdsFile(pBuffer, 0);
    while (SoundGetPendingSpuCommandType(0) != 0) {
        func_800BE790();
    }
    HeapFree(pBuffer);
    HeapFree(D_800D39C8);
    pWork = (BattleRenderState *)((u8 *)g_BattleStateWork + BATTLE_RENDER_STATE_OFFSET);
    pWork->unkDA8 = 0;
}
*/
void func_800B8774(void) {
    u8 *pStateBase;
    BattleRenderState *pState;
    s32 i;

    pStateBase = (u8 *)g_BattleStateWork;
    pState = (BattleRenderState *)(pStateBase + 0x8000);
    if (pState->colorSetIndex == 0) {
        func_800BE790();
    }
    DrawSync(0);
    for (i = 0; i != 0xB; i++) {
        BattleReleaseActorSlot(i);
    }
    GfxFreeWorkBuffers();
    WorkListsFreeAllEntries();
    SoundRemoveSedsEntry(g_BattleSoundBankMain);
    HeapFree(g_BattleSoundBankMain);
    g_IsBattle = 0;
    DrawSync(0);
    BattleFreeAllModelSlots();
    BattleFreeSceneResources();
    HeapFree(g_BattleSceneResources);
}

/* Battle state pointed to by g_BattleState; only the fields this unit reads. */
typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ SpriteData *pSprite;
    /* 0x08 */ u8 unk8[0x18];
    /* 0x20 */ s32 initialActorIndex; // the actor index the init hands to BattleSetCurrentActorSprite
    /* 0x24 */ s32 actorIndex;
    /* 0x28 */ u8 unk28[0x18];
    /* 0x40 */ void *savedActorSlot; // an index, not a pointer: BattleTearDownState passes it to BattleLoadActorSlotAnimation
} BattleUnitState; // Size: 0x44

extern BattleUnitState *g_BattleState;
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

extern s32 g_BattleLiveSlotCount;
extern s32 g_AnimScriptExtraTicks;
extern s32 g_GfxOTDepthShift;

void BattleInitFrameRenderState(void) {
    BattleActorSlot *pSlot;
    BattleEnvBlock *pEnv;
    BattleRenderState *pWork;
    u8 *pBase;
    BattleFrameEnv *pFrame;
    u8 *pOldOT;
    s32 extraTicks;
    s32 i = BATTLE_FIRST_ACTOR_SLOT;

    g_BattleLiveSlotCount = 0;
    pSlot = (BattleActorSlot *)g_BattleStateWork;
    for (; i != BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (pSlot[i].unk6 < 0x11 && pSlot[i].inUse != 0) {
            g_BattleLiveSlotCount++;
        }
    }
    g_AnimScriptExtraTicks = g_BattleLiveSlotCount / 2 - 1;
    if (g_AnimScriptExtraTicks < 0) {
        g_AnimScriptExtraTicks = 0;
    }
    pBase = (u8 *)g_BattleStateWork;
    pEnv = (BattleEnvBlock *)pBase;
    pWork = (BattleRenderState *)(pBase + BATTLE_RENDER_STATE_OFFSET);
    extraTicks = g_AnimScriptExtraTicks;
    g_AnimScriptExtraTicks = 0;
    pFrame = &pEnv->frames[0];
    pOldOT = pWork->pRenderEnv;
    g_GfxOTDepthShift = 2;
    pWork->unkDAC = extraTicks;
    if (pOldOT == (u8 *)pFrame) {
        pFrame = &pEnv->frames[1];
    }
    pWork->pRenderEnv = (u8 *)pFrame;
    pWork->pCurrentOT = (u_long *)pFrame->otBuffer;
    ClearOTagR((u_long *)pWork->pCurrentOT, BATTLE_OT_TAG_COUNT);
    pWork->colorSetIndex = 1;
    pEnv->frames[1].draw.isbg = pEnv->frames[0].draw.isbg;
    pEnv->frames[1].draw.r0 = pEnv->frames[0].draw.r0;
    pEnv->frames[1].draw.g0 = pEnv->frames[0].draw.g0;
    pEnv->frames[1].draw.b0 = pEnv->frames[0].draw.b0;
    pWork->pRenderEnv = (u8 *)&pEnv->frames[1];
    pWork->unkDA8 = 0;
}

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


extern u8 g_BattleActorArchiveReady;
extern u8 D_800C3688;
extern void BattleSetFlag2DBAndReleaseAssets(void);
extern void func_800C0314(void);
extern void func_800C0564(void);
extern void BattleSetAnimTaskDoneState(s32 value);
extern void BattleFreeTimerTaskIfActive(void);
extern void BattleReleaseAllActorSlots(void);
extern void BattleSetAnimTaskShort(s16 value);
extern void BattleLoadActorSlotAnimation(s32 index);

void BattleTearDownState(void) {
    SpriteData *pSprite;
    SpriteData *pOwnedSprite;
    BattleOwnedSprites *pSprites;
    s32 slot;
    s32 i;

    // retail loads the sprite before testing the pointer, which is what keeps
    // g_BattleState's address in $s1 across the calls below.
    pSprite = g_BattleState->pSprite;
    if (g_BattleState == NULL) {
        return;
    }
    D_800C3624 = 0;
    D_800C3DF0 = 0;
    g_BattleActorArchiveReady = 0;
    BattleSetFlag2DBAndReleaseAssets();
    DrawSync(0);
    slot = g_BattleState->savedActorSlot;
    SpriteSetCallback(pSprite, NULL);
    func_800C0314();
    while (g_NumFlaggedTimerWorkListEntries != BattleIsTimerTaskActive()) {
        func_800BE790();
    }
    func_800C0564();
    BattleUnloadActorArchive();
    BattleSetAnimTaskDoneState(1);
    BattleFreeTimerTaskIfActive();
    BattleLoadArchiveEntryFile();
    BattleReleaseAllActorSlots();
    D_800C3688 = 0;
    BattleSetAnimTaskShort(0xC0);
    BattleDestroyState();
    BattleLoadActorSlotAnimation(slot);
    if (g_BattleAudioFadeStarted != 0) {
        SoundSetAudioManagerVolumeWithFade(g_BattleAudioManager, 0x7F, 0x50);
    }
    g_BattleAudioFadeStarted = 0;
    pSprites = (BattleOwnedSprites *)g_BattleStateWork;
    for (i = 0; i != BATTLE_ACTOR_SLOT_COUNT; i++) {
        pOwnedSprite = pSprites->pOwnedSprites[i];
        if (pOwnedSprite != NULL) {
            pOwnedSprite->flags_0xAC.flags &= ~0x40;
        }
    }
}

void BattleSpritePlayStoredAnimation(SpriteData *pSprite) {
    SpritePlayAnimation(pSprite, (s8)pSprite->flags_0xB0.bytes[0]);
    SpriteSetCallback(pSprite, NULL);
}
