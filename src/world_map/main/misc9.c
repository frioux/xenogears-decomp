#include "common.h"
#include "main/game.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

// Entries of the prim-block table g_WorldMapPrimBlocks points at
#define WORLD_MAP_PRIM_BLOCK_COUNT 256
// CLUT ids the tile palette is built from, one GetClut per entry
#define WORLD_MAP_CLUT_ID_COUNT 16
// Tile vertices in g_WorldMapTileVertexBuffer, one path entry each
#define WORLD_MAP_TILE_VERTEX_COUNT 80
// Bit 1 of a primitive code byte: the primitive is drawn semi-transparent
#define WORLD_MAP_PRIM_CODE_SEMI_TRANS 0x2
// The 5x5 window of grid tiles this frame's tile pass fills, and the row stride of the
// level's tile-id table
#define WORLD_MAP_GRID_WINDOW 5
#define WORLD_MAP_GRID_WIDTH 9
// Halfword offset from the scratchpad base at 0x1F800000 to its clut id table
#define WORLD_MAP_SPAD_CLUT_ID_OFFSET 0x34

extern s32 g_WorldMapLevelIndex;
extern u16 g_WorldMapLevelGroupIds[];
extern u_char* g_WorldMapPrimBlocks;
extern u_char* g_WorldMapTileVertexBuffer;
extern u16 g_WorldMapClutIds[];
extern u_char* g_WorldMapSpritePrims[];
extern u_char* g_WorldMapSpritePrimCopy;
extern u_char* g_WorldMapSemiTransPrims[];
extern u_char* g_WorldMapSemiTransPrimCopy;
// Interleaved {parent id, child id} pairs of the sprite-set parent table; the
// pairs are offsets from the level's group base and a parent of -1 ends the list
extern s16 g_WorldMapSpriteSetParentPairs[];
extern u16 g_WorldMapLevelSpriteSetBase[];
extern u16 g_WorldMapTileGroupIds[];
extern u16 D_8009B64E[];
extern u16 g_WorldMapSlotRotationGroupIds[];
extern u16 D_8009B626[];

extern MATRIX g_WorldMapViewMatrix;
extern MATRIX g_WorldMapIdentityMatrix;
extern s16 g_WorldMapViewRotationZ;
extern s16 D_8009BE04;
extern u32* g_WorldMapRenderContext;
extern u_char* g_WorldMapTileVertexSteps;
extern s16 D_8009D618[];
extern s16 g_WorldMapGridTileIds[];
extern s16 g_WorldMapGridOrigin;
extern s16 g_WorldMapGridTileRowOrigin[];
extern s32 g_WorldMapPrimPacketIndex;


void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1);
s32 WorldMapSetSceneLayerTPages(s32 arg0);
void func_80099BFC(u8* pSrc, s32 count, u32 arg2, u8* pPrim);

// 0x80-byte slot in the g_WorldMapSlots table
typedef struct {
    /* 0x00 */ u8 pad0[0x20];
    /* 0x20 */ u16 state; // the per-state handler table switches on it
    /* 0x22 */ u8 pad22[0x6];
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ u8 pad34[0x4];
    /* 0x38 */ s32 dirX;
    /* 0x3C */ s32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ u8 pad44[0x6];
    /* 0x4A */ u16 moveSpeed;
    /* 0x4C */ u8 pad4C[0x4];
    // 0x50..0x5C are reused per slot mode, so no single name fits: the rotation
    // handlers accumulate a 12-bit angle in 0x50 by 0x54 (mask 0xFFF, fed to
    // RotMatrix), while the actor state machine reads 0x50/0x54 as a tile-space
    // target and 0x58/0x5C as its trail index and draw angle.
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ u8 pad60[0x20];
} WorldMapPlayer; // Size: 0x80

// 0x54-byte scene-layer entry in the g_WorldMapGroups table
typedef struct WorldMapSceneLayer {
    /* 0x00 */ s16 hidden; // nonzero: the sprite-set draw pass skips this entry
    /* 0x02 */ s16 partIndex; // index into the model's parts
    /* 0x04 */ u16 flags; // from the sprite-set definition; indexes D_8009AD2C
    /* 0x06 */ u16 pad6;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ u8* pPrimHeader; // header whose +0x4 is the prim count
    /* 0x44 */ u8 unk44[0x4];
    /* 0x48 */ u8* pPrims; // vertex buffer
    /* 0x4C */ u8* pPrimsCopy; // copy destination for pPrims
    /* 0x50 */ struct WorldMapSceneLayer* pParent;
} WorldMapSceneLayer; // Size: 0x54

extern WorldMapPlayer* g_WorldMapSlots;
extern WorldMapSceneLayer* g_WorldMapGroups;


typedef struct {
    /* 0x0 */ s32 offset;
    /* 0x4 */ s32 count;
} WorldMapTableEntry; // Size: 0x8

// 0x10-byte tile position entry in the D_8009AF30 / D_8009AF38 tables
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
    /* 0xC */ s32 unkC;
} WorldMapTilePos; // Size: 0x10

// 0x10-byte vertex built in g_WorldMapTileVertexBuffer
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 unkC;
} WorldMapTileVertex; // Size: 0x10

// 0x8-byte path entry built in g_WorldMapTileVertexSteps
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 pad6;
} WorldMapPathEntry; // Size: 0x8

extern WorldMapTilePos D_8009AF30[];
extern WorldMapTilePos D_8009AF38[];
extern int rand(void);

// The eight waypoint tiles of the current world map zone, in 1/4096 tile units
extern u16 g_WorldMapWaypointTileX[8];
extern u16 g_WorldMapWaypointTileZ[8];

// 0x20 entries of the path direction the actor walks along, ring-indexed by unk54
extern WorldMapPathEntry g_WorldMapPlayerTrail[];

// 12.4 fixed point world map position
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

extern s32 WorldMapGetTilePlaneHeight(s32 x, s32 z);
extern void WorldMapWrapDeltaScaled(WorldMapPos* p);
extern s32 func_80094154(WorldMapPos* p0, WorldMapPos* p1);

