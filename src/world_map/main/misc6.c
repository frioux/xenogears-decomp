#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"
#include "psyq/memory.h"

// g_SoundControlFlags bit: an SPU transfer command is in flight (owned by
// include/system/sound.h, which this unit does not include)
#define SOUND_CTL_FLAG_TRANSFER_BUSY (1 << 4)

// the SPU's maximum volume, which SoundPlayAudioManager takes as its volume
#define SPU_VOL_MAX 0x7F

// frames SoundSetAudioManagerVolumeWithFade takes to reach silence
#define WORLD_MAP_SOUND_FADE_OUT_FRAMES 240

// the slot actor's fixed facing angle, as the cosine/sine pair its rotation
// matrix (WorldMapUpdateSlotActor) is built from, in 1.12 fixed point
#define WORLD_MAP_SLOT_FACING_COS 0xDA6
#define WORLD_MAP_SLOT_FACING_SIN 0x85A

// how far from the player along its facing direction the slot actor is spawned:
// WorldMapInitGroup1Actor sets x and z to the player position minus each facing component
// times this
#define WORLD_MAP_SLOT_SPAWN_OFFSET 13952

// the slot's group turns by this much every frame, in 1.12 fixed point
// (a sixteenth of a turn, re-wrapped with WORLD_MAP_ANGLE_MASK in
// WorldMapUpdateSlotActor)
#define WORLD_MAP_SLOT_ANGLE_STEP 256

// an angle is a 4096-step turn counter; sums of angles are wrapped back into it.
// Owned by include/main/game.h (identical value); this unit does not include it.
#define WORLD_MAP_ANGLE_MASK 0xFFF

// a slot's type is its actor index minus the group actors ahead of it in the
// slot table (WorldMapInitSlotActor, and the group the slot drives)
#define WORLD_MAP_SLOT_ACTOR_INDEX_BASE 4

// frames a slot holds its scale after it is reset to full before state 1 starts
// shrinking it (WorldMapInitSlotActor and the slot's command-2 handler)
#define WORLD_MAP_SLOT_SCALE_HOLD_FRAMES 60

// a fading slot's two group scale factors grow by this much every frame, up to
// WORLD_MAP_SLOT_SCALE_MAX (WorldMapUpdateGroup9And10Actor and its sibling WorldMapUpdateFadeGroups)
#define WORLD_MAP_SLOT_SCALE_STEP 0x180
#define WORLD_MAP_SLOT_SCALE_MAX 0x7FFF

// the color a fading slot draws its prims with walks down by this much each frame
// until it reaches 0 and the slot deactivates (WorldMapUpdateGroup9And10Actor)
#define WORLD_MAP_SLOT_FADE_COLOR_STEP 3

// the map display buffer copied into VRAM when a mode initialises, and the
// VRAM position it is copied to
#define WORLD_MAP_SCREEN_WIDTH 320
#define WORLD_MAP_SCREEN_HEIGHT 216
#define WORLD_MAP_SCREEN_VRAM_X 0x2C0
#define WORLD_MAP_SCREEN_VRAM_Y 0x100

// the VRAM page and palette row WorldMapBuildPolyFT4Templates points the prims
// it builds at
#define WORLD_MAP_TILE_TPAGE_X 0x300
#define WORLD_MAP_TILE_TPAGE_Y 0x100
#define WORLD_MAP_TILE_CLUT_Y 0x1FF

// the archive directory this overlay's files are read through
#define WORLD_MAP_ARCHIVE_DIRECTORY_INDEX 0x24

// the prim buffer's header; the halfword at +0x4 is the number of prims that
// follow it in the group's pPrims block
typedef struct {
    /* 0x0 */ undefined32 unk00;
    /* 0x4 */ u16 count;
} WorldMapPrimHeader; // Size: 0x8

// the scratchpad's work-vector area: a world position is written here and the
// sprite draw call reads it back
typedef struct {
    /* 0x00 */ u8 unk00[0xA0];
    /* 0xA0 */ SVECTOR position;
} WorldMapScratchpad; // Size: 0xA8

// the two GTE scale vectors the group actors of a fading slot are scaled by, in
// the scratchpad. The address is a literal on purpose: naming a symbol here makes
// gcc materialise %hi/%lo through a saved register where the original stores
// through a bare lui $at.
typedef struct {
    /* 0x00 */ VECTOR first;
    /* 0x10 */ VECTOR second;
} WorldMapScaleVectors; // Size: 0x20

#define WORLD_MAP_SCALE_VECTORS ((WorldMapScaleVectors *)0x1F800000)

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ s16 command;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ undefined32 unk24;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ s32 dirX;
    /* 0x3C */ s32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ undefined32 unk48;
    /* 0x4C */ undefined32 unk4C;
    /* 0x50 */ int stateIndex; // into the per-state tables; other map modes reuse these four words
    /* 0x54 */ u16 *pStateIds;
    /* 0x58 */ u16 *pStateFrames;
    /* 0x5C */ s32 scale;
    /* 0x60 */ s32 unk60[7]; // 0x60..0x7B
    /* 0x7C */ s32 unk7C;
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x00 */ s16 hidden; // nonzero: the group draw pass skips this group
    /* 0x02 */ s16 unk02;
    /* 0x04 */ undefined16 unk04[2]; // 0x04..0x07
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ undefined32 unk14;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ WorldMapPrimHeader *pPrimHeader;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ void *pPrims[2]; // prim lists, [0] source and [1] destination; g_WorldMapPrimPacketIndex picks one
    /* 0x50 */ undefined32 unk50;
} WorldMapGroup; // Size: 0x54

typedef struct {
    /* 0x000 */ WorldMapGroup groups[17]; // 0x000..0x593
} WorldMapRoot;

typedef struct {
    /* 0x0 */ u16 *pStateIds;
    /* 0x4 */ u16 *pStateFrames;
} WorldMapPath; // Size: 0x8

extern WorldMapActor *g_WorldMapSlots;
extern WorldMapRoot *g_WorldMapGroups;
extern u8 g_WorldMapPlayerPosition[];
extern u32 g_WorldMapPlayerPositionY;
extern u32 g_WorldMapPlayerPositionZ;
extern u16 g_WorldMapScriptStateIds[];
extern u16 g_WorldMapScriptStateFrames[];
extern u16 g_WorldMapSlotScales[];
extern u16 g_WorldMapGroupInitialScale[];
extern WorldMapPath g_WorldMapActorScripts[];
extern u8 D_8009A660[];
extern u32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTarget[];
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapScrollX[];
extern s32 g_WorldMapScrollZ;
extern u32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapAreaIndex;
extern u32 g_WorldMapScrollTargetX;
extern u32 g_WorldMapScrollTargetY;
extern u32 g_WorldMapScrollTargetZ;

extern void WorldMapSetupPrimTemplates(void *, void *, int);
extern void WorldMapSetSpriteSetParent(int, int);
extern void WorldMapInitSpriteGroupSprites(int, void *, void *);
extern void WorldMapWrapPosition(VECTOR *);
extern int func_80093A5C(int, int);
extern void SoundSetAudioManagerVolumeWithFade(void *, int, int);
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void *);
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
// the sound-effect bank header, read for the sed id this overlay's effects are
// played from. The field is the one SoundFile declares at 0x14; this unit does
// not include include/system/sound.h (its g_SoundControlFlags is wider than the
// one this overlay reads), so the two words it needs are declared here.
typedef struct {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ u16 sedId;
} SoundSedsBank; // Size: unknown, only +0x14 is read

extern SoundSedsBank *g_MenuSedsFile;
extern u8 *g_WorldMapWorkBuffer1;
extern u8 *g_WorldMapWorkBuffer2;
extern u8 *g_WorldMapTilePrimBuffer;
extern u8 *g_WorldMapTilePrimBufferCopy;
extern u8 *g_WorldMapDecompressedMapData;
extern s16 g_WorldMapViewRotationY;
extern u16 g_NextMapId;
extern u16 g_NextMapRotationY;
extern u16 g_NextMapFlags;
extern u32 g_WorldMapExitFlag;
extern u16 g_WorldMapAreaExitModes[];
// the three effects an area plays together as one sound, i.e. the low half of
// the id SoundPlaySoundEffectOnFreePair takes, one triple per map area
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
} WorldMapAreaSfxIds; // Size: 0x6

