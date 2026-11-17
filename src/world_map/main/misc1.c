#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/archive.h"
#include "system/memory.h"

#define WORLD_MAP_TILE_GROUP_COUNT 16

// the grid's rotation advances by this much every frame
#define WORLD_MAP_GRID_ANGLE_STEP 64

// the GTE projection distance the map is drawn and fogged through
#define WORLD_MAP_PROJECTION_DISTANCE 256

// the two buffers every map draw/disp env covers: one screen each, one frame apart
#define WORLD_MAP_SCREEN_WIDTH 320
#define WORLD_MAP_SCREEN_HEIGHT 216

// the four quads the frame projects out of g_WorldMapQuadVertices, and one POLY_FT4 per quad
#define WORLD_MAP_QUAD_COUNT 4

// the slot pool WorldMapAllocPrimBuffers holds, one WorldMapSlot and one POLY_FT4 each
#define WORLD_MAP_SLOT_COUNT 16

// the two per-frame map work buffers
#define WORLD_MAP_WORK_BUFFER_SIZE 4096

// archive entries loaded before the map data itself exists
#define WORLD_MAP_SOUND_ARCHIVE_INDEX 0x24
#define WORLD_MAP_COMPRESSED_MAP_ARCHIVE_INDEX 0x25
#define WORLD_MAP_MENU_RESOURCES_ARCHIVE_INDEX 0x26

// where the tile and slot textures live in VRAM: one texture page, and the CLUT row its palettes sit in
#define WORLD_MAP_TEXTURE_VRAM_X 0x380
#define WORLD_MAP_TEXTURE_VRAM_Y 0x100
#define WORLD_MAP_CLUT_VRAM_Y 0x1FE

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void func_80029AFC(StreamDataQueueEntry* pQueue, int arg1, int arg2);
extern void WorldMapGetGameStatePosition(void* pVector);

// The stream request table at g_WorldMapStreamQueue: g_WorldMapStreamQueue and the D_ symbols
// that follow are its entries, one object each. Retail stores every entry field through its own
// absolute symbol, and that shape is load-bearing - one `StreamDataQueueEntry[]` object makes gcc
// keep the base in a saved register across the HeapAlloc calls below and the function grows 8 bytes.
// g_WorldMapPlayerPosition/B0/B4 are the 12.4 fixed point position written by
// WorldMapGetGameStatePosition.
extern void* g_MenuResourcesFile;
extern void* g_MenuSedsFile;
extern void* g_WorldMapWorkBuffer1;
extern void* g_WorldMapWorkBuffer2;
extern s32 g_WorldMapMode;
extern void* g_WorldMapLoadedFileData;
extern void* g_WorldMapDecompressedMapData;
extern s32 g_WorldMapReloadFileIndex;
extern s32 D_8009C17C;
extern s32 g_WorldMapPlayerRotation;
extern void* g_WorldMapTexturePackData;
extern s32 g_WorldMapPlayerPosition;
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 D_8009D3C4;
extern s32 g_WorldMapSedsBankFileIndex;
extern StreamDataQueueEntry g_WorldMapStreamQueue;
extern StreamDataQueueEntry D_8009D400;
extern StreamDataQueueEntry D_8009D408;
extern StreamDataQueueEntry D_8009D410;
extern StreamDataQueueEntry D_8009D418;
extern StreamDataQueueEntry D_8009D420;
extern void* g_WorldMapCompressedMapData;

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ u16 id;
    /* 0x4 */ s16 z;
    /* 0x6 */ u16 pad6;
} WorldMapPoint; // Size: 0x8

extern WorldMapPoint* g_WorldMapPoints;

// The world map frame render context g_WorldMapRenderContext points at: the two ordering
// table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long* pOt[2];
} WorldMapRenderContext; // partial view

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800712D0);

int WorldMapModeInitNop(void) {
    return 1;
}

extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapPathFrame;
extern s16 g_WorldMapSlotUpdateFlags;
extern s32 g_WorldMapScrollX;
extern s32 g_WorldMapCameraTarget;
extern WorldMapRenderContext* g_WorldMapRenderContext;
extern s32 g_WorldMapGridAngle;

extern void WorldMapBuildRotationMatrix(s32* p);
extern void func_80097244(s32* p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_80085CDC(void);
extern void func_8008615C(void);
extern void func_800747DC(void);
extern void func_800848F4(void);
extern void func_800980D4(s32* p);
extern void func_800981C8(s32* p);
extern void WorldMapWaitForLoadSlotFree(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32* p);
extern void func_8009932C(u_long* pOt0, u_long* pOt1, s32* pState);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void func_800740B8(void);

s32 WorldMapUpdateFrameWithSlotProjection(void) {
    s32* pState;
    s32 pad[2];

    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildRotationMatrix(&g_WorldMapPathFrame);
    } else {
        func_80097244(&g_WorldMapPathFrame);
    }
    func_80089748();
    func_80089C78();
    func_80085CDC();
    func_8008615C();
    func_800747DC();
    func_800848F4();
    func_800980D4(&g_WorldMapScrollX);
    if (g_WorldMapSlotUpdateFlags != 0) {
        func_800981C8(&g_WorldMapCameraTarget);
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
    if (((u16*)g_GameState.unk1838)[5] == 0) {
        func_800740B8();
    }
    return 1;
}
typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
} WorldMapTableEntry; // Size: 0x8

extern u16 D_8009B564;
extern u16 D_8009B566;
extern WorldMapTableEntry D_8009B57C;
extern WorldMapTableEntry D_8009B58C;
extern s32 g_WorldMapLevelIndex;
extern s32 g_WorldMapWdsFileIndex;
extern s32 g_WorldMapSoundBankFileIndex;
extern s32 g_WorldMapMode7SoundBankFileIndex;
extern s32 g_WorldMapGridImageFileIndex;
extern s32 D_8009BCC8;
extern s32 g_WorldMapTileRows;
extern s32 g_WorldMapTileColumns;
extern s32 D_8009BD08;
extern s32 g_WorldMapAreaMode;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071B9C);
/* best: 96.74% (308 bytes, same size). Everything matches except the placement
   of the D_8009B564 base: retail materialises it in the block BEFORE the pre-check
   (lui v1 / addiu %lo(B564) with addend 0) and then adds 2 inside the if, while
   gcc folds the +2 into a rematerialised la inside the if and leaves a nop in the
   pre-block. Instruction count and every other insn are identical, so the only
   remaining lever is keeping that address in a register across the branch; gcc
   cse always rematerialises it. Shapes that got here:
   - pre-check tests the SYMBOL D_8009B566 (not *pTable), so gcc folds that load
     to a fresh lui v0 (matches retail);
   - the pointer is &D_8009B564 (NOT &D_8009B566) plus a separate increment, which
     is what produces retail's la-with-addend-0 + addiu 2 pattern;
   - the loop is a do/while with the increment first, which reproduces retail's
     rotated body exactly;
   - the table index after the loop is nLevel (retail does sll v0,a0,3 after
     addiu a0,a0,1, so the entry index and g_WorldMapLevelIndex both use the counter), and
     pEntry is formed BEFORE the g_WorldMapLevelIndex store.
   Tried with no change: pTable = &D_8009B564 + nLevel, (u16*)((u8*)pTable + 2),
   the increment statement before vs after the if, and pTable[1] as the pre-check
   (that one loads through the base register instead of folding).
   Round 50: gcc forms the la as %lo(D_8009B566)+2 (the symbol it already loaded
   for the pre-check), not %lo(D_8009B564); gcc-2.7.2-cdk drops this to 81.1% and
   -G0 does not compile this unit, so the preset is not the lever.
*/
/*
void func_80071B9C(s32 arg0, s32 arg1) {
    WorldMapTableEntry* pEntry;
    u16* pTable;
    s32 nLevel;

    if (arg0 < 8) {
        nLevel = 1;
        pTable = &D_8009B564;
        if (arg1 >= D_8009B566) {
            pTable++;
            do {
                pTable++;
                nLevel++;
            } while (arg1 >= *pTable);
        }
        pEntry = (WorldMapTableEntry*)(nLevel * 8 + (s32)&D_8009B57C);
        g_WorldMapLevelIndex = nLevel - 1;
    } else {
        pEntry = (WorldMapTableEntry*)(arg0 * 8 + (s32)&D_8009B58C);
    }
    D_8009D3C4 = pEntry->unk0 + 1;
    D_8009C17C = pEntry->unk0 + 2;
    g_WorldMapReloadFileIndex = pEntry->unk0 + 3;
    g_WorldMapWdsFileIndex = pEntry->unk0 + 4;
    g_WorldMapSoundBankFileIndex = pEntry->unk0 + 5;
    g_WorldMapSedsBankFileIndex = pEntry->unk0 + 6;
    g_WorldMapMode7SoundBankFileIndex = pEntry->unk0 + 7;
    D_8009BCC8 = pEntry->unk0 + 8;
    g_WorldMapGridImageFileIndex = pEntry->unk0 + 9;
    D_8009BD08 = pEntry->unk0 + 0xA;
    g_WorldMapTileColumns = pEntry->unk2;
    g_WorldMapTileRows = pEntry->unk4;
    g_WorldMapAreaMode = pEntry->unk6;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071CDC);
/* best: 84.51% (508 bytes vs 500). Every decoded fact matches and loop 1 is
   byte-identical; only loop 2's register shape differs. Retail keeps TWO
   running pointers advanced by 8 (a2 = index ptr = &g_WorldMapStreamQueue, a1 = pData ptr =
   &D_8009D3FC with a2 = a1-4, both incremented right after their own store),
   nCount in $s1 and the loop index in $a0; gcc keeps ONE queue pointer (v1,
   +8) with a `sw v0,4(v1)` displacement, nCount in $a2 and the index in $s1.
   Retail also has no `andi 0xff` after the partyMembers lbu (gcc 2.7.2 masks it
   because the u8 feeds a short store). Tried with no change: s32 locals for the
   ids (breaks loop 1), pData stored before archiveIndex, nCount++ folded into
   `pQueue[nCount++].pData`, terminator stores through the absolute symbols
   g_WorldMapStreamQueue/D_8009D3FC (grows the frame to 0x38).
*/
/*
extern void* g_WorldMapPartyActorAnimFile0[3];
extern void* g_WorldMapActorAnimFiles[3];
extern s32 D_8009C170;

void func_80071CDC(void) {
    StreamDataQueueEntry* pQueue;
    s32 i;
    s32 nCount;
    u8 nCharacterId;
    u8 nGearId;

    for (i = 0; i < 3; i++) {
        nCharacterId = g_GameState.partyMembers[i];
        if (nCharacterId != CHARACTER_ID_NONE) {
            g_WorldMapPartyActorAnimFile0[i] = HeapAlloc(ArchiveDecodeAlignedSize(nCharacterId + 2), 0);
            nGearId = g_GameState.characters[nCharacterId].gearId;
            if (nGearId != GEAR_ID_NONE) {
                g_WorldMapActorAnimFiles[i] = HeapAlloc(ArchiveDecodeAlignedSize(nGearId + 0x13), 0);
            } else {
                g_WorldMapActorAnimFiles[i] = 0;
            }
        } else {
            g_WorldMapActorAnimFiles[i] = 0;
            g_WorldMapPartyActorAnimFile0[i] = 0;
        }
    }
    pQueue = (StreamDataQueueEntry*)&g_WorldMapStreamQueue;
    D_8009C170 = 0;
    nCount = 0;
    for (i = 0; i < 3; i++) {
        nCharacterId = g_GameState.partyMembers[i];
        if (nCharacterId != CHARACTER_ID_NONE) {
            pQueue[nCount].archiveIndex = nCharacterId + 2;
            pQueue[nCount].pData = g_WorldMapPartyActorAnimFile0[i];
            D_8009C170++;
            nCount++;
            nGearId = g_GameState.characters[nCharacterId].gearId;
            if (nGearId != GEAR_ID_NONE) {
                pQueue[nCount].archiveIndex = nGearId + 0x13;
                pQueue[nCount].pData = g_WorldMapActorAnimFiles[i];
                nCount++;
            }
        }
    }
    pQueue[nCount].archiveIndex = 0;
    pQueue[nCount].pData = 0;
    func_80029AFC(pQueue, 0, 0);
}
*/


INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071EF0);
/* best: 98.00% (256 bytes vs 252). Only the g_WorldMapStreamQueue store differs: the
   original reuses the $a0 that already holds the address for the func_80029AFC
   call and emits a bare "sh $v1,0($a0)"; gcc re-forms the symbol address and
   needs one extra lui $at. A local StreamDataQueueEntry* does not stop that
   folding, and neither does routing the address through an s32 local or a
   volatile pointer: cse replaces the store's (mem (symbol_ref)) with %hi/%lo
   regardless, because the address pseudo is created after the store. Dead end
   unless the address comes from a runtime base.
   Round 51: permuter 434 iterations, base=125 best=125 - no lever in the body.

void func_80071EF0(void) {
    void* pBuffer;
    StreamDataQueueEntry* pQueue;

    g_WorldMapTexturePackData = HeapAlloc(ArchiveDecodeAlignedSize(D_8009C17C), 1);
    g_WorldMapLoadedFileData = HeapAlloc(ArchiveDecodeAlignedSize(g_WorldMapReloadFileIndex), 1);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009D3C4), 1);
    pQueue = (StreamDataQueueEntry*)&g_WorldMapStreamQueue;
    g_WorldMapDecompressedMapData = pBuffer;
    D_8009D410.archiveIndex = 0;
    g_WorldMapStreamQueue.pData = pBuffer;
    D_8009D410.pData = 0;
    pQueue->archiveIndex = D_8009D3C4;
    D_8009D400.archiveIndex = D_8009C17C;
    D_8009D408.archiveIndex = g_WorldMapReloadFileIndex;
    D_8009D400.pData = g_WorldMapTexturePackData;
    D_8009D408.pData = g_WorldMapLoadedFileData;
    func_80029AFC(pQueue, 0, 0);
}
*/

void WorldMapQueueTwoFileLoads(void) {
    void* pBuffer;

    g_MenuResourcesFile = HeapAlloc(ArchiveDecodeAlignedSize(WORLD_MAP_MENU_RESOURCES_ARCHIVE_INDEX), 1);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(WORLD_MAP_COMPRESSED_MAP_ARCHIVE_INDEX), 1);
    g_WorldMapStreamQueue.archiveIndex = WORLD_MAP_COMPRESSED_MAP_ARCHIVE_INDEX;
    g_WorldMapCompressedMapData = pBuffer;
    g_WorldMapStreamQueue.pData = pBuffer;
    D_8009D400.archiveIndex = WORLD_MAP_MENU_RESOURCES_ARCHIVE_INDEX;
    D_8009D408.archiveIndex = 0;
    D_8009D408.pData = 0;
    D_8009D400.pData = g_MenuResourcesFile;
    func_80029AFC(&g_WorldMapStreamQueue, 0, 0);
}

extern s32 g_WorldMapAudioManagerLoaded;
extern s32 g_WorldMapWdsFileIndex;
extern s32 g_WorldMapSoundBankFileIndex;
extern s32 g_WorldMapMode7SoundBankFileIndex;
extern s32 D_8009BCC8;
extern void* g_WorldMapWdsFileBuffer;
extern void* g_WorldMapSoundBankBuffer;
extern void* g_WorldMapMode7SoundBankBuffer;
extern void* D_8009C614;

void WorldMapQueueFileLoads(void) {
    void* pBuffer;

    g_WorldMapAudioManagerLoaded++;
    g_WorldMapStreamQueue.archiveIndex = g_WorldMapWdsFileIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_WorldMapWdsFileIndex), 1);
    g_WorldMapWdsFileBuffer = pBuffer;
    g_WorldMapStreamQueue.pData = pBuffer;
    D_8009D400.archiveIndex = g_WorldMapSoundBankFileIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_WorldMapSoundBankFileIndex), 0);
    g_WorldMapSoundBankBuffer = pBuffer;
    D_8009D400.pData = pBuffer;
    D_8009D408.archiveIndex = g_WorldMapSedsBankFileIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_WorldMapSedsBankFileIndex), 0);
    g_MenuSedsFile = pBuffer;
    D_8009D408.pData = pBuffer;
    D_8009D410.archiveIndex = g_WorldMapMode7SoundBankFileIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_WorldMapMode7SoundBankFileIndex), 0);
    g_WorldMapMode7SoundBankBuffer = pBuffer;
    D_8009D410.pData = pBuffer;
    D_8009D418.archiveIndex = D_8009BCC8;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009BCC8), 0);
    D_8009C614 = pBuffer;
    D_8009D418.pData = pBuffer;
    D_8009D420.archiveIndex = 0;
    D_8009D420.pData = 0;
    func_80029AFC(&g_WorldMapStreamQueue, 0, 0);
}

void WorldMapLoadSedsBank(void) {
    s32 nSize;

    nSize = ArchiveDecodeAlignedSize(g_WorldMapSedsBankFileIndex);
    g_MenuSedsFile = HeapAlloc(nSize, 0);
    ArchiveReadFileToBuffer(g_WorldMapSedsBankFileIndex, g_MenuSedsFile, 0, 0);
}

extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapMode4EntryTriggered;
extern s32 g_WorldMapL2R2Pressed;
extern s32 g_WorldMapL2R2PressedLast;
extern s32 g_WorldMapL2R2PressEdge;
extern s32 g_WorldMapWrapPositionsCallback;
extern s32 D_8009C178;
extern s32 g_WorldMapInitModeIndex;
extern s32 g_WorldMapAreaIndex;
extern int g_AnimScriptExtraTicks;
extern s16 g_SoundControlFlags;
extern u8 g_SoundBankLoadBuffer[];
extern u8 g_FieldPlayerWalkmeshMaterial;
extern void* g_SoundAudioManager;
extern void* g_SoundPendingAudioManager;
extern void* g_GameCurLoadedWDS;

