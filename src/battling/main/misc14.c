#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/model.h"

// .bss/.sdata of the overlay (supplied by splat's asm segments)
extern u8 g_BattleDrawBufferIndex;
extern u32 D_80092818[];
extern s32 D_80092810;
extern s32 D_80092820;
extern u32 D_800928E4;
extern s32 g_GfxOTDepthShift;
extern s32 D_80050104;
extern s32 D_80091C30;
extern s32 D_80096D88;
extern s32 D_80096D8C;
extern s32 D_80092914;
extern u8 *D_80092938;
extern RECT *D_80092868;

extern void func_80030A30(s32 index, void *pLight);
extern u32 func_800405E4(void);
extern void func_8008BB3C(void *pCtx);

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u16 pad12;
} Misc14Entry; // Size: 0x14

void BattleEnableBackColor(void) {
    D_80092810 = 1;
}

void BattleDisableBackColor(void) {
    D_80092810 = 0;
}

extern void ModelRenderPacket(void *pPart, void *pPacket, u_long *pOT, s32 flags);

// One drawn model part (the record src/battling/main/misc13.c declares as
// Misc13ModelPart): the part's flags and colours, the packet area to fill for
// each draw buffer, and the model part itself.
typedef struct {
    /* 0x00 */ u32 flags;
    /* 0x04 */ s32 unk04;
    /* 0x08 */ u8* pPacket[2]; // packet area, one per draw buffer
    /* 0x10 */ s32 unk10;
    /* 0x14 */ ModelPart* pModelPart;
    /* 0x18 */ u8 r;
    /* 0x19 */ u8 g;
    /* 0x1A */ u8 b;
    /* 0x1B */ u8 pad1B;
    /* 0x1C */ s32 unk1C;
} Misc14ModelPart; // Size: 0x20

void BattleRenderActorModel(Misc14ModelPart *pPart) {
    D_80050104 = 0;
    if (D_80092810 != 0) {
        if (pPart->flags & 0x10) {
            gte_SetBackColor(pPart->r, pPart->g, pPart->b);
        } else {
            gte_SetBackColor(0x40, 0x40, 0x40);
        }
    }
    ModelRenderPacket(pPart->pModelPart, pPart->pPacket[g_BattleDrawBufferIndex], (u_long *)D_800928E4, pPart->unk04);
}

// The colour word BattleSetColorIfChanged copies in one move; the fourth byte
// of the destination word is the prim code and is written separately.
typedef struct {
    u8 b[4];
} Misc14Quad; // Size: 0x4

void BattleSetColorIfChanged(u8 *pSrc) {
    u8 *pDst = (u8 *)&D_80092818[g_BattleDrawBufferIndex];

    if (pSrc[0] == pDst[0] && pSrc[1] == pDst[1] && pSrc[2] == pDst[2]) {
        D_80092914 = 0;
    } else {
        D_80092914 = 1;
        *(Misc14Quad *)pDst = *(Misc14Quad *)pSrc;
        pDst[3] = 0x20;
    }
}

void BattleSetEntryColor(u8 *pActor) {
    s32 index = g_BattleDrawBufferIndex;
    u8 *pModel = *(u8 **)(pActor + 4);
    u8 *pData = *(u8 **)(pModel + 0xC);
    s32 count = *(s16 *)(pData + 2);
    Misc14Entry *pEntry = *(Misc14Entry **)(pData + index * 4 + 0x10);
    u32 value = D_80092818[index];

    while (--count != -1) {
        pEntry->unk4 = value;
        pEntry++;
    }
}

// The block a node composes its placement into, named by the node's 0x8C
// pointer: the matrix the node builds at 0x4C and the two it is multiplied
// into, 0x6C first.
typedef struct {
    /* 0x00 */ u8 pad00[0x0C];
    /* 0x0C */ MATRIX unk0C;
    /* 0x2C */ u8 pad2C[0x20];
    /* 0x4C */ MATRIX unk4C;
    /* 0x6C */ MATRIX unk6C;
} Misc14ModelMatrices; // Size: 0x8C

