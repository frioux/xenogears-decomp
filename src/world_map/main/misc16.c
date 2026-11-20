#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"

#define WORLD_MAP_GRID_CELL_COUNT 81
#define WORLD_MAP_GRID_WIDTH 9
#define WORLD_MAP_TILE_SECTOR_SIZE 0x710

extern VECTOR g_WorldMapClipPlaneAxisY;
extern VECTOR g_WorldMapClipPlaneAxisX;
extern VECTOR g_WorldMapClipPlaneDirXNeg;
extern VECTOR g_WorldMapClipPlaneDirXPos;
extern VECTOR g_WorldMapClipPlaneDirYNeg;
extern VECTOR g_WorldMapClipPlaneDirYPos;
extern VECTOR g_WorldMapClipPlaneYZ1;
extern VECTOR g_WorldMapClipPlaneXZ0;
extern VECTOR g_WorldMapClipPlaneXZ1;
extern VECTOR g_WorldMapClipPlaneYZ0;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapMisc16Struct; // Size: 0xC

extern u16 g_WorldMapSlotUpdateFlags;
extern s16 g_WorldMapGridOrigin;
extern s16 D_8009C83C;

typedef struct {
    /* 0x0 */ s16 sin;
    /* 0x2 */ s16 pad2;
} WorldMapMisc16TblEntry; // Size: 0x4

typedef struct {
    /* 0x0 */ u32 vxy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ u16 pad6;
} WorldMapMisc16Pkt; // Size: 0x8

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 pad2;
    /* 0x4 */ s16 y;
} WorldMapMisc16Rect; // Size: 0x6

typedef struct {
    /* 0x0 */ s16 grid[81];
} WorldMapMisc16Grid; // Size: 0xA2

extern WorldMapMisc16Grid D_8009D318;
extern WorldMapMisc16Grid g_WorldMapGridTileIds;
extern s32 g_WorldMapTileColumns;
extern s32 g_WorldMapTileRows;

extern WorldMapMisc16TblEntry D_800523F0[];
extern s32 g_WorldMapGridShadePhase;
extern s32 g_WorldMapGridAngle;

void WorldMapComputeClipPlaneVectors(void) {
    OuterProduct0(&g_WorldMapClipPlaneDirXNeg, &g_WorldMapClipPlaneAxisY, &g_WorldMapClipPlaneXZ0);
    OuterProduct0(&g_WorldMapClipPlaneAxisY, &g_WorldMapClipPlaneDirXPos, &g_WorldMapClipPlaneXZ1);
    OuterProduct0(&g_WorldMapClipPlaneDirYNeg, &g_WorldMapClipPlaneAxisX, &g_WorldMapClipPlaneYZ0);
    OuterProduct0(&g_WorldMapClipPlaneAxisX, &g_WorldMapClipPlaneDirYPos, &g_WorldMapClipPlaneYZ1);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800980D4);
