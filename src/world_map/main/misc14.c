#include "common.h"

// Same sentinel as include/main/game.h's WORLD_MAP_AREA_NONE (this unit does not include
// game.h); duplicated here so the name is used, not the literal.
#define WORLD_MAP_AREA_NONE (-1)

#include "system/memory.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} SVec; // Size: 0xC

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} SVec4; // Size: 0x10

typedef struct {
    /* 0x0 */ s16 left;
    /* 0x2 */ s16 top;
    /* 0x4 */ s16 width;
    /* 0x6 */ s16 height;
    /* 0x8 */ s16 kind;
    /* 0xA */ s16 unkA;
    /* 0xC */ u16 value;
    /* 0xE */ s16 mode;
} WorldMapRect; // Size: 0x10

extern int Vsync(int mode);
extern u32 ArchiveGetDebugTable(void);
extern s32 g_WorldMapLoadQueueHead;
extern s32 g_WorldMapLoadQueueTail;
extern s32 g_WorldMapLoadRecordCount;
extern SVec* g_WorldMapCdRequestRecords;
extern SVec4* g_WorldMapFileLoadRecords;
extern void* g_WorldMapCdSectorRemainder;
extern void* g_WorldMapCdRequestQueue[16];
extern void* g_WorldMapFileLoadRequestQueue[16];
extern WorldMapRect** g_WorldMapRects;
extern void* g_WorldMapAreaRect;
extern s16 g_WorldMapAreaId;
extern s16 g_WorldMapMode4AreaId;
extern void WorldMapSortCdRequestsBySector(SVec* pRecord);
extern void WorldMapSortFileLoadsByOffset(SVec4* pRecord);
extern void WorldMapUpdateLoadQueue(void);
extern void WorldMapWrapPosition(void* p);
extern s32 WorldMapGetTileHalfIndex(void* p);
extern s16 WorldMapGetTileKindValue(s16 row, s16 col);

// Scratchpad probe at 0x1F800000: the near/far pair at 0x20/0x30 is projected
// in place and the winning one is copied down to 0x00. v.x/v.z is a scratch
// temp; 0x10 and 0x18 are the x- and z-references WorldMapProbeCellEdgeXFromReference and
// WorldMapProbeCellEdgeZFromReference measure from. The three stores of a projection must keep the
// temp first: writing the scaled field first lets gcc forward the temp and drop
// the reload of it that the original does.
typedef struct {
    /* 0x00 */ SVec4 v;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ SVec4 near;
    /* 0x30 */ SVec4 far;
} WorldMapProbe; // Size: 0x40

s32 WorldMapSelectAreaAtPosition(SVec* p, s32 index) {
    WorldMapRect* pRect;
    u16 x;
    u16 z;

    x = (u32)p->x >> 12;
    pRect = g_WorldMapRects[index];
    z = (u32)p->z >> 12;
    while (pRect->kind != -1) {
        if ((x >= pRect->left) & (x <= pRect->left + pRect->width) & (z >= pRect->top) &
            (z <= pRect->top + pRect->height)) {
            if (pRect->mode == 4) {
                g_WorldMapAreaRect = (void*)-1;
                g_WorldMapAreaId = WORLD_MAP_AREA_NONE;
                g_WorldMapMode4AreaId = pRect->value;
            } else {
                g_WorldMapAreaRect = pRect;
                g_WorldMapMode4AreaId = WORLD_MAP_AREA_NONE;
                g_WorldMapAreaId = pRect->value;
            }
            return 1;
        }
        pRect++;
    }
    g_WorldMapAreaRect = (void*)-1;
    g_WorldMapAreaId = WORLD_MAP_AREA_NONE;
    g_WorldMapMode4AreaId = WORLD_MAP_AREA_NONE;
    return 0;
}

s32 WorldMapFindAreaOfModeAtPosition(SVec* p, s32 index, s32 mode) {
    WorldMapRect* pRect;
    u16 x;
    u16 z;

    x = (u32)p->x >> 12;
    pRect = g_WorldMapRects[index];
    z = (u32)p->z >> 12;
    while (pRect->kind != -1) {
        if ((x >= pRect->left) & (x <= pRect->left + pRect->width) & (z >= pRect->top) &
            (z <= pRect->top + pRect->height)) {
            if (pRect->mode == mode) {
                g_WorldMapAreaRect = pRect;
                return 1;
            }
        }
        pRect++;
    }
    return 0;
}

void WorldMapNop(void) {
}

s32 WorldMapProbeCellEdgeXFromReference(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->z << 12) / pAim->x;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pProbe->unk10 & 0xFFF80000) - pPos->x;
    pProbe->v.x = edge;
    pNear->x = edge - 1;
    pFar->x = pProbe->v.x;
    pNear->z = ((scale * pNear->x) >> 12) + pPos->z;
    pFar->z = ((scale * pFar->x) >> 12) + pPos->z;
    pNear->x += pPos->x;
    pFar->x += pPos->x;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 3 : 0;
}