// A model node (the record src/battling/main/misc13.c declares as Misc13Node):
// a state tag, a payload whose type depends on the state, the matrices this
// unit composes every frame, the block they are composed into and two children.
typedef struct Misc14ModelNode {
    /* 0x00 */ s32 state;
    /* 0x04 */ void* pData;
    /* 0x08 */ void (*pUpdate)(void);
    /* 0x0C */ MATRIX unk0C;
    /* 0x2C */ SVECTOR unk2C;
    /* 0x34 */ VECTOR unk34;
    /* 0x44 */ SVECTOR unk44;
    /* 0x4C */ MATRIX unk4C;
    /* 0x6C */ MATRIX unk6C;
    /* 0x8C */ Misc14ModelMatrices* pMatrices;
    /* 0x90 */ struct Misc14ModelNode* pChildA;
    /* 0x94 */ struct Misc14ModelNode* pChildB;
} Misc14ModelNode; // Size: 0x9C

// The state-5 payload as this function reads it (the layout
// src/battling/main/misc15.c calls a ModelBlock): a type word and the record
// the shadow pass is run from, whose vertex area is at +0x8 and whose source
// data is at +0xC.
typedef struct {
    /* 0x00 */ u8 pad00[0x8];
    /* 0x08 */ u8* pVerts;
    /* 0x0C */ void* pData;
} Misc14ShadowRecord; // Size: 0x10

typedef struct {
    /* 0x00 */ s32 type;
    /* 0x04 */ Misc14ModelMatrices* pMatrices;
    /* 0x08 */ u32 pad08;
    /* 0x0C */ Misc14ShadowRecord* pShadow;
} Misc14State5Data; // Size: 0x10

extern s32 D_8009289C;
// The three words the node's translation is offset by; src/battling/main/misc16.c
// declares them the same way (one global per word, read through .vx).
extern VECTOR D_80096FA8;
extern VECTOR D_80096FAC;
extern VECTOR D_80096FB0;
extern void func_80030B14(MATRIX* pMatrix);
extern void func_800731F8(MATRIX* pMatrix, SVECTOR* pVector);
extern void BattleProjectShadowVerts(void* pSource, u8* pVerts);

void BattleUpdateNodeTree(Misc14ModelNode* pNode) {
    if (pNode->pUpdate != NULL) {
        pNode->pUpdate();
    }
    switch (pNode->state) {
    case 2:
        RotMatrix(&pNode->unk44, &pNode->unk0C);
        if (D_8009289C != 0) {
            MulMatrix0(&pNode->pMatrices->unk0C, &pNode->unk0C, &pNode->unk6C);
        } else {
            pNode->unk6C = pNode->unk0C;
        }
        func_800731F8(&pNode->unk0C, (SVECTOR*)((u8*)pNode->pData + 0x10));
        pNode->unk4C = pNode->unk0C;
        pNode->unk4C.t[2] = 0;
        pNode->unk4C.t[1] = 0;
        pNode->unk4C.t[0] = 0;
        pNode->unk0C.t[0] = pNode->unk34.vx - D_80096FA8.vx;
        pNode->unk0C.t[1] = pNode->unk34.vy - D_80096FAC.vx;
        pNode->unk0C.t[2] = pNode->unk34.vz - D_80096FB0.vx;
        CompMatrix(&pNode->pMatrices->unk0C, &pNode->unk0C, &pNode->unk0C);
        break;
    case 0:
    case 1:
        if (pNode->pMatrices != NULL) {
            pNode->unk34.vx = pNode->unk2C.vx;
            pNode->unk34.vy = pNode->unk2C.vy;
            pNode->unk34.vz = pNode->unk2C.vz;
            RotMatrix(&pNode->unk44, &pNode->unk0C);
            TransMatrix(&pNode->unk0C, &pNode->unk34);
            MulMatrix0(&pNode->pMatrices->unk6C, &pNode->unk0C, &pNode->unk6C);
            CompMatrix(&pNode->pMatrices->unk4C, &pNode->unk0C, &pNode->unk4C);
            CompMatrix(&pNode->pMatrices->unk0C, &pNode->unk0C, &pNode->unk0C);
        }
        if (pNode->state == 1 && (*(u32*)pNode->pData & 1) == 0) {
            func_80030B14(&pNode->unk6C);
            gte_SetRotMatrix_gpr(&pNode->unk0C);
            BattleRenderActorModel(pNode->pData);
        }
        break;
    case 5:
        if (((Misc14State5Data*)pNode->pData)->type == 1) {
            pNode->unk0C = ((Misc14State5Data*)pNode->pData)->pMatrices->unk4C;
            gte_SetRotMatrix_gpr(&pNode->unk0C);
            if (D_80092914 != 0) {
                BattleSetEntryColor((u8*)pNode);
            }
            BattleProjectShadowVerts(((Misc14State5Data*)pNode->pData)->pShadow->pData,
                                     ((Misc14State5Data*)pNode->pData)->pShadow->pVerts);
        }
        break;
    }
    if (pNode->pChildB != NULL) {
        BattleUpdateNodeTree(pNode->pChildB);
    }
    if (pNode->pChildA != NULL) {
        BattleUpdateNodeTree(pNode->pChildA);
    }
}

