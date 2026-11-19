#include "common.h"
#include "system/menu.h"
#include "system/animation.h"
#include "psyq/inline_c.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800248D4);

// Work list and image-list setup; g_GfxWorkListCapacity is the work list entry budget.
extern u32 g_GfxWorkListCapacity;
extern u8 g_IsBattle;
extern u8 g_GfxSpritesUseBaseMatrix;
extern void WorkListsReset(void);

void GfxResetWorkLists(void) {
    g_IsBattle = 0;
    g_GfxSpritesUseBaseMatrix = 0;
    g_GfxWorkListCapacity = 0x2000;
    WorkListsReset();
    SpriteAnimFrameListReset();
}



extern void SpriteAnimFrameListReset(void);
extern void SpriteAnimFrameListClear(void);

// Defined here so that the compiler places it in .sbss (%gp_rel)
Image* g_GfxImageList[2];

void GfxAllocateWorkBuffers(int workBufferSize, unsigned int allocFlag) {
    void* pWorkBuffers;

    g_GfxWorkBufferSize = workBufferSize;
    pWorkBuffers = HeapAlloc(workBufferSize * NUM_RENDER_CONTEXTS, allocFlag);
    g_GfxWorkBuffers = pWorkBuffers;
    g_GfxWorkBuffer2 = pWorkBuffers + workBufferSize;
    D_80059304 = 0;
    g_GfxSpriteTileListHeads = 0;
    g_GfxImageList[0] = NULL;
    SpriteAnimFrameListReset();
}

void GfxFreeWorkBuffers(void) {
    HeapFree(g_GfxWorkBuffers);
    SpriteAnimFrameListClear();
}

void GfxSetCurrentOT(u_long* ot) {
    g_GfxCurOT = ot;
}








// Maybe start of a TU
// ===========================================================


// Set g_GfxBaseMatrix matrix
void GfxSetBaseMatrix(MATRIX* matrix) {
    g_GfxBaseMatrix = *matrix;
}




// Maybe start of a TU
// ===========================================================

// Loops over the linked list given by g_GfxImageList[g_GfxCurContext].
// LoadImage on entries with an address, ClearImage otherwise.
// Set the list to NULL afterwards.
// GfxShapeTransfer
void GfxFlushImageList(void) {
    Image* pListHead;
    Image* pImage;

    pListHead = g_GfxImageList[g_GfxCurContext];
    for (pImage = pListHead; pImage != NULL; pImage = pImage->pNext) {
        if (pImage->addr) {
            LoadImage(&pImage->rect, pImage->addr);
        } else {
            ClearImage(&pImage->rect, 0x0, 0x0, 0x0);
        }
    }

    g_GfxImageList[g_GfxCurContext] = NULL;
}

typedef struct {
    void* pData; // Sprite Tile data stuff
    struct LinkedListEntry* pNext;
} LinkedListEntry;

// include/system/animation.h already types g_GfxSpriteTileListHeads as a scalar; the real
// object is the two per-context list heads, so address it through a cast.
#define g_GfxSpriteTileLists ((LinkedListEntry**)&g_GfxSpriteTileListHeads)

// g_GfxWorkBuffers and the g_GfxWorkBuffer2 that follows it are the two
// per-context buffer bases, reached as one array through the symbol's address:
// indexing the symbol is what makes maspsx emit %hi/%lo rather than %gp_rel.
// animation.h names them singly (GfxAllocateWorkBuffers sets each), so the
// pair is read here through a view.
typedef struct {
    /* 0x0 */ void* pBuffers[NUM_RENDER_CONTEXTS];
} GfxWorkBufferBases;

#define g_GfxWorkBufferBases ((GfxWorkBufferBases*)&g_GfxWorkBuffers)

// g_GfxCurWorkBufferBase is a tentative definition so maspsx emits %gp_rel for it.
s32 g_GfxCurWorkBufferBase;

void GfxSetRenderContext(int context) {
    // The tile-list heads are the two words at g_GfxSpriteTileListHeads. The pointer local
    // with the embedded assignment is what keeps retail's single context*4
    // before the symbol address: a plain index or a typed head array moves the
    // sll after the lui and stalls at 95.00%. The s32* element type is part of
    // that (a LinkedListEntry** local gives the same 95.00%).
    s32* pLists;
    LinkedListEntry* pCurEntry;
    void* pPrimBuffer;

    pCurEntry = ((LinkedListEntry**)(pLists = &g_GfxSpriteTileListHeads))[context];
    pPrimBuffer = g_GfxWorkBufferBases->pBuffers[context];
    g_GfxCurContext = context;
    g_GfxCurWorkBuffer = pPrimBuffer;
    g_GfxCurWorkBufferBase = pPrimBuffer;
    g_GfxCurWorkBufferEnd = pPrimBuffer + g_GfxWorkBufferSize;
    while (pCurEntry != NULL) {
        HeapFree(pCurEntry->pData);
        pCurEntry = pCurEntry->pNext;
    }
    g_GfxSpriteTileLists[context] = NULL;
}
// GfxQueueSpriteTileData
void GfxQueueSpriteTileData(void* pData) {
    LinkedListEntry* pNewEntry;

    pNewEntry = g_GfxCurWorkBuffer;
    g_GfxCurWorkBuffer = pNewEntry + 1;
    if (pNewEntry) {
        pNewEntry->pData = pData;
        pNewEntry->pNext = g_GfxSpriteTileLists[g_GfxCurContext];
        g_GfxSpriteTileLists[g_GfxCurContext] = pNewEntry;
    }
}

// GfxQueueShapeTransfer
// Add Image to current g_GfxImageList[g_GfxCurContext] linked list
//
// height is declared s16 so that cdk's gcc loads the incoming stack word
// (lw v1,0x10(sp)) before the sltu instead of inside the branch.
void GfxQueueImageTransfer(u_long* addr, int x, int y, int width, s16 height) {
    Image* pCurrent;
    Image* pNext;

    pCurrent = g_GfxCurWorkBuffer;
    pNext = pCurrent + 1;
    if (pNext < (Image*) g_GfxCurWorkBufferEnd) {
        pCurrent->rect.h = height;
        pCurrent->rect.x = x;
        pCurrent->rect.y = y;
        pCurrent->rect.w = width;
        pCurrent->addr = addr;
        g_GfxCurWorkBuffer = pNext;
        pCurrent->pNext = g_GfxImageList[g_GfxCurContext];
        g_GfxImageList[g_GfxCurContext] = pCurrent;
    }
}

// Handler table of the sprite work task, indexed by its handler id:
// SpriteDrawProjected, SpriteTaskNoOp, SpriteDrawModelParts, NULL, NULL, SpriteDrawProjected,
// SpriteDrawProjected, SpriteDrawModelParts, SpriteDrawTile, func_80025544, NULL, NULL,
// NULL, NULL, SpriteDrawProjected, SpriteDrawLitModelParts.
extern WorkListCallback_t g_SpriteTaskHandlers[];

// Sets pStruct->unk8 callback
void SpriteSetTaskHandler(WorkListEntry* pTask, int handlerIndex) {
    WorkListSetTaskCallback(pTask, g_SpriteTaskHandlers[handlerIndex]);
}

extern s32 g_GfxOTDepthShift;
extern u8 D_800C3664;
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void SpriteDrawFrame(SpriteData* pSpriteData, void* pArg);

// SpriteData as this unit reads it: the per-context frames-data pair at 0x2C
// (two scalars in the shared header, because other units assign them singly),
// curSpriteFrame as an unsigned halfword, the colour word at 0x28 (copied
// whole into a primitive), the 0x3C flag word (bits 25-27 are read as a whole
// word here, where the header's bitfields over them narrow to lbu 0x3F) and the
// upper halfword of field_0x40, which is read with lhu.
typedef struct {
    /* 0x00 */ u8 unk0[0x28];
    /* 0x28 */ u32 color; // red, green, blue and the primitive code as one word
    /* 0x2C */ void* pFramesData[NUM_RENDER_CONTEXTS];
    /* 0x34 */ u16 curSpriteFrame; // unsigned here: -1 is a valid frame
    /* 0x36 */ u8 unk36[0x6];
    /* 0x3C */ SpriteFlagWord flags_0x3C;
    /* 0x40 */ u16 flags0x40Low;
    /* 0x42 */ u16 flags0x40High;
} SpriteDataView;

#define g_SpriteFramesData(pSpriteData) \
    (((SpriteDataView*)(pSpriteData))->pFramesData)
#define g_SpriteFlags0x3C(pSpriteData) \
    (((SpriteDataView*)(pSpriteData))->flags_0x3C)
#define g_SpriteFlags0x40High(pSpriteData) \
    (((SpriteDataView*)(pSpriteData))->flags0x40High)

