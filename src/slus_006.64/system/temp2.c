#include "common.h"
#include "system/menu.h"
#include "system/animation.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800248D4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80024F20);



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
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025044);
/*
Matches on GCC 2.7.2-970404, ASPSX 2.67
Co-Authored-By: dezgeg <dezgeg@users.noreply.github.com>

typedef struct {
    RECT rect;
    u_long* addr;
    struct Image* pNext;
} Image;

int g_GfxCurContext;
extern Image* g_GfxImageList[2];

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
*/

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


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025180);
/*
typedef struct {
    void* pData; // Sprite Tile data stuff
    struct LinkedListEntry* pNext;
} LinkedListEntry;

extern LinkedListEntry* D_80059300[2];
int g_GfxCurContext;
void* g_GfxCurWorkBuffer;

void func_80025180(void* pData) {
    LinkedListEntry* pNewEntry;

    pNewEntry = (LinkedListEntry*) g_GfxCurWorkBuffer;
    g_GfxCurWorkBuffer = pNewEntry + 1;
    if (pNewEntry) {
        pNewEntry->pData = pData;
        pNewEntry->pNext = D_80059300[g_GfxCurContext];
        D_80059300[g_GfxCurContext] = pNewEntry;
    }
}
*/

// GfxQueueShapeTransfer
// Add Image to current g_GfxImageList[g_GfxCurContext] linked list
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800251C8);
/*
Matches, but uses variable in COMMON so can't compile in yet.

struct ImageEntry {
    RECT rect;
    u_long* addr;
    struct ImageEntry* pNext;
};

typedef struct ImageEntry ImageEntry;

extern ImageEntry* g_GfxImageList[];
int g_GfxCurContext; // Cur index?
void* g_GfxCurWorkBufferEnd;
ImageEntry* g_GfxCurWorkBuffer;

// GfxQueueShapeTransfer
void func_800251C8(u_long* addr, int x, int y, int width, int height) {
    ImageEntry* pCurrent;
    ImageEntry* pNext;

    pCurrent = (ImageEntry*) g_GfxCurWorkBuffer;
    pNext = pCurrent + 1;
    if (pNext < g_GfxCurWorkBufferEnd) {
        pCurrent->rect.h = height;
        pCurrent->rect.x = x;
        pCurrent->rect.y = y;
        pCurrent->rect.w = width;
        pCurrent->addr = addr;
        g_GfxCurWorkBuffer = (void*) pNext;

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002541C);
/*
Matches on GCC 2.7.2-970404, ASPSX 2.67
Co-Authored-By: dezgeg <dezgeg@users.noreply.github.com>
Co-Authored-By: Mc-muffin <Mc-muffin@users.noreply.github.com>

extern MATRIX D_8004FBB8;
extern s32 D_80050100;
void* g_GfxCurWorkBufferEnd; // End of Prim buffer
extern u_long* g_GfxCurOT;
void* g_GfxCurWorkBuffer; // Prim buffer

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
    if (pSpriteData->frameIdToRender == 0) {
        pPrim = g_GfxCurWorkBuffer;
        pNewBufferHead = pPrim + 1;
        if (pNewBufferHead < g_GfxCurWorkBufferEnd) {
            pMatrix = &D_8004FBB8;
            vec.vx = pSpriteData->unkX >> 16;
            vec.vy = pSpriteData->unkY >> 16;
            vec.vz = pSpriteData->unkZ >> 16;
            g_GfxCurWorkBuffer = pNewBufferHead;
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            nOTOffset = RotTransPers(&vec, &pPrim->x0, &nFlag, &nFlag) >> D_80050100;
            pSpriteData->unk2E = nOTOffset;

            // SetTile1 / 8 / 16
            setlen(pPrim, 2);
            *((u32*)&pPrim->r0) =  *((u32*)&pSpriteData->primR);
            
            AddPrim(&g_GfxCurOT[nOTOffset], pPrim);

            pPrimTPage = g_GfxCurWorkBuffer;
            pNewBufferHead_2 = pPrimTPage + 1;
            if (pNewBufferHead_2 < g_GfxCurWorkBufferEnd) {
                g_GfxCurWorkBuffer = pNewBufferHead_2;

                // SpriteData->flags3C & 0x60 => tpage
                // setDrawTPage does ((u_long *)(p))[1] = _get_mode(dfe, dtd, tpage)
                // _get_mode(dfe, dtd, tpage) would OR in 0x200 and 0x400 if dfe or dtd was not 0,
                // so we're only left with the tpage as a possibility
                setDrawTPage(pPrimTPage, 0, 0, pSpriteData->flags3C & 0x60);
                AddPrim(&g_GfxCurOT[nOTOffset], pPrimTPage);
            }
        }
    }
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025544);

void func_80025710(void) {}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025718);

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026A0C);

void func_80026B9C(void) {
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026BA4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026DCC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026F44);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80026FE8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002709C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800273C4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800278F8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80027D40);
