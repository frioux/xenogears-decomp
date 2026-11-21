#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/archive.h"
#include "system/controller.h"
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

// one swap buffer's ordering table holds this many entries; drawing starts at the last one
#define WORLD_MAP_OT_SIZE 0x400

// archive entries loaded before the map data itself exists
#define WORLD_MAP_SOUND_ARCHIVE_INDEX 0x24
#define WORLD_MAP_COMPRESSED_MAP_ARCHIVE_INDEX 0x25
#define WORLD_MAP_MENU_RESOURCES_ARCHIVE_INDEX 0x26

// where the tile and slot textures live in VRAM: one texture page, and the CLUT row its palettes sit in
#define WORLD_MAP_TEXTURE_VRAM_X 0x380
#define WORLD_MAP_TEXTURE_VRAM_Y 0x100
#define WORLD_MAP_CLUT_VRAM_Y 0x1FE

// the CLUT column of the tile prims' palettes and of the slot prims' palettes
#define WORLD_MAP_TILE_CLUT_VRAM_X 0x110
#define WORLD_MAP_SLOT_CLUT_VRAM_X 0x120

// WorldMapUpdateLoadQueue's "a load is still in flight, nothing to use yet" result
#define WORLD_MAP_LOAD_QUEUE_PENDING 3

// The frozen screen the fade draws over: the 320x240 picture copied into VRAM as three
// columns (the last one half as wide) and the tpage row they sit at.
#define WORLD_MAP_SCREEN_TPAGE_X 0x2C0
#define WORLD_MAP_SCREEN_TPAGE_Y 0x100
#define WORLD_MAP_SCREEN_COLUMN_COUNT 3
#define WORLD_MAP_SCREEN_COLUMN_WIDTH 0x80
#define WORLD_MAP_SCREEN_COLUMN_WIDTH_LAST 0x40

// the fade quad covers the whole frame buffer, not the 216-line draw area the envs set up
#define WORLD_MAP_BUFFER_HEIGHT 0xF0

// g_WorldMapAreaId while no map area is loaded

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, int arg1, int arg2);
extern void WorldMapGetGameStatePosition(void* pVector);

// The stream request table at g_WorldMapStreamQueue: g_WorldMapStreamQueue and the D_ symbols
// that follow are its entries, one object each, NOT g_WorldMapStreamQueue[n]. Round 359 measured the
// indexed spelling on both WorldMapQueue*FileLoads bodies and it changes the instruction stream:
// one `StreamDataQueueEntry[]` object makes gcc materialise the base into $s0 in the prologue
// (WorldMapQueueFileLoads 344 bytes vs retail's 340, frame 0x20 vs 0x18, and `move a0,s0` at the
// ArchiveStartStreamQueue call), and the entry-0 store then folds into the `la` gcc emits for the
// queue argument (WorldMapQueueTwoFileLoads 160 bytes vs retail's 164). Retail reaches every entry
// field through its own %hi/%lo absolute symbol, which is what the per-entry objects reproduce.
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
extern void* g_WorldMapPartyActorAnimFile0[3];
extern void* g_WorldMapActorAnimFiles[3];
extern s32 D_8009C170;

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ u16 id;
    /* 0x4 */ s16 z;
    /* 0x6 */ u16 pad6;
} WorldMapPoint; // Size: 0x8

extern WorldMapPoint* g_WorldMapPoints;

// One of the two swap buffers at g_WorldMapGfxEnvs: the draw and disp env a frame is
// staged through, and the two ordering table heads it fills.
typedef struct {
    /* 0x00 */ DRAWENV drawEnv; // Size: 0x5C
    /* 0x5C */ DISPENV dispEnv; // Size: 0x14
    /* 0x70 */ u_long* pOt[2];
} WorldMapGfxEnv; // Size: 0x78

// The fog colour SetFarColor takes; func_800931D8 multiplies the texture colours by
// the same three factors.
typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
} WorldMapColorFactors; // Size: 0x3

extern s32 g_WorldMapPrimPacketIndex;
extern s32 g_WorldMapLoopActive;
extern WorldMapGfxEnv* g_WorldMapRenderContext;
extern WorldMapGfxEnv g_WorldMapGfxEnvs[2];
extern s32 g_WorldMapScrollTargetX;
extern u8 g_FieldPlayerWalkmeshMaterial;
extern u8 g_MenuDebugEnabled;
extern u16 g_WorldMapPad1Held;
extern u16 g_WorldMapPad1Pressed;
extern u16 g_WorldMapPad1Released;
extern u16 g_WorldMapPad2Pressed;
extern u16 g_WorldMapPad2Released;
extern u16 g_WorldMapPad2Held;
extern u_char g_WorldMapCdSyncResult;
extern u8 g_WorldMapMode4AreaRect[];
extern s32 g_WorldMapL2R2PressEdge;
extern s32 g_WorldMapFadeActive;
extern s32 g_WorldMapMode4EntryTriggered;
extern s32 g_WorldMapExpiredTimerCount;
extern s32 g_WorldMapMode;
extern s32 g_WorldMapFadeLevel;
extern s16 g_WorldMapAreaId;
extern s16 g_WorldMapMode4AreaId;
extern s32 g_WorldMapAreaMode;
extern void* g_WorldMapAreaRect;
extern u8 g_SoundEffectBankIndex;
extern u8 g_MenuOverlayIndex;
extern u8 g_ShopIndex;

extern int ControllerPopState(void);
extern int ControllerGetType(int controllerIndex);
extern s16 WorldMapGetTileHalfIndex(void* p);
extern s32 func_80075E7C(void* p, s32 arg1);
extern s32 WorldMapUpdateLoadQueue(void);
extern void WorldMapWaitForQueuedLoads(void);
extern void WorldMapUpdateSlots(void);
extern void WorldMapEnterMenuMode(void);
extern void WorldMapExitMenuMode(void);
extern void WorldMapFlushDrawAndCache(void);
extern void WorldMapWaitForButtonRelease(void);
extern void WorldMapWaitForControllerConnected(void);
extern void WorldMapUpdateTileAnimEntries(void);
extern void WorldMapUpdateFrameAnimEntries(void);
extern void MenuMain(void);
extern void GfxSetRenderContext(int context);
extern void GfxFlushImageList(void);
extern void SpriteAnimFrameListFlush(void);
extern void GameCheckAndHandleSoftReset(void);

