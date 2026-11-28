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

// the pitch (the first word of the camera rotation) a scrolling camera actor stops
// raising its view rotation at (WorldMapScrollCameraActor, in every arm that ramps it)
#define WORLD_MAP_CAMERA_ROT_MAX 0x80

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

// the fade the mode initialisers play before the map is drawn: this many frames of
// WorldMapDrawFadeFrames, lifting the shade by WORLD_MAP_INIT_FADE_COLOR_STEP each frame
#define WORLD_MAP_INIT_FADE_FRAMES 64
#define WORLD_MAP_INIT_FADE_COLOR_STEP 4

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
extern s16 g_WorldMapViewRotationY[];
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
extern void WorldMapDrawFadeFrames(int, int, int, int);
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
extern int WorldMapScrollCameraActor(int);
extern int WorldMapInitGroup1Actor(int);
extern int func_8007EE34(int);
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
extern void WorldMapInitSpriteGroupsAtSlotPosition(void);
extern void func_80080A28(void);
extern void WorldMapUpdateFadeGroups(void);
extern int WorldMapUpdateSlotActorScriptState(int);
extern void WorldMapStartSlot(s32, s32);
extern void SoundPlaySoundEffectOnFreePair(s32);
extern int func_80080578(int);
extern int WorldMapUpdateCameraSlot(int);
extern int WorldMapInitActorScript(int);
extern s16 g_WorldMapCameraRot[];
// Second declaration with an asm label: a bare scalar read/write leaves the address to the
// assembler ($at / the destination register) at each site, where the array form keeps it in one
// register - retail re-forms it per access.
extern s16 g_WorldMapCameraRotScalar __asm__("g_WorldMapCameraRot");
extern u16 g_WorldMapViewRotationZ;
extern u16 g_WorldMapPathFrame;
extern u16 D_8009BD42[];
extern u16 D_8009BD4A;
extern s32 g_WorldMapCameraHeight;
extern int rand(void);
extern void WorldMapBuildPathFrame(void *, void *, s32, void *);

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
        pActor->command = 0;
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
    WorldMapDrawFadeFrames(WORLD_MAP_INIT_FADE_FRAMES, 0, WORLD_MAP_INIT_FADE_COLOR_STEP, 2);
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
    WorldMapAddSlot(func_8007E450, WorldMapScrollCameraActor);
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
    g_NextMapRotationY = (u16)g_WorldMapViewRotationY[0];
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

   ROUND 420 re-measured 97.97 (148 bytes) and read the -dl dispositions for this
   body (cpp -DSKIP_ASM, then tools/gcc-2.7.2-psx/cc1 with the unit flags plus
   -da -dl): `73 in 6  75 in 4  76 in 6  77 in 4  78 in 5  79 in 3`, with
   `Hard regs used: 2 3 4 5 6`, `Registers live at start: 4 29 30` and NO
   `N regs to allocate` line - global-alloc never runs for this single-block
   function, so the hand-out is local-alloc's alone. 73 is pActor (the
   `reg/v:SI` plus of the g_WorldMapSlots load 76 and the `index << 7` 75, tied
   to 76 by addsi3), 77/78/79 are posX/posY/posZ (loads at insns 35/38/41), and
   the pair that differs is 73<->79: 73 is used 7 times over 20 insns, 79 10
   times over 18 insns, so the ascending-n_refs hand-out (the 0x1000 constant 80
   first, then 73, then 77/78/79) IS retail's $v1/$a0/$a1/$a2, and our compile
   allocated 79 before 73 instead - a quantity-set difference from posZ's extra
   REG_DEP_ANTI refs, not a spelling of this body. Shapes re-measured this round:
   the three values as initialised declarations inside the do-block (96.62, posX
   takes $v1), the chained
   `g_WorldMapScrollTargetX = g_WorldMapCameraTarget[0] = *(u32 *)g_WorldMapPlayerPosition;`
   form for all three (85.68 - it also moves the FadeLevel store above the
   loads), and the body without the posZ alias (96.62).
*/
// 97.97% (148 bytes = retail's 37 insns), round 434. Body unchanged from round 420:
//    the store tail is retail's exactly; the ONLY delta is the pActor register
//    (retail $v1, ours $a2) and the consequent posZ swap ($a2 / $v1). All 24
//    differing lines are that one rotation. The -da/-dl analysis in the old note
//    below (73 = pActor tied to the slots load, 79 = posZ, local-alloc handout)
//    still stands: the value locals are always allocated before the pointer, in
//    every spelling measured.
//    Round 434 measured four new shapes, none better: the same body WITHOUT the
//    do-block (36.03 - the command store's li sinks and the tail loses retail's
//    delay slot, so the do-block is load-bearing); pActor replaced by
//    'g_WorldMapSlots[index].field' at every site, no pointer local (80.41 - cse
//    keeps one add but the load lands in v0 and the sll moves after it); the whole
//    statement list inside the do-block with the declaration outside (71.16 - the
//    slots load sinks into the block); the plain 'posX = *(u32 *)g_WorldMapPlayerPosition;'
//    without the posZ alias (96.62 - posX/posY then take v1/a0 and the loads keep
//    retail's order, so the alias is what buys a0/a1 for X/Y but costs the pActor
//    register). Preset fully screened: 2.6.0 48.62, 2.7.2-cdk 92.03.
//    PARKED - no spelling of this body moves the rotation.
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E450);



