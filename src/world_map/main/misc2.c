#include "common.h"
#include "main/game.h"
#include "psyq/libapi.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

extern int Vsync(int mode);
extern int rand(void);
extern void SoundStopAllAudioManagers(void);
extern void SoundDestroyAudioManager(s32* pFile);
extern void* SoundCreateAudioManagerOnHeap(void* pFile);
extern void SoundPlayAudioManager(s32* pFile, s32 volume, s32 frames);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);

typedef struct {
    /* 0x0 */ s16 tile;
    /* 0x2 */ s16 duration;
} WorldMapFrame; // Size: 0x4

typedef struct {
    /* 0x0 */ RECT rect;
    /* 0x8 */ u32 unk8;
    /* 0xC */ WorldMapFrame* pFrames;
} WorldMapSprite; // Size: 0x10

typedef struct {
    /* 0x0 */ u32 pImageData;
    /* 0x4 */ WorldMapSprite* pSprite;
    /* 0x8 */ s16 frameIndex;
    /* 0xA */ s16 frameTimer;
} WorldMapEntry; // Size: 0xC

// Header of an animation set: a count followed by one offset per entry,
// relative to the header itself.
typedef struct {
    /* 0x0 */ s32 count;
    /* 0x4 */ s32 offsets[1];
} WorldMapAnimSet; // Size: 0x8

typedef struct {
    /* 0x0 */ u32 data[0x800];
} ActorBlock; // Size: 0x2000

typedef struct {
    /* 0x0 */ s16 data[0x10];
} SlotBlock; // Size: 0x20

typedef struct {
    /* 0x0 */ u32 data[0xA0];
} UnkBlock; // Size: 0x280

typedef struct {
    /* 0x00 */ u8 pad0[0x24];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u16 unk26;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ u8 pad34[0x24];
    /* 0x58 */ s32 trailDelay;
    /* 0x5C */ u8 pad5C[0x24];
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ u16 pad6;
} Vector16; // Size: 0x8

extern void WorldMapInitGfxEnv(void);
extern void WorldMapDrawFadeFrames(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern void WorldMapLoadTextureClut(void);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapAllocSpritePrimBuffers(void);
extern void SystemTransferPaletteToVRAM(short xDest, short yDest);
extern void ControllerResetState(void);
extern void ArchiveCdDataSync(s32 arg0);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void* g_MenuResourcesFile;
extern void* g_WorldMapDecompressBuffer;
extern s32 g_WorldMapReloadFileIndex;
extern void* g_WorldMapLoadedFileData;
extern u8* g_WorldMapRenderContext;
extern u_long* g_WorldMapSavedVramTexture;
extern u_long* g_WorldMapSavedVramStrip;
extern s32 g_WorldMapSavedModeFlag;
extern s32 g_WorldMapMode4EntryTriggered;
extern u_char g_FieldPlayerWalkmeshMaterial;

extern u32 g_WorldMapSaveBlock[];
extern u8 g_WorldMapTrail[];
extern s32 g_WorldMapScrollX;
extern s32 g_WorldMapScrollY;
extern s32 g_WorldMapScrollZ;
extern s32 g_WorldMapCameraRot;
extern s32 g_WorldMapViewRotationZ;
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapGridOrigin;
extern s32 g_WorldMapGridTileRowOrigin;
extern s16 g_WorldMapTrailCursor;
extern s32 g_WorldMapCameraHeight;
extern s16 g_WorldMapPlayerAngle;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;

extern s32 WorldMapGetTileGroup(s32 arg0);
extern s32 WorldMapGetTileHalfIndex(s32 arg0);
extern s16 D_8009A3A0[];
extern u16 D_8009B578[];
extern u8* g_WorldMapTileGroupData[];
extern u8 D_800658DC[];
extern u8 g_GameSelectedSkinIndex;

extern WorldMapActor* g_WorldMapSlots;
extern s32 g_WorldMapMode;
extern u8* g_WorldMapSlotPrims;
extern u8* g_WorldMapSlotPrimsCopy;
extern s32 g_WorldMapSlotPositionIndex;
extern Vector16* g_WorldMapSlotPositions;
extern s32 g_WorldMapDecompressedMapData;
extern s32 g_WorldMapModelData;
extern s32 g_WorldMapSpriteSetTable;
extern s32 g_WorldMapModelRelocs;
extern s32 g_WorldMapSpriteGroups;
extern s32 g_WorldMapPrimBlocks;
extern WorldMapAnimSet* g_WorldMapTileAnimSet;
extern WorldMapAnimSet* g_WorldMapFrameAnimSet;
extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern s32 g_WorldMapTileAnimEntryCount;
extern u8 g_WorldMapTileAnimSprites[];
extern s16 D_8009C872[1];
extern s16 g_WorldMapRandomTimers[];
extern s32 g_WorldMapRandomTimerCount;
extern s32 g_WorldMapRandomTimerPeriod;
extern s32 g_WorldMapRandomTimerCountdown;
extern s32 g_WorldMapExpiredTimerCount;
extern s32* g_SoundAudioManager;
extern s32 g_WorldMapFrameAnimEntryCount;
extern WorldMapEntry* g_WorldMapTileAnimEntries;
extern WorldMapEntry* g_WorldMapFrameAnimEntries;
extern u8 g_WorldMapFrameAnimSprites[];
extern u8 g_SoundBankLoadBuffer[];

void WorldMapFreePrimBuffers(void) {
    HeapFree(g_WorldMapSlotPrimsCopy);
    HeapFree(g_WorldMapSlotPrims);
    HeapFree(g_WorldMapSlotPositions);
}

void WorldMapAddSlotPosition(s16 arg0, s32* pVec) {
    s32 index;

    index = g_WorldMapSlotPositionIndex;
    g_WorldMapSlotPositions[index].vx = pVec[0] >> 12;
    g_WorldMapSlotPositions[index].vz = pVec[2] >> 12;
    g_WorldMapSlotPositions[index].vy = arg0;
    g_WorldMapSlotPositionIndex = (index + 1) & 0xF;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800747DC);
void WorldMapInitTileAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 count;
    s32 i;

    count = g_WorldMapTileAnimSet->count;
    g_WorldMapTileAnimEntryCount = count;
    pEntry = (WorldMapEntry*)HeapAlloc(count * sizeof(WorldMapEntry), 0);
    g_WorldMapTileAnimEntries = pEntry;
    for (i = 0; i < g_WorldMapTileAnimEntryCount; i++, pEntry++) {
        pEntry->pImageData = (u32)g_WorldMapTileAnimSet + g_WorldMapTileAnimSet->offsets[i];
        pEntry->pSprite = &((WorldMapSprite*)g_WorldMapTileAnimSprites)[i];
        pEntry->frameIndex = 0;
        pEntry->frameTimer = 1;
    }
}

