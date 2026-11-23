#include "common.h"

#include "psyq/libgte.h"

// Same sentinel as include/main/game.h's WORLD_MAP_AREA_NONE (this unit does not include
// game.h); duplicated here so the name is used, not the literal.
#define WORLD_MAP_AREA_NONE (-1)

// An angle is a 4096-step turn counter, and every sum of angles is wrapped back
// into it. Owned by include/main/game.h (identical value); this unit does not
// include it.
#define WORLD_MAP_ANGLE_MASK 0xFFF
// The quarter turn WorldMapBuildDirectionFromPositions adds to ratan2's result
// (which is measured from +x) before rcos/rsin take it.
#define WORLD_MAP_ANGLE_QUARTER_TURN 0x400


// One 16-byte row of the world map tile-kind table. The kinds are read as s16
// pairs over the byte row; a s16 member makes gcc re-form the base address.
typedef struct {
    /* 0x0 */ u8 kind[16];
} WorldMapTileKindRow; // Size: 0x10

// One 0x28-byte world map primitive packet at D_8009D2B8. The first word is the
// display-list tag (its top byte is the packet length, set by the assembler's
// setlen); the rest is written field by field through the D_8009D2B8+ offsets.
typedef struct {
    /* 0x00 */ u32 tag;
    /* 0x04 */ u8 unk4[0x24];
} WorldMapPrim; // Size: 0x28

// One tile spans WORLD_MAP_TILE_SIZE world-position units; the same define lives in
// include/main/game.h, which this unit does not include.
// WorldMapPos is 12.4 fixed point, so a tile is WORLD_MAP_TILE_SIZE << 12 of those
// (the 0x800000 the wrap functions add, and the 2047 WorldMapGetTileEntry masks to).
#define WORLD_MAP_TILE_SIZE 2048
// A tile cell is four zones of 9x9 sub-cells, each sub-cell a 4-byte record; the zone
// grid is WORLD_MAP_TILE_ZONE_WIDTH wide, which is the row stride WorldMapGetTileEntry
// multiplies the sub-cell row by.
#define WORLD_MAP_TILE_ZONE_COUNT 4
// The quadrant index WorldMapGetTileEntry builds for WorldMapTileCell.zones: the low bit
// selects the far half of the tile in x, bit 1 the far half in z.
#define WORLD_MAP_TILE_ZONE_X_HALF 0x1
#define WORLD_MAP_TILE_ZONE_Z_HALF 0x2
#define WORLD_MAP_TILE_ZONE_WIDTH 9
#define WORLD_MAP_TILE_ZONE_CELLS (WORLD_MAP_TILE_ZONE_WIDTH * WORLD_MAP_TILE_ZONE_WIDTH)
#define WORLD_MAP_TILE_CELL_RECORD_SIZE 4
// A tile is WORLD_MAP_TILE_SUBCELLS x WORLD_MAP_TILE_SUBCELLS sub-cells, which is the
// row length of WorldMapTileCell.values and the reason WorldMapGetTileValue packs the
// two sub-cell indices into one index as 4-bit fields.
#define WORLD_MAP_TILE_SUBCELLS 16
// The shape field of a tile cell value: the low nibble WorldMapGetTileShape
// returns, which WorldMapGetTileHalfIndex indexes the per-shape tables with.
#define WORLD_MAP_TILE_SHAPE_MASK 0xF
// WorldMapWrapDelta/Scaled treat a position delta larger than eight tiles as a wrap and
// move it by the map's span; the same eight tiles in both position forms.
#define WORLD_MAP_WRAP_DELTA_TILES 8

// World map tile cell: the zone grids (0x510 bytes, where WorldMapGetTileValue
// reads its values at) followed by the sub-cell values.
typedef struct {
    /* 0x000 */ u8 zones[WORLD_MAP_TILE_ZONE_COUNT * WORLD_MAP_TILE_ZONE_CELLS * WORLD_MAP_TILE_CELL_RECORD_SIZE];
    /* 0x510 */ s16 values[WORLD_MAP_TILE_SUBCELLS * WORLD_MAP_TILE_SUBCELLS];
} WorldMapTileCell; // Size: 0x710

// 12.4 fixed point world map position. y is the plane height read back by
// WorldMapGetTilePlaneHeight; WorldMapGetTileValue only reads x and z.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

// WorldMapMenuSlot.state values, followed by WorldMapUpdateAreaNameString: the slot's
// string holds nothing yet, or it holds the current area's name.
#define WORLD_MAP_SLOT_STATE_EMPTY 0
#define WORLD_MAP_SLOT_STATE_NAMED 1

// One slot of the world map slot table g_WorldMapSlots, as the area-name string work
// reads it. state is 0 while the slot's string holds nothing and 1 once it holds the
// current area's name, which is areaId; WorldMapUpdateAreaNameString follows both.
typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ s16 state;
    /* 0x22 */ u8 unk22[0x2E];
    /* 0x50 */ s32 areaId;
    /* 0x54 */ u8 pad54[0x2C];
} WorldMapMenuSlot; // Size: 0x80

// Bit 7 of a zone cell's flags byte: the cell is the "up" corner of its quad, so its
// height is the one the plane normal and the interpolation are measured from.
#define WORLD_MAP_ZONE_CELL_UP_CORNER 0x80

// One 4-byte record of a tile's zone grid, which WorldMapGetTileEntry points at.
// height is that zone cell's terrain height.
typedef struct {
    /* 0x0 */ s8 height;
    /* 0x1 */ u8 flags; // WORLD_MAP_ZONE_CELL_UP_CORNER marks the quad's up corner
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
} WorldMapTileZoneCell; // Size: 0x4