s32 WorldMapProbeCellEdgeXFromPosition(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->z << 12) / pAim->x;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pPos->x & 0xFFF80000) - pPos->x;
    pProbe->v.x = edge;
    pNear->x = edge;
    pFar->x = pProbe->v.x - 1;
    pNear->z = ((scale * pNear->x) >> 12) + pPos->z;
    pFar->z = ((scale * pFar->x) >> 12) + pPos->z;
    pNear->x += pPos->x;
    pFar->x += pPos->x;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 3 : 0;
}

s32 WorldMapProbeCellEdgeZFromReference(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->x << 12) / pAim->z;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pProbe->unk18 & 0xFFF80000) - pPos->z;
    pProbe->v.z = edge;
    pNear->z = edge - 1;
    pFar->z = pProbe->v.z;
    pNear->x = ((scale * pNear->z) >> 12) + pPos->x;
    pFar->x = ((scale * pFar->z) >> 12) + pPos->x;
    pNear->z += pPos->z;
    pFar->z += pPos->z;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 2 : 0;
}

s32 WorldMapProbeCellEdgeZFromPosition(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->x << 12) / pAim->z;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pPos->z & 0xFFF80000) - pPos->z;
    pProbe->v.z = edge;
    pNear->z = edge;
    pFar->z = pProbe->v.z - 1;
    pNear->x = ((scale * pNear->z) >> 12) + pPos->x;
    pFar->x = ((scale * pFar->z) >> 12) + pPos->x;
    pNear->z += pPos->z;
    pFar->z += pPos->z;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 2 : 0;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094A5C);
extern s32 func_80094088(SVec* p0, SVec* p1, SVec* p2);
extern s32 func_80094A5C(void* p0, SVec* p1, s32 mode, s16 kind);

s32 WorldMapResolveStepAgainstTerrain(void* p0, SVec* p1, SVec* p2, s32 mode, s16 kind) {
    s32 ret;
    s32 state;
    s32 result;

    state = func_80094A5C(p0, p1, mode, kind);
    switch (state) {
    case 0:
        ret = 1;
        break;
    case 1:
        result = func_80094088((SVec*)0x1F800000, p1, p2);
        ret = 0;
        if (result == 0) {
            p2->z = 0;
            p2->y = 0;
            p2->x = 0;
        }
        break;
    case 2:
        p2->x = p1->x < 0 ? -0x1000 : 0x1000;
        p2->z = 0;
        p2->y = 0;
        ret = 0;
        break;
    case 3:
        p2->y = 0;
        p2->x = 0;
        p2->z = p1->z < 0 ? -0x1000 : 0x1000;
        ret = 0;
        break;
    }
    return ret;
}

void WorldMapAlignDirectionToReference(SVec* p0, SVec* p1, SVec* p2) {
    s32 dot = p2->x * p0->x + p2->z * p0->z;

    if (dot < 0) {
        p1->x = -p2->x;
        p1->z = -p2->z;
    } else if (dot > 0) {
        p1->x = p2->x;
        p1->z = p2->z;
    } else {
        p1->z = 0;
        p1->x = 0;
    }
    p1->y = 0;
}

void WorldMapAlignPerpendicularToReference(void* pOut, SVec* pDir, SVec* pResult) {
    s32 dot;
    s32 nx;

    *(s32*)0x1F800018 = 0;
    *(s32*)0x1F800010 = 0;
    *(s32*)0x1F800014 = -0x1000;
    OuterProduct12((VECTOR*)pOut, (VECTOR*)0x1F800010, (VECTOR*)0x1F800020);
    VectorNormal((VECTOR*)0x1F800020, (VECTOR*)0x1F800000);
    nx = ((SVec*)0x1F800000)->x;
    dot = nx * pDir->x + ((SVec*)0x1F800000)->z * pDir->z;
    if (dot < 0) {
        pResult->x = -nx;
        pResult->z = -((SVec*)0x1F800000)->z;
    } else if (dot > 0) {
        pResult->x = nx;
        pResult->z = ((SVec*)0x1F800000)->z;
    } else {
        pResult->z = 0;
        pResult->x = 0;
    }
    pResult->y = 0;
}
// A step leaves the triangle it started in through one of the triangle's three
// edges, so the mesh carries a neighbour index per edge: -1 where the triangle
// has no neighbour on that side.
typedef struct {
    /* 0x0 */ s16 vert[3];
    /* 0x6 */ s16 next[3];
    /* 0xC */ u16 kind;
} WorldMapStepTriangle; // Size: 0xE

