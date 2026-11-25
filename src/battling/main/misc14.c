#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/model.h"

// .bss/.sdata of the overlay (supplied by splat's asm segments)
extern u8 g_BattleDrawBufferIndex;
extern u32 g_BattleDrawBufferColor[];
extern s32 g_BattleBackColorEnabled;
extern s32 g_BattleTimerStartCount;
extern u32 g_BattleCurrentOrderingTable;
extern s32 g_GfxOTDepthShift;
extern s32 g_ModelRenderTestFlags;
extern s32 g_BattlePendingDisplayWork;
extern s32 D_80096D88;
extern s32 D_80096D8C;
extern s32 g_BattleEntryColorChanged;
extern u8 *g_BattleRootOrderingTable;
extern RECT *g_BattleDrawSlot;

extern void func_80030A30(s32 index, void *pLight);
extern u32 GetGp(void);
extern void func_8008BB3C(void *pCtx);

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 color; // the draw-buffer colour word BattleSetEntryColor copies into every entry
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u16 pad12;
} Misc14Entry; // Size: 0x14

void BattleEnableBackColor(void) {
    g_BattleBackColorEnabled = 1;
}

void BattleDisableBackColor(void) {
    g_BattleBackColorEnabled = 0;
}

extern void ModelRenderPacket(void *pPart, void *pPacket, u_long *pOT, s32 flags);

// One drawn model part (the record src/battling/main/misc13.c declares as
// Misc13ModelPart): the part's flags and colours, the packet area to fill for
// each draw buffer, and the model part itself.
typedef struct {
    /* 0x00 */ u32 flags;
    /* 0x04 */ s32 renderMode;
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
    g_ModelRenderTestFlags = 0;
    if (g_BattleBackColorEnabled != 0) {
        if (pPart->flags & 0x10) {
            gte_SetBackColor(pPart->r, pPart->g, pPart->b);
        } else {
            gte_SetBackColor(0x40, 0x40, 0x40);
        }
    }
    ModelRenderPacket(pPart->pModelPart, pPart->pPacket[g_BattleDrawBufferIndex], (u_long *)g_BattleCurrentOrderingTable, pPart->renderMode);
}

// The colour word BattleSetColorIfChanged copies in one move; the fourth byte
// of the destination word is the prim code and is written separately.
typedef struct {
    u8 b[4];
} Misc14Quad; // Size: 0x4

void BattleSetColorIfChanged(u8 *pSrc) {
    u8 *pDst = (u8 *)&g_BattleDrawBufferColor[g_BattleDrawBufferIndex];

    if (pSrc[0] == pDst[0] && pSrc[1] == pDst[1] && pSrc[2] == pDst[2]) {
        g_BattleEntryColorChanged = 0;
    } else {
        g_BattleEntryColorChanged = 1;
        *(Misc14Quad *)pDst = *(Misc14Quad *)pSrc;
        pDst[3] = 0x20;
    }
}

// The block a node composes its placement into, named by the node's 0x8C
// pointer: the rotation matrix the node builds at 0x4C and the two it is
// multiplied into, 0x6C first.
typedef struct {
    /* 0x00 */ u8 pad00[0x0C];
    /* 0x0C */ MATRIX localMatrix;
    /* 0x2C */ u8 pad2C[0x20];
    /* 0x4C */ MATRIX rotMatrix;
    /* 0x6C */ MATRIX worldMatrix;
} Misc14ModelMatrices; // Size: 0x8C

// A model node (the record src/battling/main/misc13.c declares as Misc13Node):
// a state tag, a payload whose type depends on the state, the matrices this
// unit composes every frame, the block they are composed into and two children.
typedef struct Misc14ModelNode {
    /* 0x00 */ s32 state;
    /* 0x04 */ void* pData;
    /* 0x08 */ void (*pUpdate)(void);
    /* 0x0C */ MATRIX localMatrix;
    /* 0x2C */ SVECTOR sourcePos;
    /* 0x34 */ VECTOR position;
    /* 0x44 */ SVECTOR rotation;
    /* 0x4C */ MATRIX rotMatrix;
    /* 0x6C */ MATRIX worldMatrix;
    /* 0x8C */ Misc14ModelMatrices* pMatrices;
    /* 0x90 */ struct Misc14ModelNode* pChildA;
    /* 0x94 */ struct Misc14ModelNode* pChildB;
} Misc14ModelNode; // Size: 0x9C

// The render record a state-5 payload points at (the record
// src/battling/main/misc15.c declares as RenderRecord): the vertex area and
// source data the shadow pass runs from, then one buffer of 0x14-byte entries
// per draw buffer, whose word at +0x4 carries the colour they are drawn with.
typedef struct {
    /* 0x00 */ undefined16 unk0;
    /* 0x02 */ s16 entryCount; // entries in each of the buffers at 0x10
    /* 0x04 */ u8 pad04[0x4];
    /* 0x08 */ u8* pVerts;
    /* 0x0C */ void* pData;
    /* 0x10 */ Misc14Entry* pEntries[2]; // one entry buffer per draw buffer
} Misc14RenderRecord; // Size: 0x18

typedef struct {
    /* 0x00 */ s32 type;
    /* 0x04 */ Misc14ModelMatrices* pMatrices;
    /* 0x08 */ u32 pad08;
    /* 0x0C */ Misc14RenderRecord* pRender;
} Misc14State5Data; // Size: 0x10

void BattleSetEntryColor(Misc14ModelNode *pNode) {
    s32 index = g_BattleDrawBufferIndex;
    Misc14RenderRecord *pRender = ((Misc14State5Data *)pNode->pData)->pRender;
    s32 count = pRender->entryCount;
    Misc14Entry *pEntry = pRender->pEntries[index];
    u32 value = g_BattleDrawBufferColor[index];

    while (--count != -1) {
        pEntry->color = value;
        pEntry++;
    }
}

extern s32 g_BattleMultiplyNodeMatrices;
// The three words the node's translation is offset by; src/battling/main/misc16.c
// declares them the same way (one global per word, read through .vx).
extern VECTOR g_BattleViewOrigin;
extern VECTOR g_BattleViewOriginY;
extern VECTOR g_BattleViewOriginZ;
extern void func_80030B14(MATRIX* pMatrix);
extern void func_800731F8(MATRIX* pMatrix, SVECTOR* pVector);
extern void BattleProjectShadowVerts(void* pSource, u8* pVerts);

