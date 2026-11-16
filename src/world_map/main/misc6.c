#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"
#include "psyq/memory.h"

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
    /* 0x50 */ int unk50;
    /* 0x54 */ void *unk54;
    /* 0x58 */ void *unk58;
    /* 0x5C */ s32 unk5C[8]; // 0x5C..0x7B
    /* 0x7C */ s32 unk7C;
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x00 */ s16 unk00;
    /* 0x02 */ s16 unk02;
    /* 0x04 */ undefined16 unk04[2]; // 0x04..0x07
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ undefined32 unk14;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ void *pPrimHeader; // header whose +0x4 is the prim count
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ void *pPrims;
    /* 0x4C */ undefined32 pPrimsCopy;
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
extern u32 D_8009C5B0;
extern u32 D_8009C5B4;
extern u16 g_WorldMapScriptStateIds;
extern u16 g_WorldMapScriptStateFrames[];
extern u16 g_WorldMapSlotScales[];
extern u16 D_8009A684[];
extern WorldMapPath g_WorldMapActorScripts[];
extern u8 D_8009A660[];
extern u32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTarget[];
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 g_WorldMapScrollX[];
extern s32 D_8009BBBC;
extern u32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapAreaIndex;
extern u32 g_WorldMapScrollTargetX;
extern u32 g_WorldMapScrollTargetY;
extern u32 g_WorldMapScrollTargetZ;