void BattleSetModelLights(Misc14ModelNode **pNodes) {
    func_80030A30(0, pNodes[0]->pData);
    func_80030A30(1, pNodes[1]->pData);
    func_80030A30(2, pNodes[2]->pData);
}

// The per-frame display work of one battle actor group (the pointer the group
// carries at +0x284): a double-buffered ordering table and prim area, the two
// draw modes set up for it, and the prim queued when its 0x10 flag is set.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ u_long* pOt[2];     // ordering table, one per draw buffer
    /* 0x0C */ u_long* pPrim[2];   // prim area, one per draw buffer
    /* 0x14 */ s16 count;          // ordering table length in words
    /* 0x16 */ u8 flags;
    /* 0x17 */ u8 unk17;
    /* 0x18 */ DR_MODE drawModes[2][2]; // [half][draw buffer]
    /* 0x48 */ u8 prim48[2][0x10];
} Misc14DisplayWork; // Size: 0x68

extern s32 D_80091C30;

void BattleClearOrderingTable(Misc14DisplayWork *pWork) {
    s32 n = pWork->count;

    ClearOTagR(pWork->pOt[g_BattleDrawBufferIndex], n);
    D_800928E4 = (u32)pWork->pOt[g_BattleDrawBufferIndex];
    g_GfxOTDepthShift = pWork->unk17;
}

void BattleSetOrderingTableTarget(s32 arg0) {
    D_80091C30 = arg0;
}

void BattleCaptureTimerCount(void) {
    D_80092820 = GetRCnt(0xF2000001);
}

void BattleCompactOrderingTable(s32 arg0) {
    Misc14DisplayWork *pWork = (Misc14DisplayWork *)D_80091C30;
    s32 timeout;
    s32 start;
    s32 diff;
    s32 i;
    u32 *pPrim;
    u32 *pNext;

    if (pWork == NULL) {
        return;
    }
    start = D_80092820;
    D_80091C30 = 0;
    if (arg0 != 0) {
        timeout = ((arg0 << 4) - arg0) << 4;
    } else {
        timeout = 0xC0;
    }
    i = pWork->count - 1;
    if (i < 4) {
        return;
    }
    do {
        diff = GetRCnt(0xF2000001) - start;
        if (diff < 0) {
            diff += 0x10000;
        }
        if (timeout < diff) {
            break;
        }
        pPrim = (u32 *)pWork->pOt[g_BattleDrawBufferIndex][i];
        pNext = (u32 *)((*pPrim & 0xFFFFFF) + 0x80000000);
        if (*(u8 *)((u8 *)pNext + 3) == 0) {
            do {
                if (i < 5) {
                    break;
                }
                pNext = (u32 *)((*pNext & 0xFFFFFF) + 0x80000000);
                i--;
            } while (*(u8 *)((u8 *)pNext + 3) == 0);
            *pPrim = (*pPrim & 0xFF000000) | ((u32)pNext & 0xFFFFFF);
        }
        i--;
    } while (i >= 4);
}

extern void func_800453E8(DR_MODE *pDrmode, RECT *pRect);
extern void func_8004546C(DR_MODE *pDrmode, RECT *pRect);