// The world map's frame loop: each pass reads the pads, drains the streaming load queue,
// flips the swap buffer and draws a frame, until something sets g_WorldMapLoopActive to 0.
void WorldMapMainLoop(void) {
    WorldMapGfxEnv* pEnv;
    RECT rect;
    s32 i;

    g_WorldMapRenderContext = &g_WorldMapGfxEnvs[1];
    g_WorldMapPrimPacketIndex = 1;
    g_WorldMapLoopActive = 1;
    do {
        // the pads are rebuilt from the controller states pushed since the last frame
        g_WorldMapPad2Pressed = 0;
        g_WorldMapPad2Released = 0;
        g_WorldMapPad2Held = 0;
        g_WorldMapPad1Pressed = 0;
        g_WorldMapPad1Released = 0;
        g_WorldMapPad1Held = 0;
        while (ControllerPopState() != 0) {
            g_WorldMapPad1Held |= g_C1ButtonState;
            g_WorldMapPad2Held |= g_C2ButtonState;
            g_WorldMapPad1Released |= g_C1ButtonStateReleased;
            g_WorldMapPad2Released |= g_C2ButtonStateReleased;
            g_WorldMapPad1Pressed |= g_C1ButtonStatePressedOnce;
            g_WorldMapPad2Pressed |= g_C2ButtonStatePressedOnce;
        }
        // hold the frame until a load lands
        while (WorldMapUpdateLoadQueue() == WORLD_MAP_LOAD_QUEUE_PENDING) {
            Vsync(0);
        }
        CdSync(1, &g_WorldMapCdSyncResult);
        pEnv = g_WorldMapGfxEnvs;
        if (g_WorldMapRenderContext == pEnv) {
            pEnv++;
        }
        g_WorldMapRenderContext = pEnv;
        g_WorldMapPrimPacketIndex = (g_WorldMapPrimPacketIndex == 0);
        ClearOTagR(pEnv->pOt[0], WORLD_MAP_OT_SIZE);
        GfxSetRenderContext(g_WorldMapPrimPacketIndex);
        SpriteAnimFrameListFlush();
        WorldMapUpdateSlots();
        DrawSync(0);
        Vsync(2);
        GameCheckAndHandleSoftReset();
        PutDispEnv(&g_WorldMapRenderContext->dispEnv);
        PutDrawEnv(&g_WorldMapRenderContext->drawEnv);
        if (g_FieldPlayerWalkmeshMaterial == 0 && g_WorldMapL2R2PressEdge != 0 && g_WorldMapFadeActive == 0
            && g_WorldMapMode4EntryTriggered == 0 && g_WorldMapAreaId == WORLD_MAP_AREA_NONE
            && g_WorldMapMode4AreaId == g_WorldMapAreaId && g_WorldMapLoopActive != 0
            && g_WorldMapExpiredTimerCount == 0) {
            // L2+R2 leaves mode 4's map for the free-roaming map, but only from inside a
            // map whose player tile is a road end (half index 4 is the other kind)
            g_WorldMapL2R2PressEdge = 0;
            if (WorldMapGetTileHalfIndex(&g_WorldMapScrollTargetX) != 4) {
                for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                    g_GameState.savedGearRide[i] = g_GameState.gearRide[i];
                }
                if (g_GameState.gearRide[0] != 0) {
                    g_GameState.gearRide[2] = 0;
                    g_GameState.gearRide[1] = 0;
                    g_GameState.gearRide[0] = 0;
                } else {
                    // a party slot rides its gear when that character has one equipped
                    if (g_GameState.characters[g_GameState.partyMembers[0]].gearId != CHARACTER_ID_NONE) {
                        g_GameState.gearRide[0] = 1;
                    }
                    if (g_GameState.characters[g_GameState.partyMembers[1]].gearId != CHARACTER_ID_NONE) {
                        g_GameState.gearRide[1] = 1;
                    }
                    if (g_GameState.characters[g_GameState.partyMembers[2]].gearId != CHARACTER_ID_NONE) {
                        g_GameState.gearRide[2] = 1;
                    }
                }
                WorldMapSyncGearRideActors();
            }
        } else {
            g_WorldMapL2R2PressEdge = 0;
        }
        if (g_WorldMapFadeActive == 0 && g_WorldMapMode4EntryTriggered == 0 && g_WorldMapLoopActive != 0
            && g_WorldMapExpiredTimerCount == 0 && (g_WorldMapPad1Released & CTRL_BTN_START) != 0) {
            WorldMapWaitForButtonRelease();
        }
        if (g_WorldMapFadeActive == 0 && g_WorldMapMode4EntryTriggered == 0 && g_WorldMapLoopActive != 0
            && g_WorldMapExpiredTimerCount == 0 && ControllerGetType(0) == 0) {
            WorldMapWaitForControllerConnected();
        }
        if (g_WorldMapFadeActive == 0 && g_WorldMapMode4EntryTriggered == 0
            && g_WorldMapAreaId == WORLD_MAP_AREA_NONE
            && g_WorldMapMode4AreaId == g_WorldMapAreaId && g_WorldMapLoopActive != 0
            && g_WorldMapExpiredTimerCount != 0
            && func_80075E7C(&g_WorldMapScrollTargetX, g_GameState.fieldScriptMemory[0]) == 1) {
            // a mode 4 map's script ended: hand its party state and gear ride back to the
            // free-roaming map
            g_WorldMapLoopActive = 0;
            g_WorldMapAreaMode = 1;
            g_SoundEffectBankIndex = 0;
            g_GameState.savedGearRide[0] = g_GameState.gearRide[0];
            g_GameState.savedGearRide[1] = g_GameState.gearRide[1];
            g_GameState.savedGearRide[2] = g_GameState.gearRide[2];
        }
        g_WorldMapExpiredTimerCount = 0;
        if ((g_WorldMapPad1Released & CTRL_BTN_SELECT) != 0) {
            g_GameState.hideTileLayer ^= 1;
        }
        if (g_WorldMapFadeActive == 0 && g_WorldMapMode4EntryTriggered != 0 && g_WorldMapLoopActive != 0) {
            if (g_WorldMapMode > 0) {
                if (g_WorldMapMode < 4) {
                    WorldMapEnterMenuMode();
                    g_MenuOverlayIndex = 0;
                    g_MenuDebugEnabled = 0;
                    g_ShopIndex = 1;
                    WorldMapFlushDrawAndCache();
                    MenuMain();
                    WorldMapFlushDrawAndCache();
                    WorldMapExitMenuMode();
                } else if (g_WorldMapMode < 8) {
                    g_WorldMapLoopActive = 0;
                    g_WorldMapAreaMode = 0;
                    g_WorldMapAreaRect = g_WorldMapMode4AreaRect;
                    g_GameState.worldMapState |= 0x2000;
                }
            }
        } else {
            g_WorldMapMode4EntryTriggered = 0;
        }
        GfxFlushImageList();
        WorldMapUpdateTileAnimEntries();
        WorldMapUpdateFrameAnimEntries();
        SetGeomOffset(WORLD_MAP_SCREEN_WIDTH / 2, g_WorldMapFadeLevel);
        DrawOTag(g_WorldMapRenderContext->pOt[0] + (WORLD_MAP_OT_SIZE - 1));
    } while (g_WorldMapLoopActive != 0);
    ResetGraph(1);
    if (g_WorldMapPrimPacketIndex == 0) {
        // clear the frame buffer's second page before the map hands the screen back
        rect.x = 0;
        rect.y = WORLD_MAP_SCREEN_HEIGHT;
        rect.w = WORLD_MAP_SCREEN_WIDTH;
        rect.h = WORLD_MAP_SCREEN_HEIGHT;
        MoveImage(&rect, 0, 0);
    }
    WorldMapWaitForQueuedLoads();
    DrawSync(0);
    Vsync(0);
    PutDispEnv(&g_WorldMapGfxEnvs[1].dispEnv);
}