extern void WorldMapSetupPrimTemplates(void *, void *, int);
extern void WorldMapSetSpriteSetParent(int, int);
extern void func_80089160(int, void *, void *);
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
extern u8 *g_MenuSedsFile;
extern u8 *g_WorldMapWorkBuffer1;
extern u8 *g_WorldMapWorkBuffer2;
extern u8 *g_WorldMapTilePrimBuffer;
extern u8 *g_WorldMapTilePrimBufferCopy;
extern u8 *g_WorldMapDecompressedMapData;
extern u16 g_WorldMapViewRotationY[];
extern u16 g_NextMapId;
extern u16 g_NextMapRotationY;
extern u16 g_NextMapFlags;
extern u32 D_8009BBC4;
extern u16 g_WorldMapAreaExitModes[];
extern void WorldMapInitGfxEnv(void);
extern void WorldMapDeactivateSpriteGroup(s32);
extern void func_80072DB4(int, int, int, int);
extern int ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX D_8009BE4C;
extern s32 g_WorldMapTpageAbr;
extern s32 D_8009D3CC;
extern s32 D_8009D804;
extern void WorldMapWrapTileVertexPositions(void);
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern int ArchiveCdDataSync(int);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern s16 g_WorldMapSpawnPositions[];
extern s16 D_8009A5B6[];
extern s16 D_8009A5B8[];
extern s16 D_8009A674[];
extern s16 D_8009A678[];
extern s32 g_WorldMapPrimPacketIndex;
extern void WorldMapSetPrimColors(u8 *, s32, s32, s32, s32);
extern void WorldMapBuildPolyFT4Templates(WorldMapGroup *, u8 *, int, int);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadTextureClut(void);
extern void func_800979C8(void);
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
extern void func_80097BC0(s32 *);
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
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern int WorldMapSetActorScriptTables(int);
extern void func_8007DE98(int);
extern int func_8007E450(int);
extern void func_8007E4E4(int);
extern int func_8007ECA4(int);
extern void func_8007EE34(int);
extern int WorldMapInitSlotActor(int);
extern int func_8007F968(int);
extern void func_8007FC8C(int);
extern int func_8007FD30(int);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapRelocateMapEntryOffsets(void);
extern void func_80085FE0(void);
extern void WorldMapInitNop(void);
extern void WorldMapUpdateFrame(void);
extern void WorldMapInitSlotAtScrollPosition(void);
extern void func_80080944(void);
extern void func_80080A28(void);
extern void func_80080AC4(void);
extern void func_80080370(void);
extern int func_80080578(int);
extern int func_80080600(int);
extern int WorldMapInitActorScript(int);
extern s16 g_WorldMapCameraRot;
extern u16 g_WorldMapViewRotationZ;
extern u16 g_WorldMapPathFrame;
extern u16 D_8009BD42;
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
    pGroup->unk00 = 0;
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
        SVECTOR *pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);

        pActor->command = 0;
        pRoot->groups[12].unk00 = 1;
        pRoot->groups[15].unk00 = 1;
        WorldMapDeactivateSpriteGroup(0x18);
        pScratch->vx = pActor->x >> 12;
        pScratch->vy = pActor->y >> 12;
        pScratch->vz = pActor->z >> 12;
        func_80089160(0x1D, pScratch, 0);
        return 3;
    }
    {
    SVECTOR *pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);

    pActor->z += pActor->dirZ;
    WorldMapWrapPosition((VECTOR *)&pActor->x);
    pActor->y = func_80093A5C(pActor->x, pActor->z) - 0x4000;
    pRoot->groups[12].x = pActor->x >> 12;
    pRoot->groups[12].y = pActor->y >> 12;
    pRoot->groups[12].z = pActor->z >> 12;
    pScratch->vx = pActor->x >> 12;
    pScratch->vy = pActor->y >> 12;
    pScratch->vz = pActor->z >> 12;
    func_80089160(0x18, pScratch, 0);
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
    pGroup->unk00 = 0;
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
    SVECTOR *pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);

    pActor->z += pActor->dirZ;
    WorldMapWrapPosition((VECTOR *)&pActor->x);
    pActor->y = func_80093A5C(pActor->x, pActor->z) - 0x4000;
    pGroup->x = pActor->x >> 12;
    pGroup->y = pActor->y >> 12;
    pGroup->z = pActor->z >> 12;
    pScratch->vx = pActor->x >> 12;
    pScratch->vy = pActor->y >> 12;
    pScratch->vz = pActor->z >> 12;
    func_80089160(0x19, pScratch, 0);
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
    pGroup->unk00 = 1;
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
        *(volatile s16 *)&pGroup->unk00 = 0;
        pActor->x = g_WorldMapCameraTarget[0];
        pActor->z = D_8009BE30 + 0x400000;
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
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    D_8009BE4C = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
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
    D_8009C5B0 = nOffset;
    /* split only here: with an inline << 12 the scheduler steals the slot */
    nOffset = D_8009A5B8[nIndex * 4];
    nOffset <<= 12;
    D_8009C5B4 = nOffset;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    func_800979C8();
    WorldMapQueueFileLoads();
    func_800736DC();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    g_GameCurLoadedWDS = SoundLoadWdsFile(g_WorldMapWdsFileBuffer, 0);
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    while (g_SoundControlFlags & 0x10) {
    }
    HeapFree(g_WorldMapWdsFileBuffer);
    SoundAddSedsEntry(g_MenuSedsFile);
    memcpy(g_SoundBankLoadBuffer, g_WorldMapSoundBankBuffer, ArchiveDecodeAlignedSize(g_WorldMapSoundBankFileIndex));
    g_SoundAudioManager = SoundCreateAudioManagerOnHeap(g_SoundBankLoadBuffer);
    SoundPlayAudioManager(g_SoundAudioManager, 0x7F, 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapSetActorScriptTables, func_8007DE98);
    WorldMapAddSlot(func_8007E450, func_8007E4E4);
    WorldMapAddSlot(func_8007ECA4, func_8007EE34);
    WorldMapAddSlot(WorldMapInitSlotActor, func_8007F968);
    WorldMapAddSlot(WorldMapInitSlotActor, func_8007F968);
    WorldMapAddSlot(WorldMapInitSlotActor, func_8007F968);
    WorldMapAddSlot(WorldMapInitSlotActor, func_8007F968);
    WorldMapAddSlot(WorldMapInitSlotActor, func_8007F968);
    WorldMapAddSlot(func_8007FC8C, func_8007FD30);
    WorldMapAddSlot(WorldMapEnterNop, WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    WorldMapInitRandomTimers();
}

void WorldMapExitToMap1A1(void) {
    s32 nIndex;
    u16 unkWord;
    u16 unkValue;

    SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0xF0);
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
    D_8009BBC4 = 1;
    g_NextMapRotationY = g_WorldMapViewRotationY[0];
    g_NextMapFlags = unkValue;
}

int WorldMapSetActorScriptTables(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    RECT rect;

    pActor->unk54 = g_WorldMapActorScripts[g_WorldMapAreaIndex].pStateIds;
    /* the original object relocates the second word of the pair against its own
       symbol D_8009A660 (= g_WorldMapActorScripts + 4) */
    pActor->unk58 = *(u16 **)&D_8009A660[g_WorldMapAreaIndex * sizeof(WorldMapPath)];
    pActor->unk50 = 0;
    pActor->state = *(u16 *)pActor->unk54;
    pActor->stateTimer = ((u16 *)pActor->unk58)[pActor->unk50];
    pActor->unk50++;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007DE98);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E450);
