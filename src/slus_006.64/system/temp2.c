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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800257F0);

extern void func_800B1F6C(void* pDirTransforms, void* pFramesData, u_long* pOT,
                          int flags, s32 arg4, s32 arg5);

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002675C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026A0C);
/*
Best 77.49% (380 bytes against 404). Structure is right; three source-level
gaps remain. 1) The original spills x to a stack slot (sh t0,0x18(sp) at entry,
lhu 0x18(sp) per iteration) which is what makes its frame 0x48 where this one is
0x40; declaring xOffset volatile restores the frame and slot but perturbs the
record-field loads (75.19%). 2) In the loop the original reduces the sprite
address to pSprites+i*40 walking (addiu s3,40) plus an invariant context*20
(s7), while gcc 2.7.2-cdk reduces (context+i*2) and multiplies by 20 per
iteration; `i*2 + context` and `context + i*2` compile the same, and int vs u32
i makes no difference. 3) The original loads all six record fields before it
stores any of them (lhu+sb for u/v, so the values pass through 16-bit locals,
which this attempt already does).
Under gcc-2.7.2-psx this C scores 72.10%, so cdk is the right compiler.
// Builds one SPRT per record of the dirent into pSprites, two sprites per
// record (one per render context), and the DR_MODE that follows them.
// Returns the record count plus one, the number of 2-sprite slots written.
s32 func_80026A0C(MenuResourceDirectory* dir, u32 index, SPRT* pSprites,
                  int context, u16 x, u16 y) {
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    SPRT* pPrim;
    u16 xOffset;
    u16 x0;
    u16 y0;
    u16 u0;
    u16 v0;
    u16 w;
    u16 h;
    int i;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    xOffset = x;
    if (pDirent->count) {
        for (i = 0; i != pDirent->count; i++) {
            pPrim = &pSprites[context + i * 2];
            SetSprt(pPrim);
            pData = &pDirent->data[i];
            pPrim->clut = GetClut(pData->clutX, pData->clutY);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            x0 = xOffset + pData->x;
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
    SetDrawMode((DR_MODE*)&pSprites[i * 2 + context], 0, 0,
                GetTPage(pData->texPageId, 0, (s16)pData->texPageX, pData->texPageY) & 0xFFFF,
                NULL);
    return pDirent->count + 1;
}
*/

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800273C4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800278F8);

void func_80027D40(void* pData) {
    if (pData) {
        HeapFree(pData);
    }
}
