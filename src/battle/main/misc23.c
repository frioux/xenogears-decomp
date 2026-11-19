#include "common.h"
#include "battle/actor.h"
#include "libgear/main.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

extern u8 *g_BattleColorSourceA;
extern u8 *g_BattleColorSourceB;
extern u8 *g_BattleSpriteRenderData;
extern u8 *g_BattlePartPrimBuffers[2];
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
extern void func_800A64E4(void);
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

// Allocation block owned by the structure passed to BattleFreeModelListEntryBuffers:
// one 0x24-byte record of BattleActor.pModelListEntries, the same record misc21.c
// declares as BattleModelListEntry.
typedef struct {
    /* 0x0  */ u8 unk0[0x14];
    /* 0x14 */ void *pPoints; // 8 bytes (three s16) per point
    /* 0x18 */ void *pRecords; // 0x10 bytes per record
    /* 0x1C */ void *pPackets; // one packet pointer per point
    /* 0x20 */ void *unk20;
} Misc23Block;

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

extern s32 g_BattleActorStepTimer;
extern u16 g_BattleTweenVectorAngle;
extern u8 g_BattleExtraMatrixFlag;
extern u16 g_BattleShadeAngle;
extern u8 g_BattleStateBit;
extern u8 D_800C3ACC[];
extern u8 D_800D330A[];
extern void *g_BattleStageData;
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5EB4);
/* 99.16% (unit preset XenoBattle260Div0): the whole function is recovered and the
   compiled body is retail's exact size (1424 bytes) with retail's frame, register
   roles and instruction order. The ONLY difference is the operand order of the 30
   address addus of the two colour-copy loops: retail has "addu v0,v0,a1" /
   "addu v0,v0,a2" (the freshly loaded pool base first), gcc emits "addu v0,a1,v0"
   (biv first). Same value, different encoding, so objdiff cannot pass it and the
   linked bytes differ.
   This is the SAME tie as the parked func_800A64E4 below (98.71%, 18 sites), so one
   source lever would close both. Shapes tried at that sum, each leaving everything
   else byte-identical: D_X[i * 0x24 + K] (parked below), *(u8 *)(D_X + i * 0x24 + K),
   the same with a mem_addr cast, the mem_addr sum grouped as (i * 0x24 + K), an
   embedded assignment (pBase = (mem_addr)D_X) per load, two running byte offsets
   (off24 += 0x24, off18 += 0x18) instead of the strength-reduced bivs, and a
   volatile D_X (that one is worse: 98.03%, 1440 bytes). A 4-minute permuter run
   found nothing (993 iterations, base=300 best=300). So the order is set by loop.c
   when it builds the address giv, not by the source spelling.
   What DID matter, for whoever unparks it: the first clear loop is a BYTE OFFSET
   loop - for (offset = 0x12; offset >= 0; offset -= 6) g_BattleColorTintParams[offset] = 0; -
   and it needs its own local (retail keeps it in $v0, the other three loops use
   $a3); with i * 6 gcc keeps a counter and an offset register and the window is
   five instructions wrong. The func_800A3640 call takes 19 parameters - a0 is
   &g_BattleImage, a1 0, a2 1, a3 0x601 and the fifteen stack words 0x10..0x48 are
   0, D_800D2D30, D_800D2D34, 0, 0, 0, 0, D_800D2D30, D_800D2D34, D_800D2D2C,
   D_800C3EA8, 0, 0, 0, 0 (the s16 ones are passed as promoted shorts, which is
   why each is one sign-extended word in its own slot) - and with that prototype in
   scope that whole call is byte-identical. D_800D360C = 0 belongs between the call
   and the first HeapAlloc; the two 0x88 allocations fill g_BattleColorTintBase (the copy
   destination) and g_BattleColorTintWork. */
