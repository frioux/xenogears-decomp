#include "common.h"
#include "battle/actor.h"
#include "libgear/main.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

extern u8 *g_BattleColorSourceA;
extern u8 *g_BattleColorSourceB;
extern u8 *g_BattleSpriteRenderData;
extern u8 *g_BattlePartPrimBuffers; /* [2] in misc22; only element 0 is used here */
/* Array view of the same symbol: BattleCopyColorTintToRenderData's guard has to be a pointer test, and
   over the scalar extern 'g_BattlePartPrimBuffers[0] != NULL' tests a BYTE
   (2 instructions and 8 bytes too many). */
extern u8 *g_BattlePartPrimBuffersArray[2] __asm__("g_BattlePartPrimBuffers");
/* Volatile alias of the same symbol: retail reloads the pointer for loop 1's first
   prim store instead of reusing the guard's load, and a volatile read cannot be cse'd. */
extern u8 * volatile g_BattlePartPrimBuffersVolatile __asm__("g_BattlePartPrimBuffers");
extern u8 g_BattleSpritePool[];
extern u8 g_BattleBlockPool[];
extern u16 g_BattleCurrentActorSlot;
extern u16 g_BattleCommandActorMask;
extern u8 g_BattleColorTintPending;
extern BattleActor *g_BattleActors[];

extern void BattleFreeBlockPool(u8 *);
extern void BattleFreeSpritePool(u8 *);
extern void func_800A9FF0(s32);
extern void BattleAppendActorCommandBytes(void *, void *, void *, void *);
extern s32 BattleFindFirstActorInMask(void);
extern void BattleFreeImageEntry(void *);
extern void BattleCopyColorTintToRenderData(void);
extern void BattleSetCurrentSpriteAndTarget(s32, s32);
extern void func_800A3640(void *, s32, s32, s16, s32, s16, s16, s32, s32, s32, s32, s16, s16, s16, s16, s32, s32, s32, s32);

extern u8 *g_BattleImageList;
extern u16 g_BattleImageLoaded;
extern u_long *g_BattleImageData;
extern RECT g_BattleImageRect;
extern u8 *g_BattleColorTintBase;
extern void *g_BattleColorTintWork;
extern u8 g_BattleActorSwitchFlag;
extern u8 g_FlagTimerWorkListAdds;
extern s32 g_NumFlaggedTimerWorkListEntries;
extern s32 g_BattleCommandRowIndex;
extern u8 g_BattleActorCommandIds[];

// Per-unit 6-byte state records, unit id 0..3; the original spells each byte
// as its own symbol (a struct folds the six loads into one base, which retail
// does not do: it rebuilds the address of every byte from its own %hi).
extern u8 g_BattleColorTintParams[];
extern u8 g_BattleColorBlendMode[];
extern u8 g_BattleColorBlendWeight[];
extern s8 g_BattleColorTintRed[];
extern s8 g_BattleColorTintGreen[];
extern s8 g_BattleColorTintBlue[];

// One 8-byte colour record of the sprite palette stream BattleCopyColorTintToRenderData copies the
// tint bytes into: the three bytes it writes, then five the stream leaves unused.
typedef struct {
    /* 0x0 */ u8 red;
    /* 0x1 */ u8 green;
    /* 0x2 */ u8 blue;
    /* 0x3 */ u8 pad3[5];
} BattleColorRecord; // Size: 0x8

// Four colour records per actor slot (stride 0x24).
typedef struct {
    /* 0x00 */ BattleColorRecord colours[4];
    /* 0x20 */ u8 pad20[4];
} BattleColorBlock; // Size: 0x24

// One colour record per actor slot (stride 0x18).
typedef struct {
    /* 0x0  */ u8 red;
    /* 0x1  */ u8 green;
    /* 0x2  */ u8 blue;
    /* 0x3  */ u8 pad3[0x15];
} BattleColorSlot; // Size: 0x18

// BattleCopyColorTintToRenderData's view of g_BattleSpriteRenderData: the two per-slot colour
// arrays it writes, at the two strides the renderer walks them with.
typedef struct {
    /* 0x0000 */ u8 unk0[0x16E0];
    /* 0x16E0 */ BattleColorSlot unk16E0[4];
    /* 0x1740 */ BattleColorBlock unk1740[4];
} BattleSpriteColorView; // Size: 0x17D0

// The same function's view of g_BattlePartPrimBuffers[0].
typedef struct {
    /* 0x284 */ u8 unk0[0x284];
    /* 0x284 */ BattleColorSlot unk284[4];
    /* 0x2E4 */ BattleColorBlock unk2E4[2];
} BattlePartColorView; // Size: 0x32C

// One 0x10-byte record of Misc23Block::pRecords: the five section counts and
// offsets the model-list renderer reads back out of the part stream.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 unk6;
    /* 0x8 */ u16 unk8;
    /* 0xA */ u16 unkA;
    /* 0xC */ u16 unkC;
    /* 0xE */ u16 unkE;
} Misc23PartEntry; // Size: 0x10

// One 0x18-byte point of a model-list entry's trail (stride 0x18): the scaled
// part-stream value, the row it sits on, then the position the vertex takes.
typedef struct {
    /* 0x0  */ s16 unk0;
    /* 0x2  */ s16 unk2;
    /* 0x4  */ s16 x;
    /* 0x6  */ s16 y;
    /* 0x8  */ s16 z;
    /* 0xA  */ u8 padA[0xE];
} Misc23TrailPoint; // Size: 0x18

// One 0x58-byte textured trail segment func_800A7064 builds: the point indices
// the segment spans, then the two Gouraud triangles that cover them.
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ POLY_GT3 polys[2];
} Misc23TrailSegment; // Size: 0x58

// Allocation block owned by the structure passed to BattleFreeModelListEntryBuffers:
// one 0x24-byte record of BattleActor.pModelListEntries, the same record misc21.c
// declares as BattleModelListEntry.
typedef struct {
    /* 0x0  */ u16 unk0; // the first halfword of the record's part stream
    /* 0x2  */ u16 pad2;
    /* 0x4  */ s16 numVectors; // SVECTOR count for pVectors
    /* 0x6  */ s16 numSegments; // trail segment count for pSegments
    /* 0x8  */ s16 numPoints; // point count of the block pPoints[0] starts
    /* 0xA  */ s16 numSections; // section count for pSections
    /* 0xC  */ u8 red0; // the two colours of the trail's top and bottom row
    /* 0xD  */ u8 green0;
    /* 0xE  */ u8 blue0;
    /* 0xF  */ u8 red1;
    /* 0x10 */ u8 green1;
    /* 0x11 */ u8 blue1;
    /* 0x12 */ u16 pad12;
    /* 0x14 */ SVECTOR *pVectors;
    /* 0x18 */ Misc23PartEntry *pSections;
    /* 0x1C */ Misc23TrailPoint **pPoints; // one run of points per vector
    /* 0x20 */ Misc23TrailSegment *pSegments;
} Misc23Block; // Size: 0x24

// 0xE-byte node of the tree rooted at g_BattleTriangles, indexed by g_BattleTriangleCount.
typedef struct {
    /* 0x0 */ s16 vertexIndex0; // the triangle's three vertices, indexes into g_BattleTriangleVertices
    /* 0x2 */ s16 vertexIndex1;
    /* 0x4 */ s16 vertexIndex2;
    /* 0x6 */ s16 childIndex0; // the three child nodes, searched in this order
    /* 0x8 */ s16 childIndex1;
    /* 0xA */ s16 childIndex2;
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 visitStamp; // set to g_BattleTriangleVisitStamp once the node was tested
} Misc23TreeEntry; // Size: 0xE

// The streamed stage record g_BattleStageData points at. misc29.c declares the
// same record as BattleStageData and owns the two SVEC origins at 0x47C/0x482.
typedef struct {
    /* 0x000 */ u8 unk0[0x344];
    /* 0x344 */ s16 modelScale; // scaled into an actor's scale when its caller passes no skeleton entries
    /* 0x346 */ u8 unk346[0x2];
    /* 0x348 */ s16 blockPoolSize;  // size passed to BattleCreateBlockPool
    /* 0x34A */ s16 spritePoolSize; // size passed to BattleCreateSpritePool
    /* 0x34C */ u8 unk34C[0x128];
    /* 0x474 */ u8 backColorRed; // the colour SetBackColor is given
    /* 0x475 */ u8 backColorGreen;
    /* 0x476 */ u8 backColorBlue;
    /* 0x477 */ u8 unk477[0x1];
    /* 0x478 */ u8 spriteColorRed; // the colour the actor's screen quads are drawn in
    /* 0x479 */ u8 spriteColorGreen;
    /* 0x47A */ u8 spriteColorBlue;
    /* 0x47B */ u8 unk47B[0xD];
} BattleStageData; // Size: 0x488

// One slot of the model part list table BattleInitializeMesh fills in: the parts
// and their count, or a NULL pParts while the slot is free. BattleInitializeStage
// clears all BATTLE_MODEL_SLOT_COUNT of them.
typedef struct {
    /* 0x0 */ void **pParts;
    /* 0x4 */ u32 count;
} BattleModelPartList; // Size: 0x8

extern BattleModelPartList D_800C3ACC[];

extern s32 g_BattleActorStepTimer;
extern u16 g_BattleTweenVectorAngle;
extern u8 g_BattleExtraMatrixFlag;
extern u16 g_BattleShadeAngle;
extern u8 g_BattleStateBit;
extern u8 D_800D330A[];
extern BattleStageData *g_BattleStageData;
extern void BattleCreateBlockPool(u8 *, s32);
extern void BattleCreateSpritePool(u8 *, s32);
extern void BattleClearBlockSlots(void);
extern void BattleFreeModelMesh(void *, s32);
extern void SoundRemoveSedsEntry(void *);
extern void BattleFreeAllSpriteBlocks(u8 *);
extern void BattleFreeSpriteBlocksByTag(u8 *, void *, s32);
extern void BattleFreeModelSkeleton(void *);
extern void BattleFreeActorBuffer(void *);

extern s32 g_BattleTriangleVertices;
extern s32 g_BattleTriangleCount;
extern s32 g_BattleTriangles;
extern u8 g_BattleTriangleVisitStamp;
extern s32 BattleSearchTriangleTree(s32, s32, s32);

