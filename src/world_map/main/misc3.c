#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "system/sound.h"

// the map display buffer copied into VRAM when a mode initialises, and the VRAM
// position it is copied to (same defines as misc4.c)
#define WORLD_MAP_SCREEN_WIDTH 320
#define WORLD_MAP_SCREEN_HEIGHT 216
#define WORLD_MAP_SCREEN_VRAM_X 0x2C0
#define WORLD_MAP_SCREEN_VRAM_Y 0x100

// the archive directory this overlay's files are read through (same define as misc4.c)
#define WORLD_MAP_ARCHIVE_DIRECTORY_INDEX 0x24

// the grid angle advances by this step every rendered frame (same define as misc1.c)
#define WORLD_MAP_GRID_ANGLE_STEP 64

// an angle is a 4096-step turn counter, wrapped back into it (same define as misc13.c)
#define WORLD_MAP_ANGLE_MASK 0xFFF

// half of the 4096-step turn, added to a view rotation to flip its facing
#define WORLD_MAP_ANGLE_HALF_TURN 0x800

// A scroll step under this snaps the player straight onto the target instead of moving.
#define WORLD_MAP_STEP_SNAP_MAX 512

// A view rotation or camera height step under this snaps straight onto the target instead of
// moving; its steps are the /8 ones, so this is not the scroll path's limit above.
#define WORLD_MAP_VIEW_STEP_SNAP_MAX 64

// the slot target at 0x28, which the pad nudges and the 0x38 view rotation interpolates
// toward, is clamped into this range
#define WORLD_MAP_SLOT_TARGET_MIN (-0x180000)
#define WORLD_MAP_SLOT_TARGET_MAX 0x18000

// One slot of the pool g_WorldMapSlots (stride 0x80), partial view: only the script
// cursor and the wait countdown are used here. The cursor is halfword
// addressed; 0x50 of a slot is a pointer to the current command word.
typedef struct {
    /* 0x00 */ u8 unk0[0x22];
    /* 0x22 */ s16 waitFrames;
    /* 0x24 */ u8 unk24[0x2C];
    /* 0x50 */ s16* pc;
    /* 0x54 */ u8 unk54[0x2C];
} WorldMapScriptSlot; // Size: 0x80

// The world map frame render context g_WorldMapRenderContext points at: the two ordering
// table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long* pOt[2];
} WorldMapRenderContext; // partial view

extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;

extern void WorldMapDeactivateSpriteGroup(s32 arg0);
extern void WorldMapClearGroupEntryFlag(s32 arg0);
extern void WorldMapStartSlot(s32 arg0, s32 arg1);
extern s32* g_SoundAudioManager;

extern void SoundSetAudioManagerVolumeWithFade(s32* pManager);
extern void WorldMapInitSpriteGroupSprites(s32 arg0, void* arg1, s32 arg2);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern void SoundFadeElementVolumeById(s32 arg0, s32 arg1, s32 arg2);

extern u8 g_WorldMapPlayerPosition[];
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapCameraHeight;
extern SoundFile* g_MenuSedsFile;

extern WorldMapScriptSlot* g_WorldMapSlots;
extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapPathFrame;
extern s16 g_WorldMapSlotUpdateFlags;
extern s32 g_WorldMapScrollX;
extern s32 g_WorldMapCameraTarget;
extern WorldMapRenderContext* g_WorldMapRenderContext;
extern s32 g_WorldMapGridAngle;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_WorldMapViewRotationY;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 g_WorldMapExitFlag;

extern void func_8009932C(u_long* pOt0, u_long* pOt1, s32* pState);

extern void WorldMapBuildRotationMatrix(s32* p);
extern void func_80097244(s32* p);
extern void WorldMapEmitSpriteTrailEntries(void);
extern void func_80089C78(void);
extern void WorldMapDrawGridTiles(void);
extern void func_800848F4(void);
extern void WorldMapUpdateSlotGridOrigin(s32* p);
extern void WorldMapBuildTileIdGrid(s32* p);
extern void WorldMapWaitForLoadSlotFree(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32* p);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void WorldMapFreeSpriteSets(void);
extern void WorldMapFreeSpritePrimBuffers(void);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeGridImages(void);
extern void WorldMapFreeSlotPool(void);
extern void HeapFree(void* pMem);

typedef s32 (*WorldMapScriptHandler)(WorldMapScriptSlot* pSlot, s32 arg1, s16 arg2, s16 arg3);
extern WorldMapScriptHandler g_WorldMapScriptOpHandlers[];

