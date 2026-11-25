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
extern void WorldMapSetupPrimTemplates(WorldMapEntry *pEntry, POLY_FT4 *pPrim, s32 count);
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
// Second declarations with an asm label: an address taken from a scalar global is a
// rematerialisable constant, so cc1 re-forms lui/addiu at each use or folds the store
// into the $at macro; the array form keeps the address in a register.
extern s16 g_WorldMapCameraRotTable[] __asm__("g_WorldMapCameraRot");
extern s16 g_WorldMapViewRotation[] __asm__("g_WorldMapViewRotationY");
extern s16 g_WorldMapViewRotationZTable[] __asm__("g_WorldMapViewRotationZ");
extern u16 D_8009BD42[];
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
extern void WorldMapDrawFadeFrames(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
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
extern s32 WorldMapInitGroupSlotAtPlayerPosition(s32 index);
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
// the camera easing helpers (misc3.c) and the wrap check they feed their deltas to
extern s32 WorldMapApproachValue(s32 value, s32 target, s32 step);
extern void WorldMapApproachViewRotation(WorldMapSlot *pSlot, s32 *pScratch);
extern void WorldMapApproachHeight(WorldMapSlot *pSlot, s32 *pScratch);
extern void func_80076FA8(WorldMapSlot *pSlot, s32 *pScratch);
extern void WorldMapWrapPosition(s32 *pPos);
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
// Best: 84.76% (164 bytes / 41 insns against retail's 42 - the missing instruction is retail's load-delay nop after the pos.z load, so this body cannot link).
//   The branch, every immediate, every relocation and the three WorldMapInitSpriteGroupSprites calls match once the three scratchpad stores use the literal address 0x1F8000A0: retail writes them as lui $at,0x1F80 + sh $v0,0xA0($at) with no relocation, while naming D_1F8000A0 emits %hi/%lo and forces the base into a saved register (76.71%). Residues: gcc sinks the command store below the pos.x/pos.z loads and hoists the pos.z load where retail keeps statement order, and the slot base lands in $a3 instead of $a2.
//   Round 481 measured on this body: three s32 locals, all three expressions inline, and the (u8 *)PSX_SCRATCH + 0xA0 cast spelling all score 82.19 with the same 41 insns; the positive if (command == 1) form is 84.76 like the early return, so the tie is allocator/scheduler and not the CFG. A do/while barrier after each of the three scratchpad stores (the trick that closed WorldMapInitGroupSlotAtPlayerPosition) is byte-identical here at 84.76 - the hoisted pos.z load crosses it. Earlier: a 36-row try_flags sweep (gcc 2.6.0/2.7.2/cdk x -O1..-O3 x -G0/-G4/-G8 x mcpu 3000/r8000) tops out at 84.76; addressing the slot as g_WorldMapSlots[index] per statement scores 81.95; moving the command store to either end stays 84.76.
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
}*/
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
// Best: 94.87%, 156 bytes / 39 insns = retail's exact size; re-measured this round.
// ONE residue: the first call's `move a0,s0` sits 3 slots late (after `li v1,-2048`
// instead of just before the pos.y store). sched2 placement: the copy has no
// dependences, so no source statement moves it (rounds 377 and this round re-measured
// round 377's five pArg/0xFFF80000-local shapes: identical body). Presets ruled out.
// Round 458 measured the empty-statement barrier as the only knob that MOVES the copy,
// and both of its positions miss retail by one: dropping the do/while entirely is
// 92.64 / 12 differing lines (the copy lands right after the Z load and `sll a1,a0,7`
// takes $a1); moving the barrier ABOVE the pos.y store instead of below it is 89.74 /
// 4 differing lines with the copy at its region's head, i.e. one slot LATE (after the
// Z load) and the Z load hoisted above the pos.y store; barrier on both sides is
// identical to the one below (94.87 / 2 lines). So sched2 ranks the copy below the Z
// load wherever the region starts, and retail's position - between the 0xFFF80000 lui
// and its store - is not a region head in any of the three shapes.

