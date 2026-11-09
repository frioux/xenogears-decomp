#include "common.h"
#include "system/menu.h"
#include "system/animation.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800248D4);

// Work list and image-list setup; D_800591A8 is the work list entry budget.
extern u32 D_800591A8;
extern u8 D_800591AD;
extern u8 D_800591AE;
extern void WorkListsReset(void);

void func_80024F20(void) {
    D_800591AD = 0;
    D_800591AE = 0;
    D_800591A8 = 0x2000;
    WorkListsReset();
    func_8001D298();
}



extern void func_8001D298(void);
extern void func_8001D2A4(void);

// Defined here so that the compiler places it in .sbss (%gp_rel)
Image* g_GfxImageList[2];

void GfxAllocateWorkBuffers(int workBufferSize, unsigned int allocFlag) {
    void* pWorkBuffers;

    g_GfxWorkBufferSize = workBufferSize;
    pWorkBuffers = HeapAlloc(workBufferSize * NUM_RENDER_CONTEXTS, allocFlag);
    g_GfxWorkBuffers = pWorkBuffers;
    g_GfxWorkBuffer2 = pWorkBuffers + workBufferSize;
    D_80059304 = 0;
    D_80059300 = 0;
    g_GfxImageList[0] = NULL;
    func_8001D298();
}

void GfxFreeWorkBuffers(void) {
    HeapFree(g_GfxWorkBuffers);
    func_8001D2A4();
}

void GfxSetCurrentOT(u_long* ot) {
    g_GfxCurOT = ot;
}








// Maybe start of a TU
// ===========================================================


// Set D_8004FBB8 matrix
void func_80024FF4(MATRIX* matrix) {
    D_8004FBB8 = *matrix;
}




// Maybe start of a TU
// ===========================================================

// Loops over the linked list given by g_GfxImageList[g_GfxCurContext].
// LoadImage on entries with an address, ClearImage otherwise.
// Set the list to NULL afterwards.
// GfxShapeTransfer
void func_80025044(void) {
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

// include/system/animation.h already types D_80059300 as a scalar; the real
// object is the two per-context list heads, so address it through a cast.
#define g_GfxSpriteTileLists ((LinkedListEntry**)&D_80059300)

// This function is in another TU than func_80024F64 due to GP Rel variables
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800250E0);
/*
Best 87.38%: blocked on the GP-rel split. The original object reads

g_GfxWorkBuffers through %hi/%lo here (it was an extern in this function's
original TU) but through %gp_rel in GfxAllocateWorkBuffers, which defines it.
maspsx decides per symbol, so one temp2.c cannot produce both spellings; the
only diff is `lw a0,0(gp); addu v1,v1,a0` vs `lui v0,%hi; addiu v0,v0,%lo;
addu v1,v1,v0; lw v0,0(v1)`.
Declare D_80059524 as a tentative definition (not extern) so maspsx emits
%gp_rel for it.
s32 D_80059524;
void func_800250E0(int context) {
    void* pPrimBuffer;
    LinkedListEntry* pCurEntry;

    pCurEntry = g_GfxSpriteTileLists[context];
    pPrimBuffer = ((void**)g_GfxWorkBuffers)[context];
    g_GfxCurContext = context;
    g_GfxCurWorkBuffer = pPrimBuffer;
    D_80059524 = pPrimBuffer;
    g_GfxCurWorkBufferEnd = pPrimBuffer + g_GfxWorkBufferSize;
    while (pCurEntry != NULL) {
        HeapFree(pCurEntry->pData);
        pCurEntry = pCurEntry->pNext;
    }

    g_GfxSpriteTileLists[context] = NULL;
}
*/