void BattleAttachActorPrims(Misc14DisplayWork *pWork) {
    BattleSetOrderingTableTarget((s32)pWork);
    AddPrims(D_80092938, pWork->pPrim[g_BattleDrawBufferIndex], pWork->pOt[g_BattleDrawBufferIndex]);
    if ((pWork->flags & 4) == 0) {
        func_800453E8(&pWork->drawModes[0][g_BattleDrawBufferIndex], D_80092868);
    }
    if ((pWork->flags & 8) == 0) {
        func_8004546C(&pWork->drawModes[1][g_BattleDrawBufferIndex], D_80092868 + 1);
    }
    if (pWork->flags & 0x10) {
        AddPrim(D_80092938, pWork->prim48[g_BattleDrawBufferIndex]);
    }
    AddPrim(D_80092938, &pWork->drawModes[1][g_BattleDrawBufferIndex]);
    AddPrim(D_80092938, &pWork->drawModes[0][g_BattleDrawBufferIndex]);
}

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ u32 unk18;
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ u32 unk20;
    /* 0x24 */ u32 unk24;
} Misc14Reloc; // Size: 0x28

extern void BattleLoadImageWithTpageClut(void *pImg);

Misc14Reloc *BattleRelocateBlockPointers(Misc14Reloc *pData) {
    s32 delta = (s32)pData - (s32)pData->unk1C;
    u32 *pSlot;
    u32 i;

    pData->unk1C = (u32)pData;
    pData->unk0 += delta;
    pData->unk4 += delta;
    pData->unk10 += delta;
    pData->unk14 += delta;
    pData->unk18 += delta;
    pData->unk20 += delta;
    pData->unk24 += delta;
    if (pData->unkC != 0) {
        pData->unkC += delta;
        BattleLoadImageWithTpageClut((void *)pData->unkC);
    }
    if (pData->unk8 != 0) {
        pData->unk8 += delta;
        for (i = 1; i < ((u32 *)pData->unk8)[0] + 1; i++) {
            pSlot = (u32 *)(i * 4 + pData->unk8);
            if (*pSlot != 0) {
                *pSlot += delta;
            }
        }
    }
    return pData;
}

void BattleLoadRelocatedArchiveEntry(s32 entryIndex) {
    void *pBuffer;

    HeapSetCurrentContentType(0xA);
    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 0);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    ArchiveCdDataSync(0);
    BattleRelocateBlockPointers(pBuffer);
}