// World map player (stride 0x80), the scroll and height fields
// WorldMapApproachViewRotation, func_80076FA8 and WorldMapApproachHeight touch. Same layout
// as misc12.c's local WorldMapPlayer.
typedef struct {
    /* 0x00 */ u8 unk0[0x28];
    /* 0x28 */ s32 x; // world scroll position
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 w; // fourth word of the x/y/z quad, copied whole with it
    /* 0x38 */ s32 rotX; // 12.12 fixed point: >> 12 is the angle in 0..0xFFF
    /* 0x3C */ s32 rotY;
    /* 0x40 */ s32 rotZ;
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ s32 targetRotX; // where rotX/Y/Z interpolate to
    /* 0x54 */ s32 targetRotY;
    /* 0x58 */ s32 targetRotZ;
    /* 0x5C */ s32 cameraHeight; // g_WorldMapCameraHeight interpolates to this
    /* 0x60 */ u8 unk60[0x20];
} WorldMapPlayer; // Size: 0x80, the stride of g_WorldMapSlots

// Four-word position record; misc12.c copies the same bytes through a Quad.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapPos4; // Size: 0x10

// Scroll delta buffer: the wrapped xyz offset at 0x00 and its 1/8 step at 0x10.
typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 pad0C;
    /* 0x10 */ s32 stepX;
    /* 0x14 */ s32 stepY;
    /* 0x18 */ s32 stepZ;
} WorldMapScrollDelta; // Size: 0x1C

// Eight name bytes swapped through the scratchpad by func_8007756C; the type
// is byte-aligned so gcc copies it with lwl/lwr.
typedef struct {
    /* 0x0 */ u8 name[8];
} WorldMapName8; // Size: 0x8


extern void WorldMapWrapDelta(WorldMapScrollDelta* pDelta);
extern void func_80096F18(s16* pOut, s32* pSrc, s32 nHeight, u16* pAngles);
extern s32 g_WorldMapFadeLevel;
extern s32 D_8009BD48;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapScrollTargetX;
// volatile: retail re-reads Y and Z in the delta block; without it cc1 reuses the values
// it loaded for the compare and the body comes out four instructions short.
extern volatile s32 g_WorldMapScrollTargetY;
extern volatile s32 g_WorldMapScrollTargetZ;
extern s32 g_WorldMapScrollZ;
extern s16 g_WorldMapCameraRot;
// Second declarations with an asm label: an address taken from a scalar global is a
// rematerialisable constant, so cc1 re-forms lui/addiu at each use; the array form makes
// it a value that survives a call in a saved register, and gives a store's base a register
// instead of the $at macro form.
extern s32 g_WorldMapScrollXTable[] __asm__("g_WorldMapScrollX");
extern s32 g_WorldMapScrollZTable[] __asm__("g_WorldMapScrollZ");
extern s32 g_WorldMapScrollTargetXTable[] __asm__("g_WorldMapScrollTargetX");
extern s16 g_WorldMapCameraRotTable[] __asm__("g_WorldMapCameraRot");
extern s16 g_WorldMapViewRotationYTable[] __asm__("g_WorldMapViewRotationY");
extern s32 g_WorldMapPathFrameTable[] __asm__("g_WorldMapPathFrame");
extern s32 g_WorldMapCameraTargetTable[] __asm__("g_WorldMapCameraTarget");
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;

s32 WorldMapUpdateFrame(void) {
    s32* pState;

    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildRotationMatrix(&g_WorldMapPathFrame);
    } else {
        func_80097244(&g_WorldMapPathFrame);
    }
    WorldMapEmitSpriteTrailEntries();
    func_80089C78();
    WorldMapDrawGridTiles();
    func_800848F4();
    WorldMapUpdateSlotGridOrigin(&g_WorldMapScrollX);
    if (g_WorldMapSlotUpdateFlags != 0) {
        WorldMapBuildTileIdGrid(&g_WorldMapCameraTarget);
        WorldMapWaitForLoadSlotFree();
        func_80098CC0();
    }
    pState = &g_WorldMapCameraTarget;
    func_800983A0(pState);
    func_8009932C(g_WorldMapRenderContext->pOt[0], g_WorldMapRenderContext->pOt[1], pState);
    g_WorldMapGridAngle = g_WorldMapGridAngle + WORLD_MAP_GRID_ANGLE_STEP;
    func_80073B04();
    func_800737EC();
    func_80086798();
    return 1;
}

