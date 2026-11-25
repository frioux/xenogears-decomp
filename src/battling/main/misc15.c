#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

// The block BattleCreateWorkNode hangs off a heap node; BattleFreeWorkBlock releases it.
typedef struct WorkBlock WorkBlock;
struct WorkBlock {
    /* 0x0 */ s16 count;
    /* 0x2 */ u16 polyCount;
    /* 0x4 */ u32 pVertices;
    /* 0x8 */ void* pMem;
    /* 0xC */ void* pData;
    /* 0x10 */ void* pDelayedMem;
    /* 0x14 */ void* pDelayedMemEnd;
}; // Size: 0x18

// A 16-byte heap node allocated by BattleCreateWorkNode.
typedef struct HeapNode {
    /* 0x0 */ s32 type;
    /* 0x4 */ void* pOwner;
    /* 0x8 */ void* pWork;
    /* 0xC */ WorkBlock* pBlock;
} HeapNode; // Size: 0x10

// One 8-byte vertex run of a packet node: four vertex indices into the model's
// vertex array (func_8008C4B0/620 scale each by 8). The triangle path reads
// v0..v2, the quad path all four.
typedef struct {
    /* 0x0 */ u16 v0;
    /* 0x2 */ u16 v1;
    /* 0x4 */ u16 v2;
    /* 0x6 */ u16 v3;
} VertexIndexSet; // Size: 0x8

// One entry of the packet list at g_BattlePacketNodeCursor: a flag byte and a count, then
// count 8-byte vertex runs immediately after this 4-byte header.
typedef struct {
    /* 0x0 */ u8 flags;
    /* 0x1 */ u8 pad1; // alignment before the count halfword; no reader touches it
    /* 0x2 */ s16 count;
    /* 0x4 */ VertexIndexSet verts[0];
} PacketNode; // Size: 0x4

// The work data a model block holds at +0x14: the counts BattleInitWorkBlock
// turns into delayed prims, and the head of the packet list.
typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ u16 count;
    /* 0x04 */ u16 polyCount;
    /* 0x06 */ u16 nodeCount;
    /* 0x08 */ u32 pVertices;
    /* 0x0C */ u8 unkC[0x4];
    /* 0x10 */ PacketNode* pNodes;
} WorkData; // Size: 0x14

// A model's render record (model block +0xC): the vertex count and pointer,
// the transform data and one packet pointer per on-screen copy, which
// g_BattleDrawBufferIndex indexes.
typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ u16 count;
    /* 0x04 */ u8 unk4[0x4];
    /* 0x08 */ void* pVerts;
    /* 0x0C */ WorkData* pData;
    /* 0x10 */ void* pPackets[1];
} RenderRecord; // Size: 0x14

// A model's sub-part (model block +0x4); its +4 slot is the address of the
// part's flags word.
typedef struct {
    /* 0x0 */ u8 unk0[0x4];
    /* 0x4 */ s32* pFlags;
} ModelPart; // Size: 0x8

// The model data block a tree node points at (node +0x4): its type word, the
// sub-part, the render record and the work data.
typedef struct {
    /* 0x00 */ s32 type;
    /* 0x04 */ ModelPart* pPart;
    /* 0x08 */ u8 unk8[0x4];
    /* 0x0C */ RenderRecord* pRender;
    /* 0x10 */ u8 unk10[0x4];
    /* 0x14 */ WorkData* pWorkData;
} ModelBlock; // Size: 0x18

// A node of the object tree BattleInitObjectTree builds: its model block and
// the two children the tree is walked through.
typedef struct ObjectNode ObjectNode;
struct ObjectNode {
    /* 0x00 */ s32 type;
    /* 0x04 */ ModelBlock* pModel;
    /* 0x08 */ u8 unk8[0x88];
    /* 0x90 */ ObjectNode* pChild90;
    /* 0x94 */ ObjectNode* pChild94;
}; // Size: 0x98

// An effect vertex as the trail code reads it: three u16 coordinates (the
// original loads them with lhu even where a SVECTOR would fit).
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad6;
} EffectVert; // Size: 0x8