typedef struct {
    /* 0x0 */ u8 unk0[8];
    /* 0x8 */ WorldMapStepTriangle tris[1];
} WorldMapStepMesh; // Size: 0x16

// One entry of the g_WorldMapGroups sprite-set table (stride 0x54); only the
// mesh of the set is read here.
typedef struct {
    /* 0x00 */ u8 unk0[0x44];
    /* 0x44 */ WorldMapStepMesh* pMesh;
    /* 0x48 */ u8 unk48[0xC];
} WorldMapStepSpriteSet; // Size: 0x54

// The walk keeps its working vectors in the scratchpad: one candidate direction
// per mesh edge, the position the step is resolved at, and the normal and the
// interpolated plane height of the triangle it is standing in.
typedef struct {
    /* 0x00 */ u8 unk0[0x30];
    /* 0x30 */ SVec4 dir[3]; // 0x30, 0x40, 0x50
    /* 0x60 */ SVec probe;
    /* 0x6C */ u32 pad6C;
    /* 0x70 */ SVec normalA;
    /* 0x7C */ u32 pad7C;
    /* 0x80 */ SVec normalB;
} WorldMapStepScratch; // Size: 0x8C

#define WORLD_MAP_STEP_SCRATCH ((WorldMapStepScratch*)0x1F800000)

// How a step is resolved: against the terrain only, by searching the sprite
// sets under the new position, or by walking the mesh of the sprite set the
// step is already standing in (2 and 3 share the walk).
#define WORLD_MAP_STEP_TERRAIN 0
#define WORLD_MAP_STEP_SEARCH 1
#define WORLD_MAP_STEP_WALK_EDGE 2
#define WORLD_MAP_STEP_WALK 3

#define WORLD_MAP_STEP_NONE (-1)

// A triangle the step crosses into is adopted when the plane of its normal is
// within this many units of the height the step is being resolved at.
#define WORLD_MAP_STEP_HEIGHT_TOLERANCE 11

// func_80085760's clip code: inside the triangle, past edge 0/1/2, past the
// corner between two of them, or outside the mesh altogether.
#define WORLD_MAP_CLIP_INSIDE 0
#define WORLD_MAP_CLIP_EDGE01 3
#define WORLD_MAP_CLIP_EDGE02 5
#define WORLD_MAP_CLIP_EDGE12 6

extern WorldMapStepSpriteSet* g_WorldMapGroups;
extern s32 D_8009C840;   // sprite set the step is being resolved in, -1 when none
extern s32 D_8009C16C;   // current triangle of that set
extern s16 D_8009D718[]; // candidate triangles: a triangle index and its kind per pair
extern s32 WorldMapGetTilePlaneHeight(s32 x, s32 z);
extern s32 WorldMapFindSpriteSet(void* pPos, s16* pSet);
extern s32 func_80085760(SVec* pPos, SVec* pRef, s32 setIndex, s32 triIndex);
extern s32 WorldMapCheckTriangleEdgeSide(SVec* pPos, s32 yOffset, u16 setIndex, u16 triIndex);


INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095414);
/* best: 60.05% (mine 2280 bytes vs retail's 2240, so 40 bytes / 10 insns long).
   Structure is mapped and the body compiles; what is left is the frame and the
   register allocation:
     - retail's frame is 0xB8 and it spills BOTH pBase (a0 -> 0x20(sp)) and
       scale (a3 -> 0x28(sp)) and reloads them at every use (`lw t1,0x20(sp)`);
       mine is 0xA8 and keeps pBase in a saved register, so the s-registers
       retail gives to i/found/triIndex go elsewhere.
     - retail keeps the scratchpad base 0x1F800000 in $fp (`lui $fp,(0x1F800080
       >> 16)`, parked in the first GetTilePlaneHeight jal delay slot) and
       reaches 0x30/0x40/0x50/0x60/0x70/0x80 as fp+offset; the early stores to
       0x60/0x64/0x68 use fresh `lui $at` instead.  With literal addresses gcc
       folds the whole 0x1F800060 into one saved register (the failure the
       func_80095CD4 note below describes); the WORLD_MAP_STEP_SCRATCH struct
       spelling below is 0.2% better.  Retail MIXES two spellings: the case-1
       ProjectTriangleNormal call uses the symbols (`addiu a0,fp,%lo(D_1F800060)`,
       `%lo(D_1F800070)`, `%lo(D_1F800080)`), while case 0's call and the three
       WorldMapAlignDirectionToReference calls use raw fp+0x30/0x40/0x50/0x60/
       0x70/0x80 offsets with no relocation.  Note the func_80095CD4 park below
       claims the symbolic spelling "emits %hi/%lo relocations retail does not
       have" -- for func_80095414 retail does have them, so try the symbols here.
   Facts worth keeping:
     - D_8009C840 is the sprite set the step is resolved in (-1 = none) and
       D_8009C16C its current triangle; both are s32 globals (lw/sw) holding
       16-bit values, read lhu for the triangle index and sign extended by the
       stores.  WORLD_MAP_STEP_NONE names the -1 of both.
     - ONE s16 local is both the address-taken output of WorldMapFindSpriteSet
       and the spilled set index (slot 0x18): it is `lh` where it is used
       plainly and `lhu` where cast to u16 for a u16 parameter, which is how a
       single variable explains both loads.
     - mesh triangle stride 0xE: vert[3] at 0x0, the three edge neighbours at
       0x6/0x8/0xA (-1 = none), a u16 at 0xC compared against 0 and 1.
       Neighbour[0] pairs with 0x1F800030, [1] with 0x40, [2] with 0x50.
     - func_80085760's clip code: 0 inside, 1/2/4 past an edge, 3/5/6 past a
       corner, >= 8 out of range.  mask = which of the two neighbours exist,
       kindMask = which of them have the 0xC word == 0.
     - the walk is `do { ... } while (again)` with `again = 1` before it, and the
       "give up and resolve against the terrain" block (ret = WorldMapResolve-
       StepAgainstTerrain(...); D_8009C16C = -1; D_8009C840 = -1; again = 0;) is
       reached from the -1 arms of cases 1/2/4 and the mask==0 arms of 3/5/6 and
       is ONE shared block in retail.  A `goto` behind a `continue` gives that
       shape; six duplicated copies would rely on -fcross-jumping to merge.
     - outer case body source order is 0, 1, 2, 4, 3, 5, 6 (retail's body
       address order).  Cases 2 and 3 of the outer switch share the walk, so
       the `mode` variable is 1 (no current set) or 3 (walk) and the cases 0/2
       are unreachable today but present (the 4-way case tree proves 4 labels).
*/
/*
s32 func_80095414(SVec* pBase, SVec* pDelta, SVec* pOut, s32 scale, s32 kind) {
    WorldMapStepMesh* pMesh;
    WorldMapStepTriangle* pTri;
    s32 mode;
    s32 ret;
    s32 again;
    s32 count;
    s32 found;
    s32 i;
    s32 mask;
    s32 kindMask;
    s32 state;
    s32 heightDelta;
    s16 setIndex;
    s16 next;
    s16 next2;
    u16 triIndex;

    WORLD_MAP_STEP_SCRATCH->probe.x = pBase->x + ((pDelta->x * scale) >> 12);
    WORLD_MAP_STEP_SCRATCH->probe.z = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition(&WORLD_MAP_STEP_SCRATCH->probe);
    WORLD_MAP_STEP_SCRATCH->probe.y =
        WorldMapGetTilePlaneHeight(WORLD_MAP_STEP_SCRATCH->probe.x, WORLD_MAP_STEP_SCRATCH->probe.z) - 0x4000;

    pOut->x = pBase->x + ((pDelta->x * scale) >> 12);
    pOut->z = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition(pOut);
    pOut->y = WorldMapGetTilePlaneHeight(pOut->x, pOut->z) - 0x4000;

    ret = 0;
    if (D_8009C840 == WORLD_MAP_STEP_NONE) {
        mode = WORLD_MAP_STEP_SEARCH;
    } else {
        mode = WORLD_MAP_STEP_WALK;
    }
    switch (mode) {
    case WORLD_MAP_STEP_TERRAIN:
        ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
        D_8009C16C = WORLD_MAP_STEP_NONE;
        D_8009C840 = WORLD_MAP_STEP_NONE;
        return ret;
    case WORLD_MAP_STEP_SEARCH:
        count = WorldMapFindSpriteSet((void*)0x1F800060, &setIndex);
        if (count == 0) {
            return WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
        }
        found = 0;
        for (i = 0; i < count; i += 2) {
            if (WorldMapCheckTriangleEdgeSide(&WORLD_MAP_STEP_SCRATCH->probe, 0x70, (u16)setIndex,
                                             (u16)D_8009D718[i]) == 0) {
                D_8009D718[i] = WORLD_MAP_STEP_NONE;
                found += 2;
            }
        }
        if (found == count) {
            return WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
        }
        ret = 0;
        for (i = 0; i < count; i += 2) {
            next = D_8009D718[i];
            if (next == WORLD_MAP_STEP_NONE) {
                continue;
            }
            if (D_8009D718[i + 1] == 1) {
                continue;
            }
            WorldMapProjectTriangleNormal(&WORLD_MAP_STEP_SCRATCH->probe, &WORLD_MAP_STEP_SCRATCH->normalA,
                                          &WORLD_MAP_STEP_SCRATCH->normalB, (u16)setIndex, (u16)next);
            heightDelta = WORLD_MAP_STEP_SCRATCH->normalA.y - (pBase->y >> 12);
            if (heightDelta < 0) {
                heightDelta = -heightDelta;
            }
            if (heightDelta < WORLD_MAP_STEP_HEIGHT_TOLERANCE) {
                pOut->y = WORLD_MAP_STEP_SCRATCH->normalA.y << 12;
                D_8009C840 = setIndex;
                D_8009C16C = next;
                ret = 1;
                break;
            }
        }
        if (ret != 0) {
            return ret;
        }
        WorldMapAlignPerpendicularToReference(&WORLD_MAP_STEP_SCRATCH->normalB, pDelta, pOut);
        return ret;
    case WORLD_MAP_STEP_WALK_EDGE:
    case WORLD_MAP_STEP_WALK:
        setIndex = D_8009C840;
        triIndex = D_8009C16C;
        pMesh = g_WorldMapGroups[setIndex].pMesh;
        pTri = pMesh->tris;
        again = 1;
        do {
            state = func_80085760(pBase, &WORLD_MAP_STEP_SCRATCH->probe, setIndex, (s16)triIndex);
            switch (state) {
            case WORLD_MAP_CLIP_INSIDE:
                ret = 1;
                again = 0;
                WorldMapProjectTriangleNormal(&WORLD_MAP_STEP_SCRATCH->probe, &WORLD_MAP_STEP_SCRATCH->normalA,
                                              &WORLD_MAP_STEP_SCRATCH->normalB, (u16)setIndex, triIndex);
                D_8009C16C = (s16)triIndex;
                pOut->y = WORLD_MAP_STEP_SCRATCH->normalA.y << 12;
                D_8009C840 = setIndex;
                break;
            case 1:
                next = pTri[(s16)triIndex].next[0];
                if (next == WORLD_MAP_STEP_NONE) goto resolve;
                if (pTri[next].kind != 1) {
                    break;
                }
                triIndex = next;
                ret = 0;
                again = 0;
                WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[0]);
                break;
            case 2:
                next = pTri[(s16)triIndex].next[1];
                if (next == WORLD_MAP_STEP_NONE) goto resolve;
                if (pTri[next].kind != 1) {
                    break;
                }
                triIndex = next;
                ret = 0;
                again = 0;
                WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[1]);
                break;
            case 4:
                next = pTri[(s16)triIndex].next[2];
                if (next == WORLD_MAP_STEP_NONE) goto resolve;
                triIndex = next;
                if (pTri[next].kind != 1) {
                    break;
                }
                ret = 0;
                again = 0;
                WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[2]);
                break;
            case WORLD_MAP_CLIP_EDGE01:
                next = pTri[(s16)triIndex].next[0];
                next2 = pTri[(s16)triIndex].next[1];
                mask = (next != WORLD_MAP_STEP_NONE);
                if (next2 != WORLD_MAP_STEP_NONE) {
                    mask |= 2;
                }
                switch (mask) {
                case 2:
                    if (pTri[next2].kind != 1) {
                        break;
                    }
                    triIndex = next2;
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[1]);
                    break;
                case 1:
                    if (pTri[next].kind != 1) {
                        break;
                    }
                    triIndex = next;
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[0]);
                    break;
                case 3:
                    kindMask = (pTri[next].kind == 0);
                    if (pTri[next2].kind == 0) {
                        kindMask |= 2;
                    }
                    switch (kindMask) {
                    case 0:
                        pOut->z = 0;
                        pOut->y = 0;
                        pOut->x = 0;
                        ret = 0;
                        again = 0;
                        break;
                    case 1:
                        triIndex = pTri[(s16)triIndex].next[0];
                        break;
                    case 2:
                        triIndex = next2;
                        break;
                    case 3:
                        triIndex = pTri[(s16)triIndex].next[1];
                        break;
                    }
                    break;
                default:
                    goto resolve;
                }
                break;
            case WORLD_MAP_CLIP_EDGE02:
                next = pTri[(s16)triIndex].next[0];
                next2 = pTri[(s16)triIndex].next[2];
                mask = (next != WORLD_MAP_STEP_NONE);
                if (next2 != WORLD_MAP_STEP_NONE) {
                    mask |= 2;
                }
                switch (mask) {
                case 2:
                    triIndex = next2;
                    if (pTri[next2].kind != 1) {
                        break;
                    }
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[2]);
                    break;
                case 1:
                    if (pTri[next].kind != 1) {
                        break;
                    }
                    triIndex = next;
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[0]);
                    break;
                case 3:
                    kindMask = (pTri[next].kind == 0);
                    if (pTri[next2].kind == 0) {
                        kindMask |= 2;
                    }
                    switch (kindMask) {
                    case 0:
                        pOut->z = 0;
                        pOut->y = 0;
                        pOut->x = 0;
                        ret = 0;
                        again = 0;
                        break;
                    case 1:
                        triIndex = pTri[(s16)triIndex].next[0];
                        break;
                    case 2:
                        triIndex = next2;
                        break;
                    case 3:
                        triIndex = pTri[(s16)triIndex].next[1];
                        break;
                    }
                    break;
                default:
                    goto resolve;
                }
                break;
            case WORLD_MAP_CLIP_EDGE12:
                next = pTri[(s16)triIndex].next[1];
                next2 = pTri[(s16)triIndex].next[2];
                mask = (next != WORLD_MAP_STEP_NONE);
                if (next2 != WORLD_MAP_STEP_NONE) {
                    mask |= 2;
                }
                switch (mask) {
                case 2:
                    triIndex = next2;
                    if (pTri[next2].kind != 1) {
                        break;
                    }
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[2]);
                    break;
                case 1:
                    if (pTri[next].kind != 1) {
                        break;
                    }
                    triIndex = next;
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &WORLD_MAP_STEP_SCRATCH->dir[1]);
                    break;
                case 3:
                    kindMask = (pTri[next].kind == 0);
                    if (pTri[next2].kind == 0) {
                        kindMask |= 2;
                    }
                    switch (kindMask) {
                    case 0:
                        pOut->z = 0;
                        pOut->y = 0;
                        pOut->x = 0;
                        ret = 0;
                        again = 0;
                        break;
                    case 1:
                        triIndex = pTri[(s16)triIndex].next[1];
                        break;
                    case 2:
                        triIndex = next2;
                        break;
                    case 3:
                        triIndex = pTri[(s16)triIndex].next[1];
                        break;
                    }
                    break;
                default:
                    goto resolve;
                }
                break;
            }
            continue;
        resolve:
            ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
            D_8009C16C = WORLD_MAP_STEP_NONE;
            D_8009C840 = WORLD_MAP_STEP_NONE;
            again = 0;
        } while (again);
        break;
    }
    return ret;
}
*/

