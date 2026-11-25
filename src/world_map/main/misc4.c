#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/memory.h"
#include "system/model.h"

// the map display buffer copied into VRAM when a mode initialises, and the VRAM
// position it is copied to
#define WORLD_MAP_SCREEN_WIDTH 320
#define WORLD_MAP_SCREEN_HEIGHT 216
#define WORLD_MAP_SCREEN_VRAM_X 0x2C0
#define WORLD_MAP_SCREEN_VRAM_Y 0x100

// the archive directory this overlay's files are read through
#define WORLD_MAP_ARCHIVE_DIRECTORY_INDEX 0x24

// the grid angle advances by this step every rendered frame (same define as misc1.c)
#define WORLD_MAP_GRID_ANGLE_STEP 64

// a POLY_FT4's packet length in words past its tag (same define as misc10.c)
#define WORLD_MAP_PRIM_TAG_LEN 9

// the sprite group func_8007A430 draws and deactivates, and the length its state timer
// is reset to (the same 96 frames WorldMapSetActorTimer60 hands the slot)
#define WORLD_MAP_SPRITE_GROUP9 9
#define WORLD_MAP_ACTOR_TIMER_FRAMES 96

// the scratchpad SVECTOR the sprite-group draw calls read
#define WORLD_MAP_TILE_POS ((SVECTOR *)0x1F8000A0)

// the scale animation an actor runs: the angle advances by a fixed step each frame,
// wrapping at WORLD_MAP_SCALE_ANGLE_LIMIT, and each wrap advances the scale until it
// reaches the maximum that ends the animation.
#define WORLD_MAP_SCALE_ANGLE_STEP 0xC0
#define WORLD_MAP_SCALE_ANGLE_LIMIT 0x800
#define WORLD_MAP_SCALE_STEP 0x100
#define WORLD_MAP_ACTOR_SCALE_MAX 0x6000

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapPos4; // Size: 0x10

// One control point of the camera path table at g_WorldMapCameraPathPoints (stride 8): halfword
// coordinates plus the -1 terminator that ends the table at +0x6.
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ s16 unk6;
} WorldMapPathPoint; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 pad0[0x4];
    /* 0x04 */ s16 command;
    /* 0x06 */ u8 pad6[0x1A];
    /* 0x20 */ u16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ u8 pad24[0x4];
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 w; // fourth word of the x/y/z quad, copied whole with it
    /* 0x38 */ u8 pad38[0x4];
    /* 0x3C */ s32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ u8 pad44[0xC];
    // 0x50..0x64 are reused per slot mode, so no single name fits: the rotation
    // handlers accumulate a 12-bit angle in 0x50 by 0x54 (mask 0xFFF, fed to
    // RotMatrixYXZ) while the scroll handlers read 0x50 as a path position and
    // 0x58 as the speed added to it.
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ u8 pad68[0x4];
    /* 0x6C */ s32 scaleAngle; // 12-bit angle; rcos of it scales the model
    /* 0x70 */ s32 scale;
    /* 0x74 */ s32 scaleStep;
    /* 0x78 */ u8 pad78[0x8];
} WorldMapActor; // Size: 0x80

// One entry of the g_WorldMapGroups table (stride 0x54). 0x48 and 0x4C are the source
// and destination prim lists (g_WorldMapGroups[14]/[15] feed WorldMapSetupPrimTemplates).
typedef struct {
    /* 0x00 */ u16 hidden; // nonzero: the sprite-set draw pass skips this entry
    /* 0x02 */ u16 pad2;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ WorldMapPos4 pos;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ ModelPart *pModel;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ void *pPackets[2];
    /* 0x50 */ u32 unk50;
} WorldMapEntry; // Size: 0x54

// g_WorldMapGroups seen as its WorldMapEntry table (stride 0x54); the entries this unit
// touches are spelled out, the rest is untouched space.
typedef struct {
    /* 0x000 */ s32 unk0;
    /* 0x004 */ s32 unk4;
    /* 0x008 */ s32 x;
    /* 0x00C */ s32 y;
    /* 0x010 */ s32 z;
    /* 0x014 */ u8 pad14[0x4];
    /* 0x018 */ SVECTOR rot;
    /* 0x020 */ u8 pad20[0x478];
    /* 0x498 */ WorldMapEntry entry14;
    /* 0x4EC */ WorldMapEntry entry15;
    /* 0x540 */ WorldMapEntry entry16;
    /* 0x594 */ u8 pad594[0xC];
} WorldMapState; // Size: 0x5A0

// The world map frame render context g_WorldMapRenderContext points at: partial view of the
// two ordering table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long *pOt[2];
} WorldMapRenderContext; // partial view

extern WorldMapActor *g_WorldMapSlots;
extern u16 g_WorldMapSlotStateIds;
extern u16 g_WorldMapSlotStateFrames[];
extern WorldMapState *g_WorldMapGroups;
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapRotationMatrixValid;
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
extern void WorldMapInitSpriteGroupSprites(s32, void *, s32);
extern void WorldMapSetupPrimTemplates(WorldMapEntry *, POLY_FT4 *, s32);
extern u8 *g_MenuSedsFile;
extern void WorldMapWrapPosition(void *);
extern s32 func_80093A5C(s32, s32);
extern void SoundPlaySoundEffectOnFreePair(s32);