void WorldMapFreeTileAnimEntries(void) {
    HeapFree(g_WorldMapTileAnimEntries);
}

void WorldMapUpdateTileAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 i;

    pEntry = g_WorldMapTileAnimEntries;
    for (i = 0; i < g_WorldMapTileAnimEntryCount; i++, pEntry++) {
        pEntry->frameTimer--;
        if (pEntry->frameTimer == 0) {
            pEntry->frameIndex++;
            pEntry->frameTimer = pEntry->pSprite->pFrames[pEntry->frameIndex].duration;
            if (pEntry->frameTimer < 0) {
                pEntry->frameIndex = 0;
                pEntry->frameTimer = pEntry->pSprite->pFrames[0].duration;
            }
            LoadImage(&pEntry->pSprite->rect,
                      (u_long*)(pEntry->pImageData + (pEntry->pSprite->pFrames[pEntry->frameIndex].tile << 4)));
        }
    }
}

void WorldMapInitFrameAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 count;
    s32 i;

    count = g_WorldMapFrameAnimSet->count;
    g_WorldMapFrameAnimEntryCount = count;
    pEntry = (WorldMapEntry*)HeapAlloc(count * sizeof(WorldMapEntry), 0);
    g_WorldMapFrameAnimEntries = pEntry;
    for (i = 0; i < g_WorldMapFrameAnimEntryCount; i++, pEntry++) {
        pEntry->pImageData = (u32)g_WorldMapFrameAnimSet + g_WorldMapFrameAnimSet->offsets[i];
        pEntry->pSprite = &((WorldMapSprite*)g_WorldMapFrameAnimSprites)[i];
        pEntry->frameIndex = 0;
        pEntry->frameTimer = 1;
    }
}

void WorldMapFreeFrameAnimEntries(void) {
    HeapFree(g_WorldMapFrameAnimEntries);
}

void WorldMapUpdateFrameAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 offset;
    s32 i;

    pEntry = g_WorldMapFrameAnimEntries;
    for (i = 0; i < g_WorldMapFrameAnimEntryCount; i++, pEntry++) {
        pEntry->frameTimer--;
        if (pEntry->frameTimer == 0) {
            pEntry->frameIndex++;
            pEntry->frameTimer = pEntry->pSprite->pFrames[pEntry->frameIndex].duration;
            if (pEntry->frameTimer < 0) {
                pEntry->frameIndex = 0;
                pEntry->frameTimer = pEntry->pSprite->pFrames[0].duration;
            }
            offset = pEntry->pSprite->rect.h * pEntry->pSprite->rect.w * 2;
            LoadImage(&pEntry->pSprite->rect,
                      (u_long*)(pEntry->pImageData +
                                pEntry->pSprite->pFrames[pEntry->frameIndex].tile * offset));
        }
    }
}

void WorldMapInitRandomTimers(void) {
    s32 i;
    s32 screenWidth;
    u16 flags;
    s16* p;

    i = 0xF;
    p = D_8009C872;
    for (; i >= 0; i--) {
        *p = 0;
        p--;
    }
    flags = g_GameState.worldMapState;
    g_WorldMapRandomTimerCountdown = 1;
    if (!(flags & 0x4000)) {
        // retail reuses the flags temporary's register for the width
        flags = 0x180;
        screenWidth = flags;
    } else {
        screenWidth = 0x300;
    }
    g_WorldMapRandomTimerPeriod = screenWidth;
    g_WorldMapRandomTimerCount = 1;
    g_WorldMapExpiredTimerCount = 0;
}

void WorldMapUpdateRandomTimers(void) {
    s32 value;
    s32 j;
    s32 i;

    if (--g_WorldMapRandomTimerCountdown == 0) {
        for (i = 0; i < g_WorldMapRandomTimerCount; i++) {
            if (i != 0) {
                do {
                    value = rand() % g_WorldMapRandomTimerPeriod + 1;
                    for (j = 0; j < i; j++) {
                        if (g_WorldMapRandomTimers[j] == value) {
                            break;
                        }
                    }
                } while (j < i);
            } else {
                value = rand() % g_WorldMapRandomTimerPeriod + 1;
            }
            g_WorldMapRandomTimers[i] = value;
        }
        g_WorldMapRandomTimerCountdown = g_WorldMapRandomTimerPeriod;
    }
    g_WorldMapExpiredTimerCount = 0;
    for (i = 0; i < g_WorldMapRandomTimerCount; i++) {
        if (--g_WorldMapRandomTimers[i] == 0) {
            g_WorldMapExpiredTimerCount++;
        }
    }
}
typedef struct {
    /* 0x0 */ s32 w0;
    /* 0x4 */ s32 w1;
    /* 0x8 */ s32 w2;
    /* 0xC */ s32 w3;
} WorldMapQuad; // Size: 0x10

typedef struct {
    /* 0x0 */ s16 h0;
    /* 0x2 */ s16 h2;
    /* 0x4 */ s16 h4;
    /* 0x6 */ s16 h6;
} WorldMapShortQuad; // Size: 0x8

// The world-map save slot at g_WorldMapSaveBlock: the actor block followed by the small
// world-map globals. Each field is the save slot's copy of the world-map global of the
// same name (WorldMapSaveState writes it, WorldMapLoadState restores it); the loader
// reads cameraRot and gridOrigin back as halfword blocks.
typedef struct {
    /* 0x0000 */ ActorBlock actors;
    /* 0x2000 */ WorldMapQuad scrollTarget;
    /* 0x2010 */ s32 playerAngle;
    /* 0x2014 */ s32 randomTimerPeriod;
    /* 0x2018 */ s32 randomTimerCount;
    /* 0x201C */ s32 randomTimerCountdown;
    /* 0x2020 */ SlotBlock randomTimers;
    /* 0x2040 */ UnkBlock trail;
    /* 0x22C0 */ s32 trailCursor;
    /* 0x22C4 */ s32 cameraRot;
    /* 0x22C8 */ s32 viewRotationZ;
    /* 0x22CC */ s32 cameraHeight;
    /* 0x22D0 */ s32 fadeLevel;
    /* 0x22D4 */ WorldMapQuad scroll;
    /* 0x22E4 */ s32 gridOrigin;
    /* 0x22E8 */ s32 unk22E8; // g_WorldMapGridTileRowOrigin
    /* 0x22EC */ WorldMapQuad cameraTarget;
} WorldMapSaveData; // Size: 0x22FC