// Projects the sprite's position and queues it into the OT at its own depth
// offset, either straight (bit 24) or through the model transform (bit 29).
void SpriteDrawProjected(WorkListEntry* pTask) {
    SVECTOR vec;
    long vecTrans[3];
    long sxy;
    long flag;
    SpriteData* pSpriteData;
    MATRIX* pMatrix;
    s32 nOTOffset;

    pSpriteData = pTask->unk4;
    if ((((u32)pSpriteData->flags_0xB0.flags >> 8) & 1) && D_800C3664) {
        return;
    }

    vec.vx = pSpriteData->position.x >> 16;
    vec.vy = pSpriteData->position.y >> 16;
    vec.vz = pSpriteData->position.z >> 16;
    pMatrix = &g_GfxBaseMatrix;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    nOTOffset = (RotTransPers(&vec, &sxy, (u_long*)&sxy, &flag) >> g_GfxOTDepthShift) +
               pSpriteData->field_0x30;
    if (flag & 0x8000) {
        nOTOffset = 0;
    }
    pSpriteData->field_0x2e = nOTOffset;

    if (pSpriteData->field_0x3C_2 & 1) {
        SpriteUpdateTransformMatrix(pSpriteData);
        vecTrans[0] = pSpriteData->position.x >> 16;
        vecTrans[1] = pSpriteData->position.y >> 16;
        vecTrans[2] = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, (VECTOR*)vecTrans);
        SetRotMatrix(&pSpriteData->pBase->transformMatrix);
        SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        // Re-read rather than keep a copy: the GTE calls above clobber the
        // register the word was in.
        if (((u32)g_SpriteFlags0x3C(pSpriteData).flags >> 25) & 1) {
            nOTOffset = 0xFFF;
        } else {
            nOTOffset = pSpriteData->field_0x30;
        }
        if ((u32)(nOTOffset - 1) < 0xFFF) {
            func_8001E3D8(pSpriteData, &g_GfxCurOT[nOTOffset]);
        }
    } else {
        if (pSpriteData->field_0x3C_5 & 1) {
            nOTOffset = ((SpriteData*)pSpriteData->field_0x70)->field_0x2e;
        }
        if ((u32)(nOTOffset - 1) < 0xFFF) {
            SpriteDrawFrame(pSpriteData, &g_GfxCurOT[nOTOffset]);
        }
    }
}

void SpriteDrawTile(WorkListEntry* pTask) {
    SVECTOR vec;
    int nFlag;
    int nOTOffset;
    DR_TPAGE* pPrimTPage;
    TILE_1* pPrim;
    TILE_1* pNewBufferHead;
    DR_TPAGE* pNewBufferHead_2;
    SpriteData* pSpriteData;
    MATRIX* pMatrix;

    pSpriteData = pTask->unk4;
    if (((SpriteDataView*)pSpriteData)->curSpriteFrame == 0) {
        pPrim = g_GfxCurWorkBuffer;
        pNewBufferHead = pPrim + 1;
        if (pNewBufferHead < (TILE_1*)g_GfxCurWorkBufferEnd) {
            pMatrix = &g_GfxBaseMatrix;
            vec.vx = pSpriteData->position.x >> 16;
            vec.vy = pSpriteData->position.y >> 16;
            vec.vz = pSpriteData->position.z >> 16;
            g_GfxCurWorkBuffer = pNewBufferHead;
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            nOTOffset = RotTransPers(&vec, &pPrim->x0, &nFlag, &nFlag) >> g_GfxOTDepthShift;
            pSpriteData->field_0x2e = nOTOffset;

            // SetTile1
            setlen(pPrim, 2);
            *((u32*)&pPrim->r0) = ((SpriteDataView*)pSpriteData)->color;

            AddPrim(&g_GfxCurOT[nOTOffset], pPrim);

            pPrimTPage = g_GfxCurWorkBuffer;
            pNewBufferHead_2 = pPrimTPage + 1;
            if (pNewBufferHead_2 < (DR_TPAGE*)g_GfxCurWorkBufferEnd) {
                g_GfxCurWorkBuffer = pNewBufferHead_2;

                // Only the tpage bits of flags_0x3C make it into the primitive.
                setDrawTPage(pPrimTPage, 0, 0, pSpriteData->field_0x3C_0 & 0x60);
                AddPrim(&g_GfxCurOT[nOTOffset], pPrimTPage);
            }
        }
    }
}

/*
Un-park screen: cdk 89.57%, gcc-2.6.0 88.37%, gcc-2.7.2 88.24% - the park
stays, and cdk is still the closest of the three.
Best 89.57% (464 bytes against 460). cdk is the right compiler here: the same C
scores 87.24% under gcc-2.7.2-psx, so this one is source-level. Two gaps: gcc
sinks the `lhu` of pPrim->x0 - and the `lhu` of xOffset, which the original
issues before the g_GfxCurWorkBuffer load - so the original fills a load-delay
slot that gcc leaves as a nop, and xOffset/prim end up in s3/s1 instead of
s1/s3. Ruled out (all produced byte-identical output, so neither statement
order nor copies are the lever): assigning prim before xOffset, a pNext local
with a nested if, a second TILE pointer for the tail, aliasing the tail through
a u16 pointer, and folding nHalf into the field updates. gcc coalesces every
copy; the s1/s3 swap and the missing move-a1-tail copy must come from elsewhere
in the source (prim is the higher-priority pseudo here, lower there). Both x0 reads (signed for the width, unsigned for the centre) have to
stay in the source.
Also tried and byte-identical: a u32 pNewBufferHead local with the neighbour's
`pNewBufferHead < g_GfxCurWorkBufferEnd` idiom, and reading x0 (u16) before
nWidth. The reorder is gcc moving the xOffset lhu two insns down, after the two
gp loads; that reorder is what puts pPrim in s1 instead of s3.
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025544);
/*
void func_80025544(WorkListEntry* pTask) {
    SpriteData* pSpriteData;
    MATRIX* pMatrix;
    TILE* pPrim;
    DR_TPAGE* pPrimTPage;
    SVECTOR vec;
    SVECTOR vecOffset;
    SVECTOR sxy1;
    long sxy2;
    long flag;
    u16 xOffset;
    u16 x0;
    s32 nWidth;
    s32 nHalf;
    s32 nOTOffset;
    u32 pNewBufferHead;

    pSpriteData = pTask->unk4;
    if (*(u16*)&pSpriteData->curSpriteFrame != 0) {
        return;
    }
    xOffset = *(u16*)((u8*)pSpriteData + 0x36);
    pPrim = g_GfxCurWorkBuffer;
    pNewBufferHead = pPrim + 1;
    if (!(pNewBufferHead < g_GfxCurWorkBufferEnd)) {
        return;
    }

    vec.vx = pSpriteData->position.x >> 16;
    vec.vy = pSpriteData->position.y >> 16;
    vec.vz = pSpriteData->position.z >> 16;
    pMatrix = &g_GfxBaseMatrix;
    g_GfxCurWorkBuffer = pPrim + 1;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);

    vecOffset = vec;
    *(u16*)&vecOffset.vx += xOffset;
    nOTOffset = RotTransPers3(&vec, &vecOffset, &vec, (long*)&pPrim->x0,
                              (long*)&sxy1, &sxy2, &flag, &flag) >> g_GfxOTDepthShift;
    pSpriteData->field_0x2e = nOTOffset;

    x0 = *(u16*)&pPrim->x0;
    nWidth = sxy1.vx - pPrim->x0;
    if (nWidth == 0) {
        nWidth = 1;
    }
    if (nWidth < 0) {
        nWidth = -nWidth;
    }
    nHalf = nWidth / 2;
    pPrim->x0 = x0 - nHalf;
    pPrim->y0 = *(u16*)&pPrim->y0 - nHalf;

    setlen(pPrim, 3);
    *((u32*)&pPrim->r0) = *((u32*)&pSpriteData->red);
    pPrim->h = nWidth;
    pPrim->w = nWidth;
    AddPrim(&g_GfxCurOT[nOTOffset], pPrim);

    pPrimTPage = g_GfxCurWorkBuffer;
    if (!((u8*)(pPrimTPage + 1) < (u8*)g_GfxCurWorkBufferEnd)) {
        return;
    }
    g_GfxCurWorkBuffer = pPrimTPage + 1;
    setDrawTPage(pPrimTPage, 0, 0, pSpriteData->field_0x3C_0 & 0x60);
    AddPrim(&g_GfxCurOT[nOTOffset], pPrimTPage);
}
*/

void SpriteTaskNoOp(void) {}

extern void SpriteUpdateTransformMatrix(SpriteData* pSpriteData);
extern void ModelRenderPacket(void* pDirTransforms, void* pFramesData, u_long* pOT, int flags);

// Draws the sprite through its transform matrix, then its model parts.
void SpriteDrawModelParts(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vec;
    SpriteData* pSpriteData;

    pSpriteData = pTask->unk4;
    SpriteUpdateTransformMatrix(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, &vec);
        if (!(pSpriteData->field_0x3C_2 & 1)) {
            CompMatrix(&g_GfxBaseMatrix, &pSpriteData->pBase->transformMatrix, &matrix);
        }
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        ModelRenderPacket(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, g_SpriteFlags0x40High(pSpriteData) & 4);
    }
}

extern void func_800B1F6C(void* pDirTransforms, void* pFramesData, u_long* pOT,
                          int flags, s32 arg4, s32 arg5);
extern MATRIX g_GfxColorMatrix;
extern MATRIX g_GfxLightSourceMatrix;

// The standalone base object pBase points at carries a second rotation/scale
// pair past the 0x40 SpriteDataI1 covers; SVECTOR's 8-byte size puts the scale
// exactly where g_GfxColorMatrix keeps its 0xC00 identity diagonal. The cast is
// written at each read on purpose: one pointer local across the g_GfxColorMatrix
// stores turns retail's four pBase reloads into one and costs the match (93%).
typedef struct {
    /* 0x00 */ SpriteDataI1 base;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ SVECTOR rotation;
    /* 0x4C */ SVECTOR scale;
} SpriteBaseExt; // Size: 0x54