// The slot handlers every map installs, and the extra list a level adds (indexed by
// g_WorldMapLevelIndex). Each table is terminated by a NULL enter handler.
typedef s32 (*WorldMapSlotHandler)();

typedef struct {
    /* 0x0 */ WorldMapSlotHandler pEnterHandler;
    /* 0x4 */ WorldMapSlotHandler pUpdateHandler;
} WorldMapSlotInitEntry; // Size: 0x8

extern WorldMapSlotInitEntry g_WorldMapSlotInitEntries[];
extern WorldMapSlotInitEntry* g_WorldMapLevelSlotInitEntries[];

extern void func_80071EF0(void);
extern s32 ArchiveDataSync(void);
extern void ArchiveCdDataSync(s32);
extern s32 Vsync(s32);
extern void SystemTransferPaletteToVRAM(s32, s32);
extern void WorldMapDecompressMapTables(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern void SoundStopAllAudioManagers(void);
extern void SoundDestroyAudioManager(void* pManager);
extern void func_80073448(s32 areaIndex);
extern void WorldMapLoadState(void);
extern void WorldMapSyncGearRideActors(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapLoadClutAndTpageIds(void);
extern void func_800736DC(void);
extern void func_80073E30(void);
extern void WorldMapRelocateMapEntryOffsets(void);
extern void GfxAllocateWorkBuffers(s32 size, s32 flags);
extern void WorldMapAllocPrimBuffers(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void* SoundLoadWdsFile(void* pBuffer, s32 arg1);
extern void func_80097BC0(void* pPosition);
extern void WorldMapUpdateLoadQueue(void);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void WorldMapRefreshGrid(void* pTarget);
extern void WorldMapWaitForQueuedLoads(void);
extern void SoundAddSedsEntry(void* pFile);
extern void* SoundCreateAudioManagerOnHeap(void* pFile);
extern void SoundPlayAudioManager(void* pManager, s32 volume, s32 arg2);
extern void SoundReloadAudioManagerSamples(void* pManager, s32 volume, s32 arg2);
extern void func_80089160(s32 arg0, s32 arg1, s32 arg2);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void func_80085FE0(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapSetSlotEnterHandler(WorldMapSlotHandler pHandler, s32 index);
extern void WorldMapAddSlot(WorldMapSlotHandler pEnterHandler, WorldMapSlotHandler pUpdateHandler);

extern s32 WorldMapModeInitFadeRect();
extern s32 func_8008A52C();
extern s32 WorldMapModeInitCharacterSprite1();
extern s32 WorldMapModeInitCharacterSprite2();
extern s32 WorldMapModeInitPartyActor0();
extern s32 WorldMapModeInitPartyActor1();
extern s32 WorldMapModeInitPartyActor2();
extern s32 func_8008E4F4();
extern s32 WorldMapModeInitSpriteSets2And3();
extern s32 WorldMapInitAreaNameString();
extern s32 func_80092DF8();
extern s32 WorldMapSetSceneSpriteSetParents();
extern s32 func_8008868C();
extern s32 WorldMapSetSceneLayerTPages();
extern s32 func_80088C90();

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072238);
/* best: 99.81% (1892 bytes = retail's exact size, 472 insns). The whole function is
   instruction-identical except ONE store: the two `= 1` stores (g_AnimScriptExtraTicks,
   D_8009C178) which retail emits as `sw v1` after the five zero stores, with `li v1,1`
   hoisted to the top of the block; ours emits the first `sw` right after the `la` of
   WorldMapWrapTileVertexPositions. The register choice is the whole lever: as long as
   the constant lands in $2 it collides with the callback address and gcc must emit the
   stores before it. Source orders measured: the statement order of that 9-store block
   moves the score 98.7-99.83 and none of 11 orders reached 100 (best 99.83 with
   Z4,Z1,Z2,A1,Z5,TP,Z3,DM,C1). The body below is the natural order (99.81).
   Found by the permuter: `pLoadBuffer = g_SoundBankLoadBuffer;` at the head of the
   0-branch and using it as memcpy's dst is what fixed the sound block (memcpy was
   otherwise keeping &g_SoundBankLoadBuffer in $s0 and taking $s1 from pBankBuffer,
   where retail rematerialises the address into $a0); it took the function from 97.94
   to 98.88 and is kept below. The permuter's other two edits were wrong code (a
   pointer truncated through a `volatile unsigned short`) and were not ported.
   A jump table (jtbl_8006FAF4, 6 words) belongs to this function: converting it is
   still the right first step for this unit's table chain, so an un-park must land
   check_bin.sh world_map. */
/*
void func_80072238(void) {
    RECT rect;
    void* pBankBuffer;
    u8* pLoadBuffer;
    WorldMapSlotInitEntry* pEntry;
    WorldMapSlotInitEntry* pLevelEntries;
    s32 nSlot;
    s32 nFileIndex;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = WORLD_MAP_SCREEN_WIDTH;
    rect.h = WORLD_MAP_SCREEN_HEIGHT;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    ArchiveCdDataSync(0);
    func_80071EF0();
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapTables();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_AnimScriptExtraTicks = 1;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 4;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapL2R2Pressed = 0;
    g_WorldMapL2R2PressedLast = 0;
    g_WorldMapL2R2PressEdge = 0;
    g_WorldMapRotationMatrixValid = 0;
    D_8009C178 = 1;
    g_WorldMapWrapPositionsCallback = (s32)WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    if (g_WorldMapInitModeIndex == 0) {
        SoundFreeLoadedWdsAndAudioManager();
    } else {
        SoundStopAllAudioManagers();
        SoundDestroyAudioManager(g_SoundAudioManager);
        g_SoundAudioManager = g_SoundPendingAudioManager;
        g_SoundPendingAudioManager = NULL;
    }
    if (g_GameState.worldMapMenuActive != 0) {
        WorldMapSetPositionFromState();
    } else if (g_WorldMapInitModeIndex == 0) {
        func_80073448(g_WorldMapAreaIndex);
    } else {
        WorldMapLoadState();
        WorldMapSyncGearRideActors();
    }
    ArchiveCdDataSync(0);
    WorldMapLoadTextureClut();
    WorldMapLoadClutAndTpageIds();
    WorldMapInitSpriteSets();
    WorldMapQueueFileLoads();
    func_800736DC();
    func_80073E30();
    WorldMapRelocateMapEntryOffsets();
    GfxAllocateWorkBuffers(0x1400, 0);
    WorldMapAllocPrimBuffers();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    if (g_WorldMapInitModeIndex == 0) {
        g_GameCurLoadedWDS = SoundLoadWdsFile(g_WorldMapWdsFileBuffer, 0);
    }
    ArchiveSetIndex(WORLD_MAP_SOUND_ARCHIVE_INDEX, 0);
    if (g_WorldMapInitModeIndex == 0) {
        func_80097BC0(&g_WorldMapPlayerPosition);
        do {
            WorldMapUpdateLoadQueue();
            Vsync(0);
        } while (WorldMapGetQueuedLoadCount() >= 2);
    } else {
        WorldMapRefreshGrid(&g_WorldMapCameraTarget);
        WorldMapWaitForQueuedLoads();
    }
    if (g_WorldMapInitModeIndex == 0) {
        pLoadBuffer = g_SoundBankLoadBuffer;
        while (g_SoundControlFlags & 0x10) {
        }
        HeapFree(g_WorldMapWdsFileBuffer);
        SoundAddSedsEntry(g_MenuSedsFile);
        if (g_WorldMapMode == 7) {
            nFileIndex = g_WorldMapMode7SoundBankFileIndex;
            pBankBuffer = g_WorldMapMode7SoundBankBuffer;
        } else {
            nFileIndex = g_WorldMapSoundBankFileIndex;
            pBankBuffer = g_WorldMapSoundBankBuffer;
        }
        memcpy(pLoadBuffer, pBankBuffer, ArchiveDecodeAlignedSize(nFileIndex));
        g_SoundAudioManager = SoundCreateAudioManagerOnHeap(g_SoundBankLoadBuffer);
        SoundPlayAudioManager(g_SoundAudioManager, 0x7F, 0);
    } else {
        HeapFree(g_WorldMapWdsFileBuffer);
        SoundAddSedsEntry(g_MenuSedsFile);
        if (g_WorldMapMode == 7) {
            nFileIndex = g_WorldMapMode7SoundBankFileIndex;
            pBankBuffer = g_WorldMapMode7SoundBankBuffer;
        } else {
            nFileIndex = g_WorldMapSoundBankFileIndex;
            pBankBuffer = g_WorldMapSoundBankBuffer;
        }
        memcpy(g_SoundBankLoadBuffer, pBankBuffer, ArchiveDecodeAlignedSize(nFileIndex));
        SoundReloadAudioManagerSamples(g_SoundAudioManager, 0x7F, 0xF0);
    }
    switch (g_WorldMapInitModeIndex) {
    case 0:
        for (nSlot = 0; g_WorldMapSlotInitEntries[nSlot].pEnterHandler != NULL; nSlot++) {
            WorldMapAddSlot(g_WorldMapSlotInitEntries[nSlot].pEnterHandler, g_WorldMapSlotInitEntries[nSlot].pUpdateHandler);
        }
        pLevelEntries = g_WorldMapLevelSlotInitEntries[g_WorldMapLevelIndex];
        if (pLevelEntries->pEnterHandler != NULL) {
            pEntry = pLevelEntries;
            do {
                WorldMapAddSlot(pEntry->pEnterHandler, pEntry->pUpdateHandler);
                pEntry++;
            } while (pEntry->pEnterHandler != NULL);
        }
        break;
    case 1:
        WorldMapSetSlotEnterHandler(WorldMapModeInitFadeRect, 0);
        WorldMapSetSlotEnterHandler(func_8008A52C, 1);
        WorldMapSetSlotEnterHandler(WorldMapModeInitCharacterSprite1, 2);
        WorldMapSetSlotEnterHandler(WorldMapModeInitCharacterSprite2, 3);
        WorldMapSetSlotEnterHandler(WorldMapModeInitPartyActor0, 4);
        WorldMapSetSlotEnterHandler(WorldMapModeInitPartyActor1, 5);
        WorldMapSetSlotEnterHandler(WorldMapModeInitPartyActor2, 6);
        WorldMapSetSlotEnterHandler(func_8008E4F4, 7);
        WorldMapSetSlotEnterHandler(WorldMapModeInitSpriteSets2And3, 8);
        WorldMapSetSlotEnterHandler(WorldMapInitAreaNameString, 0xC);
        WorldMapSetSlotEnterHandler(func_80092DF8, 0xD);
        WorldMapSetSlotEnterHandler(WorldMapModeInitNop, 0xE);
        switch (g_WorldMapLevelIndex - 3) {
        case 0:
            WorldMapSetSlotEnterHandler(WorldMapSetSceneSpriteSetParents, 0xF);
            WorldMapSetSlotEnterHandler(func_8008868C, 0x10);
            break;
        case 1:
            WorldMapSetSlotEnterHandler(WorldMapSetSceneSpriteSetParents, 0xF);
            WorldMapSetSlotEnterHandler(func_8008868C, 0x10);
            WorldMapSetSlotEnterHandler(WorldMapSetSceneLayerTPages, 0x11);
            WorldMapSetSlotEnterHandler(func_80088C90, 0x13);
            break;
        case 2:
        case 3:
        case 4:
            WorldMapSetSlotEnterHandler(WorldMapSetSceneSpriteSetParents, 0xF);
            WorldMapSetSlotEnterHandler(func_8008868C, 0x10);
            // fallthrough
        case 5:
            WorldMapSetSlotEnterHandler(WorldMapSetSceneLayerTPages, 0x11);
            break;
        }
        break;
    }
    g_FieldPlayerWalkmeshMaterial = 0;
    if (g_WorldMapLevelIndex == 0) {
        func_80089160(0xE, 0, 0);
        g_FieldPlayerWalkmeshMaterial = 1;
    }
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80085FE0();
    if (g_WorldMapInitModeIndex == 0) {
        WorldMapInitRandomTimers();
    }
    SystemTransferPaletteToVRAM(0x130, 0x1E0);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_8007299C);
/* best: 98.37%, 532 bytes (same size). Everything matches except the four
   insns of `g_NextMapFlags |= 0x8000`: retail materialises the address once
   (lui v1,%hi / addiu v1,v1,%lo) and loads and stores through v1, gcc folds the
   address into each access (lui v0,%hi + lhu %lo, then lui $at + sh %lo).
   Tried with no change: a u16* local, a two-statement pointer load/store, and
   `volatile u16`. Second note: the first loop's `addu` needed the index first,
   i.e. `(u8*)(i * 0x80 + (s32)g_WorldMapSlots)`; the ptr-first spelling gives the
   reversed addu. Sibling teardown WorldMapAllocPrimBuffers has the same 98.55% class.
   Round 50: re-confirmed; `u16* pFlags = &g_NextMapFlags; *pFlags |= 0x8000;` (and with a
   `volatile u16*`) still gives lui v0 / lhu %lo(v0) / lui $at + sh %lo: gcc folds the
   address into the load, so the loaded value occupies the only free register and the
   store needs $at. Retail needs the address in a THIRD register (v1) used twice.
*/
/*
extern void* g_SoundAudioManager;
extern void* g_WorldMapSlots;
extern void* g_WorldMapTilePrimBuffer;
extern void* g_WorldMapTilePrimBufferCopy;
extern u16 g_NextMapFlags;
extern s32 g_WorldMapAreaMode;
extern void* g_WorldMapPartyActorAnimFile0[3];
extern void* g_WorldMapActorAnimFiles[3];

extern void SoundSetAudioManagerVolumeWithFade(void* p, s32 arg1, s32 arg2);
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void* p);
extern void SpriteDestroy(void* p);
extern void WorldMapSaveState(void);
extern void WorldMapFreeAreaNameString(void);
extern void WorldMapFreeMode4AreaNameString(void);
extern void WorldMapFreeSpriteSets(void);
extern void WorldMapFreeSpritePrimBuffers(void);
extern void GfxFreeWorkBuffers(void);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreePrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeGridImages(void);
extern void WorldMapFreeSlotPool(void);
extern void WorldMapFreeLoadQueues(void);

void func_8007299C(void) {
    u8* p;
    s32 i;

    if (g_WorldMapAreaMode == 0) {
        SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0xF0);
    }
    SoundStopAllElements();
    SoundRemoveSedsEntry(g_MenuSedsFile);
    HeapFree(g_MenuSedsFile);
    for (i = 0; i < 0x40; i++) {
        p = (u8*)(i * 0x80 + (s32)g_WorldMapSlots);
        if (*(void**)(p + 0x4C) != NULL) {
            SpriteDestroy(*(void**)(p + 0x4C));
            *(void**)((u8*)g_WorldMapSlots + i * 0x80 + 0x4C) = NULL;
        }
    }
    if (g_WorldMapAreaMode == 1) {
        WorldMapSaveState();
        g_NextMapFlags |= 0x8000;
    }
    WorldMapFreeAreaNameString();
    WorldMapFreeMode4AreaNameString();
    WorldMapFreeSpriteSets();
    WorldMapFreeSpritePrimBuffers();
    GfxFreeWorkBuffers();
    WorldMapFreeTileVertexBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreePrimBuffers();
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
    for (i = 0; i < 3; i++) {
        if (g_WorldMapPartyActorAnimFile0[i] != NULL) {
            HeapFree(g_WorldMapPartyActorAnimFile0[i]);
        }
        if (g_WorldMapActorAnimFiles[i] != NULL) {
            HeapFree(g_WorldMapActorAnimFiles[i]);
        }
    }
    WorldMapFreeSlotPool();
    WorldMapFreeLoadQueues();
}
*/

typedef struct {
    /* 0x00 */ DRAWENV drawEnv; // Size: 0x5C
    /* 0x5C */ DISPENV dispEnv; // Size: 0x14
    /* 0x70 */ u8 pad70[8];
} WorldMapGfxEnv; // Size: 0x78

extern WorldMapGfxEnv g_WorldMapGfxEnvs[2];
extern u8 g_WorldMapColorFactors;
extern u8 D_8009BB49;
extern u8 D_8009BB4A;
extern u8 D_8009BBDE;
extern u8 D_8009BBE0;
extern u8 D_8009BBE1;
extern u8 D_8009BBE2;
extern u8 D_8009BBE3;
extern u8 D_8009BC56;
extern u8 D_8009BC58;
extern u8 D_8009BC59;
extern u8 D_8009BC5A;
extern u8 D_8009BC5B;
extern u16 D_8009BC2C;
extern u16 D_8009BC2E;
extern u16 D_8009BC30;
extern u16 D_8009BC32;
extern u16 D_8009BCA4;
extern u16 D_8009BCA6;
extern u16 D_8009BCA8;
extern u16 D_8009BCAA;
extern s32 g_WorldMapProjectionDistance;
extern s32 g_WorldMapAreaMode;
extern s32 g_WorldMapScrollTargetX;

extern void SetNearColor(long rfc, long gfc, long bfc);

void WorldMapInitGfxEnv(void) {
    WorldMapGfxEnv* pEnv;
    s32 nNear;

    ResetGraph(1);
    g_WorldMapProjectionDistance = WORLD_MAP_PROJECTION_DISTANCE;
    SetGeomScreen(WORLD_MAP_PROJECTION_DISTANCE);
    pEnv = g_WorldMapGfxEnvs;
    SetDefDrawEnv(&pEnv[0].drawEnv, 0, 0, WORLD_MAP_SCREEN_WIDTH, WORLD_MAP_SCREEN_HEIGHT);
    SetDefDrawEnv(&pEnv[1].drawEnv, 0, WORLD_MAP_SCREEN_HEIGHT, WORLD_MAP_SCREEN_WIDTH, WORLD_MAP_SCREEN_HEIGHT);
    SetDefDispEnv(&pEnv[0].dispEnv, 0, WORLD_MAP_SCREEN_HEIGHT, WORLD_MAP_SCREEN_WIDTH, WORLD_MAP_SCREEN_HEIGHT);
    SetDefDispEnv(&pEnv[1].dispEnv, 0, 0, WORLD_MAP_SCREEN_WIDTH, WORLD_MAP_SCREEN_HEIGHT);
    D_8009BC58 = 1;
    D_8009BBE0 = 1;
    D_8009BC56 = 1;
    D_8009BBDE = 1;
    if (g_WorldMapAreaMode == 2) {
        D_8009BBE1 = 0;
        D_8009BBE2 = 0;
        D_8009BBE3 = 0;
        D_8009BC59 = 0;
        D_8009BC5A = 0;
        D_8009BC5B = 0;
    } else {
        D_8009BBE1 = 0;
        D_8009BBE2 = 0;
        D_8009BBE3 = 0x70;
        D_8009BC59 = 0;
        D_8009BC5A = 0;
        D_8009BC5B = 0x70;
    }
    D_8009BCA6 = 0xA;
    D_8009BC2E = 0xA;
    D_8009BCA8 = 0x100;
    D_8009BC30 = 0x100;
    D_8009BCA4 = 0;
    D_8009BC2C = 0;
    D_8009BCAA = 0xD8;
    D_8009BC32 = 0xD8;
    SetNearColor(0x80, 0x80, 0x80);
    SetBackColor(0x80, 0x80, 0x80);
    SetFarColor(g_WorldMapColorFactors, D_8009BB49, D_8009BB4A);
    nNear = 0x800;
    if (g_WorldMapAreaMode == 2) {
        nNear = 0xB00;
    }
    SetFogNearFar(nNear, 0xE80, g_WorldMapProjectionDistance);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072DB4);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073300);
/* best: 93.68%, 144 bytes vs retail's 152. The table is not the blocker (retail's
   is 5 entries at .rodata+0x18 with one pad word before the next object), and the
   else path is settled: `nMode = 2; if (!(gearRide[0] | gearRide[1] | gearRide[2])) nMode = 1;`
   gives retail's `bnez` with `li v0,2` in the slot; the ternary and
   `if / else if / else` forms give a `beqz` with `li v0,1` instead.
   Two residuals: (1) the missing 8 bytes are retail's TWO separate `nMode = 7`
   arms (cases 3 and 4); gcc merges the two identical blocks at every spelling
   tried - separate arms, `case 3: case 4:`, a temp copy, a double assignment,
   reordered case labels, `default` between them. Cross-jumping runs after cse, so
   any source difference that cse folds away is merged again.
   (2) the value pseudo is $a1 here and $v0 in retail.
*/
/*
void func_80073300(void) {
    s32 nMode;

    if (g_GameState.worldMapState & 0x4000) {
        switch (g_GameState.worldMapState & 0x1FFF) {
        case 0:
            return;
        case 1:
            nMode = 4;
            break;
        case 2:
            nMode = 5;
            break;
        case 3:
            nMode = 7;
            break;
        case 4:
            nMode = 7;
            break;
        default:
            return;
        }
    } else {
        nMode = 2;
        if (!(g_GameState.gearRide[0] | g_GameState.gearRide[1] | g_GameState.gearRide[2])) {
            nMode = 1;
        }
    }
    g_WorldMapMode = nMode;
}
*/

void WorldMapSetPositionFromState(void) {
    s32 nMode;
    s32 nPos;

    nMode = g_WorldMapMode;
    g_GameState.worldMapMenuActive = 0;
    switch (nMode) {
    case 1:
    case 2:
        nPos = g_GameState.worldMapPosX << 12;
        g_WorldMapPlayerPosition = nPos;
        g_WorldMapPlayerRotation = g_GameState.worldMapPlayerRotation;
        nPos = g_GameState.worldMapPosZ << 12;
        g_WorldMapPlayerPositionZ = nPos;
        break;
    case 4:
    case 5:
    case 7:
        WorldMapGetGameStatePosition(&g_WorldMapPlayerPosition);
        g_WorldMapPlayerRotation = g_GameState.scriptAngle;
        break;
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073448);
/* best: 94.74%, 228 bytes vs retail's 232 - the missing insn is the load-delay
   `nop` after `lh v0,4(a2)`. Retail's found-point block is
   `lh v0,0(a2) / sw zero,g_WorldMapPlayerPositionY / sll / sw g_WorldMapPlayerPosition / lh v0,4(a2) / nop /
   sll / sw g_WorldMapPlayerPositionZ`, i.e. the x and z loads share $v0 and the z load is NOT
   hoisted. gcc's sched1 hoists the independent z load above the stores whatever
   the statement order. `nPos = p->z;` (load into the shared temp, shift at the
   store) does produce retail's 232-byte shape but gcc then splits the two loads
   into $v0/$v1 and moves `sw zero,g_WorldMapPlayerPositionY` after both of them (91.55%);
   `nPos = p->x;` first is 89.48%. Rest matches retail exactly, including the
   `g_GameState.worldMapState &= ~0x2000` base-register read-modify-write and the
   `sll/sra` narrowed-id search loop.
*/
/*
void func_80073448(s32 arg0) {
    WorldMapPoint* p;
    s32 nPos;

    if (g_GameState.worldMapState & 0x2000) {
        g_GameState.worldMapState &= ~0x2000;
        WorldMapGetGameStatePosition(&g_WorldMapPlayerPosition);
        g_WorldMapPlayerRotation = g_GameState.scriptAngle;
    } else {
        p = g_WorldMapPoints;
        while (*(s16*)&p->id != -1) {
            if ((s16)p->id == arg0) {
                g_WorldMapPlayerPositionY = 0;
                nPos = p->x << 12;
                g_WorldMapPlayerPosition = nPos;
                nPos = p->z << 12;
                g_WorldMapPlayerPositionZ = nPos;
                return;
            }
            p++;
        }
        g_WorldMapPlayerPosition = 0;
        g_WorldMapPlayerPositionY = 0;
        g_WorldMapPlayerPositionZ = 0;
    }
}
*/

// Header of the decompressed map data block: one offset per sub-block, relative to the start
// of the block. The offsets are named after the D_ global the sub-block is stored into.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 pointTableOffset;
    /* 0x08 */ s32 modelOffset;
    /* 0x0C */ s32 modelRelocOffset;
    /* 0x10 */ s32 spriteSetOffset;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 spriteGroupOffset;
    /* 0x1C */ s32 stringTableOffset;
    /* 0x20 */ s32 tileAnimOffset;
    /* 0x24 */ s32 frameAnimOffset;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 tileGroupOffsets[WORLD_MAP_TILE_GROUP_COUNT];
} WorldMapMapDataHeader; // Size: 0x6C

// Head of the point table block: the offsets of the point array and of the four rect offsets
// that follow, relative to the block itself. The block's own sub-blocks are reached by adding
// those offsets to its first byte, so the head is also viewable as that byte array.
typedef union {
    struct {
        /* 0x0 */ s32 pointOffset;
        /* 0x4 */ s32 rectOffset;
    } offsets;
    /* 0x0 */ u8 block[1];
} WorldMapPointTableBlock; // Size: 0x8

extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern s32 g_WorldMapModelData;
extern s32 g_WorldMapModelRelocs;
extern s32 g_WorldMapSpriteSetTable;
extern s32 g_WorldMapPrimBlocks;
extern s32 g_WorldMapSpriteGroups;
extern s32 g_WorldMapStringTable;
extern s32 g_WorldMapTileAnimSet;
extern s32 g_WorldMapFrameAnimSet;
extern s32* g_WorldMapRects;
extern s32 g_WorldMapTileGroupData[];

void WorldMapDecompressMapTables(void) {
    u8* pBase;
    u8* pMapBase;
    WorldMapMapDataHeader* pHeader;
    WorldMapPointTableBlock* pTable;
    s32* pRect;
    s32 i;

    // One variable holds the compressed block and then the point table inside the decompressed
    // one: retail's codegen keeps both in the same callee-saved register.
    pTable = (WorldMapPointTableBlock*)g_WorldMapDecompressedMapData;
    g_WorldMapDecompressedMapData = LZSSHeapDecompress(pTable, 0);
    HeapFree(pTable);
    pBase = (u8*)g_WorldMapDecompressedMapData;
    pHeader = (WorldMapMapDataHeader*)pBase;
    pTable = (WorldMapPointTableBlock*)(pBase + pHeader->pointTableOffset);
    g_WorldMapModelData = (s32)(pBase + pHeader->modelOffset);
    g_WorldMapModelRelocs = (s32)(pBase + pHeader->modelRelocOffset);
    g_WorldMapSpriteSetTable = (s32)(pBase + pHeader->spriteSetOffset);
    // second live copy of the block base, as retail's codegen has
    pMapBase = pBase;
    g_WorldMapPrimBlocks = (s32)(pMapBase + pHeader->unk14);
    g_WorldMapSpriteGroups = (s32)(pMapBase + pHeader->spriteGroupOffset);
    g_WorldMapStringTable = (s32)(pMapBase + pHeader->stringTableOffset);
    g_WorldMapTileAnimSet = (s32)(pMapBase + pHeader->tileAnimOffset);
    g_WorldMapFrameAnimSet = (s32)(pMapBase + pHeader->frameAnimOffset);
    for (i = 0; i < WORLD_MAP_TILE_GROUP_COUNT; i++) {
        g_WorldMapTileGroupData[i] = (s32)(pMapBase + pHeader->tileGroupOffsets[i]);
    }
    // The sub-block offsets are relative to the point table itself, not to the block.
    g_WorldMapPoints = (WorldMapPoint*)(pTable->block + pTable->offsets.pointOffset);
    pRect = (s32*)(pTable->block + pTable->offsets.rectOffset);
    g_WorldMapRects = pRect;
    // the rects are relocated in place; retail reads through the local and reloads the global
    // once after the first store, so only that store goes through pRect
    pRect[0] = (s32)(pTable->block + pRect[0]);
    g_WorldMapRects[1] = (s32)(pTable->block + pRect[1]);
    g_WorldMapRects[2] = (s32)(pTable->block + pRect[2]);
    g_WorldMapRects[3] = (s32)(pTable->block + pRect[3]);
}

void WorldMapAllocWorkBuffers(void) {
    g_WorldMapWorkBuffer1 = HeapAlloc(WORLD_MAP_WORK_BUFFER_SIZE, 0);
    g_WorldMapWorkBuffer2 = HeapAlloc(WORLD_MAP_WORK_BUFFER_SIZE, 0);
}


/* best: 26.13% under this unit's preset (gcc 2.7.2). The blocker is NOT the
   allocator: 2.7.2 folds the `u8* p = &D_8009D198` base into absolute
   `D_8009D198+NN` symbol addresses, so the 40-odd `NN($v0)` offset stores
   become `lui $at` + store pairs and the function is 106 insns instead of 68.
   Retail keeps the base in $v0. Under gcc-2.7.2-cdk, whose `la` is two RTL
   insns (`lui %hi` + `addiu %lo`) so the address is a runtime value, the same
   source scores 90.34% and reproduces the shape (la $v0 + offsets); it only
   rotates the second colour into $v0 instead of $a1 and forms the la at its
   first use. Retail: a0=0xFF7A70, a1=0xFFF5E0, v1=0xC03745, v0=base (formed
   BEFORE the first store), a2=8, a1 reused for 0x38.
   Decoded layout: 0x24-stride sub-blocks at 0x00..0xFC, each with a word at
   +0,+8,+0x10,+0x18 and bytes at +0x23 and +0x27; colours 0xFF7A70, 0xFFF5E0,
   0xC03745; bytes 8 and 0x38; the first 0x40 bytes are reached through their
   own absolute symbols (D_8009D197, D_8009D198, D_8009D1A0, D_8009D1A8,
   D_8009D1B0, D_8009D1BC, D_8009D1C4, D_8009D1CC, D_8009D1D4).
extern u32 D_8009D1A0;
extern u32 D_8009D1A8;
extern u32 D_8009D1B0;
extern u32 D_8009D1BC;
extern u32 D_8009D1C4;
extern u32 D_8009D1CC;
extern u32 D_8009D1D4;
extern u8 D_8009D197;
extern u8 D_8009D19B;
extern u8 D_8009D198;

void func_800736DC(void) {
    u8* p;

    p = &D_8009D198;
    D_8009D1C4 = 0xFF7A70;
    D_8009D1BC = 0xFF7A70;
    D_8009D1A0 = 0xFF7A70;
    *(u32*)p = 0xFF7A70;
    D_8009D1D4 = 0xFFF5E0;
    D_8009D1CC = 0xFFF5E0;
    D_8009D1B0 = 0xFFF5E0;
    D_8009D1A8 = 0xFFF5E0;
    D_8009D197 = 8;
    D_8009D19B = 0x38;
    p[0x23] = 8;
    p[0x27] = 0x38;
    *(u32*)(p + 0x74) = 0xC03745;
    *(u32*)(p + 0x6C) = 0xC03745;
    *(u32*)(p + 0x50) = 0xC03745;
    *(u32*)(p + 0x48) = 0xC03745;
    *(u32*)(p + 0x84) = 0xFF7A70;
    *(u32*)(p + 0x7C) = 0xFF7A70;
    *(u32*)(p + 0x60) = 0xFF7A70;
    *(u32*)(p + 0x58) = 0xFF7A70;
    p[0x47] = 8;
    p[0x4B] = 0x38;
    p[0x6B] = 8;
    p[0x6F] = 0x38;
    *(u32*)(p + 0xCC) = 0xC03745;
    *(u32*)(p + 0xC4) = 0xC03745;
    *(u32*)(p + 0xBC) = 0xC03745;
    *(u32*)(p + 0xB4) = 0xC03745;
    *(u32*)(p + 0xA8) = 0xC03745;
    *(u32*)(p + 0xA0) = 0xC03745;
    *(u32*)(p + 0x98) = 0xC03745;
    *(u32*)(p + 0x90) = 0xC03745;
    p[0x8F] = 8;
    p[0x93] = 0x38;
    p[0xB3] = 8;
    p[0xB7] = 0x38;
    *(u32*)(p + 0x114) = 0xC03745;
    *(u32*)(p + 0x10C) = 0xC03745;
    *(u32*)(p + 0x104) = 0xC03745;
    *(u32*)(p + 0xFC) = 0xC03745;
    *(u32*)(p + 0xF0) = 0xC03745;
    *(u32*)(p + 0xE8) = 0xC03745;
    *(u32*)(p + 0xE0) = 0xC03745;
    *(u32*)(p + 0xD8) = 0xC03745;
    p[0xD7] = 8;
    p[0xDB] = 0x38;
    p[0xFB] = 8;
    p[0xFF] = 0x38;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800736DC);


extern u16 g_WorldMapViewRotationY;
extern MATRIX g_WorldMapViewMatrix;
extern SVECTOR g_WorldMapQuadVertices[16];

// The four quad primitives the world map projects each frame, 0x48 bytes each
// at g_WorldMapQuadPrims; g_WorldMapPrimPacketIndex picks the half of the buffer this frame draws from.
typedef struct {
    /* 0x00 */ u32 tag; // OT link word, rewritten by addPrim
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 xy0; // RotTransPers4 output, one packed (x,y) per vertex
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 xy1;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ u32 xy2;
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ u32 xy3;
    /* 0x24 */ u8 unk24[0x24];
} WorldMapQuadPrim; // Size: 0x48

extern u8 g_WorldMapQuadPrims[];
extern s32 g_WorldMapPrimPacketIndex;
extern s32 g_GfxOTDepthShift;

// Scratchpad area the frame projection works in: GTE's input rotation at 0x00 and, at 0x48,
// the word and flag RotTransPers4 writes for the fourth vertex. The two matrix work areas at
// 0x08/0x28 are addressed by their own literals in retail, so they stay an unnamed span.
typedef struct {
    /* 0x00 */ SVECTOR viewRotation;
    /* 0x08 */ u8 unk8[0x40];
    /* 0x48 */ long interpolated;
    /* 0x4C */ long flag;
} WorldMapProjectionScratch; // Size: 0x50, a view of the 0x1F800000 scratchpad

extern long RotTransPers4(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* v3, long* p0, long* p1,
                          long* p2, long* p3, long* xy0, long* xy1);
extern MATRIX* RotMatrixYXZ(SVECTOR* r, MATRIX* m);
extern MATRIX* CompMatrix(MATRIX* m0, MATRIX* m1, MATRIX* m2);
extern void SetRotMatrix(MATRIX* m);
extern void SetTransMatrix(MATRIX* m);

void func_800737EC(void) {
    WorldMapProjectionScratch* pScratch;
    SVECTOR* pVec;
    WorldMapQuadPrim* pBase;
    WorldMapQuadPrim* pPrim;
    s32 i;
    s32 nOtz;

    pScratch = (WorldMapProjectionScratch*)0x1F800000;
    *(u16*)0x1F800004 = 0;
    pScratch->viewRotation.vx = 0;
    *(u16*)0x1F800002 = g_WorldMapViewRotationY;
    RotMatrixYXZ(&pScratch->viewRotation, (MATRIX*)0x1F800028);
    *(s32*)0x1F800044 = 0;
    *(s32*)0x1F800040 = 0;
    *(s32*)0x1F80003C = 0;
    CompMatrix(&g_WorldMapViewMatrix, (MATRIX*)0x1F800028, (MATRIX*)0x1F800008);
    SetRotMatrix((MATRIX*)0x1F800008);
    SetTransMatrix((MATRIX*)0x1F800008);
    pVec = g_WorldMapQuadVertices;
    for (i = 0; i < WORLD_MAP_QUAD_COUNT; i++, pVec += 4) {
        pBase = (WorldMapQuadPrim*)(g_WorldMapQuadPrims + g_WorldMapPrimPacketIndex * 0x24);
        // the sum has to stay integer arithmetic: `&pBase[i]` flips the addu operands
        pPrim = (WorldMapQuadPrim*)(i * sizeof(WorldMapQuadPrim) + (s32)pBase);
        nOtz = RotTransPers4(pVec, pVec + 1, pVec + 2, pVec + 3,
                             (long*)&pPrim->xy0, (long*)&pPrim->xy1,
                             (long*)&pPrim->xy2, (long*)&pPrim->xy3,
                             &pScratch->interpolated, &pScratch->flag);
        if (pScratch->flag >= 0) {
            addPrim((P_TAG*)((u32*)g_WorldMapRenderContext->pOt[0] + (nOtz >> g_GfxOTDepthShift)), (P_TAG*)pPrim);
        }
    }
}

extern POLY_FT4 g_WorldMapTilePrims[];
extern DR_MODE g_WorldMapTileDrawModes[];

void WorldMapSetupMapPrims(void) {
    POLY_FT4* pPrim;
    RECT tw;
    u16 tpage;
    u16 clut;
    s32 i;

    tpage = GetTPage(0, 1, WORLD_MAP_TEXTURE_VRAM_X, WORLD_MAP_TEXTURE_VRAM_Y);
    clut = GetClut(0x110, WORLD_MAP_CLUT_VRAM_Y);
    pPrim = g_WorldMapTilePrims;
    for (i = 0; i < WORLD_MAP_QUAD_COUNT; i++, pPrim++) {
        setPolyFT4(pPrim);
        pPrim->r0 = 0x30;
        pPrim->g0 = 0x30;
        pPrim->b0 = 0x30;
        pPrim->u0 = 0;
        pPrim->v0 = 0;
        pPrim->u1 = 0xFF;
        pPrim->v1 = 0;
        pPrim->u2 = 0;
        pPrim->v2 = 0x3F;
        pPrim->u3 = 0xFF;
        pPrim->v3 = 0x3F;
        pPrim->tpage = tpage;
        pPrim->clut = clut;
        SetSemiTrans(pPrim, 1);
    }
    tw.x = 0;
    tw.y = 0;
    tw.w = 0x80;
    tw.h = 0;
    func_800453AC(&g_WorldMapTileDrawModes[0], &tw);
    tw.x = 0;
    tw.y = 0;
    tw.w = 0;
    tw.h = 0;
    func_800453AC(&g_WorldMapTileDrawModes[1], &tw);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073B04);

// The two POLY_FT4 primitives at D_8009C5C0; retail names every field store
// by its own address (D_8009C5C3 = tag.len, D_8009C5C7 = tag.code, ...), so the
// fields are declared individually and only the base is typed.
extern POLY_FT4 D_8009C5C0[2];
extern u8 D_8009C5C3;
extern u8 D_8009C5C4;
extern u8 D_8009C5C5;
extern u8 D_8009C5C6;
extern u8 D_8009C5C7;
extern s16 D_8009C5C8;
extern s16 D_8009C5CA;
extern u8 D_8009C5CC;
extern u8 D_8009C5CD;
extern u16 D_8009C5CE;
extern s16 D_8009C5D0;
extern s16 D_8009C5D2;
extern u8 D_8009C5D4;
extern u8 D_8009C5D5;
extern u16 D_8009C5D6;
extern s16 D_8009C5D8;
extern s16 D_8009C5DA;
extern u8 D_8009C5DC;
extern u8 D_8009C5DD;
extern s16 D_8009C5E0;
extern s16 D_8009C5E2;
extern u8 D_8009C5E4;
extern u8 D_8009C5E5;
extern DR_TPAGE D_8009C5A0;
extern TILE D_8009C898[0x40];

/* best: 97.23%. The store order and every per-field symbol are right; only the
   constant register allocation differs. Retail keeps 0x80 in $v0 for the whole
   first block and therefore parks 0xD0 in $t0 and 0x137 in $v1 (then reuses $t0
   for 0x7F and $v1 for 0xFF), while gcc reuses $v0 for 0xD0/0x137 since their
   live ranges do not overlap retail's. Six source orders tried (address order,
   retail store order, colours first, columns first); retail store order is the
   best. The last mismatch is the TILE loop's base: retail forms
   lui/addiu %hi(D_8009C8A6) (the array seen through tile->h) and stores at
   -0xB..0, ours forms D_8009C898+0xE, which links identically but shows as an
   addend difference.
void func_80073E30(void) {
    POLY_FT4* pPrim;
    POLY_G3* pPoly;
    TILE* pTile;
    s32 i;

    pPrim = D_8009C5C0;
    D_8009C5C3 = 9;
    D_8009C5C7 = 0x2C;
    D_8009C5CA = 0x78;
    D_8009C5D2 = 0x78;
    D_8009C5DA = 0xD7;
    D_8009C5E2 = 0xD7;
    D_8009C5C8 = 0xD0;
    D_8009C5D8 = 0xD0;
    D_8009C5D0 = 0x137;
    D_8009C5E0 = 0x137;
    D_8009C5CC = 0;
    D_8009C5CD = 0x80;
    D_8009C5D4 = 0x7F;
    D_8009C5D5 = 0x80;
    D_8009C5DC = 0;
    D_8009C5DD = 0xFF;
    D_8009C5E4 = 0x7F;
    D_8009C5E5 = 0xFF;
    D_8009C5C4 = 0x80;
    D_8009C5C5 = 0x80;
    D_8009C5C6 = 0x80;
    D_8009C5D6 = GetTPage(0, 0, 0x380, 0x100);
    D_8009C5CE = GetClut(0x100, 0x1FE);
    SetSemiTrans(pPrim, 1);
    pPrim[1] = pPrim[0];
    SetDrawTPage(&D_8009C5A0, 1, 0, GetTPage(0, 1, 0x380, 0x100));
    pPoly = (POLY_G3*)D_8009C664;
    for (i = 0; i < 8; i++, pPoly++) {
        setPolyG3(pPoly);
        pPoly->r0 = 0xFF;
        pPoly->g0 = 0x40;
        pPoly->b0 = 0x40;
        pPoly->r1 = 0;
        pPoly->g1 = 0;
        pPoly->b1 = 0;
        pPoly->r2 = 0;
        pPoly->g2 = 0;
        pPoly->b2 = 0;
        SetSemiTrans(pPoly, 1);
    }
    pTile = D_8009C898;
    for (i = 0; i < 0x40; i++, pTile++) {
        setTile(pTile);
        pTile->r0 = 0x80;
        pTile->g0 = 0x80;
        pTile->b0 = 0x10;
        pTile->w = 2;
        pTile->h = 2;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073E30);
// The world map's scratchpad block. RotMatrix writes the rotation vector at 0xB8 and the
// three translation words of the GTE matrix the map is projected through sit at 0x104; the
// 3x3 part of that matrix (0xF0) is addressed separately because it is a raw GTE block.
typedef struct {
    /* 0x000 */ u8 unk0[0xB8];
    /* 0x0B8 */ SVECTOR rotation;
    /* 0x0C0 */ u8 unkC0[0x44];
    /* 0x104 */ s32 matrixTransX;
    /* 0x108 */ s32 matrixTransY;
    /* 0x10C */ s32 matrixTransZ;
} WorldMapScratch; // Size: 0x110