void WorldMapSetSlotTPages(WorldMapSceneLayer* pLayer, u8* pSrc, s32 count, s32 abr);

void WorldMapRelocateMapEntryOffsets(void) {
    WorldMapTableEntry* pEntry;
    s32 base;
    s32 i;

    base = (s32)g_WorldMapPrimBlocks;
    pEntry = (WorldMapTableEntry*)g_WorldMapPrimBlocks;
    for (i = 0; i < WORLD_MAP_PRIM_BLOCK_COUNT; i++, pEntry++) {
        if (pEntry->offset != 0) {
            pEntry->offset += base;
        }
    }
    for (i = 0; i < WORLD_MAP_CLUT_ID_COUNT; i++) {
        g_WorldMapClutIds[i] = GetClut(0xF0, i + 0x1F0);
    }
}

// Best: 92.27% (324 bytes = retail's exact size). The ONLY delta is how
// &g_WorldMapSpritePrims is materialised: retail computes it into s0 (lui/addiu), so
// the store of HeapAlloc #1 fills the second call's delay slot (sw v0,0(s0)) and the
// later reload is `lw v1,0(s0)`; we get the assembler's `lui $at / sw v0,sym` macro
// plus a fresh `lui v1 / lw` - the same four instructions, but `li a1,1` takes the
// delay slot instead, which shifts the whole tail. Measured non-solutions, all
// 92.27: `u_char** pp = &g_WorldMapSpritePrims` (declaration initialiser AND
// assigned form), `mem_addr pp = (mem_addr)&g_...` with *(u_char**)pp, and the
// plain symbol below. The 0x200-iteration POLY_FT4 loop and the trailing 0x5000
// copy are byte-exact (the copy must be typed: memcpy of `u_char*` globals emits
// lwl/lwr and scores 60%). WorldMapAllocSemiTransPrimBuffers shows the identical idiom with
// g_WorldMapSemiTransPrims. Screen: cdk 92.21, 2.6.0 92.27, best 92.27 here - not
// a compiler miss.
void WorldMapAllocSpritePrimBuffers(void) {
    POLY_FT4* pPrim;
    POLY_FT4* pDst;
    POLY_FT4* pSrc;
    s32 i;

    g_WorldMapSpritePrims[0] = HeapAlloc(0x5000, 1);
    g_WorldMapSpritePrimCopy = HeapAlloc(0x5000, 1);
    pPrim = (POLY_FT4*)g_WorldMapSpritePrims[0];
    for (i = 0; i < 0x200; i++, pPrim++) {
        setPolyFT4(pPrim);
        setRGB0(pPrim, 0x80, 0x80, 0x80);
        pPrim->u0 = 0;
        pPrim->v0 = 0x40;
        pPrim->u1 = 0x1F;
        pPrim->v1 = 0x40;
        pPrim->u2 = 0;
        pPrim->v2 = 0x6F;
        pPrim->u3 = 0x1F;
        pPrim->v3 = 0x6F;
        setClut(pPrim, 0xF0, 0x1FF);
        setTPage(pPrim, 0, 0, 0x380, 0x100);
    }
    pSrc = (POLY_FT4*)g_WorldMapSpritePrims[0];
    pDst = (POLY_FT4*)g_WorldMapSpritePrimCopy;
    memcpy(pDst, pSrc, 0x5000);
}
void WorldMapFreeSpritePrimBuffers(void) {
    HeapFree(g_WorldMapSpritePrimCopy);
    HeapFree(g_WorldMapSpritePrims[0]);
}
// objdiff reads 99.97% / 644 bytes (retail's exact size, 161 insns) and the whole residual is
// splat's hi/lo inference on the clut copy's loop-back `addiu $v1,$v1,2`: it paired that
// addiu with the `lui $v1,0x1F80` above it and printed `%lo(D_1F800002)`, so the expected
// object holds imm 0 plus an R_MIPS_LO16 to the absolute scratchpad symbol and ours holds the
// literal 2 with no relocation. The linked word is 0x24030002 either way (tools/scripts/check_bin.sh
// world_map: OK). What the body needed, after every round up to 442 blamed a compiler split:
// the loop-invariant g_WorldMapRenderContext[0x1C] read hoisted into its own local (that is
// what kept D_8009D618's address out of a sixth callee-saved register and brought the frame
// back to 0x28), `pDst[0x34 + i]` off the scratchpad base instead of `pDst = 0x1F800068`
// (retail stores at 0x68 off 0x1F800000), the origin pair read as an s16 pointer based at
// g_WorldMapGridTileRowOrigin (displacements 0 and -4, i.e. the column is the s16 below the
// row), `pEntry = pTable - (-id)` for retail's in-place `addu v0,v0,s3`, and `row = 0;` before
// the two pointer statements so the preheader emits `move s1,zero` first.
void WorldMapDrawGridTiles(void) {
    WorldMapTableEntry* pEntry;
    WorldMapTableEntry* pTable;
    s16* pRowOrigin;
    u32 otHead;
    u16* pSrc;
    u16* pDst;
    s32 row;
    s32 id;
    s32 i;

    ((SVECTOR*)0x1F800000)->vx = -0x18;
    ((SVECTOR*)0x1F800000)->vy = -0x48;
    ((SVECTOR*)0x1F800000)->vz = 0;
    ((SVECTOR*)0x1F800008)->vx = 0x18;
    ((SVECTOR*)0x1F800008)->vy = -0x48;
    ((SVECTOR*)0x1F800008)->vz = 0;
    ((SVECTOR*)0x1F800010)->vx = -0x18;
    ((SVECTOR*)0x1F800010)->vy = 0;
    ((SVECTOR*)0x1F800010)->vz = 0;
    ((SVECTOR*)0x1F800018)->vx = 0x18;
    ((SVECTOR*)0x1F800018)->vy = 0;
    ((SVECTOR*)0x1F800018)->vz = 0;
    *(MATRIX*)0x1F800028 = g_WorldMapViewMatrix;
    *(MATRIX*)0x1F800048 = g_WorldMapIdentityMatrix;
    RotMatrixZ(-g_WorldMapViewRotationZ, (MATRIX*)0x1F800048);
    pSrc = g_WorldMapClutIds;
    pDst = (u16*)0x1F800000;
    for (i = 0; i < WORLD_MAP_CLUT_ID_COUNT; i++) {
        pDst[WORLD_MAP_SPAD_CLUT_ID_OFFSET + i] = *pSrc++;
    }
    D_8009BE04 = 0;
    row = 0;
    pRowOrigin = g_WorldMapGridTileRowOrigin;
    pTable = (WorldMapTableEntry*)g_WorldMapPrimBlocks;
    for (; row < WORLD_MAP_GRID_WINDOW; row++) {
        for (i = 0; i < WORLD_MAP_GRID_WINDOW; i++) {
            if (D_8009D618[row * WORLD_MAP_GRID_WINDOW + i] == -1) {
                continue;
            }
            // the origin pair is addressed from the row; the column is the s16 below it
            id = g_WorldMapGridTileIds[(row + pRowOrigin[0]) * WORLD_MAP_GRID_WIDTH + i + pRowOrigin[-2]];
            pEntry = pTable - (-id);
            if (pEntry->count != 0) {
                otHead = g_WorldMapRenderContext[0x1C];
                func_80099BFC((u8*)pEntry->offset, pEntry->count, otHead,
                              g_WorldMapSpritePrims[g_WorldMapPrimPacketIndex] + D_8009BE04 * 0x28);
            }
        }
    }
}


