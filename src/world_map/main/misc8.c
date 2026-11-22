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
extern s16 D_8009D7E0; // g_WorldMapGroups entry count
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
extern s16 D_8009BD28;
extern MATRIX D_8009A140;
extern MATRIX D_8009A160;
extern s32 g_GfxOTDepthShift;
extern s32 D_8009C16C;
extern s32 D_8009C840;

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
extern u16 D_8009BCE0[];
extern FadeParams D_800704DC;

extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapMode4EntryTriggered;
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(int, int, int, int);
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
extern s32 func_80084068(s32);
extern u8 *g_WorldMapRenderContext;
extern MATRIX g_WorldMapViewMatrix;
extern void WorldMapWrapDelta(void *pDelta);
extern void WorldMapWrapDeltaScaled(WorldMapPos *pDelta);
extern void ModelRenderPacket(ModelPart *pPart, void *pPacket, u_long *pOt, s32 mode);
extern s16 D_8009AD2C[];
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_ModelRenderTestFlags;
extern s32 D_800595C0;
extern s32 g_BattleEmittedPacketCount;
extern void func_800223B0(void *p, s32 angle);
extern void SpriteDrawFrame(void *p, u32 *pTbl);
extern void GfxSetBaseMatrix(MATRIX *p);
extern void AnimScriptTick(void *p);

void WorldMapBuildActorModelMatrix(WorldMapActor *pActor, WorldMapModelOut *pOut, WorldMapModel *pModel) {
    if (pActor->state == 0) {
        return;
    }
    if (pActor->state != 1) {
        return;
    }
    {
        pModel->rot.vz = 0;
        pModel->rot.vx = 0;
        pModel->rot.vy = pActor->rotY;
        pModel->scale.vx = pActor->scale & 0x7FFF;
        pModel->scale.vy = 0x1000;
        pModel->scale.vz = rcos(pActor->scaleAngle) * 6;
        RotMatrix(&pModel->rot, &pModel->mat);
        ScaleMatrix(&pModel->mat, &pModel->scale);
        pOut->mat = pModel->mat;
        pActor->red += pActor->redStep;
        pActor->green += pActor->greenStep;
        if (pActor->red >= 0x100) {
            pActor->red = WORLD_MAP_TINT_CHANNEL_MAX;
            pActor->redStep = -pActor->redStep;
        } else if (pActor->red < WORLD_MAP_TINT_RED_MIN) {
            pActor->red = WORLD_MAP_TINT_RED_MIN;
            pActor->redStep = -pActor->redStep;
        }
        if (pActor->green >= 0x100) {
            pActor->green = WORLD_MAP_TINT_CHANNEL_MAX;
            pActor->greenStep = -pActor->greenStep;
        } else if (pActor->green < WORLD_MAP_TINT_GREEN_MIN) {
            pActor->green = WORLD_MAP_TINT_GREEN_MIN;
            pActor->greenStep = -pActor->greenStep;
        }
        pActor->scaleAngle = (pActor->scaleAngle + 8) & 0xFFF;
        pActor->scale += pActor->scaleStep;
    }
}

void WorldMapBuildSpritePrims(WorldMapEntry *pEntry, POLY_FT3 *pPrim, s32 count, s32 abr) {
    s32 i;

    for (i = count - 1; i != -1; i--, pPrim++) {
        setPolyFT3(pPrim);
        pPrim->tpage = GetTPage(0, abr, WORLD_MAP_SCREEN_VRAM_X, WORLD_MAP_SCREEN_VRAM_Y);
        setRGB0(pPrim, 0, 0, 0);
        setSemiTrans(pPrim, 1);
    }
    memcpy(pEntry->pPackets[1], pEntry->pPackets[0], count * sizeof(POLY_FT3));
}

void WorldMapSetSpritePrimColor(POLY_FT3 *pPrim, s32 count, s32 r, s32 g, s32 b) {
    s32 i;

    for (i = count - 1; i != -1; i--) {
        setRGB0(pPrim, r, g, b);
        pPrim++;
    }
}

