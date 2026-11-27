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

// best: 79.24% (mine 1876 bytes / 469 insns vs retail's 1896). The score is 3.3 points below the note this park carried (82.51) and the body is the CLOSER one: retail's jump table at 0x80070C50 has 11 entries starting at mask 0 (0, 3 and 7 point at the tail .L80095124), so the switch needs an explicit `case 0` - without it cc1 biases the table by -1 (addiu + sltiu <10) and emits 10 words, i.e. 40 bytes against retail's 44, which puts misc14's second table jtbl_80070C80 (owned by the func_80095414 stub) at 0x1188 instead of 0x1190 and shifts the whole overlay. With case 0 the dispatch is retail's: sltiu <11, sll, 11-word table ending at 0x118C (the next table's .align 3 pads it to 0x1190). Case order is by mask value: 1, 2, 4, 5, 6, 8, 9, 10 - the Z_NEG arm (.L80094BD0) belongs AFTER the two Z-positive corners. What is left: (1) edges lands in $a2 and state in $v1; retail has edges in $t0 and state in $a2, so retail's state=0 is in the first compare's delay slot (addu a2,zero,zero) and mine sinks it into the jr's slot. Measured no change: declaration order (state before edges), edges=0 as a declaration initialiser instead of a statement, edges=0 placed before the cell stores. (2) mine is 15 instructions short, and about 11 of them are retail's leftover load-delay nops that this compiler FILLS (maspsx #nop slots), plus the four corner arms' clip computation re-scheduled (same ops, near.z/far.z order swapped). Presets measured on this body: gcc-2.7.2-cdk 77.21, gcc-2.6.0 81.86, -G0 82.51 (identical with the old 10-word table), -O3 does not compile. The WorldMapProbe clip/cell fields and the WORLD_MAP_EDGE / WORLD_MAP_CELL defines above belong to this body.
/*
s32 func_80094A5C(SVec* pPos, SVec* pDir, s32 scale, s16 kind) {
    WorldMapProbe* pProbe = (WorldMapProbe*)0x1F800000;
    s32 xFrom;
    s32 zFrom;
    s32 xTo;
    s32 zTo;
    s32 state;
    s32 edges = 0;
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
    case 0:
        break;
    case WORLD_MAP_EDGE_X_POS:
        state = WorldMapProbeCellEdgeXFromReference(pPos, pDir, pProbe, kind);
        break;
    case WORLD_MAP_EDGE_X_NEG:
        state = WorldMapProbeCellEdgeXFromPosition(pPos, pDir, pProbe, kind);
        break;
    case WORLD_MAP_EDGE_Z_POS:
        state = WorldMapProbeCellEdgeZFromReference(pPos, pDir, pProbe, kind);
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
    case WORLD_MAP_EDGE_Z_NEG:
        state = WorldMapProbeCellEdgeZFromPosition(pPos, pDir, pProbe, kind);
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094A5C);


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
extern s32 g_WorldMapStepSpriteSet;   // sprite set the step is being resolved in, -1 when none
extern s32 g_WorldMapStepTriangleIndex;   // current triangle of that set
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
//   - g_WorldMapStepSpriteSet is the sprite set the step is resolved in (-1 = none) and
//     g_WorldMapStepTriangleIndex its current triangle; both are s32 globals (lw/sw) holding
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
//     WorldMapResolveStepAgainstTerrain(...); g_WorldMapStepTriangleIndex = -1; g_WorldMapStepSpriteSet = -1;
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
//   g_WorldMapStepTriangleIndex and g_WorldMapStepSpriteSet before returning. The old body wrote 'return
//   WorldMapResolveStepAgainstTerrain(...)' at each of the two exits, i.e. it was
//   missing those two stores. Spelling the case as 'found=0; if (count != 0) {check
//   loop} if (count != found) { walk loop; returns } then ret = Resolve(...);
//   g_WorldMapStepTriangleIndex = -1; g_WorldMapStepSpriteSet = -1; return ret;' gives gcc ONE shared fallback
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
// ROUND 457: 71.12 -> 71.73% (2156 bytes / 539 insns vs retail 2240 / 576) from ONE source find.
//   THE pDelta REGISTER MIRROR IS FIXED IN THE SOURCE, not the allocator: the
//   search case's acceptance store must RELOAD the array -
//   g_WorldMapStepTriangleIndex = D_8009D718[i]; - and not reuse the 'next' local.
//   Retail reloads it after the WorldMapProjectTriangleNormal call ('lh v1,0x0(s0)'
//   at 0x25AE0) rather than trusting the value it passed; keeping 'next' live across
//   the call put it in a callee-saved register and that is what pushed pDelta down
//   to $s7 (retail keeps pDelta in $s6 and the walk's -1 in $s7). With the reload
//   the prologue is retail's exactly ('sw s6,0xA8(sp)' then 'addu s6,a1,zero').
//   See the ROUND 446 block above for how the search case's two give-up exits were
//   already unified.
// WHAT IS LEFT, all four measured as allocator TIES this round (no source response):
//   a. 0x1F800000 scratch base and the walk's -1 swap $s7/$s8 (retail base=$s8/fp,
//      -1=$s7; this body base=$s7, -1=$s8). Both are rematerialisable constants,
//      i.e. the two lowest-priority callee-saved slots, decided by allocno number.
//   b. pOut and {count,again} swap $s3/$s4 (retail pOut=$s3, count/again=$s4;
//      this body pOut=$s4, count/again=$s3).
//   c. 'state' (func_80085760's return) sits in $v1 here, retail keeps it in $s1
//      ('addu s1,v0,zero') and this body's $s1 holds pTri; retail's pTri is $s2.
//   d. FRAME 176 vs retail's 184: the four named slots agree and the saved set is
//      retail's ten, so retail reaches one more 8-byte anonymous slot than this body.
//   e. BLOCK ORDER, the biggest visible residual: retail emits the walk's shared
//      resolve block (.L80095B2C) BETWEEN the dir[0] tail (.L80095A14, shared by
//      state 1 and state 3 kindMask 1) and the dir[1] tail (.L80095B84); this body
//      emits all three AlignDirection calls first and the resolve block last, which
//      is why the mid-arm region reads 7 instructions MORE than retail while the
//      function totals 37 short. That order comes out of gcc's switch expansion plus
//      jump optimisation and cannot be steered from the source.
//   f. This body hoists the constant 1 into $s3 ('bne v0,s3' per arm) where retail
//      re-materialises 'li t1,1' in each arm; it is the register 'again' holds.
// MEASURED AND REJECTED this round: `again = 1;` moved to the top of the walk case
// (retail sets it in the 'j .L80095748' delay slot at 0x25590) - identical 71.73%,
// reverted. gcc-2.7.2-cdk 69.74% (try_flags), so the preset is right.
// ROUND 472: re-measured from this exact park, still 71.73% (2156 bytes / 539 insns
// against retail's 2240 / 576). Four measurements added, none moved the source:
//   i.  'resolve:' moved INSIDE the switch, after 'case 7:' (the cross-jump lever's
//       block-placement rule, levers.md i-a-cross-jumped-block-keeps): identical
//       71.73% / 2156 bytes, so the shared resolve block is not placed by the label's
//       source position here. Reverted.
//  ii.  The state 1/2/4 arms written as 'triIndex = pTri[(s16)triIndex].next[k]'
//       instead of 'triIndex = next': identical stream, because gcc already
//       re-materialises that load (one 'lhu t0' beside retail's single 'lh' + 'move'
//       per arm). The rematerialisation is not source-steerable; reverted.
// iii.  Both bodies carry TWO copies of the kindMask block (retail 0x25F40 shared by
//       states 3 and 5, and 0x260F0 for state 6; this body's two sit elsewhere), so
//       the 37-instruction deficit is NOT a missing copy there - hypothesis measured
//       and discarded.
//  iv.  PRESET re-screened: gcc-2.6.0-psx 70.24%, gcc-2.7.2-cdk-psx 69.74%, unit flags
//       71.73. The preset is right.
// Residual (f) restated with the cause: cse gives the walk's constant 1 a com register
// ($s3 here, serving both 'bne v0,s3' per arm and 'again'), and that is the slot retail
// gives pOut ($s3 pOut / $s4 again there). Every spelling of the arms' 'kind != 1' and
// of 'again = 1' feeds the same const_int, so the merge cannot be broken from source;
// retail's own 'li s4,1' shows again's register there does not serve the comparisons.
// The probe-address reloc cap from the CARRIED OVER block above still applies: a
// perfect instruction stream would read about 99.9% in objdiff and needs check_bin.
// The callee was defined in a TU that took its kind as a raw word: an unprototyped
// declaration makes each call copy the s32 rather than narrow it to the s16.
extern s32 WorldMapResolveStepAgainstTerrain();

// best: 72.47% (2124 bytes / 531 insns against retail's 2240 / 560 text insns).
// The same body WITHOUT the K&R declaration below measures 72.73% (2148 bytes) - see
// ROUND 513 (b); the K&R version is the one parked because it matches retail's three
// call sites instruction for instruction and has 16 fewer differing lines (577
// against 593). Retail is 116 bytes (29 insns) bigger, all of it block duplication
// and the shared tails, see WHAT IS LEFT.
// ROUND 513, two source finds (71.73 -> 72.73 -> 72.47):
//   (a) SEMANTIC, worth the +1.0: every walk arm that adopts a neighbour assigns it
//       BEFORE the kind test. Retail's copy is in the delay slot of the kind test
//       (`beq v1,s7` + `addu a0,v1,zero` at .L80095844, `bne v0,t1` + `addu s0,a0` at
//       .L80095868, the same pair at .L800958B4, .L8009597C, .L800959B4, .L800959DC,
//       .L80095B7C), and reorg only fills a delay slot from the branch's FALL-THROUGH
//       block, so the source read `triIndex = next; if (kind != 1) break;` - the
//       neighbour IS adopted when the kind test fails. The old body tested kind first
//       and assigned after, which also made gcc rematerialise the u16 load (`lhu
//       $8,6($2)` beside retail's single `lh` + copy). Fixed in all five arms; the
//       two `lhu` copies and 2 instructions went away and the score rose 1.0.
//   (b) THE 5th ARGUMENT IS A RAW WORD, and an unprototyped declaration after the
//       definition gets it: `extern s32 WorldMapResolveStepAgainstTerrain();` above
//       this body makes each of the three calls `lw t1,<kind>(sp)` + `sw
//       t1,0x10(sp)` exactly as retail (.L800955A0, .L80095718, .L80095B34), removing
//       the sll/sra pair the visible `s16 kind` prototype forced (6 instructions).
//       The func_80095CD4 note's K&R trick, applied from the other side. Cost 0.26
//       of objdiff fuzzy (72.73 -> 72.47) and kept anyway: 6 fewer instructions and
//       retail's own argument shape at all three sites is the truer stream; objdiff's
//       fuzzy number is not monotone (see the levers).
// WHAT IS LEFT (all measured, no source response found in rounds 457/472/513):
//   1. BLOCK ORDER, the largest residual: retail emits the walk's shared resolve
//      block (.L80095B2C) BETWEEN the dir[0] tail (.L80095A14, shared by state 1 and
//      the mask==1 arms) and the dir[1] tail (.L80095B84); this body emits the three
//      AlignDirection tails first and the resolve block last. Retail's tail order in
//      memory: mask==1 body .L800959F0, dir[0] .L80095A14, kindMask tree .L80095A30,
//      kindMask dispatch .L80095A64, kindMask 1|3 .L80095A8C, mask==0 .L80095AFC,
//      resolve .L80095B2C, EDGE12 mask==1 .L80095B60, dir[1] .L80095B84, mask==2
//      .L80095BA0, dir[2] .L80095BB8. Comes out of gcc's switch expansion plus jump
//      optimisation; the `resolve:` label position was measured inert twice.
//   2. THE CONSTANT 1 GETS A CALLEE-SAVED REGISTER here ($s3, serving the per-arm
//      `bne kind,1` and `again`) where retail re-materialises `li t1,1` per arm and
//      keeps pOut in $s3. That single difference is also residual (c) below - one
//      register short, which is what pushes pOut to $s4 and again to $s2 (retail:
//      pOut $s3, again $s4, pTri $s2, scratch base $fp, walk -1 $s7). Every spelling
//      of the arms' `kind != 1` and of `again = 1` feeds the same const_int, so the
//      merge cannot be broken from source.
//   3. FRAME 176 against retail's 184: the four named slots agree (0x10 outgoing arg,
//      0x18 setIndex, 0x20 pBase, 0x28 scale) and the saved set is retail's ten, so
//      retail reaches one more 8-byte anonymous slot than this body.
//   4. FIVE `addiu a0,fp,%lo(g_WorldMapStepProbePos)` sites (.L8009561C, .L8009568C,
//      .L80095790, .L80095C98 and the dir/normal pair .L80095690/.L80095694) form a
//      base+offset WITH a symbol relocation, which no C spelling gives ("&global" is
//      the two-instruction la, "pStep->probe" is addiu with no reloc). Linked bytes
//      are identical, so a perfect body caps near 99.9% in objdiff and is proven with
//      check_bin.sh instead (func_80095CD4's note is the same residual).
// MEASURED AND REJECTED this round: `triIndex` declared s16 instead of u16 (67.74,
// the u16 narrowing load at the walk preheader becomes lh); the inner mask switch's
// case labels reordered to 1,2,3 (identical 72.73, gcc's dispatch tree is not
// source-ordered); gcc-2.7.2-cdk 69.74 and gcc-2.6.0 70.24 (rounds 472), so the
// preset is right.
// CARRIED OVER, still true: mesh triangle stride 0xE (vert[3] 0x0, the three edge
// neighbours 0x6/0x8/0xA with -1 = none, a u16 kind at 0xC compared against 0 and 1);
// neighbour[0] pairs with 0x1F800030, [1] with 0x40, [2] with 0x50; the walk's outer
// switch table is EIGHT words wide so state 7 is a real case that falls through to
// the loop end; g_WorldMapStepSpriteSet is the sprite set the step is resolved in
// (-1 = none) and g_WorldMapStepTriangleIndex its current triangle, both s32 globals
// holding 16-bit values; the outer case body source order is 0, 1, 2, 4, 3, 5, 6.
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
    if (g_WorldMapStepSpriteSet == WORLD_MAP_STEP_NONE) {
        mode = WORLD_MAP_STEP_SEARCH;
    } else {
        mode = WORLD_MAP_STEP_WALK;
    }
    switch (mode) {
    case WORLD_MAP_STEP_TERRAIN:
        ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
        g_WorldMapStepTriangleIndex = WORLD_MAP_STEP_NONE;
        g_WorldMapStepSpriteSet = WORLD_MAP_STEP_NONE;
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
                    g_WorldMapStepSpriteSet = setIndex;
                    g_WorldMapStepTriangleIndex = D_8009D718[i];
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
        g_WorldMapStepTriangleIndex = WORLD_MAP_STEP_NONE;
        g_WorldMapStepSpriteSet = WORLD_MAP_STEP_NONE;
        return ret;
    case WORLD_MAP_STEP_WALK_EDGE:
    case WORLD_MAP_STEP_WALK:
        setIndex = g_WorldMapStepSpriteSet;
        triIndex = g_WorldMapStepTriangleIndex;
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
                g_WorldMapStepTriangleIndex = (s16)triIndex;
                pOut->y = pStep->normalA.y << 12;
                g_WorldMapStepSpriteSet = setIndex;
                break;
            case 1:
                next = pTri[(s16)triIndex].next[0];
                if (next == WORLD_MAP_STEP_NONE) goto resolve;
                triIndex = next;
                if (pTri[next].kind != 1) {
                    break;
                }
                ret = 0;
                again = 0;
                WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[0]);
                break;
            case 2:
                next = pTri[(s16)triIndex].next[1];
                if (next == WORLD_MAP_STEP_NONE) goto resolve;
                triIndex = next;
                if (pTri[next].kind != 1) {
                    break;
                }
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
                    triIndex = next2;
                    if (pTri[next2].kind != 1) {
                        break;
                    }
                    ret = 0;
                    again = 0;
                    WorldMapAlignDirectionToReference(pDelta, pOut, &pStep->dir[1]);
                    break;
                case 1:
                    triIndex = next;
                    if (pTri[next].kind != 1) {
                        break;
                    }
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
                    triIndex = next;
                    if (pTri[next].kind != 1) {
                        break;
                    }
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
                    triIndex = next;
                    if (pTri[next].kind != 1) {
                        break;
                    }
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
            g_WorldMapStepTriangleIndex = WORLD_MAP_STEP_NONE;
            g_WorldMapStepSpriteSet = WORLD_MAP_STEP_NONE;
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
// best: 99.93% (676 bytes = retail's exact size, 169 instructions = retail's count),
// re-measured this round from the unparked body with the K&R extern in place.
// TWO residuals, both measured, neither reachable from source:
// 1. RELOC ONLY, links identical: the probe-X store is spelled through the symbol
//    (g_WorldMapStepProbePos.x = ...) where retail shows the literal lui at,0x1F80
//    plus sw v0,0x60(at); the linker resolves the symbol to the same address. The
//    all-literal spelling is not usable: with all three 0x1F800060 uses literal, cse
//    holds that address in a callee-saved register ($s0) across the calls and the
//    body drops to 97.50% at the same 676 bytes - the mixed spelling is load-bearing
//    for the frame's register set.
// 2. THE ONE BYTE DIFFERENCE: inside the search loop retail forms the address as
//    lui $t0,%hi(sym) then addiu $a0,$t0,%lo(sym) - the high half in a scratch
//    pseudo - while every spelling here gives maspsx's TIED la expansion
//    (lui $a0 / addiu $a0,a0). That split is a COMPILER-VERSION signature: cc1_probe.sh
//    on this unit prints one la $4,sym for gcc-2.7.2-psx and gcc-2.6.0-psx in every
//    context (bare extern, symbol plus field offset, unbounded array extern,
//    (SVec*)(u8Ptr + 0x60), store-then-take) and the split for gcc-2.7.2-cdk-psx.
//    cdk cannot take this body: 89.1% at 644 bytes (32 short), 67.8% at 676 with
//    -fno-schedule-insns/-insns2; and the unit cannot switch to cdk (its ten matched
//    functions drop to 75-97), so this is a MIXED unit - one function whose TU was
//    built with the splitting cc1 - and neither a preset switch nor a unit split
//    buys it. Parked; levers.md o-untied-hi-lo-split-is records it as settled.
// How the 8-byte argument narrowing was removed, the find worth keeping: the
// caller's kind stays s32 and the CALLEE keeps its s16 signature, but the call is
// made through a K&R declaration, which makes the 5th argument a raw word copy
// (lw t0,0x60(sp) then sw t0,0x10(sp)) exactly as retail does. The prototyped
// declaration forces sll/sra on kind (684 bytes, 98.72%), and widening the callee to
// s32 does the same while breaking the callee's own lh load. Retail's two TUs simply
// disagreed about the prototype. The K&R declaration sits INSIDE this block, just
// above the body: park.py's unpark extracts only the definition, so re-add the extern
// line by hand after un-parking or the 8 bytes come back.
// MEASURED AND DEAD, so a later lane does not repeat them: a literal probe-X store
// (97.50%); gcc-2.7.2 at -G0 (identical 99.93), -O1 71.22, -O3/-Os rejected by cc1;
// a pointer local before the loop, a per-iteration assignment, an inline cast, an
// unbounded-array extern, and the probe_example.c store-then-take shape; three
// permuter runs from this body (1337+1273+1424 iterations, base=20 best=20 each).
// The unit preset is right: 99.93 against cdk's 89.1.
/*
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
}*/

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