// Best: 94.87%, 156 bytes / 39 insns = retail's exact size, compiles. ONE residue: the first call's move a0,s0 is 3 slots late (after li v1,-2048, retail has it between the 0xFFF80000 lui and its pos.y store). Round 458 measured the empty-statement barrier as the only knob that moves it: no barrier 92.64 / 12 lines (copy right after the Z load, sll takes $a1), barrier above the pos.y store 89.74 / 4 lines (copy one slot late at the region head, Z load hoisted above the pos.y store), barrier on both sides identical to the one below (94.87 / 2 lines). sched2 ranks the copy below the Z load wherever the region starts; plus round 377's five pArg/0xFFF80000-local shapes (identical body), a 1887-iteration permuter run (NONE) and the preset screen (cdk 87.51, 2.6.0 87.15).
// Best: 94.87%, 156 bytes / 39 insns = retail's exact size; re-measured this round (round 471), compiles.
// ONE residue: the first call's move a0,s0 sits 3 slots late (right after li v1,-2048; retail has it between the 0xFFF80000 lui and its pos.y sw).
// Round 471 measured two new positions: a do-while barrier BETWEEN the pos.x and pos.y stores is byte-identical to the existing one below pos.y (94.87, same 2 lines), and declaring s32 z first (declaration order/pseudo numbers) is identical too. Presets screened (cdk 87.51, 2.6.0 87.15), permuter 1887 iterations NONE, five pArg/local shapes identical - this is sched2's ranking of the copy, park.
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
    WorldMapAddSlot(WorldMapInitGroupSlotAtPlayerPosition, WorldMapUpdateSlotColorFade);
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
// Round 462 re-measured this body live at 94.71 / 136 bytes (34 insns) and tried the
// one remaining cheap axis: writing the six targets as direct reads of the three position
// globals (no posX/posY/posZ locals, which is the shape that fixes the load order in the
// twin) rotates the rotation the OTHER way (x v1, y a0, z a1) and rates 83.06 - do not
// retry. The two deltas above stand: the pSlot/posZ pair is one local-alloc tie (posZ wins
// the lower register because its live range is short), and the tail needs retail RTL order
// li-then-sh, which no source order produces (a store can only follow the return constant
// if it is emitted after the return value).
// Round 475 re-measured this body live at 94.71 / 136 bytes (34 insns) and tried four more
// shapes, none a new best: (a) the slot-state store moved inside the do-block (identical
// body); (b) posX/posY/posZ declared first and pSlot assigned after them (identical body -
// the a2/v1 tie is not pseudo order); (c) posX = g_WorldMapPlayerPosition.x directly, no
// posZ round-trip (x v1, y a0, z a1, 91.x - the round-trip is what makes the three loads
// land in a0/a1/a2). The two deltas stand: the pSlot/z register tie and retail's
// li-then-sh reorg tail.
// Round 475: also tried return (pSlot->state = 0, 1) to keep the return constant out of the
// delay-slot candidate list - identical body, so reorg's choice is not the comma form.
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800813E8);



