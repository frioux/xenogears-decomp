#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/memory.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/model.h"

// the map display buffer copied into VRAM when the scene initialises, and the
// VRAM position it is copied to
#define WORLD_MAP_SCREEN_WIDTH 320
#define WORLD_MAP_SCREEN_HEIGHT 216
#define WORLD_MAP_SCREEN_VRAM_X 0x2C0
#define WORLD_MAP_SCREEN_VRAM_Y 0x100

// the archive directory this overlay's files are read through
#define WORLD_MAP_ARCHIVE_DIRECTORY_INDEX 0x24

// one row of the sprite set parameter tables: x, y, z then the 11 actor slots
// WorldMapUpdateSpriteSetPrims copies into the slot
#define WORLD_MAP_SPRITE_SET_PARAM_COUNT 14

// the sprite set slot's tint channels lerp by a step and bounce inside these
// limits
#define WORLD_MAP_TINT_RED_MIN 0x40
#define WORLD_MAP_TINT_GREEN_MIN 0x80
#define WORLD_MAP_TINT_CHANNEL_MAX 0xFF

// the actor slot pool g_WorldMapSlots points at: one WorldMapActor, and one
// projected depth, per slot
#define WORLD_MAP_ACTOR_SLOT_COUNT 64

// a sprite whose projected depth reaches this is not drawn
#define WORLD_MAP_SPRITE_SZ_MAX 0xB00

// CLUT ids WorldMapLoadTextureClut builds, one GetClut per row it fills
#define WORLD_MAP_CLUT_ID_COUNT 16
// the CLUT row WorldMapLoadTextureClut reads out of VRAM and uploads back: the row
// it sits at, and how many entries wide it is
#define WORLD_MAP_TEXTURE_CLUT_VRAM_Y 0x1F0
#define WORLD_MAP_TEXTURE_CLUT_ROW_WIDTH 0x100

// the scale all three axes of a sprite set's mesh get before ScaleMatrix (a half,
// in the 1.12 form it reads)
#define WORLD_MAP_SPRITE_SET_SCALE 0x800

// an angle is a 4096-step turn counter
#define WORLD_MAP_ANGLE_FULL_TURN 0x1000
// the quarter turn WorldMapDrawSlotSprites takes off the sprite's facing angle. Owned by
// misc13.c (identical value); this unit does not include it.
#define WORLD_MAP_ANGLE_QUARTER_TURN 0x400
// sums of angles are wrapped back into the turn counter. Owned by
// include/main/game.h (identical value); this unit does not include it.
#define WORLD_MAP_ANGLE_MASK 0xFFF
// just past half a turn: the bound an angle difference is tested below to tell the
// half where the actor turns forward from the half where it turns back
#define WORLD_MAP_ANGLE_HALF_TURN_BOUND 0x801
// how far an actor slot's facing turns each frame toward its target
#define WORLD_MAP_ACTOR_TURN_STEP 0x100

// -1 in either step global means no sprite set / triangle is being resolved. Owned by
// misc14.c (identical value); this unit does not include it.
#define WORLD_MAP_STEP_NONE (-1)

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ s16 command;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 pad22;
    /* 0x24 */ s16 hidden;
    /* 0x26 */ u16 pad26;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 dirX;
    /* 0x3C */ s32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s16 angle;
    /* 0x4A */ u16 pad4A;
    /* 0x4C */ void *pSprite;
    /* 0x50 */ s32 red; // sprite-set slots' tint red; script slots keep their command cursor here
    /* 0x54 */ s32 green;
    /* 0x58 */ s32 blue;
    /* 0x5C */ s32 redStep; // sprite-set slots' red tint step; script slots keep the current facing angle here
    /* 0x60 */ s32 greenStep;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ s32 rotY;
    /* 0x6C */ s32 scaleAngle;
    /* 0x70 */ s32 scale;
    /* 0x74 */ s32 scaleStep;
    /* 0x78 */ u32 unk78;
    /* 0x7C */ s32 unk7C;
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 a;
} FadeParams; // Size: 0x4, alignment 1

// 12.4 fixed point world map position; a normal (x, y, z) shares this layout.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

// An actor's sprite keeps its world position as 16.16 fixed point: the wrap
// loop writes whole words, the GTE pass reads the integer half of each.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapSpritePos; // Size: 0xC, 16.16 fixed point

typedef struct {
    /* 0x0 */ s16 xFrac;
    /* 0x2 */ s16 x;
    /* 0x4 */ s16 yFrac;
    /* 0x6 */ s16 y;
    /* 0x8 */ s16 zFrac;
    /* 0xA */ s16 z;
} WorldMapSpritePosWhole; // Size: 0xC

// One triangle of a sprite set's mesh: three indices into WorldMapMesh::pVerts.
typedef struct {
    /* 0x0 */ s16 vert[3];
    /* 0x6 */ u8 unk6[6];
    /* 0xC */ u16 kind;
} WorldMapTriangle; // Size: 0xE

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ SVECTOR *pVerts;
    /* 0x8 */ WorldMapTriangle tris[1];
} WorldMapMesh; // Size: 0x16

typedef struct WorldMapEntry {
    /* 0x00 */ s16 hidden;
    /* 0x02 */ s16 partIndex;
    /* 0x04 */ u16 flags;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ u8 unk14[4];
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ ModelPart *pModel;
    /* 0x44 */ WorldMapMesh *pMesh;
    /* 0x48 */ void *pPackets[2]; // prim lists, [0] source and [1] destination; g_WorldMapPrimPacketIndex picks one
    /* 0x50 */ struct WorldMapEntry *pParent;
} WorldMapEntry; // Size: 0x54

extern WorldMapActor *g_WorldMapSlots; // actors, 0x80 apart
extern WorldMapEntry *g_WorldMapGroups; // sprite sets, 0x54 apart
extern s16 g_WorldMapSpriteSetCount; // g_WorldMapGroups entry count
extern s32 g_WorldMapPrimPacketIndex;
extern s16 D_8009AABC[];
extern s16 D_8009AB48[];
extern s16 D_8009ABD4[];
typedef struct {
    /* 0x00 */ VECTOR scale;
    /* 0x10 */ u8 unk10[0x90];
    /* 0xA0 */ SVECTOR rot;
    /* 0xA8 */ u8 unkA8[0x48];
    /* 0xF0 */ MATRIX mat;
} WorldMapModel; // Size: 0x110

typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ MATRIX mat;
} WorldMapModelOut; // Size: 0x40

// Partial view of the GTE work area in the scratchpad at 0x1F800000.
extern s16 D_8009D718[];