int WorldMapModeInitNop(void) {
    return 1;
}

extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapPathFrame;
extern s16 g_WorldMapSlotUpdateFlags;
extern s32 g_WorldMapScrollX;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapGridAngle;

extern void WorldMapBuildRotationMatrix(s32* p);
extern void func_80097244(s32* p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_80085CDC(void);
extern void func_8008615C(void);
extern void func_800747DC(void);
extern void func_800848F4(void);
extern void WorldMapUpdateSlotGridOrigin(s32* p);
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
    WorldMapUpdateSlotGridOrigin(&g_WorldMapScrollX);
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
    if (g_GameState.hideTileLayer == 0) {
        func_800740B8();
    }
    return 1;
}
typedef struct {
    s16 fileIndex;
    s16 tileColumns;
    s16 tileRows;
    s16 areaMode;
} WorldMapTableEntry; // Size: 0x8

extern u16 g_WorldMapLevelThresholds[];
extern u16 D_8009B566;
extern WorldMapTableEntry D_8009B57C;
extern WorldMapTableEntry D_8009B58C;
extern s32 g_WorldMapLevelIndex;
extern s32 g_WorldMapWdsFileIndex;
extern s32 g_WorldMapSoundBankFileIndex;
extern s32 g_WorldMapMode7SoundBankFileIndex;
extern s32 g_WorldMapGridImageFileIndex;
extern s32 g_WorldMapExtraFileIndex;
extern s32 g_WorldMapTileRows;
extern s32 g_WorldMapTileColumns;
extern s32 g_WorldMapGridColumnImageFileIndex;
extern s32 g_WorldMapAreaMode;