// Best: 89.94%, 620 bytes = retail's exact size, 155 instructions = retail's own.
// The command switch, the clamp-and-ease chain, the func_80096F18 call and the rand tail
// decode to retail's instructions. Four finds this round, all in the body below:
// (1) the command switch needs THREE case labels: case 2 and case 3 with empty bodies root
// cc1's balanced case tree at the middle value and reproduce retail's beq-2 / slti-3 /
// bne-1 chain, where a two-label switch roots at 1 and emits one bne.
// (2) g_WorldMapCameraRot is stored AND reloaded through a register in retail (lui a0,
// addiu a0, sh 0(reg), then lh 0(reg)); the asm-label array alias plus a volatile store
// gets that shape, where the scalar spelling emits the $at macro per use.
// (3) the ViewRotationY reload must not fold: a volatile pointer store followed by a read
// into one s32 local keeps retail's lui/lh/sll-12 pair and the two sw.
// (4) the tail: u8 *pScratch = (u8 *)0x1F800000 gives retail's lui s1,0x1f80 with 0xA2
// offsets, the scratchpad re-read has to be a volatile u16 load (retail re-reads it), and
// D_8009BD42 as an array keeps its base in one register for the lhu plus sh pair.
// Residues, all allocation: (a) the camRot address lands in $v1 where retail has $a0 and
// the 0x980000 constant in $v0 where retail has $v1 - a pure tie, both pseudos have two
// references; (b) the two lui of that constant sit four slots later than retail's; (c) sw
// s1,20(sp) is this body's 6th instruction against retail's 2nd.
// Tried and no better: a plain s16 *pRot with no alias (store through $at, 87.45); the
// ((s16 *)&g_WorldMapCameraRot)[1] neighbour read for ViewRotationY (89.57, and cc1 sinks
// the store below that load); the scalar plus a volatile read (lhu, same 620 bytes);
// storing through the alias inline instead of through the local (identical body); a plain
// read of ViewRotationY (folds, 616 bytes, 154 insns). Permuter 4 min -j2: base 1430 best
// 814, and its find reuses pRot for ViewRotationY so unk58/unk54 would read the wrong
// global - wrong code, rejected. Preset screen: cdk 80.46, gcc-2.6.0 compile-failed.
// Best: 91.65%, 616 bytes against retail 620 / 155 insns - ONE INSTRUCTION SHORT (the
// load-delay nop after the case-1 camRot reload), so it cannot link yet. Round 462
// improved the note's 89.94 by fixing the ViewRotationY read and the register pair:
// (1) the ViewRotationY reload must be a PLAIN symbol read of the s16, not a volatile
// pointer read: a volatile HImode load is expanded as lhu + sll 16 + sra 4, three insns
// where retail has lh + sll 12. The store itself stays volatile (a volatile cast on the
// global) so it is not forwarded away and the reload survives. (2) `pSlot->unk50 = 0x980000;`
// must be the arm's FIRST assignment (before the two view-angle stores): that makes the
// 0x980000 constant live where v0 holds -0x200, so it takes retail's $v1 and the camRot
// address falls to retail's $a0, and both 0x980000 stores share $v1 as retail does. With
// the ang/late order the same body takes $v0/$v1 the wrong way round (89.94). (3) the
// viewY read split as `viewY = g_WorldMapViewRotationY; ... viewY <<= 12;` keeps the lh
// and moves the CameraHeight store next to it, 91.65.
// Residual: sched2, not the C. Retail's arm is almost pure source order
// (sw 0x50 / lh rot / nop / sll / sw 0x58 / sw 0x54 / lh ViewY / sw CameraHeight / sll /
// sw 0x60 / sw 0x5C); this body hoists the ViewY load above the rot group and sinks the
// rot lh below the CameraHeight store, which costs the load-delay nop (616 vs 620). Also
// still ours: `sw s1,20(sp)` is the 5th prologue insn where retail has it 2nd.
// Tried this round, all measured, no better: direct global stores instead of the position
// locals in func_800813E8 (83.06); g_WorldMapViewRotation[0] (the 2-element asm-label
// alias) for the ViewY read, which gives lh but lets sched2 hoist the load above the
// volatile store (the store moves below its own load); a shared `s32 nHeight` for the two
// 0x980000 stores (copy-propagated away, identical code); the CameraHeight store above
// the viewY read; the two viewY stores as separate `g_WorldMapViewRotationY << 12`
// expressions (cse merges them, 90.68). Preset screens stand from the earlier note: cdk
// 80.46, gcc-2.6.0 does not compile.
// Round 475 re-measured this body live at 91.65 / 616 bytes (154 insns, retail 155 / 620 - ONE
// instruction short, so it cannot link). The missing instruction is retail's load-delay nop
// after its rot reload: retail's case-1 arm is pure source order (sh rot / sh ViewY / sh ViewRotZ /
// sw 0x50 / lh rot / nop / sll / sw 0x58 / sw 0x54 / lh ViewY / sw CameraHeight / sll / sw 0x60 /
// sw 0x5C), while this body interleaves the ViewY chain into that slot and needs no nop.
// NEW, and the trap of the round: reading the angle through the array alias
// (viewY = g_WorldMapViewRotation[0], a distinct decl from g_WorldMapViewRotationY) scores 93.66
// and reproduces retail's rot chain (reload right after the sw 0x50, ViewY lh after the rot stores)
// but it is WRONG CODE - the alias read carries no dependence on the store, so the scheduler sinks
// the ViewY store BELOW its own load and unk60/unk5C get the pre-store angle. Same with the
// volatile store plus alias read (identical output). Do not port that shape, and expect the
// permuter to find it.
// Also tried this round, all measured, no better: unk50 moved after the ViewRotZ store (retail's
// own store order) 89.98; the rot pair and the unk60/unk5C pair as chained assignments 91.65
// (identical body) / 90.68 (all four chained).
/*
s32 func_80081470(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    u8 *pScratch = (u8 *)0x1F800000;
    s16 *pRot;
    s32 viewY;
    s32 diff;
    s16 command;
    s32 shake;

    command = pSlot->command;
    switch (command) {
    case 1:
        pSlot->state = 1;
        pSlot->command = 0;
        pRot = g_WorldMapCameraRotTable;
        *(volatile s16 *)pRot = -0x80;
        pSlot->unk50 = 0x980000;
        *(volatile s16 *)&g_WorldMapViewRotationY = -0x200;
        g_WorldMapViewRotationZ = 0;
        pSlot->unk58 = pRot[0] << 12;
        pSlot->unk54 = pRot[0] << 12;
        viewY = g_WorldMapViewRotationY;
        g_WorldMapCameraHeight = 0x980000;
        viewY <<= 12;
        pSlot->unk60 = viewY;
        pSlot->unk5C = viewY;
        break;
    case 2:
        break;
    case 3:
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
    *(u16 *)(pScratch + 0xA2) = shake;
    D_8009BD42[0] = D_8009BD42[0] + shake;
    D_8009BD4A = D_8009BD4A + *(volatile u16 *)(pScratch + 0xA2);
    return 1;
}*/
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