void BattleResetPartAnim(u8 *pActor) {
    Misc14Entry *pEntry = *(Misc14Entry **)(pActor + 8);
    s32 i;

    *(s16 *)(pActor + 0x10) = 0;
    *(s16 *)(pActor + 0x12) = 0;
    for (i = 0; i < *(s16 *)(*(u8 **)pActor + 6); i++) {
        pEntry->unkC = 0;
        pEntry->unkE = 0;
        pEntry->unk4 = pEntry->unk0;
        pEntry->unk10 = 0;
        pEntry++;
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B13C);

extern u32 ModelResolvePointers(void *pModel);
extern void *BattleCreateObject(void);
extern void BattleAppendListEntry(void *pList, void *pNode);
extern void BattleSetNodeModelPart(void *pNode, void *pData);
extern void BattleSetNodeEntryList(void *pNode, void *pData);
extern void *BattleCreateEntryList(void);
extern void *BattleCreateModelPart(void);
extern void BattleAttachModelPart(void *pPart, void *pModelPart);
extern void func_8008B13C(void *pA, void *pB, void *pC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B38C);

/* Best attempt: objdiff 77.11%, 608 bytes against 592. Structure, call order,
   both loops and every offset match; three stack homes instead of retail's two
   (mine: pData 0x10, count 0x18, pNode 0x20; retail: pNode 0x10, pOther 0x18),
   so the frame is 0x60 against 0x58 and every s-register is one higher. Retail
   spills pOther (loaded once, read twice at the end) and keeps pData, count,
   pBuf, pScale, pCur, pPart, pSlot and i in registers.
   Two things did help and should be kept: reading *(s16 *)(pCur + 2) inline
   twice instead of through an `idx` local (71.54 -> 77.11: retail reloads the
   field after the BattleSetNodeModelPart call), and *(s16 *)pCur inline. Dead ends:
   pSlot as ((u8 **)pBuf)[i] (71.80), every pointer declared before use (71.54). */
/*
void *func_8008B38C(u8 *pArg) {
    u8 *pData = *(u8 **)(pArg + 4);
    u8 *pList = *(u8 **)pArg;
    u8 *pOther = *(u8 **)(pArg + 8);
    u32 count = *(u32 *)pList;
    u8 *pCur = pList + 4;
    u8 *pScale;
    u8 *pNode;
    u8 *pBuf;
    u8 *pPart;
    u8 *pListArg;
    u8 **pSlot;
    u32 i;

    ModelResolvePointers(pData);
    HeapSetCurrentContentType(0x12);
    pBuf = HeapAlloc(count * 4, 0);
    pScale = BattleCreateEntryList();
    pNode = BattleCreateObject();
    BattleSetNodeEntryList(pNode, pScale);
    *(u8 **)(pScale + 4) = pBuf;
    *(u16 *)pScale = count;
    *(u8 **)(pScale + 0x18) = pCur;
    pSlot = (u8 **)pBuf;
    for (i = 0; i < count; i++) {
        pPart = BattleCreateObject();
        *pSlot = pPart;
        if (*(s16 *)(pCur + 2) != -1) {
            pListArg = BattleCreateModelPart();
            BattleSetNodeModelPart(pPart, pListArg);
            BattleAttachModelPart(pListArg, pData + (*(s16 *)(pCur + 2) * 8 - *(s16 *)(pCur + 2)) * 8 + 0x10);
        }
        if (*(s16 *)pCur == -1) {
            pListArg = pNode;
        } else {
            pListArg = *(u8 **)(pBuf + *(s16 *)pCur * 4);
        }
        BattleAppendListEntry(pListArg, pPart);
        *(u16 *)(pPart + 0x44) = *(u16 *)(pCur + 4);
        *(u16 *)(pPart + 0x46) = *(u16 *)(pCur + 6);
        *(u16 *)(pPart + 0x48) = *(u16 *)(pCur + 8);
        *(u16 *)(pPart + 0x2C) = *(u16 *)(pCur + 0xA);
        *(u16 *)(pPart + 0x2E) = *(u16 *)(pCur + 0xC);
        *(u16 *)(pPart + 0x30) = *(u16 *)(pCur + 0xE);
        pSlot++;
        pCur += 0x10;
    }
    if (pOther != 0) {
        HeapSetCurrentContentType(0x11);
        pBuf = HeapAlloc(*(u32 *)pOther * 0x14, 0);
        *(u8 **)(pScale + 8) = pBuf;
        *(u16 *)(pScale + 2) = *(u32 *)pOther;
        for (i = 0; i < *(u32 *)pOther; i++) {
            if (*(u32 *)(pOther + 4) != 0) {
                func_8008B13C(*(void **)(pOther + 4), pBuf, pNode);
            } else {
                *(u32 *)pBuf = 0;
            }
            pBuf += 0x14;
            pOther += 4;
        }
    }
    return pNode;
}
*/

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} Misc14AnimCfg; // Size: 0x8

typedef struct {
    /* 0x0 */ u16 *unk0;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ s16 unk6;
} Misc14AnimEntry; // Size: 0x8

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u8 *unk4;
    /* 0x08 */ u16 *unk8;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s16 unk12;
} Misc14AnimTrack; // Size: 0x14

typedef struct {
    /* 0x00 */ Misc14AnimCfg *unk0;
    /* 0x04 */ Misc14AnimEntry *unk4;
    /* 0x08 */ Misc14AnimTrack *unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s16 unk12;
} Misc14Anim; // Size: 0x14

extern s32 func_8008B730(Misc14Anim *pAnim, s32 arg1, s32 arg2);

void BattleSetAnimPosition(void *pA, s32 arg1) {
    func_8008B730(pA, arg1, 1);
}

s16 BattleInterpolateAngle(s32 arg0, s32 arg1, s32 arg2) {
    s32 value;
    s32 diff;

    arg0 &= 0xFFF;
    diff = (arg0 - arg1) & 0xFFF;
    if (diff != 0) {
        if (diff < 0x800) {
            value = arg0 - diff / arg2;
        } else {
            value = arg0 + (0x1000 - diff) / arg2;
        }
    } else {
        value = arg0;
    }
    return value;
}