extern void HeapFree(void *pMem);
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void *p);
extern void WorldMapFreeSpriteSets(void);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeGridImages(void);
extern void WorldMapFreeSlotPool(void);
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 g_WorldMapExitFlag;
extern s16 g_WorldMapPathFrame;
extern s32 g_WorldMapScrollX;
extern s16 g_WorldMapSlotUpdateFlags;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapScrollZ[];
extern WorldMapRenderContext *g_WorldMapRenderContext;
extern s32 g_WorldMapGridAngle;
extern WorldMapPos4 g_WorldMapPlayerPosition;
extern void WorldMapBuildRotationMatrix(void *p);
extern void func_80097244(void *p);
extern void WorldMapEmitSpriteTrailEntries(void);
extern void func_80089C78(void);
extern void func_800848F4(void);
extern void WorldMapUpdateSlotGridOrigin(s32 *p);
extern void WorldMapBuildTileIdGrid(s32 *p);
extern void WorldMapWaitForLoadSlotFree(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32 *p);
extern void func_8009932C(u_long *pOt0, u_long *pOt1, s32 *pState);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void WorldMapDeactivateSpriteGroup(s32 arg0);
extern void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1);
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapMode4EntryTriggered;
extern s32 g_WorldMapWrapPositionsCallback;
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern void WorldMapInitGfxEnv(void);
extern void WorldMapDrawFadeFrames(s32, s32, s32, s32);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern void ArchiveCdDataSync(s32);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapLoadClutAndTpageIds(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void SoundAddSedsEntry(u8 *);
extern void ArchiveSetIndex(s32, s32);
extern void WorldMapSetGridPosition(WorldMapPos4 *);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void WorldMapUpdateLoadQueue(void);
extern s32 Vsync(s32);
extern void WorldMapAddSlot();
extern s32 WorldMapStartActorScript(s32);
extern void WorldMapUpdateActorState(void);
extern void func_8007ADD4(void);
extern void WorldMapUpdateGroup4And5Scale(void);
extern void WorldMapUpdateGroup6And7Scale(void);
extern void WorldMapUpdateGroup0Actor(void);
extern s32 WorldMapInitActorScrollScale(s32);
extern s32 WorldMapSetupGroup4And5ScaledPrims(s32);
extern s32 WorldMapSetupGroup6And7ScaledPrims(s32);
extern s32 WorldMapInitReturnThree(s32);
extern s32 WorldMapInitGroup0Actor(s32);
extern s32 WorldMapActivateSpriteGroup9Timed(s32);
extern s32 WorldMapInitActorSlotTimer40(s32 index);
extern void func_80078EA4(void);
extern void func_80096F18(s16 *pOut, s32 *pSrc, s32 nHeight, u16 *pAngles);
extern int rand(void);
extern s32 func_80079778(s32 index);
extern WorldMapPathPoint g_WorldMapCameraPathPoints[];
extern MATRIX g_WorldMapViewMatrix;
extern void WorldMapExtractRotationFromMatrix(MATRIX *, SVECTOR *);
extern void WorldMapInterpolateQuadraticVector(s32, s16 *, s16 *, s16 *, s32 *);
extern void WorldMapStartSlot(s32, s32);
extern void SoundFadeElementVolumeById(s32, s32, s32);
extern void SoundSetElementVolumeById(s32, s32);
extern s32 func_80094154(s32 *, s32 *);
// The camera jitter halfwords the path-frame update accumulates. func_80077E68 reads all
// six of them with lh, so they are s16; the parked func_80078EA4 body wants u16 for its
// own match and will have to reconcile this when it is converted.
extern s16 D_8009BD42;
extern s16 D_8009BD44;
extern s16 D_8009BD48;
extern s16 D_8009BD4A;
extern s16 D_8009BD4C;
extern s32 D_8009BD50;
extern s32 D_8009BD54;
extern s32 D_8009BD58;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern s32 WorldMapUpdateGroup14And15Scale(s32);
extern s32 func_8007A430(s32);
extern s32 WorldMapInitActorAtScrollPositionWithSound(s32);
extern s32 WorldMapSetupGroup14And15Prims(s32);
extern s32 WorldMapSetActorTimer60(s32);
extern s32 WorldMapReturnThree(void);
extern s32 WorldMapUpdateObjectAPosition(s32);
extern s32 WorldMapInitActorRotationMatrix(s32);
extern s32 WorldMapUpdateActorTerrainHeight(s32);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapInitRandomTimers(void);

void WorldMapExitToMap10E(void) {
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
    v = *(u16 *)&g_WorldMapViewRotationY;
    g_NextMapId = 0x10E;
    g_NextMapFlags = 0;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
}
s32 WorldMapResetActorSlotWithSound(s32 index) {
    WorldMapActor *p;
    u8 *q;

    p = g_WorldMapSlots;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraHeight = 0x200000;
    q = g_MenuSedsFile;
    p = p + index;
    p->state = 0;
    p->unk58 = 0;
    // do/while block makes cc1 schedule these stores after the actor cluster
    do { p->unk54 = 0; p->unk50 = 0; g_WorldMapViewRotationZ = 0; g_WorldMapViewRotationY = 0; } while (0);
    g_WorldMapCameraRot = 0;
    g_WorldMapRotationMatrixValid = 1;
    SoundPlaySoundEffectOnFreePair((((u16 *)q)[0xA] << 16) | 0xA4);
    p->stateTimer = 0x18;
    return 1;
}
// 92.97%, 1060 bytes = retail's exact size. Structure is decoded; the residual is three
//    scheduler/allocator ties: (1) entry - retail keeps the path-table base in $a0 and
//    stores p->unk54 AFTER the addu, this body keeps the base in $v1 and stores before the
//    sll; (2) case 0 - retail emits the p->state increment (addiu/sh) before the
//    g_WorldMapCameraTarget load and its sra, this body hoists the load above the store;
//    (3) tail - retail pairs each quad's two sll before its two sw, this body interleaves
//    sll,sw,sll,sw. Two things the body NEEDS: the switch must be 'switch ((s16)p->state)'
//    (a u16 switch expression makes gcc emit lhu + andi 0xffff + sltiu where retail has
//    lh + sltiu, i.e. one extra instruction; that u16 form measures 93.97 with the extra
//    andi, the signed form 92.97 without it), and D_8009BD42/48/4A must be s16 because
//    retail loads all six tail values with lh. The parked func_80078EA4 body below wants
//    those three globals as u16 for its own match - reconcile when that one is taken.
//    Measured from this body and NOT the answer: hoisting the six tail values into s32
//    locals to force retail's sll,sll,sw,sw pairing changes nothing (gcc folds them back
//    to the same RTL); a do/while around the six stores is 90.71%. Older permuter history
//    on a previous body: three runs, PERMUTER_BETTER 1950->920, only the 'mode' copy edit
//    survived and it has now been replaced by the signed switch. The copy block's 6+3
//    store order is retail's as written (x and z of each control point first, then the
//    three y halfwords); the natural x,y,z order is 91.53%.
// g_WorldMapPathFrame/D_8009BD42/D_8009BD44 are the SVECTOR at 0x8009BD40 and
// D_8009BD48/4A/4C the one at 0x8009BD48; D_8009BD50/54/58 are the three words of the
// vector at 0x8009BD50. WorldMapInterpolateQuadraticVector takes the s32* out as its
// last (stack) argument, as misc5's caller does.
// 92.97%, 1060 bytes = retail's exact size and instruction multiset. Structure is decoded;
//    residual is three ordering ties, re-measured this round (no twin exists: twin_for.py
//    reports none). (1) entry - retail keeps the path-table base in $a0, scales the index
//    into $v1 and stores p->unk54 AFTER the addu; this body stores before the sll and reuses
//    $v0 for the scale, so addu comes out index-first. (2) case 0 - retail emits the p->state
//    addiu/sh before the g_WorldMapCameraTarget lui/lw and its sra; this body hoists the load
//    above them. (3) tail - retail pairs each statement pair's two sll before its two sw,
//    this body interleaves sll,sw,sll,sw.
//    Two things the body NEEDS: the switch must be 'switch ((s16)p->state)' (a u16 switch
//    expression makes gcc emit lhu + andi 0xffff + sltiu where retail has lh + sltiu), and
//    D_8009BD42/48/4A must be s16 because retail loads all six tail values with lh. The
//    parked func_80078EA4 body below wants those three globals as u16 for its own match.
//    MEASURED DEAD this round, each byte-identical to the body below (92.97 / 1060 / 265):
//    the tail written as a typed WorldMapPos4 pair (pDst[0].x/.y/.z, pDst[1].x/.y/.z - gcc
//    canonicalises the struct member to the same RTL); the entry written as the offset-first
//    cast sum '(WorldMapPathPoint *)(grid * sizeof(WorldMapPathPoint) + (u32)table)'; the
//    case 0 vx as two statements 'pVec->vx = g_WorldMapCameraTarget >> 12; pVec->vx += 0x32;'.
//    Preset is not the question: gcc-2.7.2-cdk 80.85, -O3 fails to compile.
//    permute.sh 4 min: PERMUTER_BETTER base=2090 best=1264 and the only edit is
//    'pVec->vx = g_WorldMapCameraTarget; pVec->vx = (pVec->vx >> 12) + 0x32;' - it DOES buy
//    retail's residual-2 load position (95.26%) but emits the extra sw, 1064 bytes / 266
//    insns, so it cannot link and is not ported. That is the shape residual 2 wants: the
//    camera load has to sit after the state store and no spelling gets there for free.
/*
s32 func_80077E68(s32 index) {
    WorldMapActor *p;
    WorldMapPathPoint *pPoint;
    SVECTOR *pVec;
    WorldMapPos4 *pDst;
    s32 *pScratch;
    s32 grid;
    s32 dist;

    p = g_WorldMapSlots + index;
    pScratch = (s32 *)PSX_SCRATCH;
    grid = p->unk50 >> 12;
    pPoint = &g_WorldMapCameraPathPoints[grid];
    p->unk54 = grid;
    if (pPoint[2].unk6 != -1) {
        pVec = (SVECTOR *)((u8 *)pScratch + 0xA0);
        pVec[0].vx = pPoint[0].vx;
        pVec[0].vz = pPoint[0].vz;
        pVec[1].vx = pPoint[1].vx;
        pVec[1].vz = pPoint[1].vz;
        pVec[2].vx = pPoint[2].vx;
        pVec[2].vz = pPoint[2].vz;
        pVec[0].vy = pPoint[0].vy;
        pVec[1].vy = pPoint[1].vy;
        pVec[2].vy = pPoint[2].vy;
        WorldMapInterpolateQuadraticVector(p->unk50 & 0xFFF, (s16 *)pVec, (s16 *)&pVec[1],
                                           (s16 *)&pVec[2], pScratch);
        // the interpolated position becomes the path frame's vx/vy/vz (16.16 -> 12.4)
        D_8009BD4C = 0;
        D_8009BD48 = 0;
        D_8009BD58 = 0;
        D_8009BD50 = 0;
        D_8009BD54 = -0x1000;
        ((SVECTOR *)&g_WorldMapPathFrame)->vx = pScratch[0] >> 16;
        D_8009BD44 = -(pScratch[2] >> 16);
        D_8009BD42 = pScratch[1] >> 16;
        ((SVECTOR *)&D_8009BD48)->vy = g_WorldMapCameraTargetY >> 12;
    }
    func_80097244(&g_WorldMapPathFrame);
    WorldMapExtractRotationFromMatrix(&g_WorldMapViewMatrix, &g_WorldMapCameraRot);
    switch ((s16)p->state) {
    case 0:
        p->unk58 = p->unk58 + 5;
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer >= 0) {
            break;
        }
        p->unk58 = 0x80;
        p->state = p->state + 1;
        pVec = (SVECTOR *)((u8 *)pScratch + 0xA0);
        pVec->vy = -0x20D;
        pVec->vx = (g_WorldMapCameraTarget >> 12) + 0x32;
        pVec->vz = (g_WorldMapCameraTargetZ >> 12) - 0x70;
        WorldMapInitSpriteGroupSprites(8, pVec, 0);
        break;
    case 1:
        if (p->unk50 >= 0x6000) {
            p->state = p->state + 1;
        }
        break;
    case 2:
        p->unk58 = p->unk58 - 2;
        if (p->unk58 >= 0x19) {
            break;
        }
        p->unk58 = 0x18;
        p->state = p->state + 1;
        break;
    case 3:
        if (p->unk50 > 0x7FFF) {
            p->state = p->state + 1;
        }
        break;
    case 4:
        p->unk58 = p->unk58 - 1;
        if (p->unk58 >= 5) {
            break;
        }
        p->unk58 = 4;
        p->state = p->state + 1;
        break;
    case 5:
        if (p->unk50 > 0x83FF) {
            SoundFadeElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0xA4, 0, 0x100);
            WorldMapStartSlot(0, 0xD);
            g_WorldMapTpageAbr = 2;
            g_WorldMapDrawMode = 4;
            p->stateTimer = 0x50;
            p->unk58 = 0;
            p->state = p->state + 1;
        }
        break;
    case 6:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        break;
    }
    p->unk50 = p->unk50 + p->unk58;
    if ((s16)p->state < 6) {
        pDst = (WorldMapPos4 *)pScratch;
        pDst[0].x = D_8009BD48 << 12;
        pDst[0].y = D_8009BD4A << 12;
        pDst[0].z = D_8009BD4C << 12;
        pDst[1].x = g_WorldMapPathFrame << 12;
        pDst[1].y = D_8009BD42 << 12;
        pDst[1].z = D_8009BD44 << 12;
        dist = func_80094154((s32 *)&pDst[0], (s32 *)&pDst[1]) >> 3;
        pScratch[8] = dist;
        SoundSetElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0xA4, 0x87 - dist);
    }
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077E68);



s32 WorldMapInitActorAtScrollPosition(s32 index) {
    WorldMapActor *p;
    s32 *pSlot;

    WorldMapSetSpriteSetParent(0, 1);
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    WorldMapSetSpriteSetParent(0, 4);
    WorldMapSetSpriteSetParent(0, 5);
    WorldMapSetSpriteSetParent(0, 6);
    WorldMapSetSpriteSetParent(0, 7);
    WorldMapSetSpriteSetParent(0, 8);
    WorldMapSetSpriteSetParent(0, 9);
    WorldMapSetSpriteSetParent(0, 10);
    WorldMapSetSpriteSetParent(0, 11);
    WorldMapSetSpriteSetParent(0, 12);
    WorldMapSetSpriteSetParent(0, 13);
    p = g_WorldMapSlots + index;
    p->state = 0;
    *(WorldMapPos4 *)&p->x = g_WorldMapPlayerPosition;
    pSlot = (s32 *)p;
    pSlot[0x15] = 0x40;
    pSlot[0x16] = 0x200;
    pSlot[0x17] = 0x60;
    pSlot[0x18] = 0x300;
    pSlot[0x14] = 0;
    pSlot[0x19] = 0x50;
    *(WorldMapPos4 *)&g_WorldMapCameraTarget = *(WorldMapPos4 *)&p->x;
    return 1;
}
// 93.60%, 1376 bytes = retail exact size, instruction multiset identical. Only the COPY CHAIN register hand-out differs: everything before it (prologue, the 0x1F800000 pos quad, the angle cluster, the four scratchpad rotation vectors) is byte-identical.
// Retail gives each copy run base=$v0, scratch source=$a2, temps=$v1/$a0/$a1 (run 4, where $v0 holds the return 1, base=$v1 source=$a3 temps=$a0/$a1/$a2); we get base=$a3, source=$a1, temps=$v0/$v1/$a0 for all four, with $a2 untouched.
// The scratchpad block must be in retail emitted store order (pRot[2].vz, pRot[2].vx, pRot[1].vz, ...): with the p->unk50/58/60 loads interleaved cc1 fills their load-delay slots with the two remaining zero stores, and no do/while barrier is needed. That alone took 91.53 to 93.60. Natural per-vector vx/vy/vz order NOT measured this round.
// Flat at 93.60 (436 differing lines each): a single reused MATRIX pointer local; four separate pointer locals; the chained pEntries[1] = pEntries[2] = pEntries[3] = src form (compiles identically); a WorldMapState pointer temp for the last two runs (coalesced away); -O3 (93.6); gcc-2.7.2-cdk-psx worse (82.13); gcc-2.6.0 and -G0 do not compile this unit. One pEntries assignment instead of four makes gcc CSE the load into one register and drop three reloads, so the four assignments stay.
// permute.sh 3 min from this body: PERMUTER_BETTER base=2200 best=1060; the only edit that helps routes two of the four loads through an inline WorldMapState-to-WorldMapEntry conversion helper, i.e. a pseudo-renumbering artifact with no realistic spelling that reproduces it, so it is not ported.
/*
s32 func_800783E8(s32 index) {
    WorldMapActor *p;
    WorldMapEntry *pEntries;
    SVECTOR *pRot;

    p = g_WorldMapSlots + index;
    p->z -= 0x4000;
    g_WorldMapScrollZ[0] -= 0x4000;
    WorldMapWrapPosition(&p->x);
    *(u32 *)0x1F800000 = p->x >> 12;
    *(u32 *)0x1F800004 = p->y >> 12;
    *(u32 *)0x1F800008 = p->z >> 12;
    ((WorldMapEntry *)g_WorldMapGroups)[0].pos = *(WorldMapPos4 *)0x1F800000;
    *(WorldMapPos4 *)&g_WorldMapCameraTarget = *(WorldMapPos4 *)&p->x;
    p->unk50 = (p->unk50 + p->unk54) & 0xFFF;
    p->unk58 = (p->unk58 + p->unk5C) & 0xFFF;
    p->unk60 = (p->unk60 + p->unk64) & 0xFFF;
    // the four RotMatrixYXZ source vectors, in scratchpad: 0xA0/A8/B0 are Y rotations,
    // 0xB8 a Z rotation
    pRot = (SVECTOR *)0x1F8000A0;
    pRot[2].vz = 0;
    pRot[2].vx = 0;
    pRot[1].vz = 0;
    pRot[1].vx = 0;
    pRot[0].vz = 0;
    pRot[0].vx = 0;
    pRot[0].vy = p->unk50;
    pRot[1].vy = p->unk58;
    pRot[2].vy = p->unk60;
    pRot[3].vy = 0;
    pRot[3].vx = 0;
    pRot[3].vz = p->unk60;
    RotMatrixYXZ((SVECTOR *)0x1F8000A0, (MATRIX *)0x1F8000F0);
    RotMatrixYXZ((SVECTOR *)0x1F8000A8, (MATRIX *)0x1F800110);
    RotMatrixYXZ((SVECTOR *)0x1F8000B0, (MATRIX *)0x1F800130);
    RotMatrixYXZ((SVECTOR *)0x1F8000B8, (MATRIX *)0x1F800150);
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[3].matrix = *(MATRIX *)0x1F800150;
    pEntries[2].matrix = pEntries[3].matrix;
    pEntries[1].matrix = pEntries[2].matrix;
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[5].matrix = *(MATRIX *)0x1F800130;
    pEntries[4].matrix = pEntries[5].matrix;
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[12].matrix = *(MATRIX *)0x1F8000F0;
    pEntries[8].matrix = pEntries[12].matrix;
    pEntries[10].matrix = pEntries[8].matrix;
    pEntries[6].matrix = pEntries[10].matrix;
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[13].matrix = *(MATRIX *)0x1F800110;
    pEntries[9].matrix = pEntries[13].matrix;
    pEntries[11].matrix = pEntries[9].matrix;
    pEntries[7].matrix = pEntries[11].matrix;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_800783E8);

s32 WorldMapEnterNop(void) {
    return 1;
}

s32 WorldMapUpdateFrameWithParticles(void) {
    s32 *pState;

    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildRotationMatrix(&g_WorldMapPathFrame);
    } else {
        func_80097244(&g_WorldMapPathFrame);
    }
    WorldMapEmitSpriteTrailEntries();
    func_80089C78();
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
void WorldMapInitializeModeWithTerrainActors(void) {
    RECT rect;

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
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 0x10;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = (s32)WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    g_WorldMapPlayerPosition.x = 0x2000000;
    g_WorldMapPlayerPositionY = -0x300000;
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
    WorldMapSetGridPosition(&g_WorldMapPlayerPosition);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapInitActorSlotTimer40, func_80078EA4);
    WorldMapAddSlot(WorldMapInitActorAtScrollPositionWithSound, func_80079778);
    WorldMapAddSlot(WorldMapSetupGroup14And15Prims, WorldMapUpdateGroup14And15Scale);
    WorldMapAddSlot(WorldMapSetActorTimer60, func_8007A430);
    WorldMapAddSlot(WorldMapReturnThree, WorldMapUpdateObjectAPosition);
    WorldMapAddSlot(WorldMapInitActorRotationMatrix, WorldMapUpdateActorTerrainHeight);
    WorldMapAddSlot(WorldMapEnterNop, WorldMapUpdateFrameWithParticles);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapInitRandomTimers();
}
void WorldMapExitToMap110(void) {
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
    v = *(u16 *)&g_WorldMapViewRotationY;
    g_NextMapId = 0x110;
    g_NextMapFlags = 0;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
}

s32 WorldMapInitActorSlotTimer40(s32 index) {
    WorldMapActor *p;
    s32 scroll;
    s32 timer;

    p = g_WorldMapSlots;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraHeight = 0x200000;
    g_WorldMapRotationMatrixValid = 0;
    p = p + index;
    scroll = -0xC0;
    p->state = 0x10;
    p->unk58 = 0;
    // the do/while blocks decide the store order and where the 0x40 lands
    do {
        p->unk54 = 0;
        p->unk50 = 0;
        g_WorldMapCameraRot = scroll;
        do { g_WorldMapViewRotationY = 0x680; } while (0);
        timer = 0x40;
        g_WorldMapViewRotationZ = 0;
    } while (0);
    p->stateTimer = timer;
    p->scaleStep = 0;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078EA4);
/* 84.53%, 1580 bytes; retail is 1588. The instruction multiset is retail's exactly
   EXCEPT for one missing `lui %hi / addiu %lo` pair, and the callee-saved numbering is
   rotated. Full decode; do not rewrite it from the assembly.
   - switch ((s16)p->state) has 19 entries (cases 0-7, 16, 17, 18; 8-15 default). The
     table's trailing 0 word at 0xBC is the .align 3 pad before jtbl_8006FBB0, so the C
     switch reproduces the whole misc4 rodata range: jtbl_8006FB40 (0x50, func_80077E68,
     still asm) then this table at 0x70, then the pad, then 0xB0.
   - the tail calls func_80096F18 when `p->state != 4 && p->state != 5` (sltiu on state-4).
   - the shake halfwords at 0x1F8000A0/0xA2 are each READ TWICE in one basic block, so
     they must be volatile: `*(volatile u16 *)(pScratch + 0xA0)`; without volatile cse
     collapses the second load. The scratch base must be a `u8 *` local (absolute
     `*(s16 *)0x1F8000A0` casts give lui $at per access, and a base+0xA0 pointer local
     loses retail's `0xA0($s3)`).
   - the four increments are NOT one vector: retail reaches 0x8009BD40 through a register
     (retail's $s0, `lhu 0(v0)` / `sh 0(v0)`) but 0x8009BD42, 0x8009BD48 and 0x8009BD4A
     through their own %hi/%lo each time. Writing them as `pFrame->vy` instead makes gcc
     emit `2(s0)` and loses 4 lui (1596 bytes). They are summed as unsigned halfwords,
     hence the u16 declarations above.
   - case 16 needs a LOCAL copy of p->command: the three calls may store it, so a plain
     `p->command` reloads with a second `lh` (retail loads once and stores that register
     to g_WorldMapTpageAbr).
   THE RESIDUAL: retail materialises &g_WorldMapPathFrame INSIDE each of case 4 and case 5
   and keeps it in $s0 across the calls (`addu a0,s0,zero` for the argument, `lhu 0(s0)` /
   `sh 0(s0)` after two rand() calls). The body below assigns the pointer once in the
   entry block; gcc keeps it in a register but materialises it once, so cases 4 and 5 lose
   one lui/addiu between them (-8 bytes; everything else is byte-count identical).
   Assigning the pointer inside each case - retail's shape - makes cse fold the address
   into every use (`lhu $2,g_WorldMapPathFrame` + `sh $2,g_WorldMapPathFrame` through $at,
   and `la a0` for the argument): 83.30%, 1596 bytes, 3 callee-saved registers instead of
   4. Measured and did NOT stop the fold: `(u16 *)(mem_addr)&sym`, `s16 *pFrame` with
   `*(u16 *)pFrame`, `volatile` on the pointer and on the pointee, `do { } while (0)`
   around the assignment, a block-scoped declaration, reassigning both at the top and in
   the cases, and `s16 state = p->state` before the switch.
   Register numbering is also rotated and could not be moved: retail $s0 = &pathFrame,
   $s1 = 0x2AAAAAAB (the `% 12` magic), $s2 = p, $s3 = 0x1F800000; this body gets
   $s0 = the magic, $s1 = p, $s2 = the scratch base, $s3 = &pathFrame, and the prologue
   save order differs (retail ra,s3,s2,s1,s0).
   permute.sh from the body below (4 min, -j2): PERMUTER_BETTER base=8359 best=4145, but
   the best candidate is wrong code (it replaces the stateTimer test with a stale variable
   assigned before `case 4:`), so nothing was ported.
*/
/*
s32 func_80078EA4(s32 index) {
    WorldMapActor *p;
    u16 *pFrame;
    u8 *pScratch;
    s16 command;

    p = g_WorldMapSlots + index;
    pScratch = (u8 *)PSX_SCRATCH;
    pFrame = (u16 *)&g_WorldMapPathFrame;
    switch ((s16)p->state) {
    case 0:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0x20;
        p->state = p->state + 1;
        WorldMapStartSlot(5, 1);
        SoundPlaySoundEffectOnFreePair((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x62);
        SoundPlaySoundEffectOnFreePair((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x63);
        break;
    case 1:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0x20;
        p->state = p->state + 1;
        WorldMapStartSlot(3, 1);
        break;
    case 2:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0x30;
        p->state = p->state + 1;
        WorldMapStartSlot(4, 1);
        WorldMapStartSlot(2, 1);
        WorldMapStartSlot(6, 1);
        break;
    case 3:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0x28;
        p->state = p->state + 1;
        break;
    case 4:
        func_80096F18((s16 *)pFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot);
        p->stateTimer = p->stateTimer - 1;
        *(volatile u16 *)(pScratch + 0xA0) = rand() % 12 - 6;
        *pFrame += *(volatile u16 *)(pScratch + 0xA0);
        D_8009BD48 += *(volatile u16 *)(pScratch + 0xA0);
        *(volatile u16 *)(pScratch + 0xA2) = rand() % 12 - 6;
        D_8009BD42 += *(volatile u16 *)(pScratch + 0xA2);
        D_8009BD4A += *(volatile u16 *)(pScratch + 0xA2);
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0x78;
        p->state = p->state + 1;
        SoundPlaySoundEffectOnFreePair((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x79);
        SoundFadeElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x62, 0, 0x100);
        SoundFadeElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x63, 0, 0x100);
        break;
    case 5:
        func_80096F18((s16 *)pFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot);
        p->stateTimer = p->stateTimer - 1;
        *(volatile u16 *)(pScratch + 0xA0) = rand() % 4 - 2;
        *(volatile u16 *)(pScratch + 0xA2) = rand() % 4 - 2;
        *pFrame += *(volatile u16 *)(pScratch + 0xA0);
        D_8009BD42 += *(volatile u16 *)(pScratch + 0xA2);
        D_8009BD48 += *(volatile u16 *)(pScratch + 0xA0);
        D_8009BD4A += *(volatile u16 *)(pScratch + 0xA2);
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0x5A;
        p->state = p->state + 1;
        break;
    case 6:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0x50;
        p->state = p->state + 1;
        WorldMapStartSlot(0, 0xD);
        SoundFadeElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x79, 0, 0x100);
        g_WorldMapTpageAbr = 2;
        g_WorldMapDrawMode = 4;
        break;
    case 7:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        p->stateTimer = 0;
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        break;
    case 16:
        command = p->command;
        if (command != 1) {
            break;
        }
        SoundFadeElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x36, 0, 8);
        WorldMapStartSlot(0, 0xD);
        g_WorldMapDrawMode = 0x20;
        g_WorldMapTpageAbr = command;
        p->stateTimer = 8;
        p->command = 0;
        p->state = p->state + 1;
        break;
    case 17:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        g_WorldMapCameraHeight = 0x960000;
        g_WorldMapCameraRot = -0x30;
        g_WorldMapViewRotationY = 0x40;
        g_WorldMapViewRotationZ = 0;
        p->stateTimer = 0x10;
        p->state = p->state + 1;
        break;
    case 18:
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        WorldMapStartSlot(0, 0xC);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 0x80;
        p->state = 0;
        p->stateTimer = 1;
        break;
    }
    if (p->state != 4 && p->state != 5) {
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot);
    }
    return 1;
}*/

s32 WorldMapInitActorRotationMatrix(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;

    p = g_WorldMapSlots + index;
    s = g_WorldMapGroups;
    p->x = 0x02000000;
    p->z = 0x01500000;
    p->state = 0;
    s->entry16.rot.vx = 0;
    s->entry16.rot.vy = 0x780;
    s->entry16.rot.vz = 0;
    RotMatrixYXZ(&s->entry16.rot, &s->entry16.matrix);
    return 1;
}
s32 WorldMapUpdateActorTerrainHeight(s32 index) {
    WorldMapActor *p;

    p = g_WorldMapSlots + index;
    if (p->command != 0) {
        p->command = 0;
        g_WorldMapGroups->entry16.hidden = 1;
    }
    WorldMapWrapPosition(&p->x);
    p->y = func_80093A5C(p->x, p->z) + 0x18000;
    g_WorldMapGroups->entry16.pos.x = p->x >> 12;
    g_WorldMapGroups->entry16.pos.y = p->y >> 12;
    g_WorldMapGroups->entry16.pos.z = p->z >> 12;
    return 1;
}
s32 WorldMapInitActorAtScrollPositionWithSound(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    u16 *q;
    s32 *pSlot;

    WorldMapSetSpriteSetParent(0, 1);
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    WorldMapSetSpriteSetParent(0, 4);
    WorldMapSetSpriteSetParent(0, 5);
    WorldMapSetSpriteSetParent(0, 6);
    WorldMapSetSpriteSetParent(0, 7);
    WorldMapSetSpriteSetParent(0, 8);
    WorldMapSetSpriteSetParent(0, 9);
    WorldMapSetSpriteSetParent(0, 10);
    WorldMapSetSpriteSetParent(0, 11);
    WorldMapSetSpriteSetParent(0, 12);
    WorldMapSetSpriteSetParent(0, 13);
    p = g_WorldMapSlots + index;
    p->state = 0;
    *(WorldMapPos4 *)&p->x = g_WorldMapPlayerPosition;
    pSlot = (s32 *)p;
    pSlot[0x15] = 0x40;
    pSlot[0x16] = 0x200;
    pSlot[0x17] = 0x60;
    pSlot[0x18] = 0x300;
    pSlot[0x14] = 0;
    pSlot[0x19] = 0x50;
    *(WorldMapPos4 *)&g_WorldMapCameraTarget = *(WorldMapPos4 *)&p->x;
    s = g_WorldMapGroups;
    q = g_MenuSedsFile;
    s->rot.vx = -0x100;
    s->rot.vy = 0;
    s->rot.vz = -0x40;
    SoundPlaySoundEffectOnFreePair((q[0xA] << 16) | 0x36);
    return 1;
}
s32 func_80079778(s32 index) {
    WorldMapActor *p;
    WorldMapEntry *pEntries;
    MATRIX *pMtx;
    u8 *pScratch;

    p = g_WorldMapSlots + index;
    pScratch = (u8 *)PSX_SCRATCH;
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    switch ((s16)p->state) {
    case 0: {
        SVECTOR *pVec = (SVECTOR *)(pScratch + 0xA0);
        pVec->vx = p->x >> 12;
        pVec->vy = p->y >> 12;
        pVec->vz = p->z >> 12;
        WorldMapInitSpriteGroupSprites(11, pVec, 0);
        p->y += 0x4000;
        p->z += -0x8000;
        g_WorldMapScrollZ[0] += -0x8000;
        if (p->y < -0x18000) {
            break;
        }
        p->dirY = -0x4000;
        p->state = p->state + 1;
        WorldMapInitSpriteGroupSprites(12, pVec, 0);
        SoundPlaySoundEffectOnFreePair((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x71);
        p->stateTimer = 0x20;
        break;
    }
    case 1: {
        SVECTOR *pVec = (SVECTOR *)(pScratch + 0xA0);
        pVec->vx = p->x >> 12;
        pVec->vy = p->y >> 12;
        pVec->vz = p->z >> 12;
        WorldMapInitSpriteGroupSprites(11, pVec, 0);
        WorldMapInitSpriteGroupSprites(12, pVec, 0);
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer <= 0) {
            WorldMapDeactivateSpriteGroup(12);
        }
        pEntries[0].rot.vx += 4;
        pEntries[0].rot.vz += 2;
        p->y += p->dirY;
        p->dirY += 0x100;
        p->z += -0x8000;
        g_WorldMapScrollZ[0] += -0x8000;
        if (p->y < -0x18000) {
            break;
        }
        p->dirZ = -0x8000;
        p->state = p->state + 1;
        WorldMapInitSpriteGroupSprites(12, pVec, 0);
        SoundPlaySoundEffectOnFreePair((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0x71);
        break;
    }
    case 2: {
        SVECTOR *pVec = (SVECTOR *)(pScratch + 0xA0);
        pVec->vx = p->x >> 12;
        pVec->vy = p->y >> 12;
        pVec->vz = p->z >> 12;
        WorldMapInitSpriteGroupSprites(11, pVec, 0);
        WorldMapInitSpriteGroupSprites(12, pVec, 0);
        pEntries[0].rot.vx -= 2;
        pEntries[0].rot.vz -= 1;
        p->z += p->dirZ;
        g_WorldMapScrollZ[0] += p->dirZ;
        p->dirZ += 0x100;
        if (p->z > 0x1580000) {
            break;
        }
        p->stateTimer = 8;
        p->state = p->state + 1;
        WorldMapDeactivateSpriteGroup(12);
        WorldMapStartSlot(1, 1);
        break;
    }
    case 3:
        pEntries[0].rot.vx -= 2;
        pEntries[0].rot.vz -= 1;
        p->z += p->dirZ;
        g_WorldMapScrollZ[0] += p->dirZ;
        p->dirZ += 0x100;
        p->stateTimer = p->stateTimer - 1;
        if ((s16)p->stateTimer > 0) {
            break;
        }
        WorldMapStartSlot(1, 1);
        p->state = p->state + 1;
        break;
    case 4:
        if (p->command == 0) {
            break;
        }
        p->command = 0;
        pEntries[13].hidden = 1;
        pEntries[12].hidden = 1;
        pEntries[11].hidden = 1;
        pEntries[10].hidden = 1;
        pEntries[9].hidden = 1;
        pEntries[8].hidden = 1;
        pEntries[7].hidden = 1;
        pEntries[6].hidden = 1;
        pEntries[5].hidden = 1;
        pEntries[4].hidden = 1;
        pEntries[3].hidden = 1;
        pEntries[2].hidden = 1;
        pEntries[1].hidden = 1;
        pEntries[0].hidden = 1;
        WorldMapDeactivateSpriteGroup(11);
        break;
    }
    WorldMapWrapPosition(&p->x);
    {
        WorldMapEntry *pMat = (WorldMapEntry *)g_WorldMapGroups;
        pMat[0].pos.x = p->x >> 12;
        pMat[0].pos.y = p->y >> 12;
        pMat[0].pos.z = p->z >> 12;
    }
    *(WorldMapPos4 *)&g_WorldMapCameraTarget = *(WorldMapPos4 *)&p->x;
    p->unk50 = (p->unk50 + p->unk54) & 0xFFF;
    p->unk58 = (p->unk58 + p->unk5C) & 0xFFF;
    p->unk60 = (p->unk60 + p->unk64) & 0xFFF;
    *(s16 *)(pScratch + 0x0B4) = 0;
    *(s16 *)(pScratch + 0x0B0) = 0;
    *(s16 *)(pScratch + 0x0AC) = 0;
    *(s16 *)(pScratch + 0x0A8) = 0;
    *(s16 *)(pScratch + 0x0A4) = 0;
    *(s16 *)(pScratch + 0x0A0) = 0;
    *(s16 *)(pScratch + 0x0A2) = p->unk50;
    *(s16 *)(pScratch + 0x0AA) = p->unk58;
    *(s16 *)(pScratch + 0x0B2) = p->unk60;
    *(s16 *)(pScratch + 0x0BA) = 0;
    *(s16 *)(pScratch + 0x0B8) = 0;
    *(s16 *)(pScratch + 0x0BC) = p->unk60;
    pMtx = (MATRIX *)(pScratch + 0x0F0);
    RotMatrixYXZ((SVECTOR *)(pScratch + 0x0A0), pMtx);
    RotMatrixYXZ((SVECTOR *)(pScratch + 0x0A8), (MATRIX *)(pScratch + 0x110));
    RotMatrixYXZ((SVECTOR *)(pScratch + 0x0B0), (MATRIX *)(pScratch + 0x130));
    RotMatrixYXZ((SVECTOR *)(pScratch + 0x0B8), (MATRIX *)(pScratch + 0x150));
    {
        WorldMapEntry *pMat = (WorldMapEntry *)g_WorldMapGroups;
        pMat[3].matrix = *(MATRIX *)(pScratch + 0x150);
        pMat[2].matrix = pMat[3].matrix;
        pMat[1].matrix = pMat[2].matrix;
    }
    {
        WorldMapEntry *pMat = (WorldMapEntry *)g_WorldMapGroups;
        pMat[5].matrix = *(MATRIX *)(pScratch + 0x130);
        pMat[4].matrix = pMat[5].matrix;
    }
    {
        WorldMapEntry *pMat = (WorldMapEntry *)g_WorldMapGroups;
        pMat[12].matrix = *(MATRIX *)(pScratch + 0x0F0);
        pMat[8].matrix = pMat[12].matrix;
        pMat[10].matrix = pMat[8].matrix;
        pMat[6].matrix = pMat[10].matrix;
    }
    {
        WorldMapEntry *pMat = (WorldMapEntry *)g_WorldMapGroups;
        pMat[13].matrix = *(MATRIX *)(pScratch + 0x110);
        pMat[9].matrix = pMat[13].matrix;
        pMat[11].matrix = pMat[9].matrix;
        pMat[7].matrix = pMat[11].matrix;
    }
    RotMatrixYXZ(&((WorldMapEntry *)g_WorldMapGroups)[0].rot, pMtx);
    ((WorldMapEntry *)g_WorldMapGroups)[0].matrix = *(MATRIX *)(pScratch + 0x0F0);
    return 1;
}


void WorldMapSetupPrimTemplates(WorldMapEntry *pEntry, POLY_FT4 *pPrim, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        setlen(pPrim, WORLD_MAP_PRIM_TAG_LEN);
        pPrim->code = 0x2C;
        pPrim->tpage = GetTPage(1, 3, 0x340, 0x100);
        pPrim->clut = GetClut(0x100, 0x1FF);
        pPrim->r0 = 0x80;
        pPrim->g0 = 0x80;
        setSemiTrans(pPrim, 1);
        pPrim->b0 = 0x80; // last store sets gcc's induction base to 0x6 as retail's does
        pPrim++;
    }
    memcpy(pEntry->pPackets[1], pEntry->pPackets[0], count * sizeof(POLY_FT4));
}

s32 WorldMapSetupGroup14And15Prims(s32 index) {
    WorldMapActor *p;

    p = g_WorldMapSlots + index;
    p->state = 0;
    p->scale = 0;
    p->scaleAngle = 0;
    WorldMapSetupPrimTemplates(&g_WorldMapGroups->entry14, g_WorldMapGroups->entry14.pPackets[0],
                  g_WorldMapGroups->entry14.pModel->polyCount);
    WorldMapSetupPrimTemplates(&g_WorldMapGroups->entry15, g_WorldMapGroups->entry15.pPackets[0],
                  g_WorldMapGroups->entry15.pModel->polyCount);
    return 3;
}
s32 WorldMapUpdateGroup14And15Scale(s32 index) {
    WorldMapEntry *pEntry;
    MATRIX *pMtxA;
    MATRIX *pMtxB;
    WorldMapActor *p;
    u32 *pScratch;
    s32 result;
    s32 angle;
    s32 scale;

    pEntry = (WorldMapEntry *)g_WorldMapGroups;
    pScratch = (u32 *)PSX_SCRATCH;
    p = g_WorldMapSlots + index;
    result = 1;
    pEntry[14].pos = pEntry[0].pos;
    pEntry[15].pos = pEntry[14].pos;
    p->scaleAngle += WORLD_MAP_SCALE_ANGLE_STEP;
    if (p->scaleAngle >= WORLD_MAP_SCALE_ANGLE_LIMIT) {
        p->scale = (p->scale + WORLD_MAP_SCALE_STEP) & 0x7FFF;
    }
    if (p->scale >= WORLD_MAP_ACTOR_SCALE_MAX) {
        result = 3;
        p->command = 0;
        p->scale = 0;
        p->scaleAngle = 0;
    }
    pMtxA = (MATRIX *)&pScratch[0x3C];
    *pMtxA = g_WorldMapIdentityMatrix;
    pMtxB = (MATRIX *)&pScratch[0x44];
    *pMtxB = *pMtxA;
    // the second scale is loaded before the two 0x1000 stores, as retail does
    angle = p->scaleAngle;
    pScratch[2] = angle;
    pScratch[0] = angle;
    scale = p->scale;
    pScratch[5] = 0x1000;
    pScratch[1] = 0x1000;
    pScratch[6] = scale;
    pScratch[4] = scale;
    ScaleMatrix(pMtxA, (VECTOR *)pScratch);
    ScaleMatrix(pMtxB, (VECTOR *)&pScratch[4]);
    pEntry[14].matrix = *pMtxA;
    pEntry[15].matrix = *pMtxB;
    return result;
}

s32 WorldMapSetActorTimer60(s32 index) {
    WorldMapActor *p;

    p = g_WorldMapSlots + index;
    p->stateTimer = 0x60;
    return 3;
}

// retail's twin: misc5.c's WorldMapActivateSpriteGroup9Timed. The s16 timer local and the
// WORLD_MAP_TILE_POS-> form (not a PSX_SCRATCH+0xA0 cast) are what reproduce retail's
// load/sra/sh interleave and its addiu a0,9 in the blez delay slot: 83.41 -> 100.00.
s32 func_8007A430(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    s16 timer;
    s32 result = 1;

    p = g_WorldMapSlots + index;
    if (p->command == 1) {
        s = g_WorldMapGroups;
        p->command = 0;
        p->x = s->x << 12;
        p->y = s->y << 12;
        p->z = s->z << 12;
    }
    timer = p->stateTimer;
    timer--;
    p->stateTimer = timer;
    if (timer > 0) {
        p->z += 0x20000;
        WORLD_MAP_TILE_POS->vx = p->x >> 12;
        WORLD_MAP_TILE_POS->vy = p->y >> 12;
        WORLD_MAP_TILE_POS->vz = p->z >> 12;
        WorldMapInitSpriteGroupSprites(WORLD_MAP_SPRITE_GROUP9, WORLD_MAP_TILE_POS, 0);
    } else {
        p->stateTimer = WORLD_MAP_ACTOR_TIMER_FRAMES;
        *(WorldMapPos4 *)&p->x = g_WorldMapPlayerPosition;
        WorldMapDeactivateSpriteGroup(WORLD_MAP_SPRITE_GROUP9);
        result = 3;
    }
    return result;
}



s32 WorldMapReturnThree(void) {
    return 3;
}

s32 WorldMapUpdateObjectAPosition(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    SVECTOR *pScratch;

    p = g_WorldMapSlots + index;
    s = g_WorldMapGroups;
    p->command = 0;
    pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);
    pScratch->vx = s->x;
    pScratch->vy = s->y;
    pScratch->vz = s->z;
    WorldMapInitSpriteGroupSprites(10, pScratch, 0);
    return 3;
}