// Best: 93.98% (392 bytes = retail's exact size; the instruction multiset is identical, so
// the whole residual is the s2/s3 mirror: retail keeps pVertex in s3 and x in s2, this body the
// reverse, which is also what moves addiu s1,pVertex+4 past move s0,zero and the pVertex
// increment up behind the x store. Both Rand loops, the 0x54/0x50 bounds, the
// (D_8009AF30[i]/D_8009AF38[i]) strength-reduced strides and the double HeapAlloc prologue are
// exact. Round 442 measured the cause with the -da/-dl dumps and it is NOT a spelling question:
// the allocnos are processed 133 138 79 75 130 72 76 73 77 74, i.e. by
// floor_log2(refs)*refs/live, and 79 = pVertex (16 refs / 34 insns = 1.88) sits ahead of 75 = x
// (14 refs / 28 insns = 1.50), so pVertex takes s2 and x is pushed to s3. The mirror needs x at
// 16 refs (one floor_log2 step) or 8 fewer live insns; no spelling of this body changes either.
// Measured on the way: 2.6.0 93.78, cdk 81.80, loop 1's value written inline (retail negates in
// place in v0) 91.28, and the earlier permuter runs 926->721 with no ZERO.
// Best: 93.98% (392 bytes = retail's exact size, 98/98 insns, identical multiset). Re-measured round 470:
// the ONLY rows are the s2/s3 mirror (pVertex x, retail's pVertex s3) plus the loop-2 value
// pseudo (retail s2 = x's freed home, ours v1) and the two artefacts of that: ours emits
// negu v1,v0 / sll v0,v1,3 where retail negates in place, and the outer back-edge delay slot is
// nop where retail has a SECOND copy of x=0 (move s2,zero). That duplicate init is one more
// candidate explanation for the mirror than the earlier rounds' quotient note: retail's x is
// referenced twice by initialisers. Measured this round at 93.98 / 392: the middle loop as a
// do/while body, and x reset at the END of the outer body with for(;x<4;x++) (that one is 90.87 /
// 404, gcc emits the duplicate init but three instructions worse). Removing the redundant
// standalone x = 0 before the loops is byte-identical, so the body below is the shorter spelling.
// Best: 93.98% (392 bytes = retail's exact size, 98/98 insns, identical multiset). Round 471 re-measured the park: unchanged. Lever read: a-a-near-miss-allocno-quotient says a 4% quotient gap is two instructions of live range; this pair is 1.88 (pVertex, 16 refs/34 insns) against 1.50 (x, 14 refs/28 insns), a 25% gap, and the round-442 -da/-dl dump already pins every term to retail's own instruction stream - park on the arithmetic, do not re-spell.
/*
void func_800863E0(void) {
    WorldMapPathEntry* pPath;
    s32 xoff;
    s32 yoff;
    s32 x;
    s32 y;
    s32 i;
    s32 value;
    WorldMapTileVertex* pVertex;

    g_WorldMapTileVertexBuffer = HeapAlloc(0x500, 0);
    g_WorldMapTileVertexSteps = HeapAlloc(0x280, 0);
    pVertex = (WorldMapTileVertex*)g_WorldMapTileVertexBuffer;
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            xoff = x << 11;
            yoff = y << 11;
            for (i = 0; i < 5; i++) {
                pVertex->x = (D_8009AF30[i].unk0 + xoff) << 12;
                pVertex->z = (D_8009AF38[i].unk0 + yoff) << 12;
                value = -(rand() >> 10);
                pVertex->y = ((value << 3) - 0x200) << 12;
                pVertex++;
            }
        }
    }
    pPath = (WorldMapPathEntry*)g_WorldMapTileVertexSteps;
    for (i = 0; i < 0x50; i++) {
        value = (rand() & 3) + 1;
        pPath->x = value * 3547;
        pPath->z = -(value << 11);
        pPath->unk2 = rand() & 1;
        pPath++;
    }
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800863E0);

void WorldMapFreeTileVertexBuffers(void) {
    HeapFree(g_WorldMapTileVertexSteps);
    HeapFree(g_WorldMapTileVertexBuffer);
}
// The array form of the extern is deliberate: it is what makes gcc materialise the
// global's ADDRESS into a callee-saved register (retail: lui/addiu into s0, store of
// HeapAlloc #1 in the second call's delay slot, reload through s0) instead of the
// assembler's lui $at / sw %lo(sym) macro. Same lever as WorldMapAllocSpritePrimBuffers above.
void WorldMapAllocSemiTransPrimBuffers(void) {
    POLY_FT4* pPrim;
    POLY_FT4* pSrc;
    POLY_FT4* pDst;
    s32 i;
    s32 primCode;
    s32 primShade;
    s32 primLen;

    g_WorldMapSemiTransPrims[0] = HeapAlloc(0x2D00, 1);
    g_WorldMapSemiTransPrimCopy = HeapAlloc(0x2D00, 1);
    pPrim = (POLY_FT4*)g_WorldMapSemiTransPrims[0];
    // The three packet values are load-bearing locals: gcc hoists a value that is
    // assigned in the loop into a callee-saved register, which is how retail gets
    // 0x26/9/0x2C into s3/s5/s4 and a 0x30 frame; literals leave one of them as an
    // in-loop li (frame 0x28, 53%).
    primShade = 0x26;
    for (i = 0; i < 0x120; i++, pPrim++) {
        primLen = 9;
        primCode = 0x2C;
        setlen(pPrim, primLen);
        setcode(pPrim, primCode);
        setRGB0(pPrim, primShade, primShade, primShade);
        pPrim->tpage = GetTPage(0, 1, 0x3C0, 0x100);
        pPrim->clut = GetClut(0x130, 0x1FE);
        SetSemiTrans(pPrim, 1);
    }
    pSrc = (POLY_FT4*)g_WorldMapSemiTransPrims[0];
    pDst = (POLY_FT4*)g_WorldMapSemiTransPrimCopy;
    memcpy(pDst, pSrc, 0x2D00);
}

void WorldMapFreeSemiTransPrimBuffers(void) {
    HeapFree(g_WorldMapSemiTransPrimCopy);
    HeapFree(g_WorldMapSemiTransPrims[0]);
}

void WorldMapWrapTileVertexPositions(void) {
    WorldMapTileVertex* pDst = (WorldMapTileVertex*)g_WorldMapTileVertexBuffer;
    WorldMapPathEntry* pSrc = (WorldMapPathEntry*)g_WorldMapTileVertexSteps;
    s32 i;

    for (i = 0; i < WORLD_MAP_TILE_VERTEX_COUNT; i++) {
        s32 x = pDst[i].x + pSrc[i].x;
        s32 y = pDst[i].z + pSrc[i].z;

        if (x > 0x1FFFFFF) {
            x -= 0x2000000;
        }
        if (x < 0) {
            x += 0x2000000;
        }
        if (y > 0x1FFFFFF) {
            y -= 0x2000000;
        }
        if (y < 0) {
            y += 0x2000000;
        }
        pDst[i].x = x;
        pDst[i].z = y;
    }
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80086798);
s32 WorldMapInitSlotAngleStep8(s32 index) {
    WorldMapPlayer* pSlot = &g_WorldMapSlots[index];

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
s32 WorldMapUpdateSlotRotationMatrices(s32 index) {
    WorldMapPlayer* pSlot = &g_WorldMapSlots[index];
    SVECTOR* pVec0 = (SVECTOR*)0x1F8000A0;
    SVECTOR* pVec1 = (SVECTOR*)0x1F8000A8;
    WorldMapSceneLayer* pLayers;
    u16 angle;

    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & WORLD_MAP_ANGLE_MASK;
    pLayers = g_WorldMapGroups;
    pVec1->vx = 0;
    pVec0->vx = 0;
    pVec0->vy = pLayers[12].rot.vy;
    pVec1->vy = pLayers[14].rot.vy;
    angle = pSlot->unk50;
    pVec1->vz = angle;
    pVec0->vz = angle;
    RotMatrix(pVec0, &pLayers[12].mat);
    RotMatrix(pVec1, &pLayers[14].mat);
    return 1;
}
s32 func_800877E0(s32 index) {
    WorldMapPlayer* pSlot = &g_WorldMapSlots[index];

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
s32 WorldMapUpdateSceneLayerRotationMatrices(s32 index) {
    WorldMapPlayer* pSlot = &g_WorldMapSlots[index];
    WorldMapSceneLayer* pLayers = g_WorldMapGroups;
    s32 entryIndex = g_WorldMapLevelIndex * 2;
    s32 index1 = g_WorldMapSlotRotationGroupIds[entryIndex];
    s32 index2 = D_8009B626[entryIndex];
    SVECTOR* pVec0 = (SVECTOR*)0x1F8000A0;
    SVECTOR* pVec1 = (SVECTOR*)0x1F8000A8;
    WorldMapSceneLayer* pLayer1;
    WorldMapSceneLayer* pLayer2;
    u16 angle;
    long tmp[2]; // unused: reproduces retail's 0x20-byte stack frame

    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & WORLD_MAP_ANGLE_MASK;
    pLayer1 = &pLayers[index1];
    pVec1->vx = 0;
    pVec0->vx = 0;
    pVec0->vy = pLayer1->rot.vy;
    pLayer2 = &pLayers[index2];
    pVec1->vy = pLayer2->rot.vy;
    angle = pSlot->unk50;
    pVec1->vz = angle;
    pVec0->vz = angle;
    RotMatrix(pVec0, &pLayer1->mat);
    RotMatrix(pVec1, &pLayer2->mat);
    return 1;
}
void WorldMapSetSlotTPages(WorldMapSceneLayer* pLayer, u8* pSrc, s32 count, s32 abr) {
    long tmp[2]; // unused: reproduces retail's 0x30-byte stack frame
    POLY_FT4* pPrim;
    s32 rows = count;
    s32 tpageAbr = abr;
    s32 i = 0;

    if (rows > 0) {
        pPrim = (POLY_FT4*)pSrc;
        do {
            pPrim->tpage = GetTPage(0, tpageAbr, 0x1A0, 0xA0);
            pPrim->code |= WORLD_MAP_PRIM_CODE_SEMI_TRANS;
            pPrim++;
        } while (++i < rows);
    }
    memcpy(pLayer->pPrimsCopy, pLayer->pPrims, rows * sizeof(POLY_FT4));
}
s32 BattleInitSlotRowsForActor(s32 index) {
    WorldMapPlayer* pSlot = &g_WorldMapSlots[index];

    pSlot->unk50 = 0;
    pSlot->unk54 = 0x10;
    WorldMapSetSceneLayerTPages(index);
    return 1;
}
s32 WorldMapSetSceneLayerTPages(s32 index) {
    WorldMapSceneLayer* pLayers = g_WorldMapGroups;
    WorldMapSceneLayer* pLayer1;
    WorldMapSceneLayer* pLayer2;
    s32 entryIndex;
    long tmp[2]; // unused: reproduces retail's 0x10-byte stack slot

    entryIndex = g_WorldMapLevelIndex * 2;
    pLayer1 = &pLayers[g_WorldMapTileGroupIds[entryIndex]];
    pLayer2 = &pLayers[D_8009B64E[entryIndex]];
    WorldMapSetSlotTPages(pLayer1, pLayer1->pPrims, ((u16*)pLayer1->pPrimHeader)[2], 3);
    WorldMapSetSlotTPages(pLayer2, pLayer2->pPrims, ((u16*)pLayer2->pPrimHeader)[2], 3);
    return 1;
}
s32 WorldMapSetSceneLayerRotMatrices(s32 index) {
    WorldMapPlayer* pSlot = &g_WorldMapSlots[index];
    WorldMapSceneLayer* pLayers = g_WorldMapGroups;
    s32 entryIndex = g_WorldMapLevelIndex * 2;
    s32 index1 = g_WorldMapTileGroupIds[entryIndex];
    s32 index2 = D_8009B64E[entryIndex];
    SVECTOR* pVec0 = (SVECTOR*)0x1F8000A0;
    SVECTOR* pVec1 = (SVECTOR*)0x1F8000A8;
    WorldMapSceneLayer* pLayer1 = &pLayers[index1];
    WorldMapSceneLayer* pLayer2;
    u16 angle;
    long tmp[2]; // unused: reproduces retail's 0x20-byte stack frame

    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & WORLD_MAP_ANGLE_MASK;
    pVec1->vz = 0;
    pVec0->vz = 0;
    pVec1->vx = 0;
    pVec0->vx = 0;
    angle = pSlot->unk50;
    pVec1->vy = angle;
    pVec0->vy = angle;
    pLayer2 = &pLayers[index2];
    RotMatrix(pVec0, &pLayer1->mat);
    RotMatrix(pVec1, &pLayer2->mat);
    return 1;
}

void WorldMapBuildBasisMatrix(VECTOR* pVec, VECTOR* pWork, MATRIX* pMat) {
    pWork->vz = 0;
    pWork->vx = 0;
    pWork->vy = 0x1000;
    OuterProduct12(pWork, pVec, pWork);
    VectorNormal(pWork, pWork);
    pMat->m[0][0] = pWork->vx;
    pMat->m[0][1] = pWork->vy;
    pMat->m[0][2] = pWork->vz;
    OuterProduct12(pVec, pWork, pWork);
    VectorNormal(pWork, pWork);
    pMat->m[1][0] = pWork->vx;
    pMat->m[1][1] = pWork->vy;
    pMat->m[1][2] = pWork->vz;
    pMat->m[2][0] = pVec->vx;
    pMat->m[2][1] = pVec->vy;
    pMat->m[2][2] = pVec->vz;
    TransposeMatrix(pMat, pMat);
}
// Best: 97.30% (756 bytes = retail's exact size; the ONLY remaining delta is a
// register assignment: gcc gives pPos $s3 and the g_WorldMapWaypointTileZ base $s2 where retail
// has pPos $s2 and the AF90 base $s3 - same instruction multiset, same operands;
// permuter 365 iterations from this body (base=200 best=200) and every declaration
// order tried do not move it).
// What closed the round-117 residuals, in case a neighbour needs the trick:
//  (a) prologue order (retail: addiu sp / sw s0 BEFORE the g_WorldMapLevelIndex load, then
//      ra,s3,s2,s1) is NOT controllable on its own - it falls out of the same block
//      boundary as (b) and is correct here.
//  (b) the stolen load-delay nop after the unk2C store needs the `do { } while (0)`
//      below to include the func_80094154 CALL as well as the two scratchpad stores.
// Measured alternatives at 756 bytes: do/while over the two stores only = 92.72
// (hoists the g_WorldMapWaypointTileX/g_WorldMapWaypointTileZ base-register formations above the call, which
// pushes pPos out of s2); do/while starting at the unk2C store = 89.16 (760 bytes);
// do/while over the post-call pPos->x/tileZ/pPos->z group = 88.62.
// Round 405: re-measured 97.30 (756 bytes, identical instruction multiset). The mirror is two
// pseudos: retail wants pPos (0x1F800000) in s2 and the g_WorldMapWaypointTileZ base in s3. The -da
// global-alloc dump proves it is an allocno ORDER question and nothing else: the callee-saved
// hand-out is 73 in 16 s0, 139 in 17 s1 (g_WorldMapWaypointTileX base), 147 in 18 s2 (g_WorldMapWaypointTileZ base),
// 75 in 19 s3 (pPos), i.e. pPos is processed LAST of the four although it has the most refs,
// because its live range spans the whole else branch. A retry would have to shorten pPos's
// range without moving its lui s3,0x1f80 (retail has that instruction in the same slot),
// which no declaration or assignment move can do. Dump recipe: cc1 -O2 -G8 -da on
// build/src/world_map/main/misc9.c.i; 2.7.2 has no -dl, so the ref counts stay unread.
// Round 429 (re-measured 97.30 / 756 bytes, 189/189 insns, the same 34-line s2/s3 swap): the
// other two axes measured and closed. Preset: gcc-2.6.0 94.81, -O3 97.30, gcc-2.7.2-cdk
// compile-failed (cc1 rejects the mixed declarations in this unit), so the unit's
// XenoWorldMap preset is right for this function. Spelling: a bare `{ }` block in place of the
// load-bearing do/while(0) measures 96.77 at 752 bytes (it loses retail's load-delay nop), and
// moving `pPos = (WorldMapPos*)0x1F800000;` into the else branch measures 92.09 at 772 (the lui
// leaves retail's slot right after the first call). Reordering the locals (pPos declared first)
// and replacing the `pCount` local with direct g_GameState accesses both measure 97.30 / 756 with
// the identical 34-line swap, which is the lever a-lreg-dump prediction: the hand-out is not
// sensitive to pseudo numbering here.
// Best: 97.30% (756 bytes = retail's exact size, 189/189 insns, identical multiset). The only
// residual is the s2/s3 mirror: retail keeps pPos (0x1F800000) in s2 and the
// g_WorldMapWaypointTileZ base in s3, this body the reverse (plus the two prologue lines that
// swap with it: retail emits addiu sp/sw s0 before the first lui). Earlier rounds closed the
// other axes: gcc-2.6.0 94.81, -O3 97.30, cdk compile-fails, a bare {} block for the
// do/while(0) 96.77/752, pPos assigned inside the else 92.09/772, every local reorder 97.30.
// Round 442 quantified the -da dump it is an allocno ORDER question: the four callee-saved
// candidates are processed 73 (34 refs / 114 insns = 5.96) -> s0, 139 = X base (5 / 13 = 3.08)
// -> s1, 147 = Z base (5 / 20 = 2.00) -> s2, 75 = pPos (17 / 182 = 1.49) -> s3, i.e. by
// floor_log2(refs)*refs/live*size. pPos's live range is the whole function (its lui is in the
// entry block, where retail has it too), so overtaking the Z base needs live <= 136 or the Z
// base live >= 27 - neither is a spelling: every one of these 189 instructions is already
// retail's. Dump recipe: cc1 -O2 -G8 -da -dg on build/src/world_map/main/misc9.c.i, then
// m.i.cse2 for the pseudo ids (139/147 are the two symbol_refs, 75 is the scratchpad).
/*
s32 func_80087C6C(s32 index) {
    WorldMapPlayer* pSlot;
    WorldMapSceneLayer* pLayer;
    WorldMapPos* pPos;
    u16* pCount;
    s32 tileIndex;
    s32 dist;
    s32 i;
    s32 z;
    u16 tileZ;

    tileIndex = g_WorldMapLevelGroupIds[g_WorldMapLevelIndex];
    WorldMapSetSceneSpriteSetParents();
    pSlot = &g_WorldMapSlots[index];
    pPos = (WorldMapPos*)0x1F800000;
    pSlot->moveSpeed = 0x4000;
    pLayer = &g_WorldMapGroups[tileIndex];
    pSlot->state = 0;
    if (g_GameState.fieldScriptMemory[0] < 0xCD) {
        pSlot->x = 0xD80000;
        pSlot->z = 0x7280000;
        pLayer->mat = g_WorldMapIdentityMatrix;
        pSlot->y = WorldMapGetTilePlaneHeight(pSlot->x, pSlot->z);
    } else {
        pCount = &g_GameState.playerSlotInitCount;
        if (*pCount == 0) {
            (*pCount)++;
            pSlot->unk50 = 0;
            pSlot->x = g_WorldMapWaypointTileX[pSlot->unk50] << 12;
            z = g_WorldMapWaypointTileZ[pSlot->unk50];
        } else {
            pSlot->unk50 = g_GameState.playerTileIndex;
            pSlot->x = g_GameState.playerMapPosX << 12;
            z = g_GameState.playerMapPosZ;
        }
        pSlot->z = z << 12;
        pSlot->y = WorldMapGetTilePlaneHeight(pSlot->x, pSlot->z);
        do {
            pPos->x = g_WorldMapWaypointTileX[pSlot->unk50] << 12;
            pPos->z = g_WorldMapWaypointTileZ[pSlot->unk50] << 12;
            dist = func_80094154(&pSlot->x, pPos);
        } while (0);
        if (dist < 0) {
            dist = -dist;
        }
        if (dist < 8) {
            pSlot->unk50 = (pSlot->unk50 + 1) & 7;
        }
        pPos->x = g_WorldMapWaypointTileX[pSlot->unk50] - (pSlot->x >> 12);
        tileZ = g_WorldMapWaypointTileZ[pSlot->unk50];
        pPos->z = tileZ - (pSlot->z >> 12);
        pPos->y = 0;
        WorldMapWrapDeltaScaled(pPos);
        VectorNormal((VECTOR*)pPos, (VECTOR*)pPos);
        for (i = 0; i < 0x100; i += 8) {
            ((WorldMapPathEntry*)((u8*)g_WorldMapPlayerTrail + i))->x = pPos->x;
            ((WorldMapPathEntry*)((u8*)g_WorldMapPlayerTrail + i))->z = pPos->z;
        }
        pSlot->unk58 = 1;
        pSlot->unk54 = 0;
        pSlot->dirX = g_WorldMapPlayerTrail[1].x;
        pSlot->dirZ = g_WorldMapPlayerTrail[pSlot->unk58].z;
        pSlot->dirY = 0;
    }
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087C6C);
s32 WorldMapSetSceneSpriteSetParents(void) {
    s32 value = g_WorldMapLevelGroupIds[g_WorldMapLevelIndex];

    WorldMapSetSpriteSetParent(value, value - 4);
    WorldMapSetSpriteSetParent(value, value - 3);
    WorldMapSetSpriteSetParent(value, value - 2);
    WorldMapSetSpriteSetParent(value, value - 1);
    return 1;
}
void WorldMapWrapPosition(WorldMapPos* p);
void WorldMapExtractRotationFromMatrix(MATRIX* pMtx, SVECTOR* pRot);
void WorldMapInitSpriteGroupSprites(s32 arg0, void* pSrc, void* pRot);
void WorldMapDeactivateSpriteGroup(s32 arg0);
void WorldMapQueueZoneEntry(s32 arg0, s32* pVec, s32 arg2, s32 arg3);

// 99.92% and byte-identical (check_bin OK): the two differing rows are splat's
// symbol names for the scratchpad addends (expected/ holds `addiu s0,s4,%lo(D_1F800150)`
// where this body folds the same 0x150 into the immediate), and naming the symbols in C
// costs a `lui`+`addiu` each. Two spellings here are load-bearing: the pPos assignment
// comes first (that alone took the body from 96.81 to 99.75 by letting retail's `lui s4`
// and the g_WorldMapSlots load sit in the prologue, and it is what leaves v0 live for the
// branch-delay `addu s2`), and the sprite-group call passes its scratchpad address as
// `(u32)pPos | 0xA0` because retail encodes `ori a1,s4,0xA0` there.
s32 WorldMapUpdatePlayerSlot(s32 index) {
    WorldMapPlayer* pSlot;
    WorldMapSceneLayer* pLayer;
    WorldMapPos* pPos;
    WorldMapPos* pSlotPos;
    SVECTOR* pRot;
    s32 tileIndex;
    s32 tileDist;
    s32 dist;

    pPos = (WorldMapPos*)0x1F800000;
    tileIndex = g_WorldMapLevelGroupIds[g_WorldMapLevelIndex];
    pSlot = &g_WorldMapSlots[index];
    pLayer = &g_WorldMapGroups[tileIndex];
    if (g_GameState.fieldScriptMemory[0] < 0xCD) {
        pSlot->y = WorldMapGetTilePlaneHeight(pSlot->x, pSlot->z);
        pLayer->x = pSlot->x >> 12;
        pLayer->y = pSlot->y >> 12;
        pLayer->z = pSlot->z >> 12;
    } else {
        pSlotPos = (WorldMapPos*)&pSlot->x;
        pPos->x = g_WorldMapWaypointTileX[pSlot->unk50] << 12;
        pPos->z = g_WorldMapWaypointTileZ[pSlot->unk50] << 12;
        tileDist = func_80094154(pSlotPos, pPos);
        if (tileDist < 0) {
            tileDist = -tileDist;
        }
        if (tileDist < 8) {
            pSlot->unk50 = (pSlot->unk50 + 1) & 7;
        }
        pPos->x = g_WorldMapWaypointTileX[pSlot->unk50] - (pSlot->x >> 12);
        pPos->z = g_WorldMapWaypointTileZ[pSlot->unk50] - (pSlot->z >> 12);
        pPos->y = 0;
        WorldMapWrapDeltaScaled(pPos);
        VectorNormal((VECTOR*)pPos, (VECTOR*)pPos);
        pSlot->dirX = ((pPos->x + pSlot->dirX * 63) << 6) >> 12;
        pSlot->dirZ = ((pPos->z + pSlot->dirZ * 63) << 6) >> 12;
        VectorNormal((VECTOR*)&pSlot->dirX, (VECTOR*)&pSlot->dirX);
        g_WorldMapPlayerTrail[pSlot->unk54].x = pSlot->dirX;
        g_WorldMapPlayerTrail[pSlot->unk54].z = pSlot->dirZ;
        pSlot->unk54 = (pSlot->unk54 + 1) & 0x1F;
        pPos->x = pSlot->dirX;
        pPos->y = pSlot->dirY;
        pPos->z = -pSlot->dirZ;
        WorldMapBuildBasisMatrix((VECTOR*)pPos, (VECTOR*)0x1F800010, (MATRIX*)0x1F8000F0);
        pLayer->mat = *(MATRIX*)0x1F8000F0;
        pSlot->x += g_WorldMapPlayerTrail[pSlot->unk58].x * ((s16)pSlot->moveSpeed >> 12);
        pSlot->z += g_WorldMapPlayerTrail[pSlot->unk58].z * ((s16)pSlot->moveSpeed >> 12);
        pSlot->unk58 = (pSlot->unk58 + 1) & 0x1F;
        WorldMapWrapPosition(pSlotPos);
        pSlot->y = WorldMapGetTilePlaneHeight(pSlot->x, pSlot->z);
        pLayer->x = pSlot->x >> 12;
        pLayer->y = pSlot->y >> 12;
        pLayer->z = pSlot->z >> 12;
        pPos->x = *(u16*)&g_GameState.scriptPosX - (pSlot->x >> 12);
        pPos->z = *(u16*)&g_GameState.scriptPosZ - (pSlot->z >> 12);
        pPos->y = g_GameState.scriptPosY;
        WorldMapWrapDeltaScaled(pPos);
        dist = SquareRoot0(pPos->x * pPos->x + pPos->z * pPos->z);
        if (dist < 0x100 && pPos->y >= -0xBF) {
            pSlot->moveSpeed = 0;
        } else if (dist < 0x300 && pPos->y >= -0xBF) {
            pSlot->moveSpeed -= 0x100;
            if ((s16)pSlot->moveSpeed < 0) {
                pSlot->moveSpeed = 0;
            }
        } else {
            pSlot->moveSpeed += 0x200;
            if ((s16)pSlot->moveSpeed > 0x4000) {
                pSlot->moveSpeed = 0x4000;
            }
        }
        if ((s16)pSlot->moveSpeed >= 0x1001) {
            pRot = (SVECTOR*)((u8*)pPos + 0xA0);
            pRot->vx = pLayer->x;
            pRot->vy = pLayer->y;
            pRot->vz = pLayer->z;
            TransposeMatrix(&pLayer->mat, (MATRIX*)((u8*)pPos + 0x150));
            WorldMapExtractRotationFromMatrix((MATRIX*)((u8*)pPos + 0x150), (SVECTOR*)((u8*)pPos + 0xA8));
            // the scratchpad's low half is zero, so `|` is the offset retail encodes with ori
            WorldMapInitSpriteGroupSprites(0x13, (SVECTOR*)((u32)pPos | 0xA0), (SVECTOR*)((u8*)pPos + 0xA8));
        } else {
            WorldMapDeactivateSpriteGroup(0x13);
        }
    }
    g_GameState.playerMapPosX = pSlot->x >> 12;
    g_GameState.playerMapPosZ = pSlot->z >> 12;
    g_GameState.playerTileIndex = pSlot->unk50;
    pPos->x = pSlot->x;
    pPos->y = pSlot->y + 0x30000;
    pPos->z = pSlot->z;
    WorldMapQueueZoneEntry(index, (s32*)pPos, 0x80, 0xB0);
    return 1;
}
s32 WorldMapInitializePlayerSlot(s32 index) {
    WorldMapPlayer* pSlot;
    u16* pCount;
    s32 posZ;
    s32 posX;

    WorldMapSetLevelSpriteSetParents();
    pSlot = &g_WorldMapSlots[index];
    pSlot->unk54 = 4;
    pSlot->state = 0;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0;
    pSlot->unk5C = 0xC;
    pCount = &g_GameState.playerPlaced;
    if (*pCount == 0) {
        do {
            (*pCount)++;
        } while (0);
        // posZ carries the y constant first: sharing one variable stops gcc
        // hoisting the 0x4800000 lui above the y store.
        posZ = 0xFFD80000;
        pSlot->y = posZ;
        pSlot->x = 0;
        posZ = 0x4800000;
    } else {
        s32 mapX = *(s16*)&g_GameState.mapPosX;
        u32 fracX = g_GameState.mapPosXFraction;

        pSlot->y = 0xFFD80000;
        pSlot->x = mapX * 0x1000 + fracX;
        do {
            posZ = *(s16*)&g_GameState.mapPosZ * 0x1000 + g_GameState.mapPosZFraction;
        } while (0);
    }
    pSlot->z = posZ;
    do {
    } while (0); // keeps the x load below the z store
    posX = pSlot->x;
    pSlot->dirZ = 0xB50;
    pSlot->dirX = 0xB50;
    pSlot->dirY = 0;
    pSlot->moveSpeed = 1;
    g_GameState.mapPosXFraction = posX;
    g_GameState.mapPosX = pSlot->x >> 12;
    g_GameState.mapPosZFraction = pSlot->z;
    g_GameState.mapPosZ = pSlot->z >> 12;
    return 1;
}

s32 WorldMapSetLevelSpriteSetParents(void) {
    s32 base = g_WorldMapLevelSpriteSetBase[g_WorldMapLevelIndex];
    s32 i = 0;

    while (g_WorldMapSpriteSetParentPairs[i * 2] != -1) {
        WorldMapSetSpriteSetParent(base + g_WorldMapSpriteSetParentPairs[i * 2], base + g_WorldMapSpriteSetParentPairs[i * 2 + 1]);
        i++;
    }
    return 1;
}
// Four-word position quad copied whole out of the scratchpad
 typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapPos4; // Size: 0x10

// One entry of the g_WorldMapGroups table (stride 0x54); this unit only touches the
// position quad and the matrix a rotation step propagates down the entry chain.
typedef struct {
    /* 0x00 */ u8 pad0[0x8];
    /* 0x08 */ WorldMapPos4 pos;
    /* 0x18 */ u8 pad18[0x8];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ u8 pad40[0x14];
} WorldMapEntry; // Size: 0x54