extern WorldMapAreaSfxIds g_WorldMapAreaSfxIds[];
extern void WorldMapInitGfxEnv(void);
extern void WorldMapDeactivateSpriteGroup(s32);
extern void func_80072DB4(int, int, int, int);
extern int ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern s32 g_WorldMapMode4EntryTriggered;
extern void WorldMapWrapTileVertexPositions(void);
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern int ArchiveCdDataSync(int);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern s16 g_WorldMapSpawnPositions[];
extern s16 D_8009A5B6[];
extern s16 D_8009A5B8[];
extern s16 g_WorldMapAreaSpawnPositions[];
extern s16 D_8009A678[];
extern s32 g_WorldMapPrimPacketIndex;
extern void WorldMapSetPrimColors(u8 *, s32, s32, s32, s32);
extern void WorldMapBuildPolyFT4Templates(WorldMapGroup *, u8 *, int, int);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapLoadClutAndTpageIds(void);
extern void WorldMapQueueFileLoads(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void *g_GameCurLoadedWDS;
extern void *SoundLoadWdsFile(void *, void *);
extern void *g_WorldMapWdsFileBuffer;
extern void WorldMapSetGridPosition(s32 *);
extern void WorldMapUpdateLoadQueue(void);
extern int Vsync(int);
extern int WorldMapGetQueuedLoadCount(void);
extern u16 g_SoundControlFlags;
extern void SoundAddSedsEntry(void *);
extern int ArchiveDecodeAlignedSize(u32);
extern s32 g_WorldMapSoundBankFileIndex;
extern u8 g_SoundBankLoadBuffer[];
extern void *g_WorldMapSoundBankBuffer;
extern void *SoundCreateAudioManagerOnHeap(void *);
extern s32 *g_SoundAudioManager;
extern void SoundPlayAudioManager(s32 *, int, int);
extern void WorldMapAddSlot(void *, void *);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern int WorldMapSetActorScriptTables(int);
extern int WorldMapUpdateSlotStateMachine(int);
extern int func_8007E450(int);
extern int func_8007E4E4(int);
extern int WorldMapInitGroup1Actor(int);
extern void func_8007EE34(int);
extern int WorldMapInitSlotActor(int);
extern int WorldMapUpdateSlotActor(int);
extern int func_8007FC8C(int);
extern int WorldMapUpdateGroup9And10Actor(int);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapRelocateMapEntryOffsets(void);
extern void WorldMapAllocSpritePrimBuffers(void);
extern void WorldMapInitNop(void);
extern void WorldMapUpdateFrame(void);
extern void WorldMapInitSlotAtScrollPosition(void);
extern void func_80080944(void);
extern void func_80080A28(void);
extern void WorldMapUpdateFadeGroups(void);
extern int func_80080370(int);
extern void WorldMapStartSlot(s32, s32);
extern void SoundPlaySoundEffectOnFreePair(s32);
extern int func_80080578(int);
extern int func_80080600(int);
extern int WorldMapInitActorScript(int);
extern s16 g_WorldMapCameraRot[];
extern u16 g_WorldMapViewRotationZ;
extern u16 g_WorldMapPathFrame;
extern u16 D_8009BD42[];
extern u16 D_8009BD4A;
extern s32 g_WorldMapCameraHeight;
extern int rand(void);
extern void func_80096F18(void *, void *, s32, void *);

int WorldMapInitGroup12Actor(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    WorldMapSetSpriteSetParent(0xC, 0xF);
    pGroup = &g_WorldMapGroups->groups[12];
    pRot = &pGroup->rot;
    pMatrix = &pGroup->mat;
    pGroup->hidden = 0;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    pActor = &g_WorldMapSlots[index];
    pActor->dirZ = -0x4000;
    pActor->x = 0xE80000;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->y = 0;
    pActor->z = 0x280000;
    return 1;
}

int WorldMapUpdateGroup12Actor(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapRoot *pRoot = g_WorldMapGroups;

    if (pActor->command == 1) {
        WorldMapScratchpad *pScratch = PSX_SCRATCH;

        pActor->command = 0;
        pRoot->groups[12].hidden = 1;
        pRoot->groups[15].hidden = 1;
        WorldMapDeactivateSpriteGroup(0x18);
        pScratch->position.vx = pActor->x >> 12;
        pScratch->position.vy = pActor->y >> 12;
        pScratch->position.vz = pActor->z >> 12;
        WorldMapInitSpriteGroupSprites(0x1D, &pScratch->position, 0);
        return 3;
    }
    {
    WorldMapScratchpad *pScratch = PSX_SCRATCH;

    pActor->z += pActor->dirZ;
    WorldMapWrapPosition((VECTOR *)&pActor->x);
    pActor->y = func_80093A5C(pActor->x, pActor->z) - 0x4000;
    pRoot->groups[12].x = pActor->x >> 12;
    pRoot->groups[12].y = pActor->y >> 12;
    pRoot->groups[12].z = pActor->z >> 12;
    pScratch->position.vx = pActor->x >> 12;
    pScratch->position.vy = pActor->y >> 12;
    pScratch->position.vz = pActor->z >> 12;
    WorldMapInitSpriteGroupSprites(0x18, &pScratch->position, 0);
    }
    return 1;
}

int WorldMapInitGroup13Actor(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    WorldMapSetSpriteSetParent(0xD, 0xE);
    pGroup = &g_WorldMapGroups->groups[13];
    pRot = &pGroup->rot;
    pMatrix = &pGroup->mat;
    pGroup->hidden = 0;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    pActor = &g_WorldMapSlots[index];
    pActor->dirZ = -0x4000;
    pActor->x = 0xF80000;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->y = 0;
    pActor->z = 0x380000;
    return 1;
}

int WorldMapUpdateGroup13Actor(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapGroup *pGroup = &g_WorldMapGroups->groups[13];
    WorldMapScratchpad *pScratch = PSX_SCRATCH;

    pActor->z += pActor->dirZ;
    WorldMapWrapPosition((VECTOR *)&pActor->x);
    pActor->y = func_80093A5C(pActor->x, pActor->z) - 0x4000;
    pGroup->x = pActor->x >> 12;
    pGroup->y = pActor->y >> 12;
    pGroup->z = pActor->z >> 12;
    pScratch->position.vx = pActor->x >> 12;
    pScratch->position.vy = pActor->y >> 12;
    pScratch->position.vz = pActor->z >> 12;
    WorldMapInitSpriteGroupSprites(0x19, &pScratch->position, 0);
    return 1;
}

int WorldMapInitGroup16Actor(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    pGroup = &g_WorldMapGroups->groups[16];
    pRot = &pGroup->rot;
    pMatrix = &pGroup->mat;
    pGroup->hidden = 1;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    pActor = &g_WorldMapSlots[index];
    pActor->x = 0xD00000;
    pActor->z = 0x400000;
    pActor->y = 0xFFD80000;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->dirZ = -0x6000;
    return 3;
}

int WorldMapUpdateGroup16Actor(int index) {
    WorldMapGroup *pGroup = &g_WorldMapGroups->groups[16];
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 nPos;

    switch (pActor->command) {
    case 1:
        // volatile: keeps the g_WorldMapCameraTarget load below these two stores, as retail has it
        *(volatile s16 *)&pActor->command = 0;
        *(volatile s16 *)&pGroup->hidden = 0;
        pActor->x = g_WorldMapCameraTarget[0];
        pActor->z = g_WorldMapCameraTargetZ + 0x400000;
        break;
    case 2:
        pActor->command = 0;
        pActor->state = 2;
        break;
    }
    pActor->z += pActor->dirZ;
    WorldMapWrapPosition((VECTOR *)&pActor->x);
    pGroup->x = pActor->x >> 12;
    pGroup->y = pActor->y >> 12;
    pGroup->z = pActor->z >> 12;
    if (pActor->state == 2) {
        nPos = pActor->x;
        g_WorldMapScrollTargetX = nPos;
        nPos = pActor->y;
        g_WorldMapScrollTargetY = nPos;
        nPos = pActor->z;
        g_WorldMapScrollTargetZ = nPos;
    }
    return 1;
}
void func_8007D918(void) {
    RECT rect;
    s32 nIndex;
    s32 nOffset;
    u32 *pPos;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = WORLD_MAP_SCREEN_WIDTH;
    rect.h = WORLD_MAP_SCREEN_HEIGHT;
    MoveImage(&rect, WORLD_MAP_SCREEN_VRAM_X, WORLD_MAP_SCREEN_VRAM_Y);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 0x10;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    SoundFreeLoadedWdsAndAudioManager();
    /* declared as an array (see the extern above) for the same reason as in
       WorldMapInitializeModeWithActors: the pointer stays in $s0 across the
       body, where a scalar's &g_WorldMapPlayerPosition is folded into every use. */
    pPos = (u32 *)g_WorldMapPlayerPosition;
    nIndex = g_WorldMapAreaIndex;
    nOffset = g_WorldMapSpawnPositions[nIndex * 4] << 12;
    *pPos = nOffset;
    nOffset = D_8009A5B6[nIndex * 4] << 12;
    g_WorldMapPlayerPositionY = nOffset;
    /* split only here: with an inline << 12 the scheduler steals the slot */
    nOffset = D_8009A5B8[nIndex * 4];
    nOffset <<= 12;
    g_WorldMapPlayerPositionZ = nOffset;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    WorldMapLoadClutAndTpageIds();
    WorldMapQueueFileLoads();
    func_800736DC();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    g_GameCurLoadedWDS = SoundLoadWdsFile(g_WorldMapWdsFileBuffer, 0);
    ArchiveSetIndex(WORLD_MAP_ARCHIVE_DIRECTORY_INDEX, 0);
    WorldMapSetGridPosition(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    while (g_SoundControlFlags & SOUND_CTL_FLAG_TRANSFER_BUSY) {
    }
    HeapFree(g_WorldMapWdsFileBuffer);
    SoundAddSedsEntry(g_MenuSedsFile);
    memcpy(g_SoundBankLoadBuffer, g_WorldMapSoundBankBuffer, ArchiveDecodeAlignedSize(g_WorldMapSoundBankFileIndex));
    g_SoundAudioManager = SoundCreateAudioManagerOnHeap(g_SoundBankLoadBuffer);
    SoundPlayAudioManager(g_SoundAudioManager, SPU_VOL_MAX, 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapSetActorScriptTables, WorldMapUpdateSlotStateMachine);
    WorldMapAddSlot(func_8007E450, func_8007E4E4);
    WorldMapAddSlot(WorldMapInitGroup1Actor, func_8007EE34);
    WorldMapAddSlot(WorldMapInitSlotActor, WorldMapUpdateSlotActor);
    WorldMapAddSlot(WorldMapInitSlotActor, WorldMapUpdateSlotActor);
    WorldMapAddSlot(WorldMapInitSlotActor, WorldMapUpdateSlotActor);
    WorldMapAddSlot(WorldMapInitSlotActor, WorldMapUpdateSlotActor);
    WorldMapAddSlot(WorldMapInitSlotActor, WorldMapUpdateSlotActor);
    WorldMapAddSlot(func_8007FC8C, WorldMapUpdateGroup9And10Actor);
    WorldMapAddSlot(WorldMapEnterNop, WorldMapUpdateFrameWithParticles);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapInitRandomTimers();
}

void WorldMapExitToMap1A1(void) {
    s32 nIndex;
    u16 unkWord;
    u16 unkValue;

    SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, WORLD_MAP_SOUND_FADE_OUT_FRAMES);
    SoundStopAllElements();
    SoundRemoveSedsEntry(g_MenuSedsFile);
    HeapFree(g_MenuSedsFile);
    WorldMapFreeSpriteSets();
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
    nIndex = g_WorldMapAreaIndex;
    g_NextMapId = 0x1A1;
    unkValue = g_WorldMapAreaExitModes[nIndex];
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = (u16)g_WorldMapViewRotationY;
    g_NextMapFlags = unkValue;
}

int WorldMapSetActorScriptTables(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    RECT rect;

    pActor->pStateIds = g_WorldMapActorScripts[g_WorldMapAreaIndex].pStateIds;
    /* the original object relocates the second word of the pair against its own
       symbol D_8009A660 (= g_WorldMapActorScripts + 4) */
    pActor->pStateFrames = *(u16 **)&D_8009A660[g_WorldMapAreaIndex * sizeof(WorldMapPath)];
    pActor->stateIndex = 0;
    pActor->state = *(u16 *)pActor->pStateIds;
    pActor->stateTimer = ((u16 *)pActor->pStateFrames)[pActor->stateIndex];
    pActor->stateIndex++;
    return 1;
}

int WorldMapUpdateSlotStateMachine(int index) {
    // retail's frame is 0x28 with only ra/s0 saved: it reserves 0x10 of stack that no
    // instruction touches, which only an unused local reproduces.
    u8 unusedFrame[0x10];
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    switch (pActor->state) {
    case 1:
        pActor->stateTimer = pActor->stateTimer - 1;
        if ((s16)pActor->stateTimer < 0) {
            pActor->state = pActor->pStateIds[pActor->stateIndex];
            pActor->stateTimer = pActor->pStateFrames[pActor->stateIndex];
            pActor->stateIndex = pActor->stateIndex + 1;
        }
        break;
    case 2:
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x1C);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x1D);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x1E);
        WorldMapStartSlot(2, 2);
        WorldMapStartSlot(3, 2);
        WorldMapStartSlot(4, 3);
        WorldMapStartSlot(5, 3);
        WorldMapStartSlot(6, 3);
        WorldMapStartSlot(7, 3);
        WorldMapStartSlot(8, 3);
        pActor->state = 1;
        break;
    case 3:
        WorldMapClearGroupEntryFlag(0x22);
        WorldMapClearGroupEntryFlag(0x23);
        WorldMapClearGroupEntryFlag(0x24);
        WorldMapStartSlot(2, 3);
        WorldMapStartSlot(3, 3);
        WorldMapStartSlot(4, 3);
        WorldMapStartSlot(5, 3);
        WorldMapStartSlot(6, 3);
        WorldMapStartSlot(7, 3);
        WorldMapStartSlot(8, 3);
        pActor->state = 1;
        break;
    case 4:
        WorldMapStartSlot(2, 4);
        WorldMapStartSlot(3, 4);
        WorldMapStartSlot(4, 3);
        WorldMapStartSlot(5, 3);
        WorldMapStartSlot(6, 3);
        WorldMapStartSlot(7, 3);
        WorldMapStartSlot(8, 3);
        pActor->state = 1;
        break;
    case 5:
        WorldMapStartSlot(2, 5);
        pActor->state = 1;
        break;
    case 6:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x40;
        pActor->state = 1;
        break;
    case 7:
        WorldMapStartSlot(3, 5);
        WorldMapStartSlot(4, 4);
        WorldMapStartSlot(5, 4);
        WorldMapStartSlot(6, 4);
        WorldMapStartSlot(7, 4);
        WorldMapStartSlot(8, 4);
        WorldMapStartSlot(0, 0xC);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x40;
        pActor->state = 1;
        break;
    case 8:
        WorldMapStartSlot(2, 6);
        pActor->state = 1;
        break;
    case 9:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x80;
        pActor->state = 1;
        break;
    case 10:
        WorldMapStartSlot(0, 0xC);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x80;
        WorldMapStartSlot(9, 1);
        WorldMapStartSlot(2, 7);
        pActor->state = 1;
        break;
    case 16:
        WorldMapStartSlot(2, 4);
        WorldMapStartSlot(3, 0x10);
        WorldMapStartSlot(4, 3);
        WorldMapStartSlot(5, 3);
        WorldMapStartSlot(6, 3);
        WorldMapStartSlot(7, 3);
        WorldMapStartSlot(8, 3);
        pActor->state = 1;
        break;
    case 17:
        WorldMapStartSlot(2, 0x10);
        pActor->state = 1;
        break;
    case 18:
        WorldMapStartSlot(2, 0x11);
        pActor->state = 1;
        break;
    case 24:
        WorldMapStartSlot(2, 4);
        WorldMapStartSlot(3, 0x18);
        WorldMapStartSlot(4, 3);
        WorldMapStartSlot(5, 3);
        WorldMapStartSlot(6, 3);
        WorldMapStartSlot(7, 3);
        WorldMapStartSlot(8, 3);
        pActor->state = 1;
        break;
    case 25:
        WorldMapStartSlot(2, 0x18);
        pActor->state = 1;
        break;
    case 61:
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | g_WorldMapAreaSfxIds[g_WorldMapAreaIndex].unk0);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | g_WorldMapAreaSfxIds[g_WorldMapAreaIndex].unk2);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | g_WorldMapAreaSfxIds[g_WorldMapAreaIndex].unk4);
        pActor->state = 1;
        break;
    case 62:
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x19);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x1A);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x1B);
        pActor->state = 1;
        break;
    case 63:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 2;
        g_WorldMapDrawMode = 4;
        pActor->state = 1;
        break;
    case 64:
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        pActor->state = 0;
        break;
    case 0:
    default:
        break;
    }
    return 1;
}