s32 WorldMapInitGroupSlotAtPlayerPosition(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    WorldMapEntry *pEntry;
    s32 posZ;

    pSlot->state = 0;
    do { } while (0); // keeps the position loads below the state store
    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = g_WorldMapPlayerPositionY - 0x100000;
    pEntry = &g_WorldMapGroups[2];
    do { } while (0); // keeps the z load out of the block above
    posZ = g_WorldMapPlayerPositionZ;
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    pSlot->pos.z = posZ;
    WorldMapInitPolyFt4Prims(pEntry, pEntry->pPackets[0], pEntry->pModel->polyCount, 1);
    pEntry->x = pSlot->pos.x >> 12;
    pEntry->y = pSlot->pos.y >> 12;
    pEntry->z = pSlot->pos.z >> 12;
    return 1;
}
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
// WorldMapInitGroupSlotAtPlayerPosition above; pointer local, zero-store order and x/y/z order were
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
// Best: 96.08%, 240 bytes and 60 instructions = retail's own (0x800828DC is the next
// function, so the old "200 bytes" note was wrong). Ported from the misc8 twin
// func_8008390C's park: the position triples reuse one s32 local (retail reuses $v0), the
// camera height constant is a variable assigned right after the third position load, the
// state/command/unk5C stores are a do-block, and the two view-angle reads go through
// ((s16 *)&g_WorldMapCameraRot)[1]/[2] so store-to-load folding does not eat retail's
// lui/lh/sll (that spelling is a base+addend relocation, so diff_func can never print
// MATCH for this body; the linked bytes are identical).
// Residues: (1) the 0x500000 constant lives in $a0 here and in $a1 in retail (its lui and
// both its sw differ) - the same allocator tie misc8's func_8008390C is parked on at
// 96.15; (2) the angleZ `lh` lands between the two `<< 12` stores where retail has it
// after both.
// Tried: typed struct fields instead of the u8 *p casts (76.47 - the cast spelling is
// load-bearing), literals instead of the cameraHeight local (92.73), dropping the
// do-block (87.57), both `<< 12` stores ahead of the angleZ load (92.58), two declaration
// orders and a separate rotation local (identical), permuter 4 min -j2 (2056 iterations,
// base 95 best 95, NONE).
// Best: 96.25% (this round, up from 96.08), 240 bytes / 60 instructions = retail's exact.
// The find: the two view-angle reads (retail lh g_WorldMapViewRotationY / -Z) go through a
// 2-element array alias, extern s16 g_WorldMapViewRotation[] __asm__(g_WorldMapViewRotationY)
// declared beside the g_WorldMapCameraRotTable alias. Reading [0]/[1] is a zero/no-addend
// relocation on retail's own symbol instead of the ((s16 *)&g_WorldMapCameraRot)[1] addend
// form, and it keeps retail's whole register set (slot pointer $v1, angle in $a0). The
// label alias is what stops store-to-load folding, so the lui/lh/sll survives; spellings
// that block it another way lose the allocation: volatile s16 *pViewY store+read 87.98
// (cc1 emits lhu + sll 16/sra 4), volatile store + plain read 93.33, plain store + array
// read with the load written after both <<12 stores 93.17, u32 angleZ + v=angleZ 92.75.
// Residues: (a) the 0x500000 constant lands in $a0 here and in $a1 in retail (its lui and
// both its sw) - the allocator tie misc8's func_8008390C parks on too; (b) the Z lh is one
// slot early (between the two <<12 stores where retail has it after both).
// Kept: cameraHeight is declared with the 0x500000 initialiser (identical body, tidier),
// the state/command/unk5C do-block is load-bearing, permuter 4 min -j2 finds nothing.
// Round 458: 96.25 re-measured (240 bytes / 60 insns = retail's). Both residues are one
// allocation fact: the Z `lh` placed BEFORE the 0x3C store keeps $a0 for itself and the
// 0x500000 constant lands in $a0 too; moving the load after both <<12 stores is 93.17
// (Y and Z then share $v0) and reading Z early (right after the ViewZ store, so $a0 is
// live across the constant) puts the constant in $a1 exactly as retail but leaves the
// load where it was written - 89.90, 10 differing lines. So the constant's register is
// decided by whether $a0 is live at that point, and the only way to have both is a load
// the scheduler sinks, which sched2 does not do for this one. Park at 96.25.