s32 WorldMapRunSlotScript(s32 index) {
    WorldMapScriptSlot* pSlot = &g_WorldMapSlots[index];
    s16* pCmd;
    s32 cmd;
    s32 n = 0;
    s32 pad[2]; // unused: reproduces retail's 0x28 frame

    do {
        pCmd = pSlot->pc += n;
        cmd = *(s32*)pCmd;
        n = g_WorldMapScriptOpHandlers[cmd & 0xFFFF](pSlot, cmd >> 16, pCmd[2], pCmd[3]);
    } while (n != 0);
    return 1;
}

s32 WorldMapScriptOp00End(void) {
    g_WorldMapLoopActive = 0;
    g_WorldMapAreaMode = 0;
    return 0;
}

s32 WorldMapScriptOp01WaitFrames(WorldMapScriptSlot* pSlot, s32 arg1) {
    s16 timer = pSlot->waitFrames;

    if (timer == 0) {
        pSlot->waitFrames = arg1;
        return 0;
    }
    timer--;
    pSlot->waitFrames = timer;
    return (timer < 1) << 1;
}

s32 WorldMapScriptOp02StartSlot(s32 arg0, s32 arg1, s32 arg2) {
    WorldMapStartSlot(arg1, arg2);
    return 4;
}

s32 WorldMapScriptOp03SetPosition(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(s32*)g_WorldMapPlayerPosition = arg1 << 12;
    g_WorldMapPlayerPositionY = arg2 << 12;
    g_WorldMapPlayerPositionZ = arg3 << 12;
    return 4;
}

s32 WorldMapScriptOp04SetScratchPosition(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(u16*)0x1F8000A0 = arg1;
    *(u16*)0x1F8000A2 = arg2;
    *(u16*)0x1F8000A4 = arg3;
    return 4;
}

s32 WorldMapScriptOp05ActivateSpriteGroup(s32 arg0, s32 arg1) {
    WorldMapInitSpriteGroupSprites(arg1, (void*)0x1F8000A0, 0);
    return 2;
}

s32 WorldMapScriptOp06DeactivateSpriteGroup(s32 arg0, s32 arg1) {
    WorldMapDeactivateSpriteGroup(arg1);
    return 2;
}

s32 WorldMapScriptOp07ClearGroupEntries(s32 arg0, s32 arg1) {
    WorldMapClearGroupEntryFlag(arg1);
    return 2;
}

s32 WorldMapScriptOp08FadeMasterVolume(void) {
    SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager);
    return 4;
}

s32 WorldMapScriptOp09PlaySound(s32 arg0, s32 arg1) {
    SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | arg1);
    return 2;
}

s32 WorldMapScriptOp10FadeSound(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    SoundFadeElementVolumeById((g_MenuSedsFile->sedId << 16) | arg1, arg2, arg3);
    return 4;
}

s32 WorldMapScriptOp11SetDrawMode(s32 arg0, s32 arg1, s32 arg2) {
    g_WorldMapTpageAbr = arg1;
    g_WorldMapDrawMode = arg2;
    return 4;
}

void WorldMapApproachViewRotation(WorldMapPlayer* pPlayer, WorldMapScrollDelta* pDelta) {
    s16 rotX;
    s16 rotY;
    s16 rotZ;

    if ((pPlayer->rotX != pPlayer->targetRotX) | (pPlayer->rotY != pPlayer->targetRotY) |
        (pPlayer->rotZ != pPlayer->targetRotZ)) {
        pDelta->x = pPlayer->targetRotX - pPlayer->rotX;
        pDelta->y = pPlayer->targetRotY - pPlayer->rotY;
        pDelta->z = pPlayer->targetRotZ - pPlayer->rotZ;
        WorldMapWrapDelta(pDelta);
        pDelta->stepX = pDelta->x >> 3;
        pDelta->stepY = pDelta->y >> 3;
        pDelta->stepZ = pDelta->z >> 3;
        if ((pDelta->stepX < 0 ? -pDelta->stepX : pDelta->stepX) < WORLD_MAP_VIEW_STEP_SNAP_MAX) {
            pPlayer->rotX = pPlayer->targetRotX;
        } else {
            pPlayer->rotX += pDelta->stepX;
        }
        if ((pDelta->stepY < 0 ? -pDelta->stepY : pDelta->stepY) < WORLD_MAP_VIEW_STEP_SNAP_MAX) {
            pPlayer->rotY = pPlayer->targetRotY;
        } else {
            pPlayer->rotY += pDelta->stepY;
        }
        if ((pDelta->stepZ < 0 ? -pDelta->stepZ : pDelta->stepZ) < WORLD_MAP_VIEW_STEP_SNAP_MAX) {
            pPlayer->rotZ = pPlayer->targetRotZ;
        } else {
            pPlayer->rotZ += pDelta->stepZ;
        }
    }
    g_WorldMapCameraRot = pPlayer->rotX >> 12;
    // shortcut: the do/while block isolates this store so cc1 schedules it as a
    // unit; without it gcc hoists the next load one slot early and is 3 insns short
    do { g_WorldMapViewRotationY = pPlayer->rotY >> 12; } while (0);
    g_WorldMapViewRotationZ = pPlayer->rotZ >> 12;

}