// Builds the sprite's color and light matrices from its base scale/rotation,
// then draws its model parts; a negative flag word saves and restores the
// geometry offset around the draw.
void SpriteDrawLitModelParts(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vec;
    MATRIX colorMatrix;
    MATRIX lightMatrix;
    long geomOffsetX;
    long geomOffsetY;
    SpriteData* pSpriteData;
    s32 oldShift;

    pSpriteData = pTask->unk4;
    SpriteUpdateTransformMatrix(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        if ((pSpriteData->field_0x40 >> 1) & 1) {
            PushMatrix();
            // The scale values go in the m[i][0] slots (bytes 0, 6, 0xC), which
            // is where the static initialiser keeps its 0xC00 identity values.
            g_GfxColorMatrix.m[0][0] = ((SpriteBaseExt*)pSpriteData->pBase)->scale.vx;
            g_GfxColorMatrix.m[1][0] = ((SpriteBaseExt*)pSpriteData->pBase)->scale.vy;
            g_GfxColorMatrix.m[2][0] = ((SpriteBaseExt*)pSpriteData->pBase)->scale.vz;
            RotMatrix(&((SpriteBaseExt*)pSpriteData->pBase)->rotation,
                      &colorMatrix);
            MulMatrix0(&colorMatrix, &pSpriteData->pBase->transformMatrix,
                       &colorMatrix);
            MulMatrix0(&g_GfxLightSourceMatrix, &colorMatrix, &lightMatrix);
            SetBackColor(0x20, 0x20, 0x20);
            SetColorMatrix(&g_GfxColorMatrix);
            SetLightMatrix(&lightMatrix);
            PopMatrix();
        }

        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, &vec);
        if (!(pSpriteData->field_0x3C_2 & 1)) {
            CompMatrix(&g_GfxBaseMatrix, &pSpriteData->pBase->transformMatrix,
                       &matrix);
            SetRotMatrix(&matrix);
            SetTransMatrix(&matrix);
        } else {
            SetRotMatrix(&pSpriteData->pBase->transformMatrix);
            SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        }

        if (pSpriteData->field_0x3C_4 & 1) {
            ReadGeomOffset(&geomOffsetX, &geomOffsetY);
            SetGeomOffset(0xA0, 0x70);
        }
        if (((u32)g_SpriteFlags0x3C(pSpriteData).flags >> 25) & 1) {
            oldShift = g_GfxOTDepthShift;
            g_GfxOTDepthShift = 0x10;
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, 0xFEC,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
            g_GfxOTDepthShift = oldShift;
        } else {
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, pSpriteData->field_0x30,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
        }
        if (pSpriteData->field_0x3C_4 & 1) {
            SetGeomOffset(geomOffsetX, geomOffsetY);
        }
    }
}

// Draws the sprite through the matrix g_GfxBaseMatrix composed with its own, then
// hands the model parts to func_800B1F6C.
void SpriteDrawModelPartsInBaseMatrix(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vecResult;
    SVECTOR vec;
    SpriteData* pSpriteData;
    s32 oldShift;

    pSpriteData = pTask->unk4;
    SpriteUpdateTransformMatrix(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        ApplyMatrix(&g_GfxBaseMatrix, &vec, &vecResult);
        pSpriteData->pBase->transformMatrix.t[0] = g_GfxBaseMatrix.t[0] + vecResult.vx;
        pSpriteData->pBase->transformMatrix.t[1] = g_GfxBaseMatrix.t[1] + vecResult.vy;
        pSpriteData->pBase->transformMatrix.t[2] = g_GfxBaseMatrix.t[2] + vecResult.vz;
        SetRotMatrix(&pSpriteData->pBase->transformMatrix);
        SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        if (((u32)g_SpriteFlags0x3C(pSpriteData).flags >> 25) & 1) {
            oldShift = g_GfxOTDepthShift;
            g_GfxOTDepthShift = 0x10;
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, 0xFEC,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
            g_GfxOTDepthShift = oldShift;
        } else {
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, pSpriteData->field_0x30,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
        }
    }
}

// splat marks 80025C04, 80025D4C, 80026F44 and 80026FE8 "Handwritten
// function", but only two of them really are: 80026F44 and 80026FE8 contain
// trapping-immediate ALU ops that no gcc emits (80026F44 0x20050020 addi,
// 80026FE8 three addi and three add). 80025C04 and 80025D4C are ordinary GTE
// code and their mtc2/lwc2/swc2 all have macros in psyq/inline_c.h.
// 80025C04 best attempt below: 87.30% at the original's exact 316 bytes.
// Remaining, all allocator/scheduling, no structure left:
//  - retail's scratchpad base is $t0 and holds it across the loop; ours parks
//    the 0x1F800000 lui in $t1 and copies it to $t0 (one extra move).
//  - the accumulator at 0x1F800000: retail stores the clamped red with sh and
//    then RE-READS it in both arms of the green/blue ifs; gcc 2.7.2 keeps the
//    stored value in a register (v1) and defers the sh past the loads. Making
//    the u16 view volatile is the untried lever.
// NOT reachable, whatever the C: splat renders the two GTE buffer addresses as
// addiu $v0,$t0,%lo(D_1F800004) / %lo(D_1F800014), so the expected object
// carries an R_MIPS_LO16 reloc there with a literal-zero immediate, while any
// C spelling that keeps the base a runtime register emits addiu $v0,$t0,4 with
// no reloc. Same linked bytes (retail's lui has no %hi reloc at all, splat
// printed it as a raw (0x1F800000 >> 16)), but objdiff can never score them
// equal. Naming the symbols instead (extern s32 D_1F800004; gte_ldlvl(&D_...))
// does emit the %lo form, but then gcc gives the hi its own %hi reloc and a
// separate register, and the score falls to 80.05% with 320 bytes.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025C04);
/*
// Interpolates 16-bit colours of pSrc into pDst through GPF: the source colour
// is staged in the scratchpad as IR1-IR3, GPF blends it against the running
// colour at 0x1F800000 and the result is clamped and packed back.
void func_80025C04(s32 count, s32 intensity, u16 *pDst, u16 *pSrc) {
    s32 *pScratch = (s32 *)PSX_SCRATCH;
    u16 *pAcc = (u16 *)PSX_SCRATCH;
    u16 value;

    gte_lddp(intensity << 7);
    while (--count != -1) {
        pScratch[1] = *pSrc & 0x1F;
        pScratch[2] = *pSrc & 0x3E0;
        pScratch[3] = *pSrc & 0x7C00;
        gte_ldlvl(pScratch + 1);
        gte_gpf12();
        gte_stlvl(pScratch + 5);
        pAcc[0] = (pScratch[5] < 0x20) ? (pAcc[10] & 0x1F) : 0x1F;
        if (pScratch[6] >= 0x3E1) {
            pAcc[0] |= 0x3E0;
        } else {
            pAcc[0] |= pAcc[12] & 0x3E0;
        }
        if (pScratch[7] >= 0x7C01) {
            pAcc[0] |= 0x7C00;
        } else {
            pAcc[0] |= pAcc[14] & 0x7C00;
        }
        value = pAcc[0] | (*pSrc++ & 0x8000);
        pAcc[0] = value;
        *pDst++ = value;
    }
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025D4C);
/* Best parked version 87.89% (604 bytes, retail's exact size).
Re-parked after a re-measure under this unit's preset (XenoGraphicsDiv, gcc
2.7.2-cdk): the parked body below still scores 87.89% at retail's exact 604
bytes, identical to the gcc-2.6.0 number, so the preset is not the difference.
The permuter cannot be pointed at this stub: its base source is the unit's
preprocessed file and the GTE macros are inline asm, which the permuter's C
parser rejects ('Syntax error in base.c ... mtc2').

Un-park screen: best 87.89% (604 bytes, the original's exact size). This stub
is NOT one of the unreachable "handwritten function" ones: gte_ldlvl (lwc2
$9/$10/$11 off the long[3] scratch) and gte_stlvl_gpr (swc2 back into the same
two words) from include/psyq/inline_c.h reproduce the GTE traffic exactly, and
gte_gpf12 supplies the two nops plus GPF, so the instruction multiset is right.
What is left is one allocator permutation plus the frame. Retail keeps redBias
in s0, the shifted green/blue biases in t9/t8, mode in t6, and does the
mode-3 mfhi into s1 (frame 0x18, two saved registers); this body needs a third
saved register (s0 = greenBias, s1 = redBias, s2 = mfhi), hence a 0x20 frame,
ships greenIn/blueIn in t6/t5 where retail has t3/t2, and puts the clamped red
in t3 where retail has a0. Tried and no move: shifts inline in the body vs
mutating the bias parameters, *pSrc++ vs a tail pSrc++, folding the average
local into redIn, hoisted redValue/greenValue/blueValue locals (81.21%),
a pDelta u16* local instead of the (u16*)d casts (81.65%, 624 bytes).
Signature from the only caller, func_800A6AE8 in battle/main/misc23:
(count = w * h, pSrc, pSrc2, pDst, redBias, greenBias, blueBias, mode, depth),
src/src2/dst the same buffer there, mode 0..3, depth from the same record.
Return: the blended colour is written only when the source colour is non-zero,
and always with bit 15 set.
// Shades count 16-bit colours of pSrc into pDst: the source colour is divided
// down (mode 0 keeps it, 1 halves it, 2 quarters it, 3 replaces it with the
// average of R, G and B) and biased per channel, then the GPF interpolates the
// difference against pSrc2's colour by depth and the result is added back.

Frame is the last structural gap, found this round: retail is 0x18 with TWO
saved registers (s0 = redBias, s1 = the mode-3 average) and keeps greenBias<<5
in t9 / blueBias<<10 in t8; this body needs THREE (s0 = greenBias, s1 =
redBias, s2 = average), hence 0x20. Swapping the two bias shift statements
(blueBias<<=10 first) changes nothing - 87.89 again - so retail's
`j` delay slot holding the green shift while ours holds the blue one is
allocator freedom, not statement order. Retail also builds the output word as
(red-part | 0x8000) then green then blue with an `ori 0x8000`, where gcc 2.6.0
hoists `li v1,-32768` into the beqz delay slot and ORs it at the end; that is
a reassociation tie, not a source shape.
void func_80025D4C(s32 count, u16* pSrc, u16* pSrc2, u16* pDst,
                       s32 redBias, s32 greenBias, s32 blueBias, s32 mode,
                       s32 depth) {
    long d[3];
    s16 red;
    s16 green;
    s16 blue;
    s32 redIn;
    s32 greenIn;
    s32 blueIn;
    s32 average;
    u16 color;

    if (depth >= 0x21) {
        depth = 0x20;
    }
    gte_lddp(depth << 7);
    greenBias <<= 5;
    blueBias <<= 10;
    while (--count != -1) {
        color = *pSrc++;
        switch (mode) {
        case 0:
            redIn = color & 0x1F;
            greenIn = color & 0x3E0;
            blueIn = color & 0x7C00;
            break;
        case 1:
            redIn = (color & 0x1E) >> 1;
            greenIn = (color & 0x3C0) >> 1;
            blueIn = (color & 0x7800) >> 1;
            break;
        case 2:
            redIn = (color & 0x1C) >> 2;
            greenIn = (color & 0x380) >> 2;
            blueIn = (color & 0x7000) >> 2;
            break;
        case 3:
            average = ((color & 0x1F) + ((color & 0x3E0) >> 5) +
                       ((color & 0x7C00) >> 10)) / 3;
            redIn = average;
            greenIn = average << 5;
            blueIn = average << 10;
            break;
        }
        red = redIn + redBias;
        if (red < 0) {
            red = 0;
        }
        green = greenIn + greenBias;
        if (green < 0) {
            green = 0;
        }
        blue = blueIn + blueBias;
        if (blue < 0) {
            blue = 0;
        }
        if (red >= 0x20) {
            red = 0x1F;
        }
        if (green >= 0x3E1) {
            green = 0x3E0;
        }
        if (blue >= 0x7C01) {
            blue = 0x7C00;
        }
        d[0] = (*pSrc2 & 0x1F) - red;
        d[1] = (*pSrc2 & 0x3E0) - green;
        d[2] = (*pSrc2 & 0x7C00) - blue;
        gte_ldlvl(d);
        gte_gpf12();
        gte_stlvl_gpr(d);
        if (color != 0) {
            *pDst = 0x8000 | (red + (((u16*)d)[0] & 0x1F)) |
                    (green + (((u16*)d)[2] & 0x3E0)) |
                    (blue + (((u16*)d)[4] & 0x7C00));
        }
        pSrc2++;
        pDst++;
    }
}
*/

