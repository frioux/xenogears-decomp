#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/memory.h"
#include "psyq/rand.h"
#include "system/model.h"
#include "system/sound.h"

// The fade table is 0xC0 halfword entries, filled by WorldMapFillFadeTable; the slot's
// fade level walks between these two bounds.
#define WORLD_MAP_FADE_ENTRY_COUNT 0xC0
#define WORLD_MAP_FADE_LEVEL_MAX 0x40
#define WORLD_MAP_FADE_LEVEL_MIN 2

// The shade the fade quads are built with and reset to when a fade ends, and the shade
// their RGB channels ramp up to (the fast fade steps by 4, so it stops one step short).
#define WORLD_MAP_SHADE_NEUTRAL 0x80
#define WORLD_MAP_SHADE_MAX 0xFF
#define WORLD_MAP_SHADE_MAX_STEP4 0xFC

// Bit 1 of a primitive code byte: the primitive is drawn semi-transparent
// (same define as misc9.c)
#define WORLD_MAP_PRIM_CODE_SEMI_TRANS 0x2

// the map display buffer copied into VRAM when a mode initialises, and the VRAM
// position it is copied to
#define WORLD_MAP_SCREEN_WIDTH 320
#define WORLD_MAP_SCREEN_HEIGHT 216
#define WORLD_MAP_SCREEN_VRAM_X 0x2C0
#define WORLD_MAP_SCREEN_VRAM_Y 0x100

// the archive directory this overlay's files are read through
#define WORLD_MAP_ARCHIVE_DIRECTORY_INDEX 0x24

// g_SoundControlFlags bit: an SPU transfer command is in flight (owned by
// include/system/sound.h, which declares the flags but not their bits)
#define SOUND_CTL_FLAG_TRANSFER_BUSY (1 << 4)

// the SPU's maximum volume, which SoundPlayAudioManager takes as its volume
#define SPU_VOL_MAX 0x7F

// frames SoundSetAudioManagerVolumeWithFade takes to reach silence
// (same define as misc6.c)
#define WORLD_MAP_SOUND_FADE_OUT_FRAMES 240

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} Vec3; // Size: 0xC

typedef struct {
    /* 0x0 */ s32 w0;
    /* 0x4 */ s32 w1;
    /* 0x8 */ s32 w2;
    /* 0xC */ s32 w3;
} Quad; // Size: 0x10

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 command;
    /* 0x06 */ s16 targetSlot;
    /* 0x08 */ u8 unk8[0x18];
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ s16 hidden;
    /* 0x26 */ u8 pad26[0x2];
    /* 0x28 */ Vec3 pos;
    /* 0x34 */ u8 unk34[0x4];
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ u8 pad64[0x18];
    /* 0x7C */ s32 shakeAmount; // 12-bit fixed: rand() takes its range from it
} WorldMapSlot; // Size: 0x80

typedef struct WorldMapEntry {
    /* 0x00 */ u8 unk0[0x8];
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ u8 unk14[0xC];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ ModelPart *pModel;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ void *pPackets[2]; // prim lists, [0] source and [1] destination; g_WorldMapPrimPacketIndex picks one
    /* 0x50 */ struct WorldMapEntry *pParent;
} WorldMapEntry; // Size: 0x54

// One world map fade quad: a textured quad per fade-table entry, jittered by
// WorldMapUpdateFadeQuads so the wipe does not move in one block. The tpage word sits after the
// second vertex's texcoords, where the original object reads it.
typedef struct {
    /* 0x00 */ u8 tag[3];
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 r;
    /* 0x05 */ u8 g;
    /* 0x06 */ u8 b;
    /* 0x07 */ u8 code;
    /* 0x08 */ s16 x0;
    /* 0x0A */ s16 y0;
    /* 0x0C */ u8 u0;
    /* 0x0D */ u8 v0;
    /* 0x0E */ u16 pad0E;
    /* 0x10 */ s16 x1;
    /* 0x12 */ s16 y1;
    /* 0x14 */ u8 u1;
    /* 0x15 */ u8 v1;
    /* 0x16 */ u16 tpage;
    /* 0x18 */ s16 x2;
    /* 0x1A */ s16 y2;
    /* 0x1C */ u8 u2;
    /* 0x1D */ u8 v2;
    /* 0x1E */ u16 pad1E;
    /* 0x20 */ s16 x3;
    /* 0x22 */ s16 y3;
    /* 0x24 */ u8 u3;
    /* 0x25 */ u8 v3;
    /* 0x26 */ u16 pad26;
} WorldMapFadeQuad; // Size: 0x28

// The world map frame render context g_WorldMapRenderContext points at: partial view of the
// two ordering table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long *pOt[2];
} WorldMapRenderContext; // partial view

extern WorldMapSlot *g_WorldMapSlots;
extern WorldMapEntry *g_WorldMapGroups;
extern WorldMapRenderContext *g_WorldMapRenderContext;
extern DR_MOVE g_WorldMapDrawMovePrims[];
extern void WorldMapInitPolyFt4Prims(WorldMapEntry *pEntry, POLY_FT4 *pPrim, s32 count, s32 abr);
extern u8 D_8009A758[];
extern void WorldMapStartSlot(s32 arg0, s32 arg1);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern void SoundSetAudioManagerVolumeWithFade(s32 *pManager, s32 arg1, s32 arg2);
extern s32 *g_SoundAudioManager;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
// externs used by the parked attempts below
extern Vec3 g_WorldMapPlayerPosition;
extern MATRIX g_WorldMapIdentityMatrix;
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapRotationMatrixValid;
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
extern s16 D_1F8000A0[];
extern void WorldMapInitSpriteGroupSprites(s32, s16 *, s32);
extern u16 g_WorldMapFadeIds[];
extern u16 g_WorldMapFadeFrameCounts[];
extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetY;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern void WorldMapSetupPrimTemplates(void *, s32, s32);
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
extern s32 g_WorldMapCameraHeight;
extern SoundFile *g_MenuSedsFile;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 g_WorldMapExitFlag;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern void HeapFree(void *pMem);
extern void SoundStopAllElements(void);
extern void WorldMapFreeSpritePrimBuffers(void);
extern s32 g_WorldMapPrimPacketIndex;
extern u8 *g_WorldMapFadeQuadBuffers[];
extern u8 *D_8009D15C;
extern u16 *g_WorldMapFadeTable;
extern s16 g_WorldMapPathFrame;
extern u16 D_8009BD42;
extern u16 D_8009BD4A;
extern void func_80096F18(s16 *pOut, s32 *pSrc, s32 nHeight, u16 *pAngles);
extern void SoundRemoveSedsEntry(void *p);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteSets(void);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeSlotPool(void);
extern void WorldMapFreeGridImages(void);
// mode initialisation
typedef s32 (*WorldMapSlotFunc)(s32 index);

extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapMode4EntryTriggered;
extern s32 g_WorldMapWrapPositionsCallback;
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern void ArchiveCdDataSync(s32 arg0);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapLoadClutAndTpageIds(void);
extern void WorldMapQueueFileLoads(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void WorldMapRelocateMapEntryOffsets(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void *g_GameCurLoadedWDS;
extern void *SoundLoadWdsFile(void *pWdsFile, s32 arg1);
extern void *g_WorldMapWdsFileBuffer;
extern void ArchiveSetIndex(s32 directoryIndex, s32 entryIndex);
extern void WorldMapSetGridPosition(Vec3 *pPos);
extern void WorldMapUpdateLoadQueue(void);
extern s32 Vsync(s32 mode);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void SoundAddSedsEntry(void *pSedsFile);
extern s32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern s32 g_WorldMapSoundBankFileIndex;
extern void *g_WorldMapSoundBankBuffer;
extern u8 g_SoundBankLoadBuffer[];
extern void *SoundCreateAudioManagerOnHeap(void *pBankBuffer);
extern void WorldMapAddSlot(WorldMapSlotFunc pInit, WorldMapSlotFunc pUpdate);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapAllocSpritePrimBuffers(void);
extern void WorldMapInitRandomTimers(void);
extern s32 WorldMapModeInitFadeRect(s32 index);
extern s32 func_800925A0(s32 index);
extern s32 WorldMapBeginSlotAnimFrame(s32 index);
extern s32 WorldMapUpdateSlotState(s32 index);
extern s32 func_800813E8(s32 index);
extern s32 func_80081470(s32 index);
extern s32 func_800817A0(s32 index);
extern s32 WorldMapUpdateSlotColorFade(s32 index);
extern s32 WorldMapInitScaledGroupSlot(s32 index);
extern s32 WorldMapUpdateSlotColorFadeFast(s32 index);
extern s32 WorldMapInitFadePrims(s32 index);
extern s32 WorldMapUpdateFadeQuads(s32 index);
extern s32 WorldMapResetSlotColorFade(s32 index);
extern s32 WorldMapFillFadeTable(s32 index);
extern s32 WorldMapEnterNop(s32 index);
extern s32 WorldMapUpdateFrameWithParticles(s32 index);
extern s32 WorldMapSetSlotScriptTable(s32 index);
extern s32 WorldMapRunSlotScript(s32 index);
extern s32 func_800827EC(s32 index);
extern s32 func_800828DC(s32 index);
extern s32 WorldMapSetupSpriteSetPrims(s32 index);
extern s32 WorldMapUpdateSpriteSetPrims(s32 index);
extern s32 WorldMapReturnSuccess(s32 index);
extern s32 WorldMapWaitForQueueDrain(s32 index);
extern s32 WorldMapInitNop(s32 index);
extern s32 WorldMapUpdateFrame(s32 index);

// Preset screen: every body parked below was also compiled with this unit's
// maspsx flags under gcc-2.7.2-cdk and gcc-2.6.0, and none reaches 100% (best
// alternative 95.26, equal to its current score), so the parks are C problems
// and not this unit's compiler. Per-function alternative scores are in the
// notes beside each park.
s32 WorldMapInitSlotAtScrollPosition(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = g_WorldMapPlayerPosition.y;
    pSlot->pos.z = g_WorldMapPlayerPosition.z;
    return 1;
}
// Best: 84.76%. The branch, every immediate, every relocation and the three
// WorldMapInitSpriteGroupSprites calls match once the three scratchpad stores use the literal
// address 0x1F8000A0: retail writes them as `lui $at,0x1F80` + `sh $v0,0xA0($at)`
// with no relocation, while naming D_1F8000A0 emits %hi/%lo and forces the base
// into a saved register (that shape scores 76.71%). A 36-row try_flags sweep
// (gcc 2.6.0/2.7.2/cdk x -O1..-O3 x -G0/-G4/-G8 x mcpu 3000/r8000) tops out at
// 84.76, so the preset is not the problem. Residues: gcc sinks `sh zero,0x4(base)`
// below the pos.x/pos.z loads and hoists the pos.z load where retail keeps
// statement order, and the slot base lands in $a3 instead of $a2. Addressing the
// slot as g_WorldMapSlots[index] per statement scores 81.95; moving the command store to
// either end of the three stays at 84.76.
/*
s32 func_80080944(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 v;

    if (pSlot->command != 1) {
        return 1;
    }
    pSlot->command = 0;
    v = pSlot->pos.x >> 12;
    *(s16 *)0x1F8000A0 = v;
    v = pSlot->pos.y >> 12;
    *(s16 *)0x1F8000A2 = v;
    v = pSlot->pos.z >> 12;
    *(s16 *)0x1F8000A4 = v;
    WorldMapInitSpriteGroupSprites(0x28, (s16 *)0x1F8000A0, 0);
    WorldMapInitSpriteGroupSprites(0x29, (s16 *)0x1F8000A0, 0);
    WorldMapInitSpriteGroupSprites(0x2A, (s16 *)0x1F8000A0, 0);
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080944);

void WorldMapSetPrimColors(POLY_FT4 *pPrim, s32 count, s32 a, s32 b, s32 c) {
    s32 i;

    for (i = 0; i < count; i++, pPrim++) {
        pPrim->r0 = a;
        pPrim->g0 = b;
        pPrim->b0 = c;
    }
}

// Best: 94.87%, 156 bytes = retail's exact size; the group entries are typed
// (pModel->polyCount, pPackets[0]). The single residue is the placement of the first
// call's argument copy: retail has `move a0,s0` between the pos.y lui and its store
// (i.e. before the Z load), this gcc puts it after the Z load and `li v1,-2048`. The
// empty do/while after the pos.y store is load-bearing (retail keeps the pos.y store
// above the Z load; without it 92.64). Declaring a pEntry variable and assigning it at
// four different source positions, and passing pGroup directly (no pEntry at all),
// all measure the identical body - the copy comes from local_alloc, not from a source
// statement. A 4-minute permuter run (1887 iterations) found nothing. Preset screen:
// cdk 87.51, gcc-2.6.0 87.15.
// Round 377 re-measured after unpark: 94.87, compiles, 156 bytes. Five more cc1_probe
// shapes (each compiled alone with this unit's cc1) give the same body with the copy
// after the li -2048: a pArg local assigned between the pos.x and pos.y stores, one
// assigned after the pos.y store, 0xFFF80000 in a local first, and the unk50/unk54
// statements swapped (gcc reorders those back). Wrapping the three trailing stores in a
// do/while moves the copy to the call site but also hands z $a0, which is what blocks
// it there. So the 4-instruction hoist of the first call's argument copy is allocator
// placement, not statement order.
// Best: 94.87%, 156 bytes = retail's exact size, compiles. ONE residue: the first call's
// a0 copy. Retail has `move a0,s0` between the pos.y lui and its sw; this cc1 emits it
// between the unk54 `li v1,-2048` and its sw (4 slots later). cc1's own .s and RTL put the
// copy at that point, so it is the placement of that copy, not a post-pass reorder.
// Round 377 re-measured after unpark: 94.87, compiles, 156 bytes.
/*
s32 func_80080A28(s32 index) {
    WorldMapEntry *pGroup = g_WorldMapGroups;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 z;

    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = 0xFFF80000;
    do { } while (0); // keeps the pos.y store ahead of the Z load
    z = g_WorldMapPlayerPositionZ;
    pSlot->unk54 = -0x800;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0x80;
    pSlot->pos.z = z;
    WorldMapSetupPrimTemplates(pGroup, pGroup->pPackets[0], pGroup->pModel->polyCount);
    pGroup = pGroup + 1;
    WorldMapSetupPrimTemplates(pGroup, pGroup->pPackets[0], pGroup->pModel->polyCount);
    return 3;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080A28);
// The two GTE scale vectors the fade groups are scaled by, in the scratchpad. The
// address is a literal on purpose: naming a symbol here makes gcc materialise %hi/%lo
// through a saved register where the original stores through a bare lui $at.
typedef struct {
    /* 0x00 */ VECTOR first;
    /* 0x10 */ VECTOR second;
} WorldMapScaleVectors; // Size: 0x20

#define WORLD_MAP_SCALE_VECTORS ((WorldMapScaleVectors *)0x1F800000)

s32 WorldMapUpdateFadeGroups(s32 index) {
    WorldMapEntry *pGroup = g_WorldMapGroups;
    WorldMapEntry *pNextGroup;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 spread;
    s32 pos;

    if (pSlot->command == 1) {
        pSlot->command = 0;
    }
    pos = pSlot->pos.x >> 12;
    pGroup[1].x = pos;
    pGroup->x = pos;
    pos = pSlot->pos.y >> 12;
    pGroup[1].y = pos;
    pGroup->y = pos;
    pos = pSlot->pos.z >> 12;
    pGroup[1].z = pos;
    pGroup->z = pos;
    pGroup[1].matrix = g_WorldMapIdentityMatrix;
    pGroup->matrix = pGroup[1].matrix;
    WORLD_MAP_SCALE_VECTORS->first.vx = WORLD_MAP_SCALE_VECTORS->first.vz = pSlot->unk50;
    spread = pSlot->unk54;
    WORLD_MAP_SCALE_VECTORS->first.vy = WORLD_MAP_SCALE_VECTORS->second.vy = 0x1000;
    WORLD_MAP_SCALE_VECTORS->second.vx = WORLD_MAP_SCALE_VECTORS->second.vz = spread;
    ScaleMatrix(&pGroup->matrix, &WORLD_MAP_SCALE_VECTORS->first);
    ScaleMatrix(&pGroup[1].matrix, &WORLD_MAP_SCALE_VECTORS->second);
    pNextGroup = pGroup + 1;
    pSlot->unk50 += 0x180;
    if (pSlot->unk50 > 0x7FFF) {
        pSlot->unk50 = 0x7FFF;
    }
    pSlot->unk54 += 0x180;
    if (pSlot->unk54 > 0x7FFF) {
        pSlot->unk54 = 0x7FFF;
    }
    WorldMapSetPrimColors(pGroup->pPackets[g_WorldMapPrimPacketIndex], pGroup->pModel->polyCount,
                          pSlot->unk58, pSlot->unk58, pSlot->unk58);
    pGroup = pNextGroup;
    WorldMapSetPrimColors(pGroup->pPackets[g_WorldMapPrimPacketIndex], pGroup->pModel->polyCount,
                          pSlot->unk58, pSlot->unk58, pSlot->unk58);
    pSlot->unk58 -= 4;
    if (pSlot->unk58 < 0) {
        pSlot->unk58 = 0;
        return 3;
    }
    return 1;
}
// The world map mode whose slots fade the screen in through the fade table
// (WorldMapFillFadeTable, WorldMapUpdateSlotColorFade), initialised the same way as
// the other modes: load the map, its textures and its sound bank, then register each
// slot's init/update pair. Proposed name: WorldMapInitializeFadeMode.
void WorldMapInitializeFadeMode(void) {
    RECT rect;
    Vec3 *pPos;

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
    g_WorldMapWrapPositionsCallback = (s32)WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    SoundFreeLoadedWdsAndAudioManager();
    pPos = &g_WorldMapPlayerPosition;
    pPos->x = 0x4000000;
    g_WorldMapPlayerPositionY = 0xFFF40000;
    g_WorldMapPlayerPositionZ = 0x4000000;
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
    memcpy(g_SoundBankLoadBuffer, g_WorldMapSoundBankBuffer,
           ArchiveDecodeAlignedSize(g_WorldMapSoundBankFileIndex));
    g_SoundAudioManager = SoundCreateAudioManagerOnHeap(g_SoundBankLoadBuffer);
    SoundPlayAudioManager(g_SoundAudioManager, SPU_VOL_MAX, 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapBeginSlotAnimFrame, WorldMapUpdateSlotState);
    WorldMapAddSlot(func_800813E8, func_80081470);
    WorldMapAddSlot(func_800817A0, WorldMapUpdateSlotColorFade);
    WorldMapAddSlot(WorldMapInitScaledGroupSlot, WorldMapUpdateSlotColorFadeFast);
    WorldMapAddSlot(WorldMapInitFadePrims, WorldMapUpdateFadeQuads);
    WorldMapAddSlot(WorldMapResetSlotColorFade, WorldMapFillFadeTable);
    WorldMapAddSlot(WorldMapEnterNop, WorldMapUpdateFrameWithParticles);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapInitRandomTimers();
}

void WorldMapExitToMap1FA(void) {
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
    g_NextMapId = 0x1FA;
    g_NextMapFlags = 0;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
}
// The empty do/while is load-bearing: without it gcc hoists the g_WorldMapFadeIds load
// above the slot address formation (65.26%); with it the schedule is retail's.
s32 WorldMapBeginSlotAnimFrame(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    do { } while (0);
    pSlot->unk50 = 0;
    pSlot->state = g_WorldMapFadeIds[0];
    pSlot->stateTimer = g_WorldMapFadeFrameCounts[pSlot->unk50];
    pSlot->unk50 = pSlot->unk50 + 1;
    return 1;
}
s32 WorldMapUpdateSlotState(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    switch (pSlot->state) {
    case 1:
        pSlot->stateTimer = pSlot->stateTimer - 1;
        if ((s16)pSlot->stateTimer < 0) {
            pSlot->state = g_WorldMapFadeIds[pSlot->unk50];
            pSlot->stateTimer = g_WorldMapFadeFrameCounts[pSlot->unk50];
            pSlot->unk50 = pSlot->unk50 + 1;
        }
        break;
    case 2:
        WorldMapStartSlot(6, 1);
        pSlot->state = 1;
        break;
    case 3:
        WorldMapStartSlot(6, 0);
        pSlot->state = 1;
        break;
    case 4:
        WorldMapStartSlot(6, 2);
        pSlot->state = 1;
        break;
    case 5:
        WorldMapStartSlot(6, 3);
        pSlot->state = 1;
        break;
    case 6:
        WorldMapStartSlot(6, 4);
        pSlot->state = 1;
        break;
    case 7:
        WorldMapStartSlot(6, 5);
        pSlot->state = 1;
        break;
    case 8:
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x2E);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x2F);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x30);
        pSlot->state = 1;
        break;
    case 0x10:
        WorldMapStartSlot(3, 1);
        WorldMapStartSlot(4, 1);
        pSlot->state = 1;
        break;
    case 0x11:
        WorldMapStartSlot(3, 2);
        pSlot->state = 1;
        break;
    case 0x3F:
        SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0,
                                           WORLD_MAP_SOUND_FADE_OUT_FRAMES);
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 2;
        g_WorldMapDrawMode = 4;
        pSlot->state = 1;
        break;
    case 0x40:
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
        pSlot->state = 0;
        break;
    case 0:
    default:
        break;
    }
    return 1;
}
// Best: 94.71%, 136 bytes = retail's exact size (round 398). Ported from the misc6 twin
// func_80080578's park: the command store must be the LAST statement INSIDE the do-block,
// which keeps the li v0,1 six instructions above its sh v0,4 and the six target stores in
// source order, and the posZ alias load fixes the x/y/z load order. Two deltas left, both
// the allocator tie proved with a -da dump on that twin in round 397: (1) the register
// rotation - retail keeps pSlot in v1 with z in a2, this body takes a2 for pSlot and v1
// for z (pSlot is TIED to the lw g_WorldMapSlots result by the addu, so find_free_reg
// picks the register); (2) the reorg tail - retail emits sh v0,4 / li v0,1 / jr ra /
// sh zero,0x20, this body sh v0,4 / sh zero,0x20 / jr ra / li v0,1. Two permuter runs on
// the twin (identical body) found nothing. Preset screen: cdk 34.09 on the plain body.
/*
s32 func_800813E8(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 posX;
    s32 posY;
    s32 posZ;

    g_WorldMapRotationMatrixValid = 0;
    pSlot->shakeAmount = 0x1000;
    do {
        posZ = g_WorldMapPlayerPosition.x;
        posX = posZ;
        posY = g_WorldMapPlayerPositionY;
        posZ = g_WorldMapPlayerPositionZ;
        g_WorldMapFadeLevel = 0x78;
        g_WorldMapCameraTarget = posX;
        g_WorldMapScrollTargetX = posX;
        g_WorldMapCameraTargetY = posY;
        g_WorldMapScrollTargetY = posY;
        g_WorldMapCameraTargetZ = posZ;
        g_WorldMapScrollTargetZ = posZ;
        pSlot->command = 1;
    } while (0);
    pSlot->state = 0;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800813E8);
// Best: 78.69%, 580 bytes against retail's 636. The clamp/easing chain is byte-identical:
// unk50 -0x10000 down to 0x630000 with g_WorldMapCameraHeight eased by diff >> 5, unk54
// += 0x1000 clamped to 0x10000 and eased into unk58 by diff >> 4 driving
// g_WorldMapCameraRot, unk5C += 0x10000 clamped to 0x4B0000 and eased into unk60 the same
// way, plus the func_80096F18 call and the rand()/shakeAmount jitter. Every residue is one
// class: THIS gcc forwards a constant store to a later load where retail reloads.
// 1. retail stores -0x80 to g_WorldMapCameraRot and -0x200 to g_WorldMapViewRotationY and
//    then READS BOTH BACK to build unk58/unk54 and unk60/unk5C (retail's cameraRot address
//    is CSE'd into $a0, ViewRotationY's is rematerialised, so the original kept a pointer
//    to the rot); here gcc folds both to 0xFFF80000/0xFFE00000. A `s16 *pRot` local did not
//    stop it, and a volatile read reloads but changes the extension (lhu + sll 16/sra 4
//    instead of lh + sll 12).
// 2. the same in the tail: retail reloads the scratchpad halfword for `D_8009BD4A += ...`
//    (lhu 0xA2(s1) after the sh) where this gcc reuses the register.
// 3. the command switch: retail has the two-case decision tree (beq 2 / slti 3 / bne 1)
//    with an empty case 2; this gcc merges a bodyless case into the default and emits one
//    bne. A narrow s16 switch index did not bring the tree back.
// Preset screen not run; the body's structure is settled.
/*
s32 func_80081470(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 diff;
    s16 command;
    s16 shake;

    command = pSlot->command;
    switch (command) {
    case 1:
        pSlot->state = 1;
        pSlot->command = 0;
        g_WorldMapCameraRot = -0x80;
        g_WorldMapViewRotationY = -0x200;
        g_WorldMapViewRotationZ = 0;
        pSlot->unk50 = 0x980000;
        pSlot->unk58 = g_WorldMapCameraRot << 12;
        pSlot->unk54 = g_WorldMapCameraRot << 12;
        g_WorldMapCameraHeight = 0x980000;
        pSlot->unk60 = g_WorldMapViewRotationY << 12;
        pSlot->unk5C = g_WorldMapViewRotationY << 12;
        break;
    case 2:
        break;
    default:
        break;
    }
    if (g_WorldMapRotationMatrixValid == 0) {
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      &g_WorldMapCameraRot);
    }
    if (pSlot->state == 1) {
        pSlot->unk50 -= 0x10000;
        if (pSlot->unk50 < 0x630000) {
            pSlot->unk50 = 0x630000;
        }
        diff = pSlot->unk50 - g_WorldMapCameraHeight;
        if (diff != 0) {
            g_WorldMapCameraHeight += diff >> 5;
        }
        pSlot->unk54 += 0x1000;
        if (pSlot->unk54 > 0x10000) {
            pSlot->unk54 = 0x10000;
        }
        diff = pSlot->unk54 - pSlot->unk58;
        if (diff != 0) {
            pSlot->unk58 += diff >> 4;
            g_WorldMapCameraRot = pSlot->unk58 >> 12;
        }
        pSlot->unk5C += 0x10000;
        if (pSlot->unk5C > 0x4B0000) {
            pSlot->unk5C = 0x4B0000;
        }
        diff = pSlot->unk5C - pSlot->unk60;
        if (diff != 0) {
            pSlot->unk60 += diff >> 5;
            g_WorldMapViewRotationY = pSlot->unk60 >> 12;
        }
    }
    shake = rand() % (pSlot->shakeAmount >> 12) - (pSlot->shakeAmount >> 13);
    *(s16 *)0x1F8000A2 = shake;
    D_8009BD42 = D_8009BD42 + shake;
    D_8009BD4A = D_8009BD4A + *(s16 *)0x1F8000A2;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081470);
void WorldMapInitPolyFt4Prims(WorldMapEntry *pEntry, POLY_FT4 *pPrim, s32 count, s32 abr) {
    s32 i;

    for (i = 0; i < count; i++, pPrim++) {
        setPolyFT4(pPrim);
        pPrim->tpage = GetTPage(0, abr, 0x180, 0);
        setSemiTrans(pPrim, 1);
    }
    memcpy(pEntry->pPackets[1], pEntry->pPackets[0], count * sizeof(POLY_FT4));
}

// Best: 89.56%, 200 bytes = retail's exact size (round 398). Two finds off the old
// 79.06 body: declaring pEntry WITHOUT an initialiser and assigning it between the pos.y
// and pos.z lines puts the lw g_WorldMapGroups in retail's exact slot (after the y load,
// before the addu), and the do/while after the state store is still required.
// One residue left: the scheduler hoists the posZ load above the pos.x store, which drags
// the y addu + pos.y store below the three zero stores (retail has y add and store
// adjacent, z load then zeros then z store). Measured worse or equal: a do/while barrier
// before the pos.z line (86.48), an s32 posZ local loaded between pEntry and the zeros
// (89.56, unchanged). Preset screen: cdk 75.76, gcc-2.6.0 compile-failed.
/*
s32 func_800817A0(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    WorldMapEntry *pEntry;

    pSlot->state = 0;
    do { } while (0); // keeps the position loads below the state store
    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = g_WorldMapPlayerPositionY + 0xFFF00000;
    pEntry = &g_WorldMapGroups[2];
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    pSlot->pos.z = g_WorldMapPlayerPositionZ;
    WorldMapInitPolyFt4Prims(pEntry, pEntry->pPackets[0], pEntry->pModel->polyCount, 1);
    pEntry->x = pSlot->pos.x >> 12;
    pEntry->y = pSlot->pos.y >> 12;
    pEntry->z = pSlot->pos.z >> 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800817A0);
s32 WorldMapUpdateSlotColorFade(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    WorldMapEntry *pEntry = &g_WorldMapGroups[2];
    POLY_FT4 *pPrim;
    s32 i;

    switch (pSlot->command) {
    case 1:
        pSlot->command = 0;
        pSlot->state = 1;
        break;
    case 2:
        pSlot->command = 0;
        pSlot->state = 0;
        pPrim = pEntry->pPackets[g_WorldMapPrimPacketIndex];
        for (i = 0; i < pEntry->pModel->polyCount; i++, pPrim++) {
            pPrim->code &= ~WORLD_MAP_PRIM_CODE_SEMI_TRANS;
            pPrim->r0 = WORLD_MAP_SHADE_NEUTRAL;
            pPrim->g0 = WORLD_MAP_SHADE_NEUTRAL;
            pPrim->b0 = WORLD_MAP_SHADE_NEUTRAL;
        }
        break;
    }
    if (pSlot->state != 0) {
        if (pSlot->state == 1) {
            pSlot->unk50 += 1;
            pSlot->unk54 += 1;
            pSlot->unk58 += 1;
            if (pSlot->unk50 >= WORLD_MAP_SHADE_MAX) {
                pSlot->unk58 = WORLD_MAP_SHADE_MAX;
                pSlot->unk54 = WORLD_MAP_SHADE_MAX;
                pSlot->unk50 = WORLD_MAP_SHADE_MAX;
                pSlot->state = 0;
            }
        }
    }
    WorldMapSetPrimColors(pEntry->pPackets[g_WorldMapPrimPacketIndex],
                  pEntry->pModel->polyCount,
                  pSlot->unk50, pSlot->unk54, pSlot->unk58);
    return 1;
}
// Best: 87.03%. Every call, offset and relocation is right (the 8-word matrix
// copy from g_WorldMapIdentityMatrix, the three scratchpad words, ScaleMatrix with the
// 0x1F800000 scale vector, the matrix copy to +0x74, both WorldMapInitPolyFt4Prims
// calls). The only delta is the block of pSlot stores: gcc hoists the
// g_WorldMapPlayerPositionY/g_WorldMapPlayerPositionZ loads and the lui 0x1F80 to the top of the block and
// sinks the g_WorldMapPlayerPosition store below the three zero stores, while retail keeps
// statement order and reuses $v1 for all three loads. Same blocker as
// func_800817A0 above; pointer local, zero-store order and x/y/z order were
// all no better (vec3 through a pointer local: 87.03, zeros first: 84.43).
s32 WorldMapInitScaledGroupSlot(s32 index) {
    u8 *pEntry = (u8 *)g_WorldMapGroups;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->state = 0;
    *(volatile s32 *)&pSlot->pos.x = g_WorldMapPlayerPosition.x;
    *(volatile s32 *)&pSlot->pos.y = g_WorldMapPlayerPositionY;
    pSlot->pos.z = g_WorldMapPlayerPositionZ;
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    *(MATRIX *)(pEntry + 0x20) = g_WorldMapIdentityMatrix;
    *(s32 *)0x1F800008 = 0x1000;
    *(s32 *)0x1F800000 = 0x1000;
    *(s32 *)0x1F800004 = 0x7000;
    ScaleMatrix((MATRIX *)(pEntry + 0x20), (VECTOR *)0x1F800000);
    *(MATRIX *)(pEntry + 0x74) = *(MATRIX *)(pEntry + 0x20);
    WorldMapInitPolyFt4Prims((WorldMapEntry *)pEntry, *(u8 **)(pEntry + 0x48),
                  *(u16 *)(*(u8 **)(pEntry + 0x40) + 4), 3);
    pEntry += 0x54;
    WorldMapInitPolyFt4Prims((WorldMapEntry *)pEntry, *(u8 **)(pEntry + 0x48),
                  *(u16 *)(*(u8 **)(pEntry + 0x40) + 4), 3);
    return 1;
}s32 WorldMapUpdateSlotColorFadeFast(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    WorldMapEntry *pEntry = &g_WorldMapGroups[0];

    if (pSlot->command == 1) {
        pSlot->command = 0;
        pSlot->state = 1;
    }
    if (pSlot->state != 0) {
        if (pSlot->state == 1) {
            pSlot->unk50 += 4;
            pSlot->unk54 += 2;
            pSlot->unk58 += 1;
            if (pSlot->unk50 >= WORLD_MAP_SHADE_MAX_STEP4) {
                pSlot->unk50 = WORLD_MAP_SHADE_MAX_STEP4;
                pSlot->state = 0;
            }
        }
    }
    WorldMapSetPrimColors(pEntry->pPackets[g_WorldMapPrimPacketIndex],
                  pEntry->pModel->polyCount,
                  pSlot->unk50, pSlot->unk54, pSlot->unk58);
    pEntry++;
    WorldMapSetPrimColors(pEntry->pPackets[g_WorldMapPrimPacketIndex],
                  pEntry->pModel->polyCount,
                  pSlot->unk50, pSlot->unk54, pSlot->unk58);
    return 1;
}
s32 WorldMapInitFadePrims(s32 index) {
    u8 **ppQuadBuffers = g_WorldMapFadeQuadBuffers;
    Quad *pSrc;
    Quad *pDst;
    Quad *pEnd;
    u16 *pFill;
    u16 value;
    u8 level;
    u8 code;
    u8 shade;
    WorldMapFadeQuad *pQuad;
    s32 i;

    ppQuadBuffers[0] = HeapAlloc(WORLD_MAP_FADE_ENTRY_COUNT * sizeof(WorldMapFadeQuad), 0);
    D_8009D15C = HeapAlloc(WORLD_MAP_FADE_ENTRY_COUNT * sizeof(WorldMapFadeQuad), 0);
    g_WorldMapFadeTable = HeapAlloc(WORLD_MAP_FADE_ENTRY_COUNT * sizeof(u16), 0);
    i = 0;
    level = 9;
    code = 0x2C;
    pQuad = (WorldMapFadeQuad *)ppQuadBuffers[0];
    shade = WORLD_MAP_SHADE_NEUTRAL;
    for (; i < WORLD_MAP_FADE_ENTRY_COUNT; i++) {
        pQuad->code = code;
        pQuad->unk3 = level;
        pQuad->r = shade;
        pQuad->g = shade;
        pQuad->b = shade;
        pQuad->code |= 1;
        pQuad->tpage = GetTPage(2, 0, 0x280, 0x100);
        pQuad++;
    }
    pSrc = (Quad *)g_WorldMapFadeQuadBuffers[0];
    pDst = (Quad *)D_8009D15C;
    pEnd = pSrc + 0x1E0;
    do {
        *pDst++ = *pSrc++;
    } while (pSrc != pEnd);
    pFill = g_WorldMapFadeTable;
    // u16 local, not a literal: retail materialises the 1 once before the loop
    value = 1;
    for (i = WORLD_MAP_FADE_ENTRY_COUNT - 1; i >= 0; i--) {
        *pFill++ = value;
    }
    return 1;
}
s32 WorldMapUpdateFadeQuads(s32 index) {
    WorldMapFadeQuad *pQuad = (WorldMapFadeQuad *)g_WorldMapFadeQuadBuffers[g_WorldMapPrimPacketIndex];
    u16 *pFade = g_WorldMapFadeTable;
    DR_MOVE *pMove;
    s32 delta;
    s32 i;
    RECT rect;

    for (i = 0; i < WORLD_MAP_FADE_ENTRY_COUNT; i++) {
        delta = rand() % *pFade - *pFade / 2;
        pFade++;
        pQuad->y0 = i;
        pQuad->y1 = i;
        pQuad->v0 = i;
        pQuad->v1 = i;
        pQuad->u0 = 0;
        pQuad->u1 = 0xC0;
        pQuad->u2 = 0;
        pQuad->u3 = 0xC0;
        pQuad->x0 = delta + 0x40;
        pQuad->x1 = delta + 0x100;
        pQuad->x2 = delta + 0x40;
        pQuad->x3 = delta + 0x100;
        pQuad->y2 = i + 1;
        pQuad->y3 = i + 1;
        pQuad->v2 = i + 1;
        pQuad->v3 = i + 1;
        addPrim(g_WorldMapRenderContext->pOt[0], pQuad);
        pQuad++;
    }
    rect.x = 0x40;
    rect.w = 0xC0;
    rect.h = 0xD8;
    rect.y = g_WorldMapPrimPacketIndex * 0xD8;
    SetDrawMove(&g_WorldMapDrawMovePrims[g_WorldMapPrimPacketIndex], &rect, 0x280, 0x100);
    pMove = &g_WorldMapDrawMovePrims[g_WorldMapPrimPacketIndex];
    addPrim(g_WorldMapRenderContext->pOt[0], pMove);
    return 1;
}
s32 WorldMapResetSlotColorFade(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->state = 0;
    pSlot->unk50 = 1;
    return 1;
}

s32 WorldMapFillFadeTable(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 i;
    u16 *pFill;
    s32 start;
    s16 level;
    s16 request;

    request = pSlot->command - 1;
    switch (request) {
    case 0:
        pSlot->command = 0;
        pSlot->state = 1;
        break;
    case 1:
        pSlot->command = 0;
        pSlot->state = 2;
        break;
    case 2:
        pSlot->command = 0;
        pSlot->state = 3;
        break;
    case 3:
        pSlot->command = 0;
        pSlot->state = 4;
        break;
    case 4:
        pSlot->command = 0;
        pSlot->state = 5;
        break;
    }
    switch (pSlot->state) {
    case 0:
        break;
    case 1:
        pFill = g_WorldMapFadeTable;
        for (i = WORLD_MAP_FADE_ENTRY_COUNT - 1; i >= 0; i--) {
            *pFill++ = pSlot->unk50;
        }
        start = rand() & 0x3F;
        pFill = g_WorldMapFadeTable + start;
        i = start;
        while (i < start + (rand() & 0x1F) + 1) {
            if ((rand() & 3) == 0) {
                *pFill = (rand() & 0x3F) + 1;
            }
            i++;
            pFill++;
        }
        start = (rand() & 0x3F) + 0x40;
        pFill = g_WorldMapFadeTable + start;
        i = start;
        while (i < start + (rand() & 0x1F) + 1) {
            if ((rand() & 3) == 0) {
                *pFill = (rand() & 0x3F) + 1;
            }
            i++;
            pFill++;
        }
        start = (rand() & 0x1F) + 0x80;
        pFill = g_WorldMapFadeTable + start;
        i = start;
        while (i < start + (rand() & 0x1F) + 1) {
            if ((rand() & 3) == 0) {
                *pFill = (rand() & 0x3F) + 1;
            }
            i++;
            pFill++;
        }
        break;
    case 2:
        pFill = g_WorldMapFadeTable;
        pSlot->unk50 += 1;
        if (pSlot->unk50 > WORLD_MAP_FADE_LEVEL_MAX) {
            pSlot->unk50 = WORLD_MAP_FADE_LEVEL_MAX;
        }
        for (i = WORLD_MAP_FADE_ENTRY_COUNT - 1; i >= 0; i--) {
            *pFill++ = pSlot->unk50;
        }
        break;
    case 3:
        pFill = g_WorldMapFadeTable;
        pSlot->unk50 -= 1;
        if (pSlot->unk50 < WORLD_MAP_FADE_LEVEL_MIN) {
            pSlot->unk50 = WORLD_MAP_FADE_LEVEL_MIN;
        }
        for (i = WORLD_MAP_FADE_ENTRY_COUNT - 1; i >= 0; i--) {
            *pFill++ = pSlot->unk50;
        }
        break;
    case 4:
        pFill = g_WorldMapFadeTable;
        for (i = 0; i < WORLD_MAP_FADE_ENTRY_COUNT; i++) {
            if ((rand() & 3) == 0) {
                *pFill = (rand() & 0x3F) + 1;
            }
            pFill++;
        }
        break;
    case 5:
        pFill = g_WorldMapFadeTable;
        // s16 local, not a literal: retail materialises the 2 once before the loop
        level = WORLD_MAP_FADE_LEVEL_MIN;
        for (i = WORLD_MAP_FADE_ENTRY_COUNT - 1; i >= 0; i--) {
            *pFill++ = level;
        }
        break;
    }
    return 1;
}
// The second fade mode: the same initialisation as WorldMapInitializeFadeMode above, but the slot
// table is driven by a script and five sprite sets of its own.
void WorldMapInitializeScriptMode(void) {
    RECT rect;
    Vec3 *pPos;
    WorldMapSlotFunc pSpriteSetInit;
    WorldMapSlotFunc pSpriteSetUpdate;

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
    g_WorldMapWrapPositionsCallback = (s32)WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    SoundFreeLoadedWdsAndAudioManager();
    pPos = &g_WorldMapPlayerPosition;
    pPos->x = 0x70A9000;
    g_WorldMapPlayerPositionY = 0xFFEB8000;
    g_WorldMapPlayerPositionZ = 0x42AA000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    WorldMapLoadClutAndTpageIds();
    WorldMapQueueFileLoads();
    func_800736DC();
    WorldMapRelocateMapEntryOffsets();
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
    memcpy(g_SoundBankLoadBuffer, g_WorldMapSoundBankBuffer,
           ArchiveDecodeAlignedSize(g_WorldMapSoundBankFileIndex));
    g_SoundAudioManager = SoundCreateAudioManagerOnHeap(g_SoundBankLoadBuffer);
    SoundPlayAudioManager(g_SoundAudioManager, SPU_VOL_MAX, 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapSetSlotScriptTable, WorldMapRunSlotScript);
    WorldMapAddSlot(func_800827EC, func_800828DC);
    pSpriteSetInit = WorldMapSetupSpriteSetPrims;
    pSpriteSetUpdate = WorldMapUpdateSpriteSetPrims;
    WorldMapAddSlot(pSpriteSetInit, pSpriteSetUpdate);
    WorldMapAddSlot(pSpriteSetInit, pSpriteSetUpdate);
    WorldMapAddSlot(pSpriteSetInit, pSpriteSetUpdate);
    WorldMapAddSlot(pSpriteSetInit, pSpriteSetUpdate);
    WorldMapAddSlot(pSpriteSetInit, pSpriteSetUpdate);
    WorldMapAddSlot(WorldMapReturnSuccess, WorldMapWaitForQueueDrain);
    WorldMapAddSlot(WorldMapInitNop, WorldMapUpdateFrame);
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapAllocSpritePrimBuffers();
    WorldMapInitRandomTimers();
}

s32 WorldMapExitToMap269(void) {
    u16 v;

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
    v = *(u16 *)&g_WorldMapViewRotationY;
    g_NextMapId = 0x269;
    g_NextMapFlags = 2;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
    return 1;
}

// The live body directly below this block matches 100% (index-then-symbol
// order): the pointer local is what misallocates here.
// Parked alternative: 51.67%. Instruction multiset is identical; the original computes the
// slot pointer (sll/addu) before forming the address of D_8009A758, gcc forms
// the address first. 46.11% for the slot-pointer-first order; declaring pData
// before pSlot puts the symbol address first and reaches 51.67%. Also tried:
// two separate statements, a s32 value temp, `g_WorldMapSlots + index`, and an
// explicit offset local.
/*
s32 WorldMapSetSlotScriptTable(s32 index) {
    u8 *pData = D_8009A758;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->unk50 = (s32)pData;
    return 1;
}
*/
s32 WorldMapSetSlotScriptTable(s32 index) {
    g_WorldMapSlots[index].unk50 = (u32)D_8009A758;
    return 1;
}
// Best: 65.30%, 240 bytes against retail's 200 (objdiff prints 46.63 without the
// two do/while barriers, 19.28 with plain per-statement reads). Retail's emission order
// is reproduced exactly for the three position triples (one reused s32 local, as retail
// reuses $v0) and the volatile zero stores are what keep retail's `lh` reloads, but gcc
// permutes the constant stores of the tail and fills the three load-delay slots with
// nops where retail fills them from the tail. Preset screen: cdk 36.73, gcc-2.6.0 40.43.
/*
s32 func_800827EC(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 v;

    pSlot->shakeAmount = 0x1000;
    do { } while (0); // keeps the position stores above the constant stores
    v = g_WorldMapPlayerPosition.x;
    g_WorldMapCameraTarget = v;
    g_WorldMapScrollTargetX = v;
    pSlot->pos.x = v;
    v = g_WorldMapPlayerPositionY;
    g_WorldMapCameraTargetY = v;
    g_WorldMapScrollTargetY = v;
    pSlot->pos.y = v;
    v = g_WorldMapPlayerPositionZ;
    g_WorldMapCameraTargetZ = v;
    g_WorldMapScrollTargetZ = v;
    pSlot->pos.z = v;
    do { } while (0);
    pSlot->state = 0;
    pSlot->command = 0;
    pSlot->unk5C = 0x500000;
    g_WorldMapCameraRot = -0x220;
    *(volatile s16 *)&g_WorldMapViewRotationY = 0;
    *(volatile s16 *)&g_WorldMapViewRotationZ = 0;
    pSlot->unk50 = 0xFFDE0000;
    pSlot->unk38 = 0xFFDE0000;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapCameraHeight = 0x500000;
    pSlot->unk54 = g_WorldMapViewRotationY << 12;
    pSlot->unk3C = g_WorldMapViewRotationY << 12;
    g_WorldMapFadeLevel = 0x78;
    pSlot->unk58 = g_WorldMapViewRotationZ << 12;
    pSlot->unk40 = g_WorldMapViewRotationZ << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827EC);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800828DC);
