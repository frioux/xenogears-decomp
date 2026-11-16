#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

extern u8 *g_BattleColorSourceA;
extern u8 *g_BattleColorSourceB;
extern u8 *D_800C3EA0;
extern u8 *D_800C3D50[2];
extern u8 g_BattleSpritePool[];
extern u8 g_BattleBlockPool[];
extern u16 g_BattleCurrentActorSlot;
extern u16 g_BattleCommandActorMask;
extern u8 D_800D3611;
extern u8 *g_BattleActors[];

extern void BattleFreeBlockPool(u8 *);
extern void BattleFreeSpritePool(u8 *);
extern void func_800A9FF0(s32);
extern void BattleAppendActorCommandBytes(void *, void *, void *, void *);
extern s32 BattleFindFirstActorInMask(void);
extern void BattleFreeImageEntry(void *);
extern void func_800A64E4(void);
extern void func_800BF85C(s32, s32);
extern void func_800A3640(void *, s32, s32, s16, s32, s16, s16, s32, s32, s32, s32, s16, s16, s16, s16, s32, s32, s32, s32);

extern u8 *D_800D33E4;
extern u16 D_800D361A;
extern u_long *D_800D3604;
extern RECT D_800D3628;
extern u8 *g_BattleColorTintBase;
extern void *g_BattleColorTintWork;
extern u8 D_800C3D68;
extern u8 g_FlagTimerWorkListAdds;
extern s32 g_NumFlaggedTimerWorkListEntries;
extern s32 g_BattleCommandRowIndex;
extern u8 D_800C4000[];

// Per-unit 6-byte state records, unit id 0..3; the original spells each byte
// as its own symbol.
extern u8 g_BattleColorTintParams[];
extern u8 D_800C3AAD[];
extern u8 D_800C3AAE[];
extern u8 D_800C3AAF[];
extern u8 D_800C3AB0[];
extern u8 D_800C3AB1[];

// Allocation block owned by the structure passed to BattleFreeModelListEntryBuffers.
typedef struct {
    /* 0x0  */ u8 unk0[0x14];
    /* 0x14 */ void *unk14;
    /* 0x18 */ void *unk18;
    /* 0x1C */ void *unk1C;
    /* 0x20 */ void *unk20;
} Misc23Block;

// 0xE-byte node of the tree rooted at g_BattleTriangles, indexed by g_BattleTriangleCount.
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 unkD;
} Misc23TreeEntry; // Size: 0xE