/* 41.32% (24.00% without the volatile casts). Same body as func_80080578 plus
   three stores. Retail is in strict source order; gcc 2.7.2's post-reload
   scheduler hoists every load (and `li` of a store constant) to the top of the
   block. Tried: locals for the three copies (24.00%), volatile-qualified loads
   + store (41.32%, the loads then land at the very top), -O1
   (75.00% on func_80080578), -O3 (27.21%), gcc 2.6.0 (27.03%), -mcpu=r8000
   (32.57%). PROVEN round 12: the unit IS scheduled, so no preset can fix this
   shape. -fno-schedule-insns2 keeps this at 49.68% (80080578) but drops every
   matched function in the file (D414 75.94, D600 75.94, D690 66.42, D774
   63.91, DE14 74.97, 8032C 76.47, vs 100.00 stock); -fno-schedule-insns2
   -fno-schedule-insns gives 80.59% on 80080578 with every register correct and
   only the addu two slots early (41.61-68.76 on the matched ones). Volatile on
   all eleven touched globals reproduces the exact master store order but moves
   the three sources to $5/$6/$7 (23.21%).
int func_8007E450(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    int nScale = 0x1000;
    u32 unkAc;
    u32 unkB0;
    u32 unkB4;
    int nOne;

    g_WorldMapRotationMatrixValid = 0;
    pActor->unk7C = nScale;
    unkAc = *(u32 *)g_WorldMapPlayerPosition;
    unkB0 = D_8009C5B0;
    unkB4 = D_8009C5B4;
    g_WorldMapFadeLevel = 0x78;
    nOne = 1;
    g_WorldMapCameraTarget = unkAc;
    g_WorldMapScrollTargetX = unkAc;
    D_8009BE2C = unkB0;
    g_WorldMapScrollTargetY = unkB0;
    D_8009BE30 = unkB4;
    g_WorldMapScrollTargetZ = unkB4;
    pActor->command = nOne;
    pActor->unk58 = 0x40;
    pActor->state = 0;
    pActor->unk50 = 0;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E4E4);
/* 86.92% (1716 bytes against retail's 1752) - two switches on
   pActor->command - 1 (24 cases) and pActor->state (26 cases), then the rand
   jitter tail. Structure, case order (retail emits the bodies in source order)
   and both dispatch chains now agree; the levers that got it here:
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
   is the lever to try. (2) In case 1's clamp branch retail stores D_8009BE30
   before `sh v0,0x20(s0)`, ours the other way round. (3) case 3's second test
   is `bnez` to the else with `lui a1,0xFFFB3C00` in the delay slot, ours is
   `beqz` with the lui before the branch. (4) case 4 and case 25 order the
   unk5C[0]/unk60 load against the lhu of g_WorldMapViewRotationY differently.
   The earlier note on func_80080600 (same shape, 79.99%) still applies for the
   tail: the u16 reload of 0xA2 is real, keep `*(u16 *)(pScratch + 0xA2)`.
s32 func_8007E4E4(int index) {
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
        g_WorldMapViewRotationY[0] = 0xF50;
        g_WorldMapViewRotationZ = 0;
        break;
    case 1:
        g_WorldMapCameraHeight = 0x4D0000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0xC60;
        g_WorldMapViewRotationY[0] = 0x380;
        g_WorldMapViewRotationZ = 0;
        break;
    case 2:
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0x30;
        g_WorldMapViewRotationY[0] = 0x160;
        g_WorldMapViewRotationZ = 0;
        break;
    case 3:
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0x30;
        g_WorldMapViewRotationY[0] = 0x960;
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
        pActor->unk5C[0] = -0x8000;
        break;
    case 16:
        pActor->state = 0x10;
        pActor->unk7C = 0x1000;
        g_WorldMapCameraHeight = 0x400000;
        pActor->command = 0;
        g_WorldMapCameraRot = 0xE0;
        g_WorldMapViewRotationY[0] = 0x798;
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
        D_8009BE30 += 0x6D300;
        if (g_WorldMapCameraTarget[0] <= 0x1F9DFFF && D_8009BE30 > 0x5998000) {
            g_WorldMapCameraTarget[0] = 0x1F9E000;
            D_8009BE30 = 0x5998000;
            pActor->state = 2;
            pActor->unk7C = 0x10000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            D_8009BBBC += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY[0] -= 8;
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
        D_8009BE2C += 0xFF80;
        D_8009BE30 += 0xFFF9AA80;
        if (g_WorldMapCameraTarget[0] <= 0x1497FFF && D_8009BE30 > 0x4AF1FFF) {
            g_WorldMapCameraTarget[0] = 0x1498000;
            D_8009BE30 = 0x4AF2000;
            pActor->state = 4;
            pActor->unk5C[0] = 0x20000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFB3C00;
            D_8009BBBC += 0xFFF9AA80;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapCameraHeight += 0x20000;
        g_WorldMapViewRotationY[0] += 0x20;
        g_WorldMapCameraRot -= 4;
        break;
    case 4:
        g_WorldMapViewRotationY[0] += pActor->unk5C[0] >> 12;
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
        D_8009BE30 += 0x6D300;
        if (g_WorldMapCameraTarget[0] <= 0x2151FFF && D_8009BE30 > 0x5AA3000) {
            g_WorldMapCameraTarget[0] = 0x2152000;
            D_8009BE30 = 0x5AA3000;
            pActor->state = 0x11;
            pActor->unk7C = 0x8000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            D_8009BBBC += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY[0] -= 8;
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
        D_8009BE30 += 0x6D300;
        if (g_WorldMapCameraTarget[0] <= 0x2151FFF && D_8009BE30 > 0x5AA3000) {
            g_WorldMapCameraTarget[0] = 0x2152000;
            D_8009BE30 = 0x5AA3000;
            pActor->state = 0x19;
            pActor->unk5C[1] = 0x10000;
        } else {
            g_WorldMapScrollX[0] += 0xFFFBD300;
            D_8009BBBC += 0x6D300;
        }
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        g_WorldMapViewRotationY[0] -= 8;
        g_WorldMapCameraRot += pActor->unk5C[0] >> 12;
        pActor->unk5C[0] += 0x300;
        g_WorldMapCameraHeight += 0x8000;
        if (g_WorldMapCameraRot >= 0x81) {
            g_WorldMapCameraRot = 0x80;
        }
        break;
    case 25:
        g_WorldMapViewRotationY[0] -= pActor->unk5C[1] >> 12;
        pActor->unk5C[1] -= 0x200;
        if (pActor->unk5C[1] < 0) {
            pActor->unk5C[1] = 0;
        }
        break;
    }
    nRandom = rand() % (pActor->unk7C >> 12) - (pActor->unk7C >> 13);
    *(u16 *)(pScratch + 0xA2) = nRandom;
    D_8009BD42 += nRandom;
    D_8009BD4A += *(u16 *)(pScratch + 0xA2);
    return 1;
}
*/

