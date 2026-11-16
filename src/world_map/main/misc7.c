#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/memory.h"
#include "psyq/rand.h"
#include "system/model.h"
#include "system/sound.h"

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
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x4];
    /* 0x28 */ Vec3 pos;
    /* 0x34 */ u8 unk34[0x1C];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ u8 unk5C[0x20];
    /* 0x7C */ s32 unk7C;
} WorldMapSlot; // Size: 0x80

typedef struct {
    /* 0x00 */ u8 unk0[0x8];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0x2C];
    /* 0x40 */ ModelPart *pModel;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ void *pPackets[2]; // prim lists, [0] source and [1] destination; g_WorldMapPrimPacketIndex picks one
    /* 0x50 */ u8 unk50[0x4];
} WorldMapEntry; // Size: 0x54

extern WorldMapSlot *g_WorldMapSlots;
extern WorldMapEntry *g_WorldMapGroups;
extern void WorldMapInitPolyFt4Prims(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 abr);
extern u8 D_8009A758[];
extern void WorldMapStartSlot(s32 arg0, s32 arg1);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern void SoundSetAudioManagerVolumeWithFade(s32 *pManager, s32 arg1, s32 arg2);
extern s32 *g_SoundAudioManager;
extern s32 g_WorldMapTpageAbr;
extern s32 D_8009D3CC;
extern s32 D_8009D554;
extern s32 g_WorldMapAreaMode;
// externs used by the parked attempts below
extern Vec3 g_WorldMapPlayerPosition;
extern MATRIX g_WorldMapIdentityMatrix;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s16 D_1F8000A0[];
extern void func_80089160(s32, s16 *, s32);
extern u16 g_WorldMapFadeIds[];
extern u16 g_WorldMapFadeFrameCounts[];
extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTarget;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern void WorldMapSetupPrimTemplates(void *, s32, s32);
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 D_8009BD3C;
extern s32 g_WorldMapCameraHeight;
extern SoundFile *g_MenuSedsFile;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 D_8009BBC4;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern void HeapFree(void *pMem);
extern void SoundStopAllElements(void);
extern void WorldMapFreeSpritePrimBuffers(void);
extern s32 g_WorldMapPrimPacketIndex;
extern u8 *D_8009D158;
extern u8 *D_8009D15C;
extern u8 *g_WorldMapFadeTable;
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
// slot as g_WorldMapSlots[index] per statement scores 81.95; moving the unk4 store to
// either end of the three stays at 84.76.
/*
s32 func_80080944(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 v;

    if (pSlot->unk4 != 1) {
        return 1;
    }
    pSlot->unk4 = 0;
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

// Best: 92.64% (two shapes tie). Instruction multiset, relocations and every
// store offset match; two residues. (1) retail emits `sll a0,a0,0x7` on the
// parameter register and `addu v0,v0,a0`, gcc allocates a fresh pseudo
// (`sll a1,a0,0x7`). (2) retail's `move a0,s0` (the first call argument) lands
// between the two pos stores, gcc emits it just before them; writing the slot
// stores in retail's emission order (x, y, unk54, unk50, unk58, z) gives this
// shape, the x/y/z-first order gives the same 92.64%.
// The parameter shift cannot be re-created either: `index *= 0x80` followed by
// `(u8 *)g_WorldMapSlots + index`, `index << 7`, and a u32/int local all still give
// `sll a1,a0,0x7`; retail's `sll a0,a0,0x7` needs gcc to hand the sum pseudo the
// dying argument register, which no source spelling reached.
// Preset screen: cdk 87.51, gcc-2.6.0 87.15.
/*
s32 func_80080A28(s32 index) {
    u8 *p = g_WorldMapGroups;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = 0xFFF80000;
    pSlot->unk54 = -0x800;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0x80;
    pSlot->pos.z = D_8009C5B4;
    WorldMapSetupPrimTemplates(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    p += 0x54;
    WorldMapSetupPrimTemplates(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    return 3;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080A28);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080AC4);
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
    D_8009BBC4 = 1;
    g_NextMapRotationY = v;
}
// The empty do/while is load-bearing: without it gcc hoists the g_WorldMapFadeIds load
// above the slot address formation (65.26%); with it the schedule is retail's.
s32 WorldMapBeginSlotAnimFrame(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    do { } while (0);
    pSlot->unk50 = 0;
    pSlot->unk20 = g_WorldMapFadeIds[0];
    pSlot->unk22 = g_WorldMapFadeFrameCounts[pSlot->unk50];
    pSlot->unk50 = pSlot->unk50 + 1;
    return 1;
}
s32 WorldMapUpdateSlotState(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    switch (pSlot->unk20) {
    case 1:
        pSlot->unk22 = pSlot->unk22 - 1;
        if ((s16)pSlot->unk22 < 0) {
            pSlot->unk20 = g_WorldMapFadeIds[pSlot->unk50];
            pSlot->unk22 = g_WorldMapFadeFrameCounts[pSlot->unk50];
            pSlot->unk50 = pSlot->unk50 + 1;
        }
        break;
    case 2:
        WorldMapStartSlot(6, 1);
        pSlot->unk20 = 1;
        break;
    case 3:
        WorldMapStartSlot(6, 0);
        pSlot->unk20 = 1;
        break;
    case 4:
        WorldMapStartSlot(6, 2);
        pSlot->unk20 = 1;
        break;
    case 5:
        WorldMapStartSlot(6, 3);
        pSlot->unk20 = 1;
        break;
    case 6:
        WorldMapStartSlot(6, 4);
        pSlot->unk20 = 1;
        break;
    case 7:
        WorldMapStartSlot(6, 5);
        pSlot->unk20 = 1;
        break;
    case 8:
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x2E);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x2F);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | 0x30);
        pSlot->unk20 = 1;
        break;
    case 0x10:
        WorldMapStartSlot(3, 1);
        WorldMapStartSlot(4, 1);
        pSlot->unk20 = 1;
        break;
    case 0x11:
        WorldMapStartSlot(3, 2);
        pSlot->unk20 = 1;
        break;
    case 0x3F:
        SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0xF0);
        WorldMapStartSlot(0, 0xD);
        g_WorldMapTpageAbr = 2;
        D_8009D3CC = 4;
        pSlot->unk20 = 1;
        break;
    case 0x40:
        D_8009D554 = 0;
        g_WorldMapAreaMode = 0;
        pSlot->unk20 = 0;
        break;
    case 0:
    default:
        break;
    }
    return 1;
}
// Best: 76.59%, same class as before: instruction multiset and relocations are
// identical (the three source words are the separate globals g_WorldMapPlayerPosition,
// D_8009C5B0 and D_8009C5B4, not offsets off a Vec3 - retail names all three),
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

    pSlot->unk4 = 1;
    pSlot->unk20 = 0;
    g_WorldMapRotationMatrixValid = 0;
    pSlot->unk7C = 0x1000;
    x = g_WorldMapPlayerPosition.x;
    y = D_8009C5B0;
    z = D_8009C5B4;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraTarget = x;
    g_WorldMapScrollTargetX = x;
    D_8009BE2C = y;
    g_WorldMapScrollTargetY = y;
    D_8009BE30 = z;
    g_WorldMapScrollTargetZ = z;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800813E8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081470);
void WorldMapInitPolyFt4Prims(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 abr) {
    s32 i;

    for (i = 0; i < count; i++) {
        pDst[3] = 9;
        pDst[7] = 0x2C;
        *(u16 *)(pDst + 0x16) = GetTPage(0, abr, 0x180, 0);
        pDst[7] |= 2;
        pDst += 0x28;
    }
    memcpy(pEntry->pPackets[1], pEntry->pPackets[0], count * sizeof(POLY_FT4));
}

// Best: 66.00%. Structure, relocations and offsets are all right (the call,
// the three >>12 stores, the six slot stores); gcc's scheduler hoists the
// D_8009C5B0/D_8009C5B4 loads and the g_WorldMapGroups load above the `sh 0x20`
// store and sinks the g_WorldMapPlayerPosition load to its store, retail keeps statement
// order. Locals x/y/z and a late pEntry assignment were no better.
// Preset screen: cdk 50.54, gcc-2.6.0 58.56.
/*
s32 func_800817A0(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    WorldMapEntry *pEntry = &g_WorldMapGroups[2];

    pSlot->unk20 = 0;
    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = D_8009C5B0 - 0x100000;
    pSlot->pos.z = D_8009C5B4;
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    WorldMapInitPolyFt4Prims(pEntry, pEntry->unk48, *(u16 *)(pEntry->unk40 + 4), 1);
    pEntry->unk8 = pSlot->pos.x >> 12;
    pEntry->unkC = pSlot->pos.y >> 12;
    pEntry->unk10 = pSlot->pos.z >> 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800817A0);
s32 WorldMapUpdateSlotColorFade(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    WorldMapEntry *pEntry = &g_WorldMapGroups[2];
    POLY_FT4 *pPrim;
    s32 i;

    switch (pSlot->unk4) {
    case 1:
        pSlot->unk4 = 0;
        pSlot->unk20 = 1;
        break;
    case 2:
        pSlot->unk4 = 0;
        pSlot->unk20 = 0;
        pPrim = pEntry->pPackets[g_WorldMapPrimPacketIndex];
        for (i = 0; i < pEntry->pModel->polyCount; i++, pPrim++) {
            pPrim->code &= ~2;
            pPrim->r0 = 0x80;
            pPrim->g0 = 0x80;
            pPrim->b0 = 0x80;
        }
        break;
    }
    if (pSlot->unk20 != 0) {
        if (pSlot->unk20 == 1) {
            pSlot->unk50 += 1;
            pSlot->unk54 += 1;
            pSlot->unk58 += 1;
            if (pSlot->unk50 >= 0xFF) {
                pSlot->unk58 = 0xFF;
                pSlot->unk54 = 0xFF;
                pSlot->unk50 = 0xFF;
                pSlot->unk20 = 0;
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
// D_8009C5B0/D_8009C5B4 loads and the lui 0x1F80 to the top of the block and
// sinks the g_WorldMapPlayerPosition store below the three zero stores, while retail keeps
// statement order and reuses $v1 for all three loads. Same blocker as
// func_800817A0 above; pointer local, zero-store order and x/y/z order were
// all no better (vec3 through a pointer local: 87.03, zeros first: 84.43).
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800819C8);
/*
s32 func_800819C8(s32 index) {
    u8 *pEntry = (u8 *)g_WorldMapGroups;
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->unk20 = 0;
    pSlot->pos.x = g_WorldMapPlayerPosition.x;
    pSlot->pos.y = D_8009C5B0;
    pSlot->pos.z = D_8009C5B4;
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

    if (pSlot->unk4 == 1) {
        pSlot->unk4 = 0;
        pSlot->unk20 = 1;
    }
    if (pSlot->unk20 != 0) {
        if (pSlot->unk20 == 1) {
            pSlot->unk50 += 4;
            pSlot->unk54 += 2;
            pSlot->unk58 += 1;
            if (pSlot->unk50 >= 0xFC) {
                pSlot->unk50 = 0xFC;
                pSlot->unk20 = 0;
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

    D_8009D158 = HeapAlloc(0x1E00, 0);
    D_8009D15C = HeapAlloc(0x1E00, 0);
    g_WorldMapFadeTable = HeapAlloc(0x180, 0);
    p = D_8009D158;
    for (i = 0; i < 0xC0; i++) {
        p[7] = 0x2C;
        p[3] = 9;
        p[4] = 0x80;
        p[5] = 0x80;
        p[6] = 0x80;
        p[7] |= 1;
        *(u16 *)(p + 0x16) = GetTPage(2, 0, 0x280, 0x100);
        p += 0x28;
    }
    pSrc = (Quad *)D_8009D158;
    pDst = (Quad *)D_8009D15C;
    for (i = 0; i < 0x1E0; i++) {
        *pDst++ = *pSrc++;
    }
    pFill = (u16 *)g_WorldMapFadeTable;
    for (i = 0xBF; i >= 0; i--) {
        *pFill++ = 1;
    }
    return 1;
}
*/
// Best: 87.09%. Allocation, both loops' shapes and GetTPage are right; the
// differences are (1) retail keeps &D_8009D158 in $s0 across the three
// HeapAlloc calls (store and later load both go through it), gcc rematerialises
// lui/$at for each; (2) retail materialises 9 into $s4 before 0x2C into $s3.
// Preset screen: cdk 71.46, gcc-2.6.0 87.09.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081C3C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081D80);