// A quad trail effect: v0 is the vertex the caller writes each frame, v1..v3
// the trail, and then one LINE_F4 packet per on-screen copy.
typedef struct {
    /* 0x00 */ EffectVert v0;
    /* 0x08 */ u8 unk8[0x8];
    /* 0x10 */ EffectVert v1;
    /* 0x18 */ EffectVert v2;
    /* 0x20 */ EffectVert v3;
    /* 0x28 */ LINE_F4 prim[2];
} QuadTrail; // Size: 0x60

// A triangle trail effect: the same layout one packet shorter.
typedef struct {
    /* 0x00 */ EffectVert v0;
    /* 0x08 */ u8 unk8[0x8];
    /* 0x10 */ EffectVert v1;
    /* 0x18 */ EffectVert v2;
    /* 0x20 */ LINE_F3 prim[2];
} TriTrail; // Size: 0x50

// A two-vertex line trail effect.
typedef struct {
    /* 0x00 */ EffectVert v0;
    /* 0x08 */ u8 unk8[0x8];
    /* 0x10 */ EffectVert v1;
    /* 0x18 */ LINE_F2 prim[2];
} LineTrail; // Size: 0x38

// A 0x10-byte tile packet with an explicit width and height (the SDK's
// TILE_16 leaves those out, this one is written by BattleInitSpritePrim).
typedef struct {
    /* 0x0 */ u_long tag;
    /* 0x4 */ u_char r0, g0, b0, code;
    /* 0x8 */ short x0, y0;
    /* 0xC */ short w, h;
} TilePrim; // Size: 0x10

// A sprite effect: one tile packet per on-screen copy.
typedef struct {
    /* 0x00 */ EffectVert v0;
    /* 0x08 */ u8 unk8[0x8];
    /* 0x10 */ TilePrim prim[2];
} SpriteTrail; // Size: 0x30

// The record a prim copies its colour from; only the three colour bytes at
// +0x74 are read by this unit, the real structure is longer.
typedef struct {
    /* 0x00 */ u8 unk0[0x74];
    /* 0x74 */ u8 r0;
    /* 0x75 */ u8 g0;
    /* 0x76 */ u8 b0;
} EffectSource; // Size: 0x77+

extern s32 g_BattleObjectTreeNodeCount;
extern void* g_BattleObjectTreeRoot;
extern void* BattleCreateObjectTree(ObjectNode* pNode, void* pArg);
extern s32 BattleCreateObject(void);
extern void BattleAppendListEntry(s32 handle, void* pNode);
extern void BattleInitWorkBlock(WorkBlock* pBlock, WorkData* pData);
extern s32 rand(void);
extern u8 g_BattleDrawBufferIndex;
extern u_long* g_BattleCurrentOrderingTable;
extern void* g_pCurModelPacket;
extern void* g_pCurModelVertices;
extern void* D_80059568;
extern s32 g_BattleFramePolyCount;
extern void* g_BattlePacketNodeCursor;
extern void func_8008BD70(u8* pData, void* pPacket, void* pUnk, void* pVertices);
extern void func_8008C620(VertexIndexSet* pVerts, s16 n);
extern void func_800732AC(void* pDst, void* pSrc, s32 n);
extern void func_8008C4B0(VertexIndexSet* pVerts, s16 n);
extern void func_800316C0(void* pOT, void* pPrim);
extern void func_80031750(void* pOT, void* pPrim);
extern VECTOR g_BattleShadowPlaneNormal;
extern s32 D_8009A2CC;
extern s32 D_8009A2D0;
extern void func_8008C3A8(void* pSrc, void* pDst, u16 n);
extern void func_80031804(void* pOT, void* pPrim);
extern EffectVert* g_BattleScratchViewOrigin;
extern SVECTOR* g_BattleTrailVertScratch;
extern void func_80031708(void* pOT, void* pPrim);
// An 8-byte, byte-aligned chunk: assigning one copies 8 bytes with lwl/lwr.
typedef struct {
    u8 unk[8];
} Bytes8;