void BattleUpdateNodeTree(Misc14ModelNode* pNode) {
    if (pNode->pUpdate != NULL) {
        pNode->pUpdate();
    }
    switch (pNode->state) {
    case 2:
        RotMatrix(&pNode->rotation, &pNode->localMatrix);
        if (g_BattleMultiplyNodeMatrices != 0) {
            MulMatrix0(&pNode->pMatrices->localMatrix, &pNode->localMatrix, &pNode->worldMatrix);
        } else {
            pNode->worldMatrix = pNode->localMatrix;
        }
        func_800731F8(&pNode->localMatrix, (SVECTOR*)((u8*)pNode->pData + 0x10));
        pNode->rotMatrix = pNode->localMatrix;
        pNode->rotMatrix.t[2] = 0;
        pNode->rotMatrix.t[1] = 0;
        pNode->rotMatrix.t[0] = 0;
        pNode->localMatrix.t[0] = pNode->position.vx - g_BattleViewOrigin.vx;
        pNode->localMatrix.t[1] = pNode->position.vy - g_BattleViewOriginY.vx;
        pNode->localMatrix.t[2] = pNode->position.vz - g_BattleViewOriginZ.vx;
        CompMatrix(&pNode->pMatrices->localMatrix, &pNode->localMatrix, &pNode->localMatrix);
        break;
    case 0:
    case 1:
        if (pNode->pMatrices != NULL) {
            pNode->position.vx = pNode->sourcePos.vx;
            pNode->position.vy = pNode->sourcePos.vy;
            pNode->position.vz = pNode->sourcePos.vz;
            RotMatrix(&pNode->rotation, &pNode->localMatrix);
            TransMatrix(&pNode->localMatrix, &pNode->position);
            MulMatrix0(&pNode->pMatrices->worldMatrix, &pNode->localMatrix, &pNode->worldMatrix);
            CompMatrix(&pNode->pMatrices->rotMatrix, &pNode->localMatrix, &pNode->rotMatrix);
            CompMatrix(&pNode->pMatrices->localMatrix, &pNode->localMatrix, &pNode->localMatrix);
        }
        if (pNode->state == 1 && (*(u32*)pNode->pData & 1) == 0) {
            func_80030B14(&pNode->worldMatrix);
            gte_SetRotMatrix_gpr(&pNode->localMatrix);
            BattleRenderActorModel(pNode->pData);
        }
        break;
    case 5:
        if (((Misc14State5Data*)pNode->pData)->type == 1) {
            pNode->localMatrix = ((Misc14State5Data*)pNode->pData)->pMatrices->rotMatrix;
            gte_SetRotMatrix_gpr(&pNode->localMatrix);
            if (g_BattleEntryColorChanged != 0) {
                BattleSetEntryColor(pNode);
            }
            BattleProjectShadowVerts(((Misc14State5Data*)pNode->pData)->pRender->pData,
                                     ((Misc14State5Data*)pNode->pData)->pRender->pVerts);
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
    /* 0x17 */ u8 depthShift;
    /* 0x18 */ DR_MODE drawModes[2][2]; // [half][draw buffer]
    /* 0x48 */ u8 prim48[2][0x10];
} Misc14DisplayWork; // Size: 0x68

extern s32 g_BattlePendingDisplayWork;

void BattleClearOrderingTable(Misc14DisplayWork *pWork) {
    s32 n = pWork->count;

    ClearOTagR(pWork->pOt[g_BattleDrawBufferIndex], n);
    g_BattleCurrentOrderingTable = (u32)pWork->pOt[g_BattleDrawBufferIndex];
    g_GfxOTDepthShift = pWork->depthShift;
}

void BattleSetOrderingTableTarget(s32 arg0) {
    g_BattlePendingDisplayWork = arg0;
}

void BattleCaptureTimerCount(void) {
    g_BattleTimerStartCount = GetRCnt(0xF2000001);
}

// The word an ordered prim starts with, as this walk reads it: the low 24 bits
// are the address of the next prim and byte 3 is the prim's code, zero on the
// terminator the walk stops at.
typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 code;
} Misc14PrimCode; // Size: 0x4

void BattleCompactOrderingTable(s32 arg0) {
    Misc14DisplayWork *pWork = (Misc14DisplayWork *)g_BattlePendingDisplayWork;
    s32 timeout;
    s32 start;
    s32 diff;
    s32 i;
    u32 *pPrim;
    u32 *pNext;

    if (pWork == NULL) {
        return;
    }
    start = g_BattleTimerStartCount;
    g_BattlePendingDisplayWork = 0;
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
        if (((Misc14PrimCode *)pNext)->code == 0) {
            do {
                if (i < 5) {
                    break;
                }
                pNext = (u32 *)((*pNext & 0xFFFFFF) + 0x80000000);
                i--;
            } while (((Misc14PrimCode *)pNext)->code == 0);
            *pPrim = (*pPrim & 0xFF000000) | ((u32)pNext & 0xFFFFFF);
        }
        i--;
    } while (i >= 4);
}

extern void func_8004546C(DR_MODE *pDrmode, RECT *pRect);

void BattleAttachActorPrims(Misc14DisplayWork *pWork) {
    BattleSetOrderingTableTarget((s32)pWork);
    AddPrims(g_BattleRootOrderingTable, pWork->pPrim[g_BattleDrawBufferIndex], pWork->pOt[g_BattleDrawBufferIndex]);
    if ((pWork->flags & 4) == 0) {
        SetDrawArea(&pWork->drawModes[0][g_BattleDrawBufferIndex], g_BattleDrawSlot);
    }
    if ((pWork->flags & 8) == 0) {
        func_8004546C(&pWork->drawModes[1][g_BattleDrawBufferIndex], g_BattleDrawSlot + 1);
    }
    if (pWork->flags & 0x10) {
        AddPrim(g_BattleRootOrderingTable, pWork->prim48[g_BattleDrawBufferIndex]);
    }
    AddPrim(g_BattleRootOrderingTable, &pWork->drawModes[1][g_BattleDrawBufferIndex]);
    AddPrim(g_BattleRootOrderingTable, &pWork->drawModes[0][g_BattleDrawBufferIndex]);
}