extern s32 D_800C3E88;
extern u16 D_800C3CF0;
extern u8 g_BattleExtraMatrixFlag;
extern u16 D_800C3B7C;
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
        ((Misc23TreeEntry *)(i * 14 + g_BattleTriangles))->unkD = 0;
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
    if (((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unkD != g_BattleTriangleVisitStamp) {
        ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unkD = g_BattleTriangleVisitStamp;
        result = BattleTestPointInTriangle((void *)(((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unk0 * 8 + g_BattleTriangleVertices),
                               (void *)(((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unk2 * 8 + g_BattleTriangleVertices),
                               (void *)(((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unk4 * 8 + g_BattleTriangleVertices), arg0);
        if (result == -1) {
            return index;
        }
    }
    if (depth <= 0) {
        return -1;
    }
    result = BattleSearchTriangleTree(arg0, ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unk6, depth - 1);
    if (result >= 0) {
        return result;
    }
    res2 = BattleSearchTriangleTree(arg0, ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unk8, depth - 1);
    if (res2 >= 0) {
        return res2;
    }
    res3 = BattleSearchTriangleTree(arg0, ((Misc23TreeEntry *)(index * 14 + g_BattleTriangles))->unkA, depth - 1);
    if (res3 < 0) {
        res3 = -1;
    }
    return res3;
}

void BattleSetColorSources(s32 arg0, s32 arg1) {
    g_BattleColorSourceA = (u8 *)arg0;
    g_BattleColorSourceB = (u8 *)arg1;
}

extern u8 D_800D3600[];
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
   &D_800D3600, a1 0, a2 1, a3 0x601 and the fifteen stack words 0x10..0x48 are
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
    pEntry = *(u8 **)(D_800D33E4 + 0x118);
    for (i = 0; i < *(u8 *)(D_800D33E4 + 0x10E); i++) {
        if (*(u16 *)(pEntry + 0x1A) != 0) {
            *(void **)pEntry = D_800D3600;
        }
        pEntry += 0x30;
    }
    func_800A3640(D_800D3600, 0, 1, 0x601, 0, D_800D2D30, D_800D2D34, 0, 0, 0, 0, D_800D2D30, D_800D2D34, D_800D2D2C, D_800C3EA8, 0, 0, 0, 0);
    D_800D360C = 0;
    g_BattleColorTintBase = HeapAlloc(0x88, 1);
    g_BattleColorTintWork = HeapAlloc(0x88, 1);
    pDst = g_BattleColorTintBase;
    for (i = 0; i < 4; i++) {
        if (D_800C3EA0 != NULL) {
            pDst[0] = D_800C3EA0[i * 0x24 + 0x1740];
            pDst[1] = D_800C3EA0[i * 0x24 + 0x1741];
            pDst[2] = D_800C3EA0[i * 0x24 + 0x1742];
            pDst += 4;
            pDst[0] = D_800C3EA0[i * 0x24 + 0x1748];
            pDst[1] = D_800C3EA0[i * 0x24 + 0x1749];
            pDst[2] = D_800C3EA0[i * 0x24 + 0x174A];
            pDst += 4;
            pDst[0] = D_800C3EA0[i * 0x24 + 0x1750];
            pDst[1] = D_800C3EA0[i * 0x24 + 0x1751];
            pDst[2] = D_800C3EA0[i * 0x24 + 0x1752];
            pDst += 4;
            pDst[0] = D_800C3EA0[i * 0x24 + 0x1758];
            pDst[1] = D_800C3EA0[i * 0x24 + 0x1759];
            pDst[2] = D_800C3EA0[i * 0x24 + 0x175A];
            pDst += 4;
            pDst[0] = D_800C3EA0[i * 0x18 + 0x16E0];
            pDst[1] = D_800C3EA0[i * 0x18 + 0x16E1];
            pDst[2] = D_800C3EA0[i * 0x18 + 0x16E2];
            pDst += 4;
        } else {
            pDst += 0x14;
        }
        if (D_800C3D50[0] != NULL) {
            pDst[0] = D_800C3D50[0][i * 0x18 + 0x284];
            pDst[1] = D_800C3D50[0][i * 0x18 + 0x285];
            pDst[2] = D_800C3D50[0][i * 0x18 + 0x286];
        }
        pDst += 4;
    }
    for (i = 0; i < 2; i++) {
        if (D_800C3D50[0] != NULL) {
            pDst[0] = D_800C3D50[0][i * 0x24 + 0x2E4];
            pDst[1] = D_800C3D50[0][i * 0x24 + 0x2E5];
            pDst[2] = D_800C3D50[0][i * 0x24 + 0x2E6];
            pDst += 4;
            pDst[0] = D_800C3D50[0][i * 0x24 + 0x2EC];
            pDst[1] = D_800C3D50[0][i * 0x24 + 0x2ED];
            pDst[2] = D_800C3D50[0][i * 0x24 + 0x2EE];
            pDst += 4;
            pDst[0] = D_800C3D50[0][i * 0x24 + 0x2F4];
            pDst[1] = D_800C3D50[0][i * 0x24 + 0x2F5];
            pDst[2] = D_800C3D50[0][i * 0x24 + 0x2F6];
            pDst += 4;
            pDst[0] = D_800C3D50[0][i * 0x24 + 0x2FC];
            pDst[1] = D_800C3D50[0][i * 0x24 + 0x2FD];
            pDst[2] = D_800C3D50[0][i * 0x24 + 0x2FE];
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
            D_800D3611 = 1;
            g_BattleColorTintParams[arg0 * 6] = 1;
            D_800C3AAD[arg0 * 6] = arg1;
            D_800C3AAE[arg0 * 6] = arg2;
            D_800C3AAF[arg0 * 6] = arg3;
            D_800C3AB0[arg0 * 6] = arg4;
            D_800C3AB1[arg0 * 6] = arg5;
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
   pDst = D_800C3EA0 + i*0x24 per store, an integer (mem_addr) sum, and a plain
   offset variable (s32 and u32) incremented in the body instead of a biv - all
   still index-first. The one shape that DOES emit base-first is
   "pDst = &D_800C3EA0[i * 0x24 + 0x1740]; *pDst = ...", but it keeps 0x1740 in
   the sum ("addu v0,t1,v0; sb v1,0(v0)") and moves the base to $t1. A 4-minute
   permuter run found nothing (2036 iterations, base=300 best=300).
   try_flags: 2.7.2 -G0 95.59, cdk -G0 79.78, 2.6.0 -O1 46.66, -O3 98.71, so
   the preset is right.
   Structure, for whoever unparks it: pSrc = g_BattleColorTintBase is a table of 24-byte
   records of six 3-byte colours (advance 4 per colour). For i in 0..3: the four
   0x1740/48/50/58 groups go into D_800C3EA0 + i*0x24, the 0x16E0 group and the
   0x284 group (D_800C3D50[0]) use i*0x18, and when D_800C3EA0 is NULL the five
   groups are skipped with pSrc += 0x14. Then a second i in 0..1 loop writes the
   0x2E4/2EC/2F4/2FC groups into D_800C3D50[0] + i*0x24 (pSrc += 0x10 when it is
   NULL), and the last two colours go to the g_BattleColorSourceA / g_BattleColorSourceB pointers
   set by BattleSetColorSources. Every store reloads its base global, so the
   destination expression has to be written out in full at each store - a
   pointer local collapses the twelve loads into one. */
/*
void func_800A64E4(void) {
    u8 *pSrc;
    s32 i;

    pSrc = g_BattleColorTintBase;
    for (i = 0; i < 4; i++) {
        if (D_800C3EA0 != NULL) {
            D_800C3EA0[i * 0x24 + 0x1740] = pSrc[0];
            D_800C3EA0[i * 0x24 + 0x1741] = pSrc[1];
            D_800C3EA0[i * 0x24 + 0x1742] = pSrc[2];
            pSrc += 4;
            D_800C3EA0[i * 0x24 + 0x1748] = pSrc[0];
            D_800C3EA0[i * 0x24 + 0x1749] = pSrc[1];
            D_800C3EA0[i * 0x24 + 0x174A] = pSrc[2];
            pSrc += 4;
            D_800C3EA0[i * 0x24 + 0x1750] = pSrc[0];
            D_800C3EA0[i * 0x24 + 0x1751] = pSrc[1];
            D_800C3EA0[i * 0x24 + 0x1752] = pSrc[2];
            pSrc += 4;
            D_800C3EA0[i * 0x24 + 0x1758] = pSrc[0];
            D_800C3EA0[i * 0x24 + 0x1759] = pSrc[1];
            D_800C3EA0[i * 0x24 + 0x175A] = pSrc[2];
            pSrc += 4;
            D_800C3EA0[i * 0x18 + 0x16E0] = pSrc[0];
            D_800C3EA0[i * 0x18 + 0x16E1] = pSrc[1];
            D_800C3EA0[i * 0x18 + 0x16E2] = pSrc[2];
            pSrc += 4;
        } else {
            pSrc += 0x14;
        }
        if (D_800C3D50[0] != NULL) {
            D_800C3D50[0][i * 0x18 + 0x284] = pSrc[0];
            D_800C3D50[0][i * 0x18 + 0x285] = pSrc[1];
            D_800C3D50[0][i * 0x18 + 0x286] = pSrc[2];
        }
        pSrc += 4;
    }
    for (i = 0; i < 2; i++) {
        if (D_800C3D50[0] != NULL) {
            D_800C3D50[0][i * 0x24 + 0x2E4] = pSrc[0];
            D_800C3D50[0][i * 0x24 + 0x2E5] = pSrc[1];
            D_800C3D50[0][i * 0x24 + 0x2E6] = pSrc[2];
            pSrc += 4;
            D_800C3D50[0][i * 0x24 + 0x2EC] = pSrc[0];
            D_800C3D50[0][i * 0x24 + 0x2ED] = pSrc[1];
            D_800C3D50[0][i * 0x24 + 0x2EE] = pSrc[2];
            pSrc += 4;
            D_800C3D50[0][i * 0x24 + 0x2F4] = pSrc[0];
            D_800C3D50[0][i * 0x24 + 0x2F5] = pSrc[1];
            D_800C3D50[0][i * 0x24 + 0x2F6] = pSrc[2];
            pSrc += 4;
            D_800C3D50[0][i * 0x24 + 0x2FC] = pSrc[0];
            D_800C3D50[0][i * 0x24 + 0x2FD] = pSrc[1];
            D_800C3D50[0][i * 0x24 + 0x2FE] = pSrc[2];
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6884);
/* 99.74%: the switch and every instruction match; the function is 612 bytes
   like retail and the ONLY difference is the table operand (original names
   jtbl_80070554, ours says .rodata). DO NOT CONVERT: the stub's .s owns the
   unit's whole .rodata range (0xA64..0xA78 = 6 words, five case entries plus a
   trailing 0), and a C switch emits only the five entries, so the overlay comes
   out 4 bytes short and check_bin fails (285k bytes differ). unk3/unk4/unk5 are
   read with `lb` (s8 * 8), unk0 is the enable flag, unk1 the blend mode and
   unk2 the weight/32. The array spelling D_800C3AAx[index * 6] is retail's own:
   a struct-pointer local folds the six loads into one base + displacements and
   drops the per-field lui/addu pair.
void func_800A6884(u8 *pDst, s32 index, u8 *pSrc) {
    s32 r;
    s32 g;
    s32 b;
    s32 t;
    s32 k;

    r = ((s8 *)D_800C3AAF)[index * 6] * 8;
    g = ((s8 *)D_800C3AB0)[index * 6] * 8;
    b = ((s8 *)D_800C3AB1)[index * 6] * 8;
    if (g_BattleColorTintParams[index * 6] != 0) {
        switch (D_800C3AAD[index * 6]) {
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
    k = D_800C3AAE[index * 6];
    t = pSrc[0] - r;
    r += t * k / 32;
    t = pSrc[1] - g;
    g += t * k / 32;
    t = pSrc[2] - b;
    b += t * k / 32;
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6AE8);
/* 84.16% (gcc 2.6.0 -G0 --expand-div): full structure recovered and every call,
   store and branch shape matches. What is left is loop strength reduction: the
   original keeps D_800C3AAE[i*6] in $s7 and derives $s8 = $s7 - 1 for
   D_800C3AAD[i*6] (combine_givs on two separate symbol bivs), while gcc 2.6 here
   derives every per-unit field from the single i*6 offset biv ($at + %lo), and
   spends a saved register on a D_800C3D50 base instead, so the frame is 0x48 vs
   retail 0x50 and the $s5/$s6 numbering shifts.
   Fixed vs the first attempt (81.99 -> 84.16): the AAD dispatch is a `switch` with
   case 0..3 and case 4 (that is what emits retail's bltz range check plus the
   slti/beq tree -- an if/else chain gives sltiu), D_800C3AAF/AB0/AB1 are `s8` (retail
   loads them with lb), the buffer copy is 0x88 bytes not 0x80 (retail's movstrsi
   loop of 0x80 plus a two-word tail), pData advances 4 bytes per call, the final
   LoadImage source is D_800D3608, and &D_800D3611 lives in a pointer local so the
   memcpy argument register is reused for the "= 0" store.
   Tried for the bivs: two pointer locals (pKind/pRec -> both fold back to
   $at+s6, 83.73%), pRec[0]/pRec[-1] (84.16%, the parked form), plain symbol
   accesses (82.84%). Also note func_800A6884 is re-declared here as
   (mem_addr, s32, u8 *) and D_800C3AAD/AAE are u8 while AAF..AB1 are s8.
/*
extern void *D_800C3EA0;
extern void *D_800C3D50[];
extern s16 D_800D2D2C;
extern s16 D_800C3EA8;
extern void *D_800D3608;
extern void func_800A6884(mem_addr, s32, u8 *);
extern void func_80025D4C(s32, void *, void *, void *, s32, s32, s32, s32, s32);
extern void func_80026F44(s32, s32, void *, void *);

void func_800A6AE8(void) {
    u8 *pFlag;
    u8 *pData;
    s32 i;
    s32 j;

    pFlag = &D_800D3611;
    if (*pFlag == 0) {
        return;
    }
    if (D_800D361A == 0) {
        return;
    }
    *pFlag = 0;
    memcpy(D_800D3608, D_800D3604, D_800D2D2C * D_800C3EA8 * 2);
    memcpy(g_BattleColorTintWork, g_BattleColorTintBase, 0x88);
    for (i = 0; i < 4; i++) {
        u8 *pRec;

        pRec = &D_800C3AAE[i * 6];
        if (g_BattleColorTintParams[i * 6] == 0) {
            continue;
        }
        switch (pRec[-1]) {
        case 0:
        case 1:
        case 2:
        case 3:
            func_80025D4C(D_800D2D2C * D_800C3EA8, D_800D3608, D_800D3608, D_800D3608,
                          D_800C3AAF[i * 6], D_800C3AB0[i * 6], D_800C3AB1[i * 6],
                          pRec[-1], pRec[0]);
            break;
        case 4:
            func_80026F44(D_800D2D2C * D_800C3EA8, pRec[0], D_800D3608, D_800D3604);
            break;
        }
        pData = g_BattleColorTintWork;
        for (j = 0; j < 4; j++) {
            if (D_800C3EA0 != NULL) {
                func_800A6884((mem_addr)D_800C3EA0 + 0x173C + j * 0x24 + 4, i, pData += 4);
                func_800A6884((mem_addr)D_800C3EA0 + 0x173C + j * 0x24 + 0xC, i, pData += 4);
                func_800A6884((mem_addr)D_800C3EA0 + 0x173C + j * 0x24 + 0x14, i, pData += 4);
                func_800A6884((mem_addr)D_800C3EA0 + 0x173C + j * 0x24 + 0x1C, i, pData += 4);
                func_800A6884((mem_addr)D_800C3EA0 + 0x16DC + j * 0x18 + 4, i, pData += 4);
            } else {
                pData += 0x14;
            }
            if (D_800C3D50[0] != NULL) {
                func_800A6884((mem_addr)D_800C3D50[0] + 0x280 + j * 0x18 + 4, i, pData += 4);
            } else {
                pData += 4;
            }
        }
        for (j = 0; j < 2; j++) {
            if (D_800C3D50[0] != NULL) {
                func_800A6884((mem_addr)D_800C3D50[0] + 0x2E0 + j * 0x24 + 4, i, pData += 4);
                func_800A6884((mem_addr)D_800C3D50[0] + 0x2E0 + j * 0x24 + 0xC, i, pData += 4);
                func_800A6884((mem_addr)D_800C3D50[0] + 0x2E0 + j * 0x24 + 0x14, i, pData += 4);
                func_800A6884((mem_addr)D_800C3D50[0] + 0x2E0 + j * 0x24 + 0x1C, i, pData += 4);
            } else {
                pData += 0x10;
            }
        }
        func_800A6884(g_BattleColorSourceA, i, pData += 4);
        func_800A6884(g_BattleColorSourceB, i, pData);
    }
    LoadImage(&D_800D3628, D_800D3608);
}
*/

void BattleUploadAndFreeImageResources(void) {
    u8 *p;
    u8 *pRect;
    s32 i;

    if (D_800D361A != 0) {
        p = *(u8 **)(D_800D33E4 + 0x118);
        for (i = 0; i < *(u8 *)(D_800D33E4 + 0x10E); i++) {
            if (*(u16 *)(p + 0x1A) != 0) {
                *(s32 *)p = 0;
            }
            p += 0x30;
        }
        pRect = (u8 *)&D_800D3628;
        LoadImage((RECT *)pRect, D_800D3604);
        DrawSync(0);
        BattleFreeImageEntry(pRect - 0x28);
    }
    func_800A64E4();
    HeapFree(g_BattleColorTintBase);
    HeapFree(g_BattleColorTintWork);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7064);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7948);

void BattleFreeModelListEntryBuffers(Misc23Block *pBlock) {
    if (pBlock->unk14 != NULL) {
        HeapFree(pBlock->unk14);
        HeapFree(*(void **)pBlock->unk1C);
        HeapFree(pBlock->unk1C);
        HeapFree(pBlock->unk20);
        if (pBlock->unk18 != NULL) {
            HeapFree(pBlock->unk18);
        }
        pBlock->unk14 = NULL;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8B0C);
/* 99.12% (gcc 2.6.0, the unit preset): only the first clear loop's two live
   values are swapped - the original has the counter in $v1 and the pointer in
   $v0, gcc gives the counter $v0 and the pointer $v1, so "bgez" and the delay
   slot follow. Everything else is byte-identical. What fixed the rest: the call
   argument is an s32 local loaded before the "= 1" store (an s16 local makes gcc
   emit lhu + sll/sra). Tried for the loop: pointer/counter source order both
   ways, for-init vs separate statement, mem_addr + *(u32 *)pWord vs
   u32 *pWord--, declaration order both ways, and a 4-minute permuter run
   (1497 iterations, base=50 best=50, no find). Allocator tie-break class.
    u8 *pBuf;
    s32 i;
    mem_addr pWord;
    s32 arg1;

    pBuf = g_BattleBlockPool;
    D_800C3E88 = 0;
    D_800C3CF0 = 0;
    g_BattleExtraMatrixFlag = 0;
    D_800C3D68 = 0;
    D_800C3B7C = 0;
    arg1 = *(s16 *)(g_BattleStageData + 0x348);
    g_BattleStateBit = 1;
    BattleCreateBlockPool(pBuf, arg1);
    BattleCreateSpritePool(g_BattleSpritePool, *(s16 *)(g_BattleStageData + 0x34A));
    BattleClearBlockSlots();
    i = 0x1F;
    pWord = (mem_addr)&D_800D33E4;
    for (; i >= 0; i--) {
        *(u32 *)pWord = 0;
        pWord -= 4;
    }
    for (i = 0x98; i >= 0; i -= 8) {
        *(u32 *)(D_800C3ACC + i) = 0;
    }
    for (i = 0x14; i >= 0; i -= 0x14) {
        *(u16 *)(D_800D330A + i) = 0;
    }
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8BF0);
// Archive entry allocated by func_800A9540 / BattleAllocateArchiveFilePair, stored in g_BattleArchiveEntry.
// The 0x18-byte variant stops after unk14; the 0x20-byte one adds a second
// {u16, s32} pair at 0x18.
typedef struct {
    /* 0x0  */ u16 unk0;
    /* 0x2  */ u16 pad2;
    /* 0x4  */ void *unk4;
    /* 0x8  */ u16 unk8;
    /* 0xA  */ u16 padA;
    /* 0xC  */ void *unkC;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 pad12;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 pad1A;
    /* 0x1C */ s32 unk1C;
} Misc23ArchiveEntry; // Size: 0x20

extern Misc23ArchiveEntry *g_BattleArchiveEntry;
extern int ArchiveDecodeAlignedSize(unsigned int);
extern void func_80029AFC(Misc23ArchiveEntry *, s32, s32);
extern u8 D_800CCD88[];
extern u8 D_800CCDE8[];
extern u8 D_800C3509[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9540);
/* 90.14% (under the unit's gcc 2.6.0 --expand-div -G0; was 77.73 when this was
   first attempted under -G8). Fixed by this shape vs the old attempt:
   - count is an s32 loaded from a u8 array (lbu), NOT a u8 local: that turns
     the compare into "slt" (was sltu) and removes the "andi 0xff" before both
     "beqz count" and "addu nameLen,count".
   - pTable = D_800C3509 assigned BEFORE the if, so "lui/addiu s0" is emitted
     before the branch and &D_800C3509 lives in a callee-saved register across
     HeapAlloc ("addu v0,s0,s1; lbu s0,-1(v0)"); with the assignment after the
     call gcc 2.6 folds the -1 into %lo(D_800C3509-1) and uses $at.
   A 4-minute permuter run found base=1055 best=356 (PERMUTER_BETTER) on two
   changes: "nameLen = 2; nameLen = base + nameLen;" for the second name length,
   and moving func_80029AFC(g_BattleArchiveEntry, 0, 0) BEFORE the two pTail zero stores
   (that one is semantically wrong - retail has the sh before the jal and the sw
   in its delay slot - so port only the nameLen split and re-run the permuter
   from there, up to three times while the score falls).
   Remaining differences: (a) the original has an extra "move s3,s2" (a second
   register holding pEntry, used for the unk0/unk4 stores while s2 keeps the
   later ones) - a pHead = (u8 *)pEntry alias is CSE'd back into pEntry, so gcc
   2.6 will not produce the copy; (b) that costs $s3 to pTail and pushes count
   into $s4 (mine has count in $s3, pTail in $v1); (c) my index gets a "move"
   copy out of the sll while the original shifts straight into $s1. Structurally
   complete otherwise: every instruction and operand order matches.
void func_800A9540(s32 arg0) {
    Misc23ArchiveEntry *pEntry;
    u8 *pTable;
    u16 *pTail;
    s32 alignedIndex;
    s32 remainder;
    u32 index;
    s32 nameLen;
    s32 count;
    u8 base;

    ArchiveGetArchiveOffsetIndices(&alignedIndex, &remainder);
    ArchiveSetIndex(0x28, 1);
    HeapChangeCurrentUser(4, NULL);
    index = D_800CCD88[arg0 * 0x170] * 2;
    count = D_800CCDE8[arg0 * 0x170];
    pTable = D_800C3509;
    if (D_800C3509[index] < count) {
        count = 0;
    }
    pEntry = HeapAlloc(0x20, 1);
    g_BattleArchiveEntry = pEntry;
    base = pTable[index - 1];
    nameLen = base + 1;
    pEntry->unk0 = nameLen;
    pEntry->unk4 = HeapAlloc(ArchiveDecodeAlignedSize(nameLen), 1);
    nameLen = base + 2;
    pEntry->unk8 = nameLen;
    pEntry->unkC = HeapAlloc(ArchiveDecodeAlignedSize(nameLen), 0);
    pTail = &pEntry->unk10;
    if (count != 0) {
        nameLen = nameLen + count;
        pEntry->unk10 = nameLen;
        pEntry->unk14 = HeapAlloc(ArchiveDecodeAlignedSize(nameLen), 1);
        pTail = &pEntry->unk18;
    }
    pTail[0] = 0;
    *(s32 *)(pTail + 2) = 0;
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
    pEntry->unk0 = entryIndex;
    pEntry->unk4 = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 1);
    entryIndex = arg0 + 2;
    pEntry->unk8 = entryIndex;
    pEntry->unkC = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 1);
    pEntry->unk10 = 0;
    pEntry->unk14 = 0;
    func_80029AFC(g_BattleArchiveEntry, 0, 0);
    ArchiveSetIndex(alignedIndex, remainder);
}
// Payload reached through Misc23ArchiveEntry::unk14: a count-prefixed list of
// 16-bit entries plus the pixel block bounded by unk8..unkC.
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ u8 *unk4;
    /* 0x8 */ u8 *unk8;
    /* 0xC */ u8 *unkC;
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

    func_800A8BF0(arg0, 0, g_BattleArchiveEntry->unkC, g_BattleArchiveEntry->unk4, arg1, arg2, arg3, arg4, 0);
    g_BattleActors[arg0][0x38] = 1;
    g_BattleActors[arg0][0x22] = 1;
    if (g_BattleArchiveEntry->unk14 != 0) {
        pData = (Misc23ArchiveData *)g_BattleArchiveEntry->unk14;
        ResolveArchiveEntryPointers((u32 *)pData);
        pSrc = pData->unk4;
        count = *(s16 *)pSrc;
        if (count != 0) {
            pSrc += 4;
            size = pData->unkC - pData->unk8;
            pBuf = HeapAlloc(size, 0);
            memcpy(pBuf, pData->unk8, size);
            for (i = 0; i < count; i++) {
                flags = (i != 0) ? 7 : 2;
                if (i == count - 1) {
                    flags -= 2;
                }
                func_800A8BF0((arg0 << 1) + (i + 0xD), flags, pBuf, pData->unkC, arg1, arg2, arg3, arg4, 0);
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
            func_8002DDE4(pData->unk8, 1, arg1, arg2, 1, arg3, arg4);
        }
        HeapFree((void *)g_BattleArchiveEntry->unk14);
    }
    HeapFree(g_BattleArchiveEntry);
    DrawSync(0);
    HeapFree(g_BattleArchiveEntry->unk4);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9A50);

void BattleFreeAllModelSlots(void) {
    s32 i;

    for (i = 0; i < 0x1F; i++) {
        func_800A9FF0(i);
    }
    BattleFreeBlockPool(g_BattleBlockPool);
    BattleFreeSpritePool(g_BattleSpritePool);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9FF0);
/* 91.39%: first attempt on this stub. Shape recovered from the disassembly:
   arg0 is an index into g_BattleActors; the body is a null-checked chain of
   HeapFree/BattleFreeModelMesh/SoundRemoveSedsEntry/BattleFreeAllSpriteBlocks+BattleFreeSpriteBlocksByTag on fields
   0x00,0x04,0xA8,0xAC,0xB0 of the unit, then the 0x13..0x1E range test
   "(u32)(arg0 - 0x13) < 0xC", then two counted loops (numTextures * 0x30 through
   pTextures with BattleFreeImageEntry, numModelListEntries * 0x24 through
   pModelListEntries with BattleFreeModelListEntryBuffers),
   then HeapFree(unit)/unit = NULL, and finally the recursion into units
   arg0*2+0xD and arg0*2+0xE when arg0 < 3.
   What moved 82.96 -> 91.39: do NOT keep &g_BattleActors[arg0] in a local - retail
   re-forms the slot address (lui/addiu + sll/addiu) at every use, so index
   g_BattleActors[arg0] inline at each access. Still open: retail saves $s5 and a
   0x48 frame (mine 0x38), keeps the loop pointer in $s2/$s0 pairs, and
   re-emits "addiu v0,s4,-0x13" both in the beqz delay slot and after the
   HeapFree call, so the range test and the two loop induction variables are
   not yet the retail shape. */
/*
void func_800A9FF0(s32 arg0) {
    u8 *pSlot;
    s32 i;
    s32 count;
    s32 offset;

    if (g_BattleActors[arg0] != NULL) {
        if (*(void **)(g_BattleActors[arg0] + 0xA8) != NULL) {
            HeapFree(*(void **)(g_BattleActors[arg0] + 0xA8));
            BattleFreeModelMesh(*(void **)g_BattleActors[arg0], 1);
        }
        if (*(u8 *)(g_BattleActors[arg0] + 0x62) != 0) {
            SoundRemoveSedsEntry(*(void **)(*(u8 **)(g_BattleActors[arg0] + 0xB0) + 8));
        }
        if (*(void **)(g_BattleActors[arg0] + 0xAC) != NULL) {
            HeapFree(*(void **)(g_BattleActors[arg0] + 0xAC));
        }
        if ((u32)(arg0 - 0x13) >= 0xC) {
            if (*(void **)(g_BattleActors[arg0] + 4) != NULL) {
                BattleFreeAllSpriteBlocks(g_BattleBlockPool);
                BattleFreeSpriteBlocksByTag(g_BattleBlockPool, *(void **)(g_BattleActors[arg0] + 4), 0xFF);
                BattleFreeModelSkeleton(*(void **)(g_BattleActors[arg0] + 4));
                *(void **)(g_BattleActors[arg0] + 0) = NULL;
                *(void **)(g_BattleActors[arg0] + 4) = NULL;
            }
            BattleFreeActorBuffer(*(void **)&g_BattleActors[arg0]);
        } else if (*(void **)(g_BattleActors[arg0] + 4) != NULL) {
            BattleFreeAllSpriteBlocks(g_BattleBlockPool);
            BattleFreeSpriteBlocksByTag(g_BattleBlockPool, *(void **)(g_BattleActors[arg0] + 4), 0xFF);
            HeapFree(*(void **)(g_BattleActors[arg0] + 4));
        }
            if (*(u8 *)(g_BattleActors[arg0] + 0x10C) != 0) {
            HeapFree(*(void **)(g_BattleActors[arg0] + 0x110));
        }
        count = *(u8 *)(g_BattleActors[arg0] + 0x10E);
        if (count != 0) {
            offset = 0;
            for (i = 0; i < count; i++) {
                BattleFreeImageEntry((u8 *)*(void **)(g_BattleActors[arg0] + 0x118) + offset);
                count = *(u8 *)(g_BattleActors[arg0] + 0x10E);
                offset += 0x30;
            }
            HeapFree(*(void **)(g_BattleActors[arg0] + 0x118));
        }
        count = *(u8 *)(g_BattleActors[arg0] + 0x10D);
        if (count != 0) {
            offset = 0;
            for (i = 0; i < count; i++) {
                BattleFreeModelListEntryBuffers((Misc23Block *)((u8 *)*(void **)(g_BattleActors[arg0] + 0x114) + offset));
                count = *(u8 *)(g_BattleActors[arg0] + 0x10D);
                offset += 0x24;
            }
            HeapFree(*(void **)(g_BattleActors[arg0] + 0x114));
        }
        HeapFree(*(void **)&g_BattleActors[arg0]);
        *(void **)&g_BattleActors[arg0] = NULL;
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
    D_800C3D68 = 1;
    ((u8 *)g_BattleActors[arg0])[0x35] = 1;
    g_NumFlaggedTimerWorkListEntries = 0;
    g_FlagTimerWorkListAdds = 1;
    func_800BF85C(arg0, BattleFindFirstActorInMask());
    g_BattleCommandRowIndex = 1;
    D_800C4000[BattleFindFirstActorInMask()] = 0;
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