// One 0x10-byte row of the per-tile-shape axis table at g_WorldMapTileShapeAxis (the same
// bytes as g_WorldMapCameraPathHeightsMode7 + 0x28, which another unit reads as a
// plain s16 array): the x/z direction a position is projected on. g_WorldMapTileDiagonalAxes
// is two rows of the same shape.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ u8 unk4[4];
    /* 0x8 */ s32 z;
    /* 0xC */ u8 unkC[4];
} WorldMapTileShapeAxis; // Size: 0x10

extern void SystemStringFree(void* pString);
extern s32 WorldMapGetTileValue(WorldMapPos* p);
extern u8* WorldMapGetTileEntry(s32 x, s32 z);
extern long SquareRoot0(long a);
extern long ratan2(long y, long x);
extern int rcos(int a);
extern int rsin(int a);

// The world map frame render context g_WorldMapRenderContext points at: partial view of the
// two ordering table heads it fills each frame (the same view misc4.c and misc7.c
// declare); a head's first word is the packet this unit's D_8009D2B8 entry links in.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ WorldMapPrim *pOt[2];
} WorldMapRenderContext; // partial view

extern u8 g_WorldMapAreaNameString[];
extern u8 g_WorldMapMode4AreaNameString[];
extern WorldMapTileKindRow g_WorldMapTileKindTable[];
extern WorldMapTileShapeAxis g_WorldMapTileShapeAxis[];
extern s32 g_WorldMapGridAngle;
extern s32 g_WorldMapGridShadePhase;
extern WorldMapTileShapeAxis g_WorldMapTileDiagonalAxes[2];
extern WorldMapTileCell* g_WorldMapTileCellPointers[];
extern s32 g_WorldMapTileShapeHalfAxis[];
extern s32 g_WorldMapTileShapeHalfAxisZ[];
extern s32 g_WorldMapTileShapeHalfOrigin[];
extern s32 g_WorldMapTileShapeHalfOriginZ[];
extern s32 g_WorldMapTileColumns;
extern s32 g_WorldMapTileRows;
extern s16 g_WorldMapAreaId;
extern u16 g_WorldMapAreaNamePanelFlags;
extern u8 D_8009D500;
extern s32 SystemStringInit(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);
extern void WorldMapBuildTilePlaneNormal(WorldMapPos* p, s32 x, s32 z);
extern s32 WorldMapInterpolatePlaneHeight(WorldMapPos* p0, WorldMapPos* p1, WorldMapPos* p2);
extern void SystemStringResetIfIdle(void* pString);
extern void SystemStringPushBuffer(void* pString, void* pData);
extern void func_80034888(void* pString, u32 unk, u32 arg);
extern u8* GetStringEntry(u8* pData, s32 index);
extern u8* g_WorldMapSlots;
extern WorldMapRenderContext* g_WorldMapRenderContext;
extern u8* g_WorldMapStringTable;
extern s32 g_WorldMapPrimPacketIndex;
extern s16 g_WorldMapMode4AreaId;
extern WorldMapPrim D_8009D2B8[];
extern u8 D_8009BDCC;
extern u16 D_8009D2C0;
extern u16 D_8009D2C2;
extern u16 D_8009D2C8;
extern u16 D_8009D2CA;
extern u16 D_8009D2D0;
extern u16 D_8009D2D2;
extern u16 D_8009D2D8;
extern u16 D_8009D2DA;
extern u8 D_8009D2BB;
extern u8 D_8009D2BC;
extern u8 D_8009D2BD;
extern u8 D_8009D2BE;
extern u8 D_8009D2BF;
extern u8 D_8009D2C4;
extern u8 D_8009D2C5;
extern u8 D_8009D2CC;
extern u8 D_8009D2CD;
extern u8 D_8009D2D4;
extern u8 D_8009D2D5;
extern u8 D_8009D2DC;
extern u8 D_8009D2DD;
extern u16 D_8009D2C6;
extern u16 D_8009D2CE;
extern u16 GetTPage(int tp, int abr, int x, int y);
extern u16 GetClut(int x, int y);
extern void SetSemiTrans(void* p, int abe);


s32 WorldMapInitAreaNameString(void) {
    u8* pBuffer;

    pBuffer = g_WorldMapAreaNameString;
    g_WorldMapAreaId = WORLD_MAP_AREA_NONE;
    SystemStringInit(pBuffer, 0x3C0, 0x180, 0xA0, 0x78, 0x20, 1);
    D_8009D500 = 8;
    g_WorldMapAreaNamePanelFlags |= 2;
    SystemStringResetIfIdle(pBuffer);
    return 1;
}

s32 WorldMapUpdateAreaNameString(s32 index) {
    WorldMapMenuSlot* pSlot;

    pSlot = (WorldMapMenuSlot*)(g_WorldMapSlots + index * sizeof(WorldMapMenuSlot));
    switch (pSlot->state) {
    case WORLD_MAP_SLOT_STATE_EMPTY:
        if (g_WorldMapAreaId != WORLD_MAP_AREA_NONE) {
            SystemStringResetIfIdle(g_WorldMapAreaNameString);
            SystemStringPushBuffer(g_WorldMapAreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapAreaId));
            pSlot->state = WORLD_MAP_SLOT_STATE_NAMED;
            pSlot->areaId = g_WorldMapAreaId;
        }
        break;
    case WORLD_MAP_SLOT_STATE_NAMED:
        if (g_WorldMapAreaId == WORLD_MAP_AREA_NONE) {
            SystemStringResetIfIdle(g_WorldMapAreaNameString);
            pSlot->state = WORLD_MAP_SLOT_STATE_EMPTY;
        } else if (g_WorldMapAreaId != pSlot->areaId) {
            SystemStringResetIfIdle(g_WorldMapAreaNameString);
            SystemStringPushBuffer(g_WorldMapAreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapAreaId));
            pSlot->areaId = g_WorldMapAreaId;
        }
        break;
    }
    func_80034888(g_WorldMapAreaNameString, (u32)g_WorldMapRenderContext->pOt[0], (u32)g_WorldMapPrimPacketIndex);
    return 1;
}

