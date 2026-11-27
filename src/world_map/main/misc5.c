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

// PARKED 99.95% (876 bytes / 219 insns = retail's exact size, every instruction
// identical): the ONLY residual is the jump table's POSITION, not the C. Measured with
// check_bin.sh world_map: build/out/world_map.bin is 180422 bytes = disc's exact size
// and exactly 15 words differ - the table's 14 distinct words (retail's jtbl_8006FC50
// occupies file 0x160..0x263 with its zero pad at 0x264, ours has the pad at 0x160 and
// the table at 0x164..0x267) and the single `lw v0,%lo` addend at file 0xC904
// (0x8006FC54 against retail's 0x8006FC50). Nothing after 0x268 moves.
// WHY: misc5's object .rodata already holds WorldMapUpdateActorState's 12-word table at
// +0 (0x30 bytes) and WorldMapUpdateGroup0Actor's 9-word table at +0x68 (0x24 bytes), so the next C
// table starts at range offset 0x8C = 4 mod 8 and cc1's `.align 3` (lever
// i-jtbl-align-3-hardcoded) pads it to 0x90. The range's layout is [0x00 UAS 0x30 C]
// [0x30 ADD4 0x20 asm] [0x50 ADD4 0x18 asm] [0x68 BBEC 0x24 C] [0x8C this table 0x104]
// [0x190 four zero bytes] [0x194 C7D8 0x18 asm] [0x1AC C7D8 0x1C asm = range end], i.e.
// retail has exactly ONE pad, AFTER this table, right before the two tables at 0x194 and
// 0x1AC whose absolute addresses (0x8006FD58 / 0x8006FD70) are 8-aligned. So a table at
// range offset 0x8C was reachable in the 1998 build and is not reachable from one object
// today. FIX = orchestrator work, not a C spelling: func_8007C3B8 must be emitted by an
// object whose .rodata STARTS at 0x160, i.e. a main/misc5_b unit split with rodata
// 0x160..0x264 (exactly this table) and a plain `rodata` carve for 0x264..0x29C (the pad
// plus func_8007C7D8's two asm tables, which must NOT travel with the stub: emitted after
// this table they would land 4 bytes early); misc5_b also takes code from 0xC8C8
// (func_8007C3B8, WorldMapSnapCameraToPlayer, the func_8007C7D8 stub and everything after), needs
// `world_map/main/misc5_b.` added to XenoWorldMapDiv's paths in gears.toml (the presets
// are keyed per file), and its WorldMapPlayer/SVECTOR dependence means the prototypes and
// types its functions use move with it. Reverting WorldMapUpdateActorState and
// WorldMapUpdateGroup0Actor to stubs would also unblock this table and is the wrong trade: two
// matches lost for one.
// RE-MEASURED round 432 on this lane: un-parked, diff_func.sh prints 99.95% / 876
// bytes (retail's exact size), then re-parked. The pad is unconditional, not a
// function of our file order: the report-mode object's .rodata is 0x15C = UAS 0x30
// + BBEC 0x24 + a 4-byte .align 3 pad + this table 0x104, and BBEC's own 9-word
// table already shows the same pad in that object (next table 0x54 -> 0x58). So no
// spelling of this body can move the table; only the misc5_b split below can.
// THE BODY IS FINISHED, do not re-hunt it. Its last residual was case 0x10's
// `ori a1,a2,0xA0` + `sh zero,0xA2(a2)` pair, which sched2 sinks below the two
// g_WorldMapCameraTarget loads. Making the FIRST scratchpad store volatile pins it - and
// the call argument's address with it - exactly where retail has them: 98.13 -> 99.95 at
// the same 876 bytes (lever h-volatile-store-pins-the-arg-pair). The three plain store
// orders, the two shifts in locals, u8/s16/u16 scratch pointer locals and a 4-minute
// permuter run (2179 iterations, base=120 best=120) all measured 98.13 before that; a
// u16 *pTilePos = &pScratchPos[0x50] used for all three stores and the argument also
// measured 98.13.
// Structural facts that are settled and must be kept: the switch is over 0..0x40 and MUST
// keep `case 0: break;`, or gcc range-folds it to addiu -1 / sltiu and the table shifts
// one word; state is s16 and stateTimer u16 ((s16)--stateTimer < 0 gives
// lhu/addiu -1/sh/sll 16/bgez, exactly as WorldMapUpdateActorState case 1); the three
// loads of unk50 are retail's, not CSE failures; and the scratchpad base is the
// u16 *pScratchPos local at the top of the function with 0x50/0x51/0x52 indexes - that is
// what puts `lui a2,0x1F800000` in the beqz delay slot, forms the argument as
// `ori a1,a2,0xA0` and keeps the stores at 0xA0/0xA2/0xA4(a2). A scratch local declared
// inside the case gives a fresh lui $at per store.
// RE-MEASURED round 445 lane wm5: un-parked, diff_func.sh prints 99.95% / 876 bytes
// (retail's exact size); the only differing lines are .L label names, branch-target
// relocs and the table operand plus our lw v0,88(at) against retail's lw v0,0(at).
// permute.sh --minutes 3 printed RESULT PERMUTER_ZERO base=0 best=0 on its first score,
// i.e. the permuter scores this body as identical: no C spelling is left to find.
// check_bin.sh world_map with the body live: build/out/world_map.bin is 180422 bytes =
// disc's exact size and exactly 15 words differ, all inside the table range - 0x160..0x16C,
// 0x1A0..0x1BC, 0x260..0x264 (our 4-byte .align 3 pad at 0x160 against retail's leading
// entry, and retail's trailing zero word at 0x264 against our last entry, i.e. the table
// sits one word late with every other word identical) - plus the single lw addend at file
// 0xC904 (0x8006FC54 against retail's 0x8006FC50). No word outside the table moves, so
// the overlay does not shift and the unit split above remains the only fix.
// Also measured this round: maspsx does not emit the table pad - its only .align 3 is for
// .sbss entries, tools/maspsx/maspsx/__init__.py:560 - so the directive is cc1's and no
// assembler flag of this preset can move it.
// PARKED AGAIN, body unchanged and un-touched: do not re-hunt it.
// RE-MEASURED round 540 (audit pick): un-parked, diff_func.sh prints 99.95% / 876
// bytes (retail's exact size, no instruction differs) and permute.sh prints
// RESULT PERMUTER_ZERO base=0 best=0 on its first score, matching round 445.
// check_bin.sh world_map with the body live: 180422 bytes = disc's exact size, 30
// differing bytes - all inside the table (retail's 65 entries at file 0x160..0x263 then
// its zero word at 0x264; ours has the zero pad at 0x160 and the entries at 0x164..0x267)
// plus the single lw addend at 0xC904. Re-measured from the object: misc5's .rodata is
// 0x1C8 = the yaml range exactly, with cc1's pads at range 0x4C and 0x8C, so only this
// table is displaced. Nothing in the body changed; the split below is still the only fix.
/*
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C3B8);


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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C7D8);

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