extern void WorldMapProjectTriangleNormal(SVec* pPos, SVec* pOut, SVec* pNormal, u16 setIndex,
                                          u16 triIndex);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095CD4);
/* best: 87.03% (688 bytes vs 704). The whole body matches except the probe
   scratchpad at 0x1F800060/64/68: retail re-materialises each constant address
   (`lui $at,0x1F80` + `sw ...0x60($at)`, `lui $v0,0x1F80` + `lw ...0x64($v0)`),
   while gcc folds every 2+-use literal MEM address into a callee-saved register
   (`li $17,0x1F800000; ori $17,$17,0x60` then `sw $2,0($17)`), which also takes
   the s-registers retail gives to i/found/the sprite table. Tried: single
   `*(s32*)0x1F8000xx` literals, a typed SVec* probe local, `((SVec*)0x1F800060)`
   member accesses (75%), `volatile` on the scratch accesses, and the symbolic
   `D_1F800060.x` spelling (that one emits %hi/%lo relocations retail does not
   have). A one-store/one-load probe test still makes a const register, so the
   trigger is two MEM uses of the same literal address, not the argument uses.
   Also open: the 5th argument arrives as a word (`lw $t0,0x60($sp)`,
   `sw $t0,0x10($sp)`) but WorldMapResolveStepAgainstTerrain takes an s16, so
   ours sign-extends into $v0 first; retail never narrows.

s32 func_80095CD4(SVec* pBase, SVec* pDelta, SVec* pOut, s32 scale, s32 kind) {
    s32 height;
    s32 count;
    s32 i;
    s32 found;
    s32 ret;
    u16 spriteIndex;

    *(s32*)0x1F800060 = pBase->x + ((pDelta->x * scale) >> 12);
    *(s32*)0x1F800064 = pBase->y + ((pDelta->y * scale) >> 12);
    *(s32*)0x1F800068 = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition((void*)0x1F800060);

    pOut->x = pBase->x + ((pDelta->x * scale) >> 12);
    pOut->y = pBase->y + ((pDelta->y * scale) >> 12);
    pOut->z = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition(pOut);

    height = WorldMapGetTilePlaneHeight(*(s32*)0x1F800060, *(s32*)0x1F800068) - 0x20000;
    if (height > 0x20000) {
        height = 0x20000;
    }
    if (height < *(s32*)0x1F800064 || height < -0x280000) {
        *(s32*)0x1F800064 = height;
        pOut->y = height;
    }
    if (height < -0x280000 && *(s32*)0x1F800064 < -0x280000) {
        *(s32*)0x1F800064 = -0x280000;
        pOut->y = -0x280000;
    }

    count = WorldMapFindSpriteSet((void*)0x1F800060, (s16*)&spriteIndex);
    if (count == 0) {
        ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
    } else {
        found = 0;
        for (i = 0; i < count; i += 2) {
            if (WorldMapCheckTriangleEdgeSide(&D_1F800060, 0x70, spriteIndex, D_8009D718[i]) == 0) {
                D_8009D718[i] = -1;
                found += 2;
            }
        }
        if (found == count) {
            ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
        } else {
            pOut->x = -pDelta->x >> 1;
            pOut->y = -pDelta->y >> 1;
            pOut->z = -pDelta->z >> 1;
            ret = 0;
        }
    }
    return ret;
}
*/
extern void* D_8009D7C4;
extern void* D_8009C660;
extern u8 D_8009C58F;
extern s32 g_WorldMapCdReadState;