void WorldMapFreeAreaNameString(void) {
    SystemStringFree(g_WorldMapAreaNameString);
}

/* best: 98.05% (480 bytes = retail 480, 120 insns both) - but that measurement needs a
   declaration change this unit cannot take, so the body parked here is the 91.17 one
   (re-measured this round: 480 bytes, 120/120, 28 differing rows).
   How to reach 98.05, both steps measured in round 396: (1) declare the flags global as
   an ARRAY (`extern u16 g_WorldMapAreaNamePanelFlags[];`) and write
   `g_WorldMapAreaNamePanelFlags[0]`: cc1 2.7.2 then materialises the address once
   (`la a1` + `lhu v0,0(a1)` + `sh v0,0(a1)`, retail exactly) - 96.34. The array type is
   per DECLARATION, so it takes the already-matched WorldMapInitAreaNameString from
   100.00 to 86.14 (that function's retail really does re-form the address twice:
   `lui v0` + `lhu v0,%lo(v0)` then `lui at` + `sh`), so un-parking needs an alias symbol
   at the same address for this function only. (2) move `D_8009D2C0 = 0x98;` above
   `D_8009D2CA = 0x70;` - 96.34 -> 98.05. Measured this round: a plain
   `u16* pFlags = &g_WorldMapAreaNamePanelFlags;` local (scalar declaration, accesses
   `*pFlags`) is folded straight back to the scalar form by cse and measures the SAME
   91.17, so the address-in-a-register shape is only reachable through the declaration
   (tools/rounds/levers.md k-unbounded-array-extern-global).
   What the 28 rows of the 91.17 body are, re-read this round: exactly two things.
   (a) the flags read-modify-write: ours is retail's WorldMapInitAreaNameString shape
   (`lui v0` + `lhu v0,0(v0)`, `lui at` + `sh`, and the jal delay slot then takes
   `move a0,s0` where retail has the `sh`); (b) ONE `li` position: retail lifts
   `li t0,0x98` above the `li v0,0x70` pair (li order 9, 0x2C, 0x98, 0x70, 0x128, 0xB4,
   0x80, 0xFF, 0x3F) while every spelling tried emits each `li` immediately before its
   first store pair (9, 0x2C, 0x70, 0x128, 0x98, ...). The STORE order itself already
   matches retail's exactly, so the li position cannot be moved by reordering the pairs.
   Not a preset question: --all under 2.6.0 (91.17, and five matched functions fall to
   99.4-99.9) and under 2.7.2-cdk (91.17, most matched functions 84-100) are both worse;
   -fschedule-insns on the unit .i leaves the li order unchanged.
   The tail is a 0x28-byte struct copy (`D_8009D2B8[1] = D_8009D2B8[0];`) and the 21
   scalar stores really are through the absolute D_8009D2Bx symbols (retail uses
   `lui at` + store per site, never a base register), so a typed WorldMapPrim field
   spelling of them would change the code - leave them as symbols.
s32 func_80092DF8(void) {
    u16 flags;

    g_WorldMapMode4AreaId = WORLD_MAP_AREA_NONE;
    SystemStringInit(g_WorldMapMode4AreaNameString, 0x3C0, 0x18D, 0xA0, 0x78, 0x20, 4);
    flags = g_WorldMapAreaNamePanelFlags;
    D_8009BDCC = 8;
    g_WorldMapAreaNamePanelFlags = flags | 2;
    SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
    D_8009D2BB = 9;
    D_8009D2BF = 0x2C;
    D_8009D2C2 = 0x70;
    D_8009D2CA = 0x70;
    D_8009D2C8 = 0x128;
    D_8009D2D8 = 0x128;
    D_8009D2C0 = 0x98;
    D_8009D2D0 = 0x98;
    D_8009D2D2 = 0xB4;
    D_8009D2DA = 0xB4;
    D_8009D2C4 = 0x80;
    D_8009D2C5 = 0;
    D_8009D2CC = 0xFF;
    D_8009D2CD = 0;
    D_8009D2D4 = 0x80;
    D_8009D2D5 = 0x3F;
    D_8009D2DC = 0xFF;
    D_8009D2DD = 0x3F;
    D_8009D2BC = 0x80;
    D_8009D2BD = 0x80;
    D_8009D2BE = 0x80;
    D_8009D2CE = GetTPage(0, 0, 0x380, 0x100);
    D_8009D2C6 = GetClut(0x130, 0x1E4);
    SetSemiTrans(D_8009D2B8, 1);
    D_8009D2B8[1] = D_8009D2B8[0];
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092DF8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092FD8);
/* best: 99.58% (472 bytes = retail's 472; 118 instructions = retail's 118). The whole
   body is instruction-identical; retail's tail keeps 0xFFFFFF in $a1 and
   g_WorldMapRenderContext's address in $a3, we mirror those two (0xFFFFFF in $a3, ctx in
   $a1). The third value, 0xFF000000, is $a2 and the base / element pointer / index /
   tag word are retail's registers in both.
   The twin is WorldMapUpdateAreaNameString above (same unit): its head - the switch over
   WorldMapMenuSlot state with the two string-reset arms and the func_80034888 call -
   ports exactly, only the callees' string / id globals differ. The two tag
   read-modify-writes at the end are this function's own.
   99.45 -> 99.58: compute the element's address mask into a local
   (`primAddr = (u32)pPrim & 0xFFFFFF;`) between the tag read and the two stores
   (permuter, 1314 iterations). That moved the base, the element pointer and the index
   into retail's registers and left only the a1/a3 mirror.
   Measured after that, all 472 bytes / 118 instructions: OR operands commuted 97.67, the
   two masks as #defines 99.58 (no byte change; name them when it matches), the OT word
   in a `u32 otTag` local 99.58, a `WorldMapPrim* pOt` local for
   `g_WorldMapRenderContext->pOt[0]` 99.58 (gcc still reloads it through ctx, so retail's
   two `lw 0x70(a3)` remain), primAddr assigned above `tag = pPrim->tag` instead of below
   the tag's mask 99.58, gcc-2.7.2-cdk 90.97, -G0 99.58 (so not a preset question).
   Why the mirror is out of reach: the tail block's hard-register use counts are $a1 4,
   $a3 4, $a2 3 in BOTH versions and `-dl` shows those three pseudos are block-local
   (`in block 10`), i.e. local_alloc hand-out in its own hard-register order with $a1 and
   $a3 tied - the GIV_TIE / a-global-allocno-priority-is class in tools/rounds/levers.md.
   A second permuter run from this body (759 iterations, base 50 best 50) found nothing.
   The earlier 99.45 note's measurements (tag local in place 98.52 -> 99.45, temps on
   both statements 97.11, `pCtx` local 98.52, base inline 95.17) still apply to the
   spellings above it.
   Round 430 (re-measured with the parked body as written: 99.58%, 472 bytes, 118 insns,
   the $a1/$a3 mirror only): a third permuter run from this body found nothing (1666
   iterations, base 50 best 50), and putting the 0xFFFFFF mask in a `u32 otMask` local
   used at both sites makes it a callee-saved global allocno - 480 bytes / 120 insns,
   99.58 -> 0.00. The mask's pseudo-creation order is not the knob either
   (a-in-place-update-of-a-load-temp already measured primAddr-above-tag at 99.58).
s32 func_80092FD8(s32 index) {
    WorldMapMenuSlot* pSlot;
    WorldMapPrim* pPrims;
    WorldMapPrim* pPrim;
    u32 tag;
    u32 primAddr;

    pSlot = (WorldMapMenuSlot*)(g_WorldMapSlots + index * sizeof(WorldMapMenuSlot));
    switch (pSlot->state) {
    case WORLD_MAP_SLOT_STATE_EMPTY:
        if (g_WorldMapMode4AreaId != WORLD_MAP_AREA_NONE) {
            SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
            SystemStringPushBuffer(g_WorldMapMode4AreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapMode4AreaId));
            pSlot->state = WORLD_MAP_SLOT_STATE_NAMED;
            pSlot->areaId = g_WorldMapMode4AreaId;
        }
        break;
    case WORLD_MAP_SLOT_STATE_NAMED:
        if (g_WorldMapMode4AreaId == WORLD_MAP_AREA_NONE) {
            SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
            pSlot->state = WORLD_MAP_SLOT_STATE_EMPTY;
        } else if (g_WorldMapMode4AreaId != pSlot->areaId) {
            SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
            SystemStringPushBuffer(g_WorldMapMode4AreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapMode4AreaId));
            pSlot->areaId = g_WorldMapMode4AreaId;
        }
        break;
    }
    func_80034888(g_WorldMapMode4AreaNameString, (u32)g_WorldMapRenderContext->pOt[0], (u32)g_WorldMapPrimPacketIndex);
    if (pSlot->state == WORLD_MAP_SLOT_STATE_NAMED) {
        pPrims = D_8009D2B8;
        pPrim = &pPrims[g_WorldMapPrimPacketIndex];
        tag = pPrim->tag;
        tag = (tag & 0xFF000000) | (g_WorldMapRenderContext->pOt[0]->tag & 0xFFFFFF);
        primAddr = (u32)pPrim & 0xFFFFFF;
        pPrim->tag = tag;
        g_WorldMapRenderContext->pOt[0]->tag = (g_WorldMapRenderContext->pOt[0]->tag & 0xFF000000) | primAddr;
    }
    return 1;
}*/

