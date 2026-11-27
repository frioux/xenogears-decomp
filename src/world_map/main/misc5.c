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

s32 WorldMapUpdateActorState(s32 idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;

    switch (pPlayer->state) {
    case 0:
        break;
    case 1:
        if ((s16)--pPlayer->stateTimer < 0) {
            pPlayer->state = g_WorldMapSlotStateIds[pPlayer->unk50];
            pPlayer->stateTimer = g_WorldMapSlotStateFrames[pPlayer->unk50];
            pPlayer->unk50++;
        }
        break;
    case 2:
        WorldMapInitSpriteGroupSprites(0x11, NULL, 0);
        pPlayer->state = 1;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 1);
        break;
    case 3:
        WorldMapStartSlot(2, 2);
        pPlayer->state = 1;
        WorldMapInitSpriteGroupSprites(0xF, NULL, 0);
        WorldMapInitSpriteGroupSprites(0x10, NULL, 0);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 4);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 5);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 6);
        break;
    case 4:
        WorldMapStartSlot(2, 3);
        WorldMapStartSlot(3, 1);
        pPlayer->state = 1;
        break;
    case 5:
        WorldMapStartSlot(2, 6);
        WorldMapStartSlot(4, 1);
        WorldMapStartSlot(5, 1);
        pPlayer->state = 1;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 7);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 8);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 9);
        break;
    case 6:
        WorldMapStartSlot(2, 4);
        pPlayer->state = 1;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0xA);
        SoundSendCommandConstant((g_MenuSedsFile->sedId << 16) | 0xB);
        SoundSendCommandConstant((g_MenuSedsFile->sedId << 16) | 0xC);
        break;
    case 7:
        WorldMapStartSlot(2, 5);
        pPlayer->state = 1;
        break;
    case 8:
        WorldMapStartSlot(2, 1);
        pPlayer->state = 1;
        break;
    case 9:
        WorldMapStartSlot(6, 1);
        WorldMapStartSlot(2, 7);
        pPlayer->state = 1;
        break;
    case 10:
        WorldMapStartSlot(0, 0xD);
        g_WorldMapDrawMode = 4;
        pPlayer->state = 1;
        break;
    case 11:
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        pPlayer->state = 1;
        break;
    }
    return 1;
}

