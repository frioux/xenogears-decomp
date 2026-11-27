#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

// Tile coordinates the sprite-group calls below read out of the scratchpad: the
// three halfwords at 0x1F8000A0 are one short vector, handed over by address.
#define WORLD_MAP_TILE_POS ((SVECTOR *)0x1F8000A0)

// sprite group this slot drives, and how long its activation lasts
#define WORLD_MAP_SPRITE_GROUP9 9
#define WORLD_MAP_SPRITE_GROUP9_TIMER 60

// the map display buffer copied into VRAM when a mode initialises, and the VRAM
// position it is copied to
#define WORLD_MAP_SCREEN_WIDTH 320
#define WORLD_MAP_SCREEN_HEIGHT 216
#define WORLD_MAP_SCREEN_VRAM_X 0x2C0
#define WORLD_MAP_SCREEN_VRAM_Y 0x100

// the archive directory this overlay's files are read through
#define WORLD_MAP_ARCHIVE_DIRECTORY_INDEX 0x24

// the two scene layers each group actor pair scales (4/5 and 6/7)
#define WORLD_MAP_GROUP_LAYER_COUNT 2

// the 12-bit fixed-point scale both layer updaters clamp to
#define WORLD_MAP_SCALE_MAX 0x7F00

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 command;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ u8 unk24[0x4];
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 w; // fourth word of the x/y/z vector, copied whole with it
    /* 0x38 */ u32 dirX;
    /* 0x3C */ u32 dirY;
    /* 0x40 */ u32 dirZ; // added to z every update
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60; // path speed acceleration per frame
    /* 0x64 */ u8 unk64[0x18];
    /* 0x7C */ s32 unk7C; // 12.12 proximity scale, walked between 0x1000 and 0xF000
} WorldMapPlayer; // Size: 0x80

// sound-effect bank header: +0x14 is the sed id sound commands are played from,
// the same field SoundFile declares at the same offset
typedef struct {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ u16 sedId;
} SoundSedsBank; // Size: unknown, only +0x14 is read

// One control point of the track table at g_WorldMapGroup0PathPoints (stride 8): three halfword
// coordinates that WorldMapInterpolateQuadraticVector reads, plus the -1
// terminator that ends a track.
typedef struct {
    /* 0x00 */ s16 vx;
    /* 0x02 */ s16 vy;
    /* 0x04 */ s16 vz;
    /* 0x06 */ s16 unk6;
} WorldMapTrackPoint; // Size: 0x8

typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 w;
} WorldMapPos; // Size: 0x10

// 0x54-byte scene-layer entry. g_WorldMapGroups is an array of them: the group 4..11
// layers sit at indexes 4..11 (file offsets 0x150..0x39C), layer 0 at offset 0.
typedef struct WorldMapSceneLayer {
    /* 0x00 */ s16 hidden; // nonzero: the sprite-set draw pass skips the entry
    /* 0x02 */ u8 unk2[0x6];
    /* 0x08 */ WorldMapPos pos;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ u8 *pPrimHeader; // header whose +0x4 is the prim count
    /* 0x44 */ u8 unk44[0x4];
    /* 0x48 */ u8 *pPrims; // prim list drawn for the layer
    /* 0x4C */ u8 *pPrimsCopy; // the layer's copy of pPrims
    /* 0x50 */ struct WorldMapSceneLayer *pParent; // layer WorldMapSetSpriteSetParent hangs this set off
} WorldMapSceneLayer; // Size: 0x54

// the prim template header a layer's pPrimHeader points at: +0x4 is how many
// prims WorldMapSetupPrimTemplates copies out of the layer's pPrims
typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ u16 primCount;
} WorldMapPrimHeader; // Size: unknown, only +0x4 is read