// GfxQueueSpriteTileData
void func_80025180(void* pData) {
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
void func_800251C8(u_long* addr, int x, int y, int width, s16 height) {
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
// func_80025258, func_80025710, func_80025718, NULL, NULL, func_80025258,
// func_80025258, func_80025718, func_8002541C, func_80025544, NULL, NULL,
// NULL, NULL, func_80025258, func_800257F0.
extern WorkListCallback_t D_8004FD40[];

// Sets pStruct->unk8 callback
void func_80025224(WorkListEntry* pTask, int handlerIndex) {
    WorkListSetTaskCallback(pTask, D_8004FD40[handlerIndex]);
}

extern s32 D_80050100;
extern u8 D_800C3664;
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void func_8001E298(SpriteData* pSpriteData, void* pArg);

// Projects the sprite's position and queues it into the OT at its own depth
// offset, either straight (bit 24) or through the model transform (bit 29).
void func_80025258(WorkListEntry* pTask) {
    SVECTOR vec;
    long vecTrans[3];
    long sxy;
    long flag;
    SpriteData* pSpriteData;
    MATRIX* pMatrix;
    s32 nOTOffset;
    u32 flags;

    pSpriteData = pTask->unk4;
    if ((((u32)pSpriteData->flags_0xB0 >> 8) & 1) && D_800C3664) {
        return;
    }

    vec.vx = pSpriteData->position.x >> 16;
    vec.vy = pSpriteData->position.y >> 16;
    vec.vz = pSpriteData->position.z >> 16;
    pMatrix = &D_8004FBB8;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    nOTOffset = (RotTransPers(&vec, &sxy, (u_long*)&sxy, &flag) >> D_80050100) +
               pSpriteData->field_0x30;
    if (flag & 0x8000) {
        nOTOffset = 0;
    }
    pSpriteData->field_0x2e = nOTOffset;

    flags = *(u32*)((u8*)pSpriteData + 0x3C);
    if ((flags >> 24) & 1) {
        func_80022038(pSpriteData);
        vecTrans[0] = pSpriteData->position.x >> 16;
        vecTrans[1] = pSpriteData->position.y >> 16;
        vecTrans[2] = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, (VECTOR*)vecTrans);
        SetRotMatrix(&pSpriteData->pBase->transformMatrix);
        SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        if ((*(u32*)((u8*)pSpriteData + 0x3C) >> 25) & 1) {
            nOTOffset = 0xFFF;
        } else {
            nOTOffset = pSpriteData->field_0x30;
        }
        if ((u32)(nOTOffset - 1) < 0xFFF) {
            func_8001E3D8(pSpriteData, &g_GfxCurOT[nOTOffset]);
        }
    } else {
        if ((flags >> 29) & 1) {
            nOTOffset = ((SpriteData*)pSpriteData->field_0x70)->field_0x2e;
        }
        if ((u32)(nOTOffset - 1) < 0xFFF) {
            func_8001E298(pSpriteData, &g_GfxCurOT[nOTOffset]);
        }
    }
}

void func_8002541C(WorkListEntry* pTask) {
    SVECTOR vec;
    int nFlag;
    int nOTOffset;
    DR_TPAGE* pPrimTPage;
    TILE_1* pPrim;
    u32 pNewBufferHead;
    u32 pNewBufferHead_2;
    SpriteData* pSpriteData;
    MATRIX* pMatrix;

    pSpriteData = pTask->unk4;
    // Read as unsigned: curSpriteFrame is s16 in the struct but -1 is a valid frame
    if (*(u16*)&pSpriteData->curSpriteFrame == 0) {
        pPrim = g_GfxCurWorkBuffer;
        pNewBufferHead = pPrim + 1;
        if (pNewBufferHead < g_GfxCurWorkBufferEnd) {
            pMatrix = &D_8004FBB8;
            vec.vx = pSpriteData->position.x >> 16;
            vec.vy = pSpriteData->position.y >> 16;
            vec.vz = pSpriteData->position.z >> 16;
            g_GfxCurWorkBuffer = pNewBufferHead;
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            nOTOffset = RotTransPers(&vec, &pPrim->x0, &nFlag, &nFlag) >> D_80050100;
            pSpriteData->field_0x2e = nOTOffset;

            // SetTile1
            setlen(pPrim, 2);
            *((u32*)&pPrim->r0) = *((u32*)&pSpriteData->red);

            AddPrim(&g_GfxCurOT[nOTOffset], pPrim);

            pPrimTPage = g_GfxCurWorkBuffer;
            pNewBufferHead_2 = pPrimTPage + 1;
            if (pNewBufferHead_2 < g_GfxCurWorkBufferEnd) {
                g_GfxCurWorkBuffer = pNewBufferHead_2;

                // Only the tpage bits of flags_0x3C make it into the primitive.
                setDrawTPage(pPrimTPage, 0, 0, pSpriteData->field_0x3C_0 & 0x60);
                AddPrim(&g_GfxCurOT[nOTOffset], pPrimTPage);
            }
        }
    }
}

/*
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
    pMatrix = &D_8004FBB8;
    g_GfxCurWorkBuffer = pPrim + 1;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);

    vecOffset = vec;
    *(u16*)&vecOffset.vx += xOffset;
    nOTOffset = RotTransPers3(&vec, &vecOffset, &vec, (long*)&pPrim->x0,
                              (long*)&sxy1, &sxy2, &flag, &flag) >> D_80050100;
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

void func_80025710(void) {}

extern void func_80022038(SpriteData* pSpriteData);
extern void ModelRenderPacket(void* pDirTransforms, void* pFramesData, u_long* pOT, int flags);

// Draws the sprite through its transform matrix, then its model parts.
void func_80025718(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vec;
    SpriteData* pSpriteData;

    pSpriteData = pTask->unk4;
    func_80022038(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, &vec);
        if (!(*(u8*)((u8*)pSpriteData + 0x3F) & 1)) {
            CompMatrix(&D_8004FBB8, &pSpriteData->pBase->transformMatrix, &matrix);
        }
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        ModelRenderPacket(pSpriteData->pBase->pDirTransforms,
                          ((void**)&pSpriteData->pBase->pFramesData)[g_GfxCurContext],
                          g_GfxCurOT, *(u16*)((u8*)pSpriteData + 0x42) & 4);
    }
}

extern void func_800B1F6C(void* pDirTransforms, void* pFramesData, u_long* pOT,
                          int flags, s32 arg4, s32 arg5);
extern MATRIX D_8004FD80;
extern MATRIX D_8004FDA0;

// Builds the sprite's color and light matrices from its base scale/rotation,
// then draws its model parts; a negative flag word saves and restores the
// geometry offset around the draw.
void func_800257F0(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vec;
    MATRIX colorMatrix;
    MATRIX lightMatrix;
    long geomOffsetX;
    long geomOffsetY;
    SpriteData* pSpriteData;
    s32 oldShift;

    pSpriteData = pTask->unk4;
    func_80022038(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        if ((*(u32*)((u8*)pSpriteData + 0x40) >> 1) & 1) {
            PushMatrix();
            // The scale values go in the m[i][0] slots (bytes 0, 6, 0xC), which
            // is where the static initialiser keeps its 0xC00 identity values.
            D_8004FD80.m[0][0] = *(u16*)((u8*)pSpriteData->pBase + 0x4C);
            D_8004FD80.m[1][0] = *(u16*)((u8*)pSpriteData->pBase + 0x4E);
            D_8004FD80.m[2][0] = *(u16*)((u8*)pSpriteData->pBase + 0x50);
            RotMatrix((SVECTOR*)((u8*)pSpriteData->pBase + 0x44), &colorMatrix);
            MulMatrix0(&colorMatrix, &pSpriteData->pBase->transformMatrix,
                       &colorMatrix);
            MulMatrix0(&D_8004FDA0, &colorMatrix, &lightMatrix);
            SetBackColor(0x20, 0x20, 0x20);
            SetColorMatrix(&D_8004FD80);
            SetLightMatrix(&lightMatrix);
            PopMatrix();
        }

        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, &vec);
        if (!(*(u8*)((u8*)pSpriteData + 0x3F) & 1)) {
            CompMatrix(&D_8004FBB8, &pSpriteData->pBase->transformMatrix,
                       &matrix);
            SetRotMatrix(&matrix);
            SetTransMatrix(&matrix);
        } else {
            SetRotMatrix(&pSpriteData->pBase->transformMatrix);
            SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        }

        if (*(s32*)((u8*)pSpriteData + 0x3C) < 0) {
            ReadGeomOffset(&geomOffsetX, &geomOffsetY);
            SetGeomOffset(0xA0, 0x70);
        }
        if ((*(u32*)((u8*)pSpriteData + 0x3C) >> 25) & 1) {
            oldShift = D_80050100;
            D_80050100 = 0x10;
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          ((void**)&pSpriteData->pBase->pFramesData)[g_GfxCurContext],
                          g_GfxCurOT, 0, 0xFEC,
                          *(u8*)((u8*)pSpriteData + 0x3C) >> 5);
            D_80050100 = oldShift;
        } else {
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          ((void**)&pSpriteData->pBase->pFramesData)[g_GfxCurContext],
                          g_GfxCurOT, 0, pSpriteData->field_0x30,
                          *(u8*)((u8*)pSpriteData + 0x3C) >> 5);
        }
        if (*(s32*)((u8*)pSpriteData + 0x3C) < 0) {
            SetGeomOffset(geomOffsetX, geomOffsetY);
        }
    }
}

// Draws the sprite through the matrix D_8004FBB8 composed with its own, then
// hands the model parts to func_800B1F6C.
void func_80025A88(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vecResult;
    SVECTOR vec;
    SpriteData* pSpriteData;
    s32 oldShift;

    pSpriteData = pTask->unk4;
    func_80022038(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        ApplyMatrix(&D_8004FBB8, &vec, &vecResult);
        pSpriteData->pBase->transformMatrix.t[0] = D_8004FBB8.t[0] + vecResult.vx;
        pSpriteData->pBase->transformMatrix.t[1] = D_8004FBB8.t[1] + vecResult.vy;
        pSpriteData->pBase->transformMatrix.t[2] = D_8004FBB8.t[2] + vecResult.vz;
        SetRotMatrix(&pSpriteData->pBase->transformMatrix);
        SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        if ((*(u32*)((u8*)pSpriteData + 0x3C) >> 25) & 1) {
            oldShift = D_80050100;
            D_80050100 = 0x10;
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          ((void**)&pSpriteData->pBase->pFramesData)[g_GfxCurContext],
                          g_GfxCurOT, 0, 0xFEC,
                          *(u8*)((u8*)pSpriteData + 0x3C) >> 5);
            D_80050100 = oldShift;
        } else {
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          ((void**)&pSpriteData->pBase->pFramesData)[g_GfxCurContext],
                          g_GfxCurOT, 0, pSpriteData->field_0x30,
                          *(u8*)((u8*)pSpriteData + 0x3C) >> 5);
        }
    }
}

// splat marks 80025C04, 80025D4C, 80026F44 and 80026FE8 "Handwritten
// function": raw mtc2/mfc2/lwc2/GTE sequences against fixed GTE registers,
// i.e. Psy-Q Lite assembly, not gcc output. No C reproduces the fixed
// $9/$10/$11 traffic, so these are not reachable from C.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025C04);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025D4C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025FA8);

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
s32 func_80026A0C(MenuResourceDirectory* dir, u32 index, SPRT* pSprites,
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

void func_80026B9C(void) {
}

// Queues one semi-transparent textured quad per record of the dirent.
//
// Best 68.00%: blocked on the same GP-rel split as func_800250E0. The original
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
s32 func_80026DCC(MenuResourceDirectory* dir, u32 index, MenuResourceSprite* out,
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

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    pData = &pDirent->data[0];
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026FE8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002709C);
/*
Status: derivation only, never diffed (no percentage yet); kept so the next
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800278F8);

void func_80027D40(void* pData) {
    if (pData) {
        HeapFree(pData);
    }
}