void WorldMapBuildPolyFT4Templates(WorldMapGroup *pGroup, u8 *pPrim, int count, int abr) {
    POLY_FT4 *pPoly;
    int i;

    for (i = 0, pPoly = (POLY_FT4 *)pPrim; i < count; i++, pPoly++) {
        setPolyFT4(pPoly);
        pPoly->tpage = GetTPage(0, abr, 0x300, 0x100);
        pPoly->clut = GetClut(0, 0x1FF);
        setSemiTrans(pPoly, 1);
        setRGB0(pPoly, 0x3C, 0x3C, 0xC0);
    }
    memcpy((u8 *)pGroup->pPrimsCopy, pGroup->pPrims, count * sizeof(POLY_FT4));
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007ECA4);
/* 78.97%: instruction multiset identical (400 bytes both), only the tail block
   after RotMatrixYXZ differs in evaluation order and register choice. Retail
   evaluates the first 109*dirX multiply after the D_8009A678 lh, keeps the
   index in $v1 and the return constant ($v0=1) in the middle of the block;
   gcc hoists the multiply and puts the return constant first. Tried: pActor
   assigned before/after the WorldMapSetSpriteSetParent calls (after is right - it is what
   puts the index in $s2 and moves addu into the first WorldMapBuildPolyFT4Templates delay
   slot), nIndex declared first vs last.
int func_8007ECA4(int index) {
    int nIndex;
    WorldMapActor *pActor;
    WorldMapRoot *pRoot;
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;

    WorldMapSetSpriteSetParent(1, 2);
    WorldMapSetSpriteSetParent(1, 3);
    pRoot = g_WorldMapGroups;
    pActor = &g_WorldMapSlots[index];
    WorldMapBuildPolyFT4Templates(&pRoot->groups[1], pRoot->groups[1].pPrims, *(u16 *)((u8 *)pRoot->groups[1].pPrimHeader + 4), 3);
    WorldMapBuildPolyFT4Templates(&pRoot->groups[2], pRoot->groups[2].pPrims, *(u16 *)((u8 *)pRoot->groups[2].pPrimHeader + 4), 3);
    WorldMapBuildPolyFT4Templates(&pRoot->groups[3], pRoot->groups[3].pPrims, *(u16 *)((u8 *)pRoot->groups[3].pPrimHeader + 4), 1);
    pGroup = &g_WorldMapGroups->groups[1];
    pRot = &pGroup->rot;
    pMatrix = &pGroup->mat;
    pGroup->unk00 = 0;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    nIndex = g_WorldMapAreaIndex;
    pActor->dirX = -0x85A;
    pActor->state = 0;
    pActor->dirY = 0;
    pActor->dirZ = 0xDA6;
    pActor->unk50 = D_8009A674[nIndex * 4] << 12;
    pActor->unk54 = (void *)(D_8009A678[nIndex * 4] << 12);
    pActor->x = *(u32 *)g_WorldMapPlayerPosition - pActor->dirX * 13952;
    pActor->y = D_8009C5B0;
    pActor->z = D_8009C5B4 - pActor->dirZ * 13952;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EE34);
// 100.00% in objdiff under -G0 (99.4 under the unit preset) but the C is 45 bytes
// the wrong length at link time, so -G0 is NOT the answer for this overlay (its
// globals are absolute symbols from undefined_syms_auto, and -G0 changes that
// addressing). Do not re-try -G0 here.

int WorldMapInitSlotActor(int index) {
    int nType = index - 4;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapGroup *pGroup = &g_WorldMapGroups->groups[nType + 4];
    u16 unkValue;
    int nAbr;

    if (nType == 4) {
        nAbr = 1;
    } else {
        nAbr = 3;
    }
    WorldMapBuildPolyFT4Templates(pGroup, pGroup->pPrims, *(u16 *)((u8 *)pGroup->pPrimHeader + 4), nAbr);
    pActor->dirX = -0x85A;
    pActor->dirZ = 0xDA6;
    pActor->state = 0;
    pActor->dirY = 0;
    unkValue = g_WorldMapSlotScales[nType];
    pActor->stateTimer = 0x3C;
    pActor->unk5C[0] = unkValue;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007F968);
// 99.81% (permuter 15, from 235): every instruction now agrees with retail EXCEPT
// the 8 bytes of stack frame - retail is 0x30 with ra/s0-s4 at 0x2C..0x18, so
// vars=8 (args=16, regs=24); mine is 0x28 and stores NO locals at all, so the
// eight bytes are a slot gcc allocated and then optimized away. Both disassemble
// to the same 804 bytes of code, so no C edit that changes an instruction can
// close it. What the permuter/objdiff last bought: inlining the pScratch+0xF0
// cast instead of a pRot local, chained stores (state = (command = 0),
// pAngle->vx = (pAngle->vz = 0)), (y >> 6) >> 6, and an empty 'if (1) {}'
// after case 4. Remaining objdiff 'diffs' are reloc-only and link identically:
// jtbl_800701CC vs .rodata, and addiu s1,s0,s4,%lo(D_1F8000A0/0xA8/0x110)
// where retail names the scratchpad symbols and this unit has none - giving them
// real names in symbol_addrs.world_map.txt plus 'extern' use here should let
// those three show as matched too. Do not re-park below 99.81 without checking
// the frame size first.
/*
int func_8007F968(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapActor *pRefActor = &g_WorldMapSlots[3];
    WorldMapGroup *pGroup = &g_WorldMapGroups->groups[index];
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 nState;
    s32 nScale;

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
        pActor->stateTimer = 0x3C;
        pActor->dirX = -0x85A;
        pActor->dirZ = 0xDA6;
        pActor->state = (pActor->command = 0);
        pActor->dirY = 0;
        pActor->unk5C[0] = D_8009A684[index];
        pGroup->unk00 = 0;
        pGroup->rot.vx = pGroup->rot.vy = pGroup->rot.vz = 0;
        RotMatrixYXZ(&pGroup->rot, &pGroup->mat);
        WorldMapDeactivateSpriteGroup(0x23);
        WorldMapDeactivateSpriteGroup(0x24);
        break;
    case 3:
        pActor->stateTimer = 4;
        pActor->state = 1;
        pActor->command = 0;
        pActor->unk5C[0] = D_8009A684[index];
        pGroup->unk00 = 0;
        pGroup->rot.vx = pGroup->rot.vy = pGroup->rot.vz = 0;
        RotMatrixYXZ(&pGroup->rot, &pGroup->mat);
        WorldMapDeactivateSpriteGroup(0x23);
        WorldMapDeactivateSpriteGroup(0x24);
        break;
    case 4:
        pActor->state = 1;
        pActor->command = 0;
        pActor->stateTimer = 0x1E;
        if (1) {
        }
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
        pAngle->vy = ratan2(0x85A, 0xDA6) & 0xFFF;
        func_80089160(0x23, pPos, pAngle);
        func_80089160(0x24, pPos, pAngle);
        break;
    }
    case 1:
        if ((s16)--pActor->stateTimer < 0) {
            pActor->stateTimer = 0;
            pActor->unk5C[0] -= 0x80;
        }
        break;
    }
    {
        MATRIX *pTmp = (MATRIX *)(pScratch + 0x110);
        VECTOR *pScale = (VECTOR *)pScratch;

        ((MATRIX *)(pScratch + 0xF0))->m[0][0] = 0xDA6;
        ((MATRIX *)(pScratch + 0xF0))->m[0][1] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[0][2] = 0x85A;
        ((MATRIX *)(pScratch + 0xF0))->m[1][0] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[1][1] = -0x1000;
        ((MATRIX *)(pScratch + 0xF0))->m[1][2] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[2][0] = -0x85A;
        ((MATRIX *)(pScratch + 0xF0))->m[2][1] = 0;
        ((MATRIX *)(pScratch + 0xF0))->m[2][2] = 0xDA6;
        pGroup->rot.vz = (pGroup->rot.vz + 0x100) & 0xFFF;
        RotMatrixYXZ(&pGroup->rot, pTmp);
        MulMatrix0((MATRIX *)(pScratch + 0xF0), pTmp, &pGroup->mat);
        pGroup->x = pActor->x >> 12;
        pGroup->y = (pActor->y >> 6) >> 6;
        pGroup->z = pActor->z >> 12;
        nScale = pActor->unk5C[0];
        pScale->vz = 0x2000;
        pScale->vy = nScale;
        pScale->vx = nScale;
        ScaleMatrix(&pGroup->mat, pScale);
    }
    if (pActor->unk5C[0] < 0) {
        pActor->unk5C[0] = 0;
        pGroup->unk00 = 1;
        return 3;
    }
    return 1;
}
*/