/*
void func_800A5EB4(void) {
    u8 *pEntry;
    u8 *pDst;
    s32 offset;
    s32 i;

    for (offset = 0x12; offset >= 0; offset -= 6) {
        g_BattleColorTintParams[offset] = 0;
    }
    pEntry = *(u8 **)(g_BattleImageList + 0x118);
    for (i = 0; i < *(u8 *)(g_BattleImageList + 0x10E); i++) {
        if (*(u16 *)(pEntry + 0x1A) != 0) {
            *(void **)pEntry = g_BattleImage;
        }
        pEntry += 0x30;
    }
    func_800A3640(g_BattleImage, 0, 1, 0x601, 0, D_800D2D30, D_800D2D34, 0, 0, 0, 0, D_800D2D30, D_800D2D34, D_800D2D2C, D_800C3EA8, 0, 0, 0, 0);
    D_800D360C = 0;
    g_BattleColorTintBase = HeapAlloc(0x88, 1);
    g_BattleColorTintWork = HeapAlloc(0x88, 1);
    pDst = g_BattleColorTintBase;
    for (i = 0; i < 4; i++) {
        if (g_BattleSpriteRenderData != NULL) {
            pDst[0] = g_BattleSpriteRenderData[i * 0x24 + 0x1740];
            pDst[1] = g_BattleSpriteRenderData[i * 0x24 + 0x1741];
            pDst[2] = g_BattleSpriteRenderData[i * 0x24 + 0x1742];
            pDst += 4;
            pDst[0] = g_BattleSpriteRenderData[i * 0x24 + 0x1748];
            pDst[1] = g_BattleSpriteRenderData[i * 0x24 + 0x1749];
            pDst[2] = g_BattleSpriteRenderData[i * 0x24 + 0x174A];
            pDst += 4;
            pDst[0] = g_BattleSpriteRenderData[i * 0x24 + 0x1750];
            pDst[1] = g_BattleSpriteRenderData[i * 0x24 + 0x1751];
            pDst[2] = g_BattleSpriteRenderData[i * 0x24 + 0x1752];
            pDst += 4;
            pDst[0] = g_BattleSpriteRenderData[i * 0x24 + 0x1758];
            pDst[1] = g_BattleSpriteRenderData[i * 0x24 + 0x1759];
            pDst[2] = g_BattleSpriteRenderData[i * 0x24 + 0x175A];
            pDst += 4;
            pDst[0] = g_BattleSpriteRenderData[i * 0x18 + 0x16E0];
            pDst[1] = g_BattleSpriteRenderData[i * 0x18 + 0x16E1];
            pDst[2] = g_BattleSpriteRenderData[i * 0x18 + 0x16E2];
            pDst += 4;
        } else {
            pDst += 0x14;
        }
        if (g_BattlePartPrimBuffers[0] != NULL) {
            pDst[0] = g_BattlePartPrimBuffers[0][i * 0x18 + 0x284];
            pDst[1] = g_BattlePartPrimBuffers[0][i * 0x18 + 0x285];
            pDst[2] = g_BattlePartPrimBuffers[0][i * 0x18 + 0x286];
        }
        pDst += 4;
    }
    for (i = 0; i < 2; i++) {
        if (g_BattlePartPrimBuffers[0] != NULL) {
            pDst[0] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2E4];
            pDst[1] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2E5];
            pDst[2] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2E6];
            pDst += 4;
            pDst[0] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2EC];
            pDst[1] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2ED];
            pDst[2] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2EE];
            pDst += 4;
            pDst[0] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2F4];
            pDst[1] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2F5];
            pDst[2] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2F6];
            pDst += 4;
            pDst[0] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2FC];
            pDst[1] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2FD];
            pDst[2] = g_BattlePartPrimBuffers[0][i * 0x24 + 0x2FE];
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
*/

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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A64E4);
/* 98.71% (gcc 2.6.0 --expand-div -G0, the unit preset): the whole function is
   recovered - 232 instructions like retail, the same frame, the same registers
   ($a1 = pSrc, $a0 = pSrc+2, $a2 = i*0x24, $a3 = i*0x18, $t0 = i) and every
   load, store, displacement, null test and biv increment in retail's order.
   The ONLY difference is the operand order of the 18 address addus: retail has
   "addu v0,v0,a2" / "addu v0,v0,a3", gcc emits "addu v0,a2,v0". Semantically
   identical, but the encoding differs so check_bin cannot pass.
   Shapes tried on that sum (each one leaves everything else byte-identical,
   so this is a single isolated tie and a good permuter/unpark candidate): the
   plain array form (parked below), index constant-first (0x1740 + i*0x24), an
   embedded assignment (pDst = ...)[K] and *((pDst = base + off) + K), a
   pDst = g_BattleSpriteRenderData + i*0x24 per store, an integer (mem_addr) sum, and a plain
   offset variable (s32 and u32) incremented in the body instead of a biv - all
   still index-first. The one shape that DOES emit base-first is
   "pDst = &g_BattleSpriteRenderData[i * 0x24 + 0x1740]; *pDst = ...", but it keeps 0x1740 in
   the sum ("addu v0,t1,v0; sb v1,0(v0)") and moves the base to $t1. A 4-minute
   permuter run found nothing (2036 iterations, base=300 best=300).
   try_flags: 2.7.2 -G0 95.59, cdk -G0 79.78, 2.6.0 -O1 46.66, -O3 98.71, so
   the preset is right.
   Structure, for whoever unparks it: pSrc = g_BattleColorTintBase is a table of 24-byte
   records of six 3-byte colours (advance 4 per colour). For i in 0..3: the four
   0x1740/48/50/58 groups go into g_BattleSpriteRenderData + i*0x24, the 0x16E0 group and the
   0x284 group (g_BattlePartPrimBuffers[0]) use i*0x18, and when g_BattleSpriteRenderData is NULL the five
   groups are skipped with pSrc += 0x14. Then a second i in 0..1 loop writes the
   0x2E4/2EC/2F4/2FC groups into g_BattlePartPrimBuffers[0] + i*0x24 (pSrc += 0x10 when it is
   NULL), and the last two colours go to the g_BattleColorSourceA / g_BattleColorSourceB pointers
   set by BattleSetColorSources. Every store reloads its base global, so the
   destination expression has to be written out in full at each store - a
   pointer local collapses the twelve loads into one.
   ROUND 290 re-measure of the operand-association family: the per-store element
   pointer "pAddr = &((u8 *)g_BattleSpriteRenderData)[i * 0x24];" then
   "pAddr[0x1740] = pSrc[0];" is still index-first ("addu a0,a3,v0") and moves
   the base into $a3, and the constant-against-base grouping has no meaning here
   because retail keeps 0x1740 as the store displacement while the sum is only
   base + i*0x24. Still an open tie; the permuter has run twice (2036 and 993
   iterations) without a find. */