// A projected map triangle, in GPU packet order: colour, screen pair, colour, pair, colour,
// pair. 0x1C bytes each, four per buffer half.
typedef struct {
    /* 0x00 */ u32 tag;
    /* 0x04 */ u8 r0;
    /* 0x05 */ u8 g0;
    /* 0x06 */ u8 b0;
    /* 0x07 */ u8 code;
    /* 0x08 */ s16 x0;
    /* 0x0A */ s16 y0;
    /* 0x0C */ u8 r1;
    /* 0x0D */ u8 g1;
    /* 0x0E */ u8 b1;
    /* 0x0F */ u8 pad1;
    /* 0x10 */ s16 x1;
    /* 0x12 */ s16 y1;
    /* 0x14 */ u8 r2;
    /* 0x15 */ u8 g2;
    /* 0x16 */ u8 b2;
    /* 0x17 */ u8 pad2;
    /* 0x18 */ s16 x2;
    /* 0x1A */ s16 y2;
} WorldMapTriPrim; // Size: 0x1C

// The map-space position of one tile of the current grid.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
} WorldMapTilePos; // Size: 0x4

extern SVECTOR D_8009A340[];
extern WorldMapTriPrim D_8009C664[8];
extern WorldMapTilePos D_8009B6F4[0x20];
extern s32 g_WorldMapFadeLevel;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800740B8);
/* best: 82.80% (1176 bytes vs retail's 1240 - the body is 18 insns SHORT). Every decoded
   fact below is confirmed against the .s; the two things left are both codegen-level.

   What the function is: per frame it builds the map's GTE matrix in the scratchpad (rotY
   from g_WorldMapViewRotationY -> RotMatrix at 0x1F8000F0, then the three translation
   words at 0x1F800104/108/10C), projects 4 triangles of 3 vertices each with RTPT out of
   D_8009A340 into the 8 Gouraud packets at D_8009C664 (0x1C bytes, GPU packet order:
   tag, colour, x0y0, colour, x1y1, colour, x2y2), and then writes the visible tiles'
   screen positions into TILE D_8009C898[0x20 per buffer half], one bit of the 32-bit
   visibility word at g_GameState+0x1B2C per tile, adding the DR_TPAGE D_8009C5A0 first and
   the POLY_FT4 pair D_8009C5C0[packetIndex] last.
   The divisors are 315 for x and 341 for y (0xD00D00D1 / 0x300C0301 signed, 0xA01A01A1 /
   0x80601807 unsigned); offsets 0xCF/0x77 for the three script-set tiles and 0xD0/0x78 for
   the table tiles, which is where the -1 comes from.

   Remaining differences, in order of size:
   1. The three switch arms jump into ONE shared copy of the y division (12 insns saved);
      retail has the whole /341 sequence duplicated per arm. Tried: x0 stored inside the
      arms with a shared y (this version), both stores inside the arms, four separate
      if/else arms, and each arm with its own u16 local - gcc's cross_jump merges the
      identical tails every time. The case-body ORDER is already retail's (0x18, 0x19,
      0x1A with the 0x19 test first), and the case values are 0x18/0x19/0x1A.
   2. addPrim(ot, &D_8009C5A0) and addPrim(ot, pTile) use a base register in retail but
      gcc folds the symbol into `lui $at; sw` for the first store (both with the symbol
      spelled directly and through a DR_TPAGE* local).
   3. Frame is 0x30 where retail has 0x38 (one fewer callee-saved register: retail keeps
      both 0x1F8000F0 in $s5 and the 0x1F800000 base in $s3; mine rematerialises the
      0x1F8000F0 constant into $t8 after the RotMatrix call).
   4. Register-allocation ties: retail's 0xFFFFFF/0xFF000000 pairs are duplicated (t5/a3
      and s2/t0) and my scratchpad base and tile pointers land in different registers.
   The tail-end of loop 2 advances nTileFlags (>>= 1), pTile (0x10) and the packed
   position table pointer (4) while retail keeps TWO position pointers 2 bytes apart;
   that one is cosmetic now that pPos is a pointer local.
*/
/*
void func_800740B8(void) {
    WorldMapScratch* pScratch;
    WorldMapTriPrim* pTri;
    TILE* pTile;
    SVECTOR* pVertex;
    s32* pTarget;
    u16* pPoint18;
    u16* pPoint19;
    u16* pPoint1A;
    WorldMapTilePos* pPos;
    DR_TPAGE* pDrawPage;
    MATRIX* pGteMatrix;
    u32 nTileFlags;
    s32 nTransY;
    u16 y;
    s32 i;

    pScratch = (WorldMapScratch*)0x1F800000;
    pGteMatrix = (MATRIX*)0x1F8000F0;
    // the scroll the camera follows is one x/y/z triple, so both ends come off one base
    pTarget = &g_WorldMapScrollTargetX;
    pTri = &D_8009C664[g_WorldMapPrimPacketIndex * 4];
    pVertex = D_8009A340;
    for (i = 0; i < 4; i++) {
        pScratch->rotation.vy = 0;
        pScratch->rotation.vx = 0;
        pScratch->rotation.vz = g_WorldMapViewRotationY;
        RotMatrix(&pScratch->rotation, pGteMatrix);
        pScratch->matrixTransX = (pTarget[0] >> 12) / 315 + 0x30;
        pScratch->matrixTransZ = g_WorldMapProjectionDistance;
        nTransY = (pTarget[2] >> 12) / 341;
        pScratch->matrixTransY = nTransY + 0x78 - g_WorldMapFadeLevel;
        gte_SetRotTransMatrix_gpr(pGteMatrix);
        gte_ldv3(pVertex, pVertex + 1, pVertex + 2);
        gte_rtpt();
        gte_stsxy3(&pTri->x0, &pTri->x1, &pTri->x2);
        pVertex += 3;
        addPrim(g_WorldMapRenderContext->pOt[0], pTri);
        pTri++;
    }

    nTileFlags = ((u32*)g_GameState.fieldScriptMemory)[0x7F];
    pTile = &D_8009C898[g_WorldMapPrimPacketIndex * 0x20];
    // the three tiles the map script marks are replaced by script-set positions
    pPoint18 = (u16*)&g_GameState.scriptPosX;
    pPoint19 = &g_GameState.mapPosX;
    pPoint1A = (u16*)&g_GameState.unk1838[0xC];
    pPos = D_8009B6F4;
    pDrawPage = &D_8009C5A0;
    addPrim(g_WorldMapRenderContext->pOt[0], pDrawPage);
    for (i = 0; i < 0x20; i++) {
        if (nTileFlags & 1) {
            switch (i) {
            case 0x18:
                pTile->x0 = pPoint18[0] / 315 + 0xCF;
                y = pPoint18[2] / 341 + 0x77;
                break;
            case 0x19:
                pTile->x0 = pPoint19[0] / 315 + 0xCF;
                y = pPoint19[2] / 341 + 0x77;
                break;
            case 0x1A:
                pTile->x0 = pPoint1A[0] / 315 + 0xCF;
                y = pPoint1A[1] / 341 + 0x77;
                break;
            default:
                pTile->x0 = pPos->x + 0xD0;
                y = pPos->y + 0x78;
                break;
            }
            pTile->y0 = y;
            addPrim(g_WorldMapRenderContext->pOt[0], pTile);
        }
        nTileFlags >>= 1;
        pTile++;
        pPos++;
    }
    addPrim(g_WorldMapRenderContext->pOt[0], &D_8009C5C0[g_WorldMapPrimPacketIndex]);
}
*/
extern void* g_WorldMapSlotPositions;
extern void* g_WorldMapSlotPrims;
extern void* g_WorldMapSlotPrimsCopy;
extern s32 g_WorldMapSlotPositionIndex;

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} WorldMapSlot; // Size: 0x8