// The identity scale MATRIX the rotated quad helper starts from and the four
// corner SVECTORs it projects (their z stays at the 0x1000 (1.0) the table is
// initialised with).
extern MATRIX D_800188CC;
extern SVECTOR D_8004FDC0[];

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025FA8);

// Parked at 97.85% (912 bytes = the original's exact size) under this unit's
// preset (XenoGraphicsDiv, gcc 2.7.2-cdk). Everything but the allocator matches:
// the frame (0xA0), every stack slot (geom offset 0x58/0x5C, p/flag 0x60/0x64,
// angle 0x68, old geom screen 0x70), both clamp chains and the uv stores are
// instruction for instruction. What is left:
//  - the saved registers are rotated: retail has the data offset in s5, the loop
//    index in s6, pDirent in s7 and D_8004FDC0's %hi in s4; this body has them
//    one slot lower (s4/s5/s6) with %hi in s7. A cyclic rotation, i.e. an
//    allocator tie, not a source shape.
//  - retail's `i = 0` lands BEFORE the loop's entry branch and its angle store
//    fills that branch's delay slot; here the store fills the count load's delay
//    slot and `i = 0` fills the branch slot.
// Measured and no move: `i = 0` as an explicit statement before the for, the
// for-init moved in/out of the init clause, the angle store before/inside/after
// the if-count guard wrapper, nCount as a separate local for the guard, 2*i vs
// i*2, and a 3-minute permuter run (best 324 vs base 490, no zero). The
// u16 localAngle[2] array and the geom/p/flag pairs declared as long[2] arrays
// are LOAD-BEARING: an 8-byte slot for each is what reproduces retail's frame
// (a plain u16 local is hoisted into a register by loop.c and the function drops
// to 93.45%, and scalars instead of arrays give a 0x98 frame). The `if (count !=
// 0)` wrapper must NOT be there: without it gcc inverts the loop and the entry
// test/init order changes.
/*
// Draws each record of the dirent as a rotated, scaled textured quad: builds a
// rotation/scale matrix, projects the record rectangle's corners through it with
// the geometry offset swapped for (x, y) for the duration, then writes the uv
// rectangle. A quad that ends up mirrored loses its first texel column (and row)
// so the neighbouring texel is not sampled, and a quarter turn loses one column
// unconditionally.
s32 func_80025FA8(MenuResourceDirectory* dir, u32 index, POLY_FT4* pPrims,
                  s32 context, s16 x, s16 y, s16 scaleX, s16 scaleY,
                  s16 angle) {
    MATRIX matrix;
    long scale[3];
    long oldGeomOffset[2];
    long pFlag[2];
    long oldGeomScreen;
    u16 localAngle[2];
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 nAngle;
    s32 nLeft;
    s32 nTop;
    s32 nRight;
    s32 nBottom;
    u16 u;
    u16 v;
    s32 width;
    s32 height;
    s32 i;

    matrix = D_800188CC;
    scale[0] = scaleX;
    scale[1] = scaleY;
    scale[2] = 0x1000;
    PushMatrix();
    ScaleMatrixL(&matrix, (VECTOR*)scale);
    RotMatrixZ(angle, &matrix);
    ReadGeomOffset(&oldGeomOffset[0], &oldGeomOffset[1]);
    oldGeomScreen = ReadGeomScreen();
    SetGeomOffset(x, y);
    SetGeomScreen(0x1000);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    i = 0;
    localAngle[0] = angle;
    for (; i != pDirent->count; i++) {
            pPrim = pPrims + i * 2 + context;
            pData = &pDirent->data[i];

            SetPolyFT4(pPrim);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            pPrim->tpage = GetTPage(pData->texPageId, 0, (s16)pData->texPageX,
                                    pData->texPageY);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);

            nRight = pData->width;
            nBottom = pData->height;
            nLeft = pData->x;
            nTop = (u16)pData->y;
            if (!pData->flipX) {
                D_8004FDC0[0].vx = nLeft;
                D_8004FDC0[1].vx = nLeft + nRight;
                D_8004FDC0[2].vx = nLeft + nRight;
                D_8004FDC0[3].vx = nLeft;
            } else {
                D_8004FDC0[0].vx = nLeft + nRight;
                D_8004FDC0[1].vx = nLeft;
                D_8004FDC0[2].vx = nLeft;
                D_8004FDC0[3].vx = nLeft + nRight;
            }
            if (!pData->flipY) {
                D_8004FDC0[2].vy = nTop + nBottom;
                D_8004FDC0[3].vy = nTop + nBottom;
                D_8004FDC0[0].vy = nTop;
                D_8004FDC0[1].vy = nTop;
            } else {
                D_8004FDC0[2].vy = nTop;
                D_8004FDC0[3].vy = nTop;
                D_8004FDC0[0].vy = nTop + nBottom;
                D_8004FDC0[1].vy = nTop + nBottom;
            }
            RotTransPers4(&D_8004FDC0[0], &D_8004FDC0[1], &D_8004FDC0[2],
                          &D_8004FDC0[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
                          (long*)&pPrim->x3, (long*)&pPrim->x2, &pFlag[0], &pFlag[1]);

            u = pData->u;
            v = (u16)pData->v;
            width = pData->width;
            height = pData->height;
            nAngle = localAngle[0] & 0xFFF;
            if (nAngle == 0xC00) {
                u = u - 1;
            }
            if (nAngle == 0) {
                if (pPrim->x3 < pPrim->x0) {
                    u = u - 1;
                    if ((s16)u < 0) {
                        u = 0;
                        width = width - 1;
                    }
                }
                if (pPrim->y3 < pPrim->y0) {
                    v = v - 1;
                    if ((s16)v < 0) {
                        v = 0;
                        height = height - 1;
                    }
                }
            }

            pPrim->u0 = u;
            pPrim->v0 = v;
            pPrim->u1 = u + width;
            pPrim->v1 = v;
            pPrim->u2 = u;
            pPrim->v2 = v + height;
            pPrim->u3 = u + width;
            pPrim->v3 = v + height;
    }

    SetGeomOffset(oldGeomOffset[0], oldGeomOffset[1]);
    SetGeomScreen(oldGeomScreen);
    PopMatrix();
    return pDirent->count;
}
*/

void ResourceHelperGetTexCoords(MenuResourceDirectory *dir, u32 index,
                   s32* outDirentCount,
                   s32* outtexPageId,
                   s32* outClutX, s32* outClutY, s32* outTexPageX, s32* outTexPageY) {
    int texPageX_lsb, u_shifted;
    MenuResourceDirentTextureMetadata *dirent;
    MenuResourceTextureMetadata *data;
    dirent = (MenuResourceDirentTextureMetadata *)(dir->offsets[index] + (unsigned) dir);
    data = &dirent->data[0];

    *outDirentCount = dirent->count;

    u_shifted = data->u << 0x10;
    if (data->texPageId != 0) {
        texPageX_lsb = u_shifted >> 0x12;
    } else {
        texPageX_lsb = u_shifted >> 0x14;
    }
    *outtexPageId = data->texPageId;
    *outClutX = data->clutX;
    *outClutY = data->clutY;
    *outTexPageX = (s16) (data->texPageX & 0xFFC0) + texPageX_lsb;
    *outTexPageY = (s16) (data->texPageY & 0xFF00) + data->v;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800263E4);

