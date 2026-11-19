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
    /* 0x7C */ s32 unk7C;
} WorldMapSlot; // Size: 0x80

typedef struct {
    /* 0x00 */ u8 unk0[0x8];
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ u8 unk14[0xC];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ ModelPart *pModel;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ void *pPackets[2]; // prim lists, [0] source and [1] destination; g_WorldMapPrimPacketIndex picks one
    /* 0x50 */ u8 unk50[0x4];
} WorldMapEntry; // Size: 0x54

// One world map fade quad: a textured quad per fade-table entry, jittered by
// func_80081D80 so the wipe does not move in one block. The tpage word sits after the
// second vertex's texcoords, where the original object reads it.
typedef struct {
    /* 0x00 */ u32 tag;
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
extern DR_MOVE D_8009D164[];
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
extern s16 D_1F8000A0[];
extern void func_80089160(s32, s16 *, s32);
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
// func_80089160 calls match once the three scratchpad stores use the literal
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
    func_80089160(0x28, (s16 *)0x1F8000A0, 0);
    func_80089160(0x29, (s16 *)0x1F8000A0, 0);
    func_80089160(0x2A, (s16 *)0x1F8000A0, 0);
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

// Earlier body (92.64%): instruction multiset, relocations and every
// store offset match; two residues. (1) retail emits `sll a0,a0,0x7` on the
// parameter register and `addu v0,v0,a0`, gcc allocates a fresh pseudo
// (`sll a1,a0,0x7`). (2) retail's `move a0,s0` (the first call argument) lands
// between the two pos stores, gcc emits it just before them; writing the slot
// stores in retail's emission order (x, y, unk54, unk50, unk58, z) gives this
// shape, the x/y/z-first order gives the same 92.64%.
// Preset screen: cdk 87.51, gcc-2.6.0 87.15.
// Permuter (round 181): two BETTER candidates, both FALSE gains under objdiff.
// 295 -> 220 (pos.z store moved up to second, plus an 0x80 temp for unk58)
// measures 83.64%; a rerun 295 -> 275 (hoisting the model pointer load above
// unk54 into a temp) measures 89.10%, because it makes gcc fold the lhu onto
// that register and drop retail's separate `lw v0,64(s0)`. Slot store order is
// source-insensitive here: z second, z third and an 0x80 local all reproduce
// 92.64% byte for byte.
// Best: 94.87%. Instruction multiset, relocations and every store offset match; the
// one residue is where the call-argument copy lands: retail puts move a0,s0 between
// the pos.y lui and its store (instruction 12), gcc puts it between li v1,-2048 and
// the unk54 store, i.e. after the barrier. The parameter shift is NOT a residue any
// more: sll a0,a0,0x7 now matches retail (older note claimed it could not be reached).
// The empty do/while after the pos.y store is load-bearing: without it gcc sinks the
// pos.y store below the Z load (92.64%), with it 93.33%. Hoisting the Z load into a
// local s32 z ahead of unk54 (permuter 295 -> 120 -> 60, run 217) gives 94.87%.
// Swept single and double barrier positions (after statement 0..5) with and without
// the z local: 94.87% is the cap; a barrier after unk54 or later drops below 85%,
// and an empty block or if (1) { } as the barrier gives the pre-barrier 92.64%. The
// long long cast the permuter bundled with the z local measures 82.00% at 168 bytes
// (false gain); a nested if (1) { } inside the do/while is neutral.
/*
s32 func_80080A28(s32 index) {
    u8 *p = g_WorldMapGroups;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 z;

    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = 0xFFF80000;
    do { } while (0);
    z = g_WorldMapPlayerPositionZ;
    pSlot->unk54 = -0x800;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0x80;
    pSlot->pos.z = z;
    WorldMapSetupPrimTemplates(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    p += 0x54;
    WorldMapSetupPrimTemplates(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    return 3;
}
*/
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080D00);
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
        SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0xF0);
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
// Best: 76.59%, same class as before: instruction multiset and relocations are
// identical (the three source words are the separate globals g_WorldMapPlayerPosition,
// g_WorldMapPlayerPositionY and g_WorldMapPlayerPositionZ, not offsets off a Vec3 - retail names all three),
// only the schedule of the `sh` pairs differs. With the two slot halfword
// stores written first gcc emits them right after the base addu and pushes the
// BE28/D55C pair down (this shape, 76.59%); moving them last instead reorders
// the whole tail (65.71%, and 33.94% while y/z were still struct offsets).
// Reading y/z as g_WorldMapPlayerPosition.y/.z scores 77.76% but emits lui/lw 4(a0)/8(a0)
// relocations retail does not have.
// Preset screen: cdk 76.59, gcc-2.6.0 46.47.
/*
s32 func_800813E8(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 x;
    s32 y;
    s32 z;

    pSlot->command = 1;
    pSlot->state = 0;
    g_WorldMapRotationMatrixValid = 0;
    pSlot->unk7C = 0x1000;
    x = g_WorldMapPlayerPosition.x;
    y = g_WorldMapPlayerPositionY;
    z = g_WorldMapPlayerPositionZ;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraTarget = x;
    g_WorldMapScrollTargetX = x;
    g_WorldMapCameraTargetY = y;
    g_WorldMapScrollTargetY = y;
    g_WorldMapCameraTargetZ = z;
    g_WorldMapScrollTargetZ = z;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800813E8);
// Best: 78.69%, 580 bytes against retail's 636. The clamp/easing chain is byte-identical:
// unk50 -0x10000 down to 0x630000 with g_WorldMapCameraHeight eased by diff >> 5, unk54
// += 0x1000 clamped to 0x10000 and eased into unk58 by diff >> 4 driving
// g_WorldMapCameraRot, unk5C += 0x10000 clamped to 0x4B0000 and eased into unk60 the same
// way, plus the func_80096F18 call and the rand()/unk7C jitter. Every residue is one
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
    shake = rand() % (pSlot->unk7C >> 12) - (pSlot->unk7C >> 13);
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

// Best: 66.00%. Structure, relocations and offsets are all right (the call,
// the three >>12 stores, the six slot stores); gcc's scheduler hoists the
// g_WorldMapPlayerPositionY/g_WorldMapPlayerPositionZ loads and the g_WorldMapGroups load above the `sh 0x20`
// store and sinks the g_WorldMapPlayerPosition load to its store, retail keeps statement
// order. Locals x/y/z and a late pEntry assignment were no better.
// Preset screen: cdk 50.54, gcc-2.6.0 58.56.
/*
s32 func_800817A0(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    WorldMapEntry *pEntry = &g_WorldMapGroups[2];

    pSlot->state = 0;
    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = g_WorldMapPlayerPositionY - 0x100000;
    pSlot->pos.z = g_WorldMapPlayerPositionZ;
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    WorldMapInitPolyFt4Prims(pEntry, pEntry->unk48, *(u16 *)(pEntry->unk40 + 4), 1);
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
            pPrim->code &= ~2;
            pPrim->r0 = 0x80;
            pPrim->g0 = 0x80;
            pPrim->b0 = 0x80;
        }
        break;
    }
    if (pSlot->state != 0) {
        if (pSlot->state == 1) {
            pSlot->unk50 += 1;
            pSlot->unk54 += 1;
            pSlot->unk58 += 1;
            if (pSlot->unk50 >= 0xFF) {
                pSlot->unk58 = 0xFF;
                pSlot->unk54 = 0xFF;
                pSlot->unk50 = 0xFF;
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800819C8);
/*
s32 func_800819C8(s32 index) {
    u8 *pEntry = (u8 *)g_WorldMapGroups;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->state = 0;
    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = g_WorldMapPlayerPositionY;
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
}
*/
s32 WorldMapUpdateSlotColorFadeFast(s32 index) {
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
            if (pSlot->unk50 >= 0xFC) {
                pSlot->unk50 = 0xFC;
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
/*
s32 func_80081C3C(void) {
    Quad *pSrc;
    Quad *pDst;
    u16 *pFill;
    u8 *p;
    s32 i;

    g_WorldMapFadeQuadBuffers[0] = HeapAlloc(0x1E00, 0);
    D_8009D15C = HeapAlloc(0x1E00, 0);
    g_WorldMapFadeTable = HeapAlloc(0x180, 0);
    p = g_WorldMapFadeQuadBuffers[0];
    for (i = 0; i < WORLD_MAP_FADE_ENTRY_COUNT; i++) {
        p[7] = 0x2C;
        p[3] = 9;
        p[4] = 0x80;
        p[5] = 0x80;
        p[6] = 0x80;
        p[7] |= 1;
        *(u16 *)(p + 0x16) = GetTPage(2, 0, 0x280, 0x100);
        p += 0x28;
    }
    pSrc = (Quad *)g_WorldMapFadeQuadBuffers[0];
    pDst = (Quad *)D_8009D15C;
    for (i = 0; i < 0x1E0; i++) {
        *pDst++ = *pSrc++;
    }
    pFill = g_WorldMapFadeTable;
    for (i = WORLD_MAP_FADE_ENTRY_COUNT - 1; i >= 0; i--) {
        *pFill++ = 1;
    }
    return 1;
}
*/
// Best: 87.09%. Allocation, both loops' shapes and GetTPage are right; the
// differences are (1) retail keeps &g_WorldMapFadeQuadBuffers in $s0 across the three
// HeapAlloc calls (store and later load both go through it), gcc rematerialises
// lui/$at for each; (2) retail materialises 9 into $s4 before 0x2C into $s3.
// Preset screen: cdk 71.46, gcc-2.6.0 87.09.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081C3C);

// Best: 99.88%, retail's exact 564 bytes. The instruction sequence, every call, every
// store offset and the constant allocation order all match; the one residue is gcc's
// loop giv re-base. Retail keeps the vertex pointer at prim+0x25 and addresses the
// sixteen stores at 0 .. -0x1D; this gcc picks prim+0x20 and addresses them at 5 ..
// -0x18, i.e. the same addresses with every displacement 5 larger. `addPrim` is the
// right spelling (hand-spelling the two setaddr pairs as explicit u32 masks puts the
// base at 0x25 but then allocates 0xFF000000 before 0xFFFFFF, which costs the prologue
// save pair and the tail mask registers instead, 96.50%). Declaration order, u32 vs s32
// index, RECT/pMove/i first, operand order of both setaddr sums measured no better; a
// 4-minute permuter run (2245 iterations) found nothing.
/*
s32 func_80081D80(void) {
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
        pQuad->y2 = i + 1;
        pQuad->y3 = i + 1;
        pQuad->v2 = i + 1;
        pQuad->v3 = i + 1;
        pQuad->x0 = delta + 0x40;
        pQuad->x1 = delta + 0x100;
        pQuad->x2 = delta + 0x40;
        pQuad->x3 = delta + 0x100;
        addPrim(g_WorldMapRenderContext->pOt[0], pQuad);
        pQuad++;
    }
    rect.x = 0x40;
    rect.w = 0xC0;
    rect.h = 0xD8;
    rect.y = g_WorldMapPrimPacketIndex * 0xD8;
    SetDrawMove(&D_8009D164[g_WorldMapPrimPacketIndex], &rect, 0x280, 0x100);
    pMove = &D_8009D164[g_WorldMapPrimPacketIndex];
    addPrim(g_WorldMapRenderContext->pOt[0], pMove);
    return 1;
}

"*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081D80);

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
        level = 2;
        for (i = WORLD_MAP_FADE_ENTRY_COUNT - 1; i >= 0; i--) {
            *pFill++ = level;
        }
        break;
    }
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80082324);
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
// Best: 39.15%. Blocker is gcc's store-to-load forwarding: retail stores 0 to
// g_WorldMapViewRotationY/g_WorldMapViewRotationZ and reads them back with `lh` to build the two `<< 12`
// slot stores, while this gcc folds the read to 0 and drops the sll (which also
// shuffles the whole tail). Same blocker as misc8's func_8008390C (84.75%).
// Preset screen: cdk 36.73, gcc-2.6.0 40.43.
/*
s32 func_800827EC(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 a;
    s32 b;

    pSlot->unk7C = 0x1000;
    g_WorldMapCameraTarget = g_WorldMapPlayerPosition.x;
    g_WorldMapScrollTargetX = g_WorldMapPlayerPosition.x;
    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    g_WorldMapCameraTargetY = g_WorldMapPlayerPositionY;
    g_WorldMapScrollTargetY = g_WorldMapPlayerPositionY;
    pSlot->pos.y = g_WorldMapPlayerPositionY;
    g_WorldMapCameraTargetZ = g_WorldMapPlayerPositionZ;
    g_WorldMapScrollTargetZ = g_WorldMapPlayerPositionZ;
    pSlot->pos.z = g_WorldMapPlayerPositionZ;
    pSlot->state = 0;
    pSlot->command = 0;
    pSlot->unk5C = 0x500000;
    g_WorldMapCameraRot = -0x220;
    g_WorldMapViewRotationY = 0;
    g_WorldMapViewRotationZ = 0;
    pSlot->unk50 = 0xFFDE0000;
    pSlot->unk38 = 0xFFDE0000;
    a = g_WorldMapViewRotationY;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapCameraHeight = 0x500000;
    pSlot->unk54 = a << 12;
    pSlot->unk3C = a << 12;
    b = g_WorldMapViewRotationZ;
    g_WorldMapFadeLevel = 0x78;
    pSlot->unk58 = b << 12;
    pSlot->unk40 = b << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800828DC);