void WorldMapInitLoadQueues(void) {
    u32 first = ArchiveGetDebugTable();
    u32 second = ArchiveGetDebugTable();
    void** pSlot;
    u8* pName;
    s32 i;

    if ((first == 0) | (second == 0xFFFFFFFF)) {
        pSlot = &D_8009D7C4;
        g_WorldMapLoadQueueHead = 0;
        g_WorldMapLoadQueueTail = 0;
        g_WorldMapCdReadState = 0;
        for (i = 15; i >= 0; i--) {
            *pSlot-- = NULL;
        }
        g_WorldMapCdRequestRecords = HeapAlloc(0x4200, 0);
        g_WorldMapCdSectorRemainder = HeapAlloc(0x800, 0);
        i = 7;
        pName = &D_8009C58F;
        g_WorldMapLoadRecordCount = 0;
        do {
            *pName-- = 0;
        } while (--i >= 0);
    } else {
        pSlot = &D_8009C660;
        g_WorldMapLoadQueueHead = 0;
        g_WorldMapLoadQueueTail = 0;
        g_WorldMapCdReadState = 0;
        for (i = 15; i >= 0; i--) {
            *pSlot-- = NULL;
        }
        g_WorldMapFileLoadRecords = HeapAlloc(0x5800, 0);
        g_WorldMapCdSectorRemainder = HeapAlloc(0x800, 0);
        i = 7;
        pName = &D_8009C58F;
        g_WorldMapLoadRecordCount = 0;
        do {
            *pName-- = 0;
        } while (--i >= 0);
    }
}