void WorldMapSaveState(void) {
    WorldMapSaveData* pSave;

    pSave = (WorldMapSaveData*)g_WorldMapSaveBlock;
    pSave->actors = *(ActorBlock*)g_WorldMapSlots;
    pSave->scrollTarget.w0 = g_WorldMapScrollTargetX;
    pSave->scrollTarget.w1 = g_WorldMapScrollTargetY;
    pSave->scrollTarget.w2 = g_WorldMapScrollTargetZ;
    pSave->playerAngle = g_WorldMapPlayerAngle;
    pSave->randomTimerPeriod = g_WorldMapRandomTimerPeriod;
    pSave->randomTimerCount = g_WorldMapRandomTimerCount;
    pSave->randomTimerCountdown = g_WorldMapRandomTimerCountdown;
    pSave->randomTimers = *(SlotBlock*)g_WorldMapRandomTimers;
    pSave->trail = *(UnkBlock*)g_WorldMapTrail;
    pSave->trailCursor = g_WorldMapTrailCursor;
    pSave->cameraRot = g_WorldMapCameraRot;
    pSave->viewRotationZ = g_WorldMapViewRotationZ;
    pSave->cameraHeight = g_WorldMapCameraHeight;
    pSave->fadeLevel = g_WorldMapFadeLevel;
    pSave->gridOrigin = g_WorldMapGridOrigin;
    pSave->unk22E8 = g_WorldMapGridTileRowOrigin;
    pSave->scroll.w0 = g_WorldMapScrollX;
    pSave->scroll.w1 = g_WorldMapScrollY;
    pSave->scroll.w2 = g_WorldMapScrollZ;
    pSave->cameraTarget.w0 = g_WorldMapCameraTarget;
    pSave->cameraTarget.w1 = g_WorldMapCameraTargetY;
    pSave->cameraTarget.w2 = g_WorldMapCameraTargetZ;
}

extern s32 g_WorldMapPlayerPosition;

void WorldMapLoadState(void) {
    WorldMapSaveData* pSave;

    pSave = (WorldMapSaveData*)g_WorldMapSaveBlock;
    *(ActorBlock*)g_WorldMapSlots = pSave->actors;
    *(WorldMapQuad*)&g_WorldMapPlayerPosition = pSave->scrollTarget;
    *(WorldMapQuad*)&g_WorldMapScrollTargetX = pSave->scrollTarget;
    *(SlotBlock*)g_WorldMapRandomTimers = pSave->randomTimers;
    g_WorldMapPlayerAngle = pSave->playerAngle;
    g_WorldMapRandomTimerPeriod = pSave->randomTimerPeriod;
    g_WorldMapRandomTimerCount = pSave->randomTimerCount;
    g_WorldMapRandomTimerCountdown = pSave->randomTimerCountdown;
    *(UnkBlock*)g_WorldMapTrail = pSave->trail;
    *(WorldMapShortQuad*)&g_WorldMapCameraRot = *(WorldMapShortQuad*)&pSave->cameraRot;
    *(WorldMapQuad*)&g_WorldMapScrollX = pSave->scroll;
    *(WorldMapShortQuad*)&g_WorldMapGridOrigin = *(WorldMapShortQuad*)&pSave->gridOrigin;
    *(WorldMapQuad*)&g_WorldMapCameraTarget = pSave->cameraTarget;
    g_WorldMapTrailCursor = pSave->trailCursor;
    g_WorldMapCameraHeight = pSave->cameraHeight;
    g_WorldMapFadeLevel = pSave->fadeLevel;
}

extern s32 g_WorldMapPrimPacketIndex;
extern u16 g_WorldMapViewRotationY;
extern s16 g_NextMapRotationY;
extern void* g_WorldMapTilePrimBuffer;
extern void* g_WorldMapTilePrimBufferCopy;
extern void* g_WorldMapCompressedMapData;
extern void WorldMapWaitForQueuedLoads(void);
extern void WorldMapFreeSpritePrimBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapQueueTwoFileLoads(void);
extern int ArchiveDataSync(void);
extern void LZSSDecompress(void* pSrc, void* pDst);