/* Parked: best objdiff 72.62%, 228 bytes (original 216). The instruction
   multiset and both clamp tests (slti 0x801 keep-if-0x800, slti 0x800 clamp)
   are right; the miss is allocation. Retail keeps var in $s0 with an
   out-of-place update (subu v0,s0,s2 then move s0,v0) and arg1 in $s1; gcc
   keeps the saved arg0 in $s0 and var in $a0, so it saves arg0 across the
   rand() call and updates var in place. Shapes tried: var as s16 (67.25%: right
   copies, var in $a0), var as s32 (72.62%: in-place updates, var coalesced),
   explicit (s16)arg0 cast, diff before var, var -= arg2 vs var = arg0 - arg2,
   two s16 locals for the clamp (adds a fourth s-reg, 56.25%). */
/*
extern s32 rand(void);

s16 func_8008B650(s32 arg0, s32 arg1, s32 arg2) {
    s16 var;
    s32 diff;

    diff = (arg0 - arg1) & 0xFFF;
    var = arg0;
    if (diff != 0) {
        if (diff == 0x800) {
            if (rand() & 1) {
                var -= arg2;
                if ((((s16)var - (s16)arg1) & 0xFFF) > 0x800) {
                    var = arg1;
                }
            } else {
                var += arg2;
                if ((((s16)var - (s16)arg1) & 0xFFF) >= 0x800) {
                    var = arg1;
                }
            }
        } else if (diff < 0x800) {
            var += arg2;
            if ((((s16)var - (s16)arg1) & 0xFFF) >= 0x800) {
                var = arg1;
            }
        } else {
            var -= arg2;
            if ((((s16)var - (s16)arg1) & 0xFFF) > 0x800) {
                var = arg1;
            }
        }
    }
    return var;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B650);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B730);

/* Best attempt: objdiff 91.66%, 760 bytes against 756 - one instruction too many.
   Whole structure, all four loops, the script decoder and the unused 0x10 stack
   slot are right (keep the pad[4]: without it the frame is 0x50, not retail's
   0x60). Also confirmed: the arg1 == 0 early exit returns whatever is in $v0 (the
   config pointer), so it is not `return 0/1` - it is the enclosing
   `if (arg1 != 0) { ...; return X; }` with no trailing return, which is why that
   path has a nop, not a li, in the beqz delay slot.
   What is still wrong:
   1. The three loops emit `lw count; nop; lh count; addiu i,1` where retail has
      `lw count; addiu i,1; lh count` (maspsx then needs no nop). Tried i++/++i
      and pEntry++/++pEntry - no change. Next: `i = 0; while (i < count) { ...;
      i++; }` so the increment is body code.
   2. The script decoder needs `move v1,v0` after the `lb` (retail keeps the char
      in v1 and the load in v0); gcc loads straight into v1 here.
   3. The final `return pAnim->unk12 == pAnim->unk0->unk0 && arg2 == 1;` costs an
      extra `move v0,a0` (gcc puts the sltiu in a0) and picks v1/v0 where retail
      picks a0/v1. */