void WorldMapFreeLoadQueues(void) {
    u32 first = ArchiveGetDebugTable();
    u32 second = ArchiveGetDebugTable();
    void* pDebugTable;

    if ((first == 0) | (second == 0xFFFFFFFF)) {
        pDebugTable = g_WorldMapCdRequestRecords;
    } else {
        pDebugTable = g_WorldMapFileLoadRecords;
    }
    HeapFree(pDebugTable);
    HeapFree(g_WorldMapCdSectorRemainder);
}

void WorldMapWaitForLoadSlotFree(void) {
    u32 first = ArchiveGetDebugTable();
    u32 second = ArchiveGetDebugTable();

    if ((first == 0) | (second == 0xFFFFFFFF)) {
        while (g_WorldMapCdRequestQueue[g_WorldMapLoadQueueTail] != NULL) {
            Vsync(0);
            WorldMapUpdateLoadQueue();
        }
    } else {
        while (g_WorldMapFileLoadRequestQueue[g_WorldMapLoadQueueTail] != NULL) {
            Vsync(0);
            WorldMapUpdateLoadQueue();
        }
    }
}

s32 WorldMapAddCdRequest(s32 x, s32 y, s32 z) {
    SVec* p;
    s32 index;

    if (g_WorldMapLoadRecordCount < 0x58) {
        index = g_WorldMapLoadRecordCount;
        g_WorldMapLoadRecordCount = index + 1;
        p = &g_WorldMapCdRequestRecords[g_WorldMapLoadQueueTail * 0x58];
        p += index;
        p->x = x;
        p->y = y;
        p->z = z;
        return 0;
    }
    return -1;
}

s32 WorldMapAddFileLoadRequest(s32 x, s32 y, s32 z, s32 w) {
    SVec4* p;
    s32 index;

    if (g_WorldMapLoadRecordCount < 0x58) {
        index = g_WorldMapLoadRecordCount;
        g_WorldMapLoadRecordCount = index + 1;
        p = &g_WorldMapFileLoadRecords[g_WorldMapLoadQueueTail * 0x58];
        p += index;
        p->x = x;
        p->y = y;
        p->z = z;
        p->w = w;
        return 0;
    }
    return -1;
}