/* 97.97% (148 bytes = retail's 37 insns), round 311. The store tail is now
   retail's exactly, down to `li v0,1` before `sh zero,0x20(v1)` and
   `sw zero,0x50(v1)` in the jr delay slot. The lever is the one measured on the
   twin func_80080578 below: the command store belongs INSIDE the do-block as
   its last statement, not after the block - that is what keeps `li v0,1` alive
   in v0 across the six target stores while the `sh v0,4(v1)` stays at the
   store's own position.
   The only remaining delta is the register rotation: retail keeps pActor in v1
   with the three player-position values in a0/a1/a2, ours takes a2 for pActor
   and v1/a0/a1 for the values. Reading g_WorldMapPlayerPositionZ inline at both
   stores, reading all three inline, and loading posZ just before its stores all
   leave pActor in a2, so it is an allocator tie, not a spelling.
   Round 333 re-measured 97.97 (148 bytes) and killed the remaining spellings
   for the rotation: `pActor` declared after the three value locals, a two
   statement `pActor = &g_WorldMapSlots[index];` instead of the declaration
   initialiser, and s32 instead of u32 values all measure 97.97 unchanged, while
   dropping the posZ alias costs the load order (96.62 - the alias is
   load-bearing, keep it). A 4-minute permuter run from this body (2502
   iterations, base=75 best=75) found nothing - the second run on it. The delta
   is one register swap (pActor v1 <-> posZ a2) over an otherwise byte-identical
   stream, i.e. local-alloc's tie-break. Parked.
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E450);
/* Round 397: the -da dump (tools/gcc-2.7.2-psx/cc1 ... -da on the preprocessed
   unit) proves the residual is not reachable by spelling. pActor's pseudo is
   TIED to the `lw g_WorldMapSlots` result by the two-address `addu`
   (addsi3 operand 0 tied to operand 1), so the one real decision is which hard
   register that load result gets: local-alloc's find_free_reg, processing
   pseudos in ascending pseudo-number order, hands it `$a2` where retail has
   `$v1`, and it only avoided `$a0` because the `index` parameter still holds it
   at the load's birth. Every other pseudo (X/$a0, Y/$a1, the constants/$v0)
   lands as retail does. The dump's hard-reg set is 2 3 4 5 6 with pActor =
   10 refs over 20 insns in `$a2`, i.e. one find_free_reg class-order choice.
   Preset is ruled out for this body: 2.6.0 48.62, 2.7.2-cdk 92.03 against
   2.7.2's 97.97 (try_flags.sh). Round 397 also measured the three value locals
   declared Z,Y,X (97.97, unchanged) and the chained
   `posX = posZ = *(u32 *)g_WorldMapPlayerPosition;` (97.97, unchanged).
   The percent and byte count are stale-safe: re-measured 97.97 / 148 bytes.
/*
int func_8007E450(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u32 posX;
    u32 posY;
    u32 posZ;

    g_WorldMapRotationMatrixValid = 0;
    pActor->unk7C = 0x1000;
    do {
        posZ = *(u32 *)g_WorldMapPlayerPosition;
        posX = posZ;
        posY = g_WorldMapPlayerPositionY;
        posZ = g_WorldMapPlayerPositionZ;
        g_WorldMapFadeLevel = 0x78;
        g_WorldMapCameraTarget[0] = posX;
        g_WorldMapScrollTargetX = posX;
        g_WorldMapCameraTargetY = posY;
        g_WorldMapScrollTargetY = posY;
        g_WorldMapCameraTargetZ = posZ;
        g_WorldMapScrollTargetZ = posZ;
        pActor->command = 1;
    } while (0);
    pActor->pStateFrames = (void *)0x40;
    pActor->state = 0;
    pActor->stateIndex = 0;
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E4E4);
/* 86.92% (1716 bytes against retail's 1752) - CONFIRMED in round 185, and the
   body compiles as it stands. It can never link: this stub's .s owns
   jtbl_8006FE94 and jtbl_8006FEF4 (grep dlabel), so converting it drops 36
   bytes of jump tables and the overlay shifts. Leave it INCLUDE_ASM.
   Re-measured in round 163: the body
   compiles now that the extern says `int func_8007E4E4(int)` - it returns 1,
   and the old `extern void` made the park a hard error, which is why the sweep
   scored it ERROR instead of 86.92%. BLOCKED (owns jtbl_8006FE94/FEF4), so do
   not convert it. Two switches on
   pActor->command - 1 (24 cases) and pActor->state (26 cases), then the rand
   jitter tail. Structure, case order (retail emits the bodies in source order)
   and both dispatch chains agree; the levers that got it here:
   - switch 2 needs an explicit `case 0: break;` - without it gcc subtracts the
     minimum case (1) and emits sltiu 25, retail tests the raw lh against 26.
   - g_WorldMapCameraTarget/BBB4 as `extern s32 X[]` with X[0] RMW: retail materialises the
     address into a register (lw/sw off $a3/$a0) where the scalar form emits a
     lui/$at macro store. Same trick for g_WorldMapViewRotationY (already switched to the
     array form for this attempt).
   - unk5C/unk7C must be s32 (retail sra/slti, not srl/sltiu).
   - case 16 stores 0x8000 to unk7C (0x7C), case 24 stores 0x10000 to
     unk5C[1] (0x60) - easy to confuse, they share the else block.
   Remaining deltas, all one family: (1) cases 0,1,2,3,5,16 each keep a
   `sh zero,0x4(s0)` (command = 0) BEFORE their own BD38 store; ours emits that
   store after the BD38 store, so cross-jumping folds it into the shared
   BD3A/BD3C tail. Moving `pActor->command = 0;` earlier/later in the case body
   is the lever to try - the `do { } while (0);` grouping that closed the
   scheduled-stores family in this file is the obvious next try. (2) In case 1's
   clamp branch retail stores g_WorldMapCameraTargetZ before `sh v0,0x20(s0)`, ours the other
   way round. (3) case 3's second test is `bnez` to the else with
   `lui a1,0xFFFB3C00` in the delay slot, ours is `beqz` with the lui before the
   branch. (4) case 4 and case 25 order the unk5C[0]/unk60 load against the lhu
   of g_WorldMapViewRotationY differently.
   The note on func_80080600 (same shape; 80.11% in round 312) still applies for
   the tail: the u16 reload of 0xA2 is real, keep `*(u16 *)(pScratch + 0xA2)`.
/* s32 func_8007E4E4(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 nState;
    s32 nRandom;

    nState = *(u16 *)&pActor->command - 1;
    switch (nState) {
    case 0:
        g_WorldMapCameraHeight = 0x500000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0x150;
        g_WorldMapViewRotationY = 0xF50;
        g_WorldMapViewRotationZ = 0;
        break;
    case 1:
        g_WorldMapCameraHeight = 0x4D0000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0xC60;
        g_WorldMapViewRotationY = 0x380;
        g_WorldMapViewRotationZ = 0;
        break;
    case 2:
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0x30;
        g_WorldMapViewRotationY = 0x160;
        g_WorldMapViewRotationZ = 0;
        break;
    case 3:
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0x30;
        g_WorldMapViewRotationY = 0x960;
        g_WorldMapViewRotationZ = 0;
        break;
    case 4:
        pActor->state = 1;
        pActor->command = 0;
        pActor->unk5C[0] = -0x8000;
        break;
    case 5:
        pActor->state = 3;
        pActor->unk7C = 0x1000;
        g_WorldMapCameraHeight = 0x200000;
        pActor->command = 0;
        g_WorldMapCameraRot = -0x58;
        g_WorldMapViewRotationY = 0xE58;
        g_WorldMapViewRotationZ = 0;
        break;
    case 6:
        pActor->state = 5;
        pActor->command = 0;
        pActor->unk7C = 0x40000;
        break;
    case 15:
        pActor->state = 0x10;
        pActor->command = 0;
        pActor->unk5C[0] = -0x8000;
        break;
    case 16:
        pActor->state = 0x10;
        pActor->unk7C = 0x1000;
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0xE0;
        g_WorldMapViewRotationY = 0x798;
        g_WorldMapViewRotationZ = 0;
        break;
    case 23:
        pActor->state = 0x18;
        pActor->command = 0;
        pActor->unk5C[0] = -0x8000;
        break;
    }
    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        func_80096F18(&g_WorldMapPathFrame, g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot);
    }
    switch (pActor->state) {
    case 0:
        break;
    case 1:
        g_WorldMapCameraTarget[0] += 0xFFFBD300;
        g_WorldMapCameraTargetZ += 0x6D300;
        if (g_WorldMapCameraTarget[0] <= 0x1F9DFFF && g_WorldMapCameraTargetZ > 0x5998000) {
            g_WorldMapCameraTarget[0] = 0x1F9E000;
            g_WorldMapCameraTargetZ = 0x5998000;
            pActor->state = 2;
            pActor->unk7C = 0x10000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            g_WorldMapScrollZ += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY -= 8;
        g_WorldMapCameraRot += pActor->unk5C[0] >> 12;
        pActor->unk5C[0] += 0x300;
        g_WorldMapCameraHeight += 0x8000;
        if (g_WorldMapCameraRot >= 0x81) {
            g_WorldMapCameraRot = 0x80;
        }
        break;
    case 2:
        pActor->unk7C -= 0x80;
        if (pActor->unk7C < 0x1000) {
            pActor->unk7C = 0x1000;
        }
        break;
    case 3:
        g_WorldMapCameraTarget[0] += 0xFFFB3C00;
        g_WorldMapCameraTargetY += 0xFF80;
        g_WorldMapCameraTargetZ += 0xFFF9AA80;
        if (g_WorldMapCameraTarget[0] <= 0x1497FFF && g_WorldMapCameraTargetZ > 0x4AF1FFF) {
            g_WorldMapCameraTarget[0] = 0x1498000;
            g_WorldMapCameraTargetZ = 0x4AF2000;
            pActor->state = 4;
            pActor->unk5C[0] = 0x20000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFB3C00;
            g_WorldMapScrollZ += 0xFFF9AA80;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapCameraHeight += 0x20000;
        g_WorldMapViewRotationY += 0x20;
        g_WorldMapCameraRot -= 4;
        break;
    case 4:
        g_WorldMapViewRotationY += pActor->unk5C[0] >> 12;
        pActor->unk5C[0] -= 0x800;
        if (pActor->unk5C[0] < 0) {
            pActor->unk5C[0] = 0;
            pActor->state = 0;
        }
        break;
    case 5:
        pActor->unk7C -= 0x2000;
        if (pActor->unk7C < 0x1000) {
            pActor->unk7C = 0x1000;
            pActor->state = 0;
        }
        break;
    case 16:
        g_WorldMapCameraTarget[0] += 0xFFFBD300;
        g_WorldMapCameraTargetZ += 0x6D300;
        if (g_WorldMapCameraTarget[0] <= 0x2151FFF && g_WorldMapCameraTargetZ > 0x5AA3000) {
            g_WorldMapCameraTarget[0] = 0x2152000;
            g_WorldMapCameraTargetZ = 0x5AA3000;
            pActor->state = 0x11;
            pActor->unk7C = 0x8000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            g_WorldMapScrollZ += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY -= 8;
        g_WorldMapCameraRot += pActor->unk5C[0] >> 12;
        pActor->unk5C[0] += 0x300;
        g_WorldMapCameraHeight += 0x8000;
        if (g_WorldMapCameraRot >= 0x81) {
            g_WorldMapCameraRot = 0x80;
        }
        break;
    case 17:
        pActor->unk7C -= 0x200;
        if (pActor->unk7C < 0x1000) {
            pActor->unk7C = 0x1000;
        }
        break;
    case 24:
        g_WorldMapCameraTarget[0] += 0xFFFBD300;
        g_WorldMapCameraTargetZ += 0x6D300;
        if (g_WorldMapCameraTarget[0] <= 0x2151FFF && g_WorldMapCameraTargetZ > 0x5AA3000) {
            g_WorldMapCameraTarget[0] = 0x2152000;
            g_WorldMapCameraTargetZ = 0x5AA3000;
            pActor->state = 0x19;
            pActor->unk5C[1] = 0x10000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            g_WorldMapScrollZ += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY -= 8;
        g_WorldMapCameraRot += pActor->unk5C[0] >> 12;
        pActor->unk5C[0] += 0x300;
        g_WorldMapCameraHeight += 0x8000;
        if (g_WorldMapCameraRot >= 0x81) {
            g_WorldMapCameraRot = 0x80;
        }
        break;
    case 25:
        g_WorldMapViewRotationY -= pActor->unk5C[1] >> 12;
        pActor->unk5C[1] -= 0x200;
        if (pActor->unk5C[1] < 0) {
            pActor->unk5C[1] = 0;
        }
        break;
    }
    nRandom = rand() % (pActor->unk7C >> 12) - (pActor->unk7C >> 13);
    *(u16 *)(pScratch + 0xA2) = nRandom;
    D_8009BD42 += nRandom;
    D_8009BD4A += *(u16 *)(0xA2 + pScratch);
    return 1;
}
*/