/*
void func_800A64E4(void) {
    u8 *pSrc;
    s32 i;

    pSrc = g_BattleColorTintBase;
    for (i = 0; i < 4; i++) {
        if (g_BattleSpriteRenderData != NULL) {
            g_BattleSpriteRenderData[i * 0x24 + 0x1740] = pSrc[0];
            g_BattleSpriteRenderData[i * 0x24 + 0x1741] = pSrc[1];
            g_BattleSpriteRenderData[i * 0x24 + 0x1742] = pSrc[2];
            pSrc += 4;
            g_BattleSpriteRenderData[i * 0x24 + 0x1748] = pSrc[0];
            g_BattleSpriteRenderData[i * 0x24 + 0x1749] = pSrc[1];
            g_BattleSpriteRenderData[i * 0x24 + 0x174A] = pSrc[2];
            pSrc += 4;
            g_BattleSpriteRenderData[i * 0x24 + 0x1750] = pSrc[0];
            g_BattleSpriteRenderData[i * 0x24 + 0x1751] = pSrc[1];
            g_BattleSpriteRenderData[i * 0x24 + 0x1752] = pSrc[2];
            pSrc += 4;
            g_BattleSpriteRenderData[i * 0x24 + 0x1758] = pSrc[0];
            g_BattleSpriteRenderData[i * 0x24 + 0x1759] = pSrc[1];
            g_BattleSpriteRenderData[i * 0x24 + 0x175A] = pSrc[2];
            pSrc += 4;
            g_BattleSpriteRenderData[i * 0x18 + 0x16E0] = pSrc[0];
            g_BattleSpriteRenderData[i * 0x18 + 0x16E1] = pSrc[1];
            g_BattleSpriteRenderData[i * 0x18 + 0x16E2] = pSrc[2];
            pSrc += 4;
        } else {
            pSrc += 0x14;
        }
        if (g_BattlePartPrimBuffers[0] != NULL) {
            g_BattlePartPrimBuffers[0][i * 0x18 + 0x284] = pSrc[0];
            g_BattlePartPrimBuffers[0][i * 0x18 + 0x285] = pSrc[1];
            g_BattlePartPrimBuffers[0][i * 0x18 + 0x286] = pSrc[2];
        }
        pSrc += 4;
    }
    for (i = 0; i < 2; i++) {
        if (g_BattlePartPrimBuffers[0] != NULL) {
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2E4] = pSrc[0];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2E5] = pSrc[1];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2E6] = pSrc[2];
            pSrc += 4;
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2EC] = pSrc[0];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2ED] = pSrc[1];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2EE] = pSrc[2];
            pSrc += 4;
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2F4] = pSrc[0];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2F5] = pSrc[1];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2F6] = pSrc[2];
            pSrc += 4;
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2FC] = pSrc[0];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2FD] = pSrc[1];
            g_BattlePartPrimBuffers[0][i * 0x24 + 0x2FE] = pSrc[2];
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
*/

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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6AE8);
/* 84.16% (gcc 2.6.0 -G0 --expand-div): full structure recovered and every call,
   store and branch shape matches. What is left is loop strength reduction: the
   original keeps g_BattleColorBlendWeight[i*6] in $s7 and derives $s8 = $s7 - 1 for
   g_BattleColorBlendMode[i*6] (combine_givs on two separate symbol bivs), while gcc 2.6 here
   derives every per-unit field from the single i*6 offset biv ($at + %lo), and
   spends a saved register on a g_BattlePartPrimBuffers base instead, so the frame is 0x48 vs
   retail 0x50 and the $s5/$s6 numbering shifts.
   Fixed vs the first attempt (81.99 -> 84.16): the AAD dispatch is a `switch` with
   case 0..3 and case 4 (that is what emits retail's bltz range check plus the
   slti/beq tree -- an if/else chain gives sltiu), g_BattleColorTintRed/AB0/AB1 are `s8` (retail
   loads them with lb), the buffer copy is 0x88 bytes not 0x80 (retail's movstrsi
   loop of 0x80 plus a two-word tail), pData advances 4 bytes per call, the final
   LoadImage source is D_800D3608, and &g_BattleColorTintPending lives in a pointer local so the
   memcpy argument register is reused for the "= 0" store.
   Tried for the bivs: two pointer locals (pKind/pRec -> both fold back to
   $at+s6, 83.73%), pRec[0]/pRec[-1] (84.16%, the parked form), plain symbol
   accesses (82.84%). Also note BattleApplyColorTint is re-declared here as
   (mem_addr, s32, u8 *) and g_BattleColorBlendMode/AAE are u8 while AAF..AB1 are s8.
/*
extern void *g_BattleSpriteRenderData;
extern void *g_BattlePartPrimBuffers[];
extern s16 D_800D2D2C;
extern s16 D_800C3EA8;
extern void *D_800D3608;
extern void BattleApplyColorTint(mem_addr, s32, u8 *);
extern void func_80025D4C(s32, void *, void *, void *, s32, s32, s32, s32, s32);
extern void func_80026F44(s32, s32, void *, void *);

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
}
*/

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
    func_800A64E4();
    HeapFree(g_BattleColorTintBase);
    HeapFree(g_BattleColorTintWork);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7064);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7948);