/*
s32 func_8008B730(Misc14Anim *pAnim, s32 arg1, s32 arg2) {
    Misc14AnimEntry *pEntry;
    Misc14AnimTrack *pTrack;
    u8 *pScript;
    s32 c;
    s32 i;
    s32 j;
    s32 pos;
    s32 pad[4];

    if (pAnim->unk0 == NULL) {
        return 1;
    }
    if (arg1 != 0) {
    pos = pAnim->unk12;
    if (pos + arg1 > pAnim->unk0->unk0) {
        arg1 = pAnim->unk0->unk0 - pos;
    }
    pAnim->unk12 = pos + arg1;
    arg2 -= arg1;
    if (arg2 <= 0) {
        arg2 = 1;
    }
    pEntry = pAnim->unk4;
    if (arg2 == 1) {
        for (i = 0; i < pAnim->unk0->unk4; i++) {
            *(u16 *)pEntry->unk0 = pEntry->unk4;
            ++pEntry;
        }
    } else {
        for (i = 0; i < pAnim->unk0->unk4; i++) {
            u16 value = *pEntry->unk0;

            if (pEntry->unk6 != 0) {
                *pEntry->unk0 = BattleInterpolateAngle((s16)value, (s16)pEntry->unk4, arg2);
            } else {
                *pEntry->unk0 = value + ((s16)pEntry->unk4 - (s16)value) / arg2;
            }
            ++pEntry;
        }
    }
    pTrack = pAnim->unk8;
    for (i = 0; i < pAnim->unk0->unk6; i++) {
        for (j = 0; j < arg1; j++) {
            if (pTrack->unkC != 0) {
                pTrack->unkC--;
            } else {
                pScript = pTrack->unk4;
                pTrack->unk4 = pScript + 1;
                c = *(s8 *)pScript;
                if (c & 0x80) {
                    s32 bits = c & 0x3F;

                    if (c & 0x40) {
                        pTrack->unk4 = pScript + 2;
                        pTrack->unk10 = bits | ((s8)pScript[1] << 6);
                    } else {
                        pTrack->unkC = bits;
                    }
                } else {
                    pTrack->unk10 = (s8)(c << 1) >> 1;
                }
            }
            pTrack->unkE += pTrack->unk10;
        }
        if (arg2 == 1) {
            *pTrack->unk8 = pTrack->unkE;
        } else {
            u16 value = *pTrack->unk8;

            if (pTrack->unk12 != 0) {
                *pTrack->unk8 = BattleInterpolateAngle((s16)value, (s16)pTrack->unkE, arg2);
            } else {
                *pTrack->unk8 = value + ((s16)pTrack->unkE - (s16)value) / arg2;
            }
        }
        pTrack++;
    }
    return pAnim->unk12 == pAnim->unk0->unk0 && arg2 == 1;
    }
}
*/

typedef struct {
    /* 0x00 */ u32 unk0[0x20];
    /* 0x80 */ u32 unk80;
} Misc14Work; // Size: 0x84

Misc14Work *BattleCreateWorkBlock(u32 arg0, u32 arg1, u32 arg2, s32 arg3) {
    Misc14Work *p;
    s32 i;
    u32 value;

    HeapSetCurrentContentType(3);
    p = HeapAlloc(0x84, 2);
    for (i = 0x1F; i >= 0; i--) {
        p->unk0[i] = 0;
    }
    p->unk80 = arg2;
    p->unk0[0x1C] = func_800405E4();
    value = p->unk80 + arg3 * 4;
    p->unk0[0x1F] = arg0;
    p->unk0[4] = arg1;
    p->unk0[0x1D] = value;
    p->unk0[0x1E] = value;
    func_8008BB3C(p);
    return p;
}

void BattleFreeWorkBuffer(void *pMem) {
    HeapFree(pMem);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB00);

/* Best attempt: objdiff 94.29% (same under gcc-2.6.0, gcc-2.7.2-cdk and -O1/-O3).
   Only the temporaries differ: the original loads into $t0/$t1, gcc into
   $v0/$v1; the instruction sequence and both symbols are identical. A struct
   pointer, an 8-byte struct copy (70.71%, it also bases the second load off one
   symbol) and a 2-minute permuter run (nothing better) all leave the allocation
   alone. Its sibling func_8008BB1C has the same $t0-against-$v0 tie, so both are
   one allocator question, not two. */
/*
void func_8008BB00(Misc14Entry *pDst) {
    pDst->unk0 = D_80096D88;
    pDst->unk4 = D_80096D8C;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB1C);

/* Best attempt: objdiff 97.50% - identical except that the original keeps the
   one reused temporary in $t0, gcc in $v0. Reusing a single temp is what
   reproduces the interleaved load/store order; two separate statements make gcc
   hoist the second load (73.12%). Tried: s32 instead of u32 temp, volatile
   stores, an 8-byte struct copy, a pointer local (all 97.5 or worse), -O1/-O3
   (97.5) and a 4-minute permuter run (no improvement). */
/*
void func_8008BB1C(Misc14Entry *pSrc) {
    u32 value;

    value = pSrc->unk0;
    D_80096D88 = value;
    value = pSrc->unk4;
    D_80096D8C = value;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB3C);
