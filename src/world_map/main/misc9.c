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

extern s32 g_WorldMapLevelIndex;
extern u16 g_WorldMapLevelGroupIds[];
extern u_char* g_WorldMapPrimBlocks;
extern u_char* g_WorldMapTileVertexBuffer;
extern u16 g_WorldMapClutIds[];
extern u_char* g_WorldMapSpritePrims[];
extern u_char* D_8009D7EC;
extern u_char* g_WorldMapSemiTransPrims[];
extern u_char* D_8009D7FC;
extern s16 D_8009AFA0[];
extern u16 D_8009B688[];
extern u16 g_WorldMapTileGroupIds[];
extern u16 D_8009B64E[];
extern u16 g_WorldMapSlotRotationGroupIds[];
extern u16 D_8009B626[];

typedef struct {
    /* 0x0 */ s16 col;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 row;
    /* 0x6 */ s16 unk6;
} WorldMapGridOrigin; // Size: 0x8

extern MATRIX g_WorldMapViewMatrix;
extern MATRIX g_WorldMapIdentityMatrix;
extern s16 g_WorldMapViewRotationZ;
extern s16 D_8009BE04;
extern u32* g_WorldMapRenderContext;
extern u_char* g_WorldMapTileVertexSteps;
extern s16 D_8009D618[];
extern s16 g_WorldMapGridTileIds[];
extern WorldMapGridOrigin g_WorldMapGridOrigin;
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
extern u16 D_8009AF80[8];
extern u16 D_8009AF90[8];