s32 WorldMapSubmitCdRequests(void) {
    s32 index = g_WorldMapLoadQueueTail;
    SVec* p = &g_WorldMapCdRequestRecords[index * 0x58];

    if (p->x != 0 && g_WorldMapCdRequestQueue[index] == NULL) {
        WorldMapSortCdRequestsBySector(p);
        g_WorldMapLoadRecordCount = 0;
        g_WorldMapCdRequestQueue[g_WorldMapLoadQueueTail] = p;
        g_WorldMapLoadQueueTail = (g_WorldMapLoadQueueTail + 1) & 0xF;
        return 0;
    }
    g_WorldMapLoadRecordCount = 0;
    return -1;
}

void WorldMapSortCdRequestsBySector(SVec* pList) {
    SVec* pCur = pList;
    s32 tmp[3];

    while (pCur[1].x != 0) {
        if (pCur[0].x > (u32)pCur[1].x) {
            tmp[0] = pCur[0].x;
            tmp[1] = pCur[0].y;
            tmp[2] = pCur[0].z;
            pCur[0].x = pCur[1].x;
            pCur[0].y = pCur[1].y;
            pCur[0].z = pCur[1].z;
            pCur[1].x = tmp[0];
            pCur[1].y = tmp[1];
            pCur[1].z = tmp[2];
            if (pList < pCur) {
                pCur--;
            }
        } else {
            pCur++;
        }
    }
}
void WorldMapSortFileLoadsByOffset(SVec4* pList) {
    SVec4* pCur = pList;
    u32 tmp[4];

    while (pCur[1].x != 0) {
        if ((u32)pCur[0].y > (u32)pCur[1].y) {
            tmp[0] = pCur[0].x;
            tmp[1] = pCur[0].y;
            tmp[2] = pCur[0].z;
            tmp[3] = pCur[0].w;
            pCur[0].x = pCur[1].x;
            pCur[0].y = pCur[1].y;
            pCur[0].z = pCur[1].z;
            pCur[0].w = pCur[1].w;
            pCur[1].x = tmp[0];
            pCur[1].y = tmp[1];
            pCur[1].z = tmp[2];
            pCur[1].w = tmp[3];
            if (pList < pCur) {
                pCur--;
            }
        } else {
            pCur++;
        }
    }
}
s32 WorldMapSubmitFileLoads(void) {
    s32 index = g_WorldMapLoadQueueTail;
    SVec4* p = &g_WorldMapFileLoadRecords[index * 0x58];

    if (p->x != 0 && g_WorldMapFileLoadRequestQueue[index] == NULL) {
        WorldMapSortFileLoadsByOffset(p);
        g_WorldMapLoadRecordCount = 0;
        g_WorldMapFileLoadRequestQueue[g_WorldMapLoadQueueTail] = p;
        g_WorldMapLoadQueueTail = (g_WorldMapLoadQueueTail + 1) & 0xF;
        return 0;
    }
    g_WorldMapLoadRecordCount = 0;
    return -1;
}

s32 WorldMapGetQueuedLoadCount(void) {
    s32 diff = g_WorldMapLoadQueueTail - g_WorldMapLoadQueueHead;

    if (diff < 0) {
        diff += 0x10;
    }
    return diff;
}

void WorldMapWaitForQueuedLoads(void) {
    do {
        Vsync(0);
        WorldMapUpdateLoadQueue();
    } while (WorldMapGetQueuedLoadCount() != 0);
}


typedef struct {
    /* 0x0 */ char* pName;
    /* 0x4 */ s32 offset;
    /* 0x8 */ s32 size;
    /* 0xC */ char* pBuffer;
} WorldMapLoadEntry; // Size: 0x10

extern int PCopen(char* name, int flags, int perms);
extern int PClseek(int fd, int offset, int mode);
extern int PCread(int fd, char* buff, int len);
extern int PCclose(int fd);
extern s32 D_8009BE48;
extern s32 g_WorldMapCdRetryCount;
extern s32 g_WorldMapCdReadErrorCount;
extern s32 D_8009CCB0;

void WorldMapReadFileEntries(WorldMapLoadEntry* pEntry) {
    s32 retry;
    s32 fd;

    D_8009BE48 = 0;
    D_8009CCB0 = 0;
    g_WorldMapCdReadErrorCount = 0;
    g_WorldMapCdRetryCount = 0;
    for (; pEntry->pName != NULL; pEntry++) {
        retry = 0;
        do {
            fd = PCopen(pEntry->pName, 0, 0);
            if (fd != -1) {
                break;
            }
        } while (++retry < 8);
        if (fd == -1) {
            continue;
        }
        PClseek(fd, pEntry->offset, 0);
        retry = 0;
        do {
            if (PCread(fd, pEntry->pBuffer, pEntry->size) != 0) {
                break;
            }
        } while (++retry < 8);
        retry = 0;
        do {
            if (PCclose(fd) == 0) {
                break;
            }
        } while (++retry < 8);
    }
}