typedef struct {
    /* 0x00 */ u32 pPartList;  // count word followed by the 0x10-byte part descriptors
    /* 0x04 */ u32 pModelData; // base of the 0x38-byte model part records the descriptors index
    /* 0x08 */ u32 pPointerList;
    /* 0x0C */ u32 pImage;
    /* 0x10 */ u32 pScript;      // installed as the actor's pScript (func_80084C88)
    /* 0x14 */ u32 pPartTimings; // installed as the actor's pPartTimings
    /* 0x18 */ u32 pActions;     // installed as the actor's pActions
    /* 0x1C */ u32 pSelf;
    /* 0x20 */ u32 unk20;
    /* 0x24 */ u32 unk24;
} Misc14Reloc; // Size: 0x28

extern void BattleLoadImageWithTpageClut(void *pImg);

Misc14Reloc *BattleRelocateBlockPointers(Misc14Reloc *pData) {
    s32 delta = (s32)pData - (s32)pData->pSelf;
    u32 *pSlot;
    u32 i;

    pData->pSelf = (u32)pData;
    pData->pPartList += delta;
    pData->pModelData += delta;
    pData->pScript += delta;
    pData->pPartTimings += delta;
    pData->pActions += delta;
    pData->unk20 += delta;
    pData->unk24 += delta;
    if (pData->pImage != 0) {
        pData->pImage += delta;
        BattleLoadImageWithTpageClut((void *)pData->pImage);
    }
    if (pData->pPointerList != 0) {
        pData->pPointerList += delta;
        for (i = 1; i < ((u32 *)pData->pPointerList)[0] + 1; i++) {
            pSlot = (u32 *)(i * 4 + pData->pPointerList);
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

// One spec in the block that follows a Misc14AnimCfg header: the channel of a
// node a value is driven into (3..5 its rotation x/y/z, 6..8 its source
// position x/y/z), which node of the entry list it is, and the value.
typedef struct {
    /* 0x0 */ u8 channel;
    /* 0x1 */ u8 nodeIndex;
    /* 0x2 */ u16 value;
} Misc14AnimSpec; // Size: 0x4

// The animation a part plays (the record src/battling/main/misc15.c drives with
// func_8008B730): the per-part channel config, the entry list and the tracks.
typedef struct {
    /* 0x0 */ s16 length;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 entryCount;
    /* 0x6 */ s16 trackCount;
    /* 0x8 */ Misc14AnimSpec specs[1]; // entryCount entry specs, then trackCount track specs
} Misc14AnimCfg; // Size: 0xC

typedef struct {
    /* 0x0 */ u16 *pValue;
    /* 0x4 */ u16 target;
    /* 0x6 */ s16 isAngle;
} Misc14AnimEntry; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 *pScriptStart;
    /* 0x04 */ u8 *pScript;
    /* 0x08 */ u16 *pValue;
    /* 0x0C */ u16 wait;
    /* 0x0E */ u16 value;
    /* 0x10 */ u16 step;
    /* 0x12 */ s16 isAngle;
} Misc14AnimTrack; // Size: 0x14

typedef struct {
    /* 0x00 */ Misc14AnimCfg *pConfig;
    /* 0x04 */ Misc14AnimEntry *pEntries;
    /* 0x08 */ Misc14AnimTrack *pTracks;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s16 position;
} Misc14Anim; // Size: 0x14

void BattleResetPartAnim(Misc14Anim *pAnim) {
    Misc14AnimTrack *pTrack = pAnim->pTracks;
    s32 i;

    pAnim->unk10 = 0;
    pAnim->position = 0;
    for (i = 0; i < pAnim->pConfig->trackCount; i++) {
        pTrack->wait = 0;
        pTrack->value = 0;
        pTrack->pScript = pTrack->pScriptStart;
        pTrack->step = 0;
        pTrack++;
    }
}
// The 0x10-byte part descriptor table Misc14Reloc::pPartList points at: which
// node the part is appended to, which model part record it uses, and the angles
// it is placed with. -1 in either index means "the root node" and "no model
// part".
typedef struct {
    /* 0x0 */ s16 parent;
    /* 0x2 */ s16 modelIndex;
    /* 0x4 */ u16 rotation[3];
    /* 0xA */ u16 position[3];
} Misc14PartDesc; // Size: 0x10

// Misc14Reloc::pPartList: a count word and that many part descriptors.
typedef struct {
    /* 0x0 */ u32 count;
    /* 0x4 */ Misc14PartDesc descs[1];
} Misc14PartSet; // Size: 0x14

// Misc14Reloc::pPointerList: a count word and that many script pointers, each
// of which func_8008B13C decodes into one animation entry of the list.
typedef struct {
    /* 0x0 */ u32 count;
    /* 0x4 */ void *pScripts[1];
} Misc14ScriptTable; // Size: 0x8

// The entry list a model node owns: the node table built from the part
// descriptors, the animation entries built from the script table, and the
// descriptor table itself.
typedef struct {
    /* 0x00 */ u16 nodeCount;
    /* 0x02 */ s16 animCount;
    /* 0x04 */ Misc14ModelNode **pNodes;
    /* 0x08 */ Misc14Anim *pEntries;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ s16 scaleX;
    /* 0x12 */ s16 scaleY;
    /* 0x14 */ s16 scaleZ;
    /* 0x16 */ u16 pad16;
    /* 0x18 */ Misc14PartDesc *pPartDescs;
} Misc14EntryList; // Size: 0x1C

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B13C);

/* Best attempt: objdiff 97.42%, 588 bytes against retail's 592 - ONE instruction
   short, so it cannot link as it stands. Structure is settled and everything
   else is exact: the six-arm switch in each loop IS cross-jumped by 2.7.2 into
   retail's two shared store blocks (pValue then isAngle 1 for the rotation
   cases, 0 for the source-position cases), and the two jump tables land at +0
   and +0x18 of this unit's rodata range [0xBF8, .rodata, main/misc14] in
   address order, first table at +0, so this is the whole switch family of the
   unit in one function.
   Load-bearing spellings, keep them:
   - the parameter is the raw script/base pointer and pCfg is cast from it: the
     counts are re-read as `lh 4(s0)` / `lh 6(s0)` every iteration, and the
     additions go through the second copy in s1;
   - `pTrack` is computed BEFORE the entry loop and held across it (retail keeps
     it in t2 and uses it as the track cursor later); reloading
     `pAnim->pTracks` after the loop costs an instruction and a register (96.20);
   - `pAnim->pEntries = pEntry;` before the pTracks computation (source order is
     the store order: sw 4(s3) then sw 8(s3));
   Residual, one instruction: retail's entry copy chain is
   `move t1,a0 / move s0,t1 / ... / move s1,s0` (the copy of the script base is
   materialised AFTER the two calls, chained off the pCfg register), ours is
   `move s1,a0 / move s0,s1` at entry - one move fewer. Tried and measured:
   a `u8 *pBase = pScript;` local initialised at entry (95.37, coalesced into
   one register and the s-register set shifted), the same local assigned after
   the HeapAlloc (92.54, gcc gives it t4 because no call separates it from its
   use), the parameter typed Misc14AnimCfg* with `u8 *pScript = (u8 *)pCfg;`
   (95.11, coalesced), and both C operand orders for the two `addu`s (gcc
   normalises them: retail has value-first `addu v1,v1,s1`, we always get
   `addu v1,s1,v1`). The permuter could not run on this body at all - its
   randomizer fails with 'conversion to non-scalar type requested' before the
   first iteration.
   Round 429 dead ends: writing the `pCfg = (Misc14AnimCfg *)pScript;` assignment
   at its first use instead of in the declaration is bit-identical (97.42); the
   parameter typed Misc14AnimCfg* with the loop-2 base spelled `(u8 *)pCfg`
   inline is 93.84 / 576; a third base local assigned between the HeapAlloc and
   the pAnim->pEntries store is still 92.54 (the register set shifts: pAnim
   moves to $s2). So the third copy retail has between them is not reachable by
   declaring the base a second time.
   Round 442: re-measured the park body at 97.42 / 588 bytes (the note's number is
   current) and measured the one shape the note's 92.54 line left open - the
   chained form `u8 *pBase = pScript; Misc14AnimCfg *pCfg = (Misc14AnimCfg *)
   pBase; ... pBase = (u8 *)pCfg;` after the HeapAlloc, sums written off pBase.
   gcc 2.7.2's copy propagation deletes the intermediate (`move s0,a0` only, no
   `move t1,a0` and no `move s1,s0`) and, because pScript's pseudo then dies at
   entry, the whole callee-saved set rotates: pAnim $s3 -> $s2, pNodes $s2 ->
   $s1, frame 0x38 -> 0x30, 92.54 / 580 bytes. So the three-copy chain retail
   shows is a front-end artifact of the original source, not a spelling: two
   pseudos for the base cannot both survive copy propagation here. twin_for.py
   reports no twin for this function.
   Round 455 (three more scored runs, all 97.42 / 588, no new best): placed the
   `pBase = (u8 *)pCfg;` copy BETWEEN HeapAlloc and `pAnim->pEntries = pEntry`
   (retail's exact emission slot) - combine_regs still merged it and moved the
   surviving cast copy to entry, output byte-identical to the park; wrote the two
   loop-2 sums value-first (`pSpec->value + pBase`) - gcc still emits
   `addu v1,s1,v1`, so retail's `addu v1,v1,s1` is not reachable from operand
   order; and wrote them as `&pScript[pSpec->value]` - byte-identical again. The
   mechanism is now clear: a cast copy survives only when its source is the
   CALL-CLOBBERED hard register a0 (that is the `move s1,a0` we have, and the
   `move s0,s1` after it), while retail's third copy is between two callee-saved
   pseudos and combine_regs always merges that one. Re-run the permuter only if a
   body without the `(Misc14AnimCfg *)` cast can be found. */
/*
void func_8008B13C(u8 *pScript, Misc14Anim *pAnim, Misc14ModelNode *pNode) {
    Misc14AnimCfg *pCfg = (Misc14AnimCfg *)pScript;
    Misc14ModelNode **pNodes = ((Misc14EntryList *)pNode->pData)->pNodes;
    Misc14AnimSpec *pSpec;
    Misc14AnimEntry *pEntry;
    Misc14AnimTrack *pTrack;
    Misc14ModelNode *pPart;
    u32 i;

    pAnim->pConfig = pCfg;
    HeapSetCurrentContentType(0x10);
    pEntry = HeapAlloc(pCfg->entryCount * sizeof(Misc14AnimEntry) + pCfg->trackCount * sizeof(Misc14AnimTrack), 0);
    pAnim->pEntries = pEntry;
    pTrack = pEntry + pCfg->entryCount;
    pAnim->pTracks = pTrack;
    pSpec = pCfg->specs;
    for (i = 0; i < pCfg->entryCount; i++) {
        pPart = pNodes[pSpec->nodeIndex];
        pEntry->target = pSpec->value;
        switch (pSpec->channel & 0x7F) {
        case 3:
            pEntry->pValue = (u16 *)&pPart->rotation.vx;
            pEntry->isAngle = 1;
            break;
        case 4:
            pEntry->pValue = (u16 *)&pPart->rotation.vy;
            pEntry->isAngle = 1;
            break;
        case 5:
            pEntry->pValue = (u16 *)&pPart->rotation.vz;
            pEntry->isAngle = 1;
            break;
        case 6:
            pEntry->pValue = (u16 *)&pPart->sourcePos.vx;
            pEntry->isAngle = 0;
            break;
        case 7:
            pEntry->pValue = (u16 *)&pPart->sourcePos.vy;
            pEntry->isAngle = 0;
            break;
        case 8:
            pEntry->pValue = (u16 *)&pPart->sourcePos.vz;
            pEntry->isAngle = 0;
            break;
        }
        pEntry++;
        pSpec++;
    }
    for (i = 0; i < pCfg->trackCount; i++) {
        pPart = pNodes[pSpec->nodeIndex];
        pTrack->pScript = pScript + pSpec->value;
        pTrack->pScriptStart = pScript + pSpec->value;
        switch (pSpec->channel & 0x7F) {
        case 3:
            pTrack->pValue = (u16 *)&pPart->rotation.vx;
            pTrack->isAngle = 1;
            break;
        case 4:
            pTrack->pValue = (u16 *)&pPart->rotation.vy;
            pTrack->isAngle = 1;
            break;
        case 5:
            pTrack->pValue = (u16 *)&pPart->rotation.vz;
            pTrack->isAngle = 1;
            break;
        case 6:
            pTrack->pValue = (u16 *)&pPart->sourcePos.vx;
            pTrack->isAngle = 0;
            break;
        case 7:
            pTrack->pValue = (u16 *)&pPart->sourcePos.vy;
            pTrack->isAngle = 0;
            break;
        case 8:
            pTrack->pValue = (u16 *)&pPart->sourcePos.vz;
            pTrack->isAngle = 0;
            break;
        }
        pTrack++;
        pSpec++;
    }
    BattleResetPartAnim(pAnim);
}
*/


extern u32 ModelResolvePointers(void *pModel);
extern Misc14ModelNode *BattleCreateObject(void);
extern void BattleAppendListEntry(Misc14ModelNode *pList, Misc14ModelNode *pNode);
extern void BattleSetNodeModelPart(Misc14ModelNode *pNode, Misc14ModelPart *pData);
extern void BattleSetNodeEntryList(Misc14ModelNode *pNode, Misc14EntryList *pData);
extern Misc14EntryList *BattleCreateEntryList(void);
extern Misc14ModelPart *BattleCreateModelPart(void);
extern void BattleAttachModelPart(Misc14ModelPart *pPart, ModelPart *pModelPart);
extern void func_8008B13C(u8 *pScript, Misc14Anim *pAnim, Misc14ModelNode *pNode);

/* Best attempt: objdiff 91.16%, 580 bytes / 145 insns against retail's 592 / 148.
// Everything matches except WHICH of two long-lived values gets $s4: retail spills
// the script table to 0x18(sp) and keeps the model data in $s4, gcc spills the model
// data to 0x10(sp) and keeps the script table in $s5. The prologue/epilogue (frame
// 0x58, ten saves at 0x30..0x54), the whole first loop and the second loop's tail
// are byte-identical, so this is one spill choice; it costs the `lw a3,0x10(sp)`
// reload at the BattleAttachModelPart base and one more before the first call.
// Retail also reloads the spilled table twice (`lw a3,0x18(sp)` for the null test,
// `lw s4,0x18(sp)` in the loop preheader) and reaches the loop with
// `move s0,a1` (pAnim held in $a1 up to the preheader), so its 3 extra instructions
// are all consequences of the same spill.
// Load-bearing spellings, keep them:
//  - pDesc[i] instead of pDesc++ (with the pointer form gcc's combine_givs
//    re-bases the induction register and every displacement shifts by 0x12);
//  - the argument is a Misc14Reloc (misc10's func_80084C88 passes its pData
//    straight in): pPartList / pModelData / pPointerList are {count, 0x10-byte part
//    descriptors}, the 0x38-byte model part records and {count, script pointers};
//  - the six halfword stores are the node's rotation (0x44) then sourcePos (0x2C)
//    from six u16 descriptor fields;
//  - `pAnim->pConfig = NULL;` for the null script, with `pAnim++` after the if/else.
// Round 478 (four scored runs, none better than 91.16):
//  - the script table declared INSIDE the `if (pBlock->pPointerList != NULL)` block:
//    85.38 / 584 - gcc then keeps pBlock live to re-read the field and the whole
//    prologue rotates, so the top-of-function load retail has is not reachable that
//    way;
//  - a separate `Misc14Anim *pEntry` cursor for the second loop: 90.01 / 576 -
//    gcc coalesces it into $s0 with no `move s0,a1`, one instruction FEWER than the
//    park, so retail's a1-to-s0 chain is not source-reachable by naming the cursor;
//  - preset screen on this body: gcc-2.7.2-cdk 90.34, gcc-2.6.0 does not compile
//    it, so the unit stays Default (do not switch on this function's account).
// A 4-minute permuter run from this body found nothing (earlier round), and
// twin_for.py reports no twin. To go further the spill has to flip: raise the model
// data pointer's allocno priority or lower the table's, which no declaration order
// tried so far does. */
/*
// Misc14ModelNode *func_8008B38C(Misc14Reloc *pBlock) {
//     u8 *pModelData = (u8 *)pBlock->pModelData;
//     Misc14PartSet *pParts = (Misc14PartSet *)pBlock->pPartList;
//     Misc14ScriptTable *pScripts = (Misc14ScriptTable *)pBlock->pPointerList;
//     u32 count = pParts->count;
//     Misc14PartDesc *pDesc = pParts->descs;
//     Misc14EntryList *pList;
//     Misc14ModelNode **pNodes;
//     Misc14ModelNode *pNode;
//     Misc14ModelNode *pPart;
//     Misc14ModelPart *pModelPart;
//     Misc14Anim *pAnim;
//     u32 i;
// 
//     ModelResolvePointers(pModelData);
//     HeapSetCurrentContentType(0x12);
//     pNodes = HeapAlloc(count * 4, 0);
//     pList = BattleCreateEntryList();
//     pNode = BattleCreateObject();
//     BattleSetNodeEntryList(pNode, pList);
//     pList->pNodes = pNodes;
//     pList->nodeCount = count;
//     pList->pPartDescs = pDesc;
//     for (i = 0; i < count; i++) {
//         pPart = BattleCreateObject();
//         pNodes[i] = pPart;
//         if (pDesc[i].modelIndex != -1) {
//             pModelPart = BattleCreateModelPart();
//             BattleSetNodeModelPart(pPart, pModelPart);
//             BattleAttachModelPart(pModelPart, (ModelPart *)(pModelData + (pDesc[i].modelIndex * 0x38 + 0x10)));
//         }
//         if (pDesc[i].parent == -1) {
//             BattleAppendListEntry(pNode, pPart);
//         } else {
//             BattleAppendListEntry(pNodes[pDesc[i].parent], pPart);
//         }
//         pPart->rotation.vx = pDesc[i].rotation[0];
//         pPart->rotation.vy = pDesc[i].rotation[1];
//         pPart->rotation.vz = pDesc[i].rotation[2];
//         pPart->sourcePos.vx = pDesc[i].position[0];
//         pPart->sourcePos.vy = pDesc[i].position[1];
//         pPart->sourcePos.vz = pDesc[i].position[2];
//     }
//     if (pScripts != NULL) {
//         HeapSetCurrentContentType(0x11);
//         pAnim = HeapAlloc(pScripts->count * sizeof(Misc14Anim), 0);
//         pList->pEntries = pAnim;
//         pList->animCount = pScripts->count;
//         for (i = 0; i < pScripts->count; i++) {
//             if (pScripts->pScripts[i] != NULL) {
//                 func_8008B13C(pScripts->pScripts[i], pAnim, pNode);
//             } else {
//                 pAnim->pConfig = NULL;
//             }
//             pAnim++;
//         }
//     }
//     return pNode;
// }
// */
/*
Misc14ModelNode *func_8008B38C(Misc14Reloc *pBlock) {
    u8 *pModelData = (u8 *)pBlock->pModelData;
    Misc14PartSet *pParts = (Misc14PartSet *)pBlock->pPartList;
    Misc14ScriptTable *pScripts = (Misc14ScriptTable *)pBlock->pPointerList;
    u32 count = pParts->count;
    Misc14PartDesc *pDesc = pParts->descs;
    Misc14EntryList *pList;
    Misc14ModelNode **pNodes;
    Misc14ModelNode *pNode;
    Misc14ModelNode *pPart;
    Misc14ModelPart *pModelPart;
    Misc14Anim *pAnim;
    u32 i;

    ModelResolvePointers(pModelData);
    HeapSetCurrentContentType(0x12);
    pNodes = HeapAlloc(count * 4, 0);
    pList = BattleCreateEntryList();
    pNode = BattleCreateObject();
    BattleSetNodeEntryList(pNode, pList);
    pList->pNodes = pNodes;
    pList->nodeCount = count;
    pList->pPartDescs = pDesc;
    for (i = 0; i < count; i++) {
        pPart = BattleCreateObject();
        pNodes[i] = pPart;
        if (pDesc[i].modelIndex != -1) {
            pModelPart = BattleCreateModelPart();
            BattleSetNodeModelPart(pPart, pModelPart);
            BattleAttachModelPart(pModelPart, (ModelPart *)(pModelData + (pDesc[i].modelIndex * 0x38 + 0x10)));
        }
        if (pDesc[i].parent == -1) {
            BattleAppendListEntry(pNode, pPart);
        } else {
            BattleAppendListEntry(pNodes[pDesc[i].parent], pPart);
        }
        pPart->rotation.vx = pDesc[i].rotation[0];
        pPart->rotation.vy = pDesc[i].rotation[1];
        pPart->rotation.vz = pDesc[i].rotation[2];
        pPart->sourcePos.vx = pDesc[i].position[0];
        pPart->sourcePos.vy = pDesc[i].position[1];
        pPart->sourcePos.vz = pDesc[i].position[2];
    }
    if (pScripts != NULL) {
        HeapSetCurrentContentType(0x11);
        pAnim = HeapAlloc(pScripts->count * sizeof(Misc14Anim), 0);
        pList->pEntries = pAnim;
        pList->animCount = pScripts->count;
        for (i = 0; i < pScripts->count; i++) {
            if (pScripts->pScripts[i] != NULL) {
                func_8008B13C(pScripts->pScripts[i], pAnim, pNode);
            } else {
                pAnim->pConfig = NULL;
            }
            pAnim++;
        }
    }
    return pNode;
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B38C);



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

/* Parked: best objdiff 89.68%, 216 bytes / 54 insns against retail's 224 / 56, so
// the body is two instructions SHORT and can never link whatever it scores.
// Structure and both clamp tests are exact (back = rand() & 1 at diff == 0x800,
// back = diff < 0x800 otherwise; minus arm keeps while newdiff <= 0x800, plus arm
// keeps while newdiff >= 0x800). The residual is the DISPATCH, two instructions:
// retail tests the arm selector at BOTH of its predecessors - the rand path ends
// 'andi v0,v0,1 / bnez v0,minus / subu v0,s0,s2 / j plus / addu v0,s0,s2', and the
// diff<0x800 path ends 'beqz v0,plus / subu v0,s0,s2' with the minus arm as
// fall-through - so each site carries its own arm-operand compute. A single
// 'if (back) ... else ...' makes gcc merge the test at the join (one j, one beq with
// subu in its delay slot), which is the 2 instructions we are short.
// Measured rounds 478 and 491, nothing better than 89.68:
//  - the four-copy source, 'if (diff == 0x800) { if (rand() & 1) minus else plus }
//    else if (diff != 0) { if (diff < 0x800) minus else plus }' with the two minus
//    copies and the two plus copies textually identical: 68.09 / 268 bytes /
//    67 insns (62.54 with the pairs in the other order). gcc 2.7.2's cross-jumping
//    merges the plus pair but NOT the minus pair, so it never reaches retail's
//    two-dispatch / two-body shape. Same body under gcc-2.7.2-cdk (-O2 -G8) 62.54
//    and gcc-2.6.0 55.02, so this is not the preset.
//  - round 491: the same four copies written with explicit assigns ('var = arg0 -
//    arg2' / 'var = arg0 + arg2') instead of '-=' / '+=': 76.30 / 268 / 67, and the
//    generated .s shows WHY one pair never merges - the plus copies became ONE block
//    at its own label with the compute inside, while both minus copies were emitted
//    verbatim (compute hoisted into each branch delay slot, then 12 identical insns
//    ending 'j epilogue'). Same body under gcc-2.7.2 -O3 -G8: 76.3, so not an -O
//    level question either (try_flags).
//  - 'else if (back == 0)' re-testing the selector: gcc cprops it away (back is
//    known 0 on the else edge) and emits bytes identical to the plain if/else,
//    89.68 / 216 / 54. So the 93.25 / 220 "dead else-if re-testing back" recorded
//    in an earlier round was some other spelling; it is not reachable from
//    'back == 0' or 'back'.
// The two clamp constants differing (slti 0x801 on the minus arm, slti 0x800 on the
// plus arm) is the evidence that the arms were written out twice in the original
// source; the missing piece is the merge of BOTH pairs, which gcc 2.7.2 only does
// once (tools/rounds/levers.md i-a-four-copy-arm-pair-merges-once). twin_for.py: no
// twin. Do not re-try the four-copy shapes without a compiler that merges both
// pairs. */
/*
s16 func_8008B650(s32 arg0, s32 arg1, s32 arg2) {
    s32 diff;
    s32 back;
    s16 var;

    var = arg0;
    diff = (arg0 - arg1) & 0xFFF;
    if (diff != 0) {
        if (diff == 0x800) {
            back = rand() & 1;
        } else {
            back = diff < 0x800;
        }
        if (back) {
            var -= arg2;
            if ((((s16)var - (s16)arg1) & 0xFFF) > 0x800) {
                var = arg1;
            }
        } else {
            var += arg2;
            if ((((s16)var - (s16)arg1) & 0xFFF) < 0x800) {
                var = arg1;
            }
        }
    }
    return var;
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B650);



// Best attempt: objdiff 93.45%, 764 bytes / 191 insns = the original's EXACT size, 24 differing lines in 4 residual groups. Round 469 raised this from the old 92.55 / 756 (which was one instruction SHORT, not retail's size as the old note claimed) with three measured spellings, keep them:
//    - the opcode local must be a byte: 's8 c;' assigned 'c = *(s8 *)pScript;' (92.55 -> 93.04). That is what produces retail's 'move v1,v0': a QImode pseudo promoted to int, so the 0x3F/0x40/seven-bit paths read the copy and only the 0x80 test reads the load. It also costs 8 bytes of frame, so the unused-local array must shrink from 's32 pad[2]' to 's32 pad[1]' to keep retail's 0x60 frame (pad[2] with the s8 gives 0x68).
//    - the 0x40 arm must OR IN PLACE, 'bits |= ...; pTrack->step = bits;' and not 'pTrack->step = bits | ...;' (93.13 -> 94.16). With a fresh result register gcc's cross-jumping merges the arm's 'sh step' with the else arm's 'sh step' into one shared block and the function loses an instruction; in place, the store's source is the bits pseudo and retail's 'or a0,a0,v0 / j / sh a0,2(s0)' plus its separate 'j / sh a0,-2(s0)' fall out exactly, and the mask lands in a0 as retail has it.
//    - the shifted second byte wants the cast AROUND the shift, 'bits |= (s8)(pScript[1] << 6);' - that spelling keeps retail's 'lbu' load and reaches retail's 191 instructions / 764 bytes. 'bits |= (s8)pScript[1] << 6;' folds the sign extension into a signed 'lb' + 'sll 6' (190 insns, 760 bytes, 94.16%), and routing the byte through a u8 local does the same fold.
//    Residual groups, all still open:
//    1. (2 differing lines) our shifts are 'sll 0x1e / sra 0x18' where retail has 'sll 0x18 / sra 0x12': gcc extends the ALREADY SHIFTED value, retail extends the byte first and then shifts. 's8 next = pScript[1];' and 'bits |= (s8)pScript[1] << 6;' both fold to 'lb / sll 6', so the extend-then-shift order is not reachable from these three spellings.
//    2. (all three loop tails, 9 differing lines) retail emits the counter increment BEFORE the count reload ('lw pConfig / addiu i,1 / lh count / nop / slt') where we always get 'lw pConfig / nop / lh count / addiu i,1 / slt'; loop 2 additionally has the pointer increment between them in retail. Measured identical this round: the for form, the while form with 'i++' as the last body statement, and an explicit 'if (count > 0) do {... i++;} while (i < count)' wrapper (that one does reproduce the tail order but drops the frame to 0x50 and breaks the prologue). So the increment-before-reload order is not reachable from the loop construct.
//    3. (2 differing lines) the closing compare picks $v1 for position and $v0 for length where retail uses $a0/$v1.
//    Bodies tried and dead (round 469): bits as a fresh temp (94.16 but one instruction short), the u8 'next' local, the three loop forms above. Two of the three residuals are allocator ties with everything else byte-exact, so this is a permuter-from-this-body job rather than a hand job.
// Best attempt: objdiff 93.45%, 764 bytes / 191 insns = retail's EXACT size, 24 differing lines in the 4 residual groups listed below (round 469's spellings, keep them). NOTE the 94.16% in the older text belongs to the 760-byte 'bits |= (s8)pScript[1] << 6;' body, which is one instruction SHORT and can never link; 93.45 / 764 is the honest park.
// Round 482 closed the remaining axes; do not spend another lane on this function:
//    - GROUP 2 (the three loop tails: retail puts 'addiu i,1' BEFORE the count reload, we put it after) is sched2's, not the source's: compiling this unit's own .i with the unit cc1 plus '-fno-schedule-insns -fno-schedule-insns2' gives 88.33% / 808 bytes, i.e. retail's order is not the RTL order either, and three loop forms were already measured identical in round 469.
//    - GROUP 1 (retail 'lbu / sll 0x18 / sra 0x12' = extend-the-byte-then-shift-6, ours 'sll 0x1e / sra 0x18') is closed by a cc1_probe sweep: ONLY the parked 'bits |= (s8)(pScript[1] << 6);' keeps retail's lbu (with sll30/sra24); '(s8)pScript[1] << 6', 'u8 b' local, 's8 n' assigned in a later statement, 's32 e = (s8)pScript[1]' as its own statement, a 'u16 op' local and an s8-pointing script pointer ALL fold to 'lb + sll 6' (190 insns / 760 bytes). Retail's lbu therefore needs gcc's combine to skip the extend-into-load fold, which no spelling reaches.
//    - GROUP 3 (the closing compare: retail position->$a0 / length->$v1, ours $v1/$v0 with the load reusing the dying base) is a local-alloc tie on the block's two QTYs, pre-sched2 and so not a scheduling artifact - no source shape was found for it in rounds 469/482.
//    - A 4-minute permuter run from this exact body (round 482): base=545 best=545, 1309 iterations, NONE.
//    - Presets re-measured this round on a decl-at-top copy of the same body: gcc-2.6.0 91.77, gcc-2.7.2-cdk 91.79, -G0 93.45, -O3 compile-failed. The unit's Default preset is right; nothing here says split the unit.
/*
s32 func_8008B730(Misc14Anim *pAnim, s32 arg1, s32 arg2) {
    Misc14AnimEntry *pEntry;
    Misc14AnimTrack *pTrack;
    u8 *pScript;
    s8 c;
    s32 i;
    s32 j;
    s32 pos;
    s32 pad[1];

    if (pAnim->pConfig == NULL) {
        return 1;
    }
    if (arg1 != 0) {
        pos = pAnim->position;
        if (pos + arg1 > pAnim->pConfig->length) {
            arg1 = pAnim->pConfig->length - pos;
        }
        pAnim->position += arg1;
        arg2 -= arg1;
        if (arg2 <= 0) {
            arg2 = 1;
        }
        pEntry = pAnim->pEntries;
        if (arg2 == 1) {
            for (i = 0; i < pAnim->pConfig->entryCount; i++) {
                *(u16 *)pEntry->pValue = pEntry->target;
                ++pEntry;
            }
        } else {
            for (i = 0; i < pAnim->pConfig->entryCount; i++) {
                u16 value = *pEntry->pValue;

                if (pEntry->isAngle != 0) {
                    *pEntry->pValue = BattleInterpolateAngle((s16)value, (s16)pEntry->target, arg2);
                } else {
                    *pEntry->pValue = value + ((s16)pEntry->target - (s16)value) / arg2;
                }
                ++pEntry;
            }
        }
        pTrack = pAnim->pTracks;
        for (i = 0; i < pAnim->pConfig->trackCount; i++) {
            for (j = 0; j < arg1; j++) {
                if (pTrack->wait != 0) {
                    pTrack->wait--;
                } else {
                    pScript = pTrack->pScript;
                    pTrack->pScript = pScript + 1;
                    c = *(s8 *)pScript;
                    if (c & 0x80) {
                        s32 bits = c & 0x3F;

                        if (c & 0x40) {
                            pTrack->pScript = pScript + 2;
                            bits |= (s8)(pScript[1] << 6);
                            pTrack->step = bits;
                        } else {
                            pTrack->wait = bits;
                        }
                    } else {
                        pTrack->step = (s8)(c << 1) >> 1;
                    }
                }
                pTrack->value += pTrack->step;
            }
            if (arg2 == 1) {
                *pTrack->pValue = pTrack->value;
            } else {
                u16 value = *pTrack->pValue;

                if (pTrack->isAngle != 0) {
                    *pTrack->pValue = BattleInterpolateAngle((s16)value, (s16)pTrack->value, arg2);
                } else {
                    *pTrack->pValue = value + ((s16)pTrack->value - (s16)value) / arg2;
                }
            }
            pTrack++;
        }
        if (pAnim->position != pAnim->pConfig->length) {
            return 0;
        }
        return arg2 == 1;
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B730);


// A swapped-out execution context: the register file func_8008BB3C restores
// (word 0x1C $gp, 0x1D $sp, 0x1E $fp, 0x1F $ra) and the stack it runs on.
typedef struct {
    /* 0x00 */ u32 regs[0x20];
    /* 0x80 */ u32 stackBase;
} Misc14Work; // Size: 0x84