extern WorldMapPlayer *g_WorldMapSlots;
extern WorldMapSceneLayer *g_WorldMapGroups;
extern u16 g_WorldMapSlotStateIds[];
extern u16 g_WorldMapSlotStateFrames[];
extern WorldMapTrackPoint g_WorldMapGroup0PathPoints[];
extern u16 g_WorldMapActorParamStateIds[];
extern u16 g_WorldMapActorParamStateFrames[];
extern s16 g_WorldMapCameraRot;
// The three g_WorldMapCameraTarget words at 0x8009BE28 are one x/y/z triple. A field
// address of a struct-typed symbol survives gcc's `*&x` fold where the address of the
// plain global does not, and case 4 of func_8007ADD4 keeps that address in a register.
extern WorldMapPos g_WorldMapCameraTargetTriple __asm__("g_WorldMapCameraTarget");
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
// the two offsets accumulated by the random jitter below: [0] the path frame
// func_80097244 builds the view matrix from, [1] the one WorldMapBuildPathFrame fills
extern SVECTOR g_WorldMapPathFrame[];
extern u16 D_8009BD42;
extern u16 D_8009BD44;
extern u16 D_8009BD48;
extern u16 D_8009BD4A;
extern u16 D_8009BD4C;
extern s32 D_8009BD50;
extern s32 D_8009BD54;
extern s32 D_8009BD58;
extern WorldMapTrackPoint g_WorldMapCameraPathPoints[];
extern void WorldMapWrapDelta(void *pVec);
extern MATRIX g_WorldMapViewMatrix;
extern void WorldMapBuildPathFrame(void *, void *, s32, void *);
extern void func_80097244(s32 *pPos);
extern int rand(void);
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapPlayerPosition;
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern s32 g_WorldMapScrollZ;
extern s32 g_WorldMapRotationMatrixValid;
extern SoundSedsBank *g_MenuSedsFile;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 g_WorldMapExitFlag;
extern void WorldMapSetSpriteSetParent(int, int);
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
extern void WorldMapWrapPosition(void *p);
extern s32 func_80093A5C(s32, s32);
extern void WorldMapDeactivateSpriteGroup(s32);
extern void WorldMapInitSpriteGroupSprites(s32, void *, s32);
extern void WorldMapStartSlot(s32, s32);
extern void WorldMapInterpolateQuadraticVector(s32, s16 *, s16 *, s16 *, s32 *);
extern void WorldMapExtractRotationFromMatrix(MATRIX *, SVECTOR *);
extern void SoundPlaySoundEffectOnFreePair(s32);
extern void SoundSendCommandConstant(s32);
extern s16 g_WorldMapSpriteGroup9Rotation[];
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern s32 g_WorldMapMode4EntryTriggered;
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void WorldMapDrawFadeFrames(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern s32 ArchiveCdDataSync(s32 arg0);
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
extern void SoundAddSedsEntry(void *pSeds);
extern s32 ArchiveSetIndex(s32 arg0, s32 arg1);
extern void WorldMapSetGridPosition(void *pPos);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void WorldMapUpdateLoadQueue(void);
extern s32 Vsync(s32 arg0);
extern void WorldMapAddSlot(void *p0, void *p1);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern int WorldMapInitActorParamsFromTable(int idx);
extern int func_8007C3B8(int idx);
extern int WorldMapSnapCameraToPlayer(int idx);
extern int func_8007C7D8(int idx);
extern int WorldMapInitGroup4Actor(int idx);
extern int WorldMapUpdateGroup4Actor(int idx);
extern int WorldMapInitGroup5Actor(int idx);
extern int WorldMapUpdateGroup5Actor(int idx);
extern int WorldMapInitGroup9Actor(int idx);
extern int WorldMapUpdateGroup9Actor(int idx);
extern int WorldMapInitGroup10Actor(int idx);
extern int WorldMapUpdateGroup10Actor(int idx);
extern void WorldMapInitGroup12Actor(void);
extern void WorldMapUpdateGroup12Actor(void);
extern void WorldMapInitGroup13Actor(void);
extern void WorldMapUpdateGroup13Actor(void);
extern void WorldMapInitGroup16Actor(void);
extern void WorldMapUpdateGroup16Actor(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void WorldMapSetupPrimTemplates(void *pBlock, s32 arg, s32 arg2);

// The slot handler WorldMapAddSlot(WorldMapInitActorParamsFromTable, func_8007C3B8)
// registers: switches on pPlayer->state (65 cases). Its table has to sit at an 8-aligned
// rodata offset, which is the only reason this unit is split out of main/misc5.
s32 func_8007C3B8(s32 idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    u16 *pScratchPos = (u16 *)0x1F800000;

    switch (pPlayer->state) {
    case 0:
        break;
    case 1:
        if ((s16)--pPlayer->stateTimer < 0) {
            pPlayer->state = g_WorldMapActorParamStateIds[pPlayer->unk50];
            pPlayer->stateTimer = g_WorldMapActorParamStateFrames[pPlayer->unk50];
            pPlayer->unk50++;
        }
        break;
    case 2:
        WorldMapStartSlot(2, 1);
        pPlayer->state = 1;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0xD);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0xE);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0xF);
        break;
    case 0x10:
        *(volatile u16 *)&pScratchPos[0x51] = 0;
        pScratchPos[0x50] = g_WorldMapCameraTarget >> 12;
        pScratchPos[0x52] = g_WorldMapCameraTargetZ >> 12;
        WorldMapInitSpriteGroupSprites(0x14, (void *)&pScratchPos[0x50], 0);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x10);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x11);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x12);
        pPlayer->state = 1;
        break;
    case 0x11:
        WorldMapStartSlot(2, 2);
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 1;
        pPlayer->state = 1;
        g_WorldMapDrawMode = 0x40;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x13);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x14);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x15);
        break;
    case 0x12:
        WorldMapStartSlot(2, 3);
        WorldMapStartSlot(3, 1);
        WorldMapStartSlot(4, 1);
        WorldMapStartSlot(5, 1);
        WorldMapStartSlot(6, 1);
        WorldMapStartSlot(7, 1);
        WorldMapStartSlot(0, 0xC);
        g_WorldMapTpageAbr = 1;
        g_WorldMapDrawMode = 1;
        pPlayer->state = 1;
        break;
    case 0x13:
        WorldMapStartSlot(2, 4);
        WorldMapStartSlot(9, 1);
        pPlayer->state = 1;
        break;
    case 0x14:
        WorldMapStartSlot(2, 6);
        WorldMapStartSlot(3, 2);
        WorldMapStartSlot(9, 2);
        pPlayer->state = 1;
        break;
    case 0x16:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 2;
        g_WorldMapDrawMode = 4;
        pPlayer->state = 1;
        break;
    case 0x40:
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        pPlayer->state = 1;
        break;
    }
    return 1;
}