int WorldMapInitActorScrollScale(int idx) {
    WorldMapPlayer *pPlayer;
    s32 a;
    s32 b;
    s32 c;

    a = g_WorldMapPlayerPosition;
    b = g_WorldMapPlayerPositionY;
    c = g_WorldMapPlayerPositionZ;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraHeight = 0x1C0000;
    g_WorldMapCameraRot = 0x40;
    g_WorldMapViewRotationY = 0x480;
    g_WorldMapViewRotationZ = 0;
    pPlayer = g_WorldMapSlots + idx;
    g_WorldMapCameraTarget = a;
    g_WorldMapScrollTargetX = a;
    g_WorldMapCameraTargetY = b;
    g_WorldMapScrollTargetY = b;
    g_WorldMapCameraTargetZ = c;
    g_WorldMapScrollTargetZ = c;
    // volatile store pins the sw after the six copies, as retail has it
    *(volatile s32 *)&pPlayer->unk50 = 0x1000;
    return 1;
}
// PARKED 84.96% (1068 bytes / 267 insns = retail's EXACT size and instruction count;
// check_bin.sh world_map on the live body leaves the overlay 180422 bytes with all 216
// differing bytes inside this function's code and the unit's rodata tables landing at
// retail's own offsets, so the rodata layout is NOT a blocker here the way it is for
// func_8007C3B8/func_8007C7D8). Structure is settled and matches retail block for block:
// the first switch is command-1 over 7 cases with gcc cross-jumping case 0/4/6 into one
// "command = 0; state = N" block, the second switch is state over 6 cases in which
// WorldMapBuildPathFrame is called per case (0,1,2,3,5) and case 4 has its own tail that jumps past
// it, and the jitter epilogue reads the two scratchpad halfwords twice each.
// THE BYTE-COUNT FIX THIS ROUND (1076 -> 1068): retail reaches g_WorldMapCameraTargetZ
// through an address kept in $a2 (lui+addiu, then lw 0(a2), sw 0(a2) in the beqz delay
// slot and one re-read). Under -G8 cc1 folds `s32 *p = &g_ScalarGlobal;` back to the bare
// symbol, which costs one insn for the store plus one for the rematerialised reload. The
// fold is defeated by taking the address of a FIELD of a struct-typed declaration: extern
// WorldMapPos g_WorldMapCameraTargetTriple __asm__("g_WorldMapCameraTarget"); plus
// `pTargetZ = &g_WorldMapCameraTargetTriple.z;` emits `lui a1,%hi / addiu a1,%lo+8` and
// loads/stores through $a1, and the function drops from 1076 to retail's 1068 bytes
// (82.67 -> 84.96). REQUIRES A NONZERO FIELD OFFSET: the same trick on the rotation triple
// (SVECTOR .vy, offset 2) emits `lui v0 / lhu v0,2(v0)` and rematerialises the store
// through $at - a wash (84.94), so it was reverted.
// RESIDUAL, all of it sched1/sched2's choice given our RTL - the -fno-schedule probes
// below prove the order is not reachable from the source: case 2/3 keep retail's
// command=0/state=0 store pair 2-4 slots late (retail: case 2 after the CameraRot store,
// case 3 at the top of the case), case 5 (command 6) hoists the g_WorldMapRotationMatrixValid
// store above both position loads and takes $a0/$a1 for 0xC7C00/0x3EC400 where retail uses
// $v1/$a0, case 4's post-clamp block sinks the PathFrame vx subu/sra/sh below the
// D_8009BD4C/D_8009BD48 stores, state 5 increments g_WorldMapViewRotationY through a
// rematerialised $at where retail holds it in $a3, and the epilogue's load interleave
// differs. Reordering the case-2 statements compiles BYTE-IDENTICALLY, which is the
// measurement that says the store order is the scheduler's output here.
// Preset screens on THIS body: unit flags 84.96, -G0 86.25, 2.7.2-cdk 70.26 (park
// history), -fno-schedule-insns 75.94, -fno-schedule-insns2 78.09, both 75.94-ish
// (compile-failed), so scheduling is load-bearing and no preset reaches retail. NOTE the
// old "-G0 86.32 needs a unit split" conclusion still stands but is no longer worth it:
// -G8 with the struct alias is within 1.3 points of -G0 and the -G0 route breaks four
// matched neighbours of this unit (WorldMapSnapCameraToPlayer 27.96, WorldMapUpdateActorState
// 99.95, WorldMapUpdateGroup0Actor 99.82, WorldMapUpdateGroup4Actor 99.94).
// Tried and rejected earlier: volatile on the two scratchpad stores (pins them, but a
// volatile store may not fill the jal delay slot where retail keeps the 0xA0 store);
// volatile on the case-5 rotation store; extern SVECTOR g_WorldMapPathFrame[] plus
// WorldMapPos g_WorldMapCameraTarget[] (same score, broke WorldMapSnapCameraToPlayer).
/*
int func_8007ADD4(s32 idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    u16 *pScratchPos = (u16 *)PSX_SCRATCH;
    s32 *pTargetZ;
    s16 *pViewRotationY;
    s16 nState;
    s16 nAngle;

    nState = pPlayer->command - 1;
    switch (nState) {
    case 0:
        pPlayer->command = 0;
        pPlayer->state = 1;
        break;
    case 1:
        pPlayer->command = 0;
        pPlayer->state = 2;
        pPlayer->unk50 = 0x40000;
        break;
    case 2:
        pPlayer->command = 0;
        pPlayer->state = 0;
        g_WorldMapCameraHeight = 0x640000;
        g_WorldMapCameraRot = -0x1E0;
        g_WorldMapViewRotationY = 0x480;
        g_WorldMapViewRotationZ = 0;
        break;
    case 3:
        pPlayer->command = 0;
        pPlayer->state = 0;
        g_WorldMapCameraHeight = 0x1E0000;
        g_WorldMapCameraRot = 0x40;
        g_WorldMapViewRotationY = 0x418;
        g_WorldMapViewRotationZ = 0;
        g_WorldMapRotationMatrixValid = 0;
        g_WorldMapCameraTargetZ = g_WorldMapPlayerPositionZ;
        break;
    case 4:
        pPlayer->command = 0;
        pPlayer->state = 3;
        break;
    case 5:
        pPlayer->command = 0;
        pPlayer->state = 4;
        pPlayer->unk58 = g_WorldMapPlayerPosition + 0xC7C00;
        g_WorldMapRotationMatrixValid = 1;
        pPlayer->unk5C = g_WorldMapPlayerPositionZ + 0x3EC400;
        break;
    case 6:
        pPlayer->command = 0;
        pPlayer->state = 5;
        break;
    }
    switch (pPlayer->state) {
    case 0:
        WorldMapBuildPathFrame(&g_WorldMapPathFrame[0], &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      &g_WorldMapCameraRot);
        break;
    case 1:
        pPlayer->unk50 += 0x200;
        if (pPlayer->unk50 > 0x8000) {
            pPlayer->unk50 = 0x8000;
            pPlayer->state = 0;
        }
        WorldMapBuildPathFrame(&g_WorldMapPathFrame[0], &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      &g_WorldMapCameraRot);
        break;
    case 2:
        pPlayer->unk50 -= 0x200;
        if (pPlayer->unk50 < 0x8000) {
            pPlayer->unk50 = 0x8000;
            pPlayer->state = 0;
        }
        WorldMapBuildPathFrame(&g_WorldMapPathFrame[0], &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      &g_WorldMapCameraRot);
        break;
    case 3:
        pPlayer->unk50 -= 0x100;
        if (pPlayer->unk50 < 0x1000) {
            pPlayer->unk50 = 0x1000;
            pPlayer->state = 0;
        }
        WorldMapBuildPathFrame(&g_WorldMapPathFrame[0], &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      &g_WorldMapCameraRot);
        break;
    case 4:
        pTargetZ = &g_WorldMapCameraTargetTriple.z;
        *pTargetZ += 0x3A000;
        if (*pTargetZ > 0x77FFFFF) {
            pPlayer->state = 0;
        }
        g_WorldMapPathFrame[0].vx = (pPlayer->unk58 - g_WorldMapCameraTarget) >> 12;
        D_8009BD4C = 0;
        D_8009BD48 = 0;
        nAngle = g_WorldMapCameraTargetY >> 12;
        D_8009BD4A = nAngle;
        D_8009BD42 = nAngle - 0x40;
        D_8009BD44 = (*pTargetZ - pPlayer->unk5C) >> 12;
        func_80097244((s32 *)&g_WorldMapPathFrame[0]);
        WorldMapExtractRotationFromMatrix(&g_WorldMapViewMatrix, (SVECTOR *)&g_WorldMapCameraRot);
        break;
    case 5:
        pViewRotationY = &g_WorldMapViewRotationY;
        *pViewRotationY += 4;
        if (*pViewRotationY > 0x600) {
            pPlayer->state = 0;
        }
        WorldMapBuildPathFrame(&g_WorldMapPathFrame[0], &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      &g_WorldMapCameraRot);
        break;
    }
    pScratchPos[0x50] = rand() % (pPlayer->unk50 >> 12) - (pPlayer->unk50 >> 13);
    pScratchPos[0x51] = rand() % (pPlayer->unk50 >> 12) - (pPlayer->unk50 >> 13);
    g_WorldMapPathFrame[0].vx += pScratchPos[0x50];
    D_8009BD48 += pScratchPos[0x50];
    D_8009BD42 += pScratchPos[0x51];
    D_8009BD4A += pScratchPos[0x51];
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007ADD4);


int WorldMapSetupGroup4And5ScaledPrims(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pBlock = &g_WorldMapGroups[4];
    WorldMapSceneLayer *pLayer = &g_WorldMapGroups[4];
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    s32 scale;
    s32 i;

    for (i = 0; i < WORLD_MAP_GROUP_LAYER_COUNT; i++) {
        WorldMapSetupPrimTemplates(pBlock, (s32)pLayer->pPrims,
                                   ((WorldMapPrimHeader *)pLayer->pPrimHeader)->primCount);
        pLayer++;
        pBlock++;
    }
    pPlayer->unk54 = 0;
    pPlayer->unk50 = 0;
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    scale = pPlayer->unk50;
    pScratch[1] = 0x1000;
    pScratch[2] = scale;
    pScratch[0] = scale;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    pBlock->mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].hidden = 1;
    pBlock->hidden = 1;
    return 3;
}

int WorldMapUpdateGroup4And5Scale(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    // second layer's scale; retail keeps this field's address in a register,
    // which is what makes the player pointer land in $a2
    s32 *pScale2 = &pPlayer->unk54;
    s32 px;
    s32 x;
    s32 y;
    s32 z;
    s32 w;

    if (pPlayer->command != 0) {
        pPlayer->command = 0;
        pLayer[5].hidden = 0;
        pLayer[4].hidden = 0;
    }
    px = g_WorldMapPlayerPosition >> 12;
    y = -0x40;
    pLayer[5].pos.y = y;
    pLayer[4].pos.y = y;
    // the block pins the x stores before the z pair's load, as retail has them
    do {
        pLayer[5].pos.x = px;
        pLayer[4].pos.x = px;
    } while (0);
    z = g_WorldMapPlayerPositionZ >> 12;
    pLayer[5].pos.z = z;
    pLayer[4].pos.z = z;
    pPlayer->unk50 += 0x180;
    if (pPlayer->unk50 > 0x800) {
        *pScale2 += 0x180;
    }
    if (pPlayer->unk50 > WORLD_MAP_SCALE_MAX) {
        pPlayer->unk50 = WORLD_MAP_SCALE_MAX;
    }
    if (*pScale2 > WORLD_MAP_SCALE_MAX) {
        *pScale2 = WORLD_MAP_SCALE_MAX;
    }
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    *(MATRIX *)&pScratch[0x44] = *(MATRIX *)&pScratch[0x3C];
    w = pPlayer->unk50;
    pScratch[2] = w;
    pScratch[0] = w;
    x = *pScale2;
    pScratch[5] = 0x1000;
    pScratch[1] = 0x1000;
    pScratch[6] = x;
    pScratch[4] = x;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    ScaleMatrix((MATRIX *)&pScratch[0x44], (VECTOR *)&pScratch[4]);
    pLayer[4].mat = *(MATRIX *)&pScratch[0x3C];
    pLayer[5].mat = *(MATRIX *)&pScratch[0x44];
    return 1;
}
int WorldMapSetupGroup6And7ScaledPrims(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pBlock = &g_WorldMapGroups[6];
    WorldMapSceneLayer *pLayer = &g_WorldMapGroups[6];
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    s32 scale;
    s32 i;

    for (i = 0; i < WORLD_MAP_GROUP_LAYER_COUNT; i++) {
        WorldMapSetupPrimTemplates(pBlock, (s32)pLayer->pPrims,
                                   ((WorldMapPrimHeader *)pLayer->pPrimHeader)->primCount);
        pLayer++;
        pBlock++;
    }
    pPlayer->unk54 = 0;
    pPlayer->unk50 = 0;
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    scale = pPlayer->unk50;
    pScratch[1] = 0x1000;
    pScratch[2] = scale;
    pScratch[0] = scale;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    pBlock->mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].hidden = 1;
    pBlock->hidden = 1;
    return 3;
}

int WorldMapUpdateGroup6And7Scale(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    // second layer's scale; retail keeps this field's address in a register,
    // which is what makes the player pointer land in $a2
    s32 *pScale2 = &pPlayer->unk54;
    s32 px;
    s32 x;
    s32 y;
    s32 z;
    s32 w;

    if (pPlayer->command != 0) {
        pPlayer->command = 0;
        pLayer[7].hidden = 0;
        pLayer[6].hidden = 0;
    }
    px = g_WorldMapPlayerPosition >> 12;
    y = -0x40;
    pLayer[7].pos.y = y;
    pLayer[6].pos.y = y;
    // the block pins the x stores before the z pair's load, as retail has them
    do {
        pLayer[7].pos.x = px;
        pLayer[6].pos.x = px;
    } while (0);
    z = g_WorldMapPlayerPositionZ >> 12;
    pLayer[7].pos.z = z;
    pLayer[6].pos.z = z;
    pPlayer->unk50 += 0x180;
    if (pPlayer->unk50 > 0x800) {
        *pScale2 += 0x180;
    }
    if (pPlayer->unk50 > WORLD_MAP_SCALE_MAX) {
        pPlayer->unk50 = WORLD_MAP_SCALE_MAX;
    }
    if (*pScale2 > WORLD_MAP_SCALE_MAX) {
        *pScale2 = WORLD_MAP_SCALE_MAX;
    }
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    *(MATRIX *)&pScratch[0x44] = *(MATRIX *)&pScratch[0x3C];
    w = pPlayer->unk50;
    pScratch[2] = w;
    pScratch[0] = w;
    x = *pScale2;
    pScratch[5] = 0x1000;
    pScratch[1] = 0x1000;
    pScratch[6] = x;
    pScratch[4] = x;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    ScaleMatrix((MATRIX *)&pScratch[0x44], (VECTOR *)&pScratch[4]);
    pLayer[6].mat = *(MATRIX *)&pScratch[0x3C];
    pLayer[7].mat = *(MATRIX *)&pScratch[0x44];
    return 1;
}

int WorldMapInitReturnThree(void) {
    return 3;
}

int WorldMapActivateSpriteGroup9Timed(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    s32 position;
    s16 timer;
    int nResult = 1;

    if (pPlayer->command == 1) {
        // the block keeps the pair ahead of the position loads; as plain statements
        // reorg sinks both sh into the following lw delay slots (332 bytes)
        do {
            pPlayer->command = 0;
            pPlayer->stateTimer = WORLD_MAP_SPRITE_GROUP9_TIMER;
        } while (0);
        position = g_WorldMapPlayerPosition;
        pPlayer->x = position;
        position = g_WorldMapPlayerPositionY;
        pPlayer->y = position;
        position = g_WorldMapPlayerPositionZ;
        pPlayer->z = position;
    }
    timer = pPlayer->stateTimer;
    timer--;
    pPlayer->stateTimer = timer;
    if (timer > 0) {
        pPlayer->x += -0x95D0;
        pPlayer->z += 0x2F130;
        WORLD_MAP_TILE_POS->vx = pPlayer->x >> 12;
        WORLD_MAP_TILE_POS->vy = pPlayer->y >> 12;
        WORLD_MAP_TILE_POS->vz = pPlayer->z >> 12;
        WorldMapInitSpriteGroupSprites(WORLD_MAP_SPRITE_GROUP9, WORLD_MAP_TILE_POS,
                      (s32)g_WorldMapSpriteGroup9Rotation);
    } else {
        pPlayer->stateTimer = WORLD_MAP_SPRITE_GROUP9_TIMER;
        *(WorldMapPos *)&pPlayer->x = *(WorldMapPos *)&g_WorldMapPlayerPosition;
        WorldMapDeactivateSpriteGroup(WORLD_MAP_SPRITE_GROUP9);
        nResult = 3;
    }
    return nResult;
}

int WorldMapInitGroup0Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(0, 1);
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    pLayer = g_WorldMapGroups;
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk50 = 0;
    pPlayer->unk54 = 0x100;
    pPlayer->unk58 = 0x100;
    pLayer->rot.vx = 0;
    pLayer->rot.vy = 0;
    pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    return 3;
}

s32 WorldMapUpdateGroup0Actor(s32 idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer;
    WorldMapTrackPoint *pPoint;
    u32 *pScratch = (u32 *)0x1F800000;
    VECTOR *pVec0;
    VECTOR *pVec1;
    VECTOR *pVec2;
    VECTOR *pVec3;
    SVECTOR *pRot;
    SVECTOR *pOutRot;
    MATRIX *pMat;
    s32 result = 1;

    switch (pPlayer->unk50 >> 12) {
    case 0:
    case 1:
        pPlayer->unk50 += pPlayer->unk58;
        pPlayer->unk54 += 4;
        break;
    case 2:
    case 3:
    case 4:
        pPlayer->unk50 += pPlayer->unk58;
        pPlayer->unk58 -= 8;
        if (pPlayer->unk58 < 0x80) {
            pPlayer->unk58 = 0x80;
        }
        pPlayer->unk54 += 4;
        break;
    case 5:
    case 6:
    case 7:
        pPlayer->unk50 += pPlayer->unk58;
        pPlayer->unk58 += 8;
        if (pPlayer->unk58 > 0x100) {
            pPlayer->unk58 = 0x100;
        }
        pPlayer->unk54 -= 0x10;
        break;
    case 8:
        result = 3;
        break;
    }
    pPoint = &g_WorldMapGroup0PathPoints[pPlayer->unk50 >> 12];
    if (pPoint[2].unk6 != -1) {
        WorldMapInterpolateQuadraticVector(pPlayer->unk50 & 0xFFF, (s16 *)pPoint,
                                           (s16 *)&pPoint[1], (s16 *)&pPoint[2],
                                           (s32 *)pScratch);
    }
    pVec1 = (VECTOR *)&pScratch[4];
    pPoint = &g_WorldMapGroup0PathPoints[(pPlayer->unk50 + 0x80) >> 12];
    WorldMapInterpolateQuadraticVector((pPlayer->unk50 + 0x80) & 0xFFF, (s16 *)pPoint,
                                       (s16 *)&pPoint[1], (s16 *)&pPoint[2], (s32 *)pVec1);
    pLayer = g_WorldMapGroups;
    pLayer->pos.x = (s16)(((s32 *)pScratch)[0] >> 16);
    pLayer->pos.y = ((s32 *)pScratch)[1] >> 16;
    pLayer->pos.z = ((s32 *)pScratch)[2] >> 16;
    pVec0 = (VECTOR *)pScratch;
    pVec1->vx = (pVec1->vx - pVec0->vx) >> 12;
    pVec1->vy = (pVec1->vy - pVec0->vy) >> 12;
    pVec1->vz = -((pVec1->vz - pVec0->vz) >> 12);
    VectorNormal(pVec1, pVec0);
    pRot = (SVECTOR *)&pScratch[0x28];
    pRot->vx = 0;
    pRot->vy = ratan2(pVec0->vx, pVec0->vz) & 0xFFF;
    pRot->vz = pPlayer->unk54;
    pMat = (MATRIX *)&pScratch[0x3C];
    RotMatrixYXZ(pRot, pMat);
    pRot->vx = 0;
    pRot->vy = -0x1000;
    pRot->vz = 0;
    ApplyMatrix(pMat, pRot, pVec1);
    pVec3 = (VECTOR *)&pScratch[0xC];
    OuterProduct12(pVec0, pVec1, pVec3);
    pVec2 = (VECTOR *)&pScratch[8];
    VectorNormal(pVec3, pVec2);
    OuterProduct12(pVec0, pVec2, pVec3);
    VectorNormal(pVec3, pVec1);
    pMat->m[0][0] = pVec2->vx;
    pMat->m[0][1] = pVec2->vy;
    pMat->m[0][2] = pVec2->vz;
    pMat->m[1][0] = pVec1->vx;
    pMat->m[1][1] = pVec1->vy;
    pMat->m[1][2] = pVec1->vz;
    pMat->m[2][0] = pVec0->vx;
    pMat->m[2][1] = pVec0->vy;
    pMat->m[2][2] = pVec0->vz;
    TransposeMatrix(pMat, &pLayer->mat);
    pOutRot = (SVECTOR *)&pScratch[0x2A];
    WorldMapExtractRotationFromMatrix(pMat, pOutRot);
    if (pLayer->pos.y >= -0x7F) {
        pRot->vx = pLayer->pos.x;
        pRot->vy = pLayer->pos.y;
        pRot->vz = pLayer->pos.z;
        pOutRot->vz = -*(u16 *)&pOutRot->vz;
        WorldMapInitSpriteGroupSprites(0x12, pRot, (s32)pOutRot);
    }
    return result;
}
void WorldMapInitializeModeWithGroupActors(void) {
    RECT rect;
    s32 *pPos = &g_WorldMapPlayerPosition;
    s32 *pScratch; // shared: one register for the position store and the group-13 handler

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
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    pScratch = pPos;
    *pScratch = 0xD00000;
    g_WorldMapPlayerPositionY = 0xFFF60000;
    g_WorldMapPlayerPositionZ = 0x400000;
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
    WorldMapAddSlot((void *)WorldMapModeInitFadeRect, (void *)func_800925A0);
    WorldMapAddSlot((void *)WorldMapInitActorParamsFromTable, (void *)func_8007C3B8);
    WorldMapAddSlot((void *)WorldMapSnapCameraToPlayer, (void *)func_8007C7D8);
    WorldMapAddSlot((void *)WorldMapInitGroup4Actor, (void *)WorldMapUpdateGroup4Actor);
    WorldMapAddSlot((void *)WorldMapInitGroup5Actor, (void *)WorldMapUpdateGroup5Actor);
    WorldMapAddSlot((void *)WorldMapInitGroup9Actor, (void *)WorldMapUpdateGroup9Actor);
    WorldMapAddSlot((void *)WorldMapInitGroup10Actor, (void *)WorldMapUpdateGroup10Actor);
    WorldMapAddSlot((void *)WorldMapInitGroup12Actor, (void *)WorldMapUpdateGroup12Actor);
    pScratch = (void *)WorldMapUpdateGroup13Actor;
    WorldMapAddSlot((void *)WorldMapInitGroup13Actor, pScratch);
    WorldMapAddSlot((void *)WorldMapInitGroup16Actor, (void *)WorldMapUpdateGroup16Actor);
    WorldMapAddSlot((void *)WorldMapEnterNop, (void *)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapInitRandomTimers();
}

void WorldMapExitToMap111(void) {
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
    g_NextMapId = 0x111;
    g_NextMapFlags = 2;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
}

int WorldMapInitActorParamsFromTable(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots;
    s32 *pCount;

    pPlayer += idx;
    pCount = &pPlayer->unk50;
    *pCount = 0;
    pPlayer->state = g_WorldMapActorParamStateIds[0];
    pPlayer->stateTimer = g_WorldMapActorParamStateFrames[*pCount];
    *pCount = *pCount + 1;
    return 1;
}