void WorldMapEnterMenuMode(void) {
    RECT rect;
    void* pTemp;
    u_int size;
    s16* pOut;
    u16* pFlags;
    s32 i;

    i = 0;
    pFlags = &g_GameState.worldMapMenuActive;
    pOut = (s16*)(pFlags + 3);
    *pFlags = 1;
    g_NextMapRotationY = (g_WorldMapViewRotationY + 0x2000) & 0x3FFF;
    for (; i < 3; i++, pOut++) {
        *pOut = g_GameState.gearRide[i];
    }
    g_WorldMapSavedModeFlag = g_FieldPlayerWalkmeshMaterial;
    if ((s16)WorldMapGetTileHalfIndex((s32)&g_WorldMapScrollTargetX) == 4) {
        g_FieldPlayerWalkmeshMaterial = 1;
    }
    WorldMapWaitForQueuedLoads();
    WorldMapFreeSpritePrimBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreePrimTemplateBuffers();
    HeapFree(g_WorldMapTilePrimBuffer);
    HeapFree(g_WorldMapTilePrimBufferCopy);
    // probe the heap base to size the decompression buffer
    pTemp = HeapAlloc(4, 1);
    HeapFree(pTemp);
    pTemp = (void*)((u32)pTemp & 0xFFFFFF);
    size = (u32)pTemp + 0xFFE3B004;
    g_WorldMapDecompressBuffer = HeapAlloc(size, 1);
    WorldMapQueueTwoFileLoads();
    g_WorldMapSavedVramTexture = HeapAlloc(0x10000, 0);
    g_WorldMapSavedVramStrip = HeapAlloc(0xC800, 0);
    rect.x = 0x180;
    rect.y = 0x100;
    rect.w = 0x80;
    rect.h = 0x100;
    StoreImage(&rect, g_WorldMapSavedVramTexture);
    rect.x = 0;
    rect.y = 0x1B0;
    rect.w = 0x140;
    rect.h = 0x50;
    StoreImage(&rect, g_WorldMapSavedVramStrip);
    if (g_WorldMapPrimPacketIndex == 0) {
        rect.x = 0;
        rect.y = 0xD8;
        rect.w = 0x140;
        rect.h = 0xD8;
        MoveImage(&rect, 0, 0);
    }
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    WorldMapDrawFadeFrames(0x10, 0, 8, 2);
    do {
    } while (ArchiveDataSync() >= 2);
    LZSSDecompress(g_WorldMapCompressedMapData, g_WorldMapDecompressBuffer);
    HeapFree(g_WorldMapCompressedMapData);
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0xE0);
    DrawSync(0);
    ArchiveCdDataSync(0);
}
void WorldMapExitMenuMode(void) {
    RECT rect;
    void* pBuffer;
    s32 mode;
    s32 w;
    s32 h;

    w = 0x100;
    h = 0x140;
    HeapChangeCurrentUser(3, 0);
    ArchiveSetIndex(0x24, 0);
    HeapFree(g_MenuResourcesFile);
    HeapFree(g_WorldMapDecompressBuffer);
    WorldMapInitGfxEnv();
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_WorldMapReloadFileIndex), 1);
    g_WorldMapLoadedFileData = pBuffer;
    ArchiveReadFileToBuffer(g_WorldMapReloadFileIndex, pBuffer, 0, 0);
    WorldMapDrawFadeFrames(0x10, 0x80, -8, 2);
    rect.x = 0x2C0;
    rect.y = w;
    rect.w = h;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0);
    MoveImage(&rect, 0, 0xD8);
    ClearOTagR(*(u_long**)(g_WorldMapRenderContext + 0x70), 0x400);
    ArchiveCdDataSync(0);
    WorldMapLoadTextureClut();
    rect.x = 0x180;
    rect.y = w;
    rect.w = 0x80;
    rect.h = w;
    LoadImage(&rect, g_WorldMapSavedVramTexture);
    rect.x = 0;
    rect.y = 0x1B0;
    rect.w = h;
    rect.h = 0x50;
    LoadImage(&rect, g_WorldMapSavedVramStrip);
    DrawSync(0);
    HeapFree(g_WorldMapSavedVramTexture);
    HeapFree(g_WorldMapSavedVramStrip);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapAllocSpritePrimBuffers();
    SystemTransferPaletteToVRAM(0x130, 0x1E0);
    Vsync(0);
    ControllerResetState();
    mode = g_WorldMapSavedModeFlag;
    g_WorldMapMode4EntryTriggered = 0;
    g_GameState.worldMapMenuActive = 0;
    g_FieldPlayerWalkmeshMaterial = mode;
    WorldMapSyncGearRideActors();
}