/* 88.66%: instruction multiset identical. Retail materialises all the
   immediates first (lui/ori v1 = 0x1498000, lui/ori a0 = 0x4AF2000,
   li v0 = 3 for the return) and only then starts storing, and it defers
   the unk58 = 0x80 store to the very end; gcc reorders my stores the
   other way (0x58 right after 0x54). Tried literals and int locals for
   the two big constants - gcc folds the locals straight back into the
   stores. Group bases are groups[9] (s1 + 0x2F4) and groups[10]
   (s1 + 0x348). Retail's store order is 0x28, 0x2C, 0x54, 0x20, 0x30, 0x50,
   0x58 (the last one deferred), which is the source order below.
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FC8C);
/* 88.66%
int func_8007FC8C(int index) {
    WorldMapRoot *pRoot = g_WorldMapGroups;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    int nX;
    int nZ;

    WorldMapSetupPrimTemplates(&pRoot->groups[9], pRoot->groups[9].pPrims, *(u16 *)((u8 *)pRoot->groups[9].pPrimHeader + 4));
    WorldMapSetupPrimTemplates(&pRoot->groups[10], pRoot->groups[10].pPrims, *(u16 *)((u8 *)pRoot->groups[10].pPrimHeader + 4));
    nX = 0x1498000;
    nZ = 0x4AF2000;
    pActor->x = nX;
    pActor->y = 0xFFF80000;
    pActor->unk54 = (void *)-0x800;
    pActor->state = 0;
    pActor->z = nZ;
    pActor->unk50 = 0;
    pActor->unk58 = (void *)0x80;
    return 3;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FD30);
/* 90.00%: every store, call and relocation is right and only 4 bytes short of
   retail's 0x240; the deltas are the two group pointers (retail keeps
   &g_WorldMapGroups->groups[9] in $s2 - computed in the bne delay slot - and
   groups[10] in $s3 - computed in the beqz delay slot - and stores through
   $s1 displacements), plus gcc hoisting the unk50/unk54 loads of the scratchpad
   block and picking different registers than retail's all-in-$v0. Note unk54
   and unk58 are void* in the struct: comparing them needs the (s32) casts below
   or gcc drops the clamp (sltu) and the whole unk58 branch.
int func_8007FD30(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapRoot *pRoot = g_WorldMapGroups;
    WorldMapGroup *pGroup9 = &pRoot->groups[9];
    WorldMapGroup *pGroup10 = &pRoot->groups[10];
    s32 nX;
    s32 nY;
    s32 nZ;

    if (pActor->command == 1) {
        pActor->command = 0;
    }
    nX = pActor->x >> 12;
    pGroup10->x = nX;
    pGroup9->x = nX;
    nY = pActor->y >> 12;
    pGroup10->y = nY;
    pGroup9->y = nY;
    nZ = pActor->z >> 12;
    pGroup10->z = nZ;
    pGroup9->z = nZ;
    pGroup10->mat = g_WorldMapIdentityMatrix;
    pGroup9->mat = pGroup10->mat;
    nX = pActor->unk50;
    *(s32 *)0x1F800008 = nX;
    *(s32 *)0x1F800000 = nX;
    *(s32 *)0x1F800014 = 0x1000;
    *(s32 *)0x1F800004 = 0x1000;
    nY = (s32)pActor->unk54;
    *(s32 *)0x1F800018 = nY;
    *(s32 *)0x1F800010 = nY;
    ScaleMatrix(&pGroup9->mat, (VECTOR *)0x1F800000);
    ScaleMatrix(&pGroup10->mat, (VECTOR *)0x1F800010);
    nX = pActor->unk50 + 0x180;
    pActor->unk50 = nX;
    if (nX > 0x7FFF) {
        pActor->unk50 = 0x7FFF;
    }
    nY = (s32)pActor->unk54 + 0x180;
    pActor->unk54 = (void *)nY;
    if (nY > 0x7FFF) {
        pActor->unk54 = (void *)0x7FFF;
    }
    WorldMapSetPrimColors(*(u8 **)((u8 *)pGroup9 + g_WorldMapPrimPacketIndex * 4 + 0x48), *(u16 *)((u8 *)pGroup9->pPrimHeader + 4),
                  pActor->unk58, pActor->unk58, pActor->unk58);
    WorldMapSetPrimColors(*(u8 **)((u8 *)pGroup10 + g_WorldMapPrimPacketIndex * 4 + 0x48), *(u16 *)((u8 *)pGroup10->pPrimHeader + 4),
                  pActor->unk58, pActor->unk58, pActor->unk58);
    nZ = (s32)pActor->unk58 - 3;
    pActor->unk58 = (void *)nZ;
    if (nZ < 0) {
        pActor->unk58 = NULL;
        return 3;
    }
    return 1;
}
*/