s32 BattleFindGroundTriangle(s32 arg0, s32 arg1, s32 arg2) {
    s32 result;
    s32 i;

    if (g_BattleTriangleVertices == 0) {
        return -1;
    }
    if (g_BattleTriangles == 0) {
        return -1;
    }
    if (arg1 >= g_BattleTriangleCount) {
        return -1;
    }
    for (i = 0; i < arg2; i++) {
        result = BattleSearchTriangleTree(arg0, arg1, arg2);
        if (result >= 0) {
            break;
        }
    }
    g_BattleTriangleVisitStamp++;
    if (g_BattleTriangleVisitStamp != 0) {
        return result;
    }
    g_BattleTriangleVisitStamp = 1;
    for (i = 0; i < g_BattleTriangleCount; i++) {
        ((Misc23TreeEntry *)(i * 14 + g_BattleTriangles))->visitStamp = 0;
    }
    return result;
}
s32 BattleTestPointInTriangle(SVECTOR *pBase, SVECTOR *pA, SVECTOR *pB, SVECTOR *pC) {
    VECTOR d1;
    VECTOR d2;
    VECTOR n;

    d1.vx = pA->vx - pBase->vx;
    d1.vy = 0;
    d1.vz = pA->vz - pBase->vz;
    d2.vx = pC->vx - pBase->vx;
    d2.vy = 0;
    d2.vz = pC->vz - pBase->vz;
    OuterProduct0(&d1, &d2, &n);
    if (n.vy < 0) {
        return 0;
    }
    d1.vx = pB->vx - pA->vx;
    d1.vy = 0;
    d1.vz = pB->vz - pA->vz;
    d2.vx = pC->vx - pA->vx;
    d2.vy = 0;
    d2.vz = pC->vz - pA->vz;
    OuterProduct0(&d1, &d2, &n);
    if (n.vy < 0) {
        return 0;
    }
    d1.vx = pBase->vx - pB->vx;
    d1.vy = 0;
    d1.vz = pBase->vz - pB->vz;
    d2.vx = pC->vx - pB->vx;
    d2.vy = 0;
    d2.vz = pC->vz - pB->vz;
    OuterProduct0(&d1, &d2, &n);
    return -(n.vy >= 0);
}
void BattleProjectPointOntoTriangle(SVECTOR *pBase, SVECTOR *pA, SVECTOR *pB, SVECTOR *pOut, VECTOR *pNormal) {
    VECTOR nA;
    VECTOR nB;
    long d[3];

    d[0] = pA->vx - pBase->vx;
    d[1] = pA->vy - pBase->vy;
    d[2] = pA->vz - pBase->vz;
    VectorNormal((VECTOR *)d, &nA);
    d[0] = pB->vx - pBase->vx;
    d[1] = pB->vy - pBase->vy;
    d[2] = pB->vz - pBase->vz;
    VectorNormal((VECTOR *)d, &nB);
    OuterProduct12(&nA, &nB, pNormal);
    if (pNormal->vy == 0) {
        pOut->vy = 0;
    } else {
        pOut->vy = pBase->vy + (-((pOut->vx - pBase->vx) * pNormal->vx) - (pOut->vz - pBase->vz) * pNormal->vz) / pNormal->vy;
    }
}
s32 BattleSearchTriangleTree(s32 arg0, s32 index, s32 depth) {
    s32 result;
    s32 res2;
    s32 res3;

    if (index < 0) {
        return -1;
    }
    if (((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->visitStamp != g_BattleTriangleVisitStamp) {
        ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->visitStamp = g_BattleTriangleVisitStamp;
        result = BattleTestPointInTriangle((void *)(((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->vertexIndex0 * 8 + g_BattleTriangleVertices),
                               (void *)(((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->vertexIndex1 * 8 + g_BattleTriangleVertices),
                               (void *)(((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->vertexIndex2 * 8 + g_BattleTriangleVertices), arg0);
        if (result == -1) {
            return index;
        }
    }
    if (depth <= 0) {
        return -1;
    }
    result = BattleSearchTriangleTree(arg0, ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->childIndex0, depth - 1);
    if (result >= 0) {
        return result;
    }
    res2 = BattleSearchTriangleTree(arg0, ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->childIndex1, depth - 1);
    if (res2 >= 0) {
        return res2;
    }
    res3 = BattleSearchTriangleTree(arg0, ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->childIndex2, depth - 1);
    if (res3 < 0) {
        res3 = -1;
    }
    return res3;
}

void BattleSetColorSources(s32 arg0, s32 arg1) {
    g_BattleColorSourceA = (u8 *)arg0;
    g_BattleColorSourceB = (u8 *)arg1;
}

extern u8 g_BattleImage[];
extern s32 D_800D360C;
extern s16 D_800D2D2C;
extern s16 D_800D2D30;
extern s16 D_800D2D34;
extern s16 D_800C3EA8;

void BattleInitColorTintTable(void) {
    Temp8 *pTexture;
    u8 *pDst;
    s32 offset;
    s32 i;

    for (offset = 0x12; offset >= 0; offset -= 6) {
        g_BattleColorTintParams[offset] = 0;
    }
    pTexture = ((BattleActor *)g_BattleImageList)->pTextures;
    for (i = 0; i < ((BattleActor *)g_BattleImageList)->numTextures; i++) {
        if (pTexture->isLoaded != 0) {
            // Through void** so gcc reloads the image-list pointer and its count every iteration, as
            // retail does; a typed store through pSource lets it hoist both (3 instructions short).
            *(void **)pTexture = g_BattleImage;
        }
        pTexture++;
    }
    func_800A3640(g_BattleImage, 0, 1, 0x601, 0, D_800D2D30, D_800D2D34, 0, 0, 0, 0, D_800D2D30, D_800D2D34, D_800D2D2C, D_800C3EA8, 0, 0, 0, 0);
    D_800D360C = 0;
    g_BattleColorTintBase = HeapAlloc(0x88, 1);
    g_BattleColorTintWork = HeapAlloc(0x88, 1);
    pDst = g_BattleColorTintBase;
    for (i = 0; i < 4; i++) {
        if (g_BattleSpriteRenderData != NULL) {
            pDst[0] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[0].red;
            pDst[1] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[0].green;
            pDst[2] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[0].blue;
            pDst += 4;
            pDst[0] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[1].red;
            pDst[1] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[1].green;
            pDst[2] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[1].blue;
            pDst += 4;
            pDst[0] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[2].red;
            pDst[1] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[2].green;
            pDst[2] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[2].blue;
            pDst += 4;
            pDst[0] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[3].red;
            pDst[1] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[3].green;
            pDst[2] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[3].blue;
            pDst += 4;
            pDst[0] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk16E0[i].red;
            pDst[1] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk16E0[i].green;
            pDst[2] = ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk16E0[i].blue;
            pDst += 4;
        } else {
            pDst += 0x14;
        }
        if (g_BattlePartPrimBuffers != NULL) {
            pDst[0] = ((BattlePartColorView *)g_BattlePartPrimBuffersVolatile)->unk284[i].red;
            pDst[1] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk284[i].green;
            pDst[2] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk284[i].blue;
        }
        pDst += 4;
    }
    for (i = 0; i < 2; i++) {
        if (g_BattlePartPrimBuffers != NULL) {
            pDst[0] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[0].red;
            pDst[1] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[0].green;
            pDst[2] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[0].blue;
            pDst += 4;
            pDst[0] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[1].red;
            pDst[1] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[1].green;
            pDst[2] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[1].blue;
            pDst += 4;
            pDst[0] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[2].red;
            pDst[1] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[2].green;
            pDst[2] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[2].blue;
            pDst += 4;
            pDst[0] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[3].red;
            pDst[1] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[3].green;
            pDst[2] = ((BattlePartColorView *)g_BattlePartPrimBuffers)->unk2E4[i].colours[3].blue;
            pDst += 4;
        } else {

            pDst += 0x10;
        }
    }
    pDst[0] = g_BattleColorSourceA[0];
    pDst[1] = g_BattleColorSourceA[1];
    pDst[2] = g_BattleColorSourceA[2];
    pDst += 4;
    pDst[0] = g_BattleColorSourceB[0];
    pDst[1] = g_BattleColorSourceB[1];
    pDst[2] = g_BattleColorSourceB[2];
}
void BattleSetActorRectParams(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    if (arg0 < 4) {
        if (arg1 >= 0) {
            g_BattleColorTintPending = 1;
            g_BattleColorTintParams[arg0 * 6] = 1;
            g_BattleColorBlendMode[arg0 * 6] = arg1;
            g_BattleColorBlendWeight[arg0 * 6] = arg2;
            g_BattleColorTintRed[arg0 * 6] = arg3;
            g_BattleColorTintGreen[arg0 * 6] = arg4;
            g_BattleColorTintBlue[arg0 * 6] = arg5;
        } else {
            g_BattleColorTintParams[arg0 * 6] = 0;
        }
    }
}

void BattleCopyColorTintToRenderData(void) {
    u8 *pSrc;
    s32 i;

    pSrc = g_BattleColorTintBase;
    for (i = 0; i < 4; i++) {
        if (g_BattleSpriteRenderData != NULL) {
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[0].red = pSrc[0];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[0].green = pSrc[1];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[0].blue = pSrc[2];
            pSrc += 4;
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[1].red = pSrc[0];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[1].green = pSrc[1];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[1].blue = pSrc[2];
            pSrc += 4;
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[2].red = pSrc[0];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[2].green = pSrc[1];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[2].blue = pSrc[2];
            pSrc += 4;
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[3].red = pSrc[0];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[3].green = pSrc[1];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk1740[i].colours[3].blue = pSrc[2];
            pSrc += 4;
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk16E0[i].red = pSrc[0];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk16E0[i].green = pSrc[1];
            ((BattleSpriteColorView *)g_BattleSpriteRenderData)->unk16E0[i].blue = pSrc[2];
            pSrc += 4;
        } else {
            pSrc += 0x14;
        }
        if (g_BattlePartPrimBuffersArray[0] != NULL) {
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk284[i].red = pSrc[0];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk284[i].green = pSrc[1];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk284[i].blue = pSrc[2];
        }
        pSrc += 4;
    }
    for (i = 0; i < 2; i++) {
        if (g_BattlePartPrimBuffersArray[0] != NULL) {
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[0].red = pSrc[0];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[0].green = pSrc[1];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[0].blue = pSrc[2];
            pSrc += 4;
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[1].red = pSrc[0];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[1].green = pSrc[1];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[1].blue = pSrc[2];
            pSrc += 4;
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[2].red = pSrc[0];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[2].green = pSrc[1];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[2].blue = pSrc[2];
            pSrc += 4;
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[3].red = pSrc[0];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[3].green = pSrc[1];
            ((BattlePartColorView *)g_BattlePartPrimBuffersArray[0])->unk2E4[i].colours[3].blue = pSrc[2];
            pSrc += 4;
        } else {
            pSrc += 0x10;
        }
    }
    g_BattleColorSourceA[0] = pSrc[0];
    g_BattleColorSourceA[1] = pSrc[1];
    g_BattleColorSourceA[2] = pSrc[2];
    pSrc += 4;
    g_BattleColorSourceB[0] = pSrc[0];
    g_BattleColorSourceB[1] = pSrc[1];
    g_BattleColorSourceB[2] = pSrc[2];
}

void BattleApplyColorTint(u8 *pDst, s32 index, u8 *pSrc) {
    s32 r;
    s32 g;
    s32 b;
    s32 t;
    s32 k;

    r = g_BattleColorTintRed[index * 6] * 8;
    g = g_BattleColorTintGreen[index * 6] * 8;
    b = g_BattleColorTintBlue[index * 6] * 8;
    if (g_BattleColorTintParams[index * 6] != 0) {
        switch (g_BattleColorBlendMode[index * 6]) {
        case 0:
            r += pSrc[0];
            g += pSrc[1];
            b += pSrc[2];
            break;
        case 1:
            r += pSrc[0] >> 1;
            g += pSrc[1] >> 1;
            b += pSrc[2] >> 1;
            break;
        case 2:
            r += pSrc[0] >> 2;
            g += pSrc[1] >> 2;
            b += pSrc[2] >> 2;
            break;
        case 3:
            t = (pSrc[0] + pSrc[1] + pSrc[2]) / 3;
            r += t;
            g += t;
            b += t;
            break;
        case 4:
            r = 0;
            g = 0;
            b = 0;
            break;
        }
    }
    k = g_BattleColorBlendWeight[index * 6];
    r += (pSrc[0] - r) * k / 32;
    g += (pSrc[1] - g) * k / 32;
    b += (pSrc[2] - b) * k / 32;
    if (r >= 0x100) {
        pSrc[0] = 0xFF;
    } else if (r < 0) {
        pSrc[0] = 0;
    } else {
        pSrc[0] = r;
    }
    if (g >= 0x100) {
        pSrc[1] = 0xFF;
    } else if (g < 0) {
        pSrc[1] = 0;
    } else {
        pSrc[1] = g;
    }
    if (b >= 0x100) {
        pSrc[2] = 0xFF;
    } else if (b < 0) {
        pSrc[2] = 0;
    } else {
        pSrc[2] = b;
    }
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
}

extern void *D_800D3608;
extern void func_80025D4C(s32, void *, void *, void *, s32, s32, s32, s32, s32);
extern void func_80026F44(s32, s32, void *, void *);

// 81.43% (gcc 2.6.0 -G0 --expand-div), 1196 bytes / 299 insns against retail's 1200 /
// 300 instructions, 177 differing lines. Round 475 re-measured the park for real: the
// old 84.16 note came from a body that did not compile (D_800D3608, func_80025D4C and
// func_80026F44 were declared INSIDE the comment block, so cc1 exited 33 and the number
// was a stale object's). The three externs now sit just above the stub, so an unpark
// compiles as-is; that is the only change this round made to the body.
// Structure matches retail everywhere it was checked this round: the two early-out
// guards, the D_800D2D2C * D_800C3EA8 memcpy, the 0x88-byte widget copy with retail's
// lwl/swl pair plus 8-byte tail, the AAD switch (bltz + slti/beq tree), the pData += 4
// post-increment argument chain, the 0x14/0x10 skips in the else arms, and the closing
// LoadImage(&g_BattleImageRect, D_800D3608). What is left is loop strength reduction and
// the saved-register budget, which is why the byte count is 4 short: retail hoists three
// loop constants into callee-saved registers before the j loops (s1 = 0x173C, s4 = 0x16DC,
// s5 = 0x280, each advancing by 0x24/0x18/0x18 in the loop tails), keeps
// g_BattleColorBlendWeight + i*6 in s7 with fp = s7 - 1 for g_BattleColorBlendMode, and
// spends s2/s3 on i/j, while this body re-forms the constant+index expression per call
// through $at and so needs one more saved register and a different frame ($68 vs $70).
// Do not spend a lane on hand shapes here: the residual is the combine_givs/loop.c
// induction-variable count, not a spelling. If it is attacked, start from the two pointer
// locals (pKind -> g_BattleColorBlendMode, pRec -> g_BattleColorBlendWeight) and the three
// advancing offset locals that retail's s1/s4/s5 demand, and gate with check_bin (this
// overlay unit owns its rodata).
/*
void func_800A6AE8(void) {
    u8 *pFlag;
    u8 *pData;
    s32 i;
    s32 j;

    pFlag = &g_BattleColorTintPending;
    if (*pFlag == 0) {
        return;
    }
    if (g_BattleImageLoaded == 0) {
        return;
    }
    *pFlag = 0;
    memcpy(D_800D3608, g_BattleImageData, D_800D2D2C * D_800C3EA8 * 2);
    memcpy(g_BattleColorTintWork, g_BattleColorTintBase, 0x88);
    for (i = 0; i < 4; i++) {
        u8 *pRec;

        pRec = &g_BattleColorBlendWeight[i * 6];
        if (g_BattleColorTintParams[i * 6] == 0) {
            continue;
        }
        switch (pRec[-1]) {
        case 0:
        case 1:
        case 2:
        case 3:
            func_80025D4C(D_800D2D2C * D_800C3EA8, D_800D3608, D_800D3608, D_800D3608,
                          g_BattleColorTintRed[i * 6], g_BattleColorTintGreen[i * 6], g_BattleColorTintBlue[i * 6],
                          pRec[-1], pRec[0]);
            break;
        case 4:
            func_80026F44(D_800D2D2C * D_800C3EA8, pRec[0], D_800D3608, g_BattleImageData);
            break;
        }
        pData = g_BattleColorTintWork;
        for (j = 0; j < 4; j++) {
            if (g_BattleSpriteRenderData != NULL) {
                BattleApplyColorTint((mem_addr)g_BattleSpriteRenderData + 0x173C + j * 0x24 + 4, i, pData += 4);
                BattleApplyColorTint((mem_addr)g_BattleSpriteRenderData + 0x173C + j * 0x24 + 0xC, i, pData += 4);
                BattleApplyColorTint((mem_addr)g_BattleSpriteRenderData + 0x173C + j * 0x24 + 0x14, i, pData += 4);
                BattleApplyColorTint((mem_addr)g_BattleSpriteRenderData + 0x173C + j * 0x24 + 0x1C, i, pData += 4);
                BattleApplyColorTint((mem_addr)g_BattleSpriteRenderData + 0x16DC + j * 0x18 + 4, i, pData += 4);
            } else {
                pData += 0x14;
            }
            if (g_BattlePartPrimBuffers[0] != NULL) {
                BattleApplyColorTint((mem_addr)g_BattlePartPrimBuffers[0] + 0x280 + j * 0x18 + 4, i, pData += 4);
            } else {
                pData += 4;
            }
        }
        for (j = 0; j < 2; j++) {
            if (g_BattlePartPrimBuffers[0] != NULL) {
                BattleApplyColorTint((mem_addr)g_BattlePartPrimBuffers[0] + 0x2E0 + j * 0x24 + 4, i, pData += 4);
                BattleApplyColorTint((mem_addr)g_BattlePartPrimBuffers[0] + 0x2E0 + j * 0x24 + 0xC, i, pData += 4);
                BattleApplyColorTint((mem_addr)g_BattlePartPrimBuffers[0] + 0x2E0 + j * 0x24 + 0x14, i, pData += 4);
                BattleApplyColorTint((mem_addr)g_BattlePartPrimBuffers[0] + 0x2E0 + j * 0x24 + 0x1C, i, pData += 4);
            } else {
                pData += 0x10;
            }
        }
        BattleApplyColorTint(g_BattleColorSourceA, i, pData += 4);
        BattleApplyColorTint(g_BattleColorSourceB, i, pData);
    }
    LoadImage(&g_BattleImageRect, D_800D3608);
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6AE8);



void BattleUploadAndFreeImageResources(void) {
    Temp8 *pRecord;
    RECT *pRect;
    s32 i;

    if (g_BattleImageLoaded != 0) {
        pRecord = g_BattleActors[31]->pTextures;
        for (i = 0; i < g_BattleActors[31]->numTextures; i++) {
            if (pRecord->isLoaded != 0) {
                pRecord->pSource = NULL;
            }
            pRecord++;
        }
        pRect = &g_BattleImageRect;
        LoadImage(pRect, g_BattleImageData);
        DrawSync(0);
        // the rect is the +0x28 member of the image record BattleFreeImageEntry takes
        BattleFreeImageEntry((u8 *)pRect - 0x28);
    }
    BattleCopyColorTintToRenderData();
    HeapFree(g_BattleColorTintBase);
    HeapFree(g_BattleColorTintWork);
}
// Parked: 565 of retail's 569 instructions, 2260 bytes against 2276, objdiff 71.13%
// under this unit's preset (gcc 2.6.0 -G0 --expand-div); frame 0x138 against retail's
// 0x130. objdiff's fuzzy number is an alignment artifact on this body (it read 13.46%
// for a 562-insn version); judge by instruction count, byte count and frame size.
// THE BODY IS lib_gear/main/main func_801E1A14's (twin_for 1.000: the two objects are
// the same 569 instructions, the other unit's park is the same task), ported with that
// unit's one fix: nSegs declared u16 (16-bit unsigned) instead of s32 is worth +3
// instructions and +12 bytes here (562 / 2248 -> 565 / 2260), because retail's nSegs
// pick is `sltu $v0,$v1,$a3` (0x800A7590) and an s32 declaration compiles it to slt.
// REMAINING 4 instructions / 16 bytes + the 8-byte frame, allocation only:
// - the rowStep division. Retail runs the FULLY CHECKED signed form at 0x800A75B0:
//   `div $zero,$v0,$a3` with both break 7 (divide by zero) and the `addiu at,-1 /
//   bne / lui 0x8000 / bne / break 6` INT_MIN/-1 guard, and its divisor $a3 is the
//   raw zero-extended `lhu` value with no mask. This compile emits only maspsx's
//   break 7 plus an `andi 0xffff` on the divisor: the u16 declaration buys the sltu
//   pick but then lets gcc fold the signed division into an unsigned one.
// - measured: `rowStep = vSpan / (s16)nSegs;` restores 2 of those 4 instructions
//   (567 insns, 2268 bytes) but needs a cast no 1998 programmer would write, so it is
//   not the parked body; it is recorded here in case a later round wants the size.
// - the frame: both sides spill exactly ONE parameter, but not the same one. Ours homes
//   rowBase at 0x18 and re-reads texX/texY from their incoming 0x140/0x144 slots;
//   retail homes numSections at 0x18 (one word copy in the prologue, `lw 0x14C` then
//   `sw 0x18`, read back twice at the tail - lhu for the pEntry->numSections store, lw
//   for the count test and the *0x10 allocation) and keeps rowBase in s6 with
//   texX/texY/texZ in s0/s2/s3. Both sides save the same ten registers
//   (ra,fp,s0-s7), so this is a permutation of which value spills, not a missing local;
//   declaring a second local for numSections only moves the extra read, it does not
//   free the slot.
// Everything else agrees with retail: the five HeapAllocs and their sizes, the four
// failure paths with their exact frees, the point loop's per-i SVECTOR re-reads, the
// four SetPolyGT3 store orders, the uStep division's checks, the sltu nSegs pick and
// the eight section clears.
// Dead ends already measured on this body (do not redo): s32/unsigned parameter widths
// for texX/texY/texZ (s32 with the (s16) narrowing inline is required; the s16 version
// is 543 insns and a 0x138 frame), hoisted nV0/nV1 texture-row locals (collapses six
// mults per j to two), `nRow = uRow + j` instead of a separate accumulator, and reading
// pEntry->pVectors[i] per j. Permuter not run: 71% is below the STOP RULE exception and
// the gap is spill and address-forming code, not a statement or operand shape.
/*
void func_800A7064(Misc23Block *pEntry, u16 *pCur, s32 rowBase, s32 scale, s32 texX, s32 texY, s32 texZ,
                   s32 numSections, s16 tileX, s16 tileY, s16 uSpan, s16 vSpan, s16 clutX, s16 clutY, u8 red0,
                   u8 green0, u8 blue0, u8 red1, u8 green1, u8 blue1) {
    SVECTOR *pVectors;
    Misc23TrailPoint **pPoints;
    Misc23TrailPoint *pPointData;
    Misc23TrailPoint *pPoint;
    Misc23TrailSegment *pSegments;
    Misc23TrailSegment *pSeg;
    Misc23PartEntry *pSection;
    POLY_GT3 *pPoly;
    u16 *pCount;
    u16 *pData;
    u8 *pIndex;
    s16 pageX;
    s16 pageY;
    u16 tpage;
    u16 clut;
    u16 rowStep;
    s16 uStep;
    u16 nSegs;
    s32 nRaw;
    s32 uStart;
    s32 uEnd;
    s32 uRow;
    s32 nRow;
    s32 i;
    s32 j;
    s32 k;
    pEntry->numVectors = *pCur++;
    pEntry->numSegments = *pCur++ << 1;
    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);

    pVectors = HeapAlloc(pEntry->numVectors * sizeof(SVECTOR), 0);
    if (pVectors == NULL) {
        pEntry->pVectors = NULL;
        return;
    }
    pEntry->pVectors = pVectors;

    for (i = 0; i < pEntry->numVectors; i++) {
        pVectors[i].vx = (*pCur++ + (s16)texX) * scale / 4096;
        pVectors[i].vy = (*pCur++ + (s16)texY) * scale / 4096;
        pVectors[i].vz = (*pCur++ + (s16)texZ) * scale / 4096;
    }

    nRaw = pCur[pEntry->numVectors];
    pEntry->numPoints = nRaw + pEntry->numVectors;

    pPoints = HeapAlloc(pEntry->numVectors * sizeof(Misc23TrailPoint *), 0);
    if (pPoints == NULL) {
        pEntry->pVectors = NULL;
        HeapFree(NULL);
        return;
    }
    pEntry->pPoints = pPoints;

    pData = pCur + pEntry->numVectors + 1;
    pIndex = (u8 *)(pData + nRaw);
    pPointData = HeapAlloc(pEntry->numPoints * sizeof(Misc23TrailPoint), 0);
    if (pPointData == NULL) {
        HeapFree(NULL);
        pEntry->pVectors = NULL;
        HeapFree(pEntry->pPoints);
        return;
    }

    pPoint = pPointData;
    pCount = pCur;
    for (i = 0; i < pEntry->numVectors; i++) {
        pPoints[i] = pPoint;
        for (j = 0; j < *pCount; j++) {
            pPoint->unk0 = *pData++ * scale / 4096;
            pPoint->unk2 = *pIndex++ + rowBase;
            pPoint->x = pVectors[i].vx;
            pPoint->y = pVectors[i].vy;
            pPoint->z = pVectors[i].vz;
            pPoint++;
        }
        pPoint->unk0 = 0;
        pPoint->unk2 = 0;
        pPoint->x = pVectors[i].vx;
        pPoint->y = pVectors[i].vy;
        pPoint->z = pVectors[i].vz;
        pPoint++;
        pCount++;
    }

    pSegments = HeapAlloc(pEntry->numSegments * sizeof(Misc23TrailSegment), 0);
    if (pSegments == NULL) {
        HeapFree(NULL);
        pEntry->pVectors = NULL;
        HeapFree(pEntry->pPoints);
        HeapFree(pPointData);
        return;
    }
    pEntry->pSegments = pSegments;

    pageX = tileX / 64 * 64;
    pageY = tileY / 256 * 256;
    tpage = GetTPage(0, 1, pageX, pageY);
    clut = GetClut(clutX, clutY);
    texX = (tileX - pageX) * 4;
    texY = tileY - pageY;
    uStep = uSpan / (pEntry->numVectors - 1);
    uStart = 0;
    uEnd = uStep;
    uRow = 0;

    pCount = pCur;
    pSeg = pSegments;
    for (i = 0; i < pEntry->numVectors - 1; i++) {
        nSegs = pCount[1];
        if (pCount[0] < nSegs) {
            nSegs = pCount[0];
        }
        rowStep = vSpan / nSegs;
        nRow = uRow;
        for (j = 0; j < nSegs; j++) {
            pSeg->unk0 = nRow;
            pSeg->unk4 = nRow + 1;
            pSeg->unk2 = nRow + *pCount + 1;
            for (k = 0; k < 2; k++) {
                pPoly = &pSeg->polys[k];
                SetPolyGT3(pPoly);
                pPoly->tpage = tpage;
                pPoly->u0 = texX + uStart;
                pPoly->v0 = texY + rowStep * j;
                pPoly->clut = clut;
                pPoly->v1 = texY + rowStep * j;
                pPoly->u2 = texX + uStart;
                pPoly->u1 = texX + uEnd;
                pPoly->v2 = texY + rowStep * (j + 1);
            }
            pSeg++;
            pSeg->unk0 = nRow + *pCount + 1;
            pSeg->unk4 = nRow + 1;
            pSeg->unk2 = nRow + *pCount + 2;
            for (k = 0; k < 2; k++) {
                pPoly = &pSeg->polys[k];
                SetPolyGT3(pPoly);
                pPoly->tpage = tpage;
                pPoly->u0 = texX + uEnd;
                pPoly->u1 = texX + uEnd;
                pPoly->clut = clut;
                pPoly->u2 = texX + uStart;
                pPoly->v0 = texY + rowStep * j;
                pPoly->v1 = texY + rowStep * (j + 1);
                pPoly->v2 = texY + rowStep * (j + 1);
            }
            pSeg++;
            nRow++;
        }
        uStart += uStep;
        uEnd += uStep;
        uRow += pCount[0] + 1;
        pCount++;
    }

    pEntry->red0 = red0;
    pEntry->green0 = green0;
    pEntry->blue0 = blue0;
    pEntry->red1 = red1;
    pEntry->green1 = green1;
    pEntry->blue1 = blue1;
    pEntry->numSections = numSections;
    if (numSections > 0) {
        pSection = HeapAlloc(numSections * sizeof(Misc23PartEntry), 0);
        if (pSection == NULL) {
            pEntry->numSections = 0;
        }
        if (pEntry->numSections > 0) {
            pEntry->pSections = pSection;
            for (i = 0; i < pEntry->numSections; i++) {
                pSection[i].unk0 = 0;
                pSection[i].unk2 = 0;
                pSection[i].unk4 = 0;
                pSection[i].unk6 = 0;
                pSection[i].unk8 = 0;
                pSection[i].unkA = 0;
                pSection[i].unkC = 0;
                pSection[i].unkE = 0;
            }
        }
    } else {
        pEntry->pSections = NULL;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7064);



INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7948);

void BattleFreeModelListEntryBuffers(Misc23Block *pBlock) {
    if (pBlock->pVectors != NULL) {
        HeapFree(pBlock->pVectors);
        // the point block is reached through the first run pointer into it
        HeapFree(pBlock->pPoints[0]);
        HeapFree(pBlock->pPoints);
        HeapFree(pBlock->pSegments);
        if (pBlock->pSections != NULL) {
            HeapFree(pBlock->pSections);
        }
        pBlock->pVectors = NULL;
    }
}

void BattleInitializeStage(void) {
    u8 *pBuf;
    s32 i;
    s32 j;
    u32 *pWord;
    s32 blockPoolSize;

    pBuf = g_BattleBlockPool;
    g_BattleActorStepTimer = 0;
    g_BattleTweenVectorAngle = 0;
    g_BattleExtraMatrixFlag = 0;
    g_BattleActorSwitchFlag = 0;
    g_BattleShadeAngle = 0;
    blockPoolSize = g_BattleStageData->blockPoolSize;
    g_BattleStateBit = 1;
    BattleCreateBlockPool(pBuf, blockPoolSize);
    BattleCreateSpritePool(g_BattleSpritePool, g_BattleStageData->spritePoolSize);
    BattleClearBlockSlots();
    j = 0x1F;
    // clears the 0x80 bytes of slot parameter words that end at g_BattleImageList
    pWord = (u32 *)&g_BattleImageList;
    for (; j >= 0; j--) {
        *pWord = 0;
        pWord--;
    }
    // 20 entries
    for (i = 19; i >= 0; i--) {
        D_800C3ACC[i].pParts = NULL;
    }
    for (i = 0x14; i >= 0; i -= 0x14) {
        *(u16 *)(D_800D330A + i) = 0;
    }
}
// The two resolved archive buffers func_800A979C hands func_800A8BF0. Both open
// with a count of relocatable words that ResolveArchiveEntryPointers fixes, then
// the blocks those words point at.
typedef struct {
    /* 0x0 */ u32 pointerCount;
    /* 0x4 */ void **ppParts; // [1] is the part block func_800AA898 takes, [2] its address
    /* 0x8 */ Temp8 *pSoundBank; // whose +8 SEDS entry becomes the actor's sound bank 1
} Misc23PartBuffer;

typedef struct {
    /* 0x0  */ u32 pointerCount;
    /* 0x4  */ u8 *pImage; // the model image func_8002DDE4 is given
    /* 0x8  */ u8 *pFile; // the model file the actor keeps its own copy of
    /* 0xC  */ u8 *pFileEnd; // its end, and the entries BattleCreateModelSkeleton walks
    /* 0x10 */ void **ppBlocks; // [1] the model header, [2...] one block per model list entry
} Misc23ModelBuffer;

// The model record a model buffer's ppBlocks[1] points at: the actor's size, the
// command it starts with, how many of each sub-record it carries, and then the
// variable-length part stream the model list entries are unpacked from.
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 height;
    /* 0x04 */ u16 width;
    /* 0x06 */ u16 depth;
    /* 0x08 */ s16 unk8; // scaled by the stage's modelScale to give the actor's scale
    /* 0x0A */ u8 command;
    /* 0x0C */ u16 flags; // copied into the actor's flags4A
    /* 0x0E */ u8 numEntries;
    /* 0x0F */ u8 padF;
    /* 0x10 */ u8 numTextures;
    /* 0x11 */ u8 pad11;
    /* 0x12 */ u8 numModelListEntries;
    /* 0x13 */ u8 pad13;
    /* 0x14 */ u16 data[1]; // the part stream: one variable-length record per model list entry
} Misc23ModelHeader;

#define BATTLE_ACTOR_ARRAY_COUNT 32
#define BATTLE_MODEL_SLOT_COUNT 20
extern s32 D_800C3B6C;
extern u8 *D_800C3B70;
extern u8 D_800C3EB7[];
extern void ResolveArchiveEntryPointers(u32 *);
extern void func_80030988(s16, s16, s32, s32);
extern void func_8002DDE4(void *, s16, s16, s16, s16, s16, s16);
extern void func_800A7064(Misc23Block *, u16 *, s32, s32, s32, s32, s32, s32, s16, s16, s16, s16, s16, s16, u8,
                          u8, u8, u8, u8, u8);
extern void func_800AA898(BattleActor *, u8 *, void *, void *);
extern BattleModelPartList *BattleInitializeMesh(u8 *, BattleModelPartList *);
extern BattleSprite *BattleCreateModelSkeleton(void *, void *, s32, s32, s16, s16, s16, s16);
extern s32 ModelFileTrimHeapBlock(void *);
extern s32 ModelUnresolvePointers(void *);
extern s32 SoundFindSedsEntry(void *, s16);
extern void SoundAddSedsEntry(void *);
extern void BattleAllocActorEntryArray(BattleActor *);
extern void BattleSnapActorToGround(BattleActor *);

// 90.05% (unit preset: gcc 2.6.0 -G0 --expand-div), 2400 bytes / 600 instructions
// against retail's 2384 / 596. Body fully recovered; the residual is the whole
// callee-saved hand-out plus 4 instructions of size.
// WHAT CLOSED 89.84 -> 90.05 (round 507), keep it: the first HeapAlloc's result went
// through the `pModelFile` local, which the final block REASSIGNS, so that one pseudo
// needed a callee-saved register for the whole function. Retail keeps nothing there
// (`jal HeapAlloc / move a0,v0 / sw a0,D_800C3B70 / jal memcpy`), i.e. the source stores
// the call's result straight into D_800C3B70 and hands the global to memcpy. Dropping
// the local: 2404 -> 2400 bytes, 601 -> 600 instructions.
// Register map, ours vs retail - this IS the residual, everything else is a register name:
//   retail s0 pFileEnd | s1 pHeader     | s2 pModelBuffer | s3 pActor  | s4 mode
//          s5 pImage   | s6 pFile       | s7 pSpawnPos    | s8 flags
//   ours   s0 pFileEnd | s1 pModelBuffer| s2 pActor      | s3 pImage  | s4 pFile
//          s5 pSpawnPos| s6 mode        | s7 pHeader      | s8 flags
// (the notes' old map was wrong about ours). Retail's pHeader outranks pModelBuffer in
// global-alloc's order and ours ranks it last; retail then reuses $s2 for the poly loop's
// actor copy where we reuse $s1, and so on down the line. Both hold the same COUNT of
// long-lived values (8 plus flags in $s8, with ppBlocks spilled to 0x80(sp) in both).
// Measured in round 507 and NOT the cause: the emission order of the pHeader/pImage/pFile/
// pFileEnd loads (90.04, identical registers - the ranking decides, not the order), and
// the `for (;;) { if (...) break; if (++i >= N) break; }` loop shape (byte-identical).
// ppBlocks written as a re-read of pModelBuffer->ppBlocks at both sites instead of a
// local is 87.38 / 2420: retail's 0x80(sp) slot is a genuinely spilled LOCAL, keep it.
// WHAT CLOSED 84.71 -> 89.73 this round, keep it: the early block holds TWO VALUES IN
// LOCALS ACROSS CALLS - `u8 **ppParts = pPartBuffer->ppParts;` used by both
// ResolveArchiveEntryPointers calls, and `Temp8 *pSoundBank = pPartBuffer->pSoundBank;`
// assigned AFTER them and used by the pStart/pResult test and both SEDS calls. Retail
// keeps ppParts in $s1 and pSoundBank in $s0, and writing them as plain re-reads of the
// struct makes gcc reload pPartBuffer->ppParts twice. The extra long-lived locals also
// flip the allocation the old note called unreachable: pPartBuffer now spills to 0x58
// (retail's `sw a2,88(sp)`, six reloads) and the flags word is copied by `move s8,s0`
// into $s8 with five `andi v0,s8,...` tests, exactly retail. `s16 mode` (was s32) is
// worth 0.11 more.
// Remaining size, by jal-anchored instruction index (orig vs ours): equal at memcpy,
// +4 at BattleInitializeMesh, +5 from SetPolyFT4 on. So:
// (1) +4, the D_800C3B6C scan. Retail tests at the top and falls into the head; cc1
//     forwards the `D_800C3B6C = 0` store and PEELS the first test as
//     `lw v0,D_800C3ACC+0` before the head. Tried and BYTE-IDENTICAL: the
//     `while (cond && ++i < COUNT)` form, so this is not the loop spelling.
// (2) +1, the poly preamble (ours has no `move s4,zero` for mode, so the +1 is elsewhere
//     in that block; retail's 6-instruction preamble is 5 here).
// (3) -1, the mode computation: retail does `andi 0x4` + `sltiu s4,v0,1` into a 16-bit
//     pseudo and then `sll a1,s4,0x10 / sra` to pass it; ours emits
//     `srl 2 / xori 1 / andi a1,v0,1` straight into the argument register.
//     `!(flags4A & 4)` is byte-identical to `(... & 4) == 0`, so the sltiu is not a
//     spelling of the comparison.
// (4) -1, pModelSource: retail has two `bnez` to ONE out-of-line `sw zero,172(s3)`; ours
//     if-converts and leaves a dead `move s3,zero`. `if ((flags & 4) || (flags & 0x80))`
//     is byte-identical to the if/else-if pair.
// What is left is hand-out order, not pressure: see the register map at the top.
// Preset ruled out on this body with try_flags.sh: 2.7.2 -G0 89.08, 2.7.2-cdk -G0
// 77.34, 2.6.0 -O3 89.84 (identical), -O1 60.76.
// ROUND 520 re-measured 90.05% / 2400 bytes / 600 insns and closed two more axes:
// - the loop shape is NOT the +4. Three spellings measure byte-identical on this body: the
//   `while (cond) { if (++i >= N) break; }` above, the retail-CFG `do { if (ptr == NULL)
//   break; } while (++i < N);`, and `for (;;) { if (ptr == NULL) break; if (++i >= N) break; }`.
//   gcc 2.6.0 always peels the entry test into the block after the `D_800C3B6C = 0` store
//   (constant-addressed `lw`) and rotates the null test into the back edge; retail tests at
//   the loop top and falls into the head, so retail's source could not be re-expressed here.
// - the LOCAL DECLARATION ORDER is dead too: reordering the locals into retail's rank order
//   (pHeader, pActor, mode, pImage, pFile, pFileEnd first) is byte-identical at 90.05.
// Permuter (4 min, -j 2, from this body): base 6644 -> best 5970, three edits. Measured
// separately: the `new_var = pHeader;` alias (used for the flags4A store) ALONE is 89.38 /
// 2416 bytes, and the inverted `if (!((flags & 4) || (flags & 0x80)))` is provably identical
// (see (4) above). The third edit is the find: `mode = 0xFE;` before the poly UV stores, with
// `u1`/`v2`/`u3` storing `mode`, measures 90.26% / 2404 bytes / 601 insns / 465 differing
// lines - one instruction MORE and 19 differing lines FEWER. It is not shippable source (it
// reuses `mode` for a UV constant), but it proves `mode`'s LIVE LENGTH is load-bearing: the
// pseudo it makes lives across the mesh/skeleton calls and the whole hand-out moves toward
// retail's map. That is the axis to permute from next, not declaration order.
// ROUND 533 re-unparked and re-measured this body: 90.05% / 2400 bytes / 600 instructions,
// i.e. the note below is not stale and the body still compiles. Two NEW spellings on the
// mode axis, both 603 instructions / 2412 bytes / 89.93% / 490 differing lines, worse:
//   if ((flags & 0x40) == 0) { mode = ...; } else { mode = 0; }   and the one-statement
//   ternary  mode = (flags & 0x40) ? 0 : (pActor->flags4A & 0x4) == 0;
// are byte-identical to each other and both make cc1 emit srl 2 / xori 1 / andi 1 plus a dead
// move v0,zero and a move a1,v0. So the default-before-if form above is the best one, and the
// round-521 "default into the else arm" lever does NOT hold for this body.
// WHY mode IS THE HAND-OUT, read off retail own stream: retail sltiu s4,v0,1 then
// sll a1,s4,0x10 / sra a1,a1,0x10 says mode is a 16-bit variable that SURVIVES A CALL - it is
// given a callee-saved register for a two-reference pseudo whose def and use are in one block -
// and retail then reuses that same $s4 for the poly loop counter (move s4,zero, addiu s4,s4,1).
// Ours folds mode into $a1, so every poly-loop constant sits one register lower (ours 0xB8 /
// 0xFE / 0xC0 in $s4/$s5/$s6 against retail $s5/$s6/$s7) and $s7 falls to pHeader where retail
// has it in $s1. The only source that would make mode call-crossing-live is one that uses it
// after func_8002DDE4, and nothing in the body does: that is the blocker, not a spelling.
/*
void func_800A8BF0(s32 slot, u16 flags, Misc23PartBuffer *pPartBuffer, Misc23ModelBuffer *pModelBuffer, s16 arg4,
                   s16 arg5, s16 arg6, s16 arg7, s16 *pSpawnPos) {
    BattleModelPartList *pPartList;
    Misc23ModelHeader *pHeader;
    BattleActor *pActor;
    s16 mode;
    u8 *pImage;
    u8 *pFile;
    u8 *pFileEnd;
    void **ppBlocks;
    void **ppParts;
    Temp8 *pSoundBank;
    Misc23Block *pBlocks;
    u16 *pData;
    u8 *pModelFile;
    s32 blockSize;
    s32 count;
    s32 i;
    s32 k;

    HeapChangeCurrentUser(4, NULL);
    if (slot >= BATTLE_ACTOR_ARRAY_COUNT) {
        return;
    }
    if (g_BattleActors[slot] != NULL) {
        return;
    }
    pActor = HeapAlloc(0x11C, 0);
    if ((flags & 0x1) == 0) {
        ResolveArchiveEntryPointers((u32 *)pModelBuffer);
        ResolveArchiveEntryPointers((u32 *)pModelBuffer->ppBlocks);
    }
    pActor->hasSedsEntry1 = 0;
    pActor->hasSedsEntry2 = 0;
    if ((flags & 0x4) == 0) {
        ResolveArchiveEntryPointers((u32 *)pPartBuffer);
        ResolveArchiveEntryPointers((u32 *)pPartBuffer->pSoundBank);
        ppParts = pPartBuffer->ppParts;
        ResolveArchiveEntryPointers((u32 *)ppParts);
        ResolveArchiveEntryPointers((u32 *)ppParts[1]);
        pSoundBank = pPartBuffer->pSoundBank;
        if (pSoundBank->pStartPixels != pSoundBank->pResultPixels) {
            if (SoundFindSedsEntry(pSoundBank->pStartPixels, 0) == 0) {
                SoundAddSedsEntry(pSoundBank->pStartPixels);
                pActor->hasSedsEntry1 = 1;
            }
        }
    }
    ppBlocks = pModelBuffer->ppBlocks;
    pHeader = (Misc23ModelHeader *)ppBlocks[1];
    pImage = pModelBuffer->pImage;
    pFile = pModelBuffer->pFile;
    pFileEnd = pModelBuffer->pFileEnd;
    g_BattleActors[slot] = pActor;
    pActor->height = pHeader->height;
    pActor->width = pHeader->width;
    pActor->depth = pHeader->depth;
    pActor->commands[0] = pHeader->command;
    pActor->flags4A = pHeader->flags;
    if (pActor->flags4A & 0x200) {
        func_80030988(2, 2, 0x40, 0x40);
    }
    if ((flags & 0x1) == 0) {
        mode = 0;
        if ((flags & 0x40) == 0) {
            mode = (pActor->flags4A & 0x4) == 0;
        }
        func_8002DDE4(pImage, mode, arg4, arg5, mode, arg6, arg7);
        blockSize = pFileEnd - pFile;
        D_800C3B70 = HeapAlloc(blockSize, 1);
        memcpy(D_800C3B70, pFile, blockSize);
        D_800C3B6C = 0;
        while (D_800C3ACC[D_800C3B6C].pParts != NULL) {
            if (++D_800C3B6C >= BATTLE_MODEL_SLOT_COUNT) {
                break;
            }
        }
        BattleInitializeMesh(D_800C3B70, &D_800C3ACC[D_800C3B6C]);
    }
    pActor->pMesh = &D_800C3ACC[D_800C3B6C];
    if ((flags & 0x40) == 0) {
        if (pActor->flags4A & 0x4) {
            pActor->pSprites = BattleCreateModelSkeleton(pActor->pMesh, pFileEnd, 2, 0, 0, 0, 0, 0);
        } else {
            pActor->pSprites = BattleCreateModelSkeleton(pActor->pMesh, pFileEnd, 2, 1, arg4, arg5, arg6, arg7);
        }
    } else {
        pActor->pSprites = BattleCreateModelSkeleton(pActor->pMesh, pFileEnd, 0, 0, 0, 0, 0, 0);
    }
    if (pSpawnPos != NULL) {
        pActor->pSprites->pos[0] = pSpawnPos[0];
        pActor->pSprites->pos[1] = pSpawnPos[1];
        pActor->pSprites->pos[2] = pSpawnPos[2];
    }
    if ((flags & 0x1) == 0) {
        pActor->textureOffsetX = arg4;
        pActor->textureOffsetY = arg5;
        pActor->blendOffsetX = arg6;
        pActor->blendOffsetY = arg7;
    } else {
        pActor->textureOffsetX = -1;
    }
    if ((flags & 0x4) || (flags & 0x80)) {
        pActor->pModelSource = NULL;
    } else {
        pActor->pModelSource = (mem_addr)pPartBuffer;
    }
    for (i = 0; i < 2; i++) {
        SetPolyFT4(&pActor->polys[i]);
        SetSemiTrans(&pActor->polys[i], 1);
        pActor->polys[i].r0 = g_BattleStageData->spriteColorRed;
        pActor->polys[i].g0 = g_BattleStageData->spriteColorGreen;
        pActor->polys[i].b0 = g_BattleStageData->spriteColorBlue;
        pActor->polys[i].clut = GetClut(0x30, 0x1CC);
        pActor->polys[i].tpage = GetTPage(0, 2, 0x380, 0);
        pActor->polys[i].u0 = 0xC0;
        pActor->polys[i].v0 = 0xC0;
        pActor->polys[i].u1 = 0xFE;
        pActor->polys[i].v1 = 0xC0;
        pActor->polys[i].u2 = 0xC0;
        pActor->polys[i].v2 = 0xFE;
        pActor->polys[i].u3 = 0xFE;
        pActor->polys[i].v3 = 0xFE;
    }
    if ((flags & 0x40) == 0) {
        pActor->scale = (pHeader->unk8 * g_BattleStageData->modelScale) >> 12;
    } else {
        pActor->scale = pHeader->unk8;
    }
    pActor->numEntries = pHeader->numEntries;
    BattleAllocActorEntryArray(pActor);
    pActor->numTextures = pHeader->numTextures;
    if (pActor->numTextures != 0) {
        pActor->pTextures = HeapAlloc(pActor->numTextures * sizeof(Temp8), 0);
        for (i = 0; i < pActor->numTextures; i++) {
            pActor->pTextures[i].isLoaded = 0;
            pActor->pTextures[i].pPixels = NULL;
            pActor->pTextures[i].pStartPixels = NULL;
            pActor->pTextures[i].pResultPixels = NULL;
        }
    }
    pActor->numModelListEntries = pHeader->numModelListEntries;
    if (pActor->numModelListEntries != 0) {
        pActor->pModelListEntries = HeapAlloc(pActor->numModelListEntries * sizeof(Misc23Block), 0);
        pBlocks = (Misc23Block *)pActor->pModelListEntries;
        pData = pHeader->data;
        for (i = 0; i < pActor->numModelListEntries; i++) {
            count = (s16)pData[0x11];
            pBlocks[i].unk0 = pData[0];
            pData++;
            func_800A7064(&pBlocks[i], ppBlocks[i + 2], *pData++, *pData++, *pData++, *pData++, *pData++,
                          count, arg4 + *pData++, arg5 + *pData++, *pData++, *pData++, arg6 + *pData++, arg7,
                          *(u8 *)pData++, *(u8 *)pData++, *(u8 *)pData++, *(u8 *)pData++, *(u8 *)pData++,
                          *(u8 *)pData++);
            pData += 2;
            for (k = 0; k < count; k++) {
                pBlocks[i].pSections[k].unk6 = *pData++;
                pBlocks[i].pSections[k].unkE = *pData++;
                pBlocks[i].pSections[k].unk0 = *pData++;
                pBlocks[i].pSections[k].unk2 = *pData++;
                pBlocks[i].pSections[k].unk4 = *pData++;
            }
        }
    }
    pActor->unk22 = 0;
    pActor->actorSlot = slot;
    if (D_800C3EB7[slot * BATTLE_ACTOR_SLOT_STRIDE] != 0) {
        if (slot < BATTLE_ACTOR_SLOT_COUNT) {
            pActor->doUpdate = 0;
        } else {
            pActor->doUpdate = 1;
        }
    } else {
        pActor->doUpdate = 1;
    }
    if ((flags & 0x40) == 0) {
        pActor->pSoundBank1 = (u8 *)pPartBuffer->pSoundBank;
        func_800AA898(pActor, g_BattleBlockPool, &pPartBuffer->ppParts[2], pPartBuffer->ppParts[1]);
        BattleAppendActorCommandBytes(pActor, pActor, g_BattleBlockPool, NULL);
        BattleSnapActorToGround(pActor);
    }
    if ((flags & 0x2) == 0) {
        ModelFileTrimHeapBlock(D_800C3B70);
        ModelUnresolvePointers(D_800C3B70);
        blockSize = (s32)HeapGetNextBlockHeader((HeapBlock *)D_800C3B70);
        pModelFile = HeapAlloc(blockSize, 0);
        memcpy(pModelFile, D_800C3B70, blockSize);
        HeapFree(D_800C3B70);
        BattleFreeModelMesh(pActor->pMesh, 0);
        BattleInitializeMesh(pModelFile, pActor->pMesh);
        pActor->pModelFile = (mem_addr)pModelFile;
    } else {
        pActor->pModelFile = 0;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8BF0);





// Archive entry allocated by func_800A9540 / BattleAllocateArchiveFilePair, stored in g_BattleArchiveEntry.
// A list of {archive file index, buffer that file is decoded into} pairs: the
// file index at 0x0/0x8/0x10/0x18 is the argument ArchiveDecodeAlignedSize and
// ArchiveStartStreamQueue take, so the list is terminated by a zero pair. The 0x18-byte
// variant stops after the third pair, the 0x20-byte one adds a fourth.
typedef struct {
    /* 0x0  */ u16 fileIndex1;
    /* 0x2  */ u16 pad2;
    /* 0x4  */ void *pBuffer1;
    /* 0x8  */ u16 fileIndex2;
    /* 0xA  */ u16 padA;
    /* 0xC  */ void *pBuffer2;
    /* 0x10 */ u16 fileIndex3;
    /* 0x12 */ u16 pad12;
    /* 0x14 */ s32 pBuffer3;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 pad1A;
    /* 0x1C */ s32 unk1C;
} Misc23ArchiveEntry; // Size: 0x20

// The current archive position, saved around the index switch in
// func_800A9540. Retail's frame reserves 0x10 bytes for it; only the first two
// words are ever read.
typedef struct {
    /* 0x0 */ s32 alignedIndex;
    /* 0x4 */ s32 remainder;
    /* 0x8 */ s32 unk8;
    /* 0xC */ s32 unkC;
} Misc23ArchivePos; // Size: 0x10

extern Misc23ArchiveEntry *g_BattleArchiveEntry;
extern int ArchiveDecodeAlignedSize(unsigned int);
extern void ArchiveStartStreamQueue(Misc23ArchiveEntry *, s32, s32);
extern u8 g_BattleSlotTextIndex[];
extern u8 g_BattleSlotFileCount[];
extern u8 D_800C3509[];

// 98.98% at retail's exact 372 bytes / 93 instructions (unit preset gcc 2.6.0).
// Every call, the frame (0x38, locals 0x10/0x14 -> Misc23ArchivePos), all six save
// slots, both HeapAlloc flags, the 0x20 HeapAlloc size, the if-arm's store offsets
// AND the beqz delay slot are retail's. Round 462: the delay slot was a source-order
// problem, not reorg. With 'pEntry->pBuffer2 = HeapAlloc(ArchiveDecodeAlignedSize(
// nameLen), 0); pTail = &pEntry->fileIndex3;' the RTL is [call, a1=0, sw v0,12(s2),
// addiu pTail, beqz] and reorg puts the pTail addiu in the slot; storing the call's
// result through a temp and advancing pTail before the store
// ('pBuffer2 = HeapAlloc(...); pTail = &pEntry->fileIndex3; pEntry->pBuffer2 = pBuffer2;')
// gives [call, a1=0, addiu pTail, sw v0,12(s2), beqz] and reorg then puts the sw in
// the slot, which is retail's. 94.73 -> 98.98 with that one edit.
// Only residual left: TWO mirrored callee-saved pairs, retail index $s1 / pTable $s0
// and pEntry $s2 / pTail $s3, ours both reversed (the whole instruction stream is
// otherwise identical). -dg for this body: '6 regs to allocate: 78 73 72 75 76 77'
// = pTable(78) pTail(73) pEntry(72) count(75) index(76) nameLen(77), dispositions
// '71 in 16 (s0) 72 in 19 73 in 18 75 in 20 76 in 16 77 in 17 78 in 17' and -dl
// pTable 8 refs/15 insns (quotient 6.4), pTail 9/25 (4.3), pEntry 7/32 (1.75),
// count 5/34 (1.2), index 3/12 (1.0). pTable is processed FIRST and still cannot take
// $s0: its -dg conflict list names hard reg 16, i.e. the arg0/base chain (pseudo 71)
// still occupies $s0 across pTable's range, so it lands in $s1 and index later takes
// $s0 - that is the $s0/$s1 mirror, and it is a conflict, not a quotient gap. pEntry
// cannot outrank pTail because local-alloc coalesces the 'pTail = pEntry' copy and the
// stores land on pTail's pseudo (3.2x).
// Also measured: 'nameLen = pTable[index - 1] + 1' inline twice instead of a 'base'
// local (no CSE across the first HeapAlloc: 2 lbu, 380 bytes, 94.46); the pBuffer1
// store written through pEntry instead of pTail (byte-identical, 98.98); earlier
// rounds' dead levers: index '<< 1' beats '* 2' (376 bytes otherwise), index s32 vs
// u32 no change, declaration order no change, pTail->fileIndex1 for the if-arm's two
// stores 93.66/376, dropping the early 'move s3,s2' 96.15/364. A permuter run is not
// warranted below 99%.
// 99.14% at retail's exact 372 bytes / 93 instructions (unit preset gcc 2.6.0 -G0).
// Round 475: the table/index pair was flipped in - drop the pBuffer2 temp, reuse pTable
// for the second HeapAlloc result and name the zero in a local (a permuter find,
// 98.98 -> 99.14). pTable is now $s0 and index $s1, exactly retail.
// Only residual: pEntry/pTail mirrored (retail pEntry $s2 / pTail $s3, ours reversed),
// 26 differing lines. Unreachable by spelling: -dl on this body gives pEntry (pseudo 72)
// 7 refs / 32 insns, quotient 2*7*4/32 = 1.75, and pTail (73) 9 refs / 25 insns,
// 3*9*4/25 = 4.32, so pTail is processed first and takes the lowest free callee-saved
// register however the two are declared; pEntry would need 16 refs to overtake it.
// Both conflict with hard reg $s0 (the arg0 copy at entry), so neither can take $s0.
// A third 3-minute permuter run from this body: base=80 best=80 (1467 iterations).
// Also measured this round: swapping the index/pTable source order 98.98; hoisting
// arg0 * 0x170 into a slotOffset local 98.98; assigning pTable after the if instead of
// before it 94.02 / 42 lines; '*(pTable + index - 1)' for 'pTable[index - 1]' leaves
// the addu operands index-first, no change; moving 'pTail = pEntry' before
// 'nameLen = base + 1' is byte-identical and gives retail's instruction order, so it is
// kept.
// Round 484 closed the remaining cheap axes on this body. gcc-2.7.2-cdk-psx -G0 scores 85.41 (BattleAllocateArchiveFilePair 85.52, BattleUpdateAndDrawActors 78.14), so the unit's 2.6.0 -G0 preset is right and the mirror is not a compiler-version question. Two 'do { } while (0);' barriers (between 'pTail = pEntry;' and 'nameLen = base + 1;', and after 'pEntry->pBuffer2 = pTable;' before the if) are byte-identical at 99.14 / 26 lines, so the block-structure lever does not reach this pair. A fourth 3-minute permuter run from this body: 1339 iterations, base=80 best=80. NEW residual detail: the mirror is not the only difference - the 'pTable + index' sum is base-first in retail ('addu v0,s0,s1') and index-first in ours ('addu v0,s1,s0') although the source reads 'pTable[index - 1]', and the 0x8 store takes the cursor's register here where retail takes the entry's. Both residuals are register-level, so the body stays parked at 99.14.
/*
void func_800A9540(s32 arg0) {
    Misc23ArchiveEntry *pEntry;
    Misc23ArchiveEntry *pTail;
    Misc23ArchivePos pos;
    s32 slotTextIndex;
    s32 count;
    s32 index;
    u8 *pTable;
    s32 nameLen;
    s32 zero;
    u8 base;

    ArchiveGetArchiveOffsetIndices(&pos.alignedIndex, &pos.remainder);
    ArchiveSetIndex(0x28, 1);
    HeapChangeCurrentUser(4, NULL);
    slotTextIndex = g_BattleSlotTextIndex[arg0 * 0x170];
    count = g_BattleSlotFileCount[arg0 * 0x170];
    pTable = D_800C3509;
    index = slotTextIndex << 1;
    if (D_800C3509[index] < count) {
        count = 0;
    }
    pEntry = HeapAlloc(0x20, 1);
    g_BattleArchiveEntry = pEntry;
    base = pTable[index - 1];
    pTail = pEntry;
    nameLen = base + 1;
    pTail->fileIndex1 = nameLen;
    pTail->pBuffer1 = HeapAlloc(ArchiveDecodeAlignedSize(nameLen), 1);
    nameLen = base + 2;
    pEntry->fileIndex2 = nameLen;
    zero = 0;
    pTable = HeapAlloc(ArchiveDecodeAlignedSize(nameLen), zero);
    pTail = (Misc23ArchiveEntry *)&pEntry->fileIndex3;
    pEntry->pBuffer2 = pTable;
    if (count != zero) {
        nameLen += count;
        pEntry->fileIndex3 = nameLen;
        pEntry->pBuffer3 = HeapAlloc(ArchiveDecodeAlignedSize(nameLen), 1);
        pTail = (Misc23ArchiveEntry *)&pEntry->unk18;
    }
    pTail->fileIndex1 = zero;
    pTail->pBuffer1 = NULL;
    ArchiveStartStreamQueue(g_BattleArchiveEntry, zero, zero);
    ArchiveSetIndex(pos.alignedIndex, pos.remainder);
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9540);



void BattleAllocateArchiveFilePair(s32 arg0) {
    Misc23ArchiveEntry *pEntry;
    s32 alignedIndex;
    s32 remainder;
    s32 entryIndex;

    ArchiveGetArchiveOffsetIndices(&alignedIndex, &remainder);
    ArchiveSetIndex(0x28, 0);
    HeapChangeCurrentUser(4, NULL);
    pEntry = HeapAlloc(0x18, 1);
    g_BattleArchiveEntry = pEntry;
    arg0 *= 2;
    entryIndex = arg0 + 1;
    ArchiveGetFilePath(entryIndex);
    pEntry->fileIndex1 = entryIndex;
    pEntry->pBuffer1 = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 1);
    entryIndex = arg0 + 2;
    pEntry->fileIndex2 = entryIndex;
    pEntry->pBuffer2 = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 1);
    pEntry->fileIndex3 = 0;
    pEntry->pBuffer3 = 0;
    ArchiveStartStreamQueue(g_BattleArchiveEntry, 0, 0);
    ArchiveSetIndex(alignedIndex, remainder);
}

// Payload reached through Misc23ArchiveEntry::pBuffer3: a count-prefixed list of
// 16-bit entries plus the block bounded by pBlockStart..pBlockEnd.
typedef struct {
    /* 0x0 */ s32 pointerCount; // relative pointers ResolveArchiveEntryPointers fixes up
    /* 0x4 */ u8 *pEntries; // s16 count, two unused bytes, then count 8-byte entries
    /* 0x8 */ u8 *pBlockStart;
    /* 0xC */ u8 *pBlockEnd; // pBlockEnd - pBlockStart is the block's size
} Misc23ArchiveData;

extern void ResolveArchiveEntryPointers(u32 *);
extern void func_8002DDE4(void *, s16, s16, s16, s16, s16, s16);
extern void func_800A8BF0(s32, u16, Misc23PartBuffer *, Misc23ModelBuffer *, s16, s16, s16, s16, s16 *);

// 93.58% at 688 bytes / 172 insns against retail's 692 / 173 (unit preset gcc 2.6.0).
// 4 BYTES SHORT - not shippable as it stands, but the body below is one instruction
// from retail and its register map is retail's except pData/count, which are swapped
// (ours pData s3 / count s4, retail pData s4 / count s3): pSrc s1, i s2, arg1 s5,
// pBuf s6, arg0 s7, arg4-raw $30, slot s0, frame 0x68 with retail's two halfword
// spills (arg2 0x28, arg3 0x30) and no third one.
// WHAT CLOSED 82.05 -> 92.48 this round (round 534), both are keepers:
// 1. 'HeapFree(pData)'. Retail frees the pointer it already holds ('jal HeapFree /
//    move a0,s4'); spelling it 'HeapFree((void *)g_BattleArchiveEntry->pBuffer3)'
//    makes cc1 re-read the global and its field: +5 instructions / +20 bytes.
// 2. a 's32 slot' local used at BOTH the func_800A8BF0 call and the seven stores, so
//    the index is one pseudo. The old inline expression is 82.05%.
// 3. the permuter (4 min, -j 2) then found the last 1.1%: a SECOND local
//    'slotIndex = (arg0 << 1) + (i + 0xD); slot = slotIndex;', which is what hands
//    pData/count their retail registers. That is why the body below is 93.58%.
// THE CATCH: with that copy in place loop.c turns 'slot' into a self-incrementing biv
// ('addiu s0,s0,1' per iteration) where retail recomputes it from i every time
// ('sll s0,s7,1' in the bne delay slot, 'addiu v0,s2,13', 'addu s0,s0,v0'), so the
// function comes out exactly one instruction short. THE BLOCKER IS THE BIV, not the
// allocator: a spelling that keeps this register map AND recomputes slot from i
// matches. Measured, all worse:
// - without the copy local (correct size, 692 bytes): 92.48%, pData spills to 0x38,
//   frame 0x70, and the register it frees goes to a biv '(arg0<<1)+13' in $30.
// - call arg = slotIndex, stores = slot: byte-identical 93.58 / 688 (the copy coalesces).
// - stores re-spelled as the inline '(arg0 << 1) + (i + 0xD)': 86.27 / 688.
// - '(arg0 << 1) + i + 0xD' without the inner parens: 90.09 / 692.
// The old register complaints are gone; the 0x5E-scaled-by-sizeof(BattleActor) bug the
// previous note describes was already fixed by the BattleActor fields
// parentJointIndex / parentActorSlot / isAttached / skipGroundSnap / attachOffsetX/Y/Z.
// 93.58% at 688 bytes / 172 insns against retail's 692 / 173 (unit preset gcc 2.6.0).
// 4 BYTES SHORT - not shippable, but it holds retail's register map exactly.
/*
void func_800A979C(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    u8 *pSrc;
    s32 i;
    s32 count;
    Misc23ArchiveData *pData;
    u8 *pBuf;
    s32 size;
    s32 flags;
    s32 slot;
    s32 slotIndex;

    func_800A8BF0(arg0, 0, g_BattleArchiveEntry->pBuffer2, g_BattleArchiveEntry->pBuffer1, arg1, arg2, arg3, arg4, 0);
    g_BattleActors[arg0]->isBusy = 1;
    g_BattleActors[arg0]->unk22 = 1;
    if (g_BattleArchiveEntry->pBuffer3 != 0) {
        pData = (Misc23ArchiveData *)g_BattleArchiveEntry->pBuffer3;
        ResolveArchiveEntryPointers((u32 *)pData);
        pSrc = pData->pEntries;
        count = *(s16 *)pSrc;
        if (count != 0) {
            pSrc += 4;
            size = pData->pBlockEnd - pData->pBlockStart;
            pBuf = HeapAlloc(size, 0);
            memcpy(pBuf, pData->pBlockStart, size);
            for (i = 0; i < count; i++) {
                slotIndex = (arg0 << 1) + (i + 0xD);
                slot = slotIndex;
                flags = (i != 0) ? 7 : 2;
                if (i == count - 1) {
                    flags -= 2;
                }
                func_800A8BF0(slotIndex, flags, pBuf, pData->pBlockEnd, arg1, arg2, arg3, arg4, 0);
                g_BattleActors[slot]->parentJointIndex = *(u16 *)pSrc;
                g_BattleActors[slot]->parentActorSlot = arg0;
                g_BattleActors[slot]->isAttached = 2;
                g_BattleActors[slot]->skipGroundSnap = 1;
                pSrc += 2;
                g_BattleActors[slot]->attachOffsetX = *(u16 *)pSrc;
                pSrc += 2;
                g_BattleActors[slot]->attachOffsetY = *(u16 *)pSrc;
                pSrc += 2;
                g_BattleActors[slot]->attachOffsetZ = *(u16 *)pSrc;
                pSrc += 2;
            }
        } else {
            func_8002DDE4(pData->pBlockStart, 1, arg1, arg2, 1, arg3, arg4);
        }
        HeapFree(pData);
    }
    HeapFree(g_BattleArchiveEntry);
    DrawSync(0);
    HeapFree(g_BattleArchiveEntry->pBuffer1);
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A979C);



extern s32 g_BattleActorStepIncrement;
extern u16 g_BattleActorMask;
extern s16 g_BattleTweenVectorScale;
extern s32 g_ModelRenderTestFlags;
extern MATRIX *g_BattleColorMatrix;
extern u8 g_BattleBlockPoolStepPending;
extern u8 BattleClampScaledByte(s16, s16, s32);
extern s32 BattleGetActorHeight(s32);
extern s32 BattleGetActorRadius(s32);
extern void BattlePushActorOutOfCircle(s32, s32, s32, s32);
extern void BattleSteerActorTowardCircles(s32);
extern void BattleSpriteUpdateWorldPosition(BattleActor *);
extern void BattleTransformPartPositions(BattleActor **);
extern void BattleStepActorAnimation(BattleActor *, u8 *, s32, s32, s32);
extern void BattleDrawActor(BattleActor *, MATRIX *, MATRIX *, s32, s32, u_long *, s32);
extern void BattleRenderSpriteSlots(u8 *, MATRIX *, s32, u_long *, s32);
extern void func_800B026C(void *, s32, s32, s32);
extern u16 g_BattleShadeScale;

/* Three statements below are split where the original programmer would have
   written one expression, and each split is load-bearing: it decides which
   register cc1 gives a value and which instruction reorg steals into a branch
   delay slot. Folded back (timer + 1 + incr, for (step = 0; ...), for (j = 0;
   ...)) the body comes out 2 instructions short and four callee-saved registers
   rotate. */
void BattleUpdateAndDrawActors(MATRIX *pLightMatrix, MATRIX *pViewMatrix, u_long *pOt, s32 quadIndex) {
    BattleActor **ppActors;
    BattleActor **ppSlot;
    BattleSprite *pSprites;
    BattleSprite *pOtherSprites;
    s32 height;
    s32 step;
    s32 i;
    s32 j;
    s32 timer;
    u8 colors[3];

    // split: also fixes the order of the two gp-relative loads at the top
    timer = g_BattleActorStepTimer + 1;
    g_BattleActorStepTimer = timer + g_BattleActorStepIncrement;
    // split: keeps the step = 0 copy at the end of the entry block, which is
    // what leaves the loop-skip branch's delay slot free for the duplicated
    // "step * 0x38" instruction
    step = 0;
    if (g_BattleActorStepTimer >= 7) {
        g_BattleActorStepTimer = 6;
    }
    while (g_BattleActorStepTimer >= 2) {
        g_BattleActorStepTimer -= 2;
        step++;
    }
    g_BattleTweenVectorAngle += step * 0x38;
    g_BattleTweenVectorScale = (rsin((s16)g_BattleTweenVectorAngle) + 0x1000) / 800 + 4;
    g_BattleShadeAngle += 0x80;
    g_BattleShadeScale = (rsin((s16)g_BattleShadeAngle) + 0x1000) / 32;
    colors[0] = BattleClampScaledByte(g_BattleShadeScale, 0x20, g_BattleStageData->backColorRed);
    colors[1] = BattleClampScaledByte(g_BattleShadeScale, 0x20, g_BattleStageData->backColorGreen);
    colors[2] = BattleClampScaledByte(g_BattleShadeScale, 0x20, g_BattleStageData->backColorBlue);
    if (g_BattleActors[g_BattleCurrentActorSlot] != 0) {
        if (!(g_BattleActors[g_BattleCurrentActorSlot]->flags4A & 0x20)) {
            // split: j = 0 must be the first of the three preheader copies
            j = 0;
            ppActors = g_BattleActors;
            ppSlot = ppActors;
            for (; j < BATTLE_ACTOR_SLOT_COUNT; j++, ppSlot++) {
                if (*ppSlot == 0) {
                    continue;
                }
                if (g_BattleCurrentActorSlot == j) {
                    continue;
                }
                if ((*ppSlot)->parentActorSlot != 0xFF) {
                    continue;
                }
                if ((g_BattleCommandActorMask >> j) & 1) {
                    continue;
                }
                if ((*ppSlot)->doUpdate == 0) {
                    continue;
                }
                pSprites = ppActors[g_BattleCurrentActorSlot]->pSprites;
                height = BattleGetActorHeight(j);
                pOtherSprites = (*ppSlot)->pSprites;
                if (pOtherSprites->pos[1] - height < pSprites->pos[1]) {
                    if (ppActors[g_BattleCurrentActorSlot]->pSprites->pos[1] -
                            BattleGetActorHeight(g_BattleCurrentActorSlot) <
                        pOtherSprites->pos[1]) {
                        BattlePushActorOutOfCircle(g_BattleCurrentActorSlot, (*ppSlot)->pSprites->pos[0],
                                                   (*ppSlot)->pSprites->pos[2],
                                                   BattleGetActorRadius(j) + BattleGetActorRadius(g_BattleCurrentActorSlot));
                    }
                }
            }
            BattleSteerActorTowardCircles(g_BattleCurrentActorSlot);
        }
    }
    for (i = 0; i < 32; i++) {
        if (g_BattleActors[i] != 0) {
            BattleStepActorAnimation(g_BattleActors[i], g_BattleBlockPool, step, quadIndex, g_BattleActorStepIncrement);
        }
    }
    if (g_BattleBlockPoolStepPending != 0) {
        func_800B026C(g_BattleBlockPool, step, 0, quadIndex);
    }
    for (i = 0; i < 32; i++) {
        if (g_BattleActors[i] != 0) {
            if (g_BattleActors[i]->parentActorSlot < 0xFF) {
                BattleSpriteUpdateWorldPosition(g_BattleActors[i]);
            }
        }
    }
    BattleTransformPartPositions(g_BattleActors);
    SetColorMatrix(g_BattleColorMatrix);
    if (g_BattleStateBit != 0) {
        for (i = 0; i < 0x1F; i++) {
            if (g_BattleActors[i] != 0) {
                if ((g_BattleActorMask >> i) & 1) {
                    SetBackColor(colors[0], colors[1], colors[2]);
                } else {
                    SetBackColor(g_BattleStageData->backColorRed, g_BattleStageData->backColorGreen,
                                 g_BattleStageData->backColorBlue);
                }
                if (g_BattleActors[i]->flags4A & 0x40) {
                    g_ModelRenderTestFlags = 0;
                } else {
                    g_ModelRenderTestFlags = 1;
                }
                BattleDrawActor(g_BattleActors[i], pLightMatrix, pViewMatrix, 1, g_BattleActorStepIncrement, pOt, quadIndex);
                g_ModelRenderTestFlags = 0;
            }
        }
    }
    BattleRenderSpriteSlots(g_BattleSpritePool, pLightMatrix, step, pOt, quadIndex);
}

void BattleFreeAllModelSlots(void) {
    s32 i;

    for (i = 0; i < 0x1F; i++) {
        func_800A9FF0(i);
    }
    BattleFreeBlockPool(g_BattleBlockPool);
    BattleFreeSpritePool(g_BattleSpritePool);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9FF0);
/* 99.90% (816 bytes = retail's exact size, 204 of 204 instructions, unit preset
   gcc 2.6.0 --expand-div -G0): whole body recovered, retail's frame (0x48) and
   saved set ($s0-$s5) exact, every call/load/store and both loop rotations equal.
   The sound-bank read is typed through Temp8 (byte-identical to the raw
   (void**) cast form, 99.90).
   91.39 -> 99.90 came from the FIRST HALF's address shape: retail keeps
   &g_BattleActors[arg0] in $s1 and reloads *($s1) at every use, so the slot
   pointer is a local, built in TWO statements -
   "ppActors = g_BattleActors; ppSlot = &ppActors[arg0];" - which is what emits
   "lui/addiu %hi(g_BattleActors)" BEFORE the "sll s4,2". The one-statement form
   "ppSlot = &g_BattleActors[arg0];" emits sll first (98.80), and indexing the
   global inline everywhere is 91.39. The second half (0x10C up) reaches
   g_BattleActors[arg0] inline instead: that split is retail's own.
   The only difference left is a dead-load register tie at the two pSprites
   null tests: retail loads the compare operand into $a1 ("lw v0,0(s1);
   lw a1,4(v0); beqz a1") where cc1 reuses the dying base register
   ("lw v0,4(v0); beqz v0") - same size, same instruction count, and the value
   only feeds the branch. The tie IS reachable from C: lib_gear's
   LibGearFreeModel (this function's twin, matched 100.00% under XenoLibrary)
   has the identical "lw a1,4(v0); beqz a1" at both sites, so it is a
   whole-function local-alloc tie set by the function's pseudo numbering, not
   by the spelling at the site. Measured, none of it moves the tie: embedded
   assignment in each test (99.90); a pSprites local declared inside each arm
   before the test (99.90, byte-identical output - cc1 cse-replaces the local
   with the field); a pSprites local assigned before the range test (95.68, 792
   bytes - gcc hoists the load); u8 *pPool =
   g_BattleBlockPool declared in each arm, the twin's pool-local shape (99.90,
   byte-identical); all six declaration orders of ppActors/ppSlot/i (99.90,
   byte-identical); the twin's whole body ported verbatim, i.e. inline
   g_BattleActors[arg0] everywhere (98.90, 12 differing lines: the $a1 pair
   twice PLUS "sw zero,0(s0)" relocated into the recursion tail's delay slot,
   so the tail's inline/pointer split is genuinely retail's); that twin body
   under -G8 (98.90); this split body under -G8 (99.90); -O3 (82.98); gcc
   2.7.2 (99.63); gcc 2.7.2-cdk (92.67); a 1745-iteration permuter run
   (base=20 best=20, PERMUTER_NONE).
   Re-measured in round 417: 99.90 again, 204/204 insns, 816 bytes (so this note
   is not stale); the bare truthiness test - if ((*ppSlot)->pSprites) - with no
   != NULL is byte-identical at 99.90. New evidence on the tie itself: retail
   DOES reuse a dying $v0 for a dead load elsewhere in this same function
   (0x800AA1BC: lw v0,0(s0); lbu v0,0x10E(v0); beqz v0), so the two pSprites
   sites are not a general allocator rule but a function-wide preference that
   handed those two qtys $a1; and no local can hold that value because retail
   saves exactly $s0-$s5 (a pseudo live across the two calls would add one). */
/*
void func_800A9FF0(s32 arg0) {
    BattleActor **ppActors;
    BattleActor **ppSlot;
    s32 i;

    ppActors = g_BattleActors;
    ppSlot = &ppActors[arg0];
    if (*ppSlot != NULL) {
        if ((*ppSlot)->pModelFile != NULL) {
            HeapFree((void *)(*ppSlot)->pModelFile);
            BattleFreeModelMesh((*ppSlot)->pMesh, 1);
        }
        if ((*ppSlot)->hasSedsEntry1 != 0) {
            SoundRemoveSedsEntry(((Temp8 *)(*ppSlot)->pSoundBank1)->pStartPixels);
        }
        if ((*ppSlot)->pModelSource != NULL) {
            HeapFree((void *)(*ppSlot)->pModelSource);
        }
        if ((u32)(arg0 - 0x13) >= 0xC) {
            if ((*ppSlot)->pSprites != NULL) {
                BattleFreeAllSpriteBlocks(g_BattleBlockPool);
                BattleFreeSpriteBlocksByTag(g_BattleBlockPool, (*ppSlot)->pSprites, 0xFF);
                BattleFreeModelSkeleton((*ppSlot)->pSprites);
                (*ppSlot)->pMesh = NULL;
                (*ppSlot)->pSprites = NULL;
            }
            BattleFreeActorBuffer(*ppSlot);
        } else if ((*ppSlot)->pSprites != NULL) {
            BattleFreeAllSpriteBlocks(g_BattleBlockPool);
            BattleFreeSpriteBlocksByTag(g_BattleBlockPool, (*ppSlot)->pSprites, 0xFF);
            HeapFree((*ppSlot)->pSprites);
        }
        if (g_BattleActors[arg0]->numEntries != 0) {
            HeapFree((void *)g_BattleActors[arg0]->pEntries);
        }
        if (g_BattleActors[arg0]->numTextures != 0) {
            for (i = 0; i < g_BattleActors[arg0]->numTextures; i++) {
                BattleFreeImageEntry(g_BattleActors[arg0]->pTextures + i);
            }
            HeapFree(g_BattleActors[arg0]->pTextures);
        }
        if (g_BattleActors[arg0]->numModelListEntries != 0) {
            for (i = 0; i < g_BattleActors[arg0]->numModelListEntries; i++) {
                BattleFreeModelListEntryBuffers((Misc23Block *)g_BattleActors[arg0]->pModelListEntries + i);
            }
            HeapFree((void *)g_BattleActors[arg0]->pModelListEntries);
        }
        HeapFree(g_BattleActors[arg0]);
        g_BattleActors[arg0] = NULL;
    }
    if (arg0 < 3) {
        func_800A9FF0(arg0 * 2 + 0xD);
        func_800A9FF0(arg0 * 2 + 0xE);
    }
}
*/

void BattleQueueActorCommand(u16 arg0, u16 arg1, void *arg2) {
    g_BattleCurrentActorSlot = arg0;
    g_BattleCommandActorMask = arg1;
    ((u8 *)g_BattleActors[arg0])[0x35] = 0;
    if (g_BattleActors[arg0] != NULL) {
        BattleAppendActorCommandBytes(g_BattleActors[arg0], g_BattleActors[arg0], g_BattleBlockPool, arg2);
    }
}

void BattleSwitchActorAndQueueCommand(u16 arg0, u16 arg1, void *arg2) {
    g_BattleCurrentActorSlot = arg0;
    g_BattleCommandActorMask = arg1;
    g_BattleActorSwitchFlag = 1;
    ((u8 *)g_BattleActors[arg0])[0x35] = 1;
    g_NumFlaggedTimerWorkListEntries = 0;
    g_FlagTimerWorkListAdds = 1;
    BattleSetCurrentSpriteAndTarget(arg0, BattleFindFirstActorInMask());
    g_BattleCommandRowIndex = 1;
    g_BattleActorCommandIds[BattleFindFirstActorInMask()] = 0;
    if (g_BattleActors[arg0] != NULL) {
        BattleAppendActorCommandBytes(g_BattleActors[arg0], g_BattleActors[arg0], g_BattleBlockPool, arg2);
    }
}
void BattleQueueActorCommandPreservingSelection(u16 arg0, u16 arg1, void *arg2) {
    u16 savedIdx;
    u16 savedUnk;

    savedIdx = g_BattleCurrentActorSlot;
    savedUnk = g_BattleCommandActorMask;
    g_BattleCurrentActorSlot = arg0;
    g_BattleCommandActorMask = arg1;
    ((u8 *)g_BattleActors[arg0])[0x35] = 0;
    if (g_BattleActors[arg0] != NULL) {
        if ((s32)arg0 != BattleFindFirstActorInMask()) {
            BattleAppendActorCommandBytes(g_BattleActors[arg0], g_BattleActors[arg0], g_BattleBlockPool, arg2);
        }
    }
    g_BattleCurrentActorSlot = savedIdx;
    g_BattleCommandActorMask = savedUnk;
}