// 0x20 entries of the path direction the actor walks along, ring-indexed by unk54
extern WorldMapPathEntry D_8009CD68[];

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
// lwl/lwr and scores 60%). func_800865A0 shows the identical idiom with
// g_WorldMapSemiTransPrims. Screen: cdk 92.21, 2.6.0 92.27, best 92.27 here - not
// a compiler miss.
void func_80085FE0(void) {
    POLY_FT4* pPrim;
    POLY_FT4* pDst;
    POLY_FT4* pSrc;
    s32 i;

    g_WorldMapSpritePrims[0] = HeapAlloc(0x5000, 1);
    D_8009D7EC = HeapAlloc(0x5000, 1);
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
    pDst = (POLY_FT4*)D_8009D7EC;
    memcpy(pDst, pSrc, 0x5000);
}
void WorldMapFreeSpritePrimBuffers(void) {
    HeapFree(D_8009D7EC);
    HeapFree(g_WorldMapSpritePrims[0]);
}
// Best: 95.40% (660 bytes vs retail's 640). The quad setup in the scratchpad, both
// matrix copies (g_WorldMapViewMatrix and g_WorldMapIdentityMatrix into 0x1F800028/0x1F800048), RotMatrixZ
// and the 0x10-halfword copy of g_WorldMapClutIds are instruction-for-instruction exact, and
// the 5x5 tile loop matches too. The remaining 20 bytes are one register:
// retail hoists g_WorldMapPrimBlocks (s3) and &D_8009C83C (s4) but re-forms D_8009D618's
// address with `lui $at / addu $at,$at,v0` at each use; gcc here hoists that address
// into a sixth callee-saved register (s5), which is the extra save/restore pair and
// the 0x30 frame instead of 0x28. gcc also fills retail's `beq` delay slot (a nop)
// with the row-origin load. Second delta, cosmetic only: retail's origin base is
// &D_8009C83C with displacements 0 and -4, a struct at g_WorldMapGridOrigin gives 4 and 0 -
// the same linked bytes, so check_bin cannot see it.
// What got the score here (in order): two separate s16 externs 88.99, then ONE loop
// counter shared by the copy loop and the inner column loop - that is what puts
// retail's counter in s0 and forces `sw s0` before the save block - then the
// struct-pointer origin, 95.40.
// Round 286: re-measured after the array-extern lever landed above - unchanged, and the
// residual is now named precisely. Ours materialises the symbol address of D_8009D618
// (and, once the index is hoisted, of g_WorldMapGridTileIds) into a callee-saved register
// and adds the scaled index to it, i.e. gcc's SPLIT address form; retail keeps the
// non-split form (`lui $at,%hi(sym); addu $at,$at,v0; lh %lo(sym)(at)`), which costs no
// register - hence retail's 0x28 frame and our 0x30. The same split/non-split difference
// explains the clut copy: retail keeps 0x1F800000 in v1 and stores at displacement 0x68
// (`addiu v1,v1,%lo(D_1F800002)`), we `li 0x1F800068` (lui+ori) and store at 0, +4 bytes.
// Tried, all unchanged or worse: D_8009D618 sized [25], an explicit `slot = row * 5` biv
// (which additionally hoisted the tile-id base, 0x30 frame), -G0 (95.40), gcc-2.7.2-cdk
// (62.29, so not a preset). `-mno-split-addresses` is not an option in these cc1s.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008615C);
/*
void func_8008615C(void) {
    WorldMapTableEntry* pEntry;
    WorldMapGridOrigin* pOrigin;
    u8* pTable;
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
    pDst = (u16*)0x1F800068;
    for (i = 0; i < 0x10; i++) {
        *pDst++ = *pSrc++;
    }
    D_8009BE04 = 0;
    pTable = g_WorldMapPrimBlocks;
    pOrigin = &g_WorldMapGridOrigin;
    for (row = 0; row < 5; row++) {
        for (i = 0; i < 5; i++) {
            if (D_8009D618[row * 5 + i] == -1) {
                continue;
            }
            id = g_WorldMapGridTileIds[(row + pOrigin->row) * 9 + i + pOrigin->col];
            pEntry = (WorldMapTableEntry*)(pTable + id * 8);
            if (pEntry->count != 0) {
                func_80099BFC((u8*)pEntry->offset, pEntry->count, g_WorldMapRenderContext[0x1C],
                              g_WorldMapSpritePrims[g_WorldMapPrimPacketIndex] + D_8009BE04 * 0x28);
            }
        }
    }
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800863E0);
// Best: 93.98% (392 bytes = retail's exact size; instruction multiset identical
// apart from one systematic register tie: retail keeps pVertex in s3 and the x
// counter in s2, this body takes the reverse, which is also what moves the
// `addiu s1,pVertex,4` giv past `move s0,zero`). Both Rand loops, the 0x54/0x50
// bounds, the (D_8009AF30[i]/D_8009AF38[i]) strength-reduced strides and the
// double HeapAlloc prologue are exact.
// What the permuter found (run 1, base=926 -> 721, two runs, no ZERO):
//   yoff = y << 11 had to move INSIDE the x loop, after xoff - that alone put
//   the entry-block `x = 0` before the y-loop preheader, where retail has it
//   (a redundant `x = 0;` statement is deleted by gcc, so this is the only way).
//   Run 2 printed the same 721; run 3 (score-only) confirmed no further gain.
// Round 285 additions, all still 93.98 at 392 bytes: pVertex's declaration moved
// to the head of the list, x's declaration moved to the head, dropping the
// standalone `x = 0;`, and moving it above the pVertex assignment. The tie is
// real allocation priority, not source order.
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
    x = 0;
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
}
*/
void WorldMapFreeTileVertexBuffers(void) {
    HeapFree(g_WorldMapTileVertexSteps);
    HeapFree(g_WorldMapTileVertexBuffer);
}
// The array form of the extern is deliberate: it is what makes gcc materialise the
// global's ADDRESS into a callee-saved register (retail: lui/addiu into s0, store of
// HeapAlloc #1 in the second call's delay slot, reload through s0) instead of the
// assembler's lui $at / sw %lo(sym) macro. Same lever as func_80085FE0 above.
void func_800865A0(void) {
    POLY_FT4* pPrim;
    POLY_FT4* pSrc;
    POLY_FT4* pDst;
    s32 i;
    s32 primCode;
    s32 primShade;
    s32 primLen;

    g_WorldMapSemiTransPrims[0] = HeapAlloc(0x2D00, 1);
    D_8009D7FC = HeapAlloc(0x2D00, 1);
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
    pDst = (POLY_FT4*)D_8009D7FC;
    memcpy(pDst, pSrc, 0x2D00);
}