/* 93.98%, body is 240 bytes like retail: only the register names of the 0x800000
   constant and of the y local are swapped (retail y=a2, const=a0; ours y=a0,
   const=a2), an allocator tie - 4 decl/init orders all give the same pairing.
   Shapes that are now right and must stay: x and y are BOTH locals loaded before
   the g_WorldMapSlotUpdateFlags store (that is the retail top: x load, z load, li, sh);
   the x branch writes p->x = x + / - 0x800000 (a += reloads and emits a
   per-branch flag store); the y branch reads the y local but reloads p->z for
   the store; the tail reloads through a y temp (y = p->x; g_WorldMapGridOrigin = (y >> 23) + 2;
   y = p->z; D_8009C83C = (y >> 23) + 2;) - the direct form instead lets gcc hoist
   the second load into the first load's delay slot and defer both stores (59 insns, 236 bytes).
void func_800980D4(WorldMapMisc16Struct* p) {
    s32 y;
    s32 x;
    x = p->x;
    y = p->z;
    g_WorldMapSlotUpdateFlags = 0;
    if (x < -0x800000) {
        p->x = x + 0x800000;
        g_WorldMapSlotUpdateFlags = 4;
    } else if (x > 0x800000) {
        p->x = x - 0x800000;
        g_WorldMapSlotUpdateFlags = 8;
    }
    if (y < -0x800000) {
        p->z += 0x800000;
        g_WorldMapSlotUpdateFlags |= 1;
    } else if (y > 0x800000) {
        p->z -= 0x800000;
        g_WorldMapSlotUpdateFlags |= 2;
    }
    y = p->x;
    g_WorldMapGridOrigin = (y >> 23) + 2;
    y = p->z;
    D_8009C83C = (y >> 23) + 2;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800981C8);
/* 96.44%: same instructions in the same order, only the register names differ
   (retail col=a3 row=t1 w=t2 dst=t0 bound=t3 colBase=t3 pGrid=a0 colCur=a3 off=a1;
   ours col=t1 row=t0 w=t3 dst=a3 bound=t2 colBase=a0 pGrid=a2 colCur=v1 off=a3), so
   objdiff can never reach 100 here without snapping gcc's allocator tie-break. Shapes
   that matter and are now right: w/h must be locals or the in-loop loads are not hoisted;
   `for (i = 8; i != -1; i--)` both loops; the if/else-with-value form for the inner clamp
   (that is what duplicates `addu v0,off,colCur` into the delay slot and after r=0);
   row >>= 8 before the 0xA2-byte struct copy, colBase = col after it.
void func_800981C8(WorldMapMisc16Struct* p) {
    s32 col;
    s32 row;
    s32 colBase;
    s32 rowOff;
    s32 colCur;
    s32 i;
    s32 j;
    s32 w;
    s32 h;
    s16* pGrid;

    w = g_WorldMapTileColumns;
    h = g_WorldMapTileRows;
    col = (p->x >> 12) / 8 - ((g_WorldMapGridOrigin + 2) << 8);
    row = (p->z >> 12) / 8 - ((D_8009C83C + 2) << 8);
    if (col < 0) {
        col += w << 8;
    } else if ((w << 8) < col) {
        col -= w << 8;
    }
    if (row < 0) {
        row += h << 8;
    } else if ((h << 8) < row) {
        row -= h << 8;
    }
    col >>= 8;
    row >>= 8;
    D_8009D318 = g_WorldMapGridTileIds;
    colBase = col;
    for (i = 8; i != -1; i--) {
        if (!(row < h)) {
            row = 0;
        }
        colCur = colBase;
        rowOff = row * w;
        for (j = 8; j != -1; j--) {
            if (colCur >= w) {
                colCur = 0;
            }
            *pGrid++ = rowOff + colCur;
            colCur++;
        }
        row++;
    }
}
*/
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad6;
} WorldMapMisc16Point; // Size: 0x8

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
} WorldMapMisc16Tex; // Size: 0x8

typedef struct {
    /* 0x0 */ WorldMapMisc16Tex entry[25];
} WorldMapMisc16TileMask; // Size: 0xC8

extern WorldMapMisc16Tex D_8009D650[];
extern s16 D_8009D618[];
extern WorldMapMisc16TileMask D_8009B7A8[];
extern MATRIX g_WorldMapGridMatrix;
extern MATRIX g_WorldMapViewMatrix;
extern s32 func_800987AC(WorldMapMisc16Point* p0, WorldMapMisc16Point* p1, WorldMapMisc16Point* p2, WorldMapMisc16Point* p3);

