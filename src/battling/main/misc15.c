#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

// The block BattleCreateWorkNode hangs off a heap node; BattleFreeWorkBlock releases it.
typedef struct WorkBlock WorkBlock;
struct WorkBlock {
    /* 0x0 */ s16 count;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u32 unk4;
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

// One 8-byte vertex run of a packet node: three vertex indices into the
// model's vertex array (func_8008C4B0/620 scale each by 8) and padding.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 pad6;
} VertexIndexSet; // Size: 0x8

// One entry of the packet list at g_BattlePacketNodeCursor: a flag byte and a count, then
// count 8-byte vertex runs immediately after this 4-byte header.
typedef struct {
    /* 0x0 */ u8 flags;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ s16 count;
    /* 0x4 */ VertexIndexSet verts[0];
} PacketNode; // Size: 0x4

// The work data a model block holds at +0x14: the counts BattleInitWorkBlock
// turns into delayed prims, and the head of the packet list.
typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ u16 count;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 nodeCount;
    /* 0x08 */ u32 unk8;
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

extern s32 D_80092824;
extern void* D_80092828;
extern void* BattleCreateObjectTree(ObjectNode* pNode, void* pArg);
extern s32 BattleCreateObject(void);
extern void BattleAppendListEntry(s32 handle, void* pNode);
extern void BattleInitWorkBlock(WorkBlock* pBlock, WorkData* pData);
extern s32 rand(void);
extern u8 g_BattleDrawBufferIndex;
extern s32 D_800928E4;
extern void* g_pCurModelPacket;
extern void* g_pCurModelVertices;
extern void* D_80059568;
extern s32 D_800595C0;
extern void* g_BattlePacketNodeCursor;
extern void func_8008BD70(WorkData* pData, void* pPacket, void* pUnk, void* pVertices);
extern void func_8008C620(VertexIndexSet* pVerts, s16 n);
extern void func_800732AC(void* pDst, void* pSrc, s32 n);
extern void func_8008C4B0(VertexIndexSet* pVerts, s16 n);
extern void func_800316C0(void* pOT, void* pPrim);
extern void func_80031750(void* pOT, void* pPrim);
extern VECTOR D_8009A2C8;
extern s32 D_8009A2CC;
extern s32 D_8009A2D0;
extern void func_8008C3A8(void* pSrc, void* pDst, u16 n);
extern void func_80031804(void* pOT, void* pPrim);
extern EffectVert* D_80092830;
extern SVECTOR* D_8009282C;
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
    VectorNormal(&vec, &D_8009A2C8);
    D_8009A2C8.vx <<= 4;
    D_8009A2C8.vy <<= 4;
    D_8009A2C8.vz <<= 4;
    func_8008C3A8(pRecord->pVerts, pDst, pRecord->count);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BD70);