// Un-park screen: 81.59% under cdk (82.30% was an earlier attempt of this body),
// 75.89% under gcc-2.6.0, 76.39% under gcc-2.7.2, unchanged by -O3/-G0/aspsx.
// Parked: best 82.30% (816 bytes against 888). The corner fills, both clamps,
// the uv stores and the negation blocks already come out instruction for
// instruction; what does not match is register pressure. The original spills
// four more loop-carried values than this shape does (scale at 0x38, the x
// quotient at 0x3C, the record offset at 0x40 and pPrims at 0x44, plus
// `addu s3,s6,zero`-style eager copies before each conditional negation), so
// its source holds about four more long-lived variables than this one. What
// this attempt does reproduce: the 0x70 frame with localX at 0x10, localY at
// 0x18, u8 copies of the two mirror flags at 0x20/0x28, pDirent at 0x30 and i
// at 0x34. Adding a masked scaleVal local or (u16) truncations of the
// quotients did not move it.
// Signature is fixed by the three callers (shop_menu, member_change_menu,
// gear_shop_menu): (dir, index, pPrims, context, x, y, scale, mirrorX,
// mirrorY), mirrorY=1 for the bottom scroll-bar ornament.
/*
// Builds one textured quad per record of the dirent, scaled by `scale` and
// mirrored on either axis when asked to; u/v shrink by one texel when the quad
// comes out flipped so the last column/row is not sampled.
s32 func_800263E4(MenuResourceDirectory* dir, u32 index, POLY_FT4* pPrims,
                  s32 context, u16 x, u16 y, u16 scale, u8 mirrorX, u8 mirrorY) {
    u16 localX = x;
    u16 localY = y;
    u8 mirrorLocalX = mirrorX;
    u8 mirrorLocalY = mirrorY;
    s32 scaleVal = scale & 0xFFFF;
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 nX;
    s32 nY;
    s32 nWidth;
    s32 nHeight;
    u16 u;
    u16 v;
    u16 width;
    u16 height;
    u16 x0;
    u16 x1;
    u16 y0;
    u16 y1;
    s32 i = 0;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    if (pDirent->count != 0) {
        for (; i != pDirent->count; i++) {
            pData = &pDirent->data[i];
            pPrim = pPrims + i * 2 + context;

            nX = (s16)pData->x * scaleVal / 4096;
            nY = (s16)pData->y * scaleVal / 4096;
            nWidth = (s16)pData->width * scaleVal / 4096;
            nHeight = (s16)pData->height * scaleVal / 4096;

            SetPolyFT4(pPrim);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            pPrim->tpage = GetTPage(pData->texPageId, 0, (s16)pData->texPageX,
                                    pData->texPageY);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);

            if (mirrorLocalX) {
                nWidth = -nWidth;
                nX = -nX;
            }
            if (mirrorLocalY) {
                nY = -nY;
                nHeight = -nHeight;
            }

            u = pData->u;
            v = pData->v;
            width = pData->width;
            height = pData->height;

            if (!pData->flipX) {
                x0 = localX + nX;
                x1 = x0 + nWidth;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
            } else {
                x1 = localX + nX;
                x0 = x1 + nWidth;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
            }

            if (!pData->flipY) {
                y0 = localY + nY;
                y1 = y0 + nHeight;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
            } else {
                y1 = localY + nY;
                y0 = y1 + nHeight;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
            }

            if (pPrim->x3 < pPrim->x0) {
                u = u - 1;
                if ((s16)u < 0) {
                    u = 0;
                    width = width - 1;
                }
            }
            if (pPrim->y3 < pPrim->y0) {
                v = v - 1;
                if ((s16)v < 0) {
                    v = 0;
                    height = height - 1;
                }
            }

            pPrim->u0 = u;
            pPrim->v0 = v;
            pPrim->u1 = u + width;
            pPrim->v1 = v;
            pPrim->u2 = u;
            pPrim->v2 = v + height;
            pPrim->u3 = u + width;
            pPrim->v3 = v + height;
        }
    }
    return pDirent->count;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002675C);
/*
Un-park screen: cdk 92.96%, gcc-2.6.0 84.08%, gcc-2.7.2 88.09%, cdk -O3
92.96%, cdk -G0 92.96% - the park stays and cdk is right; the gap is the
scheduler, not the compiler.
Best 92.96% (696 bytes, same size as the original). Remaining gaps are
allocator/scheduler only: gcc hoists the lhu of scale to just after the dirent
count load (the original loads it after the pDirent store), so it fills the
beqz delay slot with the localY store where the original has a nop, and the
first quotient lands in a0/v1 instead of v1/a0.
The 91.30% attempt's srl-vs-sra puzzle is solved: assigning the y and height
quotients to u16 locals is what makes cdk gcc truncate and emit srl after the
same bgez/+0xFFF fixup (a signed int quotient gets sra, and the 91.30% attempt
had both y and height as s32). Everything else that got the earlier attempt
that far still holds: localX/localY declared before pDirent and i, the four
scaled reads cast to s16 (lh), v read through (u16), texPageX through (s16),
corners assigned in offset order, and pPrim computed before pData.
s32 func_8002675C(MenuResourceDirectory* dir, u32 index, POLY_FT4* pPrims,
                  s32 context, u16 x, u16 y, u16 scale) {
    u16 localX = x;
    u16 localY = y;
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 nX;
    u16 nY;
    s32 nWidth;
    u16 nHeight;
    u16 u;
    u16 v;
    u16 width;
    u16 height;
    u16 x0;
    u16 x1;
    u16 y0;
    u16 y1;
    s32 i = 0;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    if (pDirent->count != 0) {
        for (; i != pDirent->count; i++) {
            pPrim = pPrims + i * 2 + context;
            pData = &pDirent->data[i];

            nX = (s16)pData->x * scale / 4096;
            nY = (s16)pData->y * scale / 4096;
            nWidth = (s16)pData->width * scale / 4096;
            nHeight = (s16)pData->height * scale / 4096;

            SetPolyFT4(pPrim);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            pPrim->tpage = GetTPage(pData->texPageId, 0, (s16)pData->texPageX,
                                    pData->texPageY);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);

            u = pData->u;
            v = (u16)pData->v;
            width = pData->width;
            height = pData->height;
            if (!pData->flipX) {
                x0 = localX + nX;
                x1 = x0 + nWidth;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
            } else {
                u = pData->u - 1;
                x1 = localX + nX;
                x0 = x1 + nWidth;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
                if ((s16)u < 0) {
                    u = 0;
                    width = width - 1;
                }
            }

            if (!pData->flipY) {
                y0 = localY + nY;
                y1 = y0 + nHeight;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
            } else {
                v = pData->v - 1;
                y1 = localY + nY;
                y0 = y1 + nHeight;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
                if ((s16)v < 0) {
                    v = 0;
                    height = height - 1;
                }
            }

            pPrim->u0 = u;
            pPrim->v0 = v;
            pPrim->u1 = u + width;
            pPrim->v1 = v;
            pPrim->u2 = u;
            pPrim->v2 = v + height;
            pPrim->u3 = u + width;
            pPrim->v3 = v + height;
        }
    }
    return pDirent->count;
}
*/

// Builds one SPRT per record of the dirent into pSprites, two sprites per
// record (one per render context), and the DR_MODE that follows them.
// Returns the record count plus one, the number of 2-sprite slots written.
// i must be initialised at its declaration and the tail indexes with `i << 1`:
// both are what make gcc 2.7.2-cdk emit the original's block order.
s32 ResourceHelperBuildSprites(MenuResourceDirectory* dir, u32 index, SPRT* pSprites,
                  int context, u16 x, u16 y) {
    MenuResourceDirentTextureMetadata* pDirent;
    SPRT* pPrim;
    MenuResourceTextureMetadata* pData;
    u16 x0;
    u16 y0;
    u16 u0;
    u16 v0;
    u16 w;
    u16 h;
    int i = 0;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    if (pDirent->count) {
        for (; i != pDirent->count; i++) {
            pPrim = pSprites + i * 2 + context;
            pData = &pDirent->data[i];
            SetSprt(pPrim);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            x0 = x + pData->x;
            y0 = y + pData->y;
            u0 = pData->u;
            v0 = pData->v;
            w = pData->width;
            h = pData->height;
            pPrim->x0 = x0;
            pPrim->y0 = y0;
            pPrim->u0 = u0;
            pPrim->v0 = v0;
            pPrim->w = w;
            pPrim->h = h;
        }
    }
    pData = &pDirent->data[0];
    SetDrawMode((DR_MODE*)(pSprites + (i << 1) + context), 0, 0,
                GetTPage(pData->texPageId, 0, (s16)pData->texPageX, pData->texPageY) & 0xFFFF,
                NULL);
    return pDirent->count + 1;
}

void ResourceHelperNoOp(void) {
}