// Best: 96.42%, 240 bytes / 60 insns = retail's exact size (round 471, up from 96.25). Only 8 differing lines, two residues:
// (a) the 0x500000 constant is in $a0 here and $a1 in retail (its lui and both its sw, 3 rows);
// (b) retail has sw v0,60(v1) (the 0x3C store) BEFORE the ViewRotationZ load's lui+lh, this body one slot after (5 rows).
// Round 471 find: the Z read must name retail's own symbol. A second asm-label alias extern s16 g_WorldMapViewRotationZTable[] __asm__("g_WorldMapViewRotationZ") beside g_WorldMapViewRotation[] gives lh a0,lo(g_WorldMapViewRotationZ)(a0) exactly as retail, where the old g_WorldMapViewRotation[1] spelling emitted ViewRotationY+2 (96.25).
// Round 471 measurements, all on this body: writing the Z read AFTER both <<12 stores is 93.33 / 48 lines and shows the cause - the whole allocation rotates (slot pointer to $a1, the constant to $v1, the Z angle to $v0), same with a barrier before the read; a barrier between the 0x54 store and the read gives lh a0,2(a1) off a reused ViewRotationY base, 86.17 / 22 lines; s32 vs u32 angleZ is byte-identical (96.42); the typed-struct spelling of all eight p+0x casts is 76.63 / 84 lines, so the u8* cast spelling really is load-bearing here.
// Residues (a)+(b) are one trade: round 458 measured that reading Z early (so $a0 is live across the constant) puts the constant in $a1 as retail but leaves the load at the read position, 89.90 / 10 lines, and sched2 will not sink it. (b) is stream order - the read's statement has to precede the 0x3C store to keep the 96.42 allocation - so the two cannot be had together. Presets screened (cdk 80.46, gcc-2.6.0 does not compile), permuter 4 min NONE on the earlier body.
// Round 476 re-measured this body live: 96.42%, 240 bytes / 60 insns = retail's exact, 8 differing
// lines. Three more spellings of retail's own store order (second angle load AFTER both unk54/unk3C
// stores), all measured, all 93.33% / 48 lines with the slot pointer rotating from $v1 to $a1:
// plain stores; a chained assignment (unk3C = unk54 = v << 12); a mirrored store (unk3C = unk54 read
// back, which cse forwards into two sw v0 exactly as retail). So the store wording is not the handle -
// the SECOND angle load's pseudo creation slot decides the whole local allocation, and writing it one
// statement earlier (between the two stores, i.e. not retail's own order) is what buys 96.42.
// Also measured worse this round: one reused s32 angle local for both angles with the load after both
// stores, 94.58 / 24 lines - the first lh moves to $a1 and the second chain to $a1/$v0.
// See tools/rounds/levers.md a-store-order-spelling-cannot-move-the-loads-allocno.
/*
s32 func_800827EC(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 cameraHeight = 0x500000;
    s32 v;
    u32 angleZ;
    u8 *p = (u8 *)pSlot;

    *(s32 *)(p + 0x7C) = 0x1000;
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
    v = -0x220;
    do {
        pSlot->state = 0;
        pSlot->command = 0;
        *(s32 *)(p + 0x5C) = cameraHeight;
    } while (0);
    g_WorldMapCameraRot = v;
    g_WorldMapViewRotationY = 0;
    g_WorldMapViewRotationZ = 0;
    *(s32 *)(p + 0x50) = -0x220000;
    *(s32 *)(p + 0x38) = -0x220000;
    v = g_WorldMapViewRotation[0];
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapCameraHeight = cameraHeight;
    *(s32 *)(p + 0x54) = v << 12;
    angleZ = g_WorldMapViewRotationZTable[0];
    *(s32 *)(p + 0x3C) = v << 12;
    g_WorldMapFadeLevel = 0x78;
    *(s32 *)(p + 0x58) = angleZ << 12;
    *(s32 *)(p + 0x40) = angleZ << 12;
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827EC);




// Best: 80.07% (round 459), 1676 bytes / 419 instructions against retail's 1672 / 418.
// Structure decoded: command-1 switch (12 cases, jtbl_8007041C) sets the camera and the
// slot's unk50/unk54/unk5C/unk58, then the state switch (jtbl_8007044C) eases those with
// WorldMapApproachValue, then the rotation/height helpers and the shake tail. Both tables
// land at retail's rodata offsets, so no table shift is owed.
// Round 459 closed round 445's open question with a cc1 PROBE (no build change): compiling
// this body's own build/src/world_map/main/misc7.c.i with the unit cc1 plus
// -fno-schedule-insns -fno-schedule-insns2 gives EXACTLY retail's statement order in the
// five big arms (sw unk50, sw unk38, lh ViewRotationY, load-delay nop, sll, sw unk54, sw
// unk3C, CameraHeight sw, lh ViewRotationZ, nop, sll, ...), so sched2 - not the C wording -
// hoists that lh above the stores. Two shapes then moved the SCHEDULED stream toward retail:
// (1) a `do { } while (0);` after each pair of unk50/unk38 stores, 75.62 -> 77.56 (1684 bytes);
// (2) the CameraHeight store written BETWEEN the unk58 and unk40 statements instead of before
//     unk58, 77.56 -> 80.07 (1676 bytes, one instruction off retail).
// Residual: (a) exactly one instruction - retail puts the CameraHeight lui+sw pair in the
// ViewRotationZ load delay slot, this build emits `nop` there and stores after the sll;
// (b) the s0/s1 role swap (retail pSlot=$s1, scroll pointer=$s0; here the reverse), which
// poisons every diff row and is unchanged by where the pointer is formed (three positions
// measured, see below).
// Measured worse this round: case-11 scroll target written as plain g_WorldMapScrollTargetX
// with no pointer local 72.21 / 1668 (cse rematerialises, no third saved register); a s32
// angle local shared by unk58/unk40 79.97 / 1676; a barrier between unk58 and the
// CameraHeight store 77.14 / 1728; a barrier after unk40 in cases 5/7/9 77.16 / 1744;
// -O1 54.04, -O3 75.62 on the round-430 body. Preset screen on THIS body: gcc-2.6.0 80.25,
// gcc-2.7.2-cdk rejects the unit's flags. 2.6.0 still breaks three functions already matched
// here (WorldMapInitSlotAtScrollPosition 95.00, WorldMapBeginSlotAnimFrame 95.26,
// WorldMapUpdateSlotColorFade 96.81), so the unit is not a 2.6.0 unit.
// Round 430/445 findings kept: forming pScrollTargetX once between the func_80096F18 call and
// the state switch gives retail's prologue/epilogue; forming it inside case 11 or at the top of
// the function loses the third saved register (72.21 / 74.10). Load-bearing spellings, do not
// simplify: the ViewRotationY/Z stores must be volatile (else gcc folds the later `<< 12` to an
// immediate, 383 instructions); the `do { } while (0);` after unk3C stops gcc cross-jumping the
// two rotation quadruples of all five arms; `s32 shake`, not s16; `s16 request = pSlot->command
// - 1; switch (request)` is this unit's lhu/addiu/sll/sra idiom. Permuter (4 min, on the older
// body): NONE.
/*
s32 func_800828DC(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    u8 *pScratch = (u8 *)0x1F800000;
    s32 *pScrollTargetX;
    s16 request;
    s32 shake;

    request = pSlot->command - 1;
    switch (request) {
    case 0:
        pSlot->command = 0;
        pSlot->state = 1;
        pSlot->unk50 = pSlot->unk38;
        pSlot->unk54 = pSlot->unk3C;
        pSlot->unk58 = pSlot->unk40;
        break;
    case 1:
        pSlot->command = 0;
        pSlot->state = 0;
        pSlot->unk5C = 0x3E0000;
        do { } while (0);
        g_WorldMapCameraRot = -0x20;
        *(volatile s16 *)&g_WorldMapViewRotationY = 0xD70;
        *(volatile s16 *)&g_WorldMapViewRotationZ = 0;
        do { } while (0);
        pSlot->unk50 = 0xFFFE0000;
        pSlot->unk38 = 0xFFFE0000;
        do { } while (0);
        pSlot->unk54 = g_WorldMapViewRotationY << 12;
        pSlot->unk3C = g_WorldMapViewRotationY << 12;
        do { } while (0);
        pSlot->unk58 = g_WorldMapViewRotationZ << 12;
        g_WorldMapCameraHeight = 0x3E0000;
        pSlot->unk40 = g_WorldMapViewRotationZ << 12;
        break;
    case 2:
        pSlot->command = 0;
        pSlot->state = 2;
        break;
    case 3:
        pSlot->command = 0;
        pSlot->state = 3;
        g_WorldMapScrollTargetX = 0x07499000;
        g_WorldMapScrollTargetY = 0xFFE74000;
        g_WorldMapScrollTargetZ = 0x0408A000;
        break;
    case 4:
        pSlot->command = 0;
        pSlot->state = 4;
        break;
    case 5:
        pSlot->state = 5;
        pSlot->shakeAmount = 0x1000;
        pSlot->command = 0;
        pSlot->unk5C = 0x320000;
        do { } while (0);
        g_WorldMapCameraRot = 0x40;
        *(volatile s16 *)&g_WorldMapViewRotationY = 0xD40;
        *(volatile s16 *)&g_WorldMapViewRotationZ = 0;
        do { } while (0);
        pSlot->unk50 = 0x40000;
        pSlot->unk38 = 0x40000;
        do { } while (0);
        pSlot->unk54 = g_WorldMapViewRotationY << 12;
        pSlot->unk3C = g_WorldMapViewRotationY << 12;
        do { } while (0);
        pSlot->unk58 = g_WorldMapViewRotationZ << 12;
        g_WorldMapCameraHeight = 0x320000;
        pSlot->unk40 = g_WorldMapViewRotationZ << 12;
        g_WorldMapScrollTargetX = 0x01379000;
        pSlot->pos.x = 0x01379000;
        g_WorldMapScrollTargetY = 0xFFEE0000;
        pSlot->pos.y = 0xFFEE0000;
        g_WorldMapScrollTargetZ = 0x04B2A000;
        pSlot->pos.z = 0x04B2A000;
        break;
    case 6:
        pSlot->command = 0;
        pSlot->state = 6;
        break;
    case 7:
        pSlot->state = 7;
        pSlot->command = 0;
        pSlot->unk5C = 0x100000;
        do { } while (0);
        g_WorldMapCameraRot = -0x90;
        *(volatile s16 *)&g_WorldMapViewRotationY = -0x510;
        *(volatile s16 *)&g_WorldMapViewRotationZ = 0;
        do { } while (0);
        pSlot->unk50 = 0xFFF70000;
        pSlot->unk38 = 0xFFF70000;
        do { } while (0);
        pSlot->unk54 = g_WorldMapViewRotationY << 12;
        pSlot->unk3C = g_WorldMapViewRotationY << 12;
        do { } while (0);
        pSlot->unk58 = g_WorldMapViewRotationZ << 12;
        g_WorldMapCameraHeight = 0x100000;
        pSlot->unk40 = g_WorldMapViewRotationZ << 12;
        g_WorldMapScrollTargetX = 0x07529000;
        pSlot->pos.x = 0x07529000;
        g_WorldMapScrollTargetY = 0xFFE74000;
        pSlot->pos.y = 0xFFE74000;
        g_WorldMapScrollTargetZ = 0x02A3A000;
        pSlot->pos.z = 0x02A3A000;
        break;
    case 8:
        pSlot->command = 0;
        pSlot->state = 8;
        break;
    case 9:
        pSlot->state = 9;
        pSlot->command = 0;
        pSlot->unk5C = 0x320000;
        do { } while (0);
        g_WorldMapCameraRot = -0x160;
        *(volatile s16 *)&g_WorldMapViewRotationY = 0xDE0;
        *(volatile s16 *)&g_WorldMapViewRotationZ = 0;
        do { } while (0);
        pSlot->unk50 = 0xFFEA0000;
        pSlot->unk38 = 0xFFEA0000;
        do { } while (0);
        pSlot->unk54 = g_WorldMapViewRotationY << 12;
        pSlot->unk3C = g_WorldMapViewRotationY << 12;
        do { } while (0);
        pSlot->unk58 = g_WorldMapViewRotationZ << 12;
        g_WorldMapCameraHeight = 0x320000;
        pSlot->unk40 = g_WorldMapViewRotationZ << 12;
        g_WorldMapScrollTargetX = 0x02659000;
        pSlot->pos.x = 0x02659000;
        g_WorldMapScrollTargetY = 0xFFEF0000;
        pSlot->pos.y = 0xFFEF0000;
        g_WorldMapScrollTargetZ = 0x06D8A000;
        pSlot->pos.z = 0x06D8A000;
        break;
    case 10:
        pSlot->state = 10;
        pSlot->command = 0;
        pSlot->unk5C = 0x290000;
        do { } while (0);
        g_WorldMapCameraRot = -0x1A0;
        *(volatile s16 *)&g_WorldMapViewRotationY = 0x160;
        *(volatile s16 *)&g_WorldMapViewRotationZ = 0;
        do { } while (0);
        pSlot->unk50 = 0xFFE60000;
        pSlot->unk38 = 0xFFE60000;
        do { } while (0);
        pSlot->unk54 = g_WorldMapViewRotationY << 12;
        pSlot->unk3C = g_WorldMapViewRotationY << 12;
        do { } while (0);
        pSlot->unk58 = g_WorldMapViewRotationZ << 12;
        g_WorldMapCameraHeight = 0x290000;
        pSlot->unk40 = g_WorldMapViewRotationZ << 12;
        g_WorldMapScrollTargetX = 0x04BA9000;
        pSlot->pos.x = 0x04BA9000;
        g_WorldMapScrollTargetY = 0xFFD78000;
        pSlot->pos.y = 0xFFD78000;
        g_WorldMapScrollTargetZ = 0x01DDA000;
        pSlot->pos.z = 0x01DDA000;
        break;
    case 11:
        pSlot->command = 0;
        pSlot->state = 11;
        break;
    }
    if (g_WorldMapRotationMatrixValid == 0) {
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight,
                      (u16 *)&g_WorldMapCameraRot);
    }
    pScrollTargetX = &g_WorldMapScrollTargetX;
    switch (pSlot->state) {
    case 1:
        pSlot->unk50 = WorldMapApproachValue(pSlot->unk50, 0xFFFE0000, 0x5D17);
        pSlot->unk54 = WorldMapApproachValue(pSlot->unk54, 0x580000, 0x10000);
        break;
    case 2:
        pSlot->unk50 = WorldMapApproachValue(pSlot->unk50, 0xFFE10000, 0xFFFF0000);
        pSlot->unk54 = WorldMapApproachValue(pSlot->unk54, 0x01290000, 0x10000);
        pSlot->unk5C = WorldMapApproachValue(pSlot->unk5C, 0x490000, 0x10000);
        break;
    case 3:
        pSlot->unk50 = WorldMapApproachValue(pSlot->unk50, 0xFFFF0000, 0x10000);
        pSlot->unk54 = WorldMapApproachValue(pSlot->unk54, 0x01290000, 0x10000);
        pSlot->unk5C = WorldMapApproachValue(pSlot->unk5C, 0x2B0000, 0xFFFF0000);
        break;
    case 4:
        pSlot->shakeAmount = pSlot->shakeAmount + 0x200;
        if (pSlot->shakeAmount > 0x8000) {
            pSlot->shakeAmount = 0x8000;
            pSlot->state = 0;
        }
        break;
    case 6:
        pSlot->unk54 = WorldMapApproachValue(pSlot->unk54, 0xF40000, 0x4000);
        break;
    case 8:
        pSlot->unk50 = WorldMapApproachValue(pSlot->unk50, 0x50000, 0x2000);
        pSlot->unk54 = WorldMapApproachValue(pSlot->unk54, 0x10000, 0x8000);
        break;
    case 11:
        pSlot->unk50 = WorldMapApproachValue(pSlot->unk50, 0xFFF80000, 0x2400);
        pSlot->unk54 = WorldMapApproachValue(pSlot->unk54, 0x2C0000, 0x2C00);
        pSlot->unk5C = WorldMapApproachValue(pSlot->unk5C, 0x100000, -0x3200);
        *pScrollTargetX = WorldMapApproachValue(*pScrollTargetX, 0x04CD9000, 0x2600);
        g_WorldMapScrollTargetY = WorldMapApproachValue(g_WorldMapScrollTargetY, 0xFFEA4000, 0x2580);
        g_WorldMapScrollTargetZ = WorldMapApproachValue(g_WorldMapScrollTargetZ, 0x01C8A000, -0x2A00);
        WorldMapWrapPosition(pScrollTargetX);
        break;
    case 0:
    default:
        break;
    }
    WorldMapApproachViewRotation(pSlot, (s32 *)pScratch);
    WorldMapApproachHeight(pSlot, (s32 *)pScratch);
    func_80076FA8(pSlot, (s32 *)pScratch);
    shake = rand() % (pSlot->shakeAmount >> 12) - (pSlot->shakeAmount >> 13);
    *(s16 *)(pScratch + 0xA2) = shake;
    D_8009BD42[0] = D_8009BD42[0] + shake;
    D_8009BD4A = D_8009BD4A + *(volatile u16 *)(pScratch + 0xA2);
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800828DC);