void WorldMapFreeMode4AreaNameString(void) {
    SystemStringFree(g_WorldMapMode4AreaNameString);
}

/* best: 65.59% (356 bytes vs retail 380 = 92 insns; ours 87). ONE residual: loop.c
   hoists the three invariant products fr*t / fg*t / fb*t out of the pixel loop into
   the row loop (a mult + mflo pair before the pixel-loop head for each), 3 mult +
   3 mflo = exactly the 24 missing bytes; retail recomputes them per pixel. Everything
   else is retail's: the four lhu re-reads of *pIn, the three sb stores into a 3-byte
   u8 rgb[3] local (that is what kills the cse'd load and forces the re-reads - the
   rgb values themselves come back from the register cse recorded for the store, there
   is no lbu from the stack), srl ...15 (so t/s are u32; an s32 spelling emits sra),
   bnez with no andi (so the zero test reads *pIn, not a u16 local), pIn reset per row
   from pSrc while pOut is carried across both loops, and 0x1000 materialised in the
   row loop.
   The 69.59% / 316 byte / 79 insn body parked here before round 427 scores higher on
   objdiff's alignment metric but is 30 bytes FURTHER from retail and lacks the reload
   and sb structure; it is in git at commit 95f975b4 if a later lane wants to compare.
   Tried and measured on the reload body: pFactors[k] inline in the pixel loop 65.81
   (368 bytes, 90 insns - the products stay in the loop because a memory load is not
   hoistable, but three lbu appear); the products as three statements into a u32
   accumulator 65.59, byte-identical; t*fr operand order 65.59; u8* vs u16* parameters
   65.59 (keep u8*, it is what misc8/misc15 declare); s32 fr/fg/fb locals 61.69 (older
   measurement); u8 factor locals 60.22. Not a preset question: gcc-2.6.0 58.52, and
   gcc-2.7.2-cdk does not compile the body.
   Next attempt: find the spelling loop.c refuses to hoist for an invariant product of
   two registers that is used once in a nested conditional block. */