void BattleFreeModelListEntryBuffers(Misc23Block *pBlock) {
    if (pBlock->pPoints != NULL) {
        HeapFree(pBlock->pPoints);
        HeapFree(*(void **)pBlock->pPackets);
        HeapFree(pBlock->pPackets);
        HeapFree(pBlock->unk20);
        if (pBlock->pRecords != NULL) {
            HeapFree(pBlock->pRecords);
        }
        pBlock->pPoints = NULL;
    }
}

void BattleInitializeStage(void) {
    u8 *pBuf;
    s32 i;
    s32 j;
    mem_addr pWord;
    s32 arg1;

    pBuf = g_BattleBlockPool;
    g_BattleActorStepTimer = 0;
    g_BattleTweenVectorAngle = 0;
    g_BattleExtraMatrixFlag = 0;
    g_BattleActorSwitchFlag = 0;
    g_BattleShadeAngle = 0;
    arg1 = *(s16 *)(g_BattleStageData + 0x348);
    g_BattleStateBit = 1;
    BattleCreateBlockPool(pBuf, arg1);
    BattleCreateSpritePool(g_BattleSpritePool, *(s16 *)(g_BattleStageData + 0x34A));
    BattleClearBlockSlots();
    j = 0x1F;
    pWord = (mem_addr)&g_BattleImageList;
    for (; j >= 0; j--) {
        *(u32 *)pWord = 0;
        pWord -= 4;
    }
    for (i = 0x98; i >= 0; i -= 8) {
        *(u32 *)(D_800C3ACC + i) = 0;
    }
    for (i = 0x14; i >= 0; i -= 0x14) {
        *(u16 *)(D_800D330A + i) = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8BF0);
// Archive entry allocated by func_800A9540 / BattleAllocateArchiveFilePair, stored in g_BattleArchiveEntry.
// A list of {archive file index, buffer that file is decoded into} pairs: the
// file index at 0x0/0x8/0x10/0x18 is the argument ArchiveDecodeAlignedSize and
// func_80029AFC take, so the list is terminated by a zero pair. The 0x18-byte
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

extern Misc23ArchiveEntry *g_BattleArchiveEntry;
extern int ArchiveDecodeAlignedSize(unsigned int);
extern void func_80029AFC(Misc23ArchiveEntry *, s32, s32);
extern u8 g_BattleSlotTextIndex[];
extern u8 D_800CCDE8[];
extern u8 D_800C3509[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9540);
/* 93.51% (376 bytes against retail's 372, unit preset gcc 2.6.0 --expand-div -G0):
   full structure recovered from the disassembly; this round added the one shape the
   earlier 90.14 attempt was missing - the record filler walks the entry with a CURSOR
   local ("pTail = pEntry;" then pTail->fileIndex1/pBuffer1, and pTail =
   &pEntry->fileIndex3 / &pEntry->unk18 for the terminator), which is what produces
   retail's "move s3,s2" copy and keeps it out of the way of the pair written through
   pEntry (fileIndex2/pBuffer2). The last pair's terminator is written through the same
   struct type ("pTail->fileIndex1 = 0; pTail->pBuffer1 = NULL;" -> "sh zero,0(s3)" +
   "sw zero,4(s3)"), not through a u16*.
   Facts kept from the 90.14 note: count is an s32 loaded from a u8 array (lbu), NOT a
   u8 local (that turns the compare into slt and drops an andi); pTable = D_800C3509 is
   assigned BEFORE the if so "lui/addiu s0" comes before the branch and the base lives in
   a callee-saved register across HeapAlloc; the compare itself is written against the
   GLOBAL ("if (D_800C3509[index] < count)") while the later -1 load goes through
   pTable - that opposite-side spelling is worth ~5 points on its own.
   Still open: the frame is 0x30 against retail's 0x38 and gcc 2.6 hands pEntry $s3 and
   pTail $s2 where retail has pEntry in $s2 (declaring them in either order changes
   nothing), so retail has one more live value than this body. Everything else - every
   call, both HeapAlloc sizes, the 0/1 flag arguments, the delay-slot stores - matches. */
/*
   void func_800A9540(s32 arg0) {
       Misc23ArchiveEntry *pEntry;
       Misc23ArchiveEntry *pTail;
       u8 *pTable;
       s32 alignedIndex;
       s32 remainder;
       u32 index;
       s32 nameLen;
       s32 count;
       u8 base;

       ArchiveGetArchiveOffsetIndices(&alignedIndex, &remainder);
       ArchiveSetIndex(0x28, 1);
       HeapChangeCurrentUser(4, NULL);
       index = g_BattleSlotTextIndex[arg0 * 0x170] * 2;
       count = D_800CCDE8[arg0 * 0x170];
       pTable = D_800C3509;
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
       pEntry->pBuffer2 = HeapAlloc(ArchiveDecodeAlignedSize(nameLen), 0);
       pTail = &pEntry->fileIndex3;
       if (count != 0) {
           nameLen += count;
           pEntry->fileIndex3 = nameLen;
           pEntry->pBuffer3 = (s32)HeapAlloc(ArchiveDecodeAlignedSize(nameLen), 1);
           pTail = (Misc23ArchiveEntry *)&pEntry->unk18;
       }
       pTail->fileIndex1 = 0;
       pTail->pBuffer1 = NULL;
       func_80029AFC(g_BattleArchiveEntry, 0, 0);
       ArchiveSetIndex(alignedIndex, remainder);
   }
*/
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
    func_80029AFC(g_BattleArchiveEntry, 0, 0);
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
extern void func_8002DDE4(void *, s32, s32, s32, s32, s32, s32);
extern void func_800A8BF0(s32, u16, void *, void *, s32, s32, s32, s32, s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A979C);
/* 85.47% (gcc 2.6.0, the unit preset, -G0 --expand-div). Shape is right -
   every call, every store and the loop are in retail order; what is left is
   allocator: retail keeps arg0 in $s7 / arg2 in $s6 / arg3 in $s2 (reused as
   the loop counter) / pData in $s4 / count in $s3 and holds the raw 5th
   argument in $s8, gcc picks arg0 $s8 / arg2 $s7 / arg3 $s3 / pData $s2 /
   count $s4, which spills pData into a 0x38 slot (frame 0x70 vs retail 0x68)
   and re-sign-extends the 5th argument from its halfword spill instead of
   $s8. Also gcc folds the loop index constant into the table displacement
   ("addiu v0,v0,52" off %hi(g_BattleActors) vs retail's "sll/addu" of 0xD at
   runtime). Tried: if/else both senses (the count == 0 sense costs 12 points
   and an extra slot: 73.72 vs 85.47), repeating the index expression inline
   at all seven uses instead of a local (that is what gained 85.47 - a "slot"
   local made gcc hoist arg0 << 1 and spill it), declaration order reordered
   to retail's register order (85.47 unchanged).
/*
void func_800A979C(s32 arg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4) {
    u8 *pSrc;
    s32 i;
    s32 count;
    Misc23ArchiveData *pData;
    u8 *pBuf;
    s32 size;
    s32 flags;

    func_800A8BF0(arg0, 0, g_BattleArchiveEntry->pBuffer2, g_BattleArchiveEntry->pBuffer1, arg1, arg2, arg3, arg4, 0);
    g_BattleActors[arg0][0x38] = 1;
    g_BattleActors[arg0][0x22] = 1;
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
                flags = (i != 0) ? 7 : 2;
                if (i == count - 1) {
                    flags -= 2;
                }
                func_800A8BF0((arg0 << 1) + (i + 0xD), flags, pBuf, pData->pBlockEnd, arg1, arg2, arg3, arg4, 0);
                *(u16 *)(g_BattleActors[(arg0 << 1) + (i + 0xD)] + 0x5E) = *(u16 *)pSrc;
                g_BattleActors[(arg0 << 1) + (i + 0xD)][0x5C] = arg0;
                g_BattleActors[(arg0 << 1) + (i + 0xD)][0x5D] = 2;
                pSrc += 2;
                *(u16 *)(g_BattleActors[(arg0 << 1) + (i + 0xD)] + 0x6A) = *(u16 *)pSrc;
                pSrc += 2;
                *(u16 *)(g_BattleActors[(arg0 << 1) + (i + 0xD)] + 0x6C) = *(u16 *)pSrc;
                pSrc += 2;
                *(u16 *)(g_BattleActors[(arg0 << 1) + (i + 0xD)] + 0x6E) = *(u16 *)pSrc;
                pSrc += 2;
            }
        } else {
            func_8002DDE4(pData->pBlockStart, 1, arg1, arg2, 1, arg3, arg4);
        }
        HeapFree((void *)g_BattleArchiveEntry->pBuffer3);
    }
    HeapFree(g_BattleArchiveEntry);
    DrawSync(0);
    HeapFree(g_BattleArchiveEntry->pBuffer1);
}
*/
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
    colors[0] = BattleClampScaledByte(g_BattleShadeScale, 0x20, *(u8 *)(g_BattleStageData + 0x474));
    colors[1] = BattleClampScaledByte(g_BattleShadeScale, 0x20, *(u8 *)(g_BattleStageData + 0x475));
    colors[2] = BattleClampScaledByte(g_BattleShadeScale, 0x20, *(u8 *)(g_BattleStageData + 0x476));
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
                    SetBackColor(*(u8 *)(g_BattleStageData + 0x474), *(u8 *)(g_BattleStageData + 0x475),
                                 *(u8 *)(g_BattleStageData + 0x476));
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
/* 99.90% (816 bytes, unit preset gcc 2.6.0 --expand-div -G0): whole body recovered,
   retail's frame (0x48) and saved set ($s0-$s5) exact, every call/load/store and both
   loop rotations identical. 91.39 -> 99.90 came from the FIRST HALF's address shape:
   retail keeps &g_BattleActors[arg0] in $s1 and reloads *($s1) at every use, so the
   slot pointer is a local, and it is built in TWO statements -
   "ppActors = g_BattleActors; ppSlot = &ppActors[arg0];" - which is what emits
   "lui/addiu %hi(g_BattleActors)" BEFORE the "sll s4,2". The one-statement form
   "ppSlot = &g_BattleActors[arg0];" emits sll first (98.80), and indexing the global
   inline everywhere (the first attempt) is 91.39. The second half (0x10C up) reaches
   g_BattleActors[arg0] inline instead: that split is retail's own.
   Only difference left: retail loads the *pSprites null-test operand into $a1 in BOTH
   arms ("lw v0,0(s1); lw a1,4(v0); beqz a1") and cc1 uses $v0 ("lw v0,4(v0); beqz v0")
   - same size, same instruction count, a register tie-break on a load that only feeds
   a compare. Measured and no help: embedded assignment in the test (99.90), a pSprites
   local assigned before the range test (95.68, 792 bytes - gcc hoists the load), -O3
   (82.98), and a 1745-iteration permuter run (base=20 best=20, PERMUTER_NONE). */
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
        if ((*ppSlot)->unk62 != 0) {
            SoundRemoveSedsEntry(*(void **)((*ppSlot)->pSoundBank1 + 8));
        }
        if ((*ppSlot)->unkAC != NULL) {
            HeapFree((void *)(*ppSlot)->unkAC);
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