void WorldMapSetLevelFileIndices(s32 arg0, s32 arg1) {
    WorldMapTableEntry* pEntry;
    u16* pTable;
    s32 nLevel;

    if (arg0 < 8) {
        nLevel = 1;
        pTable = g_WorldMapLevelThresholds;
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
    D_8009D3C4 = pEntry->fileIndex + 1;
    D_8009C17C = pEntry->fileIndex + 2;
    g_WorldMapReloadFileIndex = pEntry->fileIndex + 3;
    g_WorldMapWdsFileIndex = pEntry->fileIndex + 4;
    g_WorldMapSoundBankFileIndex = pEntry->fileIndex + 5;
    g_WorldMapSedsBankFileIndex = pEntry->fileIndex + 6;
    g_WorldMapMode7SoundBankFileIndex = pEntry->fileIndex + 7;
    g_WorldMapExtraFileIndex = pEntry->fileIndex + 8;
    g_WorldMapGridImageFileIndex = pEntry->fileIndex + 9;
    g_WorldMapGridColumnImageFileIndex = pEntry->fileIndex + 0xA;
    g_WorldMapTileColumns = pEntry->tileColumns;
    g_WorldMapTileRows = pEntry->tileRows;
    g_WorldMapAreaMode = pEntry->areaMode;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071CDC);
// ROUND 374: re-measured 84.51% (508 bytes). The array-extern lever that closed
// WorldMapSetLevelFileIndices does not apply here: the queue entries are separate symbols by design.
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
   Round 289: the two-cursor shape is NOT gcc splitting the givs of one entry
   pointer. Writing loop 2 as `pEntry->archiveIndex` / `pEntry->pData` with
   `pEntry++` (the pointer form the induction-base rule recommends) measures
   84.53%, 512 bytes: the compile still keeps ONE base register with a
   `sw v0,4(a0)` displacement, and the loop then also carries an `andi 0xff`
   retail does not have. Retail's `la a1,D_8009D3FC` + `addiu a2,a1,-4` says the
   source named D_8009D3FC itself, so the two cursors are two source pointers
   (one per field), not one pointer plus displacements.
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
    ArchiveStartStreamQueue(pQueue, 0, 0);
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071EF0);
// ROUND 374: re-measured 97.37% (256 vs retail's 252). THE ARRAY SPELLING CLOSES IT:
// `extern StreamDataQueueEntry g_WorldMapStreamQueue[];` + `pQueue = g_WorldMapStreamQueue;`
// gives retail's exact 252 bytes and 99.21-99.37%, with `sh $v1,0($a0)` as retail has it and
// only the pData store left as g_WorldMapStreamQueue+4 against retail's D_8009D3FC (same
// address, links identical - round-359/374 RELOC case). It is not applied because the same
// declaration makes the matched neighbours WorldMapQueueTwoFileLoads (160 bytes, 91.95%) and
// WorldMapQueueFileLoads (344, 87.81%) materialise the queue base in a register where retail
// stores the symbol through $at. Closing this needs a SECOND NAME for 0x8009D3F8 (e.g.
// g_WorldMapStreamQueueEntries) so only this function sees the array type. Round-374
// cc1_probe: every scalar spelling of &g_WorldMapStreamQueue (plain, (void*), (u32), +0,
// &((Q*)&g_Q)[0], (u8*)) folds to the same %at macro; only array decay escapes the fold.
/* best: 98.00% (256 bytes vs 252). Only the g_WorldMapStreamQueue store differs: the
   original reuses the $a0 that already holds the address for the ArchiveStartStreamQueue
   call and emits a bare "sh $v1,0($a0)"; gcc re-forms the symbol address and
   needs one extra lui $at. A local StreamDataQueueEntry* does not stop that
   folding, and neither does routing the address through an s32 local or a
   volatile pointer: cse replaces the store's (mem (symbol_ref)) with %hi/%lo
   regardless, because the address pseudo is created after the store. Dead end
   unless the address comes from a runtime base.
   Round 51: permuter 434 iterations, base=125 best=125 - no lever in the body.
   Round 287: a volatile store (`*(volatile u16 *)&g_WorldMapStreamQueue = D_8009D3C4;`)
   is worse, not better: 89.00%, 256 bytes - gcc then keeps the address in a callee-saved
   $s0 across the store group and STILL emits `lui $at / sh %lo($at)` through %lo, so the
   store itself never picks up the register.
   Round 359: the `g_WorldMapStreamQueue[n]` spelling does not close it either - un-parked with an
   `extern StreamDataQueueEntry g_WorldMapStreamQueue[]` it reaches retail's 252 bytes but measures
   93.63%, because the array base is then materialised in a general register
   (`addiu $v1,$v1,24` / `addiu $a0,$v1,-24`) and a sixth callee-saved temporary (t2) appears.
   The body below re-measures 97.37% at 256 bytes against retail's 252.

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
    ArchiveStartStreamQueue(pQueue, 0, 0);
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
    ArchiveStartStreamQueue(&g_WorldMapStreamQueue, 0, 0);
}

extern s32 g_WorldMapAudioManagerLoaded;
extern s32 g_WorldMapWdsFileIndex;
extern s32 g_WorldMapSoundBankFileIndex;
extern s32 g_WorldMapMode7SoundBankFileIndex;
extern s32 g_WorldMapExtraFileIndex;
extern void* g_WorldMapWdsFileBuffer;
extern void* g_WorldMapSoundBankBuffer;
extern void* g_WorldMapMode7SoundBankBuffer;
extern void* g_WorldMapExtraFileBuffer;

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
    D_8009D418.archiveIndex = g_WorldMapExtraFileIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_WorldMapExtraFileIndex), 0);
    g_WorldMapExtraFileBuffer = pBuffer;
    D_8009D418.pData = pBuffer;
    D_8009D420.archiveIndex = 0;
    D_8009D420.pData = 0;
    ArchiveStartStreamQueue(&g_WorldMapStreamQueue, 0, 0);
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
extern s32 g_WorldMapFadeActive;
extern s32 g_WorldMapInitModeIndex;
extern s32 g_WorldMapAreaIndex;
extern int g_AnimScriptExtraTicks;
extern s16 g_SoundControlFlags;
extern u8 g_SoundBankLoadBuffer[];
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
extern void WorldMapSetPlayerPositionFromPoint(s32 areaIndex);
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
extern void WorldMapSetGridPosition(void* pPosition);
extern s32 WorldMapUpdateLoadQueue(void);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void WorldMapRefreshGrid(void* pTarget);
extern void WorldMapWaitForQueuedLoads(void);
extern void SoundAddSedsEntry(void* pFile);
extern void* SoundCreateAudioManagerOnHeap(void* pFile);
extern void SoundPlayAudioManager(void* pManager, s32 volume, s32 arg2);
extern void SoundReloadAudioManagerSamples(void* pManager, s32 volume, s32 arg2);
extern void WorldMapInitSpriteGroupSprites(s32 arg0, s32 arg1, s32 arg2);
extern void func_800978FC(void);
extern void WorldMapInitPrimTemplateBuffers(void);
extern void WorldMapAllocSemiTransPrimBuffers(void);
extern void WorldMapAllocSpritePrimBuffers(void);
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
extern s32 WorldMapHideSpriteSetGroups();

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072238);
/* best: 99.81% (1892 bytes = retail's exact size). Instruction-identical except the two
   `= 1` stores: retail hoists `li v1,1` above `li v0,2` and emits both `sw v1` AFTER the five
   zero stores, where this body emits them (in v0) right after the DrawMode store. Written in
   retail's own store order (ones after the zeros) cc1 instead moves the two stores ABOVE the
   zeros and scores 98.88% - so the statement order is not the lever (11 orders tried, all
   98.7-99.8).
   cc1_probe with this unit's flags shows the mechanism: with two zero stores, or with no
   trailing `la` statement, cc1 hoists only the `li` and leaves the stores in place (retail's
   shape); with five zeros plus the callback assignment last it hoists the two stores too.
   permuter (8 min, 2989 iters): PERMUTER_BETTER 80 to 70 by chaining
   `g_WorldMapMode4EntryTriggered = (g_WorldMapL2R2Pressed = 0);` - REJECTED, it reverses those
   two zero stores relative to retail.
   Kept from an earlier permuter run: `pLoadBuffer = g_SoundBankLoadBuffer;` at the head of the
   0-branch, used as memcpy's dst (memcpy otherwise keeps &g_SoundBankLoadBuffer in a saved
   register where retail rematerialises the address into a0).
   This function owns jtbl_8006FAF4 (6 words), so an un-park must land check_bin.sh world_map. */
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
    g_WorldMapFadeActive = 1;
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
        WorldMapSetPlayerPositionFromPoint(g_WorldMapAreaIndex);
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
        WorldMapSetGridPosition(&g_WorldMapPlayerPosition);
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
            WorldMapSetSlotEnterHandler(WorldMapHideSpriteSetGroups, 0x13);
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
        WorldMapInitSpriteGroupSprites(0xE, 0, 0);
        g_FieldPlayerWalkmeshMaterial = 1;
    }
    func_800978FC();
    WorldMapInitPrimTemplateBuffers();
    WorldMapAllocSemiTransPrimBuffers();
    WorldMapAllocSpritePrimBuffers();
    if (g_WorldMapInitModeIndex == 0) {
        WorldMapInitRandomTimers();
    }
    SystemTransferPaletteToVRAM(0x130, 0x1E0);
}
*/
extern u16 g_NextMapFlags[];
extern void* g_WorldMapTilePrimBuffer;
extern void* g_WorldMapTilePrimBufferCopy;
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