/*
void func_800931D8(u8* pSrc, u8* pDst, s32 count, u8* pFactors) {
    s32 i;
    s32 j;
    u32 t;
    u32 s;
    s32 fr;
    s32 fg;
    s32 fb;
    u8 rgb[3];
    u16* pIn;
    u16* pOut;

    fr = pFactors[0];
    fg = pFactors[1];
    fb = pFactors[2];
    pOut = (u16*)pDst;
    for (i = 0; i < count; i++) {
        t = (i << 12) / count;
        s = 0x1000 - t;
        pIn = (u16*)pSrc;
        for (j = 0; j < 0x100; j++) {
            if (*pIn == 0) {
                *pOut = *pIn;
            } else {
                rgb[0] = (*pIn & 0x1F) << 3;
                rgb[1] = (*pIn >> 2) & 0xF8;
                rgb[2] = (*pIn >> 7) & 0xF8;
                *pOut = (*pIn & 0x8000) | ((rgb[0] * s + fr * t) >> 15) |
                        (((rgb[1] * s + fg * t) >> 15) << 5) |
                        (((rgb[2] * s + fb * t) >> 15) << 10);
            }
            pIn++;
            pOut++;
        }
    }
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_800931D8);


void WorldMapWrapPosition(WorldMapPos* p) {
    if (p->x >= g_WorldMapTileColumns * (WORLD_MAP_TILE_SIZE << 12)) {
        p->x -= g_WorldMapTileColumns * (WORLD_MAP_TILE_SIZE << 12);
    }
    if (p->x < 0) {
        p->x += g_WorldMapTileColumns * (WORLD_MAP_TILE_SIZE << 12);
    }
    if (p->z >= g_WorldMapTileRows * (WORLD_MAP_TILE_SIZE << 12)) {
        p->z -= g_WorldMapTileRows * (WORLD_MAP_TILE_SIZE << 12);
    }
    if (p->z < 0) {
        p->z += g_WorldMapTileRows * (WORLD_MAP_TILE_SIZE << 12);
    }
}

void WorldMapWrapPositionScaled(WorldMapPos* p) {
    if (p->x >= g_WorldMapTileColumns * WORLD_MAP_TILE_SIZE) {
        p->x -= g_WorldMapTileColumns * WORLD_MAP_TILE_SIZE;
    }
    if (p->x < 0) {
        p->x += g_WorldMapTileColumns * WORLD_MAP_TILE_SIZE;
    }
    if (p->z >= g_WorldMapTileRows * WORLD_MAP_TILE_SIZE) {
        p->z -= g_WorldMapTileRows * WORLD_MAP_TILE_SIZE;
    }
    if (p->z < 0) {
        p->z += g_WorldMapTileRows * WORLD_MAP_TILE_SIZE;
    }
}

void WorldMapWrapDelta(WorldMapPos* p) {
    if (p->x < -(WORLD_MAP_WRAP_DELTA_TILES * (WORLD_MAP_TILE_SIZE << 12))) {
        p->x += g_WorldMapTileColumns * (WORLD_MAP_TILE_SIZE << 12);
    } else if (p->x > WORLD_MAP_WRAP_DELTA_TILES * (WORLD_MAP_TILE_SIZE << 12)) {
        p->x -= g_WorldMapTileColumns * (WORLD_MAP_TILE_SIZE << 12);
    }
    if (p->z < -(WORLD_MAP_WRAP_DELTA_TILES * (WORLD_MAP_TILE_SIZE << 12))) {
        p->z += g_WorldMapTileRows * (WORLD_MAP_TILE_SIZE << 12);
    } else if (p->z > WORLD_MAP_WRAP_DELTA_TILES * (WORLD_MAP_TILE_SIZE << 12)) {
        p->z -= g_WorldMapTileRows * (WORLD_MAP_TILE_SIZE << 12);
    }
}

void WorldMapWrapDeltaScaled(WorldMapPos* p) {
    if (p->x < -(WORLD_MAP_WRAP_DELTA_TILES * WORLD_MAP_TILE_SIZE)) {
        p->x += g_WorldMapTileColumns * WORLD_MAP_TILE_SIZE;
    } else if (p->x > WORLD_MAP_WRAP_DELTA_TILES * WORLD_MAP_TILE_SIZE) {
        p->x -= g_WorldMapTileColumns * WORLD_MAP_TILE_SIZE;
    }
    if (p->z < -(WORLD_MAP_WRAP_DELTA_TILES * WORLD_MAP_TILE_SIZE)) {
        p->z += g_WorldMapTileRows * WORLD_MAP_TILE_SIZE;
    } else if (p->z > WORLD_MAP_WRAP_DELTA_TILES * WORLD_MAP_TILE_SIZE) {
        p->z -= g_WorldMapTileRows * WORLD_MAP_TILE_SIZE;
    }
}

s32 WorldMapInterpolatePlaneHeight(WorldMapPos* p0, WorldMapPos* p1, WorldMapPos* p2) {
    p0->y = (-(p2->x * (p0->x - p1->x)) - p2->z * (p0->z - p1->z)) / p2->y;
    p0->y += p1->y;
    return p0->y;
}
u8* WorldMapGetTileEntry(s32 x, s32 z) {
    s16 idx;
    s16 zone;
    s16 sub;

    idx = (z >> 20) / 8 * g_WorldMapTileColumns + (x >> 20) / 8;
    x = (x >> 12) & (WORLD_MAP_TILE_SIZE - 1);
    z = (z >> 12) & (WORLD_MAP_TILE_SIZE - 1);
    zone = 0;
    if (x >= WORLD_MAP_TILE_SIZE / 2) {
        zone = WORLD_MAP_TILE_ZONE_X_HALF;
        x -= WORLD_MAP_TILE_SIZE / 2;
    }
    if (z >= WORLD_MAP_TILE_SIZE / 2) {
        zone |= WORLD_MAP_TILE_ZONE_Z_HALF;
        z -= WORLD_MAP_TILE_SIZE / 2;
    }
    sub = (z >> 4) / 8 * WORLD_MAP_TILE_ZONE_WIDTH + (x >> 4) / 8;
    return g_WorldMapTileCellPointers[idx]->zones +
           zone * (WORLD_MAP_TILE_ZONE_CELLS * WORLD_MAP_TILE_CELL_RECORD_SIZE) + sub * WORLD_MAP_TILE_CELL_RECORD_SIZE;
}
// Tile plane normal from the four zone-cell heights around the cell containing
// (x, z). The two edge vectors and their cross product are built in scratchpad at
// 0x1F800000 as absolute stores; pScratch/pEdge are deliberately the same value in
// two variables, because the second copy is what makes gcc keep the base in a saved
// register across the two calls (one variable is 95.81%).
void WorldMapBuildTilePlaneNormal(WorldMapPos* pNormal, s32 x, s32 z) {
    WorldMapTileZoneCell* pCell;
    VECTOR* pEdge;
    VECTOR* pScratch;
    s32 sum;

    pScratch = (VECTOR*)0x1F800000;
    pCell = (WorldMapTileZoneCell*)WorldMapGetTileEntry(x, z);
    // the quotients replace the parameters: retail keeps them in the parameters'
    // own registers.
    x = (u16)(x / 8);
    z = (u16)(z / 8);
    // sum picks which of the two edge pairs of the quad the position is nearer: the
    // cell's up-corner flag selects which of the table's two diagonal axes the
    // position is measured against.
    // it is written as a negated term so the compiler emits retail's negu+mult+addu.
    if (pCell->flags & WORLD_MAP_ZONE_CELL_UP_CORNER) {
        sum = x * g_WorldMapTileDiagonalAxes[1].x + (-z) * g_WorldMapTileDiagonalAxes[1].z;
        if (sum < 0) {
            *(s32*)0x1F800000 = 0x10;
            *(s32*)0x1F800008 = -0x10;
            *(s32*)0x1F800010 = 0;
            *(s32*)0x1F800018 = -0x10;
            *(s32*)0x1F800004 = pCell[10].height - pCell[0].height;
            *(s32*)0x1F800014 = pCell[9].height - pCell[0].height;
        } else {
            *(s32*)0x1F800000 = 0x10;
            *(s32*)0x1F800010 = 0x10;
            *(s32*)0x1F800008 = 0;
            *(s32*)0x1F800018 = -0x10;
            *(s32*)0x1F800004 = pCell[1].height - pCell[0].height;
            *(s32*)0x1F800014 = pCell[10].height - pCell[0].height;
        }
    } else {
        sum = (x - 0x10000) * g_WorldMapTileDiagonalAxes[0].x + (-z) * g_WorldMapTileDiagonalAxes[0].z;
        if (sum < 0) {
            *(s32*)0x1F800000 = -0x10;
            *(s32*)0x1F800008 = -0x10;
            *(s32*)0x1F800010 = -0x10;
            *(s32*)0x1F800018 = 0;
            *(s32*)0x1F800004 = pCell[9].height - pCell[1].height;
            *(s32*)0x1F800014 = pCell[0].height - pCell[1].height;
        } else {
            *(s32*)0x1F800000 = 0;
            *(s32*)0x1F800008 = -0x10;
            *(s32*)0x1F800010 = -0x10;
            *(s32*)0x1F800018 = -0x10;
            *(s32*)0x1F800004 = pCell[10].height - pCell[1].height;
            *(s32*)0x1F800014 = pCell[9].height - pCell[1].height;
        }
    }
    pEdge = pScratch;
    OuterProduct0(pEdge + 1, pEdge, pEdge + 2);
    VectorNormal(pEdge + 2, (VECTOR*)pNormal);
}
s32 WorldMapGetTilePlaneHeight(s32 x, s32 z) {
    WorldMapPos a;
    WorldMapPos b;
    WorldMapPos c;
    WorldMapTileZoneCell* pCell;

    pCell = (WorldMapTileZoneCell*)WorldMapGetTileEntry(x, z);
    a.x = (u16)(x / 8);
    a.z = -(u16)(z / 8);
    if (pCell->flags & WORLD_MAP_ZONE_CELL_UP_CORNER) {
        b.x = 0;
        b.z = 0;
        b.y = pCell->height << 12;
    } else {
        b.x = 0x10000;
        b.z = 0;
        b.y = pCell[1].height << 12;
    }
    WorldMapBuildTilePlaneNormal(&c, x, z);
    WorldMapInterpolatePlaneHeight(&a, &b, &c);
    return a.y << 3;
}
/* best: 71.71% (RE-MEASURED round 427 from this exact text: 1080 bytes vs retail 1072
   = 268 insns; ours 270). Structure is right: same 8 rcos calls in the same order,
   same multiply/shift chain, same four scratch stores at 0xA2/0xAA/0xB2/0xBA, same
   four sum arms, same GTE pair, same interpolate call. The earlier 71.82 note could
   not be reproduced because its parked text did not compile (it used s32 bases
   D_8009B254/D_8009B25C/D_8009B24C); the body below compiles and spells those as
   g_WorldMapTileDiagonalAxes[1].x, [1].z and [0].z. Residual facts, all measured:
   - Retail keeps BOTH parameters in callee-saved registers (s7 = x, s8 = z); this
     body spills z to 16(sp) and reloads it twice, and its frame is 64 against
     retail's 56. The quad block needs 9 live values (x, z, pTile, xa, x0, za, z0,
     angle, phase) and retail fits exactly that in s7,fp,s6,s1,s3,s0,s2,s5,s4.
   - Retail does NOT keep the four corner heights in registers: it stores each to
     scratch and re-reads them with lh in the arms, keeping only the last corner's
     height (h11) in a2. This body keeps h00/h01/h10/h11 in registers and emits no lh
     at all - that is the 4-ish instruction difference plus most of the pressure.
   - Retail builds the scratch base ONCE (lui s1,0x1F80 in the delay slot of the
     first bgez) and then stores sw v0,0(s1), calling OuterProduct0 with move a1,s1 /
     ori a0,s1,0x10 / addiu s0,s1,D_1F800020; this body re-forms every scratch address
     with a fresh lui/at (about 10 instructions).
   Tried and worse: heights read back from scratch in the arms 63.88 (gcc then keeps
   the 0xA2 address in a saved register instead of retail's fresh lui), a single
   shared height variable 66.24, s16 heights 67.35, the quads as a 2x2 loop 46.68
   (gcc 2.7.2 does not unroll it), x1/z1 inlined into the angle sums 71.71
   (identical). A permuter run from this body found nothing usable (its best
   candidates changed the stored values). 268 insns of this size is not a hand-edit
   job: park. */