void WorldMapFreeSemiTransPrimBuffers(void) {
    HeapFree(D_8009D7FC);
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
// register assignment: gcc gives pPos $s3 and the D_8009AF90 base $s2 where retail
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
// (hoists the D_8009AF80/D_8009AF90 base-register formations above the call, which
// pushes pPos out of s2); do/while starting at the unk2C store = 89.16 (760 bytes);
// do/while over the post-call pPos->x/tileZ/pPos->z group = 88.62.
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
            pSlot->x = D_8009AF80[pSlot->unk50] << 12;
            z = D_8009AF90[pSlot->unk50];
        } else {
            pSlot->unk50 = g_GameState.playerTileIndex;
            pSlot->x = g_GameState.playerMapPosX << 12;
            z = g_GameState.playerMapPosZ;
        }
        pSlot->z = z << 12;
        pSlot->y = WorldMapGetTilePlaneHeight(pSlot->x, pSlot->z);
        do {
            pPos->x = D_8009AF80[pSlot->unk50] << 12;
            pPos->z = D_8009AF90[pSlot->unk50] << 12;
            dist = func_80094154(&pSlot->x, pPos);
        } while (0);
        if (dist < 0) {
            dist = -dist;
        }
        if (dist < 8) {
            pSlot->unk50 = (pSlot->unk50 + 1) & 7;
        }
        pPos->x = D_8009AF80[pSlot->unk50] - (pSlot->x >> 12);
        tileZ = D_8009AF90[pSlot->unk50];
        pPos->z = tileZ - (pSlot->z >> 12);
        pPos->y = 0;
        WorldMapWrapDeltaScaled(pPos);
        VectorNormal((VECTOR*)pPos, (VECTOR*)pPos);
        for (i = 0; i < 0x100; i += 8) {
            ((WorldMapPathEntry*)((u8*)D_8009CD68 + i))->x = pPos->x;
            ((WorldMapPathEntry*)((u8*)D_8009CD68 + i))->z = pPos->z;
        }
        pSlot->unk58 = 1;
        pSlot->unk54 = 0;
        pSlot->dirX = D_8009CD68[1].x;
        pSlot->dirZ = D_8009CD68[pSlot->unk58].z;
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
void func_80089160(s32 arg0, void* pSrc, void* pRot);
void WorldMapDeactivateSpriteGroup(s32 arg0);
void WorldMapQueueZoneEntry(s32 arg0, s32* pVec, s32 arg2, s32 arg3);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087FD0);
// Best: 96.81% (1440 bytes = retail's exact size; instruction multiset identical
// apart from three residuals, none of them structure):
//  (a) prologue order. Retail spills `sw s4,48(sp) / lui s4,0x1F80` and loads
//      g_WorldMapSlots before `sll a1,s6,7`, then `addu s1,a0,a1` before the
//      g_WorldMapLevelGroupIds chain, and loads g_WorldMapGroups there too; gcc here loads
//      g_WorldMapSlots after the B674 chain, g_WorldMapGroups after the *0x54 chain (into
//      v1 not a0) and steals `addu s1,a0,a1` into the `beqz` delay slot where
//      retail has the pLayer `addu s2,a0,v0`. Swapping the first two source
//      assignments changes nothing (measured).
//  (b) func_80089160's second argument: retail `ori a1,s4,0xA0`, this body
//      `addiu a1,s4,0xA0`. The literal `(SVECTOR*)0x1F8000A0` form is worse
//      (all three 0xA0 stores become `lui $at` + store); deriving them from pPos
//      as below is what reproduces retail's `sh v0,160(s4)` bases.
//  (c) cosmetic: retail's `addiu s0,s4,%lo(D_1F800150)` / `%lo(D_1F8000A8)`
//      name absolute symbols, this body emits the same addends as immediates -
//      the linked bytes are the same.
// Everything from the WorldMapWrapDeltaScaled after func_80094154 to the
// epilogue is instruction-identical, including both smoothing filters
// (`(x + old*63) << 6 >> 12`, the two shifts are real: /64 with a *64 first),
// the D_8009CD68 ring writes, the WorldMapBuildBasisMatrix argument block and
// the whole dist threshold chain (note the two conditions must both read
// `pPos->unk4 >= -0xBF`, and the two distance variables must be separate locals
// or gcc copies the func_80094154 return into v1 and never fills the bgez slot).
// Reading pPos->z before pPos->unk4 = g_GameState.scriptPosY is required for the
// 0x182C block to match.
// Permuter run 1 (base=1695 -> 1352) only found `s16 tilt = (s16)pSlot->unk4A`
// hoisted before the 0x182E store; that scores 96.22 but costs a callee-saved
// register and grows the file to 1444, so it is not a match.
/*
s32 func_80087FD0(s32 index) {
    WorldMapPlayer* pSlot;
    WorldMapSceneLayer* pLayer;
    WorldMapPos* pPos;
    WorldMapPos* pSlotPos;
    SVECTOR* pRot;
    s32 tileIndex;
    s32 tileDist;
    s32 dist;

    tileIndex = g_WorldMapLevelGroupIds[g_WorldMapLevelIndex];
    pSlot = &g_WorldMapSlots[index];
    pLayer = &g_WorldMapGroups[tileIndex];
    pPos = (WorldMapPos*)0x1F800000;
    if (g_GameState.fieldScriptMemory[0] < 0xCD) {
        pSlot->y = WorldMapGetTilePlaneHeight(pSlot->x, pSlot->z);
        pLayer->x = pSlot->x >> 12;
        pLayer->y = pSlot->y >> 12;
        pLayer->z = pSlot->z >> 12;
    } else {
        pSlotPos = (WorldMapPos*)&pSlot->x;
        pPos->x = D_8009AF80[pSlot->unk50] << 12;
        pPos->z = D_8009AF90[pSlot->unk50] << 12;
        tileDist = func_80094154(pSlotPos, pPos);
        if (tileDist < 0) {
            tileDist = -tileDist;
        }
        if (tileDist < 8) {
            pSlot->unk50 = (pSlot->unk50 + 1) & 7;
        }
        pPos->x = D_8009AF80[pSlot->unk50] - (pSlot->x >> 12);
        pPos->z = D_8009AF90[pSlot->unk50] - (pSlot->z >> 12);
        pPos->y = 0;
        WorldMapWrapDeltaScaled(pPos);
        VectorNormal((VECTOR*)pPos, (VECTOR*)pPos);
        pSlot->dirX = ((pPos->x + pSlot->dirX * 63) << 6) >> 12;
        pSlot->dirZ = ((pPos->z + pSlot->dirZ * 63) << 6) >> 12;
        VectorNormal((VECTOR*)&pSlot->dirX, (VECTOR*)&pSlot->dirX);
        D_8009CD68[pSlot->unk54].x = pSlot->dirX;
        D_8009CD68[pSlot->unk54].z = pSlot->dirZ;
        pSlot->unk54 = (pSlot->unk54 + 1) & 0x1F;
        pPos->x = pSlot->dirX;
        pPos->y = pSlot->dirY;
        pPos->z = -pSlot->dirZ;
        WorldMapBuildBasisMatrix((VECTOR*)pPos, (VECTOR*)0x1F800010, (MATRIX*)0x1F8000F0);
        pLayer->mat = *(MATRIX*)0x1F8000F0;
        pSlot->x += D_8009CD68[pSlot->unk58].x * ((s16)pSlot->moveSpeed >> 12);
        pSlot->z += D_8009CD68[pSlot->unk58].z * ((s16)pSlot->moveSpeed >> 12);
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
            func_80089160(0x13, pRot, (SVECTOR*)((u8*)pPos + 0xA8));
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
*/
s32 WorldMapInitializePlayerSlot(s32 index) {
    WorldMapPlayer* pSlot;
    u16* pCount;
    s32 posZ;
    s32 posX;

    func_8008868C();
    pSlot = &g_WorldMapSlots[index];
    pSlot->unk54 = 4;
    pSlot->state = 0;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0;
    pSlot->unk5C = 0xC;
    pCount = &g_GameState.unk1854;
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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008868C);
// Best: 76.81% (152 bytes vs retail's 148). Two-pointer walk over the interleaved
// s16 pairs of D_8009AFA0 offset by base = D_8009B688[g_WorldMapLevelIndex], ending at a
// -1 sentinel; all four `lh`/`addu`/`jal` operands and the if + do-while shape are
// retail's. ONE residual, and everything else follows from it: retail lets the loop
// test's -1 die after the call and rematerialises `li v0,-1` before the `bne`, so
// that li is the body's last instruction and reorg then steals the exit block's
// `li v0,1` into the branch slot; gcc here keeps a second live copy in $s3
// (li s3,-1 in the preheader, frame 0x28 instead of 0x20), which makes the pY bump
// the body's last instruction and it gets stolen instead. Every decl order,
// do-while, `*pX++`/`pX += 2`, interleaved-vs-grouped increments, pointer-vs-index
// and an s32 `end = -1` local measured the same 76.81. Permuter run 1
// (base=1021 best=727, 2419 iters) only found a candidate that reads *pY for the
// guard, i.e. wrong code - rejected. Screen: cdk 61.86, 2.6.0 76.81, best 76.81 here.
/*
s32 func_8008868C(void) {
    s32 base = D_8009B688[g_WorldMapLevelIndex];
    s16* pX = D_8009AFA0;
    s16* pY = D_8009AFA0 + 1;

    if (*pX != -1) {
        do {
            s32 vx = *pX;
            s32 vy = *pY;

            pX += 2;
            pY += 2;
            WorldMapSetSpriteSetParent(base + vx, base + vy);
        } while (*pX != -1);
    }
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80088720);