void WorldMapBuildPolyFT4Templates(WorldMapGroup *pGroup, u8 *pPrim, int count, int abr) {
    POLY_FT4 *pPoly;
    int i;

    for (i = 0, pPoly = (POLY_FT4 *)pPrim; i < count; i++, pPoly++) {
        setPolyFT4(pPoly);
        pPoly->tpage = GetTPage(0, abr, WORLD_MAP_TILE_TPAGE_X, WORLD_MAP_TILE_TPAGE_Y);
        pPoly->clut = GetClut(0, WORLD_MAP_TILE_CLUT_Y);
        setSemiTrans(pPoly, 1);
        setRGB0(pPoly, 0x3C, 0x3C, 0xC0);
    }
    memcpy((u8 *)pGroup->pPrims[1], pGroup->pPrims[0], count * sizeof(POLY_FT4));
}

int WorldMapInitGroup1Actor(int index) {
    WorldMapActor *pActor;
    WorldMapRoot *pRoot;
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    int nIndex;

    WorldMapSetSpriteSetParent(1, 2);
    WorldMapSetSpriteSetParent(1, 3);
    pRoot = g_WorldMapGroups;
    pActor = &g_WorldMapSlots[index];
    WorldMapBuildPolyFT4Templates(&pRoot->groups[1], (u8 *)pRoot->groups[1].pPrims[0], pRoot->groups[1].pPrimHeader->count, 3);
    WorldMapBuildPolyFT4Templates(&pRoot->groups[2], (u8 *)pRoot->groups[2].pPrims[0], pRoot->groups[2].pPrimHeader->count, 3);
    WorldMapBuildPolyFT4Templates(&pRoot->groups[3], (u8 *)pRoot->groups[3].pPrims[0], pRoot->groups[3].pPrimHeader->count, 1);
    pGroup = &g_WorldMapGroups->groups[1];
    pRot = &pGroup->rot;
    pMatrix = &pGroup->mat;
    pGroup->hidden = 0;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    nIndex = g_WorldMapAreaIndex;
    // volatile: the stores below are a straight-line cluster that cc1 would otherwise
    // reorder; volatile keeps them in source order, which is retail's.
    *(volatile s32 *)&pActor->dirX = -WORLD_MAP_SLOT_FACING_SIN;
    *(volatile s16 *)&pActor->state = 0;
    *(volatile s32 *)&pActor->dirY = 0;
    *(volatile s32 *)&pActor->dirZ = WORLD_MAP_SLOT_FACING_COS;
    *(volatile s32 *)&pActor->stateIndex = g_WorldMapAreaSpawnPositions[nIndex * 4] << 12;
    *(volatile void **)&pActor->pStateIds = (void *)(D_8009A678[nIndex * 4] << 12);
    *(volatile s32 *)&pActor->x = *(u32 *)g_WorldMapPlayerPosition - pActor->dirX * WORLD_MAP_SLOT_SPAWN_OFFSET;
    *(volatile s32 *)&pActor->y = g_WorldMapPlayerPositionY;
    *(volatile s32 *)&pActor->z = g_WorldMapPlayerPositionZ - pActor->dirZ * WORLD_MAP_SLOT_SPAWN_OFFSET;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EE34);