s32 WorldMapResetSlotColorFade(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];

    pSlot->unk20 = 0;
    pSlot->unk50 = 1;
    return 1;
}

s32 WorldMapFillFadeTable(s32 index) {
    WorldMapSlot *pSlot = &g_WorldMapSlots[index];
    s32 i;
    u16 *pFill;
    s32 start;
    short value;
    s16 state;

    state = pSlot->unk4 - 1;
    switch (state) {
    case 0:
        pSlot->unk4 = 0;
        pSlot->unk20 = 1;
        break;
    case 1:
        pSlot->unk4 = 0;
        pSlot->unk20 = 2;
        break;
    case 2:
        pSlot->unk4 = 0;
        pSlot->unk20 = 3;
        break;
    case 3:
        pSlot->unk4 = 0;
        pSlot->unk20 = 4;
        break;
    case 4:
        pSlot->unk4 = 0;
        pSlot->unk20 = 5;
        break;
    }
    switch (pSlot->unk20) {
    case 0:
        break;
    case 1:
        pFill = (u16 *)g_WorldMapFadeTable;
        for (i = 0xBF; i >= 0; i--) {
            *pFill++ = pSlot->unk50;
        }
        start = rand() & 0x3F;
        pFill = (u16 *)g_WorldMapFadeTable + start;
        i = start;
        while (i < start + (rand() & 0x1F) + 1) {
            if ((rand() & 3) == 0) {
                *pFill = (rand() & 0x3F) + 1;
            }
            i++;
            pFill++;
        }
        start = (rand() & 0x3F) + 0x40;
        pFill = (u16 *)g_WorldMapFadeTable + start;
        i = start;
        while (i < start + (rand() & 0x1F) + 1) {
            if ((rand() & 3) == 0) {
                *pFill = (rand() & 0x3F) + 1;
            }
            i++;
            pFill++;
        }
        start = (rand() & 0x1F) + 0x80;
        pFill = (u16 *)g_WorldMapFadeTable + start;
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
        pFill = (u16 *)g_WorldMapFadeTable;
        pSlot->unk50 += 1;
        if (pSlot->unk50 >= 0x41) {
            pSlot->unk50 = 0x40;
        }
        for (i = 0xBF; i >= 0; i--) {
            *pFill++ = pSlot->unk50;
        }
        break;
    case 3:
        pFill = (u16 *)g_WorldMapFadeTable;
        pSlot->unk50 -= 1;
        if (pSlot->unk50 < 2) {
            pSlot->unk50 = 2;
        }
        for (i = 0xBF; i >= 0; i--) {
            *pFill++ = pSlot->unk50;
        }
        break;
    case 4:
        pFill = (u16 *)g_WorldMapFadeTable;
        for (i = 0; i < 0xC0; i++) {
            if ((rand() & 3) == 0) {
                *pFill = (rand() & 0x3F) + 1;
            }
            pFill++;
        }
        break;
    case 5:
        pFill = (u16 *)g_WorldMapFadeTable;
        value = 2;
        for (i = 0xBF; i >= 0; i--) {
            *pFill++ = value;
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
    D_8009BBC4 = 1;
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
// g_WorldMapViewRotationY/D_8009BD3C and reads them back with `lh` to build the two `<< 12`
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
    D_8009BE2C = D_8009C5B0;
    g_WorldMapScrollTargetY = D_8009C5B0;
    pSlot->pos.y = D_8009C5B0;
    D_8009BE30 = D_8009C5B4;
    g_WorldMapScrollTargetZ = D_8009C5B4;
    pSlot->pos.z = D_8009C5B4;
    pSlot->unk20 = 0;
    pSlot->unk4 = 0;
    *(s32 *)pSlot->unk5C = 0x500000;
    g_WorldMapCameraRot = -0x220;
    g_WorldMapViewRotationY = 0;
    D_8009BD3C = 0;
    pSlot->unk50 = 0xFFDE0000;
    ((s32 *)pSlot->unk34)[1] = 0xFFDE0000;
    a = g_WorldMapViewRotationY;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapCameraHeight = 0x500000;
    pSlot->unk54 = a << 12;
    ((s32 *)pSlot->unk34)[2] = a << 12;
    b = D_8009BD3C;
    g_WorldMapFadeLevel = 0x78;
    pSlot->unk58 = b << 12;
    ((s32 *)pSlot->unk34)[3] = b << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800828DC);