s32 func_80088720(s32 index) {
    WorldMapPlayer* pSlot;
    WorldMapEntry* pEntries;
    WorldMapEntry* pParent;
    WorldMapEntry* pEntry;
    WorldMapPos* pPos;
    WorldMapPos* pSlotPos;
    SVECTOR* pRot;
    s32 tileIndex;
    s32 dist;

    pSlot = &g_WorldMapSlots[index];
    tileIndex = g_WorldMapLevelSpriteSetBase[g_WorldMapLevelIndex];
    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & 0xFFF;
    pSlot->unk58 = (pSlot->unk58 + pSlot->unk5C) & 0xFFF;
    // the two Y-rotation source vectors, in scratchpad
    pRot = (SVECTOR*)0x1F8000A0;
    pRot[0].vz = 0;
    pRot[0].vx = 0;
    pRot[0].vy = pSlot->unk50;
    pRot[1].vz = 0;
    pRot[1].vx = 0;
    pRot[1].vy = pSlot->unk58;
    RotMatrixYXZ(pRot, (MATRIX*)0x1F8000F0);
    RotMatrixYXZ(pRot + 1, (MATRIX*)0x1F800110);
    // the global is re-read per copy run: one pointer local makes gcc CSE the load
    // and hand the base to a register retail does not use
    pEntries = (WorldMapEntry*)(tileIndex * sizeof(WorldMapEntry) + (u32)g_WorldMapGroups);
    pEntries[5].matrix = *(MATRIX*)0x1F8000F0;
    pParent = (WorldMapEntry*)(tileIndex * sizeof(WorldMapEntry) + (u32)g_WorldMapGroups);
    pParent[3].matrix = *(MATRIX*)0x1F800110;
    pParent[2].matrix = pParent[3].matrix;
    pParent[1].matrix = pParent[2].matrix;
    pParent[0].matrix = pParent[1].matrix;
    pPos = (WorldMapPos*)0x1F800000;
    pPos->x = *(u16*)&g_GameState.scriptPosX - (pSlot->x >> 12);
    pPos->z = *(u16*)&g_GameState.scriptPosZ - (pSlot->z >> 12);
    pPos->y = g_GameState.scriptPosY;
    WorldMapWrapDeltaScaled(pPos);
    dist = SquareRoot0(pPos->x * pPos->x + pPos->z * pPos->z);
    if (dist < 0x300 && pPos->y < -0x240) {
        pSlot->moveSpeed = 0;
    } else {
        pSlot->moveSpeed = 1;
    }
    pSlotPos = (WorldMapPos*)&pSlot->x;
    pSlot->x += pSlot->dirX * (s16)pSlot->moveSpeed;
    pSlot->z += pSlot->dirZ * (s16)pSlot->moveSpeed;
    WorldMapWrapPosition(pSlotPos);
    pPos->x = pSlot->x >> 12;
    pPos->y = pSlot->y >> 12;
    pPos->z = pSlot->z >> 12;
    pEntry = (WorldMapEntry*)(tileIndex * sizeof(WorldMapEntry) + (u32)g_WorldMapGroups);
    pEntry[12].pos = *(WorldMapPos4*)pPos;
    WorldMapQueueZoneEntry(index, (s32*)pSlotPos, 0x180, 0xC0);
    g_GameState.mapPosXFraction = pSlot->x;
    g_GameState.mapPosX = pSlot->x >> 12;
    g_GameState.mapPosZFraction = pSlot->z;
    g_GameState.mapPosZ = pSlot->z >> 12;
    return 1;
}


