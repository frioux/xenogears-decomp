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
    /* 0x00 */ SVec4 v;      // the edge point the probe settled on
    /* 0x10 */ SVec4 target; // where the step aims (x and z only)
    /* 0x20 */ SVec4 near;   // cell-local offset of the position
    /* 0x30 */ SVec4 far;    // cell-local offset of the target
    /* 0x40 */ s32 clip[3];  // (z << 16) | x per corner, as NormalClip takes them
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ s32 unk50[0x14];
    /* 0xA0 */ SVECTOR cell[2]; // cell index of the position, then of the target
} WorldMapProbe; // Size: 0xAC

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
    edge = (pProbe->target.x & 0xFFF80000) - pPos->x;
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
    edge = (pProbe->target.z & 0xFFF80000) - pPos->z;
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

// One bit per axis saying which way the target position lies from the position
// the step is resolved at; the two-axis combinations are the cell corners.
#define WORLD_MAP_EDGE_X_POS 0x1
#define WORLD_MAP_EDGE_X_NEG 0x2
#define WORLD_MAP_EDGE_Z_POS 0x4
#define WORLD_MAP_EDGE_Z_NEG 0x8

// A cell is WORLD_MAP_CELL_SIZE sub-units square: a position is rounded down to
// its cell with WORLD_MAP_CELL_MASK and shifted right by WORLD_MAP_CELL_SHIFT
// for the cell index.
#define WORLD_MAP_CELL_SIZE 0x80000
#define WORLD_MAP_CELL_MASK 0xFFF80000
#define WORLD_MAP_CELL_SHIFT 19

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094A5C);
/* best: 82.51% (mine 1880 bytes vs retail's 1896, so 4 instructions short).
   The body is mapped and compiles: the prologue projects the target position in,
   classifies it into the 8-way edge mask below (the two cells are the SVECTOR
   pair at 0x1F8000A0: the position's cell index, then the target's), and the
   switch dispatches to the four straight probes and the four corner arms (a
   corner arm packs (near.z << 16) | near.x and (far.z << 16) | far.x, runs
   NormalClip, then tries its two probes in the order the sign picks). What is
   left is allocation, not structure: every arm and the tail has retail's
   instructions in retail's order except
     - retail keeps the mask in $t0 and the state/return in $a2; mine picks $v1
       for the state, which costs one `move v0,v1` per corner fallback probe;
     - retail's six prologue stores (0x10, 0x18 and the four cell halves) each
       build the address with `lui $at`, mine folds the two compare loads
       through the base (retail lhu 0xA4/0xAC(s1) against my t0-relative);
     - retail's tail forms the probe position as the SYMBOL D_1F800010
       (addiu $s0,$s1,%lo(D_1F800010), one insn reusing s1's high half).
       Spelling it &pProbe->target folds to addiu s0,s1,16 (82.51%);
       declaring extern SVec4 D_1F800010 gives a fresh lui+addiu and 82.24%.
       Neither is retail's, so the folded one is kept.
   The WorldMapProbe clip/cell fields and the WORLD_MAP_EDGE and WORLD_MAP_CELL
   defines above belong to this body and stay for whoever resumes it.
*/
/*
s32 func_80094A5C(SVec* pPos, SVec* pDir, s32 scale, s16 kind) {
    WorldMapProbe* pProbe = (WorldMapProbe*)0x1F800000;
    s32 xFrom;
    s32 zFrom;
    s32 xTo;
    s32 zTo;
    s32 edges;
    s32 state;
    s32 nClip;

    pProbe->target.x = pPos->x + ((pDir->x * scale) >> 12);
    pProbe->target.z = pPos->z + ((pDir->z * scale) >> 12);
    xFrom = pPos->x >> WORLD_MAP_CELL_SHIFT;
    zFrom = pPos->z >> WORLD_MAP_CELL_SHIFT;
    xTo = pProbe->target.x >> WORLD_MAP_CELL_SHIFT;
    zTo = pProbe->target.z >> WORLD_MAP_CELL_SHIFT;
    pProbe->cell[0].vx = xFrom;
    pProbe->cell[0].vz = zFrom;
    pProbe->cell[1].vx = xTo;
    pProbe->cell[1].vz = zTo;
    edges = 0;
    if (xTo < xFrom) {
        edges = WORLD_MAP_EDGE_X_NEG;
    } else if (xFrom < xTo) {
        edges = WORLD_MAP_EDGE_X_POS;
    }
    if (pProbe->cell[1].vz < pProbe->cell[0].vz) {
        edges |= WORLD_MAP_EDGE_Z_NEG;
    } else if (pProbe->cell[0].vz < pProbe->cell[1].vz) {
        edges |= WORLD_MAP_EDGE_Z_POS;
    }
    state = 0;
    switch (edges) {
    case WORLD_MAP_EDGE_X_POS:
        state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
        break;
    case WORLD_MAP_EDGE_X_NEG:
        state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
        break;
    case WORLD_MAP_EDGE_Z_POS:
        state = WorldMapProbeCellEdgeZFromReference(pPos, pDir, pProbe, kind);
        break;
    case WORLD_MAP_EDGE_Z_NEG:
        state = WorldMapProbeCellEdgeZFromPosition(pPos, pDir, pProbe, kind);
        break;
    case WORLD_MAP_EDGE_X_POS | WORLD_MAP_EDGE_Z_POS:
        pProbe->v.x = (pPos->x & WORLD_MAP_CELL_MASK) + WORLD_MAP_CELL_SIZE;
        pProbe->v.z = (pPos->z & WORLD_MAP_CELL_MASK) + WORLD_MAP_CELL_SIZE;
        pProbe->near.x = (pPos->x - pProbe->v.x) >> 12;
        pProbe->clip[1] = 0;
        pProbe->far.x = (pProbe->target.x - pProbe->v.x) >> 12;
        pProbe->far.z = (pProbe->target.z - pProbe->v.z) >> 12;
        pProbe->near.z = (pPos->z - pProbe->v.z) >> 12;
        pProbe->clip[0] = (pProbe->near.z << 16) | (pProbe->near.x & 0xFFFF);
        pProbe->clip[2] = (pProbe->far.z << 16) | (pProbe->far.x & 0xFFFF);
        nClip = NormalClip(pProbe->clip[0], pProbe->clip[1], pProbe->clip[2]);
        if (nClip < 0) {
            state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeZFromReference(pPos, pDir, pProbe, kind);
            }
        } else if (nClip > 0) {
            state = WorldMapProbeCellEdgeZFromReference(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
            }
        } else {
            state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
        }
        break;
    case WORLD_MAP_EDGE_X_NEG | WORLD_MAP_EDGE_Z_POS:
        pProbe->v.x = pPos->x & WORLD_MAP_CELL_MASK;
        pProbe->v.z = (pPos->z & WORLD_MAP_CELL_MASK) + WORLD_MAP_CELL_SIZE;
        pProbe->near.x = (pPos->x - pProbe->v.x) >> 12;
        pProbe->clip[1] = 0;
        pProbe->far.x = (pProbe->target.x - pProbe->v.x) >> 12;
        pProbe->far.z = (pProbe->target.z - pProbe->v.z) >> 12;
        pProbe->near.z = (pPos->z - pProbe->v.z) >> 12;
        pProbe->clip[0] = (pProbe->near.z << 16) | (pProbe->near.x & 0xFFFF);
        pProbe->clip[2] = (pProbe->far.z << 16) | (pProbe->far.x & 0xFFFF);
        nClip = NormalClip(pProbe->clip[0], pProbe->clip[1], pProbe->clip[2]);
        if (nClip > 0) {
            state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeZFromReference(pPos, pDir, pProbe, kind);
            }
        } else if (nClip < 0) {
            state = WorldMapProbeCellEdgeZFromReference(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
            }
        } else {
            state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
        }
        break;
    case WORLD_MAP_EDGE_X_POS | WORLD_MAP_EDGE_Z_NEG:
        pProbe->v.x = (pPos->x & WORLD_MAP_CELL_MASK) + WORLD_MAP_CELL_SIZE;
        pProbe->v.z = pPos->z & WORLD_MAP_CELL_MASK;
        pProbe->near.x = (pPos->x - pProbe->v.x) >> 12;
        pProbe->clip[1] = 0;
        pProbe->far.x = (pProbe->target.x - pProbe->v.x) >> 12;
        pProbe->far.z = (pProbe->target.z - pProbe->v.z) >> 12;
        pProbe->near.z = (pPos->z - pProbe->v.z) >> 12;
        pProbe->clip[0] = (pProbe->near.z << 16) | (pProbe->near.x & 0xFFFF);
        pProbe->clip[2] = (pProbe->far.z << 16) | (pProbe->far.x & 0xFFFF);
        nClip = NormalClip(pProbe->clip[0], pProbe->clip[1], pProbe->clip[2]);
        if (nClip > 0) {
            state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeZFromPosition(pPos, pDir, pProbe, kind);
            }
        } else if (nClip < 0) {
            state = WorldMapProbeCellEdgeZFromPosition(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
            }
        } else {
            state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
        }
        break;
    case WORLD_MAP_EDGE_X_NEG | WORLD_MAP_EDGE_Z_NEG:
        pProbe->v.x = pPos->x & WORLD_MAP_CELL_MASK;
        pProbe->v.z = pPos->z & WORLD_MAP_CELL_MASK;
        pProbe->near.x = (pPos->x - pProbe->v.x) >> 12;
        pProbe->clip[1] = 0;
        pProbe->far.x = (pProbe->target.x - pProbe->v.x) >> 12;
        pProbe->far.z = (pProbe->target.z - pProbe->v.z) >> 12;
        pProbe->near.z = (pPos->z - pProbe->v.z) >> 12;
        pProbe->clip[0] = (pProbe->near.z << 16) | (pProbe->near.x & 0xFFFF);
        pProbe->clip[2] = (pProbe->far.z << 16) | (pProbe->far.x & 0xFFFF);
        nClip = NormalClip(pProbe->clip[0], pProbe->clip[1], pProbe->clip[2]);
        if (nClip < 0) {
            state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeZFromPosition(pPos, pDir, pProbe, kind);
            }
        } else if (nClip > 0) {
            state = WorldMapProbeCellEdgeZFromPosition(pPos, pDir, pProbe, kind);
            if (state == 0) {
                state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
            }
        } else {
            state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
        }
        break;
    }
    if (state != 0) {
        return state;
    }
    WorldMapWrapPosition(&pProbe->target);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(&pProbe->target)) != 0) {
        return 0;
    }
    pProbe->v = pProbe->target;
    return 1;
}
*/
extern s32 WorldMapGetTileShapeAxisDirection(SVec* p0, SVec* p1, SVec* p2);
extern s32 func_80094A5C(SVec* pPos, SVec* pDir, s32 scale, s16 kind);

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
        result = WorldMapGetTileShapeAxisDirection((SVec*)0x1F800000, p1, p2);
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