typedef struct {
    /* 0x00 */ VECTOR vert0; // first RotTrans output
    /* 0x10 */ VECTOR scale; // ScaleMatrix output, later the second RotTrans output
    /* 0x20 */ VECTOR vert2; // third RotTrans output
    /* 0x30 */ u8 unk30[0xC0];
    /* 0xF0 */ MATRIX mat;
} WorldMapScratch; // Size: 0x110

// The part of the scratchpad GTE work area a visibility pass uses: the three
// MVMVA outputs, the three packed clip words NormalClip consumes, and the
// sprite set's x/z distance.
typedef struct {
    /* 0x00 */ VECTOR vert0; // first MVMVA output
    /* 0x10 */ VECTOR vert1; // second
    /* 0x20 */ VECTOR vert2; // third
    /* 0x30 */ s32 clip[3]; // (z << 16) | x per vertex; [2] is the view position
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ VECTOR delta; // vx and vz only
} WorldMapClipWork; // Size: 0x4C

// The part of the scratchpad GTE work area the sprite pass projects through: the
// SVECTOR handed to RTPS, the wrapped x/z delta the sprite keeps, and one depth
// per actor slot.
typedef struct {
    /* 0x00 */ SVECTOR vec;
    /* 0x08 */ WorldMapPos delta; // x and z only
    /* 0x14 */ s32 pad14;
    /* 0x18 */ s32 depth[WORLD_MAP_ACTOR_SLOT_COUNT];
} WorldMapSpriteProjection; // Size: 0x118

// One sprite set definition inside the block g_WorldMapSpriteSetTable points at.
typedef struct {
    /* 0x0 */ u16 partIndex; // index into ModelFileHeader::modelParts
    /* 0x2 */ u16 flags;
    /* 0x4 */ s16 x;
    /* 0x6 */ s16 y;
    /* 0x8 */ s16 z;
    /* 0xA */ u16 rot[3];
} WorldMapSpriteSetDef; // Size: 0x10

typedef struct {
    /* 0x0 */ u16 count;
    /* 0x2 */ WorldMapSpriteSetDef defs[0];
} WorldMapSpriteSetTable; // Size: 0x2

extern u8 D_8009AC60[];
extern s32 g_WorldMapPlayerPosition[]; // x then the two words below it; the y/z names are separate symbols
extern ModelFileHeader *g_WorldMapModelData;
extern WorldMapSpriteSetTable *g_WorldMapSpriteSetTable;
extern s32 *g_WorldMapModelRelocs;
extern s16 g_WorldMapModelPointerCount;
extern MATRIX D_8009A140;
extern MATRIX g_WorldMapLightMatrix;
extern s32 g_GfxOTDepthShift;
extern s32 g_WorldMapStepTriangleIndex;
extern s32 g_WorldMapStepSpriteSet;

extern void ModelPartFreeLightData(void *pModelPart);
extern int Vsync(int mode);
extern void WorldMapFreeGridImages(void);
extern void WorldMapUpdateLoadQueue(void);
extern void WorldMapSetGridPosition(void *p);
extern s32 WorldMapGetQueuedLoadCount(void);
extern s32 func_80084DB8(WorldMapPos *pPos, s32 setIndex);
extern void WorldMapBuildSpritePrims(WorldMapEntry *pEntry, POLY_FT3 *pPrim, s32 count, s32 abr);
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern s16 g_WorldMapCameraRot;
extern u16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapFadeLevel;
extern void HeapFree(void *pMem);
extern void *HeapAlloc(u_int allocSize, u_int allocFlags);
extern void *g_MenuSedsFile;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 g_WorldMapExitFlag;
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void *p);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeSlotPool(void);
extern void WorldMapFreeSpriteSets(void);
extern void ModelLoadTextureBlock(void *pBlock);
extern void func_800931D8(u8 *pSrc, u8 *pDst, s32 count, u8 *pFactors);
extern void *g_WorldMapLoadedFileData;
extern u16 g_WorldMapTextureClutIds[];
extern FadeParams g_WorldMapClutFadeFactors;

extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapMode4EntryTriggered;
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void WorldMapDrawFadeFrames(int, int, int, int);
extern int ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern int ArchiveCdDataSync(int);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadClutAndTpageIds(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void SoundAddSedsEntry(void *);
extern int ArchiveSetIndex(int, int);
extern void WorldMapAddSlot(void *, void *);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern void WorldMapRunSlotScript(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern s32 WorldMapSetActorParams(s32);
extern s32 func_8008390C(s32);
extern s32 WorldMapInterpolatePlaneHeight(WorldMapPos *p0, WorldMapPos *p1, WorldMapPos *p2);
extern s32 func_80083A00(s32);
extern void WorldMapDeactivateSpriteGroup(s32 arg0);
extern s32 WorldMapApproachValue(s32 arg0, s32 arg1, s32 arg2);
extern void WorldMapInitSpriteGroupSprites(s32 arg0, SVECTOR *pPos, s16 *pRot);
extern s32 WorldMapSetActorPosition(s32);
extern s32 WorldMapUpdateSlotSpriteGroups(s32);
// Partial view of the render environment g_WorldMapRenderContext points at: the
// sprite pass only reaches the first ordering table.
typedef struct {
    /* 0x00 */ u8 unk00[0x70];
    /* 0x70 */ u_long *pOt[2];
} WorldMapRenderEnv;

extern WorldMapRenderEnv *g_WorldMapRenderContext;
extern MATRIX g_WorldMapViewMatrix;
extern void WorldMapWrapDelta(void *pDelta);
extern void WorldMapWrapDeltaScaled(WorldMapPos *pDelta);
extern void ModelRenderPacket(ModelPart *pPart, void *pPacket, u_long *pOt, s32 mode);
extern s16 D_8009AD2C[];
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_ModelRenderTestFlags;
extern s32 g_BattleFramePolyCount;
extern s32 g_BattleEmittedPacketCount;
extern void func_800223B0(void *p, s32 angle);
extern void SpriteDrawFrame(void *p, u32 *pTbl);
extern void GfxSetBaseMatrix(MATRIX *p);
extern void AnimScriptTick(void *p);

s32 WorldMapUpdateSlotSpriteGroups(s32 arg0) {
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];
    WorldMapEntry *pEntry = &g_WorldMapGroups[7];
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s32 x;
    s32 y;
    s32 z;

    switch (pActor->command) {
    case 1:
        pActor->command = 0;
        pActor->state = 1;
        g_WorldMapGroups[8].hidden = 0;
        g_WorldMapGroups[7].hidden = 0;
        break;
    case 2:
        pActor->command = 0;
        pActor->state = 2;
        WorldMapDeactivateSpriteGroup(3);
        WorldMapDeactivateSpriteGroup(5);
        WorldMapDeactivateSpriteGroup(6);
        WorldMapDeactivateSpriteGroup(7);
        WorldMapDeactivateSpriteGroup(0xA);
        break;
    case 3:
        pActor->command = 0;
        pActor->state = 3;
        WorldMapDeactivateSpriteGroup(7);
        WorldMapDeactivateSpriteGroup(8);
        WorldMapDeactivateSpriteGroup(9);
        break;
    case 4:
        pActor->command = 0;
        pActor->state = 4;
        WorldMapDeactivateSpriteGroup(5);
        WorldMapDeactivateSpriteGroup(6);
        WorldMapDeactivateSpriteGroup(7);
        WorldMapDeactivateSpriteGroup(0xA);
        WorldMapDeactivateSpriteGroup(0xB);
        WorldMapDeactivateSpriteGroup(0xC);
        break;
    }
    switch (pActor->state) {
    case 0:
        break;
    case 1:
        pActor->y = WorldMapApproachValue(pActor->y, 0xFFE80000, -0x400);
        *(s16 *)(pScratch + 0xA0) = pActor->x >> 12;
        *(s16 *)(pScratch + 0xA4) = pActor->z >> 12;
        *(s16 *)(pScratch + 0xA2) = pActor->y >> 12;
        WorldMapInitSpriteGroupSprites(3, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(5, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(6, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(7, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(0xA, (SVECTOR *)(pScratch + 0xA0), 0);
        break;
    case 2:
        pActor->y = WorldMapApproachValue(pActor->y, 0xFFE80000, -0x400);
        *(s16 *)(pScratch + 0xA0) = pActor->x >> 12;
        *(s16 *)(pScratch + 0xA4) = pActor->z >> 12;
        *(s16 *)(pScratch + 0xA2) = pActor->y >> 12;
        WorldMapInitSpriteGroupSprites(7, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(8, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(9, (SVECTOR *)(pScratch + 0xA0), 0);
        break;
    case 3:
        pActor->y = WorldMapApproachValue(pActor->y, 0xFFE80000, -0x400);
        *(s16 *)(pScratch + 0xA0) = pActor->x >> 12;
        *(s16 *)(pScratch + 0xA4) = pActor->z >> 12;
        *(s16 *)(pScratch + 0xA2) = pActor->y >> 12;
        WorldMapInitSpriteGroupSprites(5, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(6, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(7, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(0xA, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(0xB, (SVECTOR *)(pScratch + 0xA0), 0);
        WorldMapInitSpriteGroupSprites(0xC, (SVECTOR *)(pScratch + 0xA0), 0);
        break;
    case 4:
        pActor->y = WorldMapApproachValue(pActor->y, 0xFFE80000, -0x400);
        *(s16 *)(pScratch + 0xA0) = pActor->x >> 12;
        *(s16 *)(pScratch + 0xA4) = pActor->z >> 12;
        *(s16 *)(pScratch + 0xA2) = pActor->y >> 12;
        WorldMapInitSpriteGroupSprites(0xD, (SVECTOR *)(pScratch + 0xA0), 0);
        *(s16 *)(pScratch + 0xA2) = 0;
        WorldMapInitSpriteGroupSprites(0xE, (SVECTOR *)(pScratch + 0xA0), 0);
        break;
    }
    x = pActor->x >> 12;
    pEntry[1].x = x;
    pEntry[0].x = x;
    y = pActor->y >> 12;
    pEntry[1].y = y;
    pEntry[0].y = y;
    z = pActor->z >> 12;
    pEntry[1].z = z;
    pEntry[0].z = z;
    return 1;
}
void WorldMapLoadTextureClut(void) {
    RECT rect;
    FadeParams factors = g_WorldMapClutFadeFactors;
    u8 *pData;
    u8 *pDst;
    u16 *pClut;
    s32 i;

    pData = LZSSHeapDecompress(g_WorldMapLoadedFileData, 1);
    ModelLoadTextureBlock(pData);
    DrawSync(0);
    HeapFree(pData);
    HeapFree(g_WorldMapLoadedFileData);
    pData = HeapAlloc(0x200, 1);
    pDst = HeapAlloc(0x2000, 1);
    rect.x = 0;
    rect.y = WORLD_MAP_TEXTURE_CLUT_VRAM_Y;
    rect.w = WORLD_MAP_TEXTURE_CLUT_ROW_WIDTH;
    rect.h = 1;
    StoreImage(&rect, (u_long *)pData);
    DrawSync(0);
    func_800931D8(pData, pDst, WORLD_MAP_CLUT_ID_COUNT, (u8 *)&factors);
    rect.x = 0;
    rect.y = WORLD_MAP_TEXTURE_CLUT_VRAM_Y;
    rect.w = WORLD_MAP_TEXTURE_CLUT_ROW_WIDTH;
    rect.h = 0xF;
    LoadImage(&rect, (u_long *)pDst);
    DrawSync(0);
    i = 0;
    pClut = g_WorldMapTextureClutIds;
    for (; i < WORLD_MAP_CLUT_ID_COUNT; i++, pClut++) {
        *pClut = GetClut(rect.x, rect.y);
        rect.y++;
    }
    HeapFree(pDst);
    HeapFree(pData);
}
void WorldMapInitSpriteSets(void) {
    WorldMapSpriteSetTable *pSetTable;
    WorldMapSpriteSetDef *pDef;
    WorldMapMesh *pMesh;
    ModelPart *pPart;
    s32 *pRelocs;
    s32 *pCur;
    s16 count;
    s32 i;

    count = ModelResolvePointers(g_WorldMapModelData);
    g_WorldMapModelPointerCount = count;
    pRelocs = g_WorldMapModelRelocs;
    pCur = pRelocs + 1;
    g_WorldMapModelRelocs = pCur;
    for (i = 0; i < count; i++) {
        *pCur++ = (s32)pRelocs + *pCur;
    }
    count = g_WorldMapSpriteSetTable->count;
    g_WorldMapSpriteSetCount = count;
    g_WorldMapGroups = HeapAlloc(count * sizeof(WorldMapEntry), 0);
    pSetTable = g_WorldMapSpriteSetTable;
    SetColorMatrix(&D_8009A140);
    SetLightMatrix(&g_WorldMapLightMatrix);
    pDef = pSetTable->defs;
    for (i = 0; i < g_WorldMapSpriteSetCount; i++, pDef++) {
        g_WorldMapGroups[i].hidden = 0;
        g_WorldMapGroups[i].partIndex = pDef->partIndex;
        g_WorldMapGroups[i].flags = pDef->flags;
        g_WorldMapGroups[i].x = pDef->x;
        g_WorldMapGroups[i].y = pDef->y;
        g_WorldMapGroups[i].z = -pDef->z;
        g_WorldMapGroups[i].rot.vx = pDef->rot[0];
        g_WorldMapGroups[i].rot.vy = pDef->rot[1];
        g_WorldMapGroups[i].rot.vz = pDef->rot[2];
        RotMatrixYXZ(&g_WorldMapGroups[i].rot, &g_WorldMapGroups[i].mat);
        pPart = &((ModelFileHeader *)g_WorldMapModelData)->modelParts[g_WorldMapGroups[i].partIndex];
        g_WorldMapGroups[i].pModel = pPart;
        ModelAllocatePackets(pPart, &g_WorldMapGroups[i].pPackets[0], &g_WorldMapGroups[i].pPackets[1]);
        ModelInitializePacket(g_WorldMapGroups[i].pModel, g_WorldMapGroups[i].pPackets[0], 1);
        memcpy(g_WorldMapGroups[i].pPackets[1], g_WorldMapGroups[i].pPackets[0],
               g_WorldMapGroups[i].pModel->unk20[1].modelPacketSize);
        pMesh = ((WorldMapMesh **)g_WorldMapModelRelocs)[g_WorldMapGroups[i].partIndex];
        g_WorldMapGroups[i].pMesh = pMesh;
        pMesh->pVerts = (SVECTOR *)((u8 *)pMesh + (s32)pMesh->pVerts);
        g_WorldMapGroups[i].pParent = 0;
    }
    g_GfxOTDepthShift = 2;
    g_WorldMapStepTriangleIndex = WORLD_MAP_STEP_NONE;
    g_WorldMapStepSpriteSet = WORLD_MAP_STEP_NONE;
    g_WorldMapGroups[4].hidden = 1;
}

void WorldMapFreeSpriteSets(void) {
    s32 i;

    for (i = 0; i < g_WorldMapSpriteSetCount; i++) {
        HeapFree(g_WorldMapGroups[i].pPackets[0]);
        ModelPartFreeLightData(g_WorldMapGroups[i].pModel);
    }
    HeapFree(g_WorldMapGroups);
}

void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1) {
    g_WorldMapGroups[arg1].pParent = &g_WorldMapGroups[arg0];
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8_b", func_800848F4);
// Round 164: un-parked and repaired - the park did not compile (pScale and pB had
// no declarations; both are the scale VECTOR at 0x1F800010, pB was pScale's
// second name). It builds now and still measures 76.46%, 1060 bytes.
// Best: 76.46% (1060 bytes vs 1036). Structure and every call/offset in the
// head, the sprite-set copy and the tail (WrapDeltaScaled, ScaleMatrix,
// CompMatrix, SetRot/TransMatrix, RTPS, FLAG, SZ3, ModelRenderPacket) already
// agree; what is left is (a) gcc hoisting each scratchpad pointer into its own
// saved register (s0=0xF0, s7=0xA0, s8=0x10, s6=0x110) where retail keeps ONE
// base ($s1 = 0x1F800000) and derives 0xF0/0xF2/0xF4 from it, and (b) the GTE
// halves of the chain loop: retail splits the rotation-matrix load around the
// three column transforms and interleaves the translation ctc2s through
// $12-$14 (gte_SetRotMatrix_gpr order), and its first matrix load reaches the
// node through $t4-$t6, so gte_SetRotMatrix + gte_SetTransVector cannot
// reproduce it as two macro calls.
/*
void func_800848F4(void) {
    MATRIX *pMat;
    MATRIX *pOut = (MATRIX *)0x1F800110;
    SVECTOR *pRot = (SVECTOR *)0x1F8000A0;
    VECTOR *pScale = (VECTOR *)0x1F800010;
    WorldMapPos *pDelta = (WorldMapPos *)0x1F800000;
    WorldMapEntry *pChain;
    s32 camX;
    s32 camZ;
    s32 scale;
    s32 i;
    long flag;

    scale = 0x800;
    *(s32 *)&pScale->vz = scale;
    *(s32 *)&pScale->vy = scale;
    *(s32 *)&pScale->vx = scale;
    g_ModelRenderTestFlags = 3;
    g_BattleFramePolyCount = 0;
    g_BattleEmittedPacketCount = 0;
    pRot->vz = 0;
    pRot->vy = 0;
    pRot->vx = 0;
    camX = g_WorldMapCameraTarget >> 12;
    camZ = g_WorldMapCameraTargetZ >> 12;
    for (i = 0; i < g_WorldMapSpriteSetCount; i++) {
        if (g_WorldMapGroups[i].hidden != 0) {
            continue;
        }
        *pMat = g_WorldMapGroups[i].mat;
        pMat->t[1] = g_WorldMapGroups[i].x;
        pMat->t[2] = g_WorldMapGroups[i].y;
        pMat->t[3] = -g_WorldMapGroups[i].z;
        pChain = g_WorldMapGroups[i].pParent;
        while (pChain != 0) {
            pChain->mat.t[1] = pChain->x;
            pChain->mat.t[2] = pChain->y;
            pChain->mat.t[3] = -pChain->z;
            gte_SetRotMatrix(&pChain->mat);
            gte_ldclmv(&pMat->m[0][0]);
            gte_mvmva(1, 0, 3, 3, 0);
            gte_stclmv(&pMat->m[0][0]);
            gte_ldclmv(&pMat->m[0][1]);
            gte_mvmva(1, 0, 3, 3, 0);
            gte_stclmv(&pMat->m[0][1]);
            gte_ldclmv(&pMat->m[0][2]);
            gte_mvmva(1, 0, 3, 3, 0);
            gte_stclmv(&pMat->m[0][2]);
            gte_SetTransVector(&pChain->mat);
            gte_ldlv0(&pMat->t[1]);
            gte_rt();
            gte_stlvnl(&pMat->t[1]);
            pChain = pChain->pParent;
        }
        pDelta->x = pMat->t[1] - camX;
        pDelta->z = -pMat->t[3] - camZ;
        WorldMapWrapDeltaScaled(pDelta);
        pMat->t[1] = pDelta->x;
        pMat->t[3] = -pDelta->z;
        ScaleMatrix(pMat, pScale);
        CompMatrix(&g_WorldMapViewMatrix, pMat, pOut);
        SetRotMatrix(pOut);
        SetTransMatrix(pOut);
        gte_ldv0(pRot);
        gte_rtps();
        gte_stflg_val(flag);
        *(s32 *)0x1F800020 = flag;
        if (*(s32 *)0x1F800020 < 0) {
            continue;
        }
        gte_stsz((s32 *)0x1F800028);
        if (*(s32 *)0x1F800028 >= 0xD80) {
            continue;
        }
        ModelRenderPacket(g_WorldMapGroups[i].pModel, g_WorldMapGroups[i].pPackets[g_WorldMapPrimPacketIndex],
                          *(u_long **)(g_WorldMapRenderContext + 0x70), D_8009AD2C[g_WorldMapGroups[i].flags]);
    }
}
*/
// Not attempted: 259 instructions, no C written, no percentage. splat already
// labels the stub "Handwritten function" and the body is one GTE pipeline, so
// this is the class AGENTS.md calls unmatchable blind; it needs the whole body
// written before the first diff tells you anything.
// Decode, for whoever picks it up (addresses are the scratchpad, all reached
// through $s1 = 0x1F800028, i.e. write them as byte offsets off one base):
//   head: 0x1F800018/0x14/0x10 = 0x800 (scale, vz/vy/vx order), g_ModelRenderTestFlags = 3,
//         g_BattleFramePolyCount = 0, g_BattleEmittedPacketCount = 0, SVECTOR at 0x1F8000A0 = {0,0,0}.
//   per sprite set i (s4 = i, s2 = i*0x54): skip if g_WorldMapGroups[i].hidden != 0;
//         copy g_WorldMapGroups[i].mat (0x20..0x3C) to 0x1F8000F0..0x10C, then
//         overwrite 0x1F800104/108/10C = unk8/unkC/-unk10;
//   chain: while (p->unk50) { p = p->unk50; p->unk34/38/3C = unk8/unkC/-unk10;
//         gte matrix load of p->unk20 into $0-$4, three MVMVA 1,0,3,3,0 rotations
//         of the 3 shorts at 0x1F8000F0/F2/F4 (lh->mtc2 $9-$11, mfc2 $12-$14 stored
//         back as shorts at stride 6), gte load of p->0x34..0x3C into $5-$7, then
//         mtc2 $0 + lwc2 $1 8(...) and MVMVA 1,0,0,0,0 with swc2 $25/$26/$27 into
//         0x1F800104..10C; }  -- the tail call is WorldMapWrapDeltaScaled.
//   then: 0x1F800000 = 0x1F800104 - (g_WorldMapCameraTarget >> 12),
//         0x1F800008 = -0x1F80010C - (g_WorldMapCameraTargetZ >> 12);
//         ScaleMatrix(0x1F8000F0, 0x1F800010); CompMatrix(&g_WorldMapViewMatrix,
//         0x1F8000F0, 0x1F800110); SetRotMatrix/SetTransMatrix(0x1F800110);
//         RTPS on 0x1F8000A0, flag to 0x1F800020 (bltz skips), SZ3 to 0x1F800028
//         (>= 0xD80 skips), then
//         ModelRenderPacket(g_WorldMapGroups[i].pModel,
//             *(u32 *)((u8 *)g_WorldMapGroups + g_WorldMapPrimPacketIndex * 4 + 0x48),
//             *(u32 *)(g_WorldMapRenderContext + 0x70), D_8009AD2C[g_WorldMapGroups[i].flags]).
// The GTE encodings are the DMPSX 4.3 spellings (interleaved ctc2 of $12-$14 for
// the matrix load, nop;nop;.word for MVMVA), which inline_c.h still carries, so
// this is a body to write, not a wall -- but it is a two-hour job, not a stub.


s32 WorldMapFindSpriteSet(void *pArg, s16 *pOut) {
    WorldMapEntry *pEntry = g_WorldMapGroups;
    s16 result;
    s16 i;

    for (i = 0; i < g_WorldMapSpriteSetCount; i++) {
        if ((pEntry->flags & 1) && (result = func_80084DB8(pArg, i)) != 0) {
            *pOut = i;
            return result;
        }
        pEntry++;
    }
    return 0;
}

// Best: 98.53% at retail's exact 924 bytes (231 instructions); 12 assembly
// differences, down from 13 on the 98.62 body this note used to hold (objdiff's
// percentage is not monotone in the diff, judge by the diff lines). The body IS
// compiler output despite splat's "Handwritten function" marker: every call,
// GTE encoding, the 0x50 frame, the loop body and the epilogue are exact.
// What the distance-test block needed:
// - `dx = dx >= 0x800;` and later `dz = dz >= 0x800;` - REASSIGN each distance
//   to its own comparison result and accumulate with `dx |= dz` - is the only
//   spelling that gives retail's in-place `slti/slti;xori;xori` pairs (the
//   compare written into the distance's own register) and its in-place
//   `or a1,a1,v0`. `far = dx >= 0x800; far |= dz >= 0x800;` splits the compare
//   through v0 and costs 4 instructions. This is the "redefine a value into a
//   dead local" lever.
// - `dx` has to be built in three statements (`dx = pPos->x; dx = dx >> 12;
//   dx = dx - pEntry->x;`); as one expression gcc loads pPos->x into v0 after
//   the `addu s1,v1,v0` and the whole dx block moves (permuter find).
// - The distance test must be an `if/else` (body in the else) AND the `if` block
//   must end with an empty `do { } while (0);` AFTER the return - together they
//   are the only spelling that gives retail's block layout: `beq a1,<body>`
//   with the ret0 block inline (`j <epilogue>; move v0,zero`) instead of gcc
//   threading `bne -> epilogue`.
// - `pVerts = pMesh->pVerts;` must be assigned BEFORE `pWork->clip[2]` and
//   `pTri = pMesh->tris;`: it decides where `count`'s load and its spill to
//   0x18(sp) land.
// Residue, three independent allocator ties (permuter run 4 - base 345, 1872
// iterations, 5 min - on THIS body found nothing; four runs total):
// (a) 5 instructions: dx is allocated a2 here and a1 in retail. Everything else
//     in the block follows from that one choice (retail's `slti a1,a1,2048` and
//     `xori a1,a1,1` are in-place because old dx, the slt temp and the flag all
//     landed in a1). Twelve declaration orders of the locals were measured: dx
//     is a2 in every one, so local declaration order is not the lever here.
// (b) 4 instructions: the dz block's two temporaries are swapped - retail
//     coalesces pEntry->z's load with the result (`lw v1,8(a0); lw v0,16(s1);
//     sra v1; subu v0,v0,v1`), this body coalesces the sra'd pPos->z with it
//     (`lw v0,8(a0); lw v1,16(s1); sra v0; subu v0,v1,v0`). Writing the block as
//     two statements (`dz = pEntry->z; dz -= pPos->z >> 12;`) scores 98.08.
// (c) 2 instructions: the scratchpad base load (`li $16,0x1F800000`, the
//     loop-invariant constant loop.c hoists) is stolen into the dx `bgez` delay
//     slot where retail has a `nop`, so `i = 0` (`move $18,zero`) fills the
//     far-test `beqz` slot where retail has the hoisted `lui $s0,0x1F80` and
//     puts `i = 0` inside the body block. Assigning pWork inside the body block
//     is worse (95.88 / 928 bytes), and an empty `do { } while (0);` at the top
//     of the else block changes nothing.
// (d) two operands differ by relocation only (`addiu v0,s0,%lo(D_1F800010)` and
//     `%lo(D_1F800020)` in retail vs the folded `addiu v0,s0,16` / `32` here) -
//     splat named those two addresses, the linked bytes are identical, so
//     diff_func can never print MATCH; judge this function with check_bin.sh.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8_b", func_80084DB8);
/*
s32 func_80084DB8(WorldMapPos *pPos, s32 setIndex) {
    WorldMapEntry *pEntry = &g_WorldMapGroups[setIndex];
    MATRIX *pMat;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    s32 count;
    s32 dx;
    s32 dz;
    s32 i;
    WorldMapClipWork *pWork = (WorldMapClipWork *)0x1F800000;
    s32 n;
    long flag;

    dx = pPos->x;
    dx = dx >> 12;
    dx = dx - pEntry->x;
    pWork->delta.vx = dx;
    if (dx < 0) {
        dx = -dx;
    }
    dx = dx >= 0x800;
    dz = pEntry->z - (pPos->z >> 12);
    pWork->delta.vz = dz;
    if (dz < 0) {
        do {
            dz = -dz;
        } while (0);
    }
    dz = dz >= 0x800;
    dx |= dz;
    if (dx) {
        return 0;
        do {
        } while (0);
    } else {
        pMat = (MATRIX *)0x1F8000F0;
        *pMat = pEntry->mat;
        i = 0;
        n = 0;
        pMat->t[2] = 0;
        pMat->t[0] = 0;
        *(s32 *)0x1F800008 = 0x800;
        *(s32 *)0x1F800004 = 0x800;
        pMat->t[1] = pEntry->y;
        *(s32 *)0x1F800000 = 0x800;
        ScaleMatrix(pMat, (VECTOR *)0x1F800000);
        SetRotMatrix(pMat);
        SetTransMatrix(pMat);
        pMesh = pEntry->pMesh;
        count = pMesh->unk0;
        pVerts = pMesh->pVerts;
        pWork->clip[2] = (pWork->delta.vz << 16) | ((u16 *)&pWork->delta)[0];
        pTri = pMesh->tris;
        for (; i < count; i++, pTri++) {
            gte_ldv0(&pVerts[pTri->vert[0]]);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl(&pWork->vert0);
            gte_stflg(&flag);
            gte_ldv0(&pVerts[pTri->vert[1]]);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl(&pWork->vert1);
            gte_stflg(&flag);
            gte_ldv0(&pVerts[pTri->vert[2]]);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl(&pWork->vert2);
            gte_stflg(&flag);
            pWork->clip[0] = (pWork->vert0.vz << 16) | ((u16 *)&pWork->vert0)[0];
            pWork->clip[1] = (pWork->vert1.vz << 16) | (pWork->vert1.vx & 0xFFFF);
            if (NormalClip(pWork->clip[0], pWork->clip[1], pWork->clip[2]) > 0) {
                continue;
            }
            pWork->clip[0] = (pWork->vert1.vz << 16) | (pWork->vert1.vx & 0xFFFF);
            pWork->clip[1] = (pWork->vert2.vz << 16) | (pWork->vert2.vx & 0xFFFF);
            if (NormalClip(pWork->clip[0], pWork->clip[1], pWork->clip[2]) > 0) {
                continue;
            }
            pWork->clip[0] = (pWork->vert2.vz << 16) | (pWork->vert2.vx & 0xFFFF);
            pWork->clip[1] = (pWork->vert0.vz << 16) | ((u16 *)&pWork->vert0)[0];
            if (NormalClip(pWork->clip[0], pWork->clip[1], pWork->clip[2]) > 0) {
                continue;
            }
            D_8009D718[n] = i;
            D_8009D718[n + 1] = pTri->kind;
            n = n + 2;
        }
        return n;
    }
}
*/


void WorldMapProjectTriangleNormal(WorldMapPos *pPos, WorldMapPos *pOut, WorldMapPos *pNormal, u16 setIndex,
                   u16 triIndex) {
    WorldMapEntry *pEntry;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    MATRIX *pMat;
    VECTOR *pA = (VECTOR *)0x1F800000;
    VECTOR *pB;
    WorldMapScratch *pWork = (WorldMapScratch *)0x1F800000;
    long flag;

    pEntry = &g_WorldMapGroups[setIndex];
    pOut->x = (pPos->x >> 12) - pEntry->x;
    pOut->z = pEntry->z - (pPos->z >> 12);
    pMat = (MATRIX *)0x1F8000F0;
    *pMat = pEntry->mat;
    pMat->t[2] = 0;
    pMat->t[0] = 0;
    pMat->t[1] = pEntry->y;
    pWork->scale.vz = WORLD_MAP_SPRITE_SET_SCALE;
    pWork->scale.vy = WORLD_MAP_SPRITE_SET_SCALE;
    pWork->scale.vx = WORLD_MAP_SPRITE_SET_SCALE;
    pB = (VECTOR *)0x1F800010;
    ScaleMatrix(pMat, pB);
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);

    pMesh = pEntry->pMesh;
    pTri = pMesh->tris;
    pTri += triIndex;
    pVerts = pMesh->pVerts;
    RotTrans(&pVerts[pTri->vert[0]], pA, &flag);
    RotTrans(&pVerts[pTri->vert[1]], pB, &flag);
    RotTrans(&pVerts[pTri->vert[2]], &pWork->vert2, &flag);
    pB->vx -= pA->vx;
    pB->vy -= pA->vy;
    pB->vz -= pA->vz;
    pWork->vert2.vx -= pA->vx;
    pWork->vert2.vy -= pA->vy;
    pWork->vert2.vz -= pA->vz;
    OuterProduct0(&pWork->vert2, pB, &pWork->vert2);
    pWork->vert2.vx >>= 2;
    pWork->vert2.vy >>= 2;
    pWork->vert2.vz >>= 2;
    VectorNormal(&pWork->vert2, (VECTOR *)pNormal);
    WorldMapInterpolatePlaneHeight(pOut, (WorldMapPos *)pA, pNormal);
}
s32 WorldMapCheckTriangleEdgeSide(WorldMapPos *pPos, s32 yOffset, u16 setIndex, u16 triIndex) {
    WorldMapEntry *pEntry;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    MATRIX *pMat;
    VECTOR *pA = (VECTOR *)0x1F800000;
    VECTOR *pB;
    MATRIX *pLine = (MATRIX *)0x1F800110;
    VECTOR *pNormal = (VECTOR *)0x1F800030;
    VECTOR *pDst = (VECTOR *)0x1F800040;
    WorldMapScratch *pWork = (WorldMapScratch *)0x1F800000;
    long flag;
    s32 x;
    s32 y;
    s32 z;
    s32 entryZ;

    pEntry = &g_WorldMapGroups[setIndex];
    pMat = (MATRIX *)0x1F8000F0;
    *pMat = pEntry->mat;
    pMat->t[2] = 0;
    pMat->t[0] = 0;
    pMat->t[1] = pEntry->y;
    pWork->scale.vz = WORLD_MAP_SPRITE_SET_SCALE;
    pWork->scale.vy = WORLD_MAP_SPRITE_SET_SCALE;
    pWork->scale.vx = WORLD_MAP_SPRITE_SET_SCALE;
    pB = (VECTOR *)0x1F800010;
    ScaleMatrix(pMat, pB);
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);
    pMesh = pEntry->pMesh;
    pTri = pMesh->tris;
    pTri += triIndex;
    pVerts = pMesh->pVerts;
    RotTrans(&pVerts[pTri->vert[0]], pA, &flag);
    RotTrans(&pVerts[pTri->vert[1]], pB, &flag);
    RotTrans(&pVerts[pTri->vert[2]], &pWork->vert2, &flag);
    pB->vx -= pA->vx;
    pB->vy -= pA->vy;
    pB->vz -= pA->vz;
    pWork->vert2.vx -= pA->vx;
    pWork->vert2.vy -= pA->vy;
    pWork->vert2.vz -= pA->vz;
    OuterProduct0(&pWork->vert2, pB, &pWork->vert2);
    pWork->vert2.vx >>= 2;
    pWork->vert2.vy >>= 2;
    pWork->vert2.vz >>= 2;
    VectorNormal(&pWork->vert2, pNormal);
    x = (pPos->x >> 12) - pEntry->x - pA->vx;
    pLine->m[0][0] = x;
    pLine->m[1][0] = x;
    y = (pPos->y >> 12) - pA->vy;
    pLine->m[0][1] = y;
    entryZ = pEntry->z;
    z = entryZ - (pPos->z >> 12);
    pLine->m[1][1] = y - yOffset;
    z -= pWork->vert0.vz;
    pLine->m[0][2] = z;
    pLine->m[1][2] = z;
    ApplyMatrixLV(pLine, pNormal, pDst);
    return (pDst->vx ^ pDst->vy) >> 31;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8_b", func_80085760);
// Best: 78.64% in 1368 bytes vs retail's 1400 (32 bytes / 8 insns short). The
// body below is structurally right -- it is the register frame that is wrong:
// retail's frame is 0x30 and it keeps only three scratchpad bases live
// ($s1 = 0x1F800000, $s3 = 0x1F800010, $s0 = 0x1F800020, the gte_stlvnl
// destinations), so 0x1F8000F0/0x30/0x40/0x50 are ABSOLUTE constant
// expressions in the original (`sw $v0,64($at)`, `lui $a0,0x1F80; ori 0xF0`),
// and the pack's vz reads are absolute too (`lui $a0,0x1F800008; lw 0($a0)`)
// while its vx reads reuse the base register (`lhu 0($s1)`) -- i.e. the
// original's vx and vz reads were *different expressions*, which is why the
// pointer locals here push the frame to 0x40 and cost the 8 insns.
// Everything else is mapped: a0 = a WorldMapPos used only by the switch arms,
// a1 = the WorldMapPos the clip origin comes from, a2 = setIndex as s32 (no
// andi before the *0x54 chain, unlike WorldMapCheckTriangleEdgeSide whose is u16), a3 =
// triIndex. After the three MVMVA passes it packs (vz<<16)|(u16)vx for the
// three vertices and the clip origin into the three longs at 0x1F800030 and
// calls NormalClip(v0,v1,origin) -> bit 1, (v1,v2) -> bit 2, (v2,v0) -> bit 4;
// switch (clip) { case 3: re-pack the origin from a0 (& 0xFFFF on x),
// NormalClip(origin, v1, origin) != 0 -> clip = 1; case 5: same with v0 ->
// clip = 4; case 6: same with v2 -> clip = 2; } then three edge normals at
// 0x1F800030/40/50 each VectorNormal(n, n) and `return clip`. gte_stflg(&flag)
// is already exact (cfc2 $12 = $t4, nop, sw through the address register).
// The one clue to the original's spelling of those constants: the
// VectorNormal argument is built as `addiu $a0,$s1,0x30` (and 0x40, 0x50), i.e.
// pVert0 + 3/4/5 in VECTOR strides, while the clip origin's stores to
// 0x1F800040 are absolute -- so the original mixed two typed views of the same
// scratchpad slots and gcc kept them as separate pseudos. Reproduce that and
// the saved registers come down to retail's five.
/*
s32 func_80085760(WorldMapPos *pPos, WorldMapPos *pRef, s32 setIndex, s32 triIndex) {
    WorldMapEntry *pEntry;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    VECTOR *pVert0 = (VECTOR *)0x1F800000;
    VECTOR *pVert1 = (VECTOR *)0x1F800010;
    VECTOR *pVert2 = (VECTOR *)0x1F800020;
    VECTOR *pArgs = (VECTOR *)0x1F800030;
    VECTOR *pPosVec = (VECTOR *)0x1F800040;
    VECTOR *pNormal2 = (VECTOR *)0x1F800050;
    MATRIX *pMat;
    long flag;
    s32 clip;

    pEntry = &g_WorldMapGroups[setIndex];
    pPosVec->vx = (pRef->x >> 12) - pEntry->x;
    pPosVec->vz = pEntry->z - (pRef->z >> 12);
    pMat = (MATRIX *)0x1F8000F0;
    *pMat = pEntry->mat;
    pMat->t[2] = 0;
    pMat->t[0] = 0;
    pMat->t[1] = pEntry->y;
    pVert0->vz = 0x800;
    pVert0->vy = 0x800;
    pVert0->vx = 0x800;
    ScaleMatrix(pMat, pVert0);
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);
    pMesh = pEntry->pMesh;
    pTri = &pMesh->tris[triIndex];
    pVerts = pMesh->pVerts;
    gte_ldv0(&pVerts[pTri->vert[0]]);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(pVert0);
    gte_stflg(&flag);
    gte_ldv0(&pVerts[pTri->vert[1]]);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(pVert1);
    gte_stflg(&flag);
    gte_ldv0(&pVerts[pTri->vert[2]]);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(pVert2);
    gte_stflg(&flag);
    clip = 0;
    pArgs->vx = (pVert0->vz << 16) | ((u16 *)pVert0)[0];
    pArgs->vy = (pVert1->vz << 16) | ((u16 *)pVert1)[0];
    pArgs->vz = (pPosVec->vz << 16) | ((u16 *)pPosVec)[0];
    if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) > 0) {
        clip |= 1;
    }
    pArgs->vx = (pVert1->vz << 16) | ((u16 *)pVert1)[0];
    pArgs->vy = (pVert2->vz << 16) | ((u16 *)pVert2)[0];
    if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) > 0) {
        clip |= 2;
    }
    pArgs->vx = (pVert2->vz << 16) | ((u16 *)pVert2)[0];
    pArgs->vy = (pVert0->vz << 16) | ((u16 *)pVert0)[0];
    if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) > 0) {
        clip |= 4;
    }
    switch (clip) {
    case 3:
        pArgs->vx = ((pEntry->z - (pPos->z >> 12)) << 16) | (((pPos->x >> 12) - pEntry->x) & 0xFFFF);
        pArgs->vy = (pVert1->vz << 16) | ((u16 *)pVert1)[0];
        if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) != 0) {
            clip = 1;
        }
        break;
    case 5:
        pArgs->vx = ((pEntry->z - (pPos->z >> 12)) << 16) | (((pPos->x >> 12) - pEntry->x) & 0xFFFF);
        pArgs->vy = (pVert0->vz << 16) | ((u16 *)pVert0)[0];
        if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) != 0) {
            clip = 4;
        }
        break;
    case 6:
        pArgs->vx = ((pEntry->z - (pPos->z >> 12)) << 16) | (((pPos->x >> 12) - pEntry->x) & 0xFFFF);
        pArgs->vy = (pVert2->vz << 16) | ((u16 *)pVert2)[0];
        if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) != 0) {
            clip = 2;
        }
        break;
    }
    pArgs->vx = pVert1->vx - pVert0->vx;
    pArgs->vy = 0;
    pArgs->vz = pVert1->vz - pVert0->vz;
    VectorNormal(pArgs, pArgs);
    pPosVec->vx = pVert2->vx - pVert1->vx;
    pPosVec->vy = 0;
    pPosVec->vz = pVert2->vz - pVert1->vz;
    VectorNormal(pPosVec, pPosVec);
    pNormal2->vx = pVert0->vx - pVert2->vx;
    pNormal2->vy = 0;
    pNormal2->vz = pVert0->vz - pVert2->vz;
    VectorNormal(pNormal2, pNormal2);
    return clip;
}
*/
// Projects every visible actor slot through the camera, then draws the sprites
// the projection kept on screen and slews each one's turn step toward its facing.
//
// The slew stores the new step in all four arms: cc1 cross-jumps the four
// identical stores into the one `sw` retail has after the join, and that block
// boundary is what keeps the store ahead of the call's argument loads (a single
// store below the join is scheduled after them).
//
// Retail names the scratchpad addresses (D_1F800008 / D_1F800004) where the typed
// spelling folds to 8 / 4: both symbols are absolute, the linked bytes are equal,
// so judge this function with `tools/scripts/check_bin.sh world_map`.
void WorldMapDrawSlotSprites(void) {
    WorldMapActor *pActor;
    WorldMapSpriteProjection *pProj;
    MATRIX *pMat;
    WorldMapPos *pCam;
    s32 i;
    s32 d;
    s32 v;

    pProj = (WorldMapSpriteProjection *)0x1F800000;
    i = 0;
    pCam = (WorldMapPos *)&g_WorldMapCameraTarget;
    pActor = g_WorldMapSlots;
    for (; i < WORLD_MAP_ACTOR_SLOT_COUNT; i++, pActor++) {
        if ((pActor->hidden == 0) & (pActor->pSprite != 0)) {
            pProj->delta.x = pActor->x - pCam->x;
            pProj->delta.z = pActor->z - pCam->z;
            WorldMapWrapDelta(&pProj->delta);
            ((WorldMapSpritePos *)pActor->pSprite)->x = pProj->delta.x << 4;
            ((WorldMapSpritePos *)pActor->pSprite)->z = -pProj->delta.z << 4;
            ((WorldMapSpritePos *)pActor->pSprite)->y = pActor->y << 4;
        }
    }
    pMat = &g_WorldMapViewMatrix;
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);
    pActor = g_WorldMapSlots;
    for (i = 0; i < WORLD_MAP_ACTOR_SLOT_COUNT; i++, pActor++) {
        if ((pActor->hidden == 0) & (pActor->pSprite != 0)) {
            v = ((WorldMapSpritePosWhole *)pActor->pSprite)->x;
            pProj->vec.vx = v;
            v = ((WorldMapSpritePosWhole *)pActor->pSprite)->y;
            pProj->vec.vy = v;
            v = ((WorldMapSpritePosWhole *)pActor->pSprite)->z;
            pProj->vec.vz = v;
            gte_ldv0(&pProj->vec);
            gte_rtps();
            gte_stsz(&pProj->depth[i]);
        }
    }
    GfxSetBaseMatrix(&g_WorldMapViewMatrix);
    i = 0;
    pActor = g_WorldMapSlots;
    for (; i < WORLD_MAP_ACTOR_SLOT_COUNT; i++, pActor++) {
        if ((pActor->hidden == 0) & (pActor->pSprite != 0) & (pProj->depth[i] < WORLD_MAP_SPRITE_SZ_MAX)) {
            SpriteDrawFrame(pActor->pSprite, g_WorldMapRenderContext->pOt[0] + (pProj->depth[i] >> 4));
            d = pActor->angle - pActor->redStep;
            if (d < 0) {
                d += WORLD_MAP_ANGLE_FULL_TURN;
            }
            if (d < WORLD_MAP_ANGLE_HALF_TURN_BOUND) {
                if (d >= WORLD_MAP_ACTOR_TURN_STEP) {
                    d = pActor->redStep + WORLD_MAP_ACTOR_TURN_STEP;
                    pActor->redStep = d;
                } else {
                    d = pActor->angle;
                    pActor->redStep = d;
                }
            } else {
                d -= WORLD_MAP_ANGLE_FULL_TURN;
                if (d < -0xFF) {
                    d = pActor->redStep - WORLD_MAP_ACTOR_TURN_STEP;
                    pActor->redStep = d;
                } else {
                    d = pActor->angle;
                    pActor->redStep = d;
                }
            }
            func_800223B0(pActor->pSprite, (d - *(u16 *)&g_WorldMapViewRotationY - WORLD_MAP_ANGLE_QUARTER_TURN) & WORLD_MAP_ANGLE_MASK);
            AnimScriptTick(pActor->pSprite);
        }
    }
}