// The volatile casts, the pRot pointer local and the empty do-while are all
// load-bearing, not debris: without them gcc's -O2 scheduler hoists the three
// position loads above the unk7C store and sinks the six target stores below
// the unk58/unk50 stores (44.62% at the same 180 bytes); the do-while's basic-
// block boundary is what lets the unk50 store land in the jr delay slot.
int WorldMapSnapCameraToPlayer(int idx) {
    WorldMapPlayer *pPlayer;
    s32 a;
    s32 b;
    s16 *pRot;
    s32 c;

    *(volatile s32 *)&g_WorldMapFadeLevel = 0x78;
    *(volatile s32 *)&g_WorldMapCameraHeight = 0x400000;
    pPlayer = g_WorldMapSlots + idx;
    *(volatile s32 *)&g_WorldMapRotationMatrixValid = 1;
    *(volatile u32 *)&pPlayer->unk7C = 0x1000;
    a = *(volatile s32 *)&g_WorldMapPlayerPosition;
    pRot = &g_WorldMapCameraRot;
    b = *(volatile s32 *)&g_WorldMapPlayerPositionY;
    c = *(volatile s32 *)&g_WorldMapPlayerPositionZ;
    do {
    } while (0);
    *(volatile s16 *)pRot = -0x40;
    *(volatile s16 *)&g_WorldMapViewRotationY = 0;
    *(volatile s16 *)&g_WorldMapViewRotationZ = 0;
    *(volatile s32 *)&g_WorldMapCameraTarget = a;
    *(volatile s32 *)&g_WorldMapScrollTargetX = a;
    *(volatile s32 *)&g_WorldMapCameraTargetY = b;
    *(volatile s32 *)&g_WorldMapScrollTargetY = b;
    *(volatile s32 *)&g_WorldMapCameraTargetZ = c;
    *(volatile s32 *)&g_WorldMapScrollTargetZ = c;
    *(volatile s32 *)&pPlayer->unk58 = 0x40;
    pPlayer->unk50 = 0;
    return 1;
}