// Queues one semi-transparent textured quad per record of the dirent.
//
// Un-park screen: cdk 68.00%, gcc-2.6.0 67.64%, gcc-2.7.2 67.82%, cdk -G0
// 67.17% - no compiler switch helps; it is the GP-rel split below.
// Best 68.00%: blocked on the same GP-rel split as GfxSetRenderContext. The original
// object reads g_GfxCurWorkBuffer and g_GfxCurWorkBufferEnd with lui/%hi+%lo
// here (they were externs in this function's original TU), but the matched
// functions 80025180/800251C8/8002541C/80025544 need %gp_rel for both, which
// maspsx only emits when the unit itself defines the symbol in .sbss. This
// function can only match from a separate TU that does not define them.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026BA4);
/*
void func_80026BA4(MenuResourceDirectory* dir, u32 index, s32 x, s32 y, u_long* ot) {
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 count;
    s32 i;
    s32 x0;
    s32 y0;
    s32 w;
    s32 h;
    s32 u;
    s32 v;
    s32 u_shifted;
    s32 texPageX_lsb;
    s32 texPageX;
    s32 texPageY;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    count = pDirent->count;
    pPrim = g_GfxCurWorkBuffer;
    if ((u8*)(pPrim + count) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }

    pData = &pDirent->data[0];
    for (i = 0; i != count; i++, pData++) {
        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;

        setlen(pPrim, 9);
        setcode(pPrim, 0x2D);

        u_shifted = pData->u << 0x10;
        if (pData->texPageId != 0) {
            texPageX_lsb = u_shifted >> 0x12;
        } else {
            texPageX_lsb = u_shifted >> 0x14;
        }
        texPageX = (s16)(pData->texPageX & 0xFFC0) + texPageX_lsb;
        texPageY = (s16)(pData->texPageY & 0xFF00) + pData->v;

        x0 = pData->x + x;
        y0 = pData->y + y;
        w = pData->width;
        h = pData->height;
        u = pData->u;
        v = pData->v;

        pPrim->clut = GetClut(pData->clutX, pData->clutY);
        pPrim->tpage = GetTPage(pData->texPageId, 0, texPageX, texPageY);

        pPrim->x0 = x0;
        pPrim->x1 = x0 + w;
        pPrim->x2 = x0;
        pPrim->x3 = x0 + w;
        pPrim->y0 = y0;
        pPrim->y1 = y0;
        pPrim->y2 = y0 + h;
        pPrim->y3 = y0 + h;
        pPrim->u0 = u;
        pPrim->u1 = u + w;
        pPrim->u2 = u;
        pPrim->u3 = u + w;
        pPrim->v0 = v;
        pPrim->v1 = v;
        pPrim->v2 = v + h;
        pPrim->v3 = v + h;

        AddPrim(ot, pPrim);
    }
}
*/

// One sprite of a menu resource dirent, converted to screen space with its
// tpage/clut resolved. x/y stay unsigned: they are added to a screen offset.
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u8 u;
    /* 0x05 */ u8 v;
    /* 0x06 */ u8 width;
    /* 0x07 */ u8 height;
    /* 0x08 */ u16 unk8;
    /* 0x0A */ u16 tpage;
    /* 0x0C */ u16 clut;
    /* 0x0E */ u8 unkE[0xA];
} MenuResourceSprite; // Size: 0x18

// Converts the sprites of one dirent into out[]; returns the sprite count.
s32 ResourceHelperGetSpriteList(MenuResourceDirectory* dir, u32 index, MenuResourceSprite* out,
                  u16 x, u16 y) {
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    u32 count;
    u32 i;
    s32 u_shifted;
    s32 texPageX_lsb;
    s32 texPageId;
    s32 texPageX;
    s32 texPageY;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (mem_addr)dir);
    pData = pDirent->data;
    count = pDirent->count;
    for (i = 0; i != count; i++, pData++, out++) {
        u_shifted = pData->u << 0x10;
        if (pData->texPageId != 0) {
            texPageX_lsb = u_shifted >> 0x12;
        } else {
            texPageX_lsb = u_shifted >> 0x14;
        }
        texPageX = (s16)(pData->texPageX & 0xFFC0) + texPageX_lsb;
        texPageY = (s16)(pData->texPageY & 0xFF00) + pData->v;
        texPageId = pData->texPageId;
        out->clut = GetClut(pData->clutX, pData->clutY);
        out->tpage = GetTPage(texPageId, 1, texPageX, texPageY);
        out->u = pData->u;
        out->v = pData->v;
        out->width = pData->width;
        out->height = pData->height;
        out->x = pData->x + x;
        out->y = pData->y + y;
    }
    return count;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026F44);