// 100.00% in objdiff under -G0 (99.4 under the unit preset) but the C is 45 bytes
// the wrong length at link time, so -G0 is NOT the answer for this overlay (its
// globals are absolute symbols from undefined_syms_auto, and -G0 changes that
// addressing). Do not re-try -G0 here.

int WorldMapInitSlotActor(int index) {
    int nType = index - WORLD_MAP_SLOT_ACTOR_INDEX_BASE;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapGroup *pGroup = &g_WorldMapGroups->groups[nType + WORLD_MAP_SLOT_ACTOR_INDEX_BASE];
    u16 unkValue;
    int nAbr;

    if (nType == 4) {
        nAbr = 1;
    } else {
        nAbr = 3;
    }
    WorldMapBuildPolyFT4Templates(pGroup, pGroup->pPrims[0], pGroup->pPrimHeader->count, nAbr);
    pActor->dirX = -WORLD_MAP_SLOT_FACING_SIN;
    pActor->dirZ = WORLD_MAP_SLOT_FACING_COS;
    pActor->state = 0;
    pActor->dirY = 0;
    unkValue = g_WorldMapSlotScales[nType];
    pActor->stateTimer = WORLD_MAP_SLOT_SCALE_HOLD_FRAMES;
    pActor->scale = unkValue;
    return 1;
}