// best: 64.84% (mine 2132 bytes / 533 insns against retail's 2240 / 576; 34 of the
// 43 missing instructions are inside the walk).
// PROGRESS (round 433): 60.05 -> 64.84 from two things.
//   (a) The scratchpad base is now a live local (WorldMapStepScratch* pStep,
//       assigned first) so the search and walk cases reach 0x30/0x40/0x50/0x60/
//       0x70/0x80 as base+offset exactly as retail's $fp does; the early probe and
//       pOut stores stay literal 0x1F800060/0x64/0x68 casts, which is also what
//       retail does (lui at,0x1F80 plus a bare store). This also bought retail's
//       spill set: pBase at 32(sp) and scale at 40(sp), both reloaded per use.
//   (b) TWO SEMANTIC BUGS in the old body, found by reading the .s: the
//       kindMask == 3 arm of EDGE01 and of EDGE02 must adopt next[0], not next[1].
//       Retail sends BOTH mask == 3 trees to .L80095A30, whose kindMask 1 and 3
//       arm is "lhu s0,0x6" (next[0]); only EDGE12's own tree
//       (.L80095C14/.L80095C6C) is next[1], because there a1 IS next[1].
// RESIDUAL 1, MEASURED AND NOT SOURCE-REACHABLE: this function cannot read
// objdiff 100.00 even with a perfect instruction stream. Retail's expected .s
// forms the probe address as "addiu a0,fp,%lo(g_WorldMapStepProbePos)" in five
// places (0x8009561C, 0x8009568C, 0x80095790, 0x80095C98, and the dir/normal
// pair at 0x80095690/0x80095694): spimdisasm named a base+0x60 immediate because
// $fp holds the matching %hi. Assembling that .s puts an R_MIPS_LO16
// g_WorldMapStepProbePos relocation on the instruction, and no C spelling gives
// base+offset AND a symbol relocation - "&g_WorldMapStepProbePos" gives the
// two-instruction lui+addiu la (one instruction longer, no base register) while
// "pStep->probe" gives addiu a0,s8,0x60 with no relocation. The linked bytes are
// identical either way, so this is the func_80095CD4 note's residual seen from the
// other side: the cap is about 99.9% and only with every instruction right.
// RESIDUAL 2, the largest: cc1 re-loads a triangle neighbour it already holds
// sign extended, once per arm, when it is stored into the u16 triIndex. Retail
// has "lh v1,0x6(v0)" and then "addu s0,a0,zero" (a copy of the same register);
// this body has "lh $3,0x6($2)" AND "lhu $8,0x6($2)", the same pair for next2 at
// each mask == 3 site, about nine extra instructions - while retail's walk is 34
// instructions longer overall because it shares one kindMask/next[0] tail between
// the EDGE01 and EDGE02 trees, which this body also does (0x1260) but with more
// per-arm dispatch.
// FRAME: 0xB0 against retail's 0xB8. The slots agree (0x10 outgoing arg, 0x18
// setIndex, 0x20 pBase, 0x28 scale) so retail has one more 8-byte phantom slot
// than this body reaches; the saved-register set is retail's.
// Facts carried over, still true:
//   - D_8009C840 is the sprite set the step is resolved in (-1 = none) and
//     D_8009C16C its current triangle; both are s32 globals (lw/sw) holding
//     16-bit values, read lhu for the triangle index and sign extended by the
//     stores. WORLD_MAP_STEP_NONE names the -1 of both.
//   - ONE s16 local is both the address-taken output of WorldMapFindSpriteSet and
//     the spilled set index (slot 0x18): lh where it is used plainly and lhu where
//     cast to u16, which is how a single variable explains both loads.
//   - mesh triangle stride 0xE: vert[3] at 0x0, the three edge neighbours at
//     0x6/0x8/0xA (-1 = none), a u16 at 0xC compared against 0 and 1.
//     Neighbour[0] pairs with 0x1F800030, [1] with 0x40, [2] with 0x50.
//   - func_80085760's clip code: 0 inside, 1/2/4 past an edge, 3/5/6 past a
//     corner, 7 or more out of range. mask = which of the two neighbours exist,
//     kindMask = which of them have the 0xC word == 0. The outer switch's table
//     is EIGHT words wide (retail "sltiu state,8"), so state 7 is a real case
//     label that falls straight through to the loop end; "case 7: break;" in the
//     body reproduces the table width.
//   - the walk is "do { ... } while (again)" with again = 1 before it, and the
//     "give up and resolve against the terrain" block (ret =
//     WorldMapResolveStepAgainstTerrain(...); D_8009C16C = -1; D_8009C840 = -1;
//     again = 0;) is reached from the -1 arms of cases 1/2/4 and the mask == 0
//     arms of 3/5/6 and is ONE shared block in retail, which a goto behind a
//     continue reproduces.
//   - outer case body source order is 0, 1, 2, 4, 3, 5, 6 (retail's body address
//     order: .L800958C4, the state-4 arm, sits below .L80095908, the state-3
//     arm). Cases 2 and 3 of the outer switch share the walk.
// best: 71.12% (2152 bytes / 538 lines vs retail's 2240 / 576).
// Round 446: 64.84 -> 71.12 from ONE semantic fix plus the structure it implies.
//   The SEARCH case's two "give up and go to the terrain" exits are ONE block in
//   retail (.L8009570C, reached from both 'beqz s4' at 0x25B14 and 'beq s2,s4' at
//   0x25B60) and that block does more than call: it also stores -1 into BOTH
//   D_8009C16C and D_8009C840 before returning. The old body wrote 'return
//   WorldMapResolveStepAgainstTerrain(...)' at each of the two exits, i.e. it was
//   missing those two stores. Spelling the case as 'found=0; if (count != 0) {check
//   loop} if (count != found) { walk loop; returns } then ret = Resolve(...);
//   D_8009C16C = -1; D_8009C840 = -1; return ret;' gives gcc ONE shared fallback
//   block, retail's shape, and the search half of the function then matches
//   instruction for instruction.
// WHAT IS LEFT (measured by aligning both instruction streams on the call sites):
//   1. THE ONE BIG RESIDUAL IS A REGISTER MIRROR: retail keeps the walk's -1 in $s7
//      and pDelta in $s6, this body keeps -1 in $s6 and pDelta in $s7; downstream
//      that turns retail's '$s3 = pOut, $s4 = again' into '$s4 = pOut'. The prologue,
//      the whole search case and the walk's per-arm shapes are otherwise identical
//      instruction for instruction (the first 37 insns differ only in that swap), so
//      -dg/-da on this body is the next step, not more spellings. The search case's
//      own allocation (s4=count, s2=found, s1=i, s5=-1, s0=&D_8009D718) is retail's
//      EXACTLY, so the mirror starts in the walk.
//   2. FRAME: retail 0xB8, this body 0xB0. All four slots agree (0x10 outgoing arg,
//      0x18 setIndex, 0x20 pBase, 0x28 scale) and the saved set is retail's 10, so
//      retail really does have one more 8-byte anonymous slot; nothing in the body
//      reaches 0x30..0x8C.
//   3. The three WorldMapResolveStepAgainstTerrain call sites narrow the 5th argument
//      (sll/sra on kind, from the visible 's16 kind' prototype in this TU) where
//      retail copies the raw word ('lw t1,0xC8(sp); sw t1,0x10(sp)'). The
//      func_80095CD4 K&R trick cannot be applied here: the callee is DEFINED earlier
//      in this file, so its prototype is in scope.
//   4. Retail's walk shares one AlignDirectionToReference CALL per scratch vector
//      (fp+0x30/0x40/0x50) and this body does the same, but retail's EDGE01/02
//      kindMask==1|3 arms end in 'lhu s0,0x6(...)' - a RELOAD of next[0] as u16, not
//      a copy of the s16 register - so the 'triIndex = pTri[(s16)triIndex].next[0]'
//      spelling in the body is right and must not be replaced by 'triIndex = next'.
// CARRIED OVER, still true: retail's expected .s forms the probe address as
// 'addiu a0,fp,%lo(g_WorldMapStepProbePos)' (base+offset plus a symbol reloc) in five
// places, which no C spelling reproduces ('&g_WorldMapStepProbePos' gives the
// two-instruction la, 'pStep->probe' gives addiu a0,s8,0x60 with no reloc); the linked
// bytes are identical, so a perfect body caps at about 99.9% in objdiff and is proven
// with check_bin.sh instead. See func_80095CD4's note for the same residual.
/*
s32 func_80095414(SVec* pBase, SVec* pDelta, SVec* pOut, s32 scale, s32 kind) {
    WorldMapStepMesh* pMesh;
    WorldMapStepTriangle* pTri;
    WorldMapStepScratch* pStep;
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
    s16 next;
    s16 next2;
    s16 setIndex;
    u16 triIndex;

    pStep = WORLD_MAP_STEP_SCRATCH;
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
        found = 0;
        if (count != 0) {
            for (i = 0; i < count; i += 2) {
                if (WorldMapCheckTriangleEdgeSide(&pStep->probe, 0x70, (u16)setIndex,
                                                 (u16)D_8009D718[i]) == 0) {
                    D_8009D718[i] = WORLD_MAP_STEP_NONE;
                    found += 2;
                }
            }
        }
        if (count != found) {
            ret = 0;
            for (i = 0; i < count; i += 2) {
                next = D_8009D718[i];
                if (next == WORLD_MAP_STEP_NONE) {
                    continue;
                }
                if (D_8009D718[i + 1] == 1) {
                    continue;
                }
                WorldMapProjectTriangleNormal(&pStep->probe, &pStep->normalA, &pStep->normalB, (u16)setIndex,
                                              (u16)next);
                heightDelta = pStep->normalA.y - (pBase->y >> 12);
                if (heightDelta < 0) {
                    heightDelta = -heightDelta;
                }
                if (heightDelta < WORLD_MAP_STEP_HEIGHT_TOLERANCE) {
                    pOut->y = pStep->normalA.y << 12;
                    D_8009C840 = setIndex;
                    D_8009C16C = next;
                    ret = 1;
                    break;
                }
            }
            if (ret != 0) {
                return ret;
            }
            WorldMapAlignPerpendicularToReference(&pStep->normalB, pDelta, pOut);
            return ret;
        }
        ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
        D_8009C16C = WORLD_MAP_STEP_NONE;
        D_8009C840 = WORLD_MAP_STEP_NONE;
        return ret;
    case WORLD_MAP_STEP_WALK_EDGE:
    case WORLD_MAP_STEP_WALK:
        setIndex = D_8009C840;
        triIndex = D_8009C16C;
        pMesh = g_WorldMapGroups[setIndex].pMesh;
        pTri = pMesh->tris;
        again = 1;
        do {
            state = func_80085760(pBase, &pStep->probe, setIndex, (s16)triIndex);
            switch (state) {
            case WORLD_MAP_CLIP_INSIDE:
                ret = 1;
                again = 0;
                WorldMapProjectTriangleNormal(&pStep->probe, &pStep->normalA, &pStep->normalB, (u16)setIndex,
                                              triIndex);
                D_8009C16C = (s16)triIndex;
                pOut->y = pStep->normalA.y << 12;
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
                WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[0]);
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
                WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[1]);
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
                WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[2]);
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
                    WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[1]);
                    break;
                case 1:
                    if (pTri[next].kind != 1) {
                        break;
                    }
                    triIndex = next;
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[0]);
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
                        triIndex = pTri[(s16)triIndex].next[0];
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
                    WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[2]);
                    break;
                case 1:
                    if (pTri[next].kind != 1) {
                        break;
                    }
                    triIndex = next;
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[0]);
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
                        triIndex = pTri[(s16)triIndex].next[0];
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
                    WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[2]);
                    break;
                case 1:
                    if (pTri[next].kind != 1) {
                        break;
                    }
                    triIndex = next;
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[1]);
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
            case 7:
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095414);



extern void WorldMapProjectTriangleNormal(SVec* pPos, SVec* pOut, SVec* pNormal, u16 setIndex,
                                          u16 triIndex);
extern SVec g_WorldMapStepProbePos; // candidate world position in the 0x1F800000 scratchpad
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095CD4);
/* best: 99.93% (676 bytes = retail's exact size, 169 instructions = retail's
   count). Two residuals, both measured:
   1. RELOC ONLY, links identical: the probe-X store is spelled through the
      symbol (g_WorldMapStepProbePos.x = ...) where retail shows the literal
      lui at,0x1F80 plus sw 0x60(at); the linker resolves the symbol to the same
      address. The all-literal spelling is not usable: with all three 0x1F800060
      uses literal, cse holds that address in one callee-saved register and the
      body loses instructions.
   2. THE ONE REAL DIFFERENCE: inside the loop retail forms the probe address as
      "lui t0,hi" then "addiu a0,t0,lo" (hi in a scratch, untied) while every
      spelling here gives the tied "lui a0,hi" plus "addiu a0,a0,lo" of the
      maspsx la expansion; only the two register fields differ. THIS IS THE
      COMPILER, NOT THE SOURCE (round 408): cc1_probe.sh on this unit shows both
      gcc-2.7.2-psx and gcc-2.6.0-psx emit one "la $4,sym" - which GNU as then
      ties - for every spelling of the address (bare symbol, symbol plus field
      offset, array extern, u8 pointer plus 0x60, local pointer, store then take)
      and only gcc-2.7.2-cdk-psx emits the split. cdk would not match this body
      either: 89.1% at 644 bytes (32 short), and 67.8% at 676 with
      -fno-schedule-insns/-insns2. Measured and dead in the source: a pointer
      local before the loop, a per-iteration assignment, an inline cast, an
      unbounded-array extern, and a 3-minute permuter run from this body (1424
      iterations, base 20, best 20). The unit cannot switch to cdk (ten matched
      functions drop to 75-97) so this is a MIXED unit: one function whose TU was
      built with the splitting cc1, and neither a preset switch nor a unit split
      buys it.
   How the 8-byte narrowing was removed, which is the find worth keeping: the
   caller's kind stays s32 and the CALLEE keeps its s16 signature, but the call
   is made through a K&R declaration, which makes the 5th argument a raw word
   copy (lw t0,0x60(sp) then sw t0,0x10(sp)) exactly as retail does. The
   prototyped declaration forces sll/sra on kind (684 bytes, 98.72%), and
   widening the callee to s32 does the same while breaking the callee's own lh
   load. Retail's two TUs simply disagreed about the prototype.
   The K&R declaration sits INSIDE this block, just above the body: park.py's
   unpark extracts only the definition, so re-add the extern line by hand after
   un-parking or the 8 bytes come back.
   ROUND 421 re-measurement (unparked, rebuilt in this tree): 99.93% / 676
   bytes / 169 insns, the note above confirmed, and the body still compiles with
   the K&R extern in place. Rejected this round, each measured:
   - the probe-X store spelled literal like the y/z stores: cc1 hoists the
     address into a callee-saved $s0 and keeps it across the calls, 97.50% / 676
     bytes / 12 differing lines, so the symbol spelling of that ONE store is
     load-bearing for the frame's register set.
   - gcc-2.7.2 at -G0: 99.93 (identical), at -O1: 71.22; -O3 and -Os are
     rejected by cc1 on this unit.
   - cc1_probe.sh with the store-then-take shape of tools/scripts/probe_example.c
     (ProbeStoreFirst, the one shape whose docs say cc1 splits %hi from %lo)
     prints one `la $4,g_Probe` on this unit's cc1, so that shape does not reach
     the split here either: a 2.7.2 cc1 forms a single `la` in every context.
   - a second permuter run from this body: 1273 iterations, base=20 best=20.
   Nothing left in the source. Even a perfect instruction stream would still read
   99.9x here, because the probe-X store carries a relocation against
   g_WorldMapStepProbePos where retail's store is a bare constant.
extern s32 WorldMapResolveStepAgainstTerrain();
s32 func_80095CD4(SVec* pBase, SVec* pDelta, SVec* pOut, s32 scale, s32 kind) {
    s32 height;
    s32 count;
    s32 i;
    s32 found;
    s32 ret;
    u16 spriteIndex;

    g_WorldMapStepProbePos.x = pBase->x + ((pDelta->x * scale) >> 12);
    ((SVec*)0x1F800060)->y = pBase->y + ((pDelta->y * scale) >> 12);
    ((SVec*)0x1F800060)->z = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition((SVec*)0x1F800060);

    pOut->x = pBase->x + ((pDelta->x * scale) >> 12);
    pOut->y = pBase->y + ((pDelta->y * scale) >> 12);
    pOut->z = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition(pOut);

    height = WorldMapGetTilePlaneHeight(((SVec*)0x1F800060)->x, ((SVec*)0x1F800060)->z) - 0x20000;
    if (height > 0x20000) {
        height = 0x20000;
    }
    if ((height < ((SVec*)0x1F800060)->y) | (height < -0x280000)) {
        ((SVec*)0x1F800060)->y = height;
        pOut->y = height;
    }
    if ((height > -0x280000) & (((SVec*)0x1F800060)->y < -0x280000)) {
        ((SVec*)0x1F800060)->y = -0x280000;
        pOut->y = -0x280000;
    }

    count = WorldMapFindSpriteSet((SVec*)0x1F800060, (s16*)&spriteIndex);
    if (count != 0) {
        found = 0;
        for (i = 0; i < count; i += 2) {
            if (WorldMapCheckTriangleEdgeSide((SVec*)&g_WorldMapStepProbePos, 0x70, spriteIndex, D_8009D718[i]) == 0) {
                D_8009D718[i] = -1;
                found += 2;
            }
        }
        if (found != count) {
            ret = 0;
            pOut->x = -pDelta->x >> 1;
            pOut->y = -pDelta->y >> 1;
            pOut->z = -pDelta->z >> 1;
            goto done;
        }
    }
    ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
done:
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