Misc14Work *BattleCreateWorkBlock(u32 arg0, u32 arg1, u32 arg2, s32 arg3) {
    Misc14Work *p;
    s32 i;
    u32 value;

    HeapSetCurrentContentType(3);
    p = HeapAlloc(0x84, 2);
    for (i = 0x1F; i >= 0; i--) {
        p->regs[i] = 0;
    }
    p->stackBase = arg2;
    p->regs[0x1C] = GetGp();
    value = p->stackBase + arg3 * 4;
    p->regs[0x1F] = arg0;
    p->regs[4] = arg1;
    p->regs[0x1D] = value;
    p->regs[0x1E] = value;
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
   one allocator question, not two. Probed with cc1_probe.sh (round 331): two
   separate locals, a u32* destination, a (long long) struct copy, a non-void
   return type, a dead re-read and volatile sources all put the temps in $2/$3
   under 2.7.2, 2.6.0 and 2.7.2-cdk, so the $t0/$t1 choice is not reachable from
   source with these compilers.
   Round 429: re-measured 94.29 / 28 bytes (retail's exact size), tried -G0
   (94.29, so the access mode is not the mechanism) and the deferred-assignment
   and local-temp shapes again. New evidence for the orchestrator: this and its
   sibling func_8008BB1C are the ONLY two functions in the whole unit whose
   original uses $t0/$t1 (grep for 'lui $t0' in asm/battling/nonmatchings/main/
   misc14/), and both are the same two-word copy idiom. The body is one basic
   block with no calls, so local-alloc (not global_alloc) hands the two load
   QTYs the first free registers, and every compiler in tools/ starts that scan
   at $2; retail's lands at $8. So this looks like a compiler-build trait
   (REG_ALLOC_ORDER / scan start), not a spelling - it needs a different cc1,
   not a different body.
   Round 442: un-parked, re-measured 94.29 / 28 bytes / 7 insns (note current),
   re-parked; twin_for.py reports no twin.
   Round 455: un-parked and re-measured once more, 94.29 / 28 bytes / 7 insns,
   diff unchanged (only $t0/$t1 against $v0/$v1, both relocations identical), so
   the note is current and the wall is the cc1 register-scan start. */
/*
void func_8008BB00(Misc14Entry *pDst) {
    pDst->unk0 = D_80096D88;
    pDst->color = D_80096D8C;
}
*/

// Best attempt: objdiff 97.50%, 32 bytes = the original's exact size and instruction sequence; ONLY the reused load temp differs ($t0 in retail, $v0 in ours). Re-measured round 482: 97.50 / 32 bytes / 8 insns, unchanged; twin_for.py reports no twin and the lever a-call-free-single-block-temp-in-t0-is-a-compiler-trait (written from this function and its sibling func_8008BB00) closes the axis: a leaf, call-free, one-block body never reaches global_alloc, so local-alloc's scan hands the QTY the first free register and every cc1 in tools/ starts at $2. -G0, 2.6.0, 2.7.2-cdk, -O1/-O3, an 8-byte struct copy (70.71), a struct pointer, a cc1_probe spelling sweep and a 2-minute permuter run are all recorded there. This is the retail compiler's REG_ALLOC_ORDER, not a spelling - do not spend another lane on it.
/*
void func_8008BB1C(Misc14Entry *pSrc) {
    u32 value;

    value = pSrc->unk0;
    D_80096D88 = value;
    value = pSrc->color;
    D_80096D8C = value;
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB1C);




INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB3C);