// The 0x80-byte slot table at g_WorldMapSlots; misc10.c declares it in full as
// WorldMapActor. This unit only reaches the sprite pointer.
typedef struct {
    /* 0x00 */ u8 unk0[0x4C];
    /* 0x4C */ void* pSprite;
    /* 0x50 */ u8 unk50[0x30];
} WorldMapSlotActor; // Size: 0x80

extern WorldMapSlotActor* g_WorldMapSlots;
void WorldMapFreeAreaResources(void) {
    s32 i;

    if (g_WorldMapAreaMode == 0) {
        SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0xF0);
    }
    SoundStopAllElements();
    SoundRemoveSedsEntry(g_MenuSedsFile);
    HeapFree(g_MenuSedsFile);
    for (i = 0; i < 0x40; i++) {
        if (g_WorldMapSlots[i].pSprite != NULL) {
            SpriteDestroy(g_WorldMapSlots[i].pSprite);
            g_WorldMapSlots[i].pSprite = NULL;
        }
    }
    if (g_WorldMapAreaMode == 1) {
        WorldMapSaveState();
        g_NextMapFlags[0] |= 0x8000;
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

extern WorldMapColorFactors g_WorldMapColorFactors;
extern s32 g_WorldMapProjectionDistance;
extern s32 g_WorldMapAreaMode;

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
    g_WorldMapGfxEnvs[1].drawEnv.isbg = 1;
    g_WorldMapGfxEnvs[0].drawEnv.isbg = 1;
    g_WorldMapGfxEnvs[1].drawEnv.dtd = 1;
    g_WorldMapGfxEnvs[0].drawEnv.dtd = 1;
    if (g_WorldMapAreaMode == 2) {
        g_WorldMapGfxEnvs[0].drawEnv.r0 = 0;
        g_WorldMapGfxEnvs[0].drawEnv.g0 = 0;
        g_WorldMapGfxEnvs[0].drawEnv.b0 = 0;
        g_WorldMapGfxEnvs[1].drawEnv.r0 = 0;
        g_WorldMapGfxEnvs[1].drawEnv.g0 = 0;
        g_WorldMapGfxEnvs[1].drawEnv.b0 = 0;
    } else {
        g_WorldMapGfxEnvs[0].drawEnv.r0 = 0;
        g_WorldMapGfxEnvs[0].drawEnv.g0 = 0;
        g_WorldMapGfxEnvs[0].drawEnv.b0 = 0x70;
        g_WorldMapGfxEnvs[1].drawEnv.r0 = 0;
        g_WorldMapGfxEnvs[1].drawEnv.g0 = 0;
        g_WorldMapGfxEnvs[1].drawEnv.b0 = 0x70;
    }
    g_WorldMapGfxEnvs[1].dispEnv.screen.y = 10;
    g_WorldMapGfxEnvs[0].dispEnv.screen.y = 10;
    g_WorldMapGfxEnvs[1].dispEnv.screen.w = 256;
    g_WorldMapGfxEnvs[0].dispEnv.screen.w = 256;
    g_WorldMapGfxEnvs[1].dispEnv.screen.x = 0;
    g_WorldMapGfxEnvs[0].dispEnv.screen.x = 0;
    g_WorldMapGfxEnvs[1].dispEnv.screen.h = WORLD_MAP_SCREEN_HEIGHT;
    g_WorldMapGfxEnvs[0].dispEnv.screen.h = WORLD_MAP_SCREEN_HEIGHT;
    SetNearColor(0x80, 0x80, 0x80);
    SetBackColor(0x80, 0x80, 0x80);
    SetFarColor(g_WorldMapColorFactors.r, g_WorldMapColorFactors.g, g_WorldMapColorFactors.b);
    nNear = 0x800;
    if (g_WorldMapAreaMode == 2) {
        nNear = 0xB00;
    }
    SetFogNearFar(nNear, 0xE80, g_WorldMapProjectionDistance);
}