/*
NOT C-REPRODUCIBLE: this stub is handwritten PSY-Q Lite assembly, not gcc output.
Two tells, both absent from all 946 matched .s files of slus_006.64:
  `addi $a1,$zero,0x20`  - ADDI (opcode 8). gcc and maspsx only ever emit the
      ADDIU/ORI forms for `li`, so no C produces this opcode.
  `bne $zero,$t2`        - gcc's mips.md branch patterns put the register
      first (`bne $t2,$zero`, printed `bnez`); $zero first never appears.
The loop shape itself was solved before that was noticed (see below); the body
below is the best C attempt and cannot be pushed further.
Best 74.88% (160 bytes, exactly the original's size). The LOOP SHAPE:
retail's unrotated top test with `li t2,-1` hoisted once before the loop is
`while (1) { if (--count == limit) break; ... }` with `s32 limit = -1;` - a
literal -1 in the compare makes gcc rematerialise it, a limit local is what
hoists it, and the while (1) break keeps gcc from duplicating the exit test
(160 bytes instead of 168). Shapes that do NOT work: while (--count != -1) and
for (;;) { if (--count == -1) break; } 72.54, for (i = count - 1; i != -1; i--)
72.66, for (i = count; i != limit; i--) 70.59.
What is left is register allocation of the nine body values only: retail keeps
color in t6 and ir1/ir2/ir3 in t5/t4/t3, this shape gets v0/t0/v1/a1, so the
final `rgb |= 1` comes out `ori t5,t5,1` there and `li v1,1; or` here, the two
`or`s differ in operand order, and the clamp prologue fills the beqz delay slot
with the level<<7 shift where retail has a nop and shifts after. A separate
`s32 dqp; dqp = level << 7; gte_lddp(dqp);` local changed nothing.
Round 61: shifting in place (`level <<= 7; gte_lddp(level);`) DOES reproduce
retail's in-place `sll a1,a1,0x7` instead of a copy into v0 (74.63 -> 74.88);
the `level >= 0x20` clamp shape is already right. What is left is reorg: retail
leaves a nop in the clamp's bne delay slot where we have the sll, and fills the
loop's `beq a0,t2` slot with the `lhu` where we emit a nop; plus the nine body
values land in t0/v1/a1 instead of t5/t4/t3.
void func_80026F44(s32 count, s32 level, u16* pDst, u16* pSrc) {
    u16 color;
    s32 limit;
    s32 ir1;
    s32 ir2;
    s32 ir3;
    s32 rgb;

    if (level >= 0x20) {
        level = 0x20;
    }
    level <<= 7;
    gte_lddp(level);
    limit = -1;
    while (1) {
        if (--count == limit) {
            break;
        }
        color = *pSrc++;
        ir1 = color & 0x1F;
        ir2 = color & 0x3E0;
        ir3 = color & 0x7C00;
        gte_ldir123(ir1, ir2, ir3);
        gte_gpf12();
        gte_stir123_gpr(ir1, ir2, ir3);
        ir1 &= 0x1F;
        ir2 &= 0x3E0;
        ir3 &= 0x7C00;
        rgb = ir1 | ir2;
        rgb = ir3 | rgb;
        if (color != 0 && rgb == 0) {
            rgb |= 1;
        }
        *pDst++ = (color & 0x8000) | rgb;
    }
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026FE8);
/*
Best 64.58% (184 bytes against 176). The content is right - the clamp, the
gte_lddp(level << 7), the per-field diff into IR1-IR3, gte_gpf12, the masked
re-add and the shared store. Two walls, both shared with func_80026F44 and
func_80025C04 above/below (same four-stub GPF family, the only stubs of the
unit the disassembler marks "Handwritten function"):
1. LOOP SHAPE. Retail enters the loop with no pre-loop test:
       addiu a0,a0,1                 <- preheader
   .L: addiu a0,a0,-1
       beqz a0,.Lend
       body
       j .L ; addi a2,a2,2
   so the decrement lives in a0 itself. Every spelling tried
   (for (i=count;i!=0;i--), i=count+1; while(--i!=0), and the shapes parked
   under func_80026F44) makes gcc's duplicate_loop_exit_test rotate it into a
   pre-loop dec+test plus a bottom test, which is 2 instructions more than
   retail. Identical under gcc 2.6.0 / 2.7.2 / 2.7.2-cdk at -O2 (all 64.58),
   so it is not the preset.
2. Retail's result sum is `add` (signed), ours `addu`, and retail re-masks
   *in* from memory per field (andi v1,t4,0x1F / 0x3E0 / 0x7C00 each time)
   instead of keeping the three masked copies, so the register allocation of
   the nine body values differs on top of the loop.

void func_80026FE8(s32 count, s32 level, u16* pDst, u16* pSrc, u16* pSrc2) {
    s32 i;
    u16 in;
    u16 base;
    s32 b1;
    s32 b2;
    s32 b3;
    s32 ir1;
    s32 ir2;
    s32 ir3;
    s32 rgb;

    if (level >= 0x21) {
        level = 0x20;
    }
    gte_lddp(level << 7);
    i = count + 1;
    while (--i != 0) {
        in = *pSrc2++;
        base = *pSrc;
        b1 = base & 0x1F;
        b2 = base & 0x3E0;
        b3 = base & 0x7C00;
        ir1 = (in & 0x1F) - b1;
        ir2 = (in & 0x3E0) - b2;
        ir3 = (in & 0x7C00) - b3;
        gte_ldir123(ir1, ir2, ir3);
        gte_gpf12();
        gte_stir123_gpr(ir1, ir2, ir3);
        ir1 = (ir1 & 0x1F) + b1;
        ir2 = (ir2 & 0x3E0) + b2;
        ir3 = (ir3 & 0x7C00) + b3;
        rgb = ir1 | ir2;
        rgb |= ir3;
        *pDst = rgb;
        pSrc++;
        pDst++;
    }
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002709C);
/* Best parked version 80.74% (808 bytes, retail's exact size).
Re-measured this round under the unit's own preset (XenoGraphicsDiv, gcc
2.7.2-cdk): un-parked, filled in the block typedef below (the parked copy had
its fields in // comments, so it did not compile), and it scores 80.74% at the
original's exact 808 bytes - so the fills, the loop bounds and the call set
are all right and only the register/frame layout is left, as the note below
says.
Two traps this cost, worth knowing before the next attempt:
1. THE PARK BLOCK CANNOT CONTAIN A STAR-SLASH. The first un-park declared the
   fields with psyq-style inline offset comments, whose own terminators close
   the enclosing park comment: the body then reads as live C and maspsx reports
   'symbol already defined'. The fields below therefore keep their offsets as
   trailing // comments, which also means the struct compiles as soon as the
   block is un-parked (the previously parked copy had its fields commented out
   and did not compile at all).
2. permute.py died on that same broken text ('Syntax error in base.c' at the
   star-slash) before it ever reached the body, so the permuter has NOT
   actually been tried on this function - with the block fixed above it is the
   obvious next step for a body that is register/frame layout only.

Parked at 80.74%, 808 bytes (the original's exact size), first body written
from the derivation below - so the fills, the six loops' bounds and the call
set are all right. What is left is register allocation only:
  retail frame 0xA0 with the width spill at 0x70, saved regs s0 = height,
  s1 = pVec, s2 = pColors, s3 = arg11, s5 = arg12, s6 = arg13, s7 = arg2,
  fp = arg3, and clutX/clutY NOT hoisted (reloaded from 0xB0/0xB4 inside the
  FT4 loop); this body spills arg2/arg3 to the outgoing-arg area instead,
  hoists clutY into a saved register and puts the width spill at 0x98, so the
  prologue and every stack offset are +8 from retail's.
Also note the parameter types the .s proves: width and height are s32 (a0 is
spilled with sw, height gets the plain `bgez` + addiu 0xFF of a 32-bit signed
%256 - an s16 height makes gcc emit sll/sra first), arg7/arg11/arg12/arg13 are
u16 (lhu) and pVec is read with lhu at 0/4/8 for the colour halves but lw at 8
for the `vz < 0` test, so pVec->vz is both.

// Block func_8002709C allocates: the prims func_800273C4 fills, then the
// draw state it copies in.

typedef struct {
    POLY_FT4 ft4[16];  // 0x000
    POLY_F4 f4[4];     // 0x280
    POLY_G4 g4[2];     // 0x2E0
    s32 unk328;        // 0x328
    s32 unk32C;        // 0x32C
    s32 unk330;        // 0x330
    u16 width;         // 0x334
    u16 height;        // 0x336
    u16 unk338;        // 0x338
    u16 unk33A;        // 0x33A
    u16 vx;            // 0x33C
    u16 vy;            // 0x33E
    u16 vz;            // 0x340
    u16 unk342;        // 0x342
    u16 unk344;        // 0x344
    u16 unk346;        // 0x346
    u16 unk348;        // 0x348
    u16 unk34A;        // 0x34A
} Temp2FillBlock; // Size: 0x34C

void* func_8002709C(s32 width, s32 height, s32 arg2, s32 arg3, s32 clutX,
                    s32 clutY, u16 arg7, s32 arg8, VECTOR* pVec, u8* pColors,
                    u16 arg11, u16 arg12, u16 arg13) {
    DRAWENV env;
    Temp2FillBlock* pBlock;
    s32 i;
    s32 j;

    HeapChangeCurrentUser(4, 0);
    pBlock = HeapAlloc(sizeof(Temp2FillBlock), 0);
    if (pBlock == NULL) {
        return NULL;
    }
    GetDrawEnv(&env);
    pBlock->unk328 = arg2;
    pBlock->unk32C = arg3;
    pBlock->vx = pVec->vx;
    pBlock->vy = pVec->vy;
    pBlock->vz = pVec->vz;
    pBlock->unk346 = arg11;
    pBlock->unk348 = arg12;
    pBlock->unk34A = arg13;
    if (pVec->vz < 0) {
        pBlock->unk330 = -arg8;
    } else {
        pBlock->unk330 = arg8;
    }
    pBlock->height = height;
    pBlock->width = width;
    pBlock->unk338 = arg7;
    pBlock->unk33A = height % 256;
    for (i = 0; i < 16; i++) {
        SetPolyFT4(&pBlock->ft4[i]);
        SetShadeTex(&pBlock->ft4[i], 1);
        pBlock->ft4[i].clut = GetClut(clutX, clutY);
    }
    if (pColors == NULL) {
        pBlock->unk344 = 0;
        return pBlock;
    }
    pBlock->unk344 = 1;
    for (j = 0; j < 2; j++) {
        SetPolyF4(&pBlock->f4[j]);
        pBlock->f4[j].r0 = pColors[0];
        pBlock->f4[j].g0 = pColors[1];
        pBlock->f4[j].b0 = pColors[2];
        pBlock->f4[j].x0 = 0;
        pBlock->f4[j].y0 = 0;
        pBlock->f4[j].x1 = 0x140;
        pBlock->f4[j].y1 = 0;
        pBlock->f4[j].x2 = 0;
        pBlock->f4[j].x3 = 0x140;
    }
    pColors += 4;
    for (j = 0; j < 2; j++) {
        pBlock->g4[j].r0 = pColors[0];
        pBlock->g4[j].g0 = pColors[1];
        pBlock->g4[j].b0 = pColors[2];
        pBlock->g4[j].r1 = pColors[0];
        pBlock->g4[j].g1 = pColors[1];
        pBlock->g4[j].b1 = pColors[2];
        pColors += 4;
        pBlock->g4[j].r2 = pColors[0];
        pBlock->g4[j].g2 = pColors[1];
        pBlock->g4[j].b2 = pColors[2];
        pBlock->g4[j].r3 = pColors[0];
        pBlock->g4[j].g3 = pColors[1];
        pBlock->g4[j].b3 = pColors[2];
        pColors -= 4;
        SetPolyG4(&pBlock->g4[j]);
        pBlock->g4[j].x0 = 0;
        pBlock->g4[j].x1 = 0x140;
        pBlock->g4[j].x2 = 0;
        pBlock->g4[j].x3 = 0x140;
    }
    pColors += 4;
    for (j = 2; j < 4; j++) {
        SetPolyF4(&pBlock->f4[j]);
        pBlock->f4[j].r0 = pColors[0];
        pBlock->f4[j].g0 = pColors[1];
        pBlock->f4[j].b0 = pColors[2];
        pBlock->f4[j].x0 = 0;
        pBlock->f4[j].x1 = 0x140;
        pBlock->f4[j].x2 = 0;
        pBlock->f4[j].y2 = 0xF0;
        pBlock->f4[j].x3 = 0x140;
        pBlock->f4[j].y3 = 0xF0;
    }
    return pBlock;
}
*/