/* 92.11%: a `s16 *dst = (s16 *)((u8 *)PSX_SCRATCH + 0xA0)` local (rather than
   three separate `*(s16 *)(PSX_SCRATCH + 0xA0/0xA2/0xA4)` casts, which scored
   67.74%) puts the lui/ori for $a1 where the original has them, and the stores
   reach the right addresses. Only difference left: gcc hoists the 2nd load
   before the 1st store (and puts the 1st value in $a0), so the three
   load/store pairs are not adjacent as in the original.
s32 WorldMapUpdateObjectAPosition(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    s16 *dst;

    p = g_WorldMapSlots + index;
    s = g_WorldMapGroups;
    p->command = 0;
    dst = (s16 *)((u8 *)PSX_SCRATCH + 0xA0);
    dst[0] = s->x;
    dst[1] = s->y;
    dst[2] = s->z;
    WorldMapInitSpriteGroupSprites(0xA, (s32)dst, 0);
    return 3;
}
*/
void WorldMapInitializeModeWithScaledGroups(void) {
    RECT rect;

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
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 0x10;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = (s32)WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    g_WorldMapPlayerPosition.x = 0x5BED000;
    g_WorldMapPlayerPositionY = -0xA0000;
    g_WorldMapPlayerPositionZ = 0x62A8000;
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
    WorldMapSetGridPosition(&g_WorldMapPlayerPosition);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapStartActorScript, WorldMapUpdateActorState);
    WorldMapAddSlot(WorldMapInitActorScrollScale, func_8007ADD4);
    WorldMapAddSlot(WorldMapSetupGroup4And5ScaledPrims, WorldMapUpdateGroup4And5Scale);
    WorldMapAddSlot(WorldMapSetupGroup6And7ScaledPrims, WorldMapUpdateGroup6And7Scale);
    WorldMapAddSlot(WorldMapInitReturnThree, WorldMapActivateSpriteGroup9Timed);
    WorldMapAddSlot(WorldMapInitGroup0Actor, WorldMapUpdateGroup0Actor);
    WorldMapAddSlot(WorldMapEnterNop, WorldMapUpdateFrameWithParticles);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapInitRandomTimers();
}
void WorldMapExitToMap11A(void) {
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
    v = *(u16 *)&g_WorldMapViewRotationY;
    g_NextMapId = 0x11A;
    g_NextMapFlags = 0;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
}
s32 WorldMapStartActorScript(s32 index) {
    WorldMapActor *p;

    p = g_WorldMapSlots + index;
    p->unk50 = 0;
    p->state = g_WorldMapSlotStateIds;
    p->stateTimer = g_WorldMapSlotStateFrames[p->unk50];
    return 1;
}