// Retail updates all three components of the camera target as one vector here, and the
// shared base is what keeps the Y/Z loads below the X store (spelled as separate scalars
// the scheduler hoists the Z group above it, 97.56%); the volatile stores pin the state
// and scale writes where sched2 would otherwise hoist them above the CameraRot group.
s32 WorldMapScrollCameraActor(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 nState;
    s32 nRandom;

    nState = *(u16 *)&pActor->command - 1;
    switch (nState) {
    case 0:
        g_WorldMapCameraHeight = 0x500000;
        pActor->command = 0;
        g_WorldMapCameraRot[0] = 0x150;
        g_WorldMapViewRotationY[0] = 0xF50;
        g_WorldMapViewRotationZ = 0;
        break;
    case 1:
        g_WorldMapCameraHeight = 0x4D0000;
        pActor->command = 0;
        g_WorldMapCameraRot[0] = 0xC60;
        g_WorldMapViewRotationY[0] = 0x380;
        g_WorldMapViewRotationZ = 0;
        break;
    case 2:
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot[0] = 0x30;
        g_WorldMapViewRotationY[0] = 0x160;
        g_WorldMapViewRotationZ = 0;
        break;
    case 3:
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot[0] = 0x30;
        g_WorldMapViewRotationY[0] = 0x960;
        g_WorldMapViewRotationZ = 0;
        break;
    case 4:
        pActor->state = 1;
        pActor->command = 0;
        pActor->scale = -0x8000;
        break;
    case 5:
        pActor->state = 3;
        pActor->unk7C = 0x1000;
        g_WorldMapCameraHeight = 0x200000;
        pActor->command = 0;
        g_WorldMapCameraRot[0] = -0x58;
        g_WorldMapViewRotationY[0] = 0xE58;
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
        pActor->scale = -0x8000;
        break;
    case 16:
        pActor->state = 0x10;
        pActor->unk7C = 0x1000;
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot[0] = 0xE0;
        g_WorldMapViewRotationY[0] = 0x798;
        g_WorldMapViewRotationZ = 0;
        break;
    case 23:
        pActor->state = 0x18;
        pActor->command = 0;
        pActor->scale = -0x8000;
        break;
    }
    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        WorldMapBuildPathFrame(&g_WorldMapPathFrame, g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot[0]);
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
            *(volatile s16 *)&pActor->state = 2;
            pActor->unk7C = 0x10000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            g_WorldMapScrollZ += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY[0] -= 8;
        g_WorldMapCameraRotScalar += pActor->scale >> 12;
        *(volatile s32 *)&pActor->scale = *(volatile s32 *)&pActor->scale + 0x300;
        g_WorldMapCameraHeight += 0x8000;
        if (g_WorldMapCameraRot[0] > WORLD_MAP_CAMERA_ROT_MAX) {
            *(volatile s16 *)&g_WorldMapCameraRotScalar = WORLD_MAP_CAMERA_ROT_MAX;
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
        g_WorldMapCameraTarget[1] += 0xFF80;
        g_WorldMapCameraTarget[2] += 0xFFF9AA80;
        if (g_WorldMapCameraTarget[0] <= 0x1497FFF && g_WorldMapCameraTarget[2] <= 0x4AF1FFF) {
            *(volatile s32 *)&g_WorldMapCameraTarget[0] = 0x1498000;
            g_WorldMapCameraTarget[2] = 0x4AF2000;
            pActor->state = 4;
            pActor->scale = 0x20000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFB3C00;
            g_WorldMapScrollZ += 0xFFF9AA80;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapCameraHeight += 0x20000;
        g_WorldMapViewRotationY[0] += 0x20;
        g_WorldMapCameraRotScalar -= 4;
        break;
    case 4:
        g_WorldMapViewRotationY[0] += pActor->scale >> 12;
        pActor->scale -= 0x800;
        if (pActor->scale < 0) {
            pActor->scale = 0;
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
        if (g_WorldMapCameraTarget[0] <= 0x1F9DFFF && g_WorldMapCameraTargetZ > 0x5998000) {
            g_WorldMapCameraTarget[0] = 0x1F9E000;
            g_WorldMapCameraTargetZ = 0x5998000;
            *(volatile s16 *)&pActor->state = 0x11;
            pActor->unk7C = 0x8000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            g_WorldMapScrollZ += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY[0] -= 8;
        g_WorldMapCameraRotScalar += pActor->scale >> 12;
        *(volatile s32 *)&pActor->scale = *(volatile s32 *)&pActor->scale + 0x300;
        g_WorldMapCameraHeight += 0x8000;
        if (g_WorldMapCameraRot[0] > WORLD_MAP_CAMERA_ROT_MAX) {
            *(volatile s16 *)&g_WorldMapCameraRotScalar = WORLD_MAP_CAMERA_ROT_MAX;
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
            *(volatile s16 *)&pActor->state = 0x19;
            pActor->unk60[0] = 0x10000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            g_WorldMapScrollZ += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY[0] -= 8;
        g_WorldMapCameraRotScalar += pActor->scale >> 12;
        *(volatile s32 *)&pActor->scale = *(volatile s32 *)&pActor->scale + 0x300;
        g_WorldMapCameraHeight += 0x8000;
        if (g_WorldMapCameraRot[0] > WORLD_MAP_CAMERA_ROT_MAX) {
            *(volatile s16 *)&g_WorldMapCameraRotScalar = WORLD_MAP_CAMERA_ROT_MAX;
        }
        break;
    case 25:
        g_WorldMapViewRotationY[0] -= pActor->unk60[0] >> 12;
        pActor->unk60[0] -= 0x200;
        if (pActor->unk60[0] < 0) {
            pActor->unk60[0] = 0;
        }
        break;
    }
    nRandom = rand() % (pActor->unk7C >> 12) - (pActor->unk7C >> 13);
    *(u16 *)(pScratch + 0xA2) = nRandom;
    D_8009BD42[0] += nRandom;
    D_8009BD4A += *(u16 *)(0xA2 + pScratch);
    return 1;
}





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
    pActor->state = 0;
    *(volatile s32 *)&pActor->dirY = 0;
    *(volatile s32 *)&pActor->dirZ = WORLD_MAP_SLOT_FACING_COS;
    *(volatile s32 *)&pActor->stateIndex = g_WorldMapAreaSpawnPositions[nIndex * 4] << 12;
    *(volatile void **)&pActor->pStateIds = (void *)(D_8009A678[nIndex * 4] << 12);
    *(volatile s32 *)&pActor->x = *(u32 *)g_WorldMapPlayerPosition - pActor->dirX * WORLD_MAP_SLOT_SPAWN_OFFSET;
    *(volatile s32 *)&pActor->y = g_WorldMapPlayerPositionY;
    pActor->z = g_WorldMapPlayerPositionZ - pActor->dirZ * WORLD_MAP_SLOT_SPAWN_OFFSET;
    return 1;
}

// 99.47% objdiff (2676 bytes vs retail 2680, 669 vs 670 insns), round 461.
// Retail func_8007EE34 is 0xa78 = 2680 bytes / 670 instructions. Structure is complete
// (all three jump tables, both call families, every constant); the three jtbl_ vs
// .rodata table operands are expected in this unit and link identically.
// What moved the number this round (98.42 -> 99.47, 2684 -> 2676 bytes):
//  - the g_WorldMapPlayerPosition read in case 3/23 is a plain `*(u32 *)` again: a
//    volatile lvalue makes cc1 form the address with `la` (lui + addiu + lw = 3 insns)
//    where retail has the 2-insn `lw $a0,%lo(sym)($a0)` macro shape.
//  - an empty `do { } while (0);` after the command/state stores in case 3 and 23 is the
//    block boundary that keeps that load inside the merged tail instead of letting
//    sched2 hoist it into both case blocks (retail's exact 2680 bytes at that point).
//  - case 16's `pPos` declaration moved inside the `if`, and case 2's pPos inlined as
//    `((SVECTOR *)(pScratch + 0xA0))` (permuter find), removed the state-16 load-delay
//    nop: 2680 -> 2676.
// Residual = ONE missing instruction plus moves/swaps. Retail loads dirX (0x38) in EACH
// of the case 3 / case 23 blocks, ahead of the state constant, so cross-jump's common
// suffix starts at `sh zero,0x4` and the position load is shared; our compile leaves the
// dirX load in the merged tail after the two stores, i.e. our common suffix starts one
// block earlier. Everything else is 1-for-1: state 0 loads z (0x30) before pStateIds
// (0x54) in retail; `addiu s0,s4,0xA0` is materialised after the second test in retail's
// states 2/16 (ours steals it into the first bnez delay slot); the third switch's case
// 2/3 arm stores pVec1->vy (0x14) last, in the OuterProduct12 jal delay slot.
// Measured non-fixes this round: `s32 nDirX = pActor->dirX;` hoisted above the stores,
// with the do-while (2720) and without it (2696 - the whole chain is then in both case
// blocks); the position load left non-volatile with no do-while (2692: sched2 hoists
// BOTH loads into each case block); the volatile position load (2684). Two permuter runs
// from this body: base 280 -> 220 (the case-2 inline-cast find) then PERMUTER_NONE.
// Preset ruled out earlier: 2.7.2-cdk gives 83.56 on this body.

// objdiff 99.36% (2680 bytes / 670 insns = retail's exact size), round 474.
// Structure is complete: all three jump tables, both call families, every constant.
// What closed the last 4 bytes this round (2676 -> 2680):
//  - a dirX local assigned at the TOP of the case 3 and case 23 arms (nDirX = pActor->dirX)
//    is the extra load retail keeps in EACH arm (lw v1,0x38(s3) before the two stores);
//    reading pActor->dirX inline in the x expression lets cross-jump sink one load into
//    the merged tail and the function loses that instruction. The do-while stays: without
//    it sched2 hoists BOTH loads into both arms (2688/2692).
//  - the case 0 test spelled pActor->z > (s32)pActor->pStateIds (not pStateIds < z) gives
//    retail's load order: lw v1,0x30(s3) before lw v0,0x54(s3).
//  - the third switch's case 2/3 arm stores pVec1 vz, vx, vy in that order (retail: sw
//    zero,0x18, sw zero,0x10, then the jal delay slot sw 0x14).
// Residual = ONE displaced pair. Retail loads the case 3/23 tail's g_WorldMapPlayerPosition
// (lui a0 + lw a0,0) directly after the two sh stores and BEFORE the dirX*13952 chain; our
// compile schedules the same pair six instructions later. Registers already match (pos in
// a0, dirX in v1). Measured this round, all 2680 bytes: a separate position-load local in
// the tail puts the load first but then swaps the two temporaries (pos v1, dirX a0, plus a
// second dirX load) = 99.10; that local plus a second do-while = 99.10; volatile position
// load = 2684/99.65; do-while above the stores = 2684/98.90; no do-while at all =
// 2688/99.06; indexing the array as a byte (lbu, wrong width) lands the load first and
// scores 99.87 with that one instruction as the only difference, so the displacement is
// sched2's SI-mode vs QI-mode priority, not source order. Permuter (3 min, -j 2):
// PERMUTER_NONE base 120 best 120. Preset ruled out earlier: 2.7.2-cdk gives 83.56.
// ROUND 547 AUDIT: re-measured 99.36% / 2680 bytes (retail's exact size), 670 = retail's insn count; residual unchanged (the displaced g_WorldMapPlayerPosition load pair in the shared case 3/23 tail). NEW THIS ROUND - the displacement is NOT sched2 and NOT statement order: compiling this unit's own .i with the unit cc1 plus -fno-schedule-insns2, and separately -fno-schedule-insns, leaves the load in the SAME late slot (after the six dirX*13952 insns), so the late position is the RTL emission order of the subtraction: for A - B*C gcc emits the multiply's insn sequence before the leaf load. Retail keeps a separate source statement for the position read, which is why a 's32 nPosX' local read after the two stores fixes the ORDER (the load lands in retail's slot, the rest of the tail identical) - but it costs a two-way register rotation: pos takes v1 and dirX a0 where retail has pos a0 and dirX v1, each arm reloading dirX (v2 form). Measured this round, all 2680 bytes: pos local after the stores 99.10 (207 differing lines, the rotation only); pos local declared before nDirX 99.10 (byte-identical to it); s32 load cast 99.10 (identical); pos local plus inline pActor->dirX twice in the x expression 98.38 / 2676 (loses the per-arm dirX load); pos local read BEFORE the two stores 98.26 / 2688. So the two residuals are mutually exclusive as in the lever a-loads-source-position-switches-the-whole-allocation-family: order (99.10, rotation) vs registers (99.36, displaced load). No -da/-dl/-dg dump is available in tools/gcc-2.7.2-psx/cc1 (all dump flags produce no file and empty stderr), so the rotation cannot be checked with the floor_log2(refs)*refs/live arithmetic on this toolchain.
/*
int func_8007EE34(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapGroup *pGroup = &g_WorldMapGroups->groups[1];
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 nCommand;
    s32 nResult;
    s32 nIndex;
    s32 nDirX;

    nResult = 1;
    if (pActor->command != 0) {
        pActor->dirX = -WORLD_MAP_SLOT_FACING_SIN;
        pActor->dirY = 0;
        pActor->dirZ = WORLD_MAP_SLOT_FACING_COS;
        nIndex = g_WorldMapAreaIndex;
        pActor->stateIndex = g_WorldMapAreaSpawnPositions[nIndex * 4] << 12;
        pActor->pStateIds = (u16 *)(D_8009A678[nIndex * 4] << 12);
    }
    nCommand = *(u16 *)&pActor->command - 1;
    switch (nCommand) {
    case 0:
        *(volatile s16 *)&pActor->command = 0;
        *(volatile s16 *)&pActor->state = 0;
        *(volatile s32 *)&pActor->x = *(u32 *)g_WorldMapPlayerPosition - pActor->dirX * WORLD_MAP_SLOT_SPAWN_OFFSET;
        *(volatile s32 *)&pActor->y = g_WorldMapPlayerPositionY;
        pActor->z = g_WorldMapPlayerPositionZ - pActor->dirZ * WORLD_MAP_SLOT_SPAWN_OFFSET;
        break;
    case 1:
        *(volatile s16 *)&pActor->command = 0;
        *(volatile s16 *)&pActor->state = 0;
        *(volatile s32 *)&pActor->x = *(u32 *)g_WorldMapPlayerPosition - pActor->dirX * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + pActor->dirX * 0x2D00;
        pActor->z = g_WorldMapPlayerPositionZ - pActor->dirZ * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + pActor->dirZ * 0x2D00;
        break;
    case 2:
        *(volatile s16 *)&pActor->command = 0;
        *(volatile s16 *)&pActor->state = 0;
        *(volatile s32 *)&pActor->x = *(u32 *)g_WorldMapPlayerPosition - pActor->dirX * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + pActor->dirX * 0x1E00;
        pActor->z = g_WorldMapPlayerPositionZ - pActor->dirZ * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + pActor->dirZ * 0x1E00;
        break;
    case 3:
        nDirX = pActor->dirX;
        pActor->command = 0;
        pActor->state = 1;
        do { } while (0);
        pActor->x = *(u32 *)g_WorldMapPlayerPosition - nDirX * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + nDirX * 0x3300;
        pActor->z = g_WorldMapPlayerPositionZ - pActor->dirZ * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + pActor->dirZ * 0x3300;
        break;
    case 4:
        pActor->state = 2;
        *(volatile s32 *)&pActor->dirX = -0x988;
        *(volatile s32 *)&pActor->dirY = 0x227;
        pActor->command = 0;
        *(volatile s32 *)&pActor->dirZ = -0xCAB;
        *(volatile s32 *)&pActor->x = 0x1F9E000;
        pActor->z = 0x5998000;
        break;
    case 15:
        pActor->command = 0;
        pActor->state = 0x10;
        *(volatile s32 *)&pActor->x = pActor->dirX * 0x3300 + 0x56F2000;
        pActor->z = pActor->dirZ * 0x3300 + 0x7F2C000;
        break;
    case 23:
        nDirX = pActor->dirX;
        pActor->command = 0;
        pActor->state = 0x18;
        do { } while (0);
        pActor->x = *(u32 *)g_WorldMapPlayerPosition - nDirX * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + nDirX * 0x3300;
        pActor->z = g_WorldMapPlayerPositionZ - pActor->dirZ * WORLD_MAP_SLOT_SPAWN_OFFSET
                    + pActor->dirZ * 0x3300;
        break;
    }
    pActor->x += pActor->dirX * 128;
    pActor->y += pActor->dirY * 128;
    pActor->z += pActor->dirZ * 128;

    switch (pActor->state) {
    case 0:
        if (pActor->x < pActor->stateIndex && pActor->z > (s32)pActor->pStateIds) {
            pActor->x = pActor->stateIndex;
            pActor->z = (s32)pActor->pStateIds;
            WorldMapStartSlot(4, 2);
            WorldMapStartSlot(5, 2);
            WorldMapStartSlot(6, 2);
            WorldMapStartSlot(7, 2);
            WorldMapStartSlot(8, 2);
        }
        break;
    case 1: {
        SVECTOR *pPos = (SVECTOR *)(pScratch + 0xA0);

        if (pActor->x <= 0x1F9DFFF && pActor->z > 0x5998000) {
            pActor->x = 0x1F9E000;
            pActor->z = 0x5998000;
            pActor->state = 0x40;
            pPos->vx = pActor->x >> 12;
            pPos->vy = pActor->y >> 12;
            pPos->vz = pActor->z >> 12;
            WorldMapInitSpriteGroupSprites(0x20, (SVECTOR *)((u32)pScratch | 0xA0), NULL);
        }
        break;
    }
    case 2: {
        if (pActor->y > -0x80000) {
            ((SVECTOR *)(pScratch + 0xA0))->vx = 0x1498;
            ((SVECTOR *)(pScratch + 0xA0))->vz = 0x4AF2;
            ((SVECTOR *)(pScratch + 0xA0))->vy = WorldMapGetTilePlaneHeight(0x1498000, 0x4AF2000) >> 12;
            WorldMapInitSpriteGroupSprites(0x25, (SVECTOR *)(pScratch + 0xA0), NULL);
            WorldMapInitSpriteGroupSprites(0x26, (SVECTOR *)(pScratch + 0xA0), NULL);
            WorldMapInitSpriteGroupSprites(0x27, (SVECTOR *)(pScratch + 0xA0), NULL);
            pActor->state = 3;
        }
        break;
    }
    case 3:
        if (pActor->y > 0x100000) {
            pActor->x -= pActor->dirX * 128;
            pActor->y -= pActor->dirY * 128;
            pActor->z -= pActor->dirZ * 128;
            pGroup[2].hidden = 1;
            pGroup[1].hidden = 1;
            pGroup->hidden = 1;
            pActor->state = 0x41;
        }
        break;
    case 16: {
        if (pActor->x <= 0x1F9DFFF && pActor->z > 0x5998000) {
            SVECTOR *pPos = (SVECTOR *)(pScratch + 0xA0);

            pActor->state = 0x11;
            pPos->vx = 0x1F9E;
            pPos->vz = 0x5998;
            pPos->vy = pActor->y >> 12;
            WorldMapInitSpriteGroupSprites(0x20, pPos, NULL);
            WorldMapInitSpriteGroupSprites(0x21, pPos, NULL);
        }
        break;
    }
    case 17:
        if (pActor->x <= 0x199CFFF && pActor->z > 0x6367000) {
            pActor->x = 0x199D000;
            pActor->z = 0x6367000;
            pGroup[2].hidden = 1;
            pGroup[1].hidden = 1;
            pGroup->hidden = 1;
            WorldMapStartSlot(4, 5);
            WorldMapStartSlot(5, 5);
            WorldMapStartSlot(6, 5);
            WorldMapStartSlot(7, 5);
            WorldMapStartSlot(8, 5);
            nResult = 3;
        }
        break;
    case 24:
        if (pActor->x <= 0x1EB4FFF && pActor->z > 0x5EE6000) {
            pGroup[2].hidden = 1;
            pGroup[1].hidden = 1;
            pGroup->hidden = 1;
            WorldMapStartSlot(4, 2);
            WorldMapStartSlot(5, 2);
            WorldMapStartSlot(6, 2);
            WorldMapStartSlot(7, 2);
            WorldMapStartSlot(8, 2);
            pActor->state = 0x41;
        }
        break;
    case 64:
        pActor->x = 0x1F9E000;
        pActor->z = 0x5998000;
        break;
    case 65:
        nResult = 3;
        break;
    }
    WorldMapWrapPosition((VECTOR *)&pActor->x);
    pGroup->x = pActor->x >> 12;
    pGroup->y = pActor->y >> 12;
    pGroup->z = pActor->z >> 12;

    switch (pActor->state) {
    case 0:
    case 1:
    case 16:
    case 25: {
        MATRIX *pRotMat = (MATRIX *)(pScratch + 0xF0);
        MATRIX *pTmp = (MATRIX *)(pScratch + 0x110);
        VECTOR *pScale = (VECTOR *)pScratch;
        SVECTOR *pPos = (SVECTOR *)(pScratch + 0xA0);
        SVECTOR *pAngle = (SVECTOR *)(pScratch + 0xA8);

        pRotMat->m[0][0] = WORLD_MAP_SLOT_FACING_COS;
        pRotMat->m[0][1] = 0;
        pRotMat->m[0][2] = WORLD_MAP_SLOT_FACING_SIN;
        pRotMat->m[1][0] = 0;
        pRotMat->m[1][1] = -0x1000;
        pRotMat->m[1][2] = 0;
        pRotMat->m[2][0] = -WORLD_MAP_SLOT_FACING_SIN;
        pRotMat->m[2][1] = 0;
        pRotMat->m[2][2] = WORLD_MAP_SLOT_FACING_COS;
        pGroup->rot.vz = (pGroup->rot.vz + WORLD_MAP_SLOT_ANGLE_STEP) & WORLD_MAP_ANGLE_MASK;
        RotMatrixYXZ(&pGroup->rot, pTmp);
        MulMatrix0((MATRIX *)((u32)pScratch | 0xF0), pTmp, &pGroup->mat);
        pScale->vx = pScale->vy = 0x800;
        pScale->vz = 0x1800;
        ScaleMatrix(&pGroup->mat, pScale);
        pPos->vx = pActor->x >> 12;
        pPos->vy = pActor->y >> 12;
        pPos->vz = pActor->z >> 12;
        pAngle->vx = pAngle->vz = 0;
        pAngle->vy = ratan2(WORLD_MAP_SLOT_FACING_SIN, WORLD_MAP_SLOT_FACING_COS) & WORLD_MAP_ANGLE_MASK;
        WorldMapInitSpriteGroupSprites(0x22, (SVECTOR *)((u32)pScratch | 0xA0), (SVECTOR *)((u32)pScratch | 0xA8));
        break;
    }
    case 4:
    case 17:
    case 65:
        WorldMapDeactivateSpriteGroup(0x22);
        return nResult;
    case 2:
    case 3: {
        VECTOR *pVec0 = (VECTOR *)pScratch;
        VECTOR *pVec1 = (VECTOR *)(pScratch + 0x10);
        VECTOR *pVec2 = (VECTOR *)(pScratch + 0x20);
        MATRIX *pMat = (MATRIX *)(pScratch + 0x150);
        MATRIX *pRotMat = (MATRIX *)(pScratch + 0xF0);
        MATRIX *pTmp = (MATRIX *)(pScratch + 0x110);
        VECTOR *pScale = (VECTOR *)pScratch;
        SVECTOR *pPos = (SVECTOR *)(pScratch + 0xA0);
        SVECTOR *pAngle = (SVECTOR *)(pScratch + 0xA8);

        pVec0->vx = 0x988;
        pVec0->vy = -0x227;
        pVec0->vz = -0xCAB;
        pVec1->vz = 0;
        pVec1->vx = 0;
        pVec1->vy = 0x1000;
        OuterProduct12(pVec1, pVec0, pVec2);
        VectorNormal(pVec2, pVec2);
        OuterProduct12(pVec0, pVec2, pVec1);
        VectorNormal(pVec1, pVec1);
        pMat->m[0][0] = pVec2->vx;
        pMat->m[0][1] = pVec2->vy;
        pMat->m[0][2] = pVec2->vz;
        pMat->m[1][0] = pVec1->vx;
        pMat->m[1][1] = pVec1->vy;
        pMat->m[1][2] = pVec1->vz;
        pMat->m[2][0] = pVec0->vx;
        pMat->m[2][1] = pVec0->vy;
        pMat->m[2][2] = pVec0->vz;
        WorldMapExtractRotationFromMatrix(pMat, pAngle);
        TransposeMatrix(pMat, pRotMat);
        pGroup->rot.vz = (pGroup->rot.vz + WORLD_MAP_SLOT_ANGLE_STEP) & WORLD_MAP_ANGLE_MASK;
        RotMatrixYXZ(&pGroup->rot, pTmp);
        MulMatrix0(pRotMat, pTmp, &pGroup->mat);
        pScale->vx = pScale->vy = 0x800;
        pScale->vz = 0x1800;
        ScaleMatrix(&pGroup->mat, pScale);
        pPos->vx = pActor->x >> 12;
        pPos->vy = pActor->y >> 12;
        pPos->vz = pActor->z >> 12;
        WorldMapInitSpriteGroupSprites(0x22, (SVECTOR *)((u32)pScratch | 0xA0), pAngle);
        break;
    }
    }
    return nResult;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EE34);





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
   Round 396 re-measured this body live at 93.41 and tried three new shapes:
   `int result = 3;` assigned right after the calls and returned at the end
   (88.66 - cse folds it back to a plain return and the li sinks anyway), the
   same plus nX/nZ locals holding the two hoisted constants in retail's load
   order (88.66 - cse substitutes the constants into the stores and deletes the
   sets). cdk -O2 -G8 also 88.66 on that body, gcc 2.6.0 81.07, -G0 does not
   compile, so the preset is right. Root cause unchanged from round 325:
   circular - the store constants claim v0 because the 3 is not in it, and the 3
   sinks to the end because the constants hold v0.
   Round 434 (probe matrix on the tail): with the do-block the emitted order is
   retail's exactly x,y,pStateIds,state,z,stateIndex,pStateFrames and the 3 is
   emitted after the last register-needing store; WITHOUT the block the 3 is
   emitted earlier (after the pStateFrames store, before the three stores whose
   values are already in registers) but the pStateFrames store then glues itself
   to its own constant load and jumps to 4th - the 88.66 body. With `return 3`
   inside the block the 3 leads partially but pStateFrames jumps up again (88.66).
   A volatile pStateFrames store does NOT pin it (volatile may move earlier past
   non-volatile stores), so that combination is dead. PRESET_RULED_OUT:
   try_flags --all under gcc-2.7.2-cdk gives this body 93.41 (identical) while
   every matched function in the unit drops (InitGroup12/13 99.67, InitGroup16
   99.65, Group16 96.27, BuildPolyFT4 97.84), so cdk is not a candidate for the
   unit and the tail is compiler-stable across 2.7.2 and 2.7.2-cdk. Per
   instructions the residual is 14 differing of 41, every relocation equal.
   Round 471 re-measured the body below live at 93.41 (164 bytes = retail's 41
   insns) and found no new shape; it does name the mechanism. XenoWorldMapDiv
   runs cc1-2.7.2 with sched2 ON (`-fschedule-insns2` is on by default in this
   cc1: re-running the unit's own flags plus `-fno-schedule-insns2` reorders
   WorldMapInitGroup16Actor, which is matched at 100.00 as the unit builds it).
   So retail's store ORDER is the scheduler's output and is not evidence about
   the source statement order - but the register half is not the scheduler's,
   because local-alloc runs before sched2: retail's `addiu v0,zero,3` holding v0
   ACROSS the group means the `(set (reg 2) (const 3))` was already live there in
   RTL, i.e. emitted before the stores. cc1 emits it in the EXIT BLOCK of the
   `do { } while (0)` - the initial RTL of this body has `NOTE_INSN_LOOP_BEG`, a
   `code_label`, an unreachable back edge and then the `(set (reg/i:SI 2)
   (const_int 3))` after `NOTE_INSN_LOOP_END`, in a different block from the
   stores - so every `return 3` spelling lands it after them, and the loop
   optimizer hoisting the two invariant `lui/ori` pairs into the preheader is
   what makes the do-block version's order retail's at all (that is also why
   dropping the block gives 88.66). Probe matrix (cc1_probe.sh on a scratch
   replica of the tail), twelve bodies - .s store order, struct/field order,
   positions-grouped, `return 3` inside the block (twice), `nRet = 3` first, last
   and before the second call, `nRet` returned after the block, no do-block, an
   early constant-1 store into v0, and InitGroup16Actor's one-call shape - under
   gcc 2.7.2 at -O1/-O2/-O3 and gcc 2.6.0 at -O1/-O2/-O3: every one emits
   `li $2,3` last with $2 reused for the store constants (cdk rejects these
   bodies). Any local that would keep the 3 alive across the group is deleted by
   cse before regalloc, so the v0-held-by-3 RTL is not reachable from C here.
   Permuter not re-run: 93.41 is below the 99% threshold, and rounds 202/312 ran
   2111+ iterations from this exact body.
   Round 497 re-measured this body live at 93.41 (164 bytes, 41 insns) and swept
   the one degree of freedom the earlier matrices left open: all 24 source orders
   of the four trailing stores (state, z, stateIndex, pStateFrames) inside the
   do-block, compiled as a tail replica with the unit's own cc1 (2.7.2 -O2 -G8).
   Every one keeps `li $2,3` as the last instruction: local-alloc hands $v0 to
   whichever constant comes first in the block and the 3 then sinks past it, so
   the residual does not depend on statement order, and the emitted store order
   follows the source order in each case - the body below is the only order that
   emits retail's x,y,pStateIds,state,z,stateIndex,pStateFrames. Three more
   shapes probed the same way (no block at all; `do { stores; return 3; } while
   (0)`; `while (1) { stores; return 3; }`) all emit `li $2,3` immediately after
   the pStateFrames constant's store, i.e. past $v0's last def in that body and
   never at the block head, and the no-block body's emitted order degrades to
   x,y,pStateIds,pStateFrames,state,z,stateIndex (the 88.66 shape). The pre-sched2
   RTL never holds the return set at the head, which is the one thing retail needs.
*/
// best 93.41% (164 bytes = retail's 41 insns, store order and every relocation retail's; residual is only the 'addiu v0,zero,3' position and the v1/a0 constant rotation it causes). Blocked by the levers.md entry h-a-return-constant-leads-the-store-group-only-in-retail: any 'return 3' set lands after the stores and cse deletes any constant local that would move it. Round 484 re-measured this live at 93.41 and added one negative: 'u8 nRet = 3' inside the do-block returned after it also measures 93.41 (folded back), so the per-mode cse escape does not apply here.
/*
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FC8C);




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
    WorldMapDrawFadeFrames(WORLD_MAP_INIT_FADE_FRAMES, 0, WORLD_MAP_INIT_FADE_COLOR_STEP, 2);
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
    WorldMapAddSlot(WorldMapInitActorScript, WorldMapUpdateSlotActorScriptState);
    WorldMapAddSlot(func_80080578, WorldMapUpdateCameraSlot);
    WorldMapAddSlot(WorldMapInitSlotAtScrollPosition, WorldMapInitSpriteGroupsAtSlotPosition);
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
    unkValue = (u16)g_WorldMapViewRotationY[0];
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