extern void SetSemiTrans(void* p, s32 abe);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072DB4);
// ROUND 374: re-measured 96.65%, 1352 bytes against retail's 1356 - one instruction SHORT,
// so this body can never link. The missing instruction is retail's `move s2,s5`
// (nColor = color): this cc1 coalesces the copy away because color dies at it, while retail
// keeps both. Retail also folds `i = count - 1` into the parameter register
// (`addiu s7,s7,-1`) where every spelling here allocates a new one. permute.sh refuses this
// body (the struct assignment `pG4s[1] = pG4s[0];` is outside its C parser).
/* best: 96.65%, 1352 bytes vs retail's 1356 (one instruction short), 338 vs 339 insns.
   Everything outside the two blocks below is instruction-for-instruction identical:
   the three HeapAllocs, the FT4 init loop (setPolyFT4/setRGB0/setShadeTex IN THAT ORDER -
   putting setShadeTex before setRGB0 lets cse fold `code = 0x2c | 1` into one store and
   loses the lbu/ori pair; the element pointer has to be a local (`pFt4 = &pFt4s[i]`, NOT
   `pFt4s[i]` inline, which also folds), the POLY_G4 setup, the struct copy pG4s[1]=pG4s[0],
   SetDrawTPage (which belongs BEFORE the G4 block), the two PutDispEnv/PutDrawEnv pairs,
   the pEnv toggle written as `pEnv = (pEnv == g_WorldMapGfxEnvs) ? pEnv + 1 :
   g_WorldMapGfxEnvs;` (the if/else form costs a `j`), the whole draw loop body, and the
   three HeapFrees.
   LEFT: (1) the frame is 96 where retail is 88 - gcc reserves 8 bytes of var space that no
   instruction touches (all four slots used are 16/24/32/40(sp), retail's too);
   (2) the FT4 geometry stores are the right SET in the right per-prim order, but the
   value-groups come out in a different order: retail emits the 0x100 group first, then the
   0x80 group, then the rest in source order, and this compile emits 0xEF, 0x80, 0x100, rest.
   The four constants land in the same registers (v0=0x100, v1/v0=0x80, t0/v1=0xEF,
   s0=0x140) so this is sched2's group order, not a register-assignment difference; the
   x/y values are written with setXYWH + setUV4 per prim (retail's u3 of the third column is
   0x80 while its u1 is 0x40, so setUVWH does not fit);
   (3) the loop counter and nPrim are swapped (retail i=s7/nPrim=s6, here i=s6/nPrim=s7) and
   the 0xFF000000/0x00FFFFFF masks are in s5/s2 instead of s5/s3;
   (4) one instruction: retail's head test is `addiu i,i,-1; li v0,-1; beq i,v0` while every
   plain `for (i = count - 1; i != -1; i--)` folds to `beqz count; addiu i,count,-1` (loop.c
   counts the iterations). The body below avoids the fold by comparing against a local
   `nEnd = -1` fed to the for's condition, which reproduces the three-instruction test; the
   price is that nEnd needs its own register (t1 here, v0 in retail).
   Also worth knowing: `do { } while (i != -1)` emits no entry test at all (1348), and
   `while (1) { if (i == -1) break; ... }` reaches retail's exact 1356 but puts the test
   after the masked-address stores instead of before them. */