int WorldMapUpdateSlotActor(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapActor *pRefActor = &g_WorldMapSlots[3];
    WorldMapGroup *pGroup = &g_WorldMapGroups->groups[index];
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 nState;
    s32 nScale;
    SVECTOR vec; // unused: retail reserves 8 bytes of locals, gcc drops the slot without it

    // the object reads the state word unsigned here; the field stays s16 for
    // the group actors' switches on the same offset
    nState = *(u16 *)&pActor->command - 1;
    switch (nState) {
    case 0:
        pActor->command = 0;
        break;
    case 1:
        pActor->command = 0;
        pActor->state = 1;
        break;
    case 2:
        pActor->stateTimer = WORLD_MAP_SLOT_SCALE_HOLD_FRAMES;
        pActor->dirX = -WORLD_MAP_SLOT_FACING_SIN;
        pActor->dirZ = WORLD_MAP_SLOT_FACING_COS;
        pActor->state = (pActor->command = 0);
        pActor->dirY = 0;
        pActor->scale = g_WorldMapGroupInitialScale[index];
        pGroup->hidden = 0;
        pGroup->rot.vx = pGroup->rot.vy = pGroup->rot.vz = 0;
        RotMatrixYXZ(&pGroup->rot, &pGroup->mat);
        WorldMapDeactivateSpriteGroup(0x23);
        WorldMapDeactivateSpriteGroup(0x24);
        break;
    case 3:
        pActor->stateTimer = 4;
        pActor->state = 1;
        pActor->command = 0;
        pActor->scale = g_WorldMapGroupInitialScale[index];
        pGroup->hidden = 0;
        pGroup->rot.vx = pGroup->rot.vy = pGroup->rot.vz = 0;
        RotMatrixYXZ(&pGroup->rot, &pGroup->mat);
        WorldMapDeactivateSpriteGroup(0x23);
        WorldMapDeactivateSpriteGroup(0x24);
        break;
    case 4:
        pActor->state = 1;
        pActor->command = 0;
        pActor->stateTimer = 30;
        break;
    }
    switch (pActor->state) {
    case 0:
    {
        SVECTOR *pPos = (SVECTOR *)(pScratch + 0xA0);
        SVECTOR *pAngle = (SVECTOR *)(pScratch + 0xA8);

        pActor->x = pRefActor->x;
        pActor->y = pRefActor->y;
        pActor->z = pRefActor->z;
        pPos->vx = (pActor->x + 0x42D000) >> 12;
        pPos->vy = pActor->y >> 12;
        pPos->vz = (pActor->z - 0x6D3000) >> 12;
        pAngle->vx = (pAngle->vz = 0);
        pAngle->vy = ratan2(WORLD_MAP_SLOT_FACING_SIN, WORLD_MAP_SLOT_FACING_COS) & WORLD_MAP_ANGLE_MASK;
        WorldMapInitSpriteGroupSprites(0x23, pPos, pAngle);
        WorldMapInitSpriteGroupSprites(0x24, pPos, pAngle);
        break;
    }
    case 1:
        if ((s16)--pActor->stateTimer < 0) {
            pActor->stateTimer = 0;
            pActor->scale -= 0x80;
        }
        break;
    }
    {
        MATRIX *pTmp = (MATRIX *)(pScratch + 0x110);
        VECTOR *pScale = (VECTOR *)pScratch;

        ((MATRIX *)(pScratch + 0xF0))->m[0][0] = WORLD_MAP_SLOT_FACING_COS;
        ((MATRIX *)(pScratch + 0xF0))->m[0][1] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[0][2] = WORLD_MAP_SLOT_FACING_SIN;
        ((MATRIX *)(pScratch + 0xF0))->m[1][0] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[1][1] = -0x1000;
        ((MATRIX *)(pScratch + 0xF0))->m[1][2] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[2][0] = -WORLD_MAP_SLOT_FACING_SIN;
        ((MATRIX *)(pScratch + 0xF0))->m[2][1] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[2][2] = WORLD_MAP_SLOT_FACING_COS;
        pGroup->rot.vz = (pGroup->rot.vz + WORLD_MAP_SLOT_ANGLE_STEP) & WORLD_MAP_ANGLE_MASK;
        RotMatrixYXZ(&pGroup->rot, pTmp);
        MulMatrix0((MATRIX *)(pScratch + 0xF0), pTmp, &pGroup->mat);
        pGroup->x = pActor->x >> 12;
        pGroup->y = (pActor->y >> 6) >> 6;
        pGroup->z = pActor->z >> 12;
        nScale = pActor->scale;
        pScale->vz = 0x2000;
        pScale->vy = nScale;
        pScale->vx = nScale;
        ScaleMatrix(&pGroup->mat, pScale);
    }
    if (pActor->scale < 0) {
        pActor->scale = 0;
        pGroup->hidden = 1;
        return 3;
    }
    return 1;
}
/* 93.41% (164 bytes = retail's 41 insns). Store order, both calls and every
   relocation already match; the single delta is where retail's `li v0,3` sits
   - fifth instruction of the store group, BEFORE the stores - so v0 is
   occupied and the group's constants land in v1/a0, where ours materialises
   the 3 after the last store and then uses v0 for every constant.
   Measured in round 312 on this body: `nRet = 3;` before the block, as the
   block's FIRST statement and as its LAST statement all fold back to 93.41
   (copy-prop deletes the single-use assignment, so the `li` is emitted at the
   return); moving the `return` itself inside the block, and dropping the block
   entirely, both give 88.66 - the `li` does come out before the group then,
   but the pStateFrames store jumps above the state store -; the permuter
   (4 min, 2111 iterations) found nothing, as did round 202's 3-minute run.
   Note gcc 2.7.2 CAN hoist this constant: WorldMapInitGroup16Actor above is
   matched with a plain `return 3;` and its `addiu v0,zero,3` is the first
   instruction of the post-call block. So the shape exists, it is just not
   found here. The struct's 0x50/0x54/0x58 are stateIndex/pStateIds/
   pStateFrames now - the old unk50/unk54/unk58 spelling no longer compiles,
   which is why this park measured nothing until round 312.
   Round 316: the reason it cannot float here is the register, not the shape -
   gcc sinks an insn that sets a register to a constant to the earliest point
   after that register's last earlier use (WorldMapInitGroup16Actor's
   `addiu v0,zero,3` lands at the head of its post-call block exactly that
   way). Here the last $v0 use before the return is `li v0,0x80` for
   pStateFrames, so the li can never come out before that store; retail has
   that constant in v1 precisely because v0 was already taken by the 3. The
   body below is the measured 93.41 (gcc emits `lui v0,0x149; ori`;
   `lui v1,0x4af; ori`; stores; `li v0,3` last, byte-identical otherwise).
   Round 325 re-measured this and adds two facts. (a) The store ORDER in the
   body below is already retail's exactly - x, y, pStateIds, state, z,
   stateIndex, pStateFrames - so only the `li v0,3` position and the register
   rotation it causes are missing. (b) That rotation is circular, which is why
   no statement shape breaks it: retail has $v0 taken by the 3 before the store
   group, so the group's constants go to $v1 (x, y, pStateIds, pStateFrames) and
   $a0 (z); ours hands $v0 to x's constant and then $v1 to z's, with the 3
   emitted last. Measured dead in round 325: `return 3;` as the do-block's last
   statement - the `li` DOES come out earlier (88.66) but it lands AFTER the
   pStateFrames store and the pStateFrames store then jumps above the state
   store; making that one store `*(volatile s32 *)&pActor->pStateFrames` pins it
   but leaves the `li` after the group (88.66, and the constants still rotate);
   the same volatile on the current body changes nothing (93.41).
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FC8C);
/* 93.41% (164 bytes, 41 insns both): only the tail store block differs in
   register rotation and the li 3 position. Retail: lui v1/ori (x), lui a0/ori
   (z), li v0,3, then stores with y/pStateIds/pStateFrames constants in v1.
   Ours: x's constant takes v0, z's v1, and the li 3 sinks to the end because
   v0's last earlier use is the li 0x80 for pStateFrames.
   Round 396 re-measured this body live at 93.41 and tried three new shapes:
   `int result = 3;` assigned right after the calls and returned at the end
   (88.66 - cse folds it back to a plain return and the li sinks anyway),
   the same plus nX/nZ locals holding the two hoisted constants in retail's
   load order (88.66 - cse substitutes the constants into the stores and
   deletes the sets). cdk -O2 -G8 also 88.66 on that body, gcc 2.6.0 81.07,
   -G0 does not compile, so the preset is right. Root cause unchanged from
   round 325: circular - the store constants claim v0 because the 3 is not
   in it, and the 3 sinks to the end because the constants hold v0.
int func_8007FC8C(int index) {
    WorldMapRoot *pRoot = g_WorldMapGroups;
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    WorldMapSetupPrimTemplates(&pRoot->groups[9], pRoot->groups[9].pPrims[0], pRoot->groups[9].pPrimHeader->count);
    WorldMapSetupPrimTemplates(&pRoot->groups[10], pRoot->groups[10].pPrims[0], pRoot->groups[10].pPrimHeader->count);
    do {
        pActor->x = 0x1498000;
        pActor->y = 0xFFF80000;
        pActor->pStateIds = (void *)-0x800;
        pActor->state = 0;
        pActor->z = 0x4AF2000;
        pActor->stateIndex = 0;
        pActor->pStateFrames = (void *)0x80;
    } while (0);
    return 3;
}
*/
int WorldMapUpdateGroup9And10Actor(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapRoot *pRoot = g_WorldMapGroups;
    WorldMapGroup *pGroup = &pRoot->groups[9];
    WorldMapGroup *pNextGroup;
    s32 nPos;
    s32 nSpread;

    if (pActor->command == 1) {
        pActor->command = 0;
    }
    nPos = pActor->x >> 12;
    pGroup[1].x = nPos;
    pGroup->x = nPos;
    nPos = pActor->y >> 12;
    pGroup[1].y = nPos;
    pGroup->y = nPos;
    nPos = pActor->z >> 12;
    pGroup[1].z = nPos;
    pGroup->z = nPos;
    pGroup[1].mat = g_WorldMapIdentityMatrix;
    pGroup->mat = pGroup[1].mat;
    WORLD_MAP_SCALE_VECTORS->first.vx = WORLD_MAP_SCALE_VECTORS->first.vz = pActor->stateIndex;
    nSpread = (s32)pActor->pStateIds;
    WORLD_MAP_SCALE_VECTORS->first.vy = WORLD_MAP_SCALE_VECTORS->second.vy = 0x1000;
    WORLD_MAP_SCALE_VECTORS->second.vx = WORLD_MAP_SCALE_VECTORS->second.vz = nSpread;
    ScaleMatrix(&pGroup->mat, &WORLD_MAP_SCALE_VECTORS->first);
    ScaleMatrix(&pGroup[1].mat, &WORLD_MAP_SCALE_VECTORS->second);
    pNextGroup = pGroup + 1;
    pActor->stateIndex += WORLD_MAP_SLOT_SCALE_STEP;
    if (pActor->stateIndex > WORLD_MAP_SLOT_SCALE_MAX) {
        pActor->stateIndex = WORLD_MAP_SLOT_SCALE_MAX;
    }
    pActor->pStateIds = (void *)((s32)pActor->pStateIds + WORLD_MAP_SLOT_SCALE_STEP);
    if ((s32)pActor->pStateIds > WORLD_MAP_SLOT_SCALE_MAX) {
        pActor->pStateIds = (void *)WORLD_MAP_SLOT_SCALE_MAX;
    }
    WorldMapSetPrimColors(pGroup->pPrims[g_WorldMapPrimPacketIndex], pGroup->pPrimHeader->count,
                          (s32)pActor->pStateFrames, (s32)pActor->pStateFrames, (s32)pActor->pStateFrames);
    pGroup = pNextGroup;
    WorldMapSetPrimColors(pGroup->pPrims[g_WorldMapPrimPacketIndex], pGroup->pPrimHeader->count,
                          (s32)pActor->pStateFrames, (s32)pActor->pStateFrames, (s32)pActor->pStateFrames);
    pActor->pStateFrames = (void *)((s32)pActor->pStateFrames - WORLD_MAP_SLOT_FADE_COLOR_STEP);
    if ((s32)pActor->pStateFrames < 0) {
        pActor->pStateFrames = NULL;
        return 3;
    }
    return 1;
}

