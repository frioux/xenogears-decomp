#include "common.h"
#include "psyq/libgte.h"
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

extern s32 D_80092824;
extern void* D_80092828;
extern void* BattleCreateObjectTree(u8* pObj, void* pArg);
extern s32 BattleCreateObject(void);
extern void BattleAppendListEntry(s32 handle, void* pNode);
extern void BattleInitWorkBlock(WorkBlock* pBlock, u8* pData);
extern s32 rand(void);
extern u8 D_800928A0;
extern s32 D_800928E4;
extern void* g_pCurModelPacket;
extern void* g_pCurModelVertices;
extern void* D_80059568;
extern s32 D_800595C0;
extern void* D_80059528;
extern void func_8008BD70(u8* pData, void* pPacket, void* pUnk, void* pVertices);
extern void func_8008C620(void* pNode, s16 n);
extern void func_800732AC(void* pDst, void* pSrc, s32 n);
extern void func_8008C4B0(void* pNode, s16 n);
extern void func_800316C0(void* pOT, void* pPrim);
extern void func_80031750(void* pOT, void* pPrim);
extern VECTOR D_8009A2C8;
extern s32 D_8009A2CC;
extern s32 D_8009A2D0;
extern void func_8008C3A8(void* pSrc, void* pDst, u16 n);
extern void func_80031804(void* pOT, void* pPrim);
extern u8* D_80092830;
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
void BattleProjectShadowVerts(u8* pObj, u8* pDst) {
    VECTOR vec;
    VECTOR vecUnused;

    vec.vx = -8;
    vec.vy = -8;
    vec.vz = -16;
    VectorNormal(&vec, &D_8009A2C8);
    D_8009A2C8.vx <<= 4;
    D_8009A2C8.vy <<= 4;
    D_8009A2C8.vz <<= 4;
    func_8008C3A8(*(void**)(pObj + 8), pDst, *(u16*)(pObj + 2));
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BD70);
/* Best C match, 83.00%: the body is right (the loop's decrement even lands in
   both jal delay slots), but gcc emits a top test (`li v0,-1; beq s1,v0`) and
   keeps `D_80059528 = *(pData+0x10)` in the preheader, while the original
   rotates the loop (preheader `s1--`, `j` to the bottom test) and merges that
   store into the loop-tail block.  do/while (i-- != 0) with i = count-1 and
   `while (i != -1) { ...; i--; }` both drop to 74-80%.
void func_8008BD70(u8* pData, void* pPacket, void* pUnk, void* pVertices) {
    s32 i;
    u8* pNode;

    i = *(u16*)(pData + 6);
    D_80059528 = *(void**)(pData + 0x10);
    g_pCurModelPacket = pPacket;
    D_80059568 = pUnk;
    g_pCurModelVertices = pVertices;
    D_800595C0 += *(u16*)(pData + 4);
    while (--i != -1) {
        pNode = (u8*)D_80059528;
        D_80059528 = pNode + 4;
        if (pNode[0] & 8)
            func_8008C620(pNode + 4, *(s16*)(pNode + 2));
        else
            func_8008C4B0(pNode + 4, *(s16*)(pNode + 2));
        D_80059528 = (u8*)D_80059528 + (*(s16*)(pNode + 2) << 3);
    }
}
*/
void BattleInitWorkBlock(WorkBlock* pBlock, u8* pData) {
    s32 i;
    s32 j;
    s32 n;
    s32 sumA;
    s32 sumB;
    u8* pMem;
    u8* pBuf;
    u8* pNode;

    pBlock->count = *(u16*)(pData + 2);
    pBlock->unk2 = *(u16*)(pData + 4);
    pBlock->unk4 = *(u32*)(pData + 8);
    pBlock->pData = pData;
    D_80059528 = *(void**)(pData + 0x10);
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
    i = *(u16*)(pData + 6);
    while (--i != -1) {
        pNode = D_80059528;
        D_80059528 = pNode + 4;
        if (pNode[0] & 8)
            sumB += *(s16*)(pNode + 2);
        else
            sumA += *(s16*)(pNode + 2);
        D_80059528 = (u8*)D_80059528 + (*(s16*)(pNode + 2) << 3);
    }
    i = sumA * 0x14 + sumB * 0x18;
    pBuf = HeapAlloc(i * 2, 2);
    pBlock->pDelayedMem = pBuf;
    pBlock->pDelayedMemEnd = pBuf + i;
    j = *(u16*)(pData + 6);
    D_80059528 = *(void**)(pData + 0x10);
    while (--j != -1) {
        pNode = D_80059528;
        D_80059528 = pNode + 4;
        if (pNode[0] & 8) {
            n = *(s16*)(pNode + 2);
            while (--n != -1) {
                pBuf[3] = 5;
                *(u32*)(pBuf + 4) = 0x28403030;
                pBuf += 0x18;
            }
        } else {
            n = *(s16*)(pNode + 2);
            while (--n != -1) {
                pBuf[3] = 4;
                *(u32*)(pBuf + 4) = 0x20403030;
                pBuf += 0x14;
            }
        }
        D_80059528 = (u8*)D_80059528 + (*(s16*)(pNode + 2) << 3);
    }
    func_800732AC(pBlock->pDelayedMemEnd, pBlock->pDelayedMem, i);
}
void BattleAttachWorkNode(void* pObj, void* pValue) {
    *(s32*)pObj = 5;
    *((void**)pObj + 1) = pValue;
}
HeapNode* BattleCreateWorkNode(void* pOwner) {
    HeapNode* pNode;
    s32 type;
    void* pWork;

    HeapSetCurrentContentType(7);
    pNode = HeapAlloc(0x10, 2);
    type = *(s32*)pOwner;
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
void* BattleCreateObjectTree(u8* pObj, void* pArg) {
    void* pWork;
    HeapNode* pNode;
    void* pBlock;
    void* pData;
    u8* pChild;

    HeapSetCurrentContentType(6);
    pWork = (void*)BattleCreateObject();
    pNode = BattleCreateWorkNode(pObj);
    BattleAttachWorkNode(pWork, pNode);
    if (pNode->type == 1) {
        pData = *(void**)(*(u8**)(pObj + 4) + 0x14);
        HeapSetCurrentContentType(5);
        pBlock = HeapAlloc(0x18, 2);
        pNode->pBlock = pBlock;
        BattleInitWorkBlock(pBlock, pData);
    }
    pChild = *(u8**)(pObj + 0x94);
    D_80092824++;
    if (pChild != NULL)
        BattleAppendListEntry((s32)pWork, BattleCreateObjectTree(pChild, pWork));
    if (*(void**)(pObj + 0x90) != NULL)
        BattleAppendListEntry((s32)pArg, BattleCreateObjectTree(*(u8**)(pObj + 0x90), pArg));
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
void BattleQueueNodePrims(u8* pObj) {
    u8* pData;
    u8* pChk;
    u8* pBase;

    pData = *(u8**)(pObj + 4);
    if (*(s32*)pData == 1) {
        pChk = *(u8**)(pData + 4);
        if ((*(s32*)*(u8**)(pChk + 4) & 1) == 0) {
            pBase = *(u8**)(pData + 0xC);
            func_8008BD70(*(void**)(pBase + 0xC), *(void**)((u8*)pBase + 0x10 + D_800928A0 * 4),
                          (u8*)D_800928E4 + 4, *(void**)(pBase + 8));
        }
    }
    if (*(u8**)(pObj + 0x94) != NULL)
        BattleQueueNodePrims(*(u8**)(pObj + 0x94));
    if (*(u8**)(pObj + 0x90) != NULL)
        BattleQueueNodePrims(*(u8**)(pObj + 0x90));
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
void BattleShiftQuad4Verts(u8* pObj) {
    *(Bytes8*)(pObj + 0x20) = *(Bytes8*)pObj;
    *(Bytes8*)(pObj + 0x18) = *(Bytes8*)(pObj + 0x20);
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)(pObj + 0x18);
}
void BattleInitQuad4Prim(u8* pObj, u8* pSrc) {
    pObj[0x2F] = 0x4C;
    pObj[0x2B] = 6;
    *(u32*)(pObj + 0x40) = 0x55555555;
    pObj[0x2F] |= 2;
    pObj[0x2C] = pSrc[0x74];
    pObj[0x2D] = pSrc[0x75];
    pObj[0x2E] = pSrc[0x76];
    *(Words4*)(pObj + 0x44) = *(Words4*)(pObj + 0x28);
    *(Words3*)(pObj + 0x54) = *(Words3*)(pObj + 0x38);
}
void BattleAddQuad4Prim(u8* pObj, u8* pOT) {
    long local[2];
    u8* pBase;
    s32 otz;

    pBase = pObj + 0x28 + D_800928A0 * 0x1C;
    otz = RotTransPers4((SVECTOR*)pObj, (SVECTOR*)(pObj + 0x10), (SVECTOR*)(pObj + 0x18),
                        (SVECTOR*)(pObj + 0x20), (long*)(pBase + 8), (long*)(pBase + 0xC),
                        (long*)(pBase + 0x10), (long*)(pBase + 0x14), local, local);
    *(Bytes8*)(pObj + 0x20) = *(Bytes8*)(pObj + 0x18);
    *(Bytes8*)(pObj + 0x18) = *(Bytes8*)(pObj + 0x10);
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)pObj;
    func_80031750(pOT + ((otz >> 2) << 2), pBase);
}
void BattleShiftTri3Verts(u8* pObj) {
    *(Bytes8*)(pObj + 0x18) = *(Bytes8*)pObj;
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)(pObj + 0x18);
}
void BattleInitTri3Prim(u8* pObj, u8* pSrc) {
    pObj[0x27] = 0x48;
    pObj[0x23] = 5;
    *(u32*)(pObj + 0x34) = 0x55555555;
    pObj[0x27] |= 2;
    pObj[0x24] = pSrc[0x74];
    pObj[0x25] = pSrc[0x75];
    pObj[0x26] = pSrc[0x76];
    *(Words4*)(pObj + 0x38) = *(Words4*)(pObj + 0x20);
    *(Words2*)(pObj + 0x48) = *(Words2*)(pObj + 0x30);
}
void BattleAddTri3Prim(u8* pObj, u8* pOT) {
    u8* pRef;
    u8* pPkt;
    s32 i;
    long otz;

    pRef = D_80092830;
    D_8009282C[0].vx = *(u16*)(pObj + 0x0) - *(u16*)(pRef + 0x0);
    D_8009282C[0].vy = *(u16*)(pObj + 0x2) - *(u16*)(pRef + 0x2);
    D_8009282C[0].vz = *(u16*)(pObj + 0x4) - *(u16*)(pRef + 0x4);
    D_8009282C[1].vx = *(u16*)(pObj + 0x10) - *(u16*)(pRef + 0x0);
    D_8009282C[1].vy = *(u16*)(pObj + 0x12) - *(u16*)(pRef + 0x2);
    D_8009282C[1].vz = *(u16*)(pObj + 0x14) - *(u16*)(pRef + 0x4);
    D_8009282C[2].vx = *(u16*)(pObj + 0x18) - *(u16*)(pRef + 0x0);
    D_8009282C[2].vy = *(u16*)(pObj + 0x1A) - *(u16*)(pRef + 0x2);
    D_8009282C[2].vz = *(u16*)(pObj + 0x1C) - *(u16*)(pRef + 0x4);
    gte_ldv3c(D_8009282C);
    gte_rtpt();
    i = D_800928A0;
    *(Bytes8*)(pObj + 0x18) = *(Bytes8*)(pObj + 0x10);
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)pObj;
    pPkt = pObj + (i * 0x18 + 0x20);
    gte_stsxy3(pPkt + 8, pPkt + 0xC, pPkt + 0x10);
    gte_stszotz(&otz);
    func_80031708(pOT + ((otz >> 2) << 2), pPkt);
}
void BattleShiftLine2Verts(u8* pObj) {
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)pObj;
}
void BattleInitLine2Prim(u8* pObj, u8* pSrc) {
    pObj[0x1B] = 3;
    pObj[0x1F] = 0x42;
    pObj[0x1C] = pSrc[0x74];
    pObj[0x1D] = pSrc[0x75];
    pObj[0x1E] = pSrc[0x76];
    *(Words4*)(pObj + 0x28) = *(Words4*)(pObj + 0x18);
}
void BattleAddLine2Prim(u8* pObj, u8* pOT) {
    long local[2];
    u8* pBase;
    s32 otz;

    pBase = pObj + 0x18 + D_800928A0 * 0x10;
    otz = RotTransPers3((SVECTOR*)pObj, (SVECTOR*)(pObj + 0x10), (SVECTOR*)local,
                        (long*)(pBase + 8), (long*)(pBase + 0xC),
                        local, local, local);
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)pObj;
    func_800316C0(pOT + ((otz >> 2) << 2), pBase);
}
void BattleNoOpVertexShift(void) {
}
void BattleInitSpritePrim(u8* pObj, u8* pSrc) {
    s16 n;

    pObj[0x13] = 3;
    pObj[0x17] = 0x62;
    n = rand() % 2 + 2;
    *(s16*)(pObj + 0x1E) = n;
    *(s16*)(pObj + 0x1C) = n * 2;
    pObj[0x14] = pSrc[0x74];
    pObj[0x15] = pSrc[0x75];
    pObj[0x16] = pSrc[0x76];
    *(Words4*)(pObj + 0x20) = *(Words4*)(pObj + 0x10);
}
void BattleAddSpritePrim(u8* pSrc, u8* pOT) {
    SVECTOR vec;
    u8* pRef;
    u8* pDst;
    long sxy;
    s32 otz;

    pRef = D_80092830;
    vec.vx = *(u16*)(pSrc + 0) - *(u16*)(pRef + 0);
    vec.vy = *(u16*)(pSrc + 2) - *(u16*)(pRef + 2);
    vec.vz = *(u16*)(pSrc + 4) - *(u16*)(pRef + 4);
    gte_ldv0(&vec);
    gte_rtps();
    pDst = pSrc + (D_800928A0 * 0x10 + 0x10);
    gte_stsxy2(pDst + 8);
    gte_stszotz(&sxy);
    otz = sxy;
    func_80031804(pOT + ((otz >> 2) << 2), pDst);
}