/*
void func_80072DB4(s32 count, s32 color, s32 colorStep, s32 abr) {
    POLY_FT4* pFt4s;
    POLY_FT4* pFt4;
    POLY_G4* pG4s;
    POLY_G4* pG4;
    DR_TPAGE* pTpage;
    DISPENV* pDispEnv;
    WorldMapGfxEnv* pEnv;
    s32 nPrim;
    s32 i;
    s32 nColor;
    s32 nEnd;

    pFt4s = (POLY_FT4*)HeapAlloc(WORLD_MAP_SCREEN_COLUMN_COUNT * sizeof(POLY_FT4), 1);
    pG4s = (POLY_G4*)HeapAlloc(2 * sizeof(POLY_G4), 1);
    pTpage = (DR_TPAGE*)HeapAlloc(sizeof(DR_TPAGE), 1);
    for (i = 0; i < WORLD_MAP_SCREEN_COLUMN_COUNT; i++) {
        pFt4 = &pFt4s[i];
        setPolyFT4(pFt4);
        setRGB0(pFt4, 0x80, 0x80, 0x80);
        setShadeTex(pFt4, 1);
    }
    setXYWH(&pFt4s[0], 0, 0, WORLD_MAP_SCREEN_COLUMN_WIDTH, WORLD_MAP_BUFFER_HEIGHT - 1);
    setXYWH(&pFt4s[1], WORLD_MAP_SCREEN_COLUMN_WIDTH, 0, WORLD_MAP_SCREEN_COLUMN_WIDTH,
            WORLD_MAP_BUFFER_HEIGHT - 1);
    setXYWH(&pFt4s[2], WORLD_MAP_SCREEN_COLUMN_WIDTH * 2, 0, WORLD_MAP_SCREEN_COLUMN_WIDTH_LAST,
            WORLD_MAP_BUFFER_HEIGHT - 1);
    setUV4(&pFt4s[0], 0, 0, WORLD_MAP_SCREEN_COLUMN_WIDTH, 0, 0, WORLD_MAP_BUFFER_HEIGHT - 1,
           WORLD_MAP_SCREEN_COLUMN_WIDTH, WORLD_MAP_BUFFER_HEIGHT - 1);
    setUV4(&pFt4s[1], 0, 0, WORLD_MAP_SCREEN_COLUMN_WIDTH, 0, 0, WORLD_MAP_BUFFER_HEIGHT - 1,
           WORLD_MAP_SCREEN_COLUMN_WIDTH, WORLD_MAP_BUFFER_HEIGHT - 1);
    setUV4(&pFt4s[2], 0, 0, WORLD_MAP_SCREEN_COLUMN_WIDTH_LAST, 0, 0, WORLD_MAP_BUFFER_HEIGHT - 1,
           WORLD_MAP_SCREEN_COLUMN_WIDTH, WORLD_MAP_BUFFER_HEIGHT - 1);
    pFt4s[0].tpage = GetTPage(2, 0, WORLD_MAP_SCREEN_TPAGE_X, WORLD_MAP_SCREEN_TPAGE_Y);
    pFt4s[1].tpage = GetTPage(2, 0, WORLD_MAP_SCREEN_TPAGE_X + WORLD_MAP_SCREEN_COLUMN_WIDTH,
                             WORLD_MAP_SCREEN_TPAGE_Y);
    pFt4s[2].tpage = GetTPage(2, 0, WORLD_MAP_SCREEN_TPAGE_X + WORLD_MAP_SCREEN_COLUMN_WIDTH * 2,
                             WORLD_MAP_SCREEN_TPAGE_Y);
    SetDrawTPage(pTpage, 0, 1, GetTPage(0, abr, 0, 0));
    setPolyG4(&pG4s[0]);
    setXYWH(&pG4s[0], 0, 0, WORLD_MAP_SCREEN_WIDTH, WORLD_MAP_BUFFER_HEIGHT);
    setRGB0(&pG4s[0], 0, 0, 0);
    setRGB1(&pG4s[0], 0, 0, 0);
    setRGB2(&pG4s[0], 0, 0, 0);
    setRGB3(&pG4s[0], 0, 0, 0);
    SetSemiTrans(&pG4s[0], 1);
    setShadeTex(&pG4s[0], 1);
    pG4s[1] = pG4s[0];
    DrawSync(0);
    Vsync(0);
    pDispEnv = &g_WorldMapGfxEnvs[1].dispEnv;
    PutDispEnv(pDispEnv);
    PutDrawEnv((DRAWENV*)((u8*)pDispEnv - 0x5C));
    pEnv = g_WorldMapGfxEnvs;
    nPrim = 0;
    nColor = color;
    nEnd = -1;
    for (i = count - 1; i != nEnd; i--) {
        pEnv = (pEnv == g_WorldMapGfxEnvs) ? pEnv + 1 : g_WorldMapGfxEnvs;
        ClearOTagR(pEnv->pOt[0], WORLD_MAP_OT_SIZE);
        addPrim(&pEnv->pOt[0][1], &pFt4s[0]);
        addPrim(&pEnv->pOt[0][1], &pFt4s[1]);
        addPrim(&pEnv->pOt[0][1], &pFt4s[2]);
        nPrim ^= 1;
        // the sum has to stay integer arithmetic: `&pG4s[nPrim]` flips the addu operands
        pG4 = (POLY_G4*)(nPrim * sizeof(POLY_G4) + (s32)pG4s);
        setRGB0(pG4, nColor, nColor, nColor);
        setRGB1(pG4, nColor, nColor, nColor);
        setRGB2(pG4, nColor, nColor, nColor);
        setRGB3(pG4, nColor, nColor, nColor);
        addPrim(&pEnv->pOt[0][0], pG4);
        addPrim(&pEnv->pOt[0][0], pTpage);
        DrawSync(0);
        Vsync(0);
        PutDispEnv(&pEnv->dispEnv);
        PutDrawEnv(&pEnv->drawEnv);
            nColor += colorStep;
            DrawOTag(&pEnv->pOt[0][WORLD_MAP_OT_SIZE - 1]);
    }
    DrawSync(0);
    Vsync(0);
    PutDispEnv(&g_WorldMapGfxEnvs[1].dispEnv);
    HeapFree(pFt4s);
    HeapFree(pG4s);
    HeapFree(pTpage);
}
*/

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

void WorldMapSetPlayerPositionFromPoint(s32 arg0) {
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
                // the byte cast keeps gcc from hoisting this load above the two position
                // stores; retail reads z after them (`lh v0,4(a2)` then a load-delay nop).
                nPos = *(s16*)((u8*)p + offsetof(WorldMapPoint, z)) << 12;
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

// Header of the decompressed map data block: one offset per sub-block, relative to the start
// of the block. The offsets are named after the D_ global the sub-block is stored into.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 pointTableOffset;
    /* 0x08 */ s32 modelOffset;
    /* 0x0C */ s32 modelRelocOffset;
    /* 0x10 */ s32 spriteSetOffset;
    /* 0x14 */ s32 primBlocksOffset;
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
    g_WorldMapPrimBlocks = (s32)(pMapBase + pHeader->primBlocksOffset);
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

// Scratchpad area the frame projection works in: GTE's input rotation at 0x00 and, at 0x48 and
// 0x58, the word and flag RotTransPers4 writes for the fourth vertex. The two matrix work areas
// at 0x08/0x28 are addressed by their own literals in retail, so they stay an unnamed span.
typedef struct {
    /* 0x00 */ SVECTOR viewRotation;
    /* 0x08 */ u8 unk8[0x40];
    /* 0x48 */ long interpolated;
    /* 0x4C */ long flag;
    /* 0x50 */ u8 unk50[8];
    /* 0x58 */ long tileOtz;
    /* 0x5C */ long tileOtzFlag;
} WorldMapProjectionScratch; // Size: 0x60, a view of the 0x1F800000 scratchpad

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

    pScratch = (WorldMapProjectionScratch*)PSX_SCRATCH;
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
    clut = GetClut(WORLD_MAP_TILE_CLUT_VRAM_X, WORLD_MAP_CLUT_VRAM_Y);
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
    SetTexWindow(&g_WorldMapTileDrawModes[0], &tw);
    tw.x = 0;
    tw.y = 0;
    tw.w = 0;
    tw.h = 0;
    SetTexWindow(&g_WorldMapTileDrawModes[1], &tw);
}
// The two tile prims this frame draws are POLY_FT4s out of g_WorldMapTilePrims, and their
// corner u/v words scroll with the map's rotation: retail writes each u/v byte pair as one u16
// word, 20 words per prim (one pair every 4 words), the second prim of the frame's pair two
// prims further along. D_8009C750 is g_WorldMapTilePrims[0].u0, the first of those words.
extern u16 D_8009C750[];