void WorldMapSyncGearRideActors(void) {
    WorldMapActor* pActors;
    u8 ride;
    s32 count;
    s32 i;

    pActors = g_WorldMapSlots;
    for (i = 0; i < 3; i++) {
        ride = g_GameState.gearRide[i];
        if (ride != ((u8*)g_GameState.savedGearRide)[i * 2]) {
            if (ride == 0) {
                pActors[i + 1].x = pActors[i + 4].x;
                pActors[i + 1].y = pActors[i + 4].y;
                pActors[i + 1].z = pActors[i + 4].z;
                pActors[i + 1].trailDelay = pActors[i + 4].trailDelay;
            } else {
                ((s16*)((u8*)g_GameState.pad1838 + 0x122))[i * 3] = 0x400;
                pActors[i + 4].x = pActors[i + 1].x;
                pActors[i + 4].y = pActors[i + 1].y;
                pActors[i + 4].z = pActors[i + 1].z;
                pActors[i + 4].unk24 = 0;
                pActors[i + 4].trailDelay = pActors[i + 1].trailDelay;
            }
        }
    }
    count = 0;
    for (i = 0; i < 3; i++) {
        if (g_GameState.partyMembers[i] != 0xFF && g_GameState.gearRide[i] == 1) {
            count++;
        }
    }
    if (!(g_GameState.worldMapState & 0x4000)) {
        g_WorldMapMode = count != 0 ? 2 : 1;
    }
}

// 97.59% (540 bytes, 135/135 insns; retail 135): the limit walk, both loops and the memcpy are instruction-identical.
// 
// Residual: one basic block, registers only. retail keeps the limit pointer in a0 and zone in v1 then reuses them as pDst=a0 / pSrc=v1 / pEnd=a1; gcc 2.7.2 puts zone in a0 and the pointer in v1 instead, and the sum loop then runs with pDst=a1/pEnd=a0. Attempts measured (all 135 insns, all 97.59): pDst assigned before pSrc; zone=1 before pLimit=D_8009B578 and the reverse; row computed before pSrc instead of after; zone declared first vs last and as s16 (that one is worse: 138 insns, 92.25); u16* pLimit replaced by indexing D_8009B578[zone] directly; permuter 3 min -> PERMUTER_BETTER only via a dead 'if (arg1 || row) return 0; else return 0;' that buys 99.04% and is not shippable code.
// 
// Why it is unreachable from here (round 498, cc1 -da -dl -dg on this unit's own flags): zone is pseudo 74
// "used 8 times across 18 insns" and pLimit pseudo 75 "used 2 times across 8 insns", so the -dg order
// "... 74 ... 75 ..." processes zone 8 positions FIRST, and yet the dispositions hand zone $4 (a0) and pLimit
// $3 (v1); retail has the mirror. So on this body the processing order is not the hand-out order (77 is
// processed 2nd and takes $3, 79 16th and takes $4), and pLimit's 1.0 quotient against zone's 5.3 cannot be
// lifted by a spelling: pLimit is 1 def + 1 use and zone has 8 refs, both pinned by retail's own instructions.
// try_flags on this body: 2.6.0 96.21, 2.7.2-cdk 85.17, so the preset is right.
// 
// What DID close: (1) 'while (pCell[...])' anywhere in the pick loop is inverted by gcc; the goto scan (scanTile/foundTile) makes it a top test with a 'j' back edge and the 'a0 = a1' copy, and writing it on buf[i] (not a pointer) is what makes gcc strength-reduce i into a1. (2) pSrc = ... + 0x200 must be its own statement with the row offset added afterwards (row = zone << 4; pSrc += row), otherwise gcc folds (zone - 1) * 16 into +496 and drops the addiu -1.
/*
s32 func_80075E7C(s32 arg0, s32 arg1) {
    u8 buf[0x10];
    s32 zone;
    u16* pLimit;
    u8 cell;
    s32 pSrc;
    s32 pDst;
    s32 pEnd;
    s32 index;
    s32 sum;
    s32 pick;
    s32 row;
    s32 i;

    index = (s16)WorldMapGetTileGroup(arg0);
    if ((s16)WorldMapGetTileHalfIndex(arg0) == 4) {
        index = D_8009A3A0[index];
    }
    zone = 1;
    pLimit = D_8009B578;
    while (arg1 >= pLimit[zone]) {
        zone++;
    }
    zone--;
    sum = 0;
    pDst = (s32)buf;
    pSrc = (s32)g_WorldMapTileGroupData[index] + 0x200;
    row = zone << 4;
    pSrc += row;
    pEnd = pSrc + 0x10;
    do {
        *(u8*)pDst = *(u8*)pSrc;
        sum += *(u8*)pSrc;
        pSrc++;
        pDst++;
    } while (pSrc < pEnd);
    if (sum <= 0) {
        return 0;
    }
    pick = rand() % sum + 1;
    i = 0;
    do {
        pick--;
    scanTile:
        if ((cell = buf[i]) != 0) {
            goto foundTile;
        }
        i++;
        goto scanTile;
    foundTile:
        buf[i] = cell - 1;
    } while (pick > 0);
    memcpy(D_800658DC, g_WorldMapTileGroupData[index], 0x200);
    g_GameSelectedSkinIndex = i;
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075E7C);



INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076098);
void WorldMapFlushDrawAndCache(void) {
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    DrawSync(0);
    Vsync(0);
    FlushCache();
    ExitCriticalSection();
}
// One of the two swap buffers at g_WorldMapGfxEnvs: the draw and disp env a frame is
// staged through, and the two ordering table heads it fills.
typedef struct {
    /* 0x00 */ DRAWENV drawEnv; // Size: 0x5C
    /* 0x5C */ DISPENV dispEnv; // Size: 0x14
    /* 0x70 */ u_long* pOt[2];
} WorldMapGfxEnv; // Size: 0x78