void WorldMapInitializeModeWithActors(void) {
    RECT rect;
    u32 *pPos;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = WORLD_MAP_SCREEN_WIDTH;
    rect.h = WORLD_MAP_SCREEN_HEIGHT;
    MoveImage(&rect, WORLD_MAP_SCREEN_VRAM_X, WORLD_MAP_SCREEN_VRAM_Y);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 0x10;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    /* declared as an array (see the extern above): the pointer then stays in
       $s0 across the body, where a scalar's &g_WorldMapPlayerPosition is folded into every
       use. Value reads are *(u32*)g_WorldMapPlayerPosition. */
    pPos = (u32 *)g_WorldMapPlayerPosition;
    *pPos = 0x4100000;
    g_WorldMapPlayerPositionY = 0xFFF80000;
    g_WorldMapPlayerPositionZ = 0x13C0000;
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
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(g_MenuSedsFile);
    ArchiveSetIndex(WORLD_MAP_ARCHIVE_DIRECTORY_INDEX, 0);
    WorldMapSetGridPosition(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapInitActorScript, func_80080370);
    WorldMapAddSlot(func_80080578, func_80080600);
    WorldMapAddSlot(WorldMapInitSlotAtScrollPosition, func_80080944);
    WorldMapAddSlot(func_80080A28, WorldMapUpdateFadeGroups);
    WorldMapAddSlot(WorldMapInitNop, WorldMapUpdateFrame);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapAllocSpritePrimBuffers();
    WorldMapInitRandomTimers();
}
void WorldMapExitToMap84(void) {
    u16 unkValue;

    SoundStopAllElements();
    SoundRemoveSedsEntry(g_MenuSedsFile);
    HeapFree(g_MenuSedsFile);
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
    unkValue = (u16)g_WorldMapViewRotationY;
    g_NextMapId = 0x84;
    g_NextMapFlags = 2;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = unkValue;
}

int WorldMapInitActorScript(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->stateIndex = 0;
    pActor->state = g_WorldMapScriptStateIds[0];
    pActor->stateTimer = g_WorldMapScriptStateFrames[pActor->stateIndex];
    return 1;
}

/* Best: 99.92%, 520 bytes = retail's exact size, objdiff 99.92% (round 400,
   re-measured live in round 414). Every instruction is identical to retail's;
   the ONLY residual is the switch table's PLACEMENT: retail has `lw v0,0(at)`
   with %lo(jtbl_800701E0), this body `lw v0,24(at)` with %lo(.rodata) - the same
   table at object offset 0x18 (report mode) / 0x458 (matching mode) against
   retail's range offset 0x454. permute.sh on the live body: PERMUTER_ZERO
   base=0 best=0 in one iteration, i.e. the instruction stream already equals the
   original and only the relocation's target/addend differs, so no C spelling can
   close this - do not unpark it for C work.
   JTBL_LAYOUT, and round 414 supersedes the old "needs a maspsx rewrite" verdict:
   the table is convertible via the lever i-rodata-head-carve-moves route, but it
   needs a UNIT SPLIT, not a yaml-only carve. Arithmetic: misc6's .rodata range
   0x29C..0x7F4 holds eight tables at range offsets 0, 0x108, 0x168, 0x1D0, 0x230,
   0x338, 0x440 (all 8-aligned) and this one at 0x454 (4 mod 8), tiling its 0x558
   bytes with no slack. cc1's .align 3 is section-relative and hardcoded (lever
   i-jtbl-align-3-hardcoded), so a C table can only land at an 8-aligned OBJECT
   offset, and the 4 mod 8 comes from the one preceding C table -
   WorldMapUpdateSlotActor's 5-word jtbl_800701CC (0x14 bytes, not a multiple of
   8) - which is already matched in this unit and must not move. The table's
   ABSOLUTE address 0x800701E0 IS 8-aligned, so the fix is: a new main/misc6_b
   whose .rodata range starts at overlay 0x6F0 (yaml `- [0x29C, .rodata,
   main/misc6]` becomes `- [0x6F0, .rodata, main/misc6]` plus `- [0x6F0,
   .rodata, main/misc6_b]`) with the code range split at this function's file
   offset 0x10880 (`- [0x10880, c, main/misc6_b]`, misc6's `c` range ending
   there), the tail functions moved to misc6_b.c, and a gears.toml path entry for
   it because XenoWorldMapDiv is keyed per file. Its table then sits at object
   offset 0 where `.align 3` is a no-op.
   Shapes measured on the way: `default: break;` alone (no `case 0:`) makes gcc
   fold the index to `state - 1` and emit 64 entries with `sltiu ... 64` instead
   of retail's 65 / `sltiu ... 0x41` (lever i-case-0-label-keeps-table-base) - the
   `case 0:` label is what keeps the table's slot 0. The state-1 arm, the
   WorldMapStartSlot/global-set cases and the three
   SoundPlaySoundEffectOnFreePair calls are all instruction-exact.
*/
/*
int func_80080370(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    switch (pActor->state) {
    case 1:
        pActor->stateTimer = pActor->stateTimer - 1;
        if ((s16)pActor->stateTimer < 0) {
            pActor->state = g_WorldMapScriptStateIds[pActor->stateIndex];
            pActor->stateTimer = g_WorldMapScriptStateFrames[pActor->stateIndex];
            pActor->stateIndex = pActor->stateIndex + 1;
        }
        break;
    case 2:
        WorldMapStartSlot(3, 1);
        pActor->state = 1;
        break;
    case 3:
        WorldMapStartSlot(2, 2);
        pActor->state = 1;
        break;
    case 4:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x80;
        pActor->state = 1;
        break;
    case 5:
        WorldMapStartSlot(0, 0xC);
        WorldMapStartSlot(4, 1);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x80;
        pActor->state = 1;
        break;
    case 6:
        WorldMapStartSlot(2, 3);
        pActor->state = 1;
        break;
    case 7:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 2;
        g_WorldMapDrawMode = 4;
        pActor->state = 1;
        break;
    case 8:
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x16);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x17);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x18);
        pActor->state = 1;
        break;
    case 0x40:
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        pActor->state = 0;
        break;
    case 0:
    default:
        break;
    }
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080370);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080578);
/* 94.71% (136 bytes = retail's 34 insns), round 311. SOLVED: the
   constant-before-store-group question. Retail's first `li v0,1` sits six
   instructions above the `sh v0,4(v1)` that reads it. The source shape that
   reproduces it is the command store as the LAST statement INSIDE the
   do-block, not after the block. Measured on a faithful probe replica of this
   function (tools/scripts/cc1_probe.sh with the same externs and struct):
   store after the block -> the `li` is emitted adjacent to its `sh` (the old
   92.21 body); store as the block's last statement -> the `li` is hoisted to
   the top of the six-store group and the `sh` follows them, retail's shape.
   So the constant was never what needed keeping alive - the STORE's position
   was.
   Remaining deltas: (1) the register rotation - retail keeps pActor in v1 with
   the position values in a0/a1/a2, ours takes a2 for pActor and v1/a0/a1 for
   the values; (2) reorg's delay-slot choice at the end: retail ends
   `sh v0,4(v1)` / `li v0,1` / `jr ra` / `sh zero,0x20(v1)`, ours keeps the two
   halfword stores adjacent and steals the `li`. Its twin func_8007E450 above
   gets retail's tail for free because its two trailing zero-stores (state,
   stateIndex) leave a store next to the jr, which this body does not have.
   A 4-minute permuter run from the 94.71 body only proposed moving the command
   store back out of the block (i.e. back to 92.21), so it is exhausted here.
   Round 333 re-measured this body at 94.71 (136 bytes) and added: the plain
   `posX = *(u32 *)g_WorldMapPlayerPosition;` load (no posZ alias) is also
   94.71 here, and moving the tail `pActor->state = 0;` inside the do-block
   changes nothing. Putting the `return 1;` INSIDE the block with it (so
   loop.c could hoist the return constant into the preheader the way it hoists
   the command's `li v0,1`) is 86.47 and DROPS the command store - do not retry.
   ROUND 397: the register rotation is the same unfixable artifact as the twin
   func_8007E450 above, and it is now proven rather than guessed. The -da dump of
   this unit shows pActor's pseudo is tied to the `lw g_WorldMapSlots` result by
   the two-address `addu` (addsi3 ties operand 0 to operand 1), so the one real
   decision is which register that load result gets: local-alloc's find_free_reg
   hands it `$a2` where retail has `$v1` (it only skips `$a0` because the index
   parameter still holds it at the load's birth). -G0 leaves it at 94.71, 2.6.0 is
   48.62 and 2.7.2-cdk 92.03 on the twin, so the preset is not the lever either.
   Round 397 also measured `nResult` (an `int nResult = 1;` before the state store,
   `return nResult;`) to try to keep the return's `li v0,1` ahead of `sh zero,0x20`
   the way retail has it: gcc folds it and the tail is unchanged at 94.71, because
   our RTL ends `sh ... / sh zero / li v0,1 / jr` so reorg takes the li into the
   delay slot. Re-measured 94.71 / 136 bytes after the -da and -G0 screens.
*/
/*
int func_80080578(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u32 posX;
    u32 posY;
    u32 posZ;

    g_WorldMapRotationMatrixValid = 0;
    pActor->unk7C = 0x1000;
    do {
        posZ = *(u32 *)g_WorldMapPlayerPosition;
        posX = posZ;
        posY = g_WorldMapPlayerPositionY;
        posZ = g_WorldMapPlayerPositionZ;
        g_WorldMapFadeLevel = 0x78;
        g_WorldMapCameraTarget[0] = posX;
        g_WorldMapScrollTargetX = posX;
        g_WorldMapCameraTargetY = posY;
        g_WorldMapScrollTargetY = posY;
        g_WorldMapCameraTargetZ = posZ;
        g_WorldMapScrollTargetZ = posZ;
        pActor->command = 1;
    } while (0);
    pActor->state = 0;
    return 1;
}
*/