typedef struct {
    /* 0x0 */ u32 w0;
    /* 0x4 */ u32 w1;
    /* 0x8 */ u32 w2;
    /* 0xC */ u32 w3;
} WorldMapChunk; // Size: 0x10

void WorldMapAllocPrimBuffers(void) {
    WorldMapSlot* pSlot;
    POLY_FT4* pPrim;
    WorldMapChunk* pSrc;
    WorldMapChunk* pDst;
    WorldMapChunk* pEnd;
    s32 i;

    g_WorldMapSlotPositions = HeapAlloc(WORLD_MAP_SLOT_COUNT * sizeof(WorldMapSlot), 0);
    g_WorldMapSlotPrims = HeapAlloc(WORLD_MAP_SLOT_COUNT * sizeof(POLY_FT4), 0);
    g_WorldMapSlotPrimsCopy = HeapAlloc(WORLD_MAP_SLOT_COUNT * sizeof(POLY_FT4), 0);
    pSlot = (WorldMapSlot*)g_WorldMapSlotPositions;
    for (i = WORLD_MAP_SLOT_COUNT - 1; i != -1; i--) {
        pSlot->unk4 = 0;
        pSlot->unk2 = 0;
        pSlot->unk0 = 0;
        pSlot++;
    }
    pPrim = (POLY_FT4*)g_WorldMapSlotPrims;
    for (i = WORLD_MAP_SLOT_COUNT - 1; i != -1; i--) {
        setPolyFT4(pPrim);
        pPrim->r0 = 0x40;
        pPrim->g0 = 0x40;
        pPrim->b0 = 0x48;
        pPrim->u0 = 0x80;
        pPrim->v0 = 0xF0;
        pPrim->u1 = 0x8F;
        pPrim->v1 = 0xF0;
        pPrim->u2 = 0x80;
        pPrim->v2 = 0xFF;
        pPrim->u3 = 0x8F;
        pPrim->v3 = 0xFF;
        pPrim->clut = GetClut(0x120, WORLD_MAP_CLUT_VRAM_Y);
        pPrim->tpage = GetTPage(0, 0, WORLD_MAP_TEXTURE_VRAM_X, WORLD_MAP_TEXTURE_VRAM_Y);
        SetSemiTrans(pPrim, 1);
        pPrim++;
    }
    pSrc = (WorldMapChunk*)g_WorldMapSlotPrims;
    pDst = (WorldMapChunk*)g_WorldMapSlotPrimsCopy;
    // the copy is the whole slot prim buffer, counted in WorldMapChunks
    pEnd = pSrc + WORLD_MAP_SLOT_COUNT * sizeof(POLY_FT4) / sizeof(WorldMapChunk);
    do {
        *pDst++ = *pSrc++;
    } while (pSrc != pEnd);
    g_WorldMapSlotPositionIndex = 0;
}