/*
s32 func_80093A5C(s32 x, s32 z) {
    u8* pTile;
    s32 xa;
    s32 za;
    s32 x0;
    s32 z0;
    s32 angle;
    s32 phase;
    s32 sum;
    s32 h00;
    s32 h01;
    s32 h10;
    s32 h11;

    pTile = WorldMapGetTileEntry(x, z);
    xa = ((u32)x >> 19) & 7;
    x0 = xa << 9;
    za = ((u32)z >> 19) & 7;
    z0 = za << 9;

    angle = g_WorldMapGridAngle + x0;
    phase = (rcos(g_WorldMapGridShadePhase + z0) << 4) >> 3;
    h00 = (rcos(angle) * phase >> 20) + ((s8)pTile[0] << 3);
    *(s16*)0x1F8000A2 = h00;

    angle = g_WorldMapGridAngle + ((xa + 1) << 9);
    phase = (rcos(g_WorldMapGridShadePhase + z0) << 4) >> 3;
    h01 = (rcos(angle) * phase >> 20) + ((s8)pTile[4] << 3);
    *(s16*)0x1F8000AA = h01;

    angle = g_WorldMapGridAngle + x0;
    phase = (rcos(g_WorldMapGridShadePhase + ((za + 1) << 9)) << 4) >> 3;
    h10 = (rcos(angle) * phase >> 20) + ((s8)pTile[0x24] << 3);
    *(s16*)0x1F8000B2 = h10;

    angle = g_WorldMapGridAngle + ((xa + 1) << 9);
    phase = (rcos(g_WorldMapGridShadePhase + ((za + 1) << 9)) << 4) >> 3;
    h11 = (rcos(angle) * phase >> 20) + ((s8)pTile[0x28] << 3);
    *(s16*)0x1F8000BA = h11;

    xa = (u16)(x / 8);
    za = (u16)(z / 8);
    if (pTile[1] & 0x80) {
        sum = xa * g_WorldMapTileDiagonalAxes[1].x + (-za) * g_WorldMapTileDiagonalAxes[1].z;
        if (sum < 0) {
            *(s32*)0x1F800000 = 0x80;
            *(s32*)0x1F800008 = -0x80;
            *(s32*)0x1F800010 = 0;
            *(s32*)0x1F800018 = -0x80;
            *(s32*)0x1F800004 = h11 - h00;
            *(s32*)0x1F800014 = h10 - h00;
        } else {
            *(s32*)0x1F800000 = 0x80;
            *(s32*)0x1F800010 = 0x80;
            *(s32*)0x1F800008 = 0;
            *(s32*)0x1F800018 = -0x80;
            *(s32*)0x1F800004 = h01 - h00;
            *(s32*)0x1F800014 = h11 - h00;
        }
    } else {
        sum = (xa - 0x10000) * g_WorldMapTileDiagonalAxes[0].x + (-za) * g_WorldMapTileDiagonalAxes[0].z;
        if (sum < 0) {
            *(s32*)0x1F800000 = -0x80;
            *(s32*)0x1F800008 = -0x80;
            *(s32*)0x1F800010 = -0x80;
            *(s32*)0x1F800018 = 0;
            *(s32*)0x1F800004 = h10 - h01;
            *(s32*)0x1F800014 = h00 - h01;
        } else {
            *(s32*)0x1F800000 = 0;
            *(s32*)0x1F800008 = -0x80;
            *(s32*)0x1F800010 = -0x80;
            *(s32*)0x1F800018 = -0x80;
            *(s32*)0x1F800004 = h11 - h01;
            *(s32*)0x1F800014 = h10 - h01;
        }
    }
    OuterProduct0((VECTOR*)0x1F800010, (VECTOR*)0x1F800000, (VECTOR*)0x1F800020);
    VectorNormal((VECTOR*)0x1F800020, (VECTOR*)0x1F800000);
    *(s32*)0x1F800010 = (x >> 12) & 0x7F;
    *(s32*)0x1F800018 = -((z >> 12) & 0x7F);
    if (pTile[1] & 0x80) {
        *(s32*)0x1F800020 = 0;
        *(s32*)0x1F800024 = *(s16*)0x1F8000A2;
    } else {
        *(s32*)0x1F800020 = 0x80;
        *(s32*)0x1F800024 = *(s16*)0x1F8000AA;
    }
    *(s32*)0x1F800028 = 0;
    WorldMapInterpolatePlaneHeight((WorldMapPos*)0x1F800010, (WorldMapPos*)0x1F800020,
                                   (WorldMapPos*)0x1F800000);
    return *(s32*)0x1F800014 << 12;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093A5C);

s32 WorldMapGetTileValue(WorldMapPos* p) {
    s16 idx;
    s32 sub;
    WorldMapTileCell* pCell;
    s16* pValue;

    idx = ((p->z >> 20) / 8) * g_WorldMapTileColumns + (p->x >> 20) / 8;
    pCell = g_WorldMapTileCellPointers[idx];
    sub = (((p->z & ((WORLD_MAP_TILE_SIZE - 1) << 12)) >> 19) << 4) |
          ((p->x & ((WORLD_MAP_TILE_SIZE - 1) << 12)) >> 19);
    pValue = pCell->values;
    return pValue[sub];
}

s32 WorldMapGetTileHalfIndex(WorldMapPos* p) {
    s32 n;
    s32 k;
    s32 dx;
    s32 dz;
    s32 sum;

    n = WorldMapGetTileValue(p);
    k = (n & WORLD_MAP_TILE_SHAPE_MASK) * 4;
    dx = (u16)(p->x / 8) - g_WorldMapTileShapeHalfOrigin[k];
    dz = (u16)(p->z / 8) - g_WorldMapTileShapeHalfOriginZ[k];
    sum = (dx * g_WorldMapTileShapeHalfAxis[k] >> 12) + (dz * g_WorldMapTileShapeHalfAxisZ[k] >> 12);
    if (sum > 0) {
        return ((u32)n >> 4) & 7;
    }
    return ((u32)n >> 7) & 7;
}

s32 WorldMapGetTileShape(WorldMapPos* p) {
    return WorldMapGetTileValue(p) & WORLD_MAP_TILE_SHAPE_MASK;
}

s32 WorldMapGetTileAttrBits10To15(WorldMapPos* p) {
    return ((u32)WorldMapGetTileValue(p) << 16) >> 26;
}

s32 WorldMapGetTileGroup(WorldMapPos* p) {
    WorldMapTileZoneCell* pCell;

    pCell = (WorldMapTileZoneCell*)WorldMapGetTileEntry(p->x, p->z);
    return (pCell->unk3 >> 2) & 0xF;
}

s16 WorldMapGetTileKindValue(s16 row, s16 col) {
    return *(s16*)&g_WorldMapTileKindTable[row].kind[col * 2];
}

s32 WorldMapGetTileShapeAxisDirection(WorldMapPos* p, WorldMapPos* pPos, WorldMapPos* pOut) {
    s16 k;
    s32 sum;

    k = WorldMapGetTileShape(p);
    sum = pPos->x * g_WorldMapTileShapeAxis[k].x + pPos->z * g_WorldMapTileShapeAxis[k].z;
    if (sum == 0) {
        pOut->x = pPos->x;
        pOut->z = pPos->z;
        return 0;
    }
    if (sum < 0) {
        pOut->x = -g_WorldMapTileShapeAxis[k].x;
        pOut->z = -g_WorldMapTileShapeAxis[k].z;
    } else {
        pOut->x = g_WorldMapTileShapeAxis[k].x;
        pOut->z = g_WorldMapTileShapeAxis[k].z;
    }
    return 1;
}
/* best: 98.21% (112 bytes, original 112; 28 insns both). Instruction sequence, size,
   mult/mflo placement and the branches are identical; 16 differing rows are all one
   mirror. Retail ties each difference's dest to its MINUEND (`lw v0,0(a0)`, `subu
   v0,v0,v1`, and for the z pair it reuses dx's dead $v0 for the load so the subu is in
   place again, mflo a2/v1); this build ties dest to the SUBTRAHEND (`lw v1,0(a0)`, `subu
   v0,v1,v0`) and gives the z pair a FRESH $v0 with p0->z in the dead $a0 (`subu
   v0,a0,v1`, mflo a3/v0). Same six registers, permuted.
   Tried here and earlier, all measured at 112 bytes / 28 insns unless noted:
   single-expression sums 72.86, separate sqx+sqz locals 79.46, six declaration orders
   of {dx,dz,sum}, one reused `d` local, split statements (`d = p0->x; d -= p1->x;`)
   91.79. Preset: gcc-2.7.2-cdk scores the SAME 98.21 (an allocator tie, not a compiler
   one); gcc-2.6.0 refuses to compile the body. A 4-minute permuter run (2193
   iterations, base 50 best 30) found only a semantically identical duplicated negu.
   Measured this round: `u32 sum` (same allocno size, the lever's allocno_size axis) is
   byte-identical to `s32 sum` at 98.21 - the sum's mode is not part of the contest.
   This is the local-alloc qty_compare class in tools/rounds/levers.md
   (a-global-allocno-priority-is / a-lreg-dump-gives-the): there is no callee-saved
   register in play at all, so the -dl/-da dump has nothing to decide - the contest is
   between two 1-ref dying quantities inside one block. Do not hand-iterate it.
   Round 433 measured the split/in-place family (all 112 bytes / 28 insns unless
   noted): `dx = p0->x; dx -= p1->x;` alone is 91.79 (cc1 hoists one sra into the
   bgez delay slot and duplicates it); the same split plus `dx >>= 12; sum = dx*dx;`
   is 95.00 - it DOES give retail's subu tie (dest = the minuend's register) and
   retail's bgez/nop/negu/sra/mult chain exactly, but the load/dest pair moves to
   $v1 (retail $v0) and the z half to $a0/$v0, so the mirror only relocates; and
   writing the difference as `p1->x - p0->x` (identical under the abs) costs a
   `move a2,a0` parameter copy, 116 bytes / 29 insns, 94.29. So the constant form's
   mirrored tie and the in-place form's mirrored register pair are exclusive and the
   98.21 constant form above stays the best.
s32 func_80094154(WorldMapPos* p0, WorldMapPos* p1) {
    s32 dx;
    s32 dz;
    s32 sum;

    dx = p0->x - p1->x;
    if (dx < 0) {
        dx = -dx;
    }
    sum = (dx >> 12) * (dx >> 12);
    dz = p0->z - p1->z;
    if (dz < 0) {
        dz = -dz;
    }
    sum += (dz >> 12) * (dz >> 12);
    return SquareRoot0(sum);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80094154);
void WorldMapBuildDirectionFromPositions(WorldMapPos* p0, WorldMapPos* p1, s32* pVector, s16* pAngle) {
    s32 dx;
    s32 dz;

    dx = p1->x - p0->x;
    dz = p1->z - p0->z;
    *pAngle = (ratan2(dz, dx) + WORLD_MAP_ANGLE_QUARTER_TURN) & WORLD_MAP_ANGLE_MASK;
    pVector[0] = rcos(*pAngle);
    pVector[2] = -rsin(*pAngle);
}