// 79.63% (744 bytes against retail's 812, 41 differing instructions). Everything that is not
// the u/v block matches, including the RotMatrixYXZ/CompMatrix/SetRotMatrix/SetTransMatrix
// prologue, the 2-iteration RotTransPers4 loop and the four addPrim calls, so the residual is
// the uv block plus the 8-byte frame and prologue order it drags in.
//
// The block is at retail's exact address set: retail keeps ONE register (t0 = &D_8009C750) and
// emits, per store, `addiu v1,t0,<const>; addu v1,v0,v1; sh` with v0 = g_WorldMapPrimPacketIndex
// * 0x28 computed separately and RELOADED per pair (four `lw g_WorldMapPrimPacketIndex`), and it
// never folds a constant into the `sh` displacement. Measured dead spellings, all of which give
// gcc a folded `sh a3,80(v0)` (8 of them, 716-744 bytes) or a `lui $at` MASPSX macro per store:
//   - D_8009C750[idx * 0x14 + off]              (the body below: 3 addiu/addu, 1 folded)
//   - D_8009C750[idx + 2].uvN / [idx].uvN on a 0x28-byte uv struct (symbol displacements)
//   - pUv = D_8009C750; pUv[idx*0x14 + off]      (probeF: folds everything into one address)
//   - *(u16 *)(idx * 0x28 + (0x50 + (s32)pUv))   (probeG: folds to `sh 80(v0)`)
//   - the same with a volatile store lvalue       (probeH: still folds)
//   - pointer locals for the base and for base+0x50, and the same in both source orders
// A cc1 probe with the +0 store FIRST does give retail's `la $4,D_8009C750` plus three
// addiu/addu stores, so the fold is only defeated for the stores whose constant does not match
// the `la` constant; retail has none folded, which no spelling reached here. Next lever to try:
// make the u/v words' base a value cc1 cannot constant-fold (a pointer loaded from a global).
/*
void func_80073B04(void) {
    WorldMapProjectionScratch* pScratch;
    SVECTOR* pVec;
    POLY_FT4* pBase;
    POLY_FT4* pPrim;
    P_TAG* pOt;
    s32 i;
    s32 nOtz;
    s32 uvOffset;
    u16 uv;
    u16 uvFlip;

    pScratch = (WorldMapProjectionScratch*)PSX_SCRATCH;
    uv = (g_WorldMapViewRotationY >> 2) & 0x7F;
    uvFlip = uv | 0x80;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14 + 0x28] = uv;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14] = uv;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14 + 0x2C] = uvFlip;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14 + 4] = uvFlip;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14 + 0x30] = uv | 0x3F00;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14 + 8] = uv | 0x3F00;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14 + 0x34] = uvFlip | 0x3F00;
    D_8009C750[g_WorldMapPrimPacketIndex * 0x14 + 0xC] = uvFlip | 0x3F00;
    *(u16*)0x1F800004 = 0;
    pScratch->viewRotation.vx = 0;
    *(u16*)0x1F800002 = g_WorldMapViewRotationY;
    RotMatrixYXZ(&pScratch->viewRotation, (MATRIX*)0x1F800038);
    *(s32*)0x1F800054 = 0;
    *(s32*)0x1F800050 = 0;
    *(s32*)0x1F80004C = 0;
    CompMatrix(&g_WorldMapViewMatrix, (MATRIX*)0x1F800038, (MATRIX*)0x1F800018);
    SetRotMatrix((MATRIX*)0x1F800018);
    SetTransMatrix((MATRIX*)0x1F800018);
    pVec = &g_WorldMapQuadVertices[16];
    for (i = 0, uvOffset = 0; i < 2; i++, pVec += 4, uvOffset += 0x50) {
        pBase = (POLY_FT4*)(g_WorldMapPrimPacketIndex * sizeof(POLY_FT4) + (s32)g_WorldMapTilePrims);
        pPrim = (POLY_FT4*)(uvOffset + (s32)pBase);
        nOtz = RotTransPers4(pVec, pVec + 1, pVec + 2, pVec + 3,
                             (long*)&pPrim->x0, (long*)&pPrim->x1,
                             (long*)&pPrim->x2, (long*)&pPrim->x3,
                             &pScratch->tileOtz, &pScratch->tileOtzFlag);
    }
    if (pScratch->tileOtzFlag >= 0) {
        pOt = (P_TAG*)((u32*)g_WorldMapRenderContext->pOt[0] + (nOtz >> g_GfxOTDepthShift));
        addPrim(pOt, (P_TAG*)&g_WorldMapTileDrawModes[1]);
        addPrim(pOt, (P_TAG*)&g_WorldMapTilePrims[g_WorldMapPrimPacketIndex]);
        addPrim(pOt, (P_TAG*)&g_WorldMapTilePrims[g_WorldMapPrimPacketIndex + 2]);
        addPrim(pOt, (P_TAG*)&g_WorldMapTileDrawModes[0]);
    }
}*/
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
// ROUND 374: re-measured 82.80%, 1176 bytes against retail's 1248 (the note below says
// 1240; the stub is 312 instructions, so retail is 1248).
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
    pPoint1A = (u16*)&g_GameState.playerMapPosX;
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
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 pad6;
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
        pSlot->z = 0;
        pSlot->y = 0;
        pSlot->x = 0;
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
        pPrim->clut = GetClut(WORLD_MAP_SLOT_CLUT_VRAM_X, WORLD_MAP_CLUT_VRAM_Y);
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