/* Best C match, 93.67% (228 bytes vs retail 232). Two things here are load-bearing
   and worth keeping: the preheader assigns D_800595C0 BEFORE the three global
   stores (retail loads pData+4 there; the other order costs 10 points), and the
   loop counter is decremented inside both arms, which is what puts the addiu in
   both jal delay slots. Still different: gcc turns `i = count - 1; while
   (i != -1)` into a `beqz count` guard plus a fall-through body, while retail
   jumps straight to the latch (`j`, `addiu s1,s1,-1`) and has the loop test at
   the bottom; and gcc keeps the `g_BattlePacketNodeCursor = pData->pNodes` store in the
   preheader where retail merges it into the latch block, which also reorders the
   preheader loads. Writing that store at the loop top through a pCur local
   instead drops to 73.73% (gcc folds the redundant reload), `while (--i != -1)`
   with a single decrement is 87.64%, and the original 83.00% park note's
   `do/while` variants were 74-80%. Typing the body against WorkData/PacketNode
   (as below) scores 90.49% at the same 228 bytes: the raw `*(u16*)(pData + 6)`
   and `*(s16*)(pNode + 2)` spellings are worth 3 points here, so a re-park must
   go back to them.
void func_8008BD70(WorkData* pData, void* pPacket, void* pUnk, void* pVertices) {
    s32 i;
    PacketNode* pNode;

    i = pData->nodeCount - 1;
    g_BattlePacketNodeCursor = pData->pNodes;
    D_800595C0 += pData->unk4;
    g_pCurModelPacket = pPacket;
    D_80059568 = pUnk;
    g_pCurModelVertices = pVertices;
    while (i != -1) {
        pNode = g_BattlePacketNodeCursor;
        g_BattlePacketNodeCursor = pNode->verts;
        if (pNode->flags & 8) {
            i--;
            func_8008C620(pNode->verts, pNode->count);
        } else {
            i--;
            func_8008C4B0(pNode->verts, pNode->count);
        }
        g_BattlePacketNodeCursor = (u8*)g_BattlePacketNodeCursor + (pNode->count << 3);
    }
}
*/
void BattleInitWorkBlock(WorkBlock* pBlock, WorkData* pData) {
    s32 i;
    s32 j;
    s32 n;
    s32 sumA;
    s32 sumB;
    u8* pMem;
    u8* pBuf;
    PacketNode* pNode;

    pBlock->count = pData->count;
    pBlock->unk2 = pData->unk4;
    pBlock->unk4 = pData->unk8;
    pBlock->pData = pData;
    g_BattlePacketNodeCursor = pData->pNodes;
    HeapSetCurrentContentType(0x13);
    pMem = HeapAlloc(pBlock->count * 8, 2);
    i = pBlock->count;
    pBlock->pMem = pMem;
    while (--i != -1) {
        *(u16*)(pMem + 2) = 0;
        pMem += 8;
    }
    HeapSetCurrentContentType(5);
    sumA = 0;
    sumB = 0;
    i = pData->nodeCount;
    while (--i != -1) {
        pNode = g_BattlePacketNodeCursor;
        g_BattlePacketNodeCursor = (u8*)pNode + sizeof(PacketNode);
        if (pNode->flags & 8)
            sumB += pNode->count;
        else
            sumA += pNode->count;
        g_BattlePacketNodeCursor = (u8*)g_BattlePacketNodeCursor + (pNode->count << 3);
    }
    i = sumA * 0x14 + sumB * 0x18;
    pBuf = HeapAlloc(i * 2, 2);
    pBlock->pDelayedMem = pBuf;
    pBlock->pDelayedMemEnd = pBuf + i;
    j = pData->nodeCount;
    g_BattlePacketNodeCursor = pData->pNodes;
    while (--j != -1) {
        pNode = g_BattlePacketNodeCursor;
        g_BattlePacketNodeCursor = (u8*)pNode + sizeof(PacketNode);
        if (pNode->flags & 8) {
            n = pNode->count;
            while (--n != -1) {
                pBuf[3] = 5;
                *(u32*)(pBuf + 4) = 0x28403030;
                pBuf += 0x18;
            }
        } else {
            n = pNode->count;
            while (--n != -1) {
                pBuf[3] = 4;
                *(u32*)(pBuf + 4) = 0x20403030;
                pBuf += 0x14;
            }
        }
        g_BattlePacketNodeCursor = (u8*)g_BattlePacketNodeCursor + (pNode->count << 3);
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
    pWork = D_80092828;
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
    D_80092824++;
    if (pChild != NULL)
        BattleAppendListEntry((s32)pWork, BattleCreateObjectTree(pChild, pWork));
    if (pNode->pChild90 != NULL)
        BattleAppendListEntry((s32)pArg, BattleCreateObjectTree(pNode->pChild90, pArg));
    return pWork;
}
void BattleResetObjectCountAndCreateTree(void* pWork) {
    D_80092824 = 0;
    BattleCreateObjectTree(pWork, NULL);
}
void BattleInitObjectTree(void* pWork) {
    D_80092828 = pWork;
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
                          (u8*)D_800928E4 + 4, pRender->pVerts);
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
     - the reference-VECTOR read is gte_ld3_gpr(tx, ty, tz, &D_8009A2C8): one
       `la` plus 0/4/8 displacements. Written as `D_8009A2C8.vx/vy/vz` (or
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

    gte_ld3_gpr(tx, ty, tz, &D_8009A2C8);
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

    pRef = D_80092830;
    D_8009282C[0].vx = pTrail->v0.vx - pRef->vx;
    D_8009282C[0].vy = pTrail->v0.vy - pRef->vy;
    D_8009282C[0].vz = pTrail->v0.vz - pRef->vz;
    D_8009282C[1].vx = pTrail->v1.vx - pRef->vx;
    D_8009282C[1].vy = pTrail->v1.vy - pRef->vy;
    D_8009282C[1].vz = pTrail->v1.vz - pRef->vz;
    D_8009282C[2].vx = pTrail->v2.vx - pRef->vx;
    D_8009282C[2].vy = pTrail->v2.vy - pRef->vy;
    D_8009282C[2].vz = pTrail->v2.vz - pRef->vz;
    gte_ldv3c(D_8009282C);
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

    pRef = D_80092830;
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
