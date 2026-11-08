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

// This function is in another TU than func_80024F64 due to GP Rel variables
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800250E0);
/*
Matches on GCC 2.7.2-970404, ASPSX 2.67

typedef struct {
    void* pData;
    struct LinkedListEntry* pNext;
} LinkedListEntry;

s32 g_GfxCurContext;
int g_GfxWorkBufferSize;
extern LinkedListEntry* D_80059300[2];
extern int g_GfxWorkBuffers[];
s32 D_80059524;
void* g_GfxCurWorkBufferEnd;
void* g_GfxCurWorkBuffer;

void func_800250E0(int context) {
    void* pPrimBuffer;
    LinkedListEntry* pCurEntry;

    pPrimBuffer = g_GfxWorkBuffers[context];
    pCurEntry = D_80059300[context];
    g_GfxCurContext = context;
    g_GfxCurWorkBuffer = pPrimBuffer;
    D_80059524 = pPrimBuffer;
    g_GfxCurWorkBufferEnd = pPrimBuffer + g_GfxWorkBufferSize;
    while (pCurEntry != NULL) {
        HeapFree(pCurEntry->pData);
        pCurEntry = pCurEntry->pNext;
    }
    
    D_80059300[context] = NULL;
}
*/


typedef struct {
    void* pData; // Sprite Tile data stuff
    struct LinkedListEntry* pNext;
} LinkedListEntry;

// include/system/animation.h already types D_80059300 as a scalar; the real
// object is the two per-context list heads, so address it through a cast.
#define g_GfxSpriteTileLists ((LinkedListEntry**)&D_80059300)

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
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800251C8);
/*
Best 86.30%: identical instructions, but gcc inlines the height argument load
(lw v1, 0x10($sp)) inside the branch instead of hoisting it above it.
void func_800251C8(u_long* addr, int x, int y, int width, int height) {
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
*/

// Sets pStruct->unk8 callback
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025224);
/*
CALLBACK_TABLE
8004fd40 func_80025258
8004fd44 func_80025710 => Dummy function
8004fd48 func_80025718
8004fd4c NULL
8004fd50 NULL
8004fd54 func_80025258
8004fd58 func_80025258
8004fd5c func_80025718
8004fd60 func_8002541c
8004fd64 func_80025544
8004fd68 NULL
8004fd6c NULL
8004fd70 NULL
8004fd74 NULL
8004fd78 func_80025258
8004fd7c func_800257f0

void func_80025224(WorkListEntry* pTask, int handlerIndex) {
    WorkListSetTaskCallback(pTask, &CALLBACK_TABLE[handlerIndex]);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025258);

extern s32 D_80050100;

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025544);

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025A88);

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

// One slot of a menu sprite array: a SPRT packet whose two shorts after clut
// hold the sprite's size.
typedef struct {
    /* 0x00 */ u_long tag;
    /* 0x04 */ u8 r0, g0, b0, code;
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
    /* 0x0C */ u8 u;
    /* 0x0D */ u8 v;
    /* 0x0E */ u16 clut;
    /* 0x10 */ u16 width;
    /* 0x12 */ u16 height;
} MenuSpriteSlot; // Size: 0x14

// Builds one SPRT per texture record into the pOut slots at `slot`.
s32 func_80026A0C(MenuResourceDirectory* dir, u32 index, u8* pOut, u32 slot,
                  u16 x, u16 y) {
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    MenuSpriteSlot* pSlot;
    s32 i;
    s32 texX;
    s32 texY;
    s32 texU;
    s32 texV;
    s32 texW;
    s32 texH;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    for (i = 0; i != pDirent->count; i++) {
        pSlot = (MenuSpriteSlot*)(pOut + i * 0x28 + slot * 0x14);
        pData = &pDirent->data[i];
        SetSprt((SPRT*)pSlot);
        pSlot->clut = GetClut(pData->clutX, pData->clutY);
        SetSemiTrans(pSlot, 0);
        SetShadeTex(pSlot, 1);
        texX = pData->x + x;
        texY = pData->y + y;
        texU = pData->u;
        texV = pData->v;
        texW = pData->width;
        texH = pData->height;
        pSlot->x = texX;
        pSlot->y = texY;
        pSlot->u = texU;
        pSlot->v = texV;
        pSlot->width = texW;
        pSlot->height = texH;
    }
    pData = &pDirent->data[0];
    SetDrawMode((DR_MODE*)(pOut + i * 0x28 + slot * 0x14), 0, 0,
                GetTPage(pData->texPageId, 0, *(s16*)&pData->texPageX, pData->texPageY), NULL);
    return pDirent->count + 1;
}

void func_80026B9C(void) {
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026BA4);
/*
Best 54.49%. The instruction shapes line up, but this function came from an
original TU that reaches the work buffers with %hi/%lo (lui+lw), while maspsx
turns every reference in this unit into %gp_rel because
include/system/animation.h tentatively defines them here. An
extern ... asm("g_GfxCurWorkBuffer") alias does not help - maspsx keys off the
.comm, not the C declaration - so this needs the work buffers to stop being
defined in this unit.

// Draws each quad of one dirent into the current work buffer and chains it
// onto pOT; x/y are screen offsets added to every corner.
void func_80026BA4(MenuResourceDirectory* dir, u32 index, int x, int y, u_long* pOT) {
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 count;
    s32 i;
    s32 u_shifted;
    s32 texPageX_lsb;
    s32 texPageId;
    s32 texPageX;
    s32 texPageY;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    count = pDirent->count;
    if (g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) < g_GfxCurWorkBufferEnd && count != 0) {
        for (i = 0; i != count; i++) {
            pData = &pDirent->data[i];
            pPrim = g_GfxCurWorkBuffer;
            g_GfxCurWorkBuffer += sizeof(POLY_FT4);
            setlen(pPrim, 9);
            pPrim->code = 0x2D;

            u_shifted = pData->u << 0x10;
            if (pData->texPageId != 0) {
                texPageX_lsb = u_shifted >> 0x12;
            } else {
                texPageX_lsb = u_shifted >> 0x14;
            }
            texPageX = (s16)(pData->texPageX & 0xFFC0) + texPageX_lsb;
            texPageY = (s16)(pData->texPageY & 0xFF00) + pData->v;
            texPageId = pData->texPageId;
            pPrim->clut = GetClut(pData->clutX, pData->clutY);
            pPrim->tpage = GetTPage(texPageId, 0, texPageX, texPageY);

            pPrim->x0 = pData->x + x;
            pPrim->x1 = pPrim->x0 + pData->width;
            pPrim->x2 = pPrim->x0;
            pPrim->x3 = pPrim->x1;
            pPrim->y0 = pData->y + y;
            pPrim->y1 = pPrim->y0;
            pPrim->y2 = pPrim->y0 + pData->height;
            pPrim->y3 = pPrim->y2;
            pPrim->u0 = pData->u;
            pPrim->u1 = pPrim->u0 + pData->width;
            pPrim->u2 = pPrim->u0;
            pPrim->u3 = pPrim->u1;
            pPrim->v0 = pData->v;
            pPrim->v1 = pPrim->v0;
            pPrim->v2 = pPrim->v0 + pData->height;
            pPrim->v3 = pPrim->v2;
            AddPrim(pOT, pPrim);
        }
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