// A word-aligned 16-byte chunk: assigning one copies it with four lw/four sw.
typedef struct {
    s32 unk[4];
} Words4; // Size: 0x10

typedef struct {
    s32 unk[3];
} Words3; // Size: 0xC

typedef struct {
    s32 unk[2];
} Words2; // Size: 0x8

INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BC04);
void BattleProjectShadowVerts(RenderRecord* pRecord, u8* pDst) {
    VECTOR vec;
    VECTOR vecUnused;

    vec.vx = -8;
    vec.vy = -8;
    vec.vz = -16;
    VectorNormal(&vec, &g_BattleShadowPlaneNormal);
    g_BattleShadowPlaneNormal.vx <<= 4;
    g_BattleShadowPlaneNormal.vy <<= 4;
    g_BattleShadowPlaneNormal.vz <<= 4;
    func_8008C3A8(pRecord->pVerts, pDst, pRecord->count);
}
// PARKED 93.67% (228 bytes vs retail 232, 57 vs 55 insns, frame 40 with vars=8 against retail's 32/0).
// The residual is the loop ENTRY shape only. Retail computes i = count - 1 in the
// preheader and js straight to the single bottom test, sharing one
// sw g_BattlePacketNodeCursor between the preheader and the latch; cc1 duplicates the
// loop exit test instead, so the preheader gets its own test plus its own cursor store. That is
// the g-rotated-entry-j-to-bottom-test lever (round 398: unreachable from while, for,
// do-while, guarded do-while and goto-check shapes under 2.6.0, 2.7.2 and 2.7.2-cdk), so
// park on this shape and report ROTATED_ENTRY.
// Measured equal to this body (93.67): i-- inside both arms, one i-- after the if/else,
// for (; i != -1; i--) - reorg copies a single decrement into BOTH jal delay slots, so the
// two-arm spelling is not load-bearing. Lower: plain do-while 89.73 (224 bytes, no entry
// test), i-- before the if/else 90.40, cdk 93.82, -O3 93.67.
// Round 485 re-measures, all with retail's 55-insn body shape and this prototype:
//  - g-unrotated-top-tested-loop's `while (1) { if (--i == -1) break; }` (i = nodeCount)
//    DOES stop the duplicate: frame is retail's 32/0 and the size is retail's exact 232,
//    but the source-order test at the body top costs 3 insns -> 81.64% / 58 insns.
//  - `i = nodeCount; while (--i != -1)` also reaches frame 32/0 and the neighbour
//    BattleInitWorkBlock's shape, but adds `li -1 / beq / nop` at the entry: 87.64% / 236.
//  - moving the `g_BattlePacketNodeCursor = pData->pNodes` store to the last preheader
//    statement (retail stores it inside the shared block) costs the frameless prologue
//    back: 91.13% / 228 / frame 40. Original statement order is the best of the three.
// So retail's single bottom test is reached by refusal of the entry duplication, which no
// source shape reaches while keeping the fold of `count - 1 == -1` into `beqz count`
// (the fold is what keeps this body down to one extra test instead of three insns).
// The first parameter is u8* on purpose: written as WorkData* pData with
// pData->nodeCount / pNodes / polyCount the same body measures 90.49 at the same 228
// bytes, because the typed load changes the preheader DAG. BattleQueueNodePrims (the
// caller) still reads 100.00% with this prototype.
/*
void func_8008BD70(u8* pData, void* pPacket, void* pUnk, void* pVertices) {
    s32 i;
    u8* pNode;

    i = *(u16*)(pData + 6) - 1;
    g_BattlePacketNodeCursor = *(void**)(pData + 0x10);
    g_BattleFramePolyCount += *(u16*)(pData + 4);
    g_pCurModelPacket = pPacket;
    D_80059568 = pUnk;
    g_pCurModelVertices = pVertices;
    while (i != -1) {
        pNode = g_BattlePacketNodeCursor;
        g_BattlePacketNodeCursor = pNode + 4;
        if (*pNode & 8)
            func_8008C620((VertexIndexSet*)(pNode + 4), *(s16*)(pNode + 2));
        else
            func_8008C4B0((VertexIndexSet*)(pNode + 4), *(s16*)(pNode + 2));
        i--;
        g_BattlePacketNodeCursor = (u8*)g_BattlePacketNodeCursor + (*(s16*)(pNode + 2) << 3);
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BD70);

void BattleInitWorkBlock(WorkBlock* pBlock, WorkData* pData) {
    s32 i;
    s32 j;
    s32 n;
    s32 sumA;
    s32 sumB;
    VertexIndexSet* pMem;
    u8* pBuf;
    POLY_F4* pQuad;
    POLY_F3* pTri;
    PacketNode* pNode;

    pBlock->count = pData->count;
    pBlock->polyCount = pData->polyCount;
    pBlock->pVertices = pData->pVertices;
    pBlock->pData = pData;
    g_BattlePacketNodeCursor = pData->pNodes;
    HeapSetCurrentContentType(0x13);
    pMem = HeapAlloc(pBlock->count * sizeof(VertexIndexSet), 2);
    i = pBlock->count;
    pBlock->pMem = pMem;
    while (--i != -1) {
        pMem->v1 = 0;
        pMem++;
    }
    HeapSetCurrentContentType(5);
    sumA = 0;
    sumB = 0;
    i = pData->nodeCount;
    while (--i != -1) {
        pNode = g_BattlePacketNodeCursor;
        g_BattlePacketNodeCursor = pNode->verts;
        if (pNode->flags & 8)
            sumB += pNode->count;
        else
            sumA += pNode->count;
        g_BattlePacketNodeCursor = (VertexIndexSet*)g_BattlePacketNodeCursor + pNode->count;
    }
    i = sumA * sizeof(POLY_F3) + sumB * sizeof(POLY_F4);
    pBuf = HeapAlloc(i * 2, 2);
    pBlock->pDelayedMem = pBuf;
    pBlock->pDelayedMemEnd = pBuf + i;
    j = pData->nodeCount;
    g_BattlePacketNodeCursor = pData->pNodes;
    while (--j != -1) {
        pNode = g_BattlePacketNodeCursor;
        g_BattlePacketNodeCursor = pNode->verts;
        if (pNode->flags & 8) {
            n = pNode->count;
            while (--n != -1) {
                pQuad = (POLY_F4*)pBuf;
                pBuf[3] = 5;
                *(u32*)&pQuad->r0 = 0x28403030;
                pBuf += sizeof(POLY_F4);
            }
        } else {
            n = pNode->count;
            while (--n != -1) {
                pTri = (POLY_F3*)pBuf;
                pBuf[3] = 4;
                *(u32*)&pTri->r0 = 0x20403030;
                pBuf += sizeof(POLY_F3);
            }
        }
        g_BattlePacketNodeCursor = (VertexIndexSet*)g_BattlePacketNodeCursor + pNode->count;
    }
    func_800732AC(pBlock->pDelayedMemEnd, pBlock->pDelayedMem, i);
}
void BattleAttachWorkNode(HeapNode* pNode, void* pOwner) {
    pNode->type = 5;
    pNode->pOwner = pOwner;
}
HeapNode* BattleCreateWorkNode(ObjectNode* pOwner) {
    HeapNode* pNode;
    s32 type;
    void* pWork;

    HeapSetCurrentContentType(7);
    pNode = HeapAlloc(0x10, 2);
    type = pOwner->type;
    pWork = g_BattleObjectTreeRoot;
    pNode->pOwner = pOwner;
    pNode->pBlock = NULL;
    pNode->type = type;
    pNode->pWork = pWork;
    return pNode;
}
void BattleFreeWorkBlock(HeapNode* pNode) {
    WorkBlock* pBlock = pNode->pBlock;

    if (pBlock != NULL) {
        if (pBlock->pMem != NULL)
            HeapFree(pBlock->pMem);
        if (pBlock->pDelayedMem != NULL)
            HeapDelayedFree(pBlock->pDelayedMem, 2);
        HeapFree(pBlock);
    }
}
void* BattleCreateObjectTree(ObjectNode* pNode, void* pArg) {
    void* pWork;
    HeapNode* pHeapNode;
    WorkBlock* pBlock;
    WorkData* pData;
    ObjectNode* pChild;

    HeapSetCurrentContentType(6);
    pWork = (void*)BattleCreateObject();
    pHeapNode = BattleCreateWorkNode(pNode);
    BattleAttachWorkNode(pWork, pHeapNode);
    if (pHeapNode->type == 1) {
        pData = pNode->pModel->pWorkData;
        HeapSetCurrentContentType(5);
        pBlock = HeapAlloc(0x18, 2);
        pHeapNode->pBlock = pBlock;
        BattleInitWorkBlock(pBlock, pData);
    }
    pChild = pNode->pChild94;
    g_BattleObjectTreeNodeCount++;
    if (pChild != NULL)
        BattleAppendListEntry((s32)pWork, BattleCreateObjectTree(pChild, pWork));
    if (pNode->pChild90 != NULL)
        BattleAppendListEntry((s32)pArg, BattleCreateObjectTree(pNode->pChild90, pArg));
    return pWork;
}
void BattleResetObjectCountAndCreateTree(void* pWork) {
    g_BattleObjectTreeNodeCount = 0;
    BattleCreateObjectTree(pWork, NULL);
}
void BattleInitObjectTree(void* pWork) {
    g_BattleObjectTreeRoot = pWork;
    BattleResetObjectCountAndCreateTree(pWork);
}
void BattleQueueNodePrims(ObjectNode* pNode) {
    ModelBlock* pModel;
    ModelPart* pPart;
    RenderRecord* pRender;

    pModel = pNode->pModel;
    if (pModel->type == 1) {
        pPart = pModel->pPart;
        if ((*pPart->pFlags & 1) == 0) {
            pRender = pModel->pRender;
            func_8008BD70(pRender->pData, pRender->pPackets[g_BattleDrawBufferIndex],
                          g_BattleCurrentOrderingTable + 1, pRender->pVerts);
        }
    }
    if (pNode->pChild94 != NULL)
        BattleQueueNodePrims(pNode->pChild94);
    if (pNode->pChild90 != NULL)
        BattleQueueNodePrims(pNode->pChild90);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C3A8);
/* Best C match, 73.52% (272 bytes vs retail 280). Two shapes are already right
   and must be kept when this is picked up again:
     - the reference-VECTOR read is gte_ld3_gpr(tx, ty, tz, &g_BattleShadowPlaneNormal): one
       `la` plus 0/4/8 displacements. Written as `g_BattleShadowPlaneNormal.vx/vy/vz` (or
       through any pointer local) gcc folds each field into its own lui/lo
       address and the whole register allocation shifts (67.33%).
     - the loop is do/while over a signed count (`n--; while (n != 0)`); with a
       u16 parameter gcc adds an `andi v0,a2,0xffff` retail does not have.
   Still different: retail's MVMVA carries one nop before AND one after the
   command (gte_mvmva emits two before, and the 3-instruction slot then shifts
   everything after it); retail hoists `addiu a1,a1,8` above the first store and
   addresses it as -8/-4(a1) while gcc stores at 0/4(a1) after both divisions;
   retail's second numerator is `negu t7,t3` then addu then `negu` (i.e. it
   negates MAC3 first), a plain `z * ty - tz * y` gives subu; and the MAC reads
   land in t8/t9/t3 with the products in t0/t1 and the divisor in t2, against
   ours in t0/v1/a0 + v0/v1/a3.
void func_8008C3A8(SVECTOR* pSrc, SVECTOR* pDst, s32 n) {
    s32 tx;
    s32 ty;
    s32 tz;
    s32 x;
    s32 y;
    s32 z;
    s32 den;

    gte_ld3_gpr(tx, ty, tz, &g_BattleShadowPlaneNormal);
    gte_ldv0(pSrc);
    pSrc++;
    do {
        gte_mvmva(1, 0, 0, 0, 0);
        gte_ldv0(pSrc);
        gte_stmac2_val(y);
        gte_stmac1_val(x);
        den = ty - y;
        if (den != 0) {
            gte_stmac3_val(z);
            pDst->vx = (x * ty - tx * y) / den;
            pDst->vz = (z * ty - tz * y) / den;
        }
        pDst++;
        pSrc++;
        n--;
    } while (n != 0);
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C4B0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C620);
void BattleShiftQuad4Verts(QuadTrail* pTrail) {
    *(Bytes8*)&pTrail->v3 = *(Bytes8*)&pTrail->v0;
    *(Bytes8*)&pTrail->v2 = *(Bytes8*)&pTrail->v3;
    *(Bytes8*)&pTrail->v1 = *(Bytes8*)&pTrail->v2;
}
void BattleInitQuad4Prim(QuadTrail* pTrail, EffectSource* pSrc) {
    pTrail->prim[0].code = 0x4C;
    setlen(&pTrail->prim[0], 6);
    pTrail->prim[0].pad = 0x55555555;
    pTrail->prim[0].code |= 2;
    pTrail->prim[0].r0 = pSrc->r0;
    pTrail->prim[0].g0 = pSrc->g0;
    pTrail->prim[0].b0 = pSrc->b0;
    *(Words4*)&pTrail->prim[1].tag = *(Words4*)&pTrail->prim[0].tag;
    *(Words3*)&pTrail->prim[1].x2 = *(Words3*)&pTrail->prim[0].x2;
}
void BattleAddQuad4Prim(QuadTrail* pTrail, u8* pOT) {
    long local[2];
    LINE_F4* pPrim;
    s32 otz;

    pPrim = &pTrail->prim[g_BattleDrawBufferIndex];
    otz = RotTransPers4((SVECTOR*)&pTrail->v0, (SVECTOR*)&pTrail->v1, (SVECTOR*)&pTrail->v2,
                        (SVECTOR*)&pTrail->v3, (long*)&pPrim->x0, (long*)&pPrim->x1,
                        (long*)&pPrim->x2, (long*)&pPrim->x3, local, local);
    *(Bytes8*)&pTrail->v3 = *(Bytes8*)&pTrail->v2;
    *(Bytes8*)&pTrail->v2 = *(Bytes8*)&pTrail->v1;
    *(Bytes8*)&pTrail->v1 = *(Bytes8*)&pTrail->v0;
    func_80031750(pOT + ((otz >> 2) << 2), pPrim);
}
void BattleShiftTri3Verts(TriTrail* pTrail) {
    *(Bytes8*)&pTrail->v2 = *(Bytes8*)&pTrail->v0;
    *(Bytes8*)&pTrail->v1 = *(Bytes8*)&pTrail->v2;
}
void BattleInitTri3Prim(TriTrail* pTrail, EffectSource* pSrc) {
    pTrail->prim[0].code = 0x48;
    setlen(&pTrail->prim[0], 5);
    pTrail->prim[0].pad = 0x55555555;
    pTrail->prim[0].code |= 2;
    pTrail->prim[0].r0 = pSrc->r0;
    pTrail->prim[0].g0 = pSrc->g0;
    pTrail->prim[0].b0 = pSrc->b0;
    *(Words4*)&pTrail->prim[1].tag = *(Words4*)&pTrail->prim[0].tag;
    *(Words2*)&pTrail->prim[1].x2 = *(Words2*)&pTrail->prim[0].x2;
}
void BattleAddTri3Prim(TriTrail* pTrail, u8* pOT) {
    EffectVert* pRef;
    LINE_F3* pPrim;
    s32 i;
    long otz;

    pRef = g_BattleScratchViewOrigin;
    g_BattleTrailVertScratch[0].vx = pTrail->v0.vx - pRef->vx;
    g_BattleTrailVertScratch[0].vy = pTrail->v0.vy - pRef->vy;
    g_BattleTrailVertScratch[0].vz = pTrail->v0.vz - pRef->vz;
    g_BattleTrailVertScratch[1].vx = pTrail->v1.vx - pRef->vx;
    g_BattleTrailVertScratch[1].vy = pTrail->v1.vy - pRef->vy;
    g_BattleTrailVertScratch[1].vz = pTrail->v1.vz - pRef->vz;
    g_BattleTrailVertScratch[2].vx = pTrail->v2.vx - pRef->vx;
    g_BattleTrailVertScratch[2].vy = pTrail->v2.vy - pRef->vy;
    g_BattleTrailVertScratch[2].vz = pTrail->v2.vz - pRef->vz;
    gte_ldv3c(g_BattleTrailVertScratch);
    gte_rtpt();
    i = g_BattleDrawBufferIndex;
    *(Bytes8*)&pTrail->v2 = *(Bytes8*)&pTrail->v1;
    *(Bytes8*)&pTrail->v1 = *(Bytes8*)&pTrail->v0;
    pPrim = &pTrail->prim[i];
    gte_stsxy3(&pPrim->x0, &pPrim->x1, &pPrim->x2);
    gte_stszotz(&otz);
    func_80031708(pOT + ((otz >> 2) << 2), pPrim);
}
void BattleShiftLine2Verts(LineTrail* pTrail) {
    *(Bytes8*)&pTrail->v1 = *(Bytes8*)&pTrail->v0;
}
void BattleInitLine2Prim(LineTrail* pTrail, EffectSource* pSrc) {
    setlen(&pTrail->prim[0], 3);
    pTrail->prim[0].code = 0x42;
    pTrail->prim[0].r0 = pSrc->r0;
    pTrail->prim[0].g0 = pSrc->g0;
    pTrail->prim[0].b0 = pSrc->b0;
    *(Words4*)&pTrail->prim[1].tag = *(Words4*)&pTrail->prim[0].tag;
}
void BattleAddLine2Prim(LineTrail* pTrail, u8* pOT) {
    long local[2];
    LINE_F2* pPrim;
    s32 otz;

    pPrim = &pTrail->prim[g_BattleDrawBufferIndex];
    otz = RotTransPers3((SVECTOR*)&pTrail->v0, (SVECTOR*)&pTrail->v1, (SVECTOR*)local,
                        (long*)&pPrim->x0, (long*)&pPrim->x1,
                        local, local, local);
    *(Bytes8*)&pTrail->v1 = *(Bytes8*)&pTrail->v0;
    func_800316C0(pOT + ((otz >> 2) << 2), pPrim);
}
void BattleNoOpVertexShift(void) {
}
void BattleInitSpritePrim(SpriteTrail* pTrail, EffectSource* pSrc) {
    s16 n;

    setlen(&pTrail->prim[0], 3);
    pTrail->prim[0].code = 0x62;
    n = rand() % 2 + 2;
    pTrail->prim[0].h = n;
    pTrail->prim[0].w = n * 2;
    pTrail->prim[0].r0 = pSrc->r0;
    pTrail->prim[0].g0 = pSrc->g0;
    pTrail->prim[0].b0 = pSrc->b0;
    *(Words4*)&pTrail->prim[1].tag = *(Words4*)&pTrail->prim[0].tag;
}
void BattleAddSpritePrim(SpriteTrail* pTrail, u8* pOT) {
    SVECTOR vec;
    EffectVert* pRef;
    TilePrim* pPrim;
    long sxy;
    s32 otz;

    pRef = g_BattleScratchViewOrigin;
    vec.vx = pTrail->v0.vx - pRef->vx;
    vec.vy = pTrail->v0.vy - pRef->vy;
    vec.vz = pTrail->v0.vz - pRef->vz;
    gte_ldv0(&vec);
    gte_rtps();
    pPrim = &pTrail->prim[g_BattleDrawBufferIndex];
    gte_stsxy2(&pPrim->x0);
    gte_stszotz(&sxy);
    otz = sxy;
    func_80031804(pOT + ((otz >> 2) << 2), pPrim);
}