s32 WorldMapSetupSpriteSetPrims(s32 arg0) {
    WorldMapEntry *pEntry = &g_WorldMapGroups[arg0 + 78];

    WorldMapBuildSpritePrims(pEntry, pEntry->pPackets[0], pEntry->pModel->polyCount, 3);
    return 1;
}
s32 WorldMapUpdateSpriteSetPrims(s32 arg0) {
    s32 row = arg0 - 3;
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];
    WorldMapEntry *pEntry = &g_WorldMapGroups[row + 81];
    s32 *pSlots;
    s32 i;

    switch (pActor->command) {
    case 0:
        break;
    case 1:
        pSlots = &pActor->red;
        pActor->state = 1;
        pActor->command = 0;
        pEntry->x = D_8009AABC[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT];
        pEntry->y = D_8009AABC[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + 1];
        pEntry->z = D_8009AABC[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + 2];
        for (i = 3; i < WORLD_MAP_SPRITE_SET_PARAM_COUNT; i++) {
            *pSlots++ = D_8009AABC[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + i];
        }
        break;
    case 2:
        pSlots = &pActor->red;
        pActor->state = 1;
        pActor->command = 0;
        pEntry->x = D_8009AB48[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT];
        pEntry->y = D_8009AB48[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + 1];
        pEntry->z = D_8009AB48[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + 2];
        for (i = 3; i < WORLD_MAP_SPRITE_SET_PARAM_COUNT; i++) {
            *pSlots++ = D_8009AB48[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + i];
        }
        break;
    case 3:
        pActor->command = 0;
        pActor->state = 0;
        break;
    case 4:
        pSlots = &pActor->red;
        pActor->state = 1;
        pActor->command = 0;
        pEntry->x = D_8009ABD4[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT];
        pEntry->y = D_8009ABD4[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + 1];
        pEntry->z = D_8009ABD4[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + 2];
        for (i = 3; i < WORLD_MAP_SPRITE_SET_PARAM_COUNT; i++) {
            *pSlots++ = D_8009ABD4[row * WORLD_MAP_SPRITE_SET_PARAM_COUNT + i];
        }
        break;
    }
    WorldMapBuildActorModelMatrix(pActor, pEntry, (WorldMapModel *)PSX_SCRATCH);
    WorldMapSetSpritePrimColor(pEntry->pPackets[g_WorldMapPrimPacketIndex],
                               pEntry->pModel->polyCount,
                               pActor->red, pActor->green, pActor->blue);
    return 1;
}

s32 WorldMapReturnSuccess(void) {
    return 1;
}