// PARKED 74.45% (1176 bytes / 294 insns; retail 1172 / 293) - JTBL_LAYOUT blocks
// conversion: do not convert this stub without moving the unit data to C as one job.
// Proof from the table offsets: the unit rodata range starts at 0x8006FBC4 (4 mod 8)
// and its five convertible tables sit at +0x30, +0x50, +0x8C, +0x194, +0x1AC, so the
// last three are 4 mod 8 while cc1 hardcodes .align 3 before every C jump table
// (lever i-jtbl-align-3-hardcoded); no C spelling can place a table at a 4 mod 8
// offset of that section. Same verdict as func_8007C3B8 (99.95%) and func_8007ADD4
// in this unit.
// BODY STATE: structure mirrors retail block for block - first switch over command-1
// (6 cases, case 4 empty), WorldMapBuildPathFrame called once after it under
// !g_WorldMapRotationMatrixValid, second switch over state 0..6 with cases 2 -> 1 -> 0
// falling through and 3/4/5 jumping to case 0 head. state >= 7 skips straight to the
// rand epilogue, spelled `default: goto skipPath;`: the earlier park left pPoint
// uninitialised on that path and objdiff cannot see UB (honest body measures 74.45,
// the UB body 74.06 at 1180 bytes).
// RESIDUAL (1 insn / 4 bytes over retail; frame 40 against retail 48): retail holds
// &g_WorldMapScrollTargetZ in $s0 and &g_WorldMapCameraTargetZ in $s1 across the
// WorldMapWrapDelta calls and re-reads/stores through them afterwards, while ours
// re-forms both addresses, so pPlayer lands in $s1 and the scratch base in $s2 where
// retail has $s2/$s3, and case 0 loses the sra v0,a0,12 delay slot to a nop.
// Measured and rejected: pointer locals assigned inside each case are folded away
// (identical 1176/294); pointer locals initialised at the top pin them but hoist both
// forms into the prologue and add $s4 (exact 1172/293 at 73.16%, wrong form sites);
// -G0 74.45, 2.7.2-cdk -G0 73.67, 2.6.0 74.00 - the preset is settled.
// If someone revisits: retail loads both Z globals BEFORE the two scratchpad zero
// stores and computes the subtraction after them.
// ROUND 442 (redo lane): the -G0 line above is STALE. try_flags.sh now measures
// 74.45 under -G0, 74.45 under -O3 and compile-failed under 2.6.0, i.e. the same
// object - these globals are all far from $gp, so -G8 and -G0 agree here and no
// alternative preset reaches retail's shape.
// THE $s0/$s1 ADDRESS PAIRS ARE REACHABLE, but only from a struct-typed declaration.
// cc1_probe on this unit measured which spellings survive gcc's `*&x` fold:
//   extern s32 g;  s32 *p = &g;              -> folded, bare `lw $2,g`
//   extern s32 g;  s32 *p = (s32 *)&g + 2;    -> FOLDED, bare `sw $4,g+8` (so the
//                                               `+2` spelling is NOT the answer)
//   extern P g;    int *p = &g.z;             -> survives: `la $18,g+8` then
//                                               `lw 0($18)` / `sw $4,0($18)`, which is
//                                               exactly retail's $s0/$s1 pair
//   extern int g;  int *p = &((P *)&g)->z;    -> folded (a cast does not help)
// So the next lane should retype the six camera/scroll target symbols in this file as
// `extern WorldMapPos g_WorldMapCameraTarget;` / `...g_WorldMapScrollTarget;` (both
// triples are 12 contiguous bytes at 0x8009BE28 and 0x8009D55C) and update the three
// neighbours that use them: WorldMapInitActorScrollScale (.x/.y/.z at lines 323-328),
// WorldMapSnapCameraToPlayer (the volatile casts at 1083-1088) and WorldMapUpdateGroup4Actor
// (g_WorldMapScrollTargetZ = pPlayer->z). Same addresses, so the bytes should not
// move, but re-diff each of the three. Then form `pScrollZ = &g_WorldMapScrollTarget.z;`
// and `pCamZ = &g_WorldMapCameraTarget.z;` INSIDE the moveCamera block and again in
// case 6 (retail forms both pairs twice, at .L8007C950 and .L8007CA58).
// THE SPLIT THAT CLOSES THIS AND func_8007C3B8 TOGETHER (better than the 0x160/0x264
// recipe below): one main/misc5_b taking rodata [0x160, 0x29C) and code [0xC8C8, end).
// C3B8's 65-word table then sits at rel 0 and this function's two at rel 0x108/0x120,
// all three 8-aligned in absolute terms (0x8006FC50 / 0x8006FD58 / 0x8006FD70), so no
// `.align 3` pad appears and the range still tiles to 0x29C. Add
// "world_map/main/misc5_b." to XenoWorldMapDiv's paths in gears.toml.
// ONE BODY BUG still in the parked text below: retail sets the path frame's vx from
// `lh a0,2(s3)` (`((s16 *)pScratch)[1]`), not `pScratch[0] >> 16`. The honest form
// measured worse in round 442 (69.91% / 1188 bytes with the rest also rewritten), so
// the parked body is kept as it measured; fix it when the register shape is solved.
/*
s32 func_8007C7D8(s32 idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapTrackPoint *pPoint;
    s32 *pScratch = (s32 *)PSX_SCRATCH; // interpolated position (16.16), then the wrap delta
    s16 nCommand;
    s32 jitter;

    nCommand = (u16)pPlayer->command - 1;
    switch (nCommand) {
    case 0:
        pPlayer->state = 1;
        pPlayer->unk5C = 0x8200;
        pPlayer->command = 0;
        pPlayer->unk60 = 1;
        break;
    case 1:
        pPlayer->command = 0;
        pPlayer->state = 3;
        break;
    case 2:
        pPlayer->command = 0;
        pPlayer->state = 4;
        break;
    case 3:
        pPlayer->state = 5;
        pPlayer->unk5C = 0xA800;
        pPlayer->command = 0;
        pPlayer->unk60 = 8;
        break;
    case 4:
        break;
    case 5:
        pPlayer->state = 6;
        pPlayer->command = 0;
        pPlayer->unk50 = 0;
        pPlayer->unk58 = 0x40;
        break;
    }
    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildPathFrame(g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      &g_WorldMapCameraRot);
    }
    switch (pPlayer->state) {
    case 2:
        pPlayer->unk58 -= pPlayer->unk60;
        if (pPlayer->unk58 < 0) {
            pPlayer->unk58 = 0;
            pPlayer->state = 0;
        }
        // fallthrough
    case 1:
        pPlayer->unk50 += pPlayer->unk58;
        if (pPlayer->unk50 > pPlayer->unk5C) {
            pPlayer->state = 2;
        }
        // fallthrough
    case 0:
    moveCamera:
        pScratch[1] = 0;
        pScratch[0] = 0;
        pScratch[2] = g_WorldMapScrollTargetZ - g_WorldMapCameraTargetZ;
        WorldMapWrapDelta(pScratch);
        g_WorldMapScrollZ += pScratch[2];
        g_WorldMapCameraTargetZ = g_WorldMapScrollTargetZ;
        pPoint = &g_WorldMapCameraPathPoints[33 + (pPlayer->unk50 >> 12)];
        break;
    case 3:
        pPlayer->unk7C = 0xF000;
        pPlayer->state = 0;
        goto moveCamera;
    case 4:
        ((u16 *)pScratch)[0x51] = 0;
        ((u16 *)pScratch)[0x50] = g_WorldMapCameraTarget >> 12;
        ((u16 *)pScratch)[0x52] = g_WorldMapCameraTargetZ >> 12;
        WorldMapInitSpriteGroupSprites(0x1A, &((u16 *)pScratch)[0x50], 0);
        WorldMapInitSpriteGroupSprites(0x1B, &((u16 *)pScratch)[0x50], 0);
        pPlayer->unk7C -= 0x120;
        if (pPlayer->unk7C < 0x1000) {
            pPlayer->unk7C = 0x1000;
            pPlayer->state = 0;
            WorldMapDeactivateSpriteGroup(0x1A);
            WorldMapDeactivateSpriteGroup(0x1B);
        }
        goto moveCamera;
    case 5:
        pPlayer->unk58 = 0x100;
        pPlayer->state = 1;
        goto moveCamera;
    case 6:
        pScratch[1] = 0;
        pScratch[0] = 0;
        pScratch[2] = g_WorldMapScrollTargetZ - g_WorldMapCameraTargetZ;
        WorldMapWrapDelta(pScratch);
        g_WorldMapScrollZ += pScratch[2];
        g_WorldMapCameraTarget = g_WorldMapScrollTargetX;
        g_WorldMapCameraTargetY = g_WorldMapScrollTargetY;
        g_WorldMapCameraTargetZ = g_WorldMapScrollTargetZ;
        pPlayer->unk50 += pPlayer->unk58;
        if (pPlayer->unk50 >= 0x4801) { // past the end of the walk
            pPlayer->unk58--;
            if (pPlayer->unk58 < 0) {
                pPlayer->unk58 = 0;
            }
        }
        pPoint = &g_WorldMapCameraPathPoints[47 + (pPlayer->unk50 >> 12)];
        break;
    default:
        goto skipPath;
    }
    if (pPoint[2].unk6 != -1) {
        WorldMapInterpolateQuadraticVector(pPlayer->unk50 & 0xFFF, (s16 *)pPoint,
                                           (s16 *)&pPoint[1], (s16 *)&pPoint[2], pScratch);
        D_8009BD4C = 0;
        D_8009BD48 = 0;
        D_8009BD58 = 0;
        D_8009BD50 = 0;
        D_8009BD54 = -0x1000;
        g_WorldMapPathFrame[0].vx = pScratch[0] >> 16;
        D_8009BD44 = -(pScratch[2] >> 16);
        D_8009BD4A = g_WorldMapCameraTargetY >> 12;
        D_8009BD42 = pScratch[1] >> 16;
    }
    func_80097244((s32 *)g_WorldMapPathFrame);
    WorldMapExtractRotationFromMatrix(&g_WorldMapViewMatrix, (SVECTOR *)&g_WorldMapCameraRot);
skipPath:
    jitter = rand() % (pPlayer->unk7C >> 12) - (pPlayer->unk7C >> 13);
    ((u16 *)pScratch)[0x51] = jitter;
    D_8009BD42 += jitter;
    D_8009BD4A += ((u16 *)pScratch)[0x51];
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5_b", func_8007C7D8);

int WorldMapInitGroup4Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(4, 0);
    WorldMapSetSpriteSetParent(4, 2);
    WorldMapSetSpriteSetParent(4, 1);
    WorldMapSetSpriteSetParent(4, 3);
    pLayer = &g_WorldMapGroups[4];
    pLayer->hidden = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->dirZ = -0x4000;
    pPlayer->x = 0xD00000;
    pPlayer->dirY = 0;
    pPlayer->dirX = 0;
    pPlayer->y = 0;
    pPlayer->z = 0x400000;
    return 1;
}
int WorldMapUpdateGroup4Actor(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pBlock = (WorldMapSceneLayer *)&((u8 *)g_WorldMapGroups)[0x150];
    u16 *pScratchPos = (u16 *)0x1F800000;
    s32 scratchX;
    s32 scratchZ;
    s16 mode;

    mode = pPlayer->command;
    if (mode == 1) {
        pPlayer->command = 0;
        pPlayer->state = 1;
    } else if (mode == 2) {
        pPlayer->command = 0;
        pPlayer->state = 2;
    }
    pPlayer->z += pPlayer->dirZ;
    WorldMapWrapPosition(&pPlayer->x);
    pBlock->pos.x = pPlayer->x >> 12;
    pBlock->pos.y = pPlayer->y >> 12;
    pBlock->pos.z = pPlayer->z >> 12;
    pScratchPos[0x50] = pPlayer->x >> 12;
    pScratchPos[0x51] = pPlayer->y >> 12;
    pScratchPos[0x52] = pPlayer->z >> 12;
    WorldMapInitSpriteGroupSprites(0x13, (void *)&pScratchPos[0x50], 0);
    switch (pPlayer->state) {
    case 1:
        WorldMapInitSpriteGroupSprites(0x1F, (void *)&pScratchPos[0x50], 0);
        /* fallthrough */
    case 0:
        g_WorldMapScrollTargetZ = pPlayer->z;
        break;
    case 2:
        WorldMapInitSpriteGroupSprites(0x1F, (void *)&pScratchPos[0x50], 0);
        break;
    }
    return 1;
}
int WorldMapInitGroup5Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(5, 6);
    pLayer = &g_WorldMapGroups[5];
    pLayer->hidden = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->dirZ = -0x4000;
    pPlayer->x = 0xB00000;
    pPlayer->state = 0;
    pPlayer->dirY = 0;
    pPlayer->dirX = 0;
    pPlayer->y = 0;
    pPlayer->z = 0x200000;
    return 1;
}
int WorldMapUpdateGroup5Actor(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer = &g_WorldMapGroups[5];

    if (pPlayer->command == 1) {
        pPlayer->command = 0;
        pPlayer->state = 1;
    }
    pPlayer->z += pPlayer->dirZ;
    WorldMapWrapPosition(&pPlayer->x);
    pPlayer->y = func_80093A5C(pPlayer->x, pPlayer->z) - 0x4000;
    pLayer->pos.x = pPlayer->x >> 12;
    pLayer->pos.y = pPlayer->y >> 12;
    pLayer->pos.z = pPlayer->z >> 12;
    WORLD_MAP_TILE_POS->vx = pPlayer->x >> 12;
    // retail holds the scratchpad address in a general register for the vz store;
    // a fresh local folds back to the absolute-lui form (348 bytes, 95.72%), so the
    // dead index parameter carries it
    idx = 0x1F8000A0;
    WORLD_MAP_TILE_POS->vy = pPlayer->y >> 12;
    ((SVECTOR *)idx)->vz = pPlayer->z >> 12;
    WorldMapInitSpriteGroupSprites(0x15, WORLD_MAP_TILE_POS, 0);
    if (pPlayer->state == 1) {
        WORLD_MAP_TILE_POS->vx = pPlayer->x >> 12;
        WORLD_MAP_TILE_POS->vy = pPlayer->y >> 12;
        WORLD_MAP_TILE_POS->vz = pPlayer->z >> 12;
        WorldMapInitSpriteGroupSprites(0x1E, WORLD_MAP_TILE_POS, 0);
    }
    return 1;
}
int WorldMapInitGroup9Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(9, 7);
    WorldMapSetSpriteSetParent(9, 8);
    pLayer = &g_WorldMapGroups[9];
    pLayer->hidden = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->dirZ = -0x4000;
    pPlayer->dirY = 0;
    pPlayer->dirX = 0;
    pPlayer->x = 0xC00000;
    pPlayer->y = 0;
    pPlayer->z = 0;
    return 1;
}
int WorldMapUpdateGroup9Actor(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;

    if (pPlayer->command == 1) {
        pPlayer->command = 0;
        pLayer[9].hidden = 1;
        pLayer[7].hidden = 1;
        pLayer[8].hidden = 1;
        WorldMapDeactivateSpriteGroup(0x16);
        return 3;
    }
    pPlayer->z += pPlayer->dirZ;
    WorldMapWrapPosition(&pPlayer->x);
    pPlayer->y = func_80093A5C(pPlayer->x, pPlayer->z) - 0x4000;
    pLayer[9].pos.x = pPlayer->x >> 12;
    pLayer[9].pos.y = pPlayer->y >> 12;
    pLayer[9].pos.z = pPlayer->z >> 12;
    WORLD_MAP_TILE_POS->vx = pPlayer->x >> 12;
    WORLD_MAP_TILE_POS->vy = pPlayer->y >> 12;
    WORLD_MAP_TILE_POS->vz = pPlayer->z >> 12;
    WorldMapInitSpriteGroupSprites(0x16, WORLD_MAP_TILE_POS, 0);
    return 1;
}