extern int g_SpuChannelMask;
extern WorldMapGfxEnv g_WorldMapGfxEnvs[2];
extern u16 g_WorldMapPad1Released;
extern u16 g_WorldMapPad2Released;
extern u16 g_WorldMapPad1Pressed;
extern u16 g_WorldMapPad2Pressed;
extern u16 g_WorldMapPad1Held;
extern u16 g_WorldMapPad2Held;
extern u_short g_C1ButtonState;
extern u_short g_C1ButtonStatePressedOnce;
extern u_short g_C1ButtonStateReleased;
extern u_short g_C2ButtonState;
extern u_short g_C2ButtonStatePressedOnce;
extern u_short g_C2ButtonStateReleased;
extern int ControllerPopState(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);

void WorldMapWaitForButtonRelease(void) {
    RECT rect;
    DISPENV* pDispEnv;
    s32 savedValue;

    savedValue = g_SpuChannelMask;
    DrawSync(0);
    Vsync(0);
    if (g_WorldMapPrimPacketIndex == 0) {
        rect.x = 0;
        rect.y = 0xD8;
        rect.w = 0x140;
        rect.h = 0xD8;
        MoveImage(&rect, 0, 0);
    }
    pDispEnv = &g_WorldMapGfxEnvs[1].dispEnv;
    PutDispEnv(pDispEnv);
    PutDrawEnv((DRAWENV*)((u8*)pDispEnv - 0x5C));
    SoundMuteAllSpuChannels();
    do {
        DrawSync(0);
        Vsync(0);
        GraphicsDrawPauseLetters(0x88, 0x64);
        g_WorldMapPad2Pressed = 0;
        g_WorldMapPad2Released = 0;
        g_WorldMapPad2Held = 0;
        g_WorldMapPad1Pressed = 0;
        g_WorldMapPad1Released = 0;
        g_WorldMapPad1Held = 0;
        while (ControllerPopState() != 0) {
            g_WorldMapPad1Held |= g_C1ButtonState;
            g_WorldMapPad2Held |= g_C2ButtonState;
            g_WorldMapPad1Released |= g_C1ButtonStateReleased;
            g_WorldMapPad2Released |= g_C2ButtonStateReleased;
            g_WorldMapPad1Pressed |= g_C1ButtonStatePressedOnce;
            g_WorldMapPad2Pressed |= g_C2ButtonStatePressedOnce;
        }
    } while ((g_WorldMapPad1Released & 0x800) == 0);
    SoundEnableAllSpuChannels();
    DrawSync(0);
    Vsync(0);
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0xD8);
    PutDispEnv(&g_WorldMapGfxEnvs[g_WorldMapPrimPacketIndex].dispEnv);
    PutDrawEnv(&g_WorldMapGfxEnvs[g_WorldMapPrimPacketIndex].drawEnv);
    g_SpuChannelMask = savedValue;
}

extern int ControllerGetType(int controllerIndex);