/*
Derivation kept from the previous round; every offset below is taken
straight from the .s.
worker does not have to re-read 202 instructions. Every offset below is taken
straight from the .s.

Signature: 13 parameters. The only caller, func_80071828 in field/main/misc3,
sets stack slots 0x10..0x30 before the jal, i.e. args 5..13.
  a0 u16 width, a1 s16 height (a1 is stored with sh and used in the signed
  modulo below, so it is s16), a2 s32 at 0x328, a3 s32 at 0x32C,
  arg5/arg6 clutX/clutY (reloaded inside the FT4 loop, so GetClut is called
  there), arg7 u16 at 0x338, arg8 s32 negated when pVec->vz is negative,
  arg9 VECTOR* pVec (u16 read at 0, 4, 8 and the s32 at 8 tests < 0),
  arg10 u8* pColors, arg11/arg12/arg13 u16 at 0x346/0x348/0x34A.

Block returned by HeapAlloc(0x34C, 0) after HeapChangeCurrentUser(4, 0):
  0x000 POLY_FT4[16] (0x28 each), 0x280 POLY_F4[4] (0x18 each),
  0x2E0 POLY_G4[2] (0x24 each, the interleaved psyq layout),
  then s32 0x328, s32 0x32C, s32 0x330, u16 0x334 width, 0x336 height,
  0x338 arg7, 0x33A height mod 256 (signed: cdk emits the +0xFF fixup),
  0x33C/0x33E/0x340 the low halves of pVec vx/vy/vz, 0x344 flag,
  0x346/0x348/0x34A arg11..13. HeapAlloc(0x34C) is the frame check.

Statement order the asm shows: HeapChangeCurrentUser(4, 0); HeapAlloc;
if NULL return NULL; GetDrawEnv(&env) with a DRAWENV local at 0x10(sp)
(that is the whole 0xA0 frame); then 0x328, 0x32C, 0x33C, 0x33E, 0x340,
0x346, 0x348, 0x34A; then 0x330 = (pVec->vz < 0) ? -arg8 : arg8; then 0x334,
0x336, 0x338 and 0x33A; then the FT4 loop; then the pColors block.

pColors block: 0x344 = pColors ? 1 : 0. Then
  (a) two POLY_F4 at 0x280: SetPolyF4, r0/g0/b0 = pColors[0..2],
      x0=0, y0=0, x1=0x140, y1=0, x2=0, x3=0x140 - note y2 and y3 are NOT
      written here.
      then pColors += 4.
  (b) two POLY_G4 at 0x2E0: SetPolyG4, r0/g0/b0 = pColors[0..2],
      r1/g1/b1 = pColors[0..2], r2/g2/b2 = pColors[4..6],
      r3/g3/b3 = pColors[4..6], x0=0, x1=0x140, x2=0, x3=0x140.
      (the asm does addiu s2,s2,4 halfway and addiu s2,s2,-4 at the end of
      the iteration, net zero, then advances it by 4 after the loop; reading
      pColors[4..6] without the add/re-add is semantically identical but
      will not reproduce the two addius.)
      then pColors += 4.
  (c) two POLY_F4 at 0x2B0 (f4[2] and f4[3]): SetPolyF4, r0/g0/b0 =
      pColors[0..2], x0=0, x1=0x140, x2=0, y2=0xF0, x3=0x140, y3=0xF0 -
      here y0 and y1 are NOT written.
Returns the block pointer.

Why it is not written yet: the fills are six or eight byte/short stores per
primitive interleaved with three lbu loads, and the base register gcc picks
differs per block (s0 = base + i*0x28, base + i*0x18, base + i*0x24,
base + 0x30 + i*0x18), so the per-primitive statement set has to be written
EXACTLY as the original for the instruction order to come out. Start from the
fills above, diff, and only then chase the pColors add/re-add shape.
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800273C4);

// Derivation only, never diffed (no percentage yet); kept so the next worker
// does not have to re-read 274 instructions.
//
// Signature, from its only caller func_800273C4 (jal at 0x80027600):
//   (pBlock, s32 a1, s32 a2, s16 a3, u_long* pOT, s32 bank)
// a0 is the 0x34C block func_8002709C allocates; a3 is 0..0x100 from the
// caller's clamp and is the divisor's base everywhere below; arg6 is 0 or 1
// and picks which 0x140-byte half of the block is written. Every division is a
// real div (with the divide-by-zero and INT_MIN override sequences), so all
// divisors are runtime values. Frame 0x50, locals at 0x10 (u16), and y/a2/pOT
// are spilled to 0x18/0x1C/0x20 around the one call.
//
// 1. q  = (pBlock->unk328 << 8) / (a3 + 0x100)
//    h  = (0x140 - q) / 2
//    x  = h + (h * a3) / 0x100
//    d  = (s16)(a1 - x);
//    s3 = d % pBlock->unk328; if (s3 < 0) s3 += pBlock->unk328;
//    unk328 is loaded with lw here and with lhu at the modulo fixup, so the
//    source narrows it on one side. The signed remainder path (mfhi plus the
//    +unk328 fixup) is the same shape the caller spells at 0x80027570.
// 2. y = 0; x0 = 0;
//    if (a2 >= 0 && a2 <= pBlock->unk32C + 0xF0) y = (pBlock->unk32C << 8) / (a3 + 0x100);
//    One pre-test (bltz a2) plus one slt against unk32C+0xF0, and both paths
//    run addu x0,zero,zero, so it is a single if with an else, not two.
// 3. pPrimBase = (u8*)pBlock + (arg6 & 1) * 0x140;   a step of 0x28 * 8
//    if (y <= 0) return;   the test is sll/sra on y, so y is s16.
// 4. hoisted before the loop: u0 = (s16)pBlock->unk334 >> (2 - pBlock->unk338),
//    truncated to u16 in the 0x10(sp) local (sh and lhu of that slot).
// 5. loop for (i = 0; i < 8; i++, pPrim++), exit test slti i,8 with both
//    increments (0x28 each) in the delay slots. Per iteration the vertex
//    stores are sh +0x8, sh +0xA, sb +0xC, sb +0xD, sh +0x10, sh +0x12,
//    sb +0x14, sb +0x15, then the area +0x18/+0x1A/+0x1C/+0x1D and
//    +0x20/+0x22/+0x24/+0x25, with the u16 tpage result at +0x16.
//    So the 0x28-byte record is four vertices of { s16 x; s16 y; u8 u; u8 v }
//    at 0x8/0x10/0x18/0x20 plus a u16 tpage at 0x16 -- not a POLY_FT4; the
//    coordinates are 16-bit, which is why every coordinate store is sh.
//    Vertex u = the masked/wrapped column, v = (u8)pBlock->unk33A plus
//    (u8)pBlock->unk32C for the two lower vertices (both reread per use).
// 6. tpage = GetTPage((s16)pBlock->unk338, 0, ((s16)col / 64) * 64,
//                     ((s16)pBlock->unk336 / 256) * 256);
//    the (v < 0 ? v + 0x3F : v) >> 6 << 6 and (v < 0 ? v + 0xFF : v) >> 8 << 8
//    sequences are signed divide-then-multiply, so write the division out.
//    unk338 is lh (s16) and is used both as the tp argument and as the shift
//    amount of item 4; unk334 is lhu, unk336 is lh.
// 7. AddPrim(pOT, pPrim): the retail lw/lw/and 0xFF000000/and 0xFFFFFF/or/sw
//    pair IS libgs' AddPrim macro, so arg5 is a u_long* OT slot (reloaded from
//    the stack around GetTPage because it is call-clobbered).
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800278F8);

// First body written, 72.38% (972 bytes against the original's 1096). The
// structure above is confirmed (the derivation's "not a POLY_FT4" is wrong: it
// IS a POLY_FT4, whose vertex coordinates are s16, which is why every
// coordinate store is sh, and s0 = &pPrim->tpage is gcc's re-based common base,
// not a different record). Everything the .s shows is present: the same five
// divisions with the div traps, the same GetTPage arguments, AddPrim, the 8
// iteration bound and the break at rowEnd >= 0x140.
// What the diff still gets wrong, measured once:
//  - arg3 (span) is used as a raw s32 everywhere (mult v1,a3, addiu a2,a3,0x100):
//    this body declares it s32 and that part now matches, so do NOT narrow it.
//  - retail keeps arg2 in fp (move s8,t3) and computes the colour mask
//    0xFFFFFF into s5 (lui 0xff/ori 0xffff) BEFORE the loop; this body lets the
//    first fall to a stack slot and re-materialises the mask per use.
//  - retail's `half + (half * span) / 0x100` lands in its own register
//    (addu v1,v1,v0 then subu v0,a1,v1); writing it inline into the subtraction
//    fuses the two and shifts everything after it.
//  - the loop body's register assignment diverges from the first statement, so
//    the per-vertex store order (retail interleaves x/y/u/v across vertices:
//    x0,x1,x2,y2,x3,y3,u0,y0,y1,u1,v0,u2,v1,u3,v2,v3) was not chased yet.
// Worth about 20 more minutes with these four in hand.
/*
// The 0x34C render block func_8002709C allocates: sixteen textured quads (the
// two 0x140-byte banks this helper fills), then the prims and draw state it
// copies in.
typedef struct {
    POLY_FT4 ft4[16];              // 0x000
    u8 unk280[0xA8];               // 0x280
    s32 unk328;                    // 0x328
    s32 unk32C;                    // 0x32C
    s32 unk330;                    // 0x330
    u16 unk334;                    // 0x334
    u16 unk336;                    // 0x336
    u16 unk338;                    // 0x338
    u8 unk33A;                     // 0x33A
} Temp2RenderBlock; // Size: 0x33B

// Fills one bank of eight textured quads: the block's unk328 columns are spread
// over the 0x140-wide screen, each row of quads is column-shifted by unk334>>2,
// and the rows stop when they reach the screen edge or eight quads are written.
void func_800278F8(Temp2RenderBlock* pBlock, s32 left, s32 top, s32 span,
                   u_long* pOT, s32 bank) {
    POLY_FT4* pPrim;
    s32 quotient;
    s32 half;
    s32 col;
    s32 rows;
    s32 rowStart;
    s32 rowEnd;
    s32 u;
    s32 width;
    s32 height;
    s32 texColumn;
    s32 i;
    u16 uOffset;
    u16 v;

    quotient = (pBlock->unk328 << 8) / (span + 0x100);
    half = (0x140 - quotient) / 2;
    col = (s16)(left - (half + (half * span) / 0x100)) % pBlock->unk328;
    if ((s16)col < 0) {
        col += (u16)pBlock->unk328;
    }
    rows = 0;
    rowStart = 0;
    if (top >= 0 && top <= pBlock->unk32C + 0xF0) {
        rows = (pBlock->unk32C << 8) / (span + 0x100);
    }
    pPrim = (POLY_FT4*)((u8*)pBlock + (bank & 1) * 0x140);
    if ((s16)rows <= 0) {
        return;
    }
    uOffset = ((s16)pBlock->unk334 % 64) << (2 - (s16)pBlock->unk338);
    for (i = 0; i < 8; i++, pPrim++) {
        texColumn = (u16)pBlock->unk334 + ((s16)col >> (2 - (s16)pBlock->unk338));
        u = (col + uOffset) & ((0x100 >> (s16)pBlock->unk338) - 1);
        width = 0x100 - u;
        if (pBlock->unk328 < (s16)col + (s16)width) {
            width = (u16)pBlock->unk328 - col;
        }
        height = (width << 8) / (span + 0x100);
        rowEnd = rowStart + height;
        if ((s16)rowStart + (s16)height >= 0x141) {
            height = 0x140 - rowStart;
            col = (s16)(col + (height * (span + 0x100)) / 256) % pBlock->unk328;
        } else {
            col = (s16)(col + width) % pBlock->unk328;
        }
        v = pBlock->unk33A;
        pPrim->x0 = rowStart;
        pPrim->y0 = top - rows;
        pPrim->u0 = u;
        pPrim->v0 = v;
        pPrim->x1 = rowEnd;
        pPrim->y1 = top - rows;
        pPrim->u1 = width + u - 1;
        pPrim->v1 = v;
        pPrim->x2 = rowStart;
        pPrim->y2 = top;
        pPrim->u2 = u;
        pPrim->v2 = pBlock->unk33A + (u8)pBlock->unk32C;
        pPrim->x3 = rowEnd;
        pPrim->y3 = top;
        pPrim->u3 = width + u - 1;
        pPrim->v3 = pBlock->unk33A + (u8)pBlock->unk32C;
        pPrim->tpage = GetTPage((s16)pBlock->unk338, 0,
                                (s16)texColumn / 64 * 64,
                                (s16)pBlock->unk336 / 256 * 256);
        AddPrim(pOT, pPrim);
        if ((s16)rowEnd >= 0x140) {
            break;
        }
        rowStart = rowEnd;
    }
}
*/


void HeapFreeIfNotNull(void* pData) {
    if (pData) {
        HeapFree(pData);
    }
}