void WorldMapApproachHeight(WorldMapPlayer* pPlayer) {
    s32 cur = pPlayer->cameraHeight;
    s32 idx;

    if (cur != g_WorldMapCameraHeight) {
        idx = (cur - g_WorldMapCameraHeight) >> 3;
        if ((idx < 0 ? -idx : idx) < WORLD_MAP_VIEW_STEP_SNAP_MAX) {
            g_WorldMapCameraHeight = cur;
        } else {
            g_WorldMapCameraHeight = g_WorldMapCameraHeight + idx;
        }
    }
}
void func_80076FA8(WorldMapPlayer* pPlayer, WorldMapScrollDelta* pDelta) {
    if ((pPlayer->x != g_WorldMapScrollTargetXTable[0]) | (pPlayer->y != g_WorldMapScrollTargetY) |
        (pPlayer->z != g_WorldMapScrollTargetZ)) {
        pDelta->x = g_WorldMapScrollTargetXTable[0] - pPlayer->x;
        pDelta->y = g_WorldMapScrollTargetY - pPlayer->y;
        pDelta->z = g_WorldMapScrollTargetZ - pPlayer->z;
        WorldMapWrapDelta(pDelta);
        pDelta->stepX = pDelta->x >> 3;
        pDelta->stepY = pDelta->y >> 3;
        pDelta->stepZ = pDelta->z >> 3;
        if ((pDelta->stepX < 0 ? -pDelta->stepX : pDelta->stepX) < WORLD_MAP_STEP_SNAP_MAX) {
            g_WorldMapScrollXTable[0] += pDelta->x;
            pPlayer->x = g_WorldMapScrollTargetXTable[0];
        } else {
            g_WorldMapScrollXTable[0] += pDelta->stepX;
            pPlayer->x += pDelta->stepX;
        }
        if ((pDelta->stepY < 0 ? -pDelta->stepY : pDelta->stepY) < WORLD_MAP_STEP_SNAP_MAX) {
            pPlayer->y = g_WorldMapScrollTargetY;
        } else {
            pPlayer->y += pDelta->stepY;
        }
        if ((pDelta->stepZ < 0 ? -pDelta->stepZ : pDelta->stepZ) < WORLD_MAP_STEP_SNAP_MAX) {
            g_WorldMapScrollZTable[0] += pDelta->z;
            pPlayer->z = g_WorldMapScrollTargetZ;
        } else {
            g_WorldMapScrollZTable[0] += pDelta->stepZ;
            pPlayer->z += pDelta->stepZ;
        }
    }
    *(WorldMapPos4*)&g_WorldMapCameraTarget = *(WorldMapPos4*)&pPlayer->x;
}
s32 WorldMapApproachValue(s32 arg0, s32 arg1, s32 arg2) {
    s32 delta;
    s32 limit;

    if (arg0 == arg1) {
        return arg0;
    }
    delta = arg1 - arg0;
    if (delta < 0) {
        delta = -delta;
    }
    limit = arg2;
    if (limit < 0) {
        // shortcut: the no-op redefinitions keep cse from negating the parameter
        // (negu v1,a2) instead of the copy; without them this is 99.67%
        arg2++;
        arg2--;
        limit = -limit;
    }
    if (delta < limit) {
        arg0 = arg1;
    } else {
        arg0 += arg2;
    }
    return arg0;
}
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapMode4EntryTriggered;
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void WorldMapDrawFadeFrames(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapDecompressMapTables(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern s32 ArchiveCdDataSync(s32 arg0);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadClutAndTpageIds(void);
extern void func_800736DC(void);
extern void WorldMapRelocateMapEntryOffsets(void);
extern void WorldMapAllocSpritePrimBuffers(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void SoundAddSedsEntry(void* pSeds);
extern s32 ArchiveSetIndex(s32 arg0, s32 arg1);
extern void WorldMapAddSlot(void* p0, void* p1);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern void WorldMapInitSlotAngleStep8(void);
extern void WorldMapUpdateSlotRotationMatrices(void);
extern void WorldMapModeInitNop(void);
extern void WorldMapUpdateFrameWithSlotProjection(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void WorldMapInitActorAtScrollPosition(void);
extern void func_800783E8(void);
extern void WorldMapResetActorSlotWithSound(void);
extern void func_80077E68(void);
extern void WorldMapSetGridPosition(void* p);
extern void WorldMapUpdateLoadQueue(void);
extern s32 WorldMapGetQueuedLoadCount(void);
extern s32 func_8007756C(s32 arg0);
extern s32 WorldMapUpdateFrameWithScrollInput(s32 arg0);

void WorldMapInitializeMode(void) {
    RECT rect;
    s32* pPos;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = WORLD_MAP_SCREEN_WIDTH;
    rect.h = WORLD_MAP_SCREEN_HEIGHT;
    MoveImage(&rect, WORLD_MAP_SCREEN_VRAM_X, WORLD_MAP_SCREEN_VRAM_Y);
    DrawSync(0);
    WorldMapDrawFadeFrames(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapTables();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 4;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    pPos = (s32*)g_WorldMapPlayerPosition;
    *pPos = 0x7702000;
    g_WorldMapPlayerPositionY = 0xFFD00000;
    g_WorldMapPlayerPositionZ = 0x27C0000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    WorldMapLoadClutAndTpageIds();
    func_800736DC();
    WorldMapRelocateMapEntryOffsets();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveSetIndex(WORLD_MAP_ARCHIVE_DIRECTORY_INDEX, 0);
    WorldMapSetGridPosition(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot((void*)WorldMapModeInitFadeRect, (void*)func_800925A0);
    WorldMapAddSlot((void*)func_8007756C, (void*)WorldMapUpdateFrameWithScrollInput);
    WorldMapAddSlot((void*)WorldMapInitSlotAngleStep8, (void*)WorldMapUpdateSlotRotationMatrices);
    WorldMapAddSlot((void*)WorldMapModeInitNop, (void*)WorldMapUpdateFrameWithSlotProjection);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapAllocSpritePrimBuffers();
    WorldMapInitRandomTimers();
    WorldMapInitSpriteGroupSprites(0xE, NULL, 0);
}

s32 WorldMapExitMode(void) {
    u16 v;

    WorldMapFreeSpriteSets();
    WorldMapFreeSpritePrimBuffers();
    WorldMapFreeTileVertexBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreeTileAnimEntries();
    WorldMapFreeFrameAnimEntries();
    WorldMapFreeSpriteGroupTable();
    WorldMapFreePrimTemplateBuffers();
    WorldMapFreeGridImages();
    HeapFree(g_WorldMapWorkBuffer1);
    HeapFree(g_WorldMapWorkBuffer2);
    HeapFree(g_WorldMapTilePrimBuffer);
    HeapFree(g_WorldMapTilePrimBufferCopy);
    HeapFree(g_WorldMapDecompressedMapData);
    WorldMapFreeSlotPool();
    v = *(u16*)&g_WorldMapViewRotationY;
    g_NextMapId = 0x11;
    g_NextMapFlags = 7;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
    return 1;
}
// Best attempt: 97.57%, 93 instructions / 372 bytes = retail's exact size. ONE residual left: a
//    pure store-order tie in the camera-target triple, 2 differing lines.
//    What closed the OLD residual (the ViewRotationZ store, which used to land six slots late, just
//    before the FadeLevel store): spelling the player slot as a BYTE pointer built from the index
//    and reading it with cast offsets,
//        u8 *pSlot = (u8 *)g_WorldMapSlots + (arg0 << 7);
//        *(s32 *)(pSlot + 0x28) = -0x80000;
//    That re-ranks sched2's ready list for the whole early block and 93.22 -> 97.57 in one edit.
//    It is the spelling that matters, not the arithmetic: the same body with a typed pointer
//    (`WorldMapPlayer *pPlayer = (WorldMapPlayer *)((u8 *)g_WorldMapSlots + (arg0 << 7));` and
//    pPlayer->x / ->y / ->rotX) emits the same instructions but is back to 93.22, and
//    `&((WorldMapPlayer *)g_WorldMapSlots)[arg0]` is 93.22 too. Same family of re-rank as the
//    sibling WorldMapUpdateFrameWithScrollInput, which is matched on the `(u8 *)g_WorldMapSlots +
//    (arg0 << 7)` spelling.
//    Left: retail emits the triple's stores as x, Z, Y; this body emits Z, x, Y (same three
//    instructions, same registers v0/v1/a0, same loads). The scheduler emits the LAST source
//    statement's symbol store first and puts the a1 store between the two symbol stores, and it
//    does that for every source order measured: x,Y,Z -> Z,x,Y (97.57) [BEST]; x,Z,Y -> Y,x,Z
//    (97.25, and the loads rotate too); three s32 locals loaded x,Y,Z then stored Z,x,Y -> the x
//    store ends up in the jal delay slot (92.09); locals loaded x,Y,Z and stored x,Z,Y -> 92.57
//    with the load registers rotated (v1/a0/v0). An x-only local changes nothing (97.57, same two
//    lines). So the a1 store cannot be scheduled ahead of the first symbol store from C - the same
//    class the old 93.22% note called a scheduling tie, now down to one swapped pair.
//    Also measured this round: a `do { } while (0);` between the CameraHeight and ViewRotationZ
//    stores is 88.04 (it drags the pathFrame la up and sinks the Height store); a cc1 probe on the
//    unit's own .i shows the UNSCHEDULED stream is the source order, -fno-schedule-insns alone
//    leaves the ViewRotationZ store after the FadeLevel store, and -fno-schedule-insns2 is the one
//    that keeps it next to the CameraHeight store - so sched2 is the pass that moves it, and the
//    byte-pointer spelling above is what re-ranks it.
//    Older measurements, still valid: gcc-2.7.2 93.22 in all eight -O2/-O3 x -G4/-G8 x 3000/r8000
//    cells, 2.6.0-psx 90.19, 2.7.2-cdk 68.2 at every -O; a permuter run from the 93.22 body found
//    nothing (no run has been made from the 97.57 body).
//    Round 509 measured the store ORDER as a fixed law on this body: the three camera-target stores
//    come out as a right-rotation of their source order, every time - source x,Y,Z -> Z,x,Y (97.57),
//    source x,Z,Y -> Y,x,Z (97.25, the Y/Z loads swap too, so the loads track source order and the
//    stores do not), source y,z-local then x,Z,Y -> Y,x,Z (96.82, and the x load is not hoisted so
//    it lands in the jal delay slot). So retail's x,Z,Y output needs source order Z,Y,x, and that
//    body (x,y,z into locals first, stores Z,Y,x) puts the x store IN the jal delay slot for 91.66 -
//    with the x load re-formed after the Z store instead of hoisted. The empty-do-while store
//    boundary (lever h-an-empty-do-while-pins-a-run-of-stores) does pin the x store FIRST when it
//    sits between it and the Y/Z stores, but it also stops the Y/Z/height loads hoisting above it:
//    88.60 alone, and 88.11 with the four values read into locals above the barrier (the barrier
//    re-ranks the whole head: slots base and the pathFrame la move too). That is the third measured
//    cost of a barrier here, after the 88.04 of the one between the CameraHeight and ViewRotationZ
//    stores. Not worth a fourth: the residual stands at one swapped pair, 91/93 insns identical.
/*
s32 func_8007756C(s32 arg0) {
    u8 *pSlot = (u8 *)g_WorldMapSlots + (arg0 << 7);
    s16 *pRot = g_WorldMapCameraRotTable;
    u8 *pName = (u8 *)g_WorldMapPathFrameTable;

    *pRot = -0x80;
    g_WorldMapViewRotationY = 0x200;
    g_WorldMapCameraHeight = 0x400000;
    g_WorldMapViewRotationZ = 0;
    *(s32 *)(pSlot + 0x28) = -0x80000;                                // 0x28 x = -0x80000
    g_WorldMapFadeLevel = 0x78;
    *(s32 *)(pSlot + 0x2C) = g_WorldMapViewRotationYTable[0] << 12;   // 0x2C y
    *(WorldMapPos4 *)(pSlot + 0x38) = *(WorldMapPos4 *)(pSlot + 0x28);  // 0x38 rot <- 0x28
    g_WorldMapRotationMatrixValid = 0;
    *g_WorldMapCameraTargetTable = *(s32 *)g_WorldMapPlayerPosition;
    g_WorldMapCameraTargetY = g_WorldMapPlayerPositionY;
    g_WorldMapCameraTargetZ = g_WorldMapPlayerPositionZ;
    func_80096F18((s16 *)pName, g_WorldMapCameraTargetTable, g_WorldMapCameraHeight, (u16 *)pRot);
    *(WorldMapName8 *)0x1F8000A0 = *(WorldMapName8 *)pName;
    *(WorldMapName8 *)pName = *(WorldMapName8 *)&D_8009BD48;
    *(WorldMapName8 *)&D_8009BD48 = *(WorldMapName8 *)0x1F8000A0;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_8007756C);




extern u16 g_WorldMapPad1Released;
extern u16 g_WorldMapPad1Held;
extern s32 g_WorldMapProjectionDistance;
extern s32 D_8009BD48;

// Best attempt: 97.71%, 158 instructions / 632 bytes; retail is 157 / 628. ONE instruction left.
//    What moved it here from the old 94.04% park: the ASM-LABEL ARRAY spellings
//    `pRot = g_WorldMapCameraRotTable;` and `pName = (WorldMapName8*)g_WorldMapPathFrameTable;`
//    assigned together mid-block (the scalar `&g_WorldMapCameraRot` / `&g_WorldMapPathFrame`
//    forms rematerialise the address at every use). That bought retail's a3 hold of
//    &g_WorldMapCameraRot (`sh v0,0(a3)`, not `lui at`), retail's s0 hold of &g_WorldMapPathFrame
//    across the jal (`move a0,s0` in the slot instead of three lui/addiu pairs) and retail's
//    s1 = 0x1F800000 for the scratchpad, i.e. the whole rotation the old note called
//    unreachable, plus the frame (retail's -0x20 with s0/s1/ra).
//    The single residual: retail's `lui a2,%hi(g_WorldMapCameraHeight)` sits in the load-delay
//    slot of `lw v0,56(a1)` with `lw a2` after it; our compile leaves the `nop` there and
//    schedules the pair after `la a1,g_WorldMapCameraTarget` (4 slots later). Tried on this body,
//    all byte-identical to each other: the height in a local read as a statement before the two
//    rotation stores; the same read through `*(volatile s32 *)&g_WorldMapCameraHeight`; the
//    CameraTarget address in an `s32 *pTarget` local. A rotX/rotY pair of locals is worse
//    (97.32: it also renames the clamp registers a0/v1). Presets: gcc-2.6.0-psx 97.39,
//    gcc-2.7.2-cdk-psx and -O3 both compile-fail on this unit.
//    Correct in the body: the four pad tests reload g_WorldMapPad1Held (u16), 0x8000 / -0x8000 /
//    0x8004 / 0x2008 are the literals that give li + addu, the clamp is one shared
//    `sw v1,0x28(a1)` for both bounds, the compare is `(unk38 != unk28) | (unk3C != unk2C)`
//    (xor/sltu/or with the first delta in the branch delay slot), and the 0x1F8000A0 name swap is
//    an 8-byte struct move through the `u8 *` scratch base (lwl/lwr + swl/swr).
s32 WorldMapUpdateFrameWithScrollInput(s32 arg0) {
    u8* pScratch = (u8*)0x1F800000;
    u8* pSlot = (u8*)g_WorldMapSlots + (arg0 << 7);
    WorldMapName8* pName;
    s32* pTarget;
    s32 nPos;

    if (g_WorldMapPad1Released & 0x40) {
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
    }
    if (g_WorldMapPad1Held & 0x1000) {
        *(s32*)(pSlot + 0x28) += 0x8000;
    }
    if (g_WorldMapPad1Held & 0x4000) {
        *(s32*)(pSlot + 0x28) += -0x8000;
    }
    nPos = *(s32*)(pSlot + 0x28);
    if (nPos < WORLD_MAP_SLOT_TARGET_MIN) {
        *(s32*)(pSlot + 0x28) = WORLD_MAP_SLOT_TARGET_MIN;
    } else if (nPos > WORLD_MAP_SLOT_TARGET_MAX) {
        *(s32*)(pSlot + 0x28) = WORLD_MAP_SLOT_TARGET_MAX;
    }
    if (g_WorldMapPad1Held & 0x8004) {
        *(s32*)(pSlot + 0x2C) += 0xFFFF0000;
    }
    if (g_WorldMapPad1Held & 0x2008) {
        *(s32*)(pSlot + 0x2C) += 0x10000;
    }
    if ((*(s32*)(pSlot + 0x38) != *(s32*)(pSlot + 0x28)) |
        (*(s32*)(pSlot + 0x3C) != *(s32*)(pSlot + 0x2C))) {
        *(s32*)pScratch = *(s32*)(pSlot + 0x28) - *(s32*)(pSlot + 0x38);
        *(s32*)(pScratch + 4) = *(s32*)(pSlot + 0x2C) - *(s32*)(pSlot + 0x3C);
        *(s32*)(pSlot + 0x38) += *(s32*)pScratch >> 3;
        *(s32*)(pSlot + 0x3C) += *(s32*)(pScratch + 4) >> 3;
    }
    pName = (WorldMapName8*)g_WorldMapPathFrameTable;
    pTarget = g_WorldMapCameraTargetTable;
    *g_WorldMapCameraRotTable = *(s32*)(pSlot + 0x38) >> 12;
    g_WorldMapViewRotationY = *(s32*)(pSlot + 0x3C) >> 12;
    func_80096F18(pName, pTarget, g_WorldMapCameraHeight, (u16*)g_WorldMapCameraRotTable);
    *(WorldMapName8*)(pScratch + 0xA0) = *pName;
    *pName = *(WorldMapName8*)&D_8009BD48;
    *(WorldMapName8*)&D_8009BD48 = *(WorldMapName8*)(pScratch + 0xA0);
    g_WorldMapViewRotationY = (g_WorldMapViewRotationY + WORLD_MAP_ANGLE_HALF_TURN) & WORLD_MAP_ANGLE_MASK;
    SetGeomScreen(g_WorldMapProjectionDistance);
    return 1;
}



s32 WorldMapSlotEnterNop(void) {
    return 1;
}

s32 WorldMapUpdateFrameWithoutEffects(void) {
    s32* pState;

    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildRotationMatrix(&g_WorldMapPathFrame);
    } else {
        func_80097244(&g_WorldMapPathFrame);
    }
    WorldMapDrawGridTiles();
    func_800848F4();
    WorldMapUpdateSlotGridOrigin(&g_WorldMapScrollX);
    if (g_WorldMapSlotUpdateFlags != 0) {
        WorldMapBuildTileIdGrid(&g_WorldMapCameraTarget);
        WorldMapWaitForLoadSlotFree();
        func_80098CC0();
    }
    pState = &g_WorldMapCameraTarget;
    func_800983A0(pState);
    func_8009932C(g_WorldMapRenderContext->pOt[0], g_WorldMapRenderContext->pOt[1], pState);
    g_WorldMapGridAngle = g_WorldMapGridAngle + WORLD_MAP_GRID_ANGLE_STEP;
    func_80073B04();
    func_800737EC();
    func_80086798();
    return 1;
}
void WorldMapInitializeModeWithSound(void) {
    RECT rect;
    s32* pPos;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = WORLD_MAP_SCREEN_WIDTH;
    rect.h = WORLD_MAP_SCREEN_HEIGHT;
    MoveImage(&rect, WORLD_MAP_SCREEN_VRAM_X, WORLD_MAP_SCREEN_VRAM_Y);
    DrawSync(0);
    WorldMapDrawFadeFrames(0x40, 0, 4, 1);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 1;
    g_WorldMapDrawMode = 4;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    pPos = (s32*)g_WorldMapPlayerPosition;
    *pPos = 0x2000000;
    g_WorldMapPlayerPositionY = 0xFFE00000;
    g_WorldMapPlayerPositionZ = 0x2000000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    WorldMapLoadClutAndTpageIds();
    func_800736DC();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(g_MenuSedsFile);
    ArchiveSetIndex(WORLD_MAP_ARCHIVE_DIRECTORY_INDEX, 0);
    WorldMapSetGridPosition(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot((void*)WorldMapModeInitFadeRect, (void*)func_800925A0);
    WorldMapAddSlot((void*)WorldMapResetActorSlotWithSound, (void*)func_80077E68);
    WorldMapAddSlot((void*)WorldMapInitActorAtScrollPosition, (void*)func_800783E8);
    WorldMapAddSlot((void*)WorldMapEnterNop, (void*)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
}