int WorldMapInitGroup10Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(0xA, 0xB);
    pLayer = &g_WorldMapGroups[10];
    pLayer->hidden = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->dirZ = -0x4000;
    pPlayer->x = 0xE00000;
    pPlayer->dirY = 0;
    pPlayer->dirX = 0;
    pPlayer->y = 0;
    pPlayer->z = 0x100000;
    return 1;
}

int WorldMapUpdateGroup10Actor(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;

    if (pPlayer->command == 1) {
        pPlayer->command = 0;
        pLayer[10].hidden = 1;
        pLayer[11].hidden = 1;
        WorldMapDeactivateSpriteGroup(0x17);
        WORLD_MAP_TILE_POS->vx = pPlayer->x >> 12;
        WORLD_MAP_TILE_POS->vy = pPlayer->y >> 12;
        WORLD_MAP_TILE_POS->vz = pPlayer->z >> 12;
        WorldMapInitSpriteGroupSprites(0x1C, WORLD_MAP_TILE_POS, 0);
        return 3;
    }
    pPlayer->z += pPlayer->dirZ;
    WorldMapWrapPosition(&pPlayer->x);
    pPlayer->y = func_80093A5C(pPlayer->x, pPlayer->z) - 0x4000;
    pLayer[10].pos.x = pPlayer->x >> 12;
    pLayer[10].pos.y = pPlayer->y >> 12;
    pLayer[10].pos.z = pPlayer->z >> 12;
    WORLD_MAP_TILE_POS->vx = pPlayer->x >> 12;
    WORLD_MAP_TILE_POS->vy = pPlayer->y >> 12;
    WORLD_MAP_TILE_POS->vz = pPlayer->z >> 12;
    WorldMapInitSpriteGroupSprites(0x17, WORLD_MAP_TILE_POS, 0);
    return 1;
}