/* 99.69%, round 325 (was 96.01%). objdiff reports 768 bytes on both sides, so the
   body below is exactly retail's size, and the WHOLE function now matches byte
   for byte except ten instructions of the rand tail, where only the register
   NAMES differ (identical instruction multiset).
   Round 325 took 96.01 -> 99.69 with four finds, all in the body:
   - case 1 of the first switch: the angle base landed in $v1 (retail: $a0),
     which dragged the 0x800000 constant into $v0 and scrambled the rest of the
     case. What fixes it is that `pAngle = g_WorldMapCameraRot; pActor->state = 1;`
     is written TWICE, once per arm of an if/else on g_WorldMapViewRotationY:
     98.65 -> 99.69, and 0x800000 then takes $v1 as retail has it. It reads like
     debris and is load-bearing - the same family as the repo's empty do-while
     and unreachable-use levers. Without it case 1 is 98.65.
   - the case-1 constants land in retail's positions only with TWO SEQUENTIAL
     do-blocks, not one: block 1 = command, camRot; block 2 = ViewY, ViewZ,
     stateIndex. A do-block's preheader is where cc1's loop.c hoists one loop
     invariant constant per register, so ONE block puts `li v0,16` and
     `lui v1,0x80` both at its head, while two put `li v0,16` before the command
     store and `lui v1,0x80` after the camRot store - retail's exact placement.
     `stateIndex = 0x800000` must be INSIDE block 2: its `lui` is hoisted above
     the two volatile stores while its `sw` stays last.
   - the pStateFrames update wants the EMBEDDED assignment, like its unk60 twin:
     nValue = (s32)(pActor->pStateFrames = (void *)((s32)pActor->pStateFrames + (nDelta >> 4)));
     Split as `nValue = ...; pActor->pStateFrames = nValue;` the sum takes a third
     register, the sra runs before the sw, and it parks at 97.10.
   - the two `pAngle[0] << 12` / `g_WorldMapViewRotationY << 12` shifts want
     SEPARATE locals (nAngle); sharing `nValue` ties them to one register and the
     second sll lands in $a3 instead of retail's $v0.
   Still needed from earlier rounds, do not undo: the pStateFrames/pStateIds
   stores stay volatile (88.41 without), the three case-1 stores stay volatile
   (86.95 without the do-block), g_WorldMapCameraRot is declared as an unbounded
   ARRAY so cc1 keeps its address in a register (round 316), `pScratch` is
   declared before `pActor` so its save lands in retail's prologue slot, and the
   second switch needs its `case 0: break;` (four nodes make retail's chain).
   Residual: the rand tail. Retail assigns nValue=$a1, the D_8009BD42 base=$a3,
   the reloaded scratchpad value=$a2; we get $a3/$a2/$a1 - a cyclic rotation of
   the same three registers over an identical instruction stream, i.e. global
   allocation's tie-break, which it decides from the whole function's allocno
   order. Measured dead in round 325: a pointer local for D_8009BD42 (declared at
   the top or at the use), `D_8009BD42[0] = D_8009BD42[0] + nValue`, base-first
   vs index-first scratch addressing, a separate local for the >>13 shift (764
   bytes, one instruction short), a separate variable for the scratch reload (760
   bytes), a do-block around the tail (98.39, rotates the other way), a do-block
   around only the three stores (96.38), `if (1) { }` (no change), and two local
   declaration orders (no change).
   Two permuter runs from this body (2011 then 2388 iterations) got base=60 and
   found nothing further; the earlier run's only find was the if/else above.
   gcc 2.7.2-cdk is NOT this unit's preset: this body scores 90.11 under it and
   the unit's matched neighbours fall to 96.27-99.67.
   ROUND 398 re-measured 99.69 (768 bytes = retail's) and now PROVES the tail
   is not a spelling question. The -da dump of this unit (mips-linux-gnu-cpp
   -DSKIP_ASM, then tools/gcc-2.7.2-psx/cc1 with the unit flags plus -da) prints
   "79 conflicts: 73 79 2 3 4 5 6 29 64 65 66" for the nValue pseudo, i.e. v0,
   v1, a0, a1 AND a2 are all marked conflicting, so find_reg has only a3 left and
   MUST put nValue there, where retail has a1. Retail's RTL therefore differs
   in a way no shape of this tail reaches, while the RTL we already have is
   retail's down to mfhi going straight into the value, sra a0,13 for the second
   shift and li v0,1 in between. Also measured dead in round 398: the ONE
   statement form `nValue = rand() % (unk7C >> 12) - (unk7C >> 13);` splits the
   value into two pseudos (mfhi v1, subu a3,v1,a0) and stays 99.69 with an extra
   move, so the two statement split in the body below is the shape retail has -
   keep it. -da recipe: it is also the cheap way to settle any rotation here.
   measure: tools/scripts/diff_func.sh world_map/main/misc6 func_80080600
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080600);
/*
int func_80080600(int index) {
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 *pAngle;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 nValue;
    s32 nDelta;
    s32 nAngle;

    switch (pActor->command) {
    case 1:
        if (g_WorldMapViewRotationY) {
            pAngle = g_WorldMapCameraRot;
            pActor->state = 1;
        } else {
            pAngle = g_WorldMapCameraRot;
            pActor->state = 1;
        }
        do {
            pActor->command = 0;
            *(volatile s16 *)pAngle = 0x10;
        } while (0);
        do {
            *(volatile s16 *)&g_WorldMapViewRotationY = 0x8E0;
            *(volatile u16 *)&g_WorldMapViewRotationZ = 0;
            pActor->stateIndex = 0x800000;
        } while (0);
        nAngle = pAngle[0] << 12;
        *(volatile void **)&pActor->pStateFrames = (void *)nAngle;
        *(volatile void **)&pActor->pStateIds = (void *)nAngle;
        g_WorldMapCameraHeight = 0x800000;
        nAngle = g_WorldMapViewRotationY << 12;
        pActor->unk60[0] = nAngle;
        pActor->scale = nAngle;
        break;
    case 2:
        pActor->state = 2;
        pActor->command = 0;
        pActor->unk7C = 0x8000;
        break;
    case 3:
        pActor->state = 3;
        pActor->command = 0;
        pActor->unk7C = 0x80000;
        break;
    }
    if (g_WorldMapRotationMatrixValid == 0) {
        func_80096F18(&g_WorldMapPathFrame, g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot[0]);
    }
    switch (pActor->state) {
    case 1:
        nDelta = (pActor->stateIndex = pActor->stateIndex - 0x10000);
        if (nDelta <= 0x2FFFFF) {
            pActor->stateIndex = 0x300000;
        }
        nDelta = pActor->stateIndex - g_WorldMapCameraHeight;
        if (nDelta != 0) {
            g_WorldMapCameraHeight += nDelta >> 5;
        }
        nDelta = (s32)pActor->pStateIds - 0x4000;
        *(volatile void **)&pActor->pStateIds = (void *)nDelta;
        if (nDelta < -0x100000) {
            pActor->pStateIds = (void *)-0x100000;
        }
        nDelta = (s32)pActor->pStateIds - (s32)pActor->pStateFrames;
        if (nDelta != 0) {
            nValue = (s32)(pActor->pStateFrames = (void *)((s32)pActor->pStateFrames + (nDelta >> 4)));
            g_WorldMapCameraRot[0] = nValue >> 12;
        }
        nDelta = (pActor->scale = pActor->scale - 0x10000);
        if (nDelta <= 0x2DFFFF) {
            pActor->scale = 0x2E0000;
        }
        nDelta = pActor->scale - pActor->unk60[0];
        if (nDelta != 0) {
            nValue = (pActor->unk60[0] = pActor->unk60[0] + (nDelta >> 5));
            g_WorldMapViewRotationY = nValue >> 12;
        }
        break;
    case 0:
        break;
    case 2:
        break;
    case 3:
        pActor->unk7C -= 0x4000;
        if (pActor->unk7C < 0x1000) {
            pActor->unk7C = 0x1000;
            pActor->state = 0;
        }
        break;
    }
    nValue = rand() % (pActor->unk7C >> 12);
    nValue -= pActor->unk7C >> 13;
    *(s16 *)(0xA2 + pScratch) = (s16)nValue;
    D_8009BD42[0] += nValue;
    D_8009BD4A += *(u16 *)(0xA2 + pScratch);
    return 1;
}
*/