s32 WorldMapWaitForQueueDrain(s32 arg0) {
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];

    if (pActor->command == 1) {
        pActor->command = 0;
        DrawSync(0);
        Vsync(0);
        WorldMapFreeGridImages();
        WorldMapSetGridPosition(g_WorldMapPlayerPosition);
        do {
            WorldMapUpdateLoadQueue();
            Vsync(0);
        } while (WorldMapGetQueuedLoadCount() > 0);
    }
    return 1;
}
void WorldMapInitializeScene(void) {
    RECT rect;
    s32 *pVec;

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
    pVec = g_WorldMapPlayerPosition;
    *pVec = 0x1800000;
    g_WorldMapPlayerPositionY = 0xFFF00000;
    g_WorldMapPlayerPositionZ = 0x1A00000;
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
    WorldMapSetGridPosition(pVec);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot((void *)WorldMapModeInitFadeRect, (void *)func_800925A0);
    WorldMapAddSlot((void *)WorldMapSetActorParams, (void *)WorldMapRunSlotScript);
    WorldMapAddSlot((void *)func_8008390C, (void *)func_80083A00);
    WorldMapAddSlot((void *)WorldMapSetActorPosition, (void *)func_80084068);
    WorldMapAddSlot((void *)WorldMapEnterNop, (void *)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapInitRandomTimers();
}
s32 WorldMapShutdownScene(void) {
    u16 v;

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
    v = g_WorldMapViewRotationY;
    g_NextMapId = 0x269;
    g_NextMapFlags = 4;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
    return 1;
}

s32 WorldMapSetActorParams(s32 arg0) {
    g_WorldMapSlots[arg0].red = (u32)D_8009AC60;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008390C);
// Best: 96.15% at retail's exact size (244 bytes). The only difference left is
// one register: the 0x960000 both stores share sits in $a0 here and in $a1 in
// retail (`lui a1,0x96` just after the third position load). Everything else,
// retail's two `lh` reloads of the angle globals included, is byte for byte.
// What got it here, in case it helps invert that tie:
// - The two `lh` are STORE-TO-LOAD FOLDING, not a volatile problem. The fold
//   only fires when the read names the symbol the store wrote, so reading the
//   angles through the neighbouring symbol `g_WorldMapCameraRot` (base
//   0x8009BD38; +2 and +4 are the two view rotations) as s16 defeats it and
//   brings back retail's lui/lh/sll. That relocation is base+addend where retail
//   names the field, so diff_func never says MATCH for this body even when it is
//   right: judge it with check_bin.sh.
// - `v = -0x20;` before the block and `g_WorldMapCameraRot = v;` is what puts
//   -0x20 in $v0 as retail does; the literal is materialised later.
// - The Z angle needs its own variable (angleZ), loaded between the two 0x3C
//   stores; otherwise gcc coalesces it into the Y value's register and the whole
//   tail differs.
// - 0x960000 has to be a variable assigned just after the third position load.
//   As a literal it is materialised at its second use and the body shifts.
// Tried on the last tie: three permuter runs (565 -> 360 -> 155 -> 95, then
// NONE), u32/int/s32 for the constant, four declaration orders, and dropping the
// variable altogether (that one loses retail's single `lui`, 240 bytes).
// Re-measured, plus these: `-dg` says the constant is the ONLY pseudo in the body
// that needs a register (`1 regs to allocate: 78`, `78 preferences: 2`), and it
// conflicts with $v1/$a0, so $a0 comes from local-alloc's free-register scan -
// the mirror of retail's `$a1` with no reachable source lever found. Storing the
// 0x960000 as a literal and reading the field back for the global
// (`g_WorldMapCameraHeight = *(s32 *)(p + 0x5C)`) does not forward: 82.93%,
// 248 bytes, and it moves the position temp to $a0 too. The whole family is this
// shape - misc7's func_800827EC (parked there, 65.30) has retail's identical
// `lui a1,0x500000` / `sw a1,92(v1)` / `sw a1,(g_WorldMapCameraHeight)` tail, so
// the register is a property of the original's build, not of this body.
/*
s32 func_8008390C(s32 arg0) {
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];
    s32 cameraHeight;
    s32 v;
    u32 angleZ;
    u8 *p = (u8 *)pActor;

    *(s32 *)(p + 0x7C) = 0x1000;
    v = g_WorldMapPlayerPosition[0];
    g_WorldMapCameraTarget = v;
    g_WorldMapScrollTargetX = v;
    pActor->x = v;
    v = g_WorldMapPlayerPositionY;
    g_WorldMapCameraTargetY = v;
    g_WorldMapScrollTargetY = v;
    pActor->y = v;
    v = g_WorldMapPlayerPositionZ;
    cameraHeight = 0x960000;
    g_WorldMapCameraTargetZ = v;
    g_WorldMapScrollTargetZ = v;
    pActor->z = v;
    v = -0x20;
    do {
        pActor->state = 0;
        pActor->command = 0;
        *(s32 *)(p + 0x5C) = cameraHeight;
    } while (0);
    g_WorldMapCameraRot = v;
    g_WorldMapViewRotationY = 0x400;
    g_WorldMapViewRotationZ = 0;
    *(s32 *)(p + 0x50) = -0x20000;
    *(s32 *)(p + 0x38) = -0x20000;
    v = ((s16 *)&g_WorldMapCameraRot)[1];
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapCameraHeight = cameraHeight;
    *(s32 *)(p + 0x54) = v << 12;
    angleZ = ((s16 *)&g_WorldMapCameraRot)[2];
    *(s32 *)(p + 0x3C) = v << 12;
    v = angleZ;
    g_WorldMapFadeLevel = 0x78;
    *(s32 *)(p + 0x58) = v << 12;
    *(s32 *)(p + 0x40) = v << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083A00);

s32 WorldMapSetActorPosition(s32 arg0) {
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];
    s32 x;
    s32 y;
    s32 z;

    pActor->x = 0x1800000;
    pActor->y = 0x80000;
    pActor->z = 0x1A00000;
    pActor->state = 0;
    g_WorldMapGroups[8].hidden = 1;
    g_WorldMapGroups[7].hidden = 1;
    x = pActor->x >> 12;
    g_WorldMapGroups[8].x = x;
    g_WorldMapGroups[7].x = x;
    y = pActor->y >> 12;
    g_WorldMapGroups[8].y = y;
    g_WorldMapGroups[7].y = y;
    z = pActor->z >> 12;
    g_WorldMapGroups[8].z = z;
    g_WorldMapGroups[7].z = z;
    return 1;
}