void WorldMapInitializeModeWithActors(void) {
    RECT rect;
    u32 *pPos;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    D_8009BE4C = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
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
    D_8009C5B0 = 0xFFF80000;
    D_8009C5B4 = 0x13C0000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    func_800979C8();
    func_800736DC();
    WorldMapRelocateMapEntryOffsets();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(g_MenuSedsFile);
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapInitActorScript, func_80080370);
    WorldMapAddSlot(func_80080578, func_80080600);
    WorldMapAddSlot(WorldMapInitSlotAtScrollPosition, func_80080944);
    WorldMapAddSlot(func_80080A28, func_80080AC4);
    WorldMapAddSlot(WorldMapInitNop, WorldMapUpdateFrame);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80085FE0();
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
    unkValue = g_WorldMapViewRotationY[0];
    g_NextMapId = 0x84;
    g_NextMapFlags = 2;
    D_8009BBC4 = 1;
    g_NextMapRotationY = unkValue;
}

int WorldMapInitActorScript(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->unk50 = 0;
    pActor->state = g_WorldMapScriptStateIds;
    pActor->stateTimer = g_WorldMapScriptStateFrames[pActor->unk50];
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080370);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080578);
/* 34.97%: byte count is retail's (34 insns) and the instruction multiset is
   identical; only the schedule differs. Retail's MEMORY ops are in strict
   source order (sw g_WorldMapRotationMatrixValid, sw 0x7C($v1), then the three D_8009C5xx loads,
   then sw g_WorldMapFadeLevel, then the six paired stores, then sh 4($v1), sh 0x20($v1));
   only the immediate loads are hoisted (li 0x1000 before the first store,
   li 1 before the pairs). gcc 2.7.2 sched2 hoists the three D_8009C5xx loads
   to the very top of the block, above the sll, which rotates everything after
   it. Confirmed the mechanism: compiled with -fno-schedule-insns2 the tail
   (li 1 / sh 4($v1) / six pairs in x,y,z order) is exactly retail's and only
   the head differs. Volatile on the three loads changes nothing (34.97%).
   Same body as func_8007E450 (whose note has the preset screen) and as
   misc7's func_800813E8 (76.59% there, where retail's sh pairs are first).
int func_80080578(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    int nScale = 0x1000;
    u32 unkAc;
    u32 unkB0;
    u32 unkB4;
    int nOne;

    g_WorldMapRotationMatrixValid = 0;
    pActor->unk7C = nScale;
    unkAc = *(u32 *)g_WorldMapPlayerPosition;
    unkB0 = D_8009C5B0;
    unkB4 = D_8009C5B4;
    g_WorldMapFadeLevel = 0x78;
    nOne = 1;
    g_WorldMapCameraTarget = unkAc;
    g_WorldMapScrollTargetX = unkAc;
    D_8009BE2C = unkB0;
    g_WorldMapScrollTargetY = unkB0;
    D_8009BE30 = unkB4;
    g_WorldMapScrollTargetZ = unkB4;
    pActor->command = nOne;
    pActor->state = 0;
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080600);
/* 79.99% (732 bytes vs retail's 764). Structure found so far: two switches.
   switch (command) {1,2,3} = the init the other group actors get (state, unk50,
   unk54/0x58 and unk5C[0]/unk5C[1] from g_WorldMapCameraRot/g_WorldMapViewRotationY << 12,
   g_WorldMapCameraHeight = 0x800000); then `if (g_WorldMapRotationMatrixValid == 0)
   func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot)` (4 args);
   then switch (state) {case 1: camera follow on unk50/unk54/unk5C[0] with
   clamps at 0x300000 / 0x2E0000 / -0x100000 and g_WorldMapCameraRot/BD3A = pos >> 12;
   case 3: unk7C -= 0x4000, floor 0x1000, then state = 0}; then
   `sh (rand() % (unk7C >> 12) - (unk7C >> 13))` to 0x1F8000A2 and
   D_8009BD42 += it, D_8009BD4A += *(u16*)0x1F8000A2 (u16: retail reloads the
   halfword with lhu instead of reusing the stored s16), return 1.
   The wall is the case-1 prologue: retail stores g_WorldMapCameraRot = 0x10 through a
   pointer held in $a0 and then READS IT BACK with lh 0(a0); our cse forwards
   the store to the load and folds both the address and the value (0x10 << 12
   becomes a constant 0x10000), which then reorders the whole case block.
   Tried: plain globals, a `s16 *pAngle = &g_WorldMapCameraRot` local (identical RTL
   after cse), array declaration. Volatile on the store/load is the obvious
   next thing to try (retail has no lw/sw of a pointer, so it is source-level).
   Note the same `extern u8 g_WorldMapPlayerPosition[]` trick that fixed WorldMapInitializeModeWithActors may be
   what retail used here too: 0x1F8000A2 is addressed as a displacement off a
   register holding 0x1F800000, i.e. `u8 *pScratch = (u8 *)PSX_SCRATCH` is right.
int func_80080600(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u8 *pScratch = (u8 *)PSX_SCRATCH;
    s16 *pAngle;
    s32 nValue;
    s32 nDelta;

    switch (pActor->command) {
    case 1:
        pActor->state = 1;
        pActor->command = 0;
        pAngle = &g_WorldMapCameraRot;
        *pAngle = 0x10;
        g_WorldMapViewRotationY = 0x8E0;
        g_WorldMapViewRotationZ = 0;
        pActor->unk50 = 0x800000;
        nValue = *pAngle << 12;
        pActor->unk58 = nValue;
        pActor->unk54 = nValue;
        g_WorldMapCameraHeight = 0x800000;
        nValue = g_WorldMapViewRotationY << 12;
        pActor->unk5C[1] = nValue;
        pActor->unk5C[0] = nValue;
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
        WorldMapWrapPosition((VECTOR *)&pActor->x);
        func_80096F18(&g_WorldMapPathFrame, g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot);
    }
    switch (pActor->state) {
    case 1:
        nDelta = pActor->unk50 - 0x10000;
        pActor->unk50 = nDelta;
        if (nDelta <= 0x2FFFFF) {
            pActor->unk50 = 0x300000;
        }
        nDelta = pActor->unk50 - g_WorldMapCameraHeight;
        if (nDelta != 0) {
            g_WorldMapCameraHeight += nDelta >> 5;
        }
        nDelta = pActor->unk54 - 0x4000;
        pActor->unk54 = nDelta;
        if (nDelta < -0x100000) {
            pActor->unk54 = -0x100000;
        }
        nDelta = pActor->unk54 - pActor->unk58;
        if (nDelta != 0) {
            nValue = pActor->unk58 + (nDelta >> 4);
            pActor->unk58 = nValue;
            g_WorldMapCameraRot = nValue >> 12;
        }
        nDelta = pActor->unk5C[0] - 0x10000;
        pActor->unk5C[0] = nDelta;
        if (nDelta <= 0x2DFFFF) {
            pActor->unk5C[0] = 0x2E0000;
        }
        nDelta = pActor->unk5C - pActor->unk5C[1];
        if (nDelta != 0) {
            nValue = pActor->unk5C[1] + (nDelta >> 5);
            pActor->unk5C[1] = nValue;
            g_WorldMapViewRotationY = nValue >> 12;
        }
        break;
    case 3:
        pActor->unk7C -= 0x4000;
        if (pActor->unk7C < 0x1000) {
            pActor->unk7C = 0x1000;
            pActor->state = 0;
        }
        break;
    }
    nValue = rand() % ((s32)pActor->unk7C >> 12);
    nValue -= (s32)pActor->unk7C >> 13;
    *(s16 *)(pScratch + 0xA2) = nValue;
    D_8009BD42 += nValue;
    D_8009BD4A += *(u16 *)(pScratch + 0xA2);
    return 1;
}
*/