void WorldMapWaitForControllerConnected(void) {
    RECT rect;
    DISPENV* pDispEnv;
    s32 savedValue;

    savedValue = g_SpuChannelMask;
    DrawSync(0);
    Vsync(0);
    if (g_WorldMapPrimPacketIndex == 0) {
        rect.x = 0;
        rect.y = 0xD8;
        rect.w = 0x140;
        rect.h = 0xD8;
        MoveImage(&rect, 0, 0);
    }
    pDispEnv = &g_WorldMapGfxEnvs[1].dispEnv;
    PutDispEnv(pDispEnv);
    PutDrawEnv((DRAWENV*)((u8*)pDispEnv - 0x5C));
    SoundMuteAllSpuChannels();
    do {
        DrawSync(0);
        Vsync(0);
        GraphicsDrawPauseLetters(0x88, 0x64);
        g_WorldMapPad2Pressed = 0;
        g_WorldMapPad2Released = 0;
        g_WorldMapPad2Held = 0;
        g_WorldMapPad1Pressed = 0;
        g_WorldMapPad1Released = 0;
        g_WorldMapPad1Held = 0;
        while (ControllerPopState() != 0) {
            g_WorldMapPad1Held |= g_C1ButtonState;
            g_WorldMapPad2Held |= g_C2ButtonState;
            g_WorldMapPad1Released |= g_C1ButtonStateReleased;
            g_WorldMapPad2Released |= g_C2ButtonStateReleased;
            g_WorldMapPad1Pressed |= g_C1ButtonStatePressedOnce;
            g_WorldMapPad2Pressed |= g_C2ButtonStatePressedOnce;
        }
    } while (ControllerGetType(0) == 0);
    SoundEnableAllSpuChannels();
    DrawSync(0);
    Vsync(0);
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0xD8);
    PutDispEnv(&g_WorldMapGfxEnvs[g_WorldMapPrimPacketIndex].dispEnv);
    PutDrawEnv(&g_WorldMapGfxEnvs[g_WorldMapPrimPacketIndex].drawEnv);
    g_SpuChannelMask = savedValue;
}
void WorldMapPlayBgmFromBuffer(u8* pSrc, s32 index) {
    u8* pBuffer;

    SoundStopAllAudioManagers();
    SoundDestroyAudioManager(g_SoundAudioManager);
    memcpy(g_SoundBankLoadBuffer, pSrc, ArchiveDecodeAlignedSize(index));
    g_SoundAudioManager = SoundCreateAudioManagerOnHeap(g_SoundBankLoadBuffer);
    SoundPlayAudioManager(g_SoundAudioManager, 0x7F, 0);
}
void WorldMapInterpolateQuadraticVector(s32 t, s16* pRow0, s16* pRow1, s16* pRow2, s32* pOut) {
    s32 s;
    s32 iz;
    s32 zv;
    s32 xs;

    s = 0x1000 - t;
    iz = ((s * s) << 3) >> 12;
    zv = ((s * t) >> 8) + 0x8000;
    xs = ((t * t) << 3) >> 12;
    pOut[0] = pRow0[0] * iz + pRow1[0] * zv + pRow2[0] * xs;
    pOut[1] = pRow0[1] * iz + pRow1[1] * zv + pRow2[1] * xs;
    pOut[2] = pRow0[2] * iz + pRow1[2] * zv + pRow2[2] * xs;
}
void WorldMapDecompressMapData(void) {
    s32* pHeader; // block offsets, relative to the header's own address
    u8* pBase;

    pHeader = (s32*)g_WorldMapDecompressedMapData;
    g_WorldMapDecompressedMapData = (s32)LZSSHeapDecompress(pHeader, 0);
    HeapFree(pHeader);
    pBase = (u8*)g_WorldMapDecompressedMapData;
    pHeader = (s32*)pBase;
    g_WorldMapModelData = (s32)(pBase + pHeader[2]);
    g_WorldMapModelRelocs = (s32)(pBase + pHeader[3]);
    g_WorldMapSpriteSetTable = (s32)(pBase + pHeader[4]);
    g_WorldMapPrimBlocks = (s32)(pBase + pHeader[5]);
    g_WorldMapSpriteGroups = (s32)(pBase + pHeader[6]);
    g_WorldMapTileAnimSet = (WorldMapAnimSet*)(pBase + pHeader[8]);
    g_WorldMapFrameAnimSet = (WorldMapAnimSet*)(pBase + pHeader[9]);
}

int WorldMapInitNop(void) {
    return 1;
}