/* 75.17%, 1060 bytes against retail's 1036 (259 insns). Was 74.38 / 1048; the three
   changes below are new this round and must stay.
   WHAT IT IS: clip a 5x5 block of grid cells against four frustum planes. The prologue
   copies g_WorldMapGridMatrix to 0x1F8000F0, CompMatrix's g_WorldMapViewMatrix into
   0x1F800110 and sets both GTE matrices. 0x1F800040 gets -(x>>12 & 0x7FF) - 0x1000, the
   same for z goes to 0x1F800048 and its negation to 0x1F8000A4. Nine 8-byte points (vx,
   vy, vz: an SVECTOR layout) live at 0x1F8000A0 + 8*k, k = 0..8, a 3x3 grid: the vy of
   each is cleared first, in the descending order 0xE2, 0xDA ... 0xA2, each through its
   own lui $at, so those nine are absolute-cast stores and not base-relative.
   Per cell: the four grid corners (0xA0/0xA8/0xB0/0xB8) go to func_800987AC (the
   handwritten GTE clip test, 4 points -> -1/0/1) and, only when the first test returns 0,
   the five inner points 0xC0/0xC8/0xD0/0xD8/0xE0 are built and handed to four more
   tests. Every call passes its four points in the order (top-left, top-right,
   bottom-left, bottom-right); the first result goes to D_8009D618[k] (s16, 25 cells) and
   the four results are packed two per u32 into D_8009D650[k] (word0 = r0 | r1<<16,
   word1 = r2 | r3<<16; a non-zero first result is written to both words). Tail:
   nVariant = (x>=0x400) | (z>=0x400 ? 2 : 0) picks one of four 0xC8-byte records of
   D_8009B7A8 and ORs its 25 8-byte mask words into D_8009D650.
   NEW THIS ROUND (all measured live):
   FIX 4 (wrong code found in the old park): the old body passed call 3 as
   (pPt5, pPt2, pPt8, pPt7); retail is (0xC8, 0xE0, 0xB0, 0xD8), i.e. (TL, TR, BL, BR)
   like the other three calls. The swapped a1/a2 pair alone was worth nothing (74.38 ->
   74.42) but the body was WRONG, so do not restore it.
   FIX 5 (the addiu lever): retail forms 0xC0/0xC8/0xD0/0xE0 per ROW with
   `addiu $sN,$s2,K` off its scratch base and 0xD8 inside the quad block, and only
   0xA0..0xB8 as two-instruction lui/ori constants. This cc1 always folds
   `pScratch + K` to the constant 0x1F8000K and materialises it with lui/ori, because
   pScratch (0x1F800000) needs ONE instruction and cse records it; the addiu form comes
   back as soon as the base needs TWO instructions, since cse cannot record those. So
   keep ONE point base `pPt = (WorldMapMisc16Point*)(pScratch + 0xA0)` and pass
   `&pPt[k]` (k = 0..8) instead of five separate 0xC0..0xE0 pointer locals: the inner
   four then come out as `addiu $sN,$<pPt>,K` at the row head, exactly retail's shape
   (74.38 -> 75.17). The remaining price is that pPt occupies a second callee-saved
   register, which is why the whole set is still one place rotated.
   FIX 6: with pPt as the only point base, retail's per-row formation is reproduced but
   the four corner pointers must come from pPt too, so 0xA0 is materialised once into a
   register instead of spilled like retail's 0x18(sp)/0x20(sp)/0x28(sp)/0x30(sp) slots.
   Tried and NOT better: corner pointers as absolute literals (0x1F8000A0 etc.) plus the
   inner ones from pPt scores 75.42 at 1016 bytes, i.e. 5 instructions SHORT of retail,
   so it cannot link - it also loses the per-row addiu group.
   STILL WRONG: 6 instructions over. p->z is loaded with p->x before the nine zero
   stores (retail loads z after them, through the same $t6 it kept live); pClip is
   spilled and reloaded per column (retail keeps it in $s5) because pPt takes the
   register retail gives pClip; the first quad block's six stores come out
   A8,AC,B0,B4,B8,BC where retail has A8,B4,AC,B0,B8,BC (source order is already
   retail's; measured earlier: typed field stores 69.44 at 1076, a volatile on the
   first two stores and on none both 74.38); the tail OR loop is 1 instruction long
   (retail keeps two destination cursors, s3 and s3+4). Register map wanted:
   base 0x1F800000 = $s2, pTex = $s3, col = $s4, pClip = $s5, 0xE0 = $s6, row = $s7,
   0xD0 = $fp, nPack = $s1, 0xD8 = $s0.
   Not a preset question: gcc-2.7.2-cdk scores 70.06 and gcc-2.6.0 69.89 on this body.
   A 3-minute permuter run on the old 42.23 body produced only wrong-code candidates.
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800983A0);
/* void func_800983A0(WorldMapMisc16Struct* p) {
    WorldMapMisc16Point* pPt;
    WorldMapMisc16Tex* pTex;
    WorldMapMisc16Tex* pMaskEntry;
    WorldMapMisc16TileMask* pMask;
    s16* pClip;
    u8* pScratch;
    s32 nX;
    s32 nZ;
    s16 nClip;
    s32 nPack;
    s32 nVariant;
    s32 row;
    s32 col;

    pScratch = (u8*)0x1F800000;
    *(MATRIX*)0x1F8000F0 = g_WorldMapGridMatrix;
    CompMatrix(&g_WorldMapViewMatrix, (MATRIX*)0x1F8000F0, (MATRIX*)0x1F800110);
    SetRotMatrix((MATRIX*)0x1F800110);
    SetTransMatrix((MATRIX*)0x1F800110);

    pPt = (WorldMapMisc16Point*)(pScratch + 0xA0);

    nX = -((p->x >> 12) & 0x7FF) - 0x1000;
    *(s32*)0x1F800040 = nX;

    row = 0;
    *(u16*)0x1F8000E2 = 0;
    *(u16*)0x1F8000DA = 0;
    *(u16*)0x1F8000D2 = 0;
    *(u16*)0x1F8000CA = 0;
    *(u16*)0x1F8000C2 = 0;
    *(u16*)0x1F8000BA = 0;
    *(u16*)0x1F8000B2 = 0;
    *(u16*)0x1F8000AA = 0;
    *(u16*)0x1F8000A2 = 0;

    nZ = -((p->z >> 12) & 0x7FF) - 0x1000;
    *(s32*)0x1F800048 = nZ;
    *(u16*)0x1F8000A4 = -nZ;

    pClip = D_8009D618;
    pTex = D_8009D650;
    for (; row < 5; row++) {
        *(u16*)(pScratch + 0xA0) = *(s32*)0x1F800040;
        for (col = 0; col < 5; col++) {
            *(u16*)(pScratch + 0xA8) = *(u16*)(pScratch + 0xA0) + 0x800;
            *(u16*)(pScratch + 0xB4) = *(u16*)(pScratch + 0xA4) - 0x800;
            *(u16*)(pScratch + 0xAC) = *(u16*)(pScratch + 0xA4);
            *(u16*)(pScratch + 0xB0) = *(u16*)(pScratch + 0xA0);
            *(u16*)(pScratch + 0xB8) = *(u16*)(pScratch + 0xA0) + 0x800;
            *(u16*)(pScratch + 0xBC) = *(u16*)(pScratch + 0xA4) - 0x800;
            nClip = func_800987AC(&pPt[0], &pPt[1], &pPt[2], &pPt[3]);
            *pClip++ = nClip;
            if (nClip == 0) {
                *(u16*)(pScratch + 0xC0) = *(u16*)(pScratch + 0xA0) + 0x400;
                *(u16*)(pScratch + 0xC4) = *(u16*)(pScratch + 0xA4);
                *(u16*)(pScratch + 0xC8) = *(u16*)(pScratch + 0xA0);
                *(u16*)(pScratch + 0xCC) = *(u16*)(pScratch + 0xA4) - 0x400;
                *(u16*)(pScratch + 0xD0) = *(u16*)(pScratch + 0xA8);
                *(u16*)(pScratch + 0xD4) = *(u16*)(pScratch + 0xCC);
                *(u16*)(pScratch + 0xD8) = *(u16*)(pScratch + 0xC0);
                *(u16*)(pScratch + 0xDC) = *(u16*)(pScratch + 0xB4);
                *(u16*)(pScratch + 0xE0) = *(u16*)(pScratch + 0xC0);
                *(u16*)(pScratch + 0xE4) = *(u16*)(pScratch + 0xCC);
                nClip = func_800987AC(&pPt[0], &pPt[4], &pPt[5], &pPt[8]);
                nPack = nClip & 0xFFFF;
                nClip = func_800987AC(&pPt[4], &pPt[1], &pPt[8], &pPt[6]);
                nPack |= nClip << 16;
                *(u32*)pTex = nPack;
                nClip = func_800987AC(&pPt[5], &pPt[8], &pPt[2], &pPt[7]);
                nPack = nClip & 0xFFFF;
                nClip = func_800987AC(&pPt[8], &pPt[6], &pPt[7], &pPt[3]);
                nPack |= nClip << 16;
            } else {
                nPack = nClip & 0xFFFF;
                nPack |= nPack << 16;
                *(u32*)pTex = nPack;
            }
            pTex->unk4 = nPack;
            pTex++;
            *(u16*)(pScratch + 0xA0) += 0x800;
        }
        *(u16*)(pScratch + 0xA4) -= 0x800;
    }

    nX = p->x;
    nZ = p->z;
    nVariant = !(((nX >> 12) & 0x7FF) < 0x400);
    if (!(((nZ >> 12) & 0x7FF) < 0x400)) {
        nVariant |= 2;
    }
    pMask = &D_8009B7A8[nVariant];
    pTex = D_8009D650;
    pMaskEntry = pMask->entry;
    for (row = 0; row < 25; row++) {
        pTex->unk0 |= pMaskEntry->unk0;
        pTex->unk4 |= pMaskEntry->unk4;
        pTex++;
        pMaskEntry++;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800987AC);
extern s32 g_WorldMapGridImageFileIndex;
extern s32 g_WorldMapGridColumnImageFileIndex;
extern s16 g_WorldMapGridCornerTileIndices[];
extern u8* g_WorldMapTileCellPointers[];
extern u32 ArchiveGetDebugTable(void);
extern s32 ArchiveDecodeSector(s32 entryIndex);
extern u8* ArchiveGetFilePath(s32 entryIndex);
extern s32 WorldMapAddCdRequest(s32 x, s32 y, s32 z);
extern s32 WorldMapAddFileLoadRequest(s32 x, s32 y, s32 z, s32 w);
extern s32 WorldMapSubmitCdRequests(void);
extern s32 WorldMapSubmitFileLoads(void);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80098CC0);
/* 96.96%, body is exactly retail's 1644 bytes and every call, branch and loop tail
   matches: what is left is ONE allocator rotation plus one base-register choice.
   Shape that got it here (all measured, keep them):
   - the tile ids are walked with an INDEX (id = g_WorldMapGridTileIds.grid[idx]; idx++),
     never with a pId++ pointer. With a walking pointer gcc keeps a constant -1 in a
     callee-saved register for the two loop bounds, which steals the register that
     holds g_WorldMapGridTileIds and makes every loop preheader re-form that address
     (82.49% at the same byte count). The indexed form makes gcc re-materialise
     'li v0,-1' per iteration exactly as retail does, and the symbol base lands in s8.
   - the free pass (D_8009D318 vs the 81 live ids) is a plain count-up for/break loop.
   - the two ArchiveGetDebugTable() calls are BOTH evaluated before the test, so the
     condition has to be the bitwise form ((first == 0) | (second == 0xFFFFFFFF)) that
     misc15's WorldMapSubmitGridImageLoads already uses and matches with.
   - the 7-cell runs are for (i = 1; i != -1; i--) around for (j = 6; j != -1; j--),
     with idx reset to 0x49 / 0x11 at the end of the outer body (retail re-materialises
     those two constants in the outer back-branch delay slot; an idx += 0x48 or a
     row * 0x48 + 1 spelling gives addiu instead of li and loses the match).
   - the four-row/column groups walk g_WorldMapGridTileIds.grid[idx] with stride 1
     (rows) and stride 9 (columns), bases 1/0x49 and 9/0x11.
   - loaded (& 2 / & 1) must be read into two locals BEFORE the four-cell g_WorldMapGridCornerTileIndices
     loop or the masks are recomputed inside it (that was 4 bytes and 2.4 points).
   Still wrong: retail puts the outer counter in s4 and the sector pointer in s5, ours
   swaps them (declaration order, splitting the free-pass counter into its own variable
   and moving the pointer declarations all measure 96.95-96.96 identical code), retail
   keeps the idx constant in s0 where ours uses v0, and in the four-cell block retail
   forms the g_WorldMapGridTileIds address with lui $at / addu $at where ours hoists
   the base into a saved register. A 4-minute permuter run's only find was a u8
   intermediate at one call site, which truncates the offset argument - wrong code,
   do not port it. Next lever: a cc1_probe of the four-cell block, or try making the
   free pass and the nest loops share one counter variable.
/*
void func_80098CC0(void) {
    u8* pSector;
    u8* pPath;
    s32 i;
    s32 j;
    s32 k;
    s32 idx;
    s32 id;
    s32 loaded;
    s32 hasImage;
    s32 hasSector;
    u32 first;
    u32 second;

    for (i = 0; i < WORLD_MAP_GRID_CELL_COUNT; i++) {
        id = D_8009D318.grid[i];
        if (g_WorldMapTileCellPointers[id] != NULL) {
            for (j = 0; j < WORLD_MAP_GRID_CELL_COUNT; j++) {
                if (g_WorldMapGridTileIds.grid[j] == id) {
                    break;
                }
            }
            if (j == WORLD_MAP_GRID_CELL_COUNT) {
                HeapFree(g_WorldMapTileCellPointers[id]);
                g_WorldMapTileCellPointers[id] = NULL;
            }
        }
    }

    first = ArchiveGetDebugTable();
    second = ArchiveGetDebugTable();
    loaded = 0;
    if ((first == 0) | (second == 0xFFFFFFFF)) {
        pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridImageFileIndex);
        idx = 1;
        for (i = 1; i != -1; i--) {
            for (j = 6; j != -1; j--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddCdRequest((s32)(pSector + id), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 2;
                }
                idx++;
            }
            idx = 0x49;
        }
        pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridColumnImageFileIndex);
        idx = 9;
        for (i = 1; i != -1; i--) {
            for (j = 6; j != -1; j--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddCdRequest((s32)(pSector + ((id % g_WorldMapTileColumns) * g_WorldMapTileRows + id / g_WorldMapTileColumns)), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 1;
                }
                idx += WORLD_MAP_GRID_WIDTH;
            }
            idx = 0x11;
        }
        hasImage = loaded & 2;
        hasSector = loaded & 1;
        for (k = 0; k < 4; k++) {
            id = g_WorldMapGridTileIds.grid[g_WorldMapGridCornerTileIndices[k]];
            if (g_WorldMapTileCellPointers[id] == NULL) {
                g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                if (hasImage) {
                    pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridImageFileIndex);
                    WorldMapAddCdRequest((s32)(pSector + id), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                } else if (hasSector) {
                    pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridColumnImageFileIndex);
                    WorldMapAddCdRequest((s32)(pSector + ((id % g_WorldMapTileColumns) * g_WorldMapTileRows + id / g_WorldMapTileColumns)), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                }
            }
        }
        WorldMapAddCdRequest(0, 0, 0);
        WorldMapSubmitCdRequests();
    } else {
        pPath = ArchiveGetFilePath(g_WorldMapGridImageFileIndex);
        idx = 1;
        for (i = 1; i != -1; i--) {
            for (j = 6; j != -1; j--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddFileLoadRequest((s32)pPath, id << 11, WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 2;
                }
                idx++;
            }
            idx = 0x49;
        }
        pPath = ArchiveGetFilePath(g_WorldMapGridColumnImageFileIndex);
        idx = 9;
        for (i = 1; i != -1; i--) {
            for (j = 6; j != -1; j--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddFileLoadRequest((s32)pPath, ((id % g_WorldMapTileColumns) << 11) * g_WorldMapTileRows + ((id / g_WorldMapTileColumns) << 11), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 1;
                }
                idx += WORLD_MAP_GRID_WIDTH;
            }
            idx = 0x11;
        }
        hasImage = loaded & 2;
        hasSector = loaded & 1;
        for (k = 0; k < 4; k++) {
            id = g_WorldMapGridTileIds.grid[g_WorldMapGridCornerTileIndices[k]];
            if (g_WorldMapTileCellPointers[id] == NULL) {
                g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                if (hasImage) {
                    pPath = ArchiveGetFilePath(g_WorldMapGridImageFileIndex);
                    WorldMapAddFileLoadRequest((s32)pPath, id << 11, WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                } else if (hasSector) {
                    pPath = ArchiveGetFilePath(g_WorldMapGridColumnImageFileIndex);
                    WorldMapAddFileLoadRequest((s32)pPath, ((id % g_WorldMapTileColumns) << 11) * g_WorldMapTileRows + ((id / g_WorldMapTileColumns) << 11), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                }
            }
        }
        WorldMapAddFileLoadRequest(0, 0, 0, 0);
        WorldMapSubmitFileLoads();
    }
}
*/
extern WorldMapMisc16Tex D_8009D654[];
extern u16 D_8009CCB4[];
extern u16 g_WorldMapTpageIds[];
extern s32 D_8009D7DC;
extern void func_80099708(u8* pSrc, u8* p1, u8* p2, WorldMapMisc16Rect* pRect);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009932C);
/* 63.53% first attempt. Structure read off the asm and believed right: two u16 copy
   loops from D_8009CCB4 (0x40) and g_WorldMapTpageIds (7) into scratchpad 0x1F800288/0x1F800308,
   the 0x20-byte MATRIX copy of g_WorldMapGridMatrix to 0x1F800350, CompMatrix/SetRotMatrix/
   SetTransMatrix on 0x1F800370, the 5x5 tile loop (row/fp, col/s4, flat k/s7 whose *8
   biv is retail's s3), the four RECT corners built at 0x1F800330/0x338 from the volatile
   0x1F800328/0x1F80032C coords, and the four func_80099708 calls selected by
   (s16)(D_8009D650[k].unk0 | .unk2 | D_8009D654[k].unk0 | .unk2) == -1. Scratchpad
   accesses MUST stay volatile: retail reloads 0x1F800320 right after storing it and
   loads 0x1F800328 twice per iteration, which cse only allows through volatile.
   What is wrong: allocation only. Retail col=s4 k=s2? no - retail has base=s2, col=s4,
   row=s8, k=s7 and frame 0x38 (10 saves); ours puts base in s6, col in s5, spills to a
   0x40 frame (11 slots) and reorders the k++/col++ epilogue. 6 shapes not tried yet.
   Next: try col/k as a single flat counter, and read D_8009D650[k] into locals before
   the coords so the OR chain does not force the base register.
void func_8009932C(u8* p0, u8* p1, WorldMapMisc16Struct* p) {
    s32 i;
    s32 row;
    s32 col;
    s32 k;
    s16 x;
    s16 y;
    s16 all;
    u8* pImg;

    for (i = 0; i < 0x40; i++) {
        ((u16*)0x1F800288)[i] = D_8009CCB4[i];
    }
    for (i = 0; i < 7; i++) {
        ((u16*)0x1F800308)[i] = g_WorldMapTpageIds[i];
    }
    *(MATRIX*)0x1F800350 = g_WorldMapGridMatrix;
    CompMatrix(&g_WorldMapViewMatrix, (MATRIX*)0x1F800350, (MATRIX*)0x1F800370);
    SetRotMatrix((MATRIX*)0x1F800370);
    SetTransMatrix((MATRIX*)0x1F800370);
    *(s32*)0x1F800318 = -((p->x >> 12) & 0x7FF) - 0x1000;
    *(s32*)0x1F800320 = -((p->z >> 12) & 0x7FF) - 0x1000;
    *(volatile u16*)0x1F80032C = -*(s32*)0x1F800320;
    D_8009D7DC = 0;
    k = 0;
    for (row = 0; row < 5; row++) {
        *(volatile u16*)0x1F800328 = *(s32*)0x1F800318;
        for (col = 0; col < 5; col++, k++) {
            if (D_8009D618[k] == -1) {
                continue;
            }
            y = *(volatile u16*)0x1F80032C;
            x = *(volatile u16*)0x1F800328;
            pImg = g_WorldMapTileCellPointers[g_WorldMapGridTileIds.grid[(row + D_8009C83C) * 9 + col + g_WorldMapGridOrigin]];
            *(volatile u16*)0x1F800330 = *(volatile u16*)0x1F800340 = *(volatile u16*)0x1F800328 + 0x400;
            *(volatile u16*)0x1F800334 = y;
            *(volatile u16*)0x1F800338 = x;
            *(volatile u16*)0x1F80033C = *(volatile u16*)0x1F800344 = *(volatile u16*)0x1F80032C - 0x400;
            all = D_8009D650[k].unk0 | D_8009D650[k].unk2 | D_8009D654[k].unk0 | D_8009D654[k].unk2;
            if (all != -1) {
                func_80099708(pImg, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800328);
                func_80099708(pImg + 0x144, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800330);
                func_80099708(pImg + 0x288, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800338);
                func_80099708(pImg + 0x3CC, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800340);
            } else {
                if (y != all) {
                    func_80099708(pImg, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800328);
                }
                if (D_8009D650[k].unk2 != all) {
                    func_80099708(pImg + 0x144, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800330);
                }
                if (D_8009D654[k].unk0 != all) {
                    func_80099708(pImg + 0x288, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800338);
                }
                if (D_8009D654[k].unk2 != all) {
                    func_80099708(pImg + 0x3CC, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800340);
                }
            }
        }
        *(volatile u16*)0x1F800328 += 0x800;
        *(volatile u16*)0x1F80032C -= 0x800;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099708);
/* 78.20%: loop shape and instruction multiset now match retail (`for (i = 8; i != -1; i--)`
   gives the addiu/-1 countdown; the if/else assigns the *already <<16-shifted* value in each
   branch, which is what puts retail's sll in the `j` delay slot and folds the else path to
   `sll 24; sra 5`). What is left is a global register-allocation rotation: retail keeps
   pSrc in s0 (frame 0x20, saves s0+s1) and the mult temp in s1, and uses a0=x / a3=xCur /
   v1=v, while our compile keeps pSrc in a0 (frame 0x18) and puts idx/ii in a2/a3. 6 source
   shapes (incl. named product local) all score 78.20, and cdk is also 78.2, so it is not
   the preset. Retail's `addiu t0,t0,%lo(D_1F800008)` is splat naming the +8 of pPkt++, i.e.
   a reloc the C cannot emit; objdiff can never reach 100 here even if the allocation did.
void func_80099708(u32* pSrc, s32 unk1, s32 unk2, WorldMapMisc16Rect* pRect) {
    WorldMapMisc16Pkt* pPkt;
    WorldMapMisc16TblEntry* pTbl;
    u32* p;
    s32 x;
    s32 y;
    s32 xCur;
    s32 col;
    s32 iCol;
    s32 idx;
    s32 oi;
    s32 ii;
    u32 v;
    s32 c;

    pPkt = (WorldMapMisc16Pkt*)0x1F800000;
    pTbl = D_800523F0;
    p = pSrc;
    x = pRect->x;
    y = pRect->y;
    col = g_WorldMapGridShadePhase;
    for (oi = 8; oi != -1; oi--) {
        xCur = x;
        iCol = pTbl[col & 0xFFF].sin * 2;
        idx = g_WorldMapGridAngle;
        for (ii = 8; ii != -1; ii--) {
            v = *p;
            if (v & 0x1000) {
                c = (((pTbl[idx & 0xFFF].sin * iCol) >> 20) + ((s8)v << 3)) << 16;
            } else {
                c = (s8)v << 19;
            }
            pPkt->vxy = c | (xCur & 0xFFFF);
            pPkt->vz = y;
            xCur += 0x80;
            pPkt++;
            p++;
            idx += 0x200;
        }
        y -= 0x80;
        col += 0x200;
    }
    func_8009980C(pSrc);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009980C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099BFC);
