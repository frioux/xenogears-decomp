#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/effects.h"
#include "field/graphics.h"
#include "field/particles.h"


s32 FieldActorModelComputeCenterAndSize(FieldActorModelData* pModel) {
    s32 minX;
    s32 minY;
    s32 minZ;
    FieldActorModelBounds* pBounds;
    s32 deltaX;
    s32 deltaY;
    s32 deltaZ;
    s32 maxDelta;

    pBounds = pModel->pBounds;
    maxDelta = pBounds->max.vx - (minX = pBounds->min.vx);
    deltaX = maxDelta;
    deltaY = pBounds->max.vy - (minY = pBounds->min.vy);
    deltaZ = pBounds->max.vz - (minZ = pBounds->min.vz);
    if (deltaY > maxDelta) {
        maxDelta = deltaY;
    }
    if (deltaZ > maxDelta) {
        maxDelta = deltaZ;
    }

    pModel->centerX = deltaX / 2 + minX;
    pModel->centerY = deltaY / 2 + minY;
    pModel->centerZ = deltaZ / 2 + minZ;
    pModel->size = maxDelta * 2 + 1;
    return maxDelta * 2 + 1;
}

extern MATRIX g_FieldModelScreenMatrix;
extern s32 g_FieldScreenScrollX;
extern s32 g_FieldScreenScrollY;

// Project the two silhouette points of a model and check they are on screen
s32 FieldActorModelIsOnScreen(FieldActorModelData* pModel) {
    VECTOR vec;
    SVECTOR point;
    long flag;
    long sxy;
    long p;
    s32 size;
    int y1;
    int x1;
    int y2;
    int x2;

    RotTrans((SVECTOR*)&pModel->centerX, &vec, &flag);
    g_FieldModelScreenMatrix.t[0] = vec.vx;
    g_FieldModelScreenMatrix.t[1] = vec.vy;
    g_FieldModelScreenMatrix.t[2] = vec.vz;
    SetRotMatrix(&g_FieldModelScreenMatrix);
    SetTransMatrix(&g_FieldModelScreenMatrix);

    size = pModel->size;
    point.vx = -size;
    point.vy = -size;
    point.vz = 0;
    RotTransPers(&point, &sxy, &p, &flag);
    y1 = sxy >> 16;
    x1 = (s16)sxy;

    point.vx = size;
    point.vy = size;
    point.vz = 0;
    RotTransPers(&point, &sxy, &p, &flag);
    y2 = sxy >> 16;
    x2 = (s16)sxy;

    if (y1 < g_FieldScreenScrollY + 0xE0 && -g_FieldScreenScrollY < y2 && x1 < g_FieldScreenScrollX + 0x140 && -g_FieldScreenScrollX < x2) {
        return 0;
    }
    return -1;
}

extern SpriteList* g_FieldSpriteSlots;

void FieldFreeSpriteList(void) {
    HeapFree(g_FieldSpriteSlots);
    DrawSync(0);
}

void FieldInitSpriteList(void) {
    RECT rect;
    SPRT* pSprite;
    SPRT* pSprite2;
    int i;

    g_FieldSpriteSlots = HeapAlloc(0x840, 0x0);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 0x21; i++) {
        SetDrawMode(&g_FieldSpriteSlots->drModes[i][0], 0, 0, GetTPage(0, 0, 0x3C0, 0x100) & 0xFFFF, &rect);
        SetDrawMode(&g_FieldSpriteSlots->drModes[i][1], 0, 0, GetTPage(0, 0, 0x3C0, 0x140) & 0xFFFF, &rect);
        pSprite = &g_FieldSpriteSlots->sprites[i][0];
        pSprite2 = &g_FieldSpriteSlots->sprites[i][1];
        
        SetSprt(pSprite);
        setRGB0(pSprite, 0x80, 0x80, 0x80);
        if (i == 0) {
            setUV0(pSprite, 0xE0, 0x70);
            setWH(pSprite, 0x10, 0x10);
        } else {
            setUV0(pSprite, 0xE0, 0x60);
            setWH(pSprite, 8, 8);
        }
        setXY0(pSprite, 0xA0, 0x70);
        pSprite->clut = GetClut(0x100, 0xF7);
        *pSprite2 = *pSprite;
    };
}

// Set RGB of sprites
void FieldSetSpriteColor(int index, int red, int green, int blue) {
    setRGB0(&g_FieldSpriteSlots->sprites[index][0], red, green, blue);
    setRGB0(&g_FieldSpriteSlots->sprites[index][1], red, green, blue);
}

void FieldDrawSprite(int index, int x, int y, int type) {
    switch (type) {
        case 0:
            y -= 0xC;
            x -= 4;
            break;
        case 1:
            y -= 4;
            x -= 4;
            break;
    }

    g_FieldSpriteSlots->sprites[index][g_FieldCurRenderContextIndex].x0 = x;
    g_FieldSpriteSlots->sprites[index][g_FieldCurRenderContextIndex].y0 = y;
    addPrim(g_FieldCurRenderContext->ot3, &g_FieldSpriteSlots->sprites[index][g_FieldCurRenderContextIndex]);
    addPrim(g_FieldCurRenderContext->ot3, &g_FieldSpriteSlots->drModes[index][g_FieldCurRenderContextIndex]);
}

extern SpriteList2* g_FieldScreenOverlaySprites;
extern PolyList2* g_FieldScreenOverlayPolys;

void FieldInitScreenOverlayPrims(void) {
    RECT rect;
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;
    SPRT* pSprite;
    SPRT* pSprite2;
    int i;

    g_FieldScreenOverlayPolys = HeapAlloc(0x2F8, 0x0);
    g_FieldScreenOverlaySprites = HeapAlloc(0x400, 0x0);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 4; i++) {
        SetDrawMode(&g_FieldScreenOverlaySprites->drModes[i][0], 0, 0, GetTPage(0, 0, 0x3C0, 0x140), &rect);
        SetDrawMode(&g_FieldScreenOverlaySprites->drModes[i][1], 0, 0, GetTPage(0, 0, 0x3C0, 0x140), &rect);
        pSprite = &g_FieldScreenOverlaySprites->sprites[i][0];
        pSprite2 = pSprite + 1;
        SetSprt(pSprite);
        setRGB0(pSprite, 0x80, 0x80, 0x80);
        setXY0(pSprite, 0xA0, 0x70);
        if (i == 0) {
            setUV0(pSprite, 0xE0, 0x70);
            setWH(pSprite, 0x10, 0x10);
        } else {
            setUV0(pSprite, 0xE0, 0x60);
            setWH(pSprite, 0x8, 0x8);
        }
        pSprite->clut = GetClut(0x100, 0xF7);
        *pSprite2 = *pSprite;
    }

    for (i = 0; i < 3; i++) {
        pPoly = &g_FieldScreenOverlayPolys->polys[i][0];
        pPoly2 = pPoly + 1;
        SetPolyFT4(pPoly);
        setXY4(pPoly, 
           i * 0x80, 0x0, 
           i * 0x80 + 0x80, 0x0, 
           i * 0x80, 0xDF, 
           i * 0x80 + 0x80, 0xDF
        );
        setRECT(&g_FieldScreenOverlayPolys->rects[i][0], 0x0, 0x0, 0xFF, 0xFF);
        setRECT(&g_FieldScreenOverlayPolys->rects[i][1], 0x0, 0x0, 0xFF, 0xFF);
        SetDrawMode(&g_FieldScreenOverlayPolys->drModes[i][0], 0, 0, GetTPage(1, 0, 0x300 + (i * 0x40), 0x100), &g_FieldScreenOverlayPolys->rects[i][0]);
        SetDrawMode(&g_FieldScreenOverlayPolys->drModes[i][1], 0, 0, GetTPage(1, 0, 0x300 + (i * 0x40), 0x100), &g_FieldScreenOverlayPolys->rects[i][1]);
        setRGB0(pPoly, 0x80, 0x80, 0x80);
        SetSemiTrans(pPoly, 0x1);
        setUV4(pPoly, 0x0, 0x0, 0x80, 0x0, 0x0, 0xDF, 0x80, 0xDF);
        pPoly->tpage = GetTPage(1, 0, 0x300 + (i * 0x40), 0x100);
        pPoly->clut = GetClut(0, 0xF6);
        *pPoly2 = *pPoly;
    }
}

// Check whether the inventory holds at least one of the given item
s32 FieldInventoryHasItem(int itemId) {
    int i;

    for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (g_pGameState->itemIDs[i] == itemId && g_pGameState->itemQuantities[i] != 0) {
            return 0;
        }
    }
    return -1;
}

extern SpriteList2* g_FieldScreenOverlaySprites;
extern PolyList2* g_FieldScreenOverlayPolys;

extern s32 g_FieldScreenOverlayOffsetX;
extern s32 g_FieldScreenOverlayOffsetY;
extern s32 g_PlayerActorIndex;
extern s32 g_FieldScreenOverlayScaleX;
extern s32 g_FieldScreenOverlayScaleY;

void FieldDrawScreenOverlayPrims(int color) {
    ActorData* pActor;
    int i;
    int y;
    int x;
    
    pActor = g_FieldActors[g_PlayerActorIndex].pActorData;
    x = (CONV_TO_GTE(pActor->position.vx) * g_FieldScreenOverlayScaleX) >> 0x10;
    y = -(CONV_TO_GTE(pActor->position.vz) * g_FieldScreenOverlayScaleY) >> 0x10;
    
    for (i = 0; i < 1; i++) {
        if (i == 0) {
            y -= 0xC;
            x -= 4;
        }
        
        g_FieldScreenOverlaySprites->sprites[i][g_FieldCurRenderContextIndex].x0 = x + g_FieldScreenOverlayOffsetX;
        g_FieldScreenOverlaySprites->sprites[i][g_FieldCurRenderContextIndex].y0 = y + g_FieldScreenOverlayOffsetY;
        
        setRGB0(&g_FieldScreenOverlaySprites->sprites[i][g_FieldCurRenderContextIndex & 1], color, color, color);
        addPrim(g_FieldCurRenderContext->ot3, &g_FieldScreenOverlaySprites->sprites[i][g_FieldCurRenderContextIndex]);
        addPrim(g_FieldCurRenderContext->ot3, &g_FieldScreenOverlaySprites->drModes[i][g_FieldCurRenderContextIndex]);
    }

    for (i = 0; i < 3; i++) {
        setRGB0(&g_FieldScreenOverlayPolys->polys[i][g_FieldCurRenderContextIndex & 1], color, color, color);
        addPrim(g_FieldCurRenderContext->ot3, &g_FieldScreenOverlayPolys->polys[i][g_FieldCurRenderContextIndex]);
        addPrim(g_FieldCurRenderContext->ot3, &g_FieldScreenOverlayPolys->drModes[i][g_FieldCurRenderContextIndex]);
    }
}

s32 FieldCheckSpriteSlotFlag(s32 index) {
    switch (index) {
        case 0:
            if (g_pGameState->fieldScriptMemory[0x73] & 0x8) {
                return 0;
            }
            break;
        case 1:
            if (g_pGameState->fieldScriptMemory[0x73] & 0x10) {
                return 0;
            }
            break;
        case 2:
            if (g_pGameState->fieldScriptMemory[0x73] & 0x20) {
                return 0;
            }
            break;
        case 3:
            if (g_pGameState->fieldScriptMemory[0x73] & 0x40) {
                return 0;
            }
            break;
        case 4:
            if (!(g_pGameState->fieldScriptMemory[0x73] & 0x80)) {
                return 0;
            }
            break;
    }
    return -1;
}

extern RECT g_FieldSpriteSlotRects[5];

// PARKED 77.35% this round (580 bytes, retail 668, 145 vs 164 insns; the
//    77.38 body of r426 wrote pRect->x on the ARRAY walker, which is wrong code -
//    the stores go to tim.prect, retail re-loads it from 0x1C before each of the
//    four, so 77.35 is the honest spelling of the same shape).
//    Shape now: four walking readers (pRect=&rects[0], pH=&rects[0].h,
//    pW=&rects[0].w, pY=&rects[0].y) incremented at the outer bottom, slot as the
//    index, the memcpy x as the INDEXED form g_FieldSpriteSlotRects[slot].x/4
//    (retail re-forms its base inside the inner loop), prect stores into
//    tim.prect. Prologue, the archive read, ReadTIM guard and the two HeapFrees
//    all match.
//    Residual: retail runs 19 insns longer because it keeps five addresses live
//    at once (s3=&h, s4=(slot*8)+base+6 hoisted for the inner bound, s6=&y,
//    s7=&y for the store, s2=&w) plus slot spilled at 0x38 and the &x walker
//    spilled at 0x48, and it re-loads tim.prect each store; this cc1 cse-s the
//    walkers into one base + displacements, which needs two callee-saved
//    registers fewer (frame 0x68 vs retail 0x80).
//    Tried this round: fully indexed body with stores to tim.prect (68.25, 148
//    insns - that is r426's 68.25 result again), walkers+tim.prect (77.35, best),
//    walkers writing pRect on the array (77.38, wrong code).
//    Already ruled out by r426 and not retried: r426's variants (75.69 / 77.16 /
//    73.18), try_flags --sweep over 2.6.0 / 2.7.2 / 2.7.2-cdk x -O1-3 x -G4/G8 x
//    -mcpu 3000/r8000 (Default is the max), and there is no twin in the repo.
//    Next idea for a later round: the 19-insn gap is register pressure, so force
//    retail's extra live values - e.g. keep BOTH an s32 byte offset (slot*8,
//    incremented by 8) and slot, and give the inner-loop x its own hoisted
//    pointer read.
/*
void func_800AB808(void) {
    TIM_IMAGE tim;
    u_long* pFile;
    u_long* pImage;
    u_long* pDst;
    RECT* pRect;
    s16* pH;
    s16* pW;
    s16* pY;
    s32 slot;
    s32 j;

    pFile = HeapAlloc(ArchiveDecodeAlignedSize(0x802), 0);
    ArchiveReadFileToBuffer(0x802, pFile, 0, 0x80);
    ArchiveCdDataSync(0);
    pImage = HeapAlloc(0xF20, 0);
    OpenTIM(pFile);
    if (ReadTIM(&tim) != 0) {
        pRect = &g_FieldSpriteSlotRects[0];
        pH = &g_FieldSpriteSlotRects[0].h;
        pW = &g_FieldSpriteSlotRects[0].w;
        pY = &g_FieldSpriteSlotRects[0].y;
        slot = 0;
        do {
            if (FieldCheckSpriteSlotFlag(slot) == -1 && tim.paddr != NULL) {
                pDst = pImage;
                for (j = 0; j < *pH; j++) {
                    memcpy(pDst, (u_long*)tim.paddr + (j + *pY) * 0x50 + g_FieldSpriteSlotRects[slot].x / 4, *pW);
                    pDst += *pW / 4;
                }
                tim.prect->x = pRect->x / 2 + 0x300;
                tim.prect->y = *pY + 0x100;
                tim.prect->w = *pW / 2;
                tim.prect->h = *pH;
                LoadImage(tim.prect, pImage);
                DrawSync(0);
            }
            pRect++;
            pH++;
            pW++;
            pY++;
            slot++;
        } while (slot < 5);
    }

    HeapFree(pFile);
    HeapFree(pImage);
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AB808);


typedef struct {
    s32 mapNum;
    s32 unk4;
    s32 unk8;
    s32 archiveIndex;
    s32 itemId;
    s32 unk14;
    s32 unk18;
    s32 unk1C;
} FieldMapSpriteEntry; // Size: 0x20

extern FieldMapSpriteEntry g_FieldMapSpriteTable[];

extern s32 g_GameSceneMapNum;
extern u16 g_FieldButtonsPressedOnce;
extern void func_800AB808();

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800ABA98);
/* PARKED 87.23% (612 bytes, retail 668, 153 vs 160 insns). Everything but one
   construct matches: the table walk, the item check, StoreImage, the archive
   read, FieldLoadTIMWithClut, the func_800AB808 call, all three render loops and
   the LoadImage restore. The poll loop MUST be a do-while (retail enters it with
   no top test and loops back with beqz; while costs 2.37 points). FieldDraw-
   ScreenOverlayPrims must take int, not s8 (under s8 this body scores 84.86).
   Residual, one construct: retail keeps the table BASE in s2 and the scaled
   index in s1 live across the calls and re-forms addu reg,s2,s1 at each of the
   five g_FieldMapSpriteTable[index] field reads (and at the 0x1C read after
   HeapFree), i.e. five separate sums, one callee-saved register more (frame
   0x40 vs 0x38, pImage in s3 vs s2). This cc1 cse-s base + index*0x20 into one
   callee-saved register, so the body is 7 insns short and cannot link.
   Tried: pTable[index] with pTable a local pointer (best), the bare symbol form
   g_FieldMapSpriteTable[index] everywhere (75.74 - symbol+offset macros through
   $at), while instead of do-while (-2.37). Presets are not the question: this
   body scores 87.23 under 2.6.0, 87.23 under -G0, 87.08 under 2.7.2-cdk, and a
   full gcc x -O x -G x -mcpu sweep on the s8 body peaked at 82.61. Note retail
   DID common the shift (one sll) but not the sum, so only the sum is missing.
*/
/*
void func_800ABA98(void) {
    RECT rect;
    FieldMapSpriteEntry* pTable;
    void* pImage;
    void* pFile;
    s32 mapNum;
    s32 archiveIndex;
    s32 index;

    mapNum = g_GameSceneMapNum & 0x3FFF;
    pTable = g_FieldMapSpriteTable;
    for (index = 0;; index++) {
        if (pTable[index].mapNum == 0xFFFF) {
            return;
        }
        if (pTable[index].mapNum == mapNum) {
            break;
        }
    }

    if (FieldInventoryHasItem(pTable[index].itemId) != -1) {
        rect.x = 0x300;
        rect.y = 0x100;
        rect.w = 0xA0;
        rect.h = 0x100;
        g_FieldScreenOverlayScaleX = pTable[index].unk4;
        g_FieldScreenOverlayScaleY = pTable[index].unk8;
        g_FieldScreenOverlayOffsetX = pTable[index].unk14;
        g_FieldScreenOverlayOffsetY = pTable[index].unk18;

        pImage = HeapAlloc(0x14000, 0);
        StoreImage(&rect, pImage);
        DrawSync(0);

        ArchiveCdDataSync(0);
        ArchiveSetIndex(4, 0);
        archiveIndex = pTable[index].archiveIndex + 0x7FB;
        pFile = HeapAlloc(ArchiveDecodeAlignedSize(archiveIndex), 0);
        ArchiveReadFileToBuffer(archiveIndex, pFile, 0, 0x80);
        ArchiveCdDataSync(0);
        FieldLoadTIMWithClut((u_long*)pFile, 0x300, 0x100, 0, 0xF6, 0, 0);
        HeapFree(pFile);

        if (pTable[index].unk1C == 1) {
            func_800AB808();
        }
        FieldInitScreenOverlayPrims();

        for (index = 0; index < 0x10; index++) {
            FieldClearAndSwapOTag();
            FieldDrawScreenOverlayPrims(index << 3);
            FieldDisplay();
        }

        do {
            FieldClearAndSwapOTag();
            FieldDrawScreenOverlayPrims(0x80);
            FieldDisplay();
            FieldPollControllers();
        } while (!(g_FieldButtonsPressedOnce & 0x100));

        for (index = 0x10; index > 0; index--) {
            FieldClearAndSwapOTag();
            FieldDrawScreenOverlayPrims(index << 3);
            FieldDisplay();
        }

        DrawSync(0);
        HeapFree(g_FieldScreenOverlayPolys);
        HeapFree(g_FieldScreenOverlaySprites);
        LoadImage(&rect, pImage);
        DrawSync(0);
        HeapFree(pImage);
    }
}
*/

extern SpriteList3 g_FieldScreenStripSprites;

void FieldInitScreenStripSprites(void) {
    int i;
    RECT rect;
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 5; i++) {
        SetDrawMode(&g_FieldScreenStripSprites.drModes[i][0], 0, 0, GetTPage(1, 0, 0x280 + (i * 0x40), 0), &rect);
        SetDrawMode(&g_FieldScreenStripSprites.drModes[i][1], 0, 0, GetTPage(1, 0, 0x280 + (i * 0x40), 0), &rect);
        SetSprt(&g_FieldScreenStripSprites.sprites[i][0]);
        setRGB0(&g_FieldScreenStripSprites.sprites[i][0], 0x80, 0x80, 0x80);
        setXY0(&g_FieldScreenStripSprites.sprites[i][0], i * 0x80, 0x0);
        setUV0(&g_FieldScreenStripSprites.sprites[i][0], 0x0, 0x0);
        setWH(&g_FieldScreenStripSprites.sprites[i][0], 0x80, 0xE0);
        SetSemiTrans(&g_FieldScreenStripSprites.sprites[i][0], 0);
        (&g_FieldScreenStripSprites.sprites[i][0])->clut = GetClut(0, 0xE8);
        g_FieldScreenStripSprites.sprites[i][1] = g_FieldScreenStripSprites.sprites[i][0];
    }
}

extern s16 g_FieldScreenStripSpritesEnabled;

void FieldDrawScreenStripSprites(void) {
    int i;

    if (g_FieldScreenStripSpritesEnabled) {
        for (i = 0; i < 5; i++) {
            addPrim(g_FieldCurRenderContext->ot3, &g_FieldScreenStripSprites.sprites[i][g_FieldCurRenderContextIndex]);
            addPrim(g_FieldCurRenderContext->ot3, &g_FieldScreenStripSprites.drModes[i][g_FieldCurRenderContextIndex]);
        }
    }
}

s32 FieldDecodeFontGlyph(u_char* pData, s32* pIsGlyph) {
    u_short code;

    code = (pData[0] << 8) | pData[1];
    if ((u_short)(code - 0x8540) < 0x340) {
        *pIsGlyph = 1;
        return code - 0x8540;
    }
    *pIsGlyph = 0;
    return Krom2RawAdd(code);
}

// Expand the 15 16-bit rows of a 16x16 glyph bitmap into 18-byte rows of
// 0x00/0xFF mask bytes. A -1 source address means "blank glyph".
void FieldExpandGlyphBitmap(u_char* pDst, u_short* pSrc) {
    int i = 0;
    int bit;

    if (pSrc == (u_short*)-1) {
        u_char c = 0xFF;

        for (i = 0x11F; i >= 0; i--) {
            *pDst++ = c;
        }
        return;
    }

    for (i = 0; i < 0xF; i++) {
        for (bit = 7; bit >= 0; bit--) {
            *pDst++ = (u_char)((*pSrc & (1 << bit)) ? 0xFF : 0);
        }
        for (bit = 0xF; bit >= 8; bit--) {
            *pDst++ = (u_char)((*pSrc & (1 << bit)) ? 0xFF : 0);
        }
        *pDst++ = 0;
        *pDst++ = 0;
        pSrc++;
    }
}

extern s32 g_FieldTextRenderBytesRemaining;

void* FieldDrawTextRow(void* pData, s32 x, s32 frameIndex) {
    u_char glyph[0x120];
    RECT rect;
    RECT srcRect;
    u_char text[0x40];
    s32 isGlyph;
    s32 glyphCode;
    int i;
    int index;
    int line;

    rect.w = 9;
    rect.h = 0x10;
    rect.y = frameIndex << 4;

    for (i = 0; i < 0x40; i++) {
        text[i] = ((u_char*)pData)[i];
    }
    for (i = 0x11F; i >= 0; i--) {
        glyph[i] = 0;
    }

    index = 0;
    if (g_FieldTextRenderBytesRemaining <= 0) {
        line = 0;
    } else {
        line = 0;
        while (line < 0x1C) {
            if (text[index] == 0xD) {
                index++;
                break;
            }
            glyphCode = FieldDecodeFontGlyph(&text[index], &isGlyph);
            index += 2;
            if (isGlyph == 1) {
                srcRect.w = 9;
                srcRect.h = 0x10;
                srcRect.x = glyphCode % 7 * 9 + 0x380;
                srcRect.y = glyphCode / 7 * 0x10 + 0x100;
                MoveImage(&srcRect, x + line * 9, frameIndex << 4);
            } else {
                FieldExpandGlyphBitmap(glyph, glyphCode);
                rect.x = x + line * 9;
                LoadImage(&rect, glyph);
            }
            DrawSync(0);
            line++;
        }
    }

    for (i = 0x11F; i >= 0; i--) {
        glyph[i] = 0;
    }
    for (; line < 0x1C; line++) {
        rect.x = x + line * 9;
        LoadImage(&rect, glyph);
        DrawSync(0);
    }

    g_FieldTextRenderBytesRemaining -= index;
    return (u_char*)pData + index;
}

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveCdDataSync(int mode);

extern s32 g_FieldTextRenderEnabled;
extern void* g_FieldTextRenderData;
extern void* g_FieldTextRenderCursor;
extern s32 g_FieldTextRenderRow;
extern s32 g_FieldTextRenderFrameCounter;
extern s32 g_FieldFontTimBuffer;

// Load the two archive files this effect needs
void FieldLoadTextRenderFiles(void) {
    ArchiveSetIndex(4, 0);
    g_FieldTextRenderBytesRemaining = ArchiveDecodeSize(0xAB);
    g_FieldTextRenderData = HeapAlloc(ArchiveDecodeAlignedSize(0xAB), 1);
    ArchiveReadFileToBuffer(0xAB, g_FieldTextRenderData, 0, 0x80);
    ArchiveCdDataSync(0);
    g_FieldFontTimBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0xAC), 1);
    ArchiveReadFileToBuffer(0xAC, g_FieldFontTimBuffer, 0, 0x80);
    ArchiveCdDataSync(0);
}

// One 16-pixel text row: a draw mode and a sprite per column, duplicated for
// the two render contexts
typedef struct {
    DR_MODE drModes[2][4]; // 0x0
    SPRT sprites[2][4];    // 0x60
} FieldTextRowPrims; // Size: 0x100

extern POLY_GT4 g_FieldTextGradientTopQuads[2];
extern POLY_GT4 g_FieldTextGradientBottomQuads[2];
extern s32 g_FieldTextRenderPrimBlocks;

// Build the two gradient quads the text rows are drawn through, then allocate
// and fill the 16 rows of primitives
/* 97.89% - retail's exact 1520 bytes / 380 instructions, and the body matches
   instruction for instruction but for ONE displaced instruction (`li $v1,0x280`,
   see the end of this note). Every other row diff_func prints is only the
   relocation spelling: retail names each store's absolute address (splat's
   D_800AF7xx labels: `lui $at,%hi(D_800AF78E)` + `sb $v0,%lo(...)($at)`) while
   the typed array spells the same byte as base+offset, which links to the
   identical address.
   The load-bearing finding: the 56 poly field stores are in a fixed ORDER and
   sched2 preserves statement order here (it does NOT regroup them), so the
   sequence below is transcribed from the .s one store at a time. The order is
   grouped by constant value, not by vertex: the 12 x 0xFF stores (top v0/v1 and
   bottom v2/v3 - the white corners), then the two 0x18 y's, then the 12 zero
   stores (top v2/v3, bottom v0/v1 - the black corners), then XY, then UV.
   Writing one quad at a time instead scores 89.64%.
   Two other measured points: a pointer local for the quads collapses all 56
   stores into `sb off(reg)` and halves the code, so every store must name the
   global; and in the SPRT setup SetSemiTrans plus clut=GetClut must be written
   BETWEEN setRGB0 and setUV0 (that is what puts the third RGB byte in
   SetSemiTrans' jal delay slot). The inner 4x2 loop must index the block through
   g_FieldTextRenderPrimBlocks, not through the pRow local, or gcc needs one
   callee-saved register fewer and the whole copy run takes the wrong base.
   The single residual: retail emits li v1,0x280 immediately after li v0,0xff at
   the head of the store group (the x=0x280 constant stays in $v1 across four
   stores far below in the XY group), while this body emits it 25 instructions
   later beside li v0,0x18. Same instruction, same register, a different slot.
   Measured dead: a `s32 right = 0x280;` local (declaration initialiser, gcc
   folds it back to the same 97.89), moving the four x=0x280 stores up next to
   the 0xFF group (93.73 - the stores move too, sched2 does not re-sort them),
   -O3 (97.89), gcc 2.6.0 (97.82), gcc 2.7.2-cdk (70.15).
   Round 385 addenda - the slot is the SCHEDULER's, not cse's: a flat replica of
   this store group compiled with the unit's own cc1 (tools/scripts/cc1_probe.sh)
   reproduces this body's shape exactly (`li $3,0x280` immediately after
   `li $2,0x18`, 18 insns before its first `sh`) and under -O1 the li falls back
   to its use, so the stores keep their order (conservative memory deps) while
   the pure lis are hoisted greedily into one cluster headed by the 0x18 def.
   Retail alone promoted 0x280 into the first cluster beside 0xFF. Everything
   below measured at 1520 bytes unless noted: a do-while-0 around the WHOLE
   56-store group 94.64 (1512, worse - the group switches to
   register+displacement addressing), a do-while-0 around only the 12 zero
   stores 75.68 (1388), `(s16)0x280` and `(u16)0x280` on the four x stores 97.89
   (byte-identical to the plain literal), a `s32 right; right = 0x280;`
   STATEMENT assignment at the group head (the declaration initialiser was
   measured last round) folds back to the same 97.89, a store-to-load-forwarded
   reuse (`x3 = x1`) leaves the slot unchanged in the replica, and permute.sh
   4 min / 1519 iterations printed PERMUTER_NONE base=850 best=850.
   Round 397: the preset is ruled out for good - try_flags.sh --sweep on the live
   body scores 97.89 under 2.7.2 -O2 AND -O3 at -G4/-G8 with -mcpu=3000 and
   =r8000, 70.15 under 2.7.2-cdk, 59.76 under -O1 (where the li falls back to its
   use). Retail's `ori $v1,zero,0x280` sits 60 instructions before its first use
   (0x800AC408, first `sh $v1` at 0x800AC4F8); this body's sits 18 before it.
   Both builds hoist the GetTPage argument constants and the 0xFF def to the head
   of the store block and leave the 0x18 / 0xC8 defs at their use, so the
   hoisting is register- and priority-driven and the 0x280 def lost its chance at
   the head in this build only. Conclusion: retail's def was at the head of the
   store block in RTL, i.e. its source had a statement there whose constant cse
   folds away in this build (every local spelling of 0x280 measures 97.89). Not
   source-reachable: park, do not permute. Round 420 un-parked and re-measured
   this body on today's externs: it compiles and scores the same 97.89 / 1520
   bytes / 380 insns, with the displaced `li $v1,0x280` the ONLY non-relocation
   row - do not brief it again. Round 434 re-un-parked and re-measured it a
   third time: still 97.89 / 1520 / 380, and the `sh $v1,+20` first use is 12
   insns after our def (the note's 18 counts from the `li $2,0x18`). Lever
   c-displaced-li-inside is this function and says park, do not permute. Also
   measured dead this round: the round-433 per-group `do { } while (0)` store
   barrier (one empty barrier after each constant group) collapses the group to
   71.92 / 1332 bytes / 333 insns - cc1 merges the stores across the barriers -
   so the barrier family is closed for this body, not just the whole-group form.
/*
void func_800AC3AC(void) {
    FieldTextRowPrims* pRow;
    SPRT* pSprite;
    s32 i;
    s32 j;

    SetPolyGT4(&g_FieldTextGradientTopQuads[0]);
    SetPolyGT4(&g_FieldTextGradientBottomQuads[0]);

    // Retail's store order (sched2 groups by the constant register).
    g_FieldTextGradientTopQuads[0].b0 = 0xFF;
    g_FieldTextGradientTopQuads[0].g0 = 0xFF;
    g_FieldTextGradientTopQuads[0].r0 = 0xFF;
    g_FieldTextGradientTopQuads[0].b1 = 0xFF;
    g_FieldTextGradientTopQuads[0].g1 = 0xFF;
    g_FieldTextGradientTopQuads[0].r1 = 0xFF;
    g_FieldTextGradientBottomQuads[0].b2 = 0xFF;
    g_FieldTextGradientBottomQuads[0].g2 = 0xFF;
    g_FieldTextGradientBottomQuads[0].r2 = 0xFF;
    g_FieldTextGradientBottomQuads[0].b3 = 0xFF;
    g_FieldTextGradientBottomQuads[0].g3 = 0xFF;
    g_FieldTextGradientBottomQuads[0].r3 = 0xFF;
    g_FieldTextGradientTopQuads[0].y2 = 0x18;
    g_FieldTextGradientTopQuads[0].y3 = 0x18;
    g_FieldTextGradientTopQuads[0].b2 = 0;
    g_FieldTextGradientTopQuads[0].g2 = 0;
    g_FieldTextGradientTopQuads[0].r2 = 0;
    g_FieldTextGradientTopQuads[0].b3 = 0;
    g_FieldTextGradientTopQuads[0].g3 = 0;
    g_FieldTextGradientTopQuads[0].r3 = 0;
    g_FieldTextGradientBottomQuads[0].b0 = 0;
    g_FieldTextGradientBottomQuads[0].g0 = 0;
    g_FieldTextGradientBottomQuads[0].r0 = 0;
    g_FieldTextGradientBottomQuads[0].b1 = 0;
    g_FieldTextGradientBottomQuads[0].g1 = 0;
    g_FieldTextGradientBottomQuads[0].r1 = 0;
    g_FieldTextGradientTopQuads[0].x0 = 0;
    g_FieldTextGradientTopQuads[0].y0 = 0;
    g_FieldTextGradientTopQuads[0].x1 = 0x280;
    g_FieldTextGradientTopQuads[0].y1 = 0;
    g_FieldTextGradientTopQuads[0].x2 = 0;
    g_FieldTextGradientTopQuads[0].x3 = 0x280;
    g_FieldTextGradientBottomQuads[0].x0 = 0;
    g_FieldTextGradientBottomQuads[0].y0 = 0xC8;
    g_FieldTextGradientBottomQuads[0].y1 = 0xC8;
    g_FieldTextGradientBottomQuads[0].y2 = 0xE0;
    g_FieldTextGradientBottomQuads[0].y3 = 0xE0;
    g_FieldTextGradientBottomQuads[0].x1 = 0x280;
    g_FieldTextGradientBottomQuads[0].x2 = 0;
    g_FieldTextGradientBottomQuads[0].x3 = 0x280;
    g_FieldTextGradientTopQuads[0].u0 = 0;
    g_FieldTextGradientTopQuads[0].v0 = 0;
    g_FieldTextGradientTopQuads[0].u1 = 2;
    g_FieldTextGradientTopQuads[0].v1 = 0;
    g_FieldTextGradientTopQuads[0].u2 = 0;
    g_FieldTextGradientTopQuads[0].v2 = 2;
    g_FieldTextGradientTopQuads[0].u3 = 2;
    g_FieldTextGradientTopQuads[0].v3 = 2;
    g_FieldTextGradientBottomQuads[0].u0 = 0;
    g_FieldTextGradientBottomQuads[0].v0 = 0;
    g_FieldTextGradientBottomQuads[0].u1 = 2;
    g_FieldTextGradientBottomQuads[0].v1 = 0;
    g_FieldTextGradientBottomQuads[0].u2 = 0;
    g_FieldTextGradientBottomQuads[0].v2 = 2;
    g_FieldTextGradientBottomQuads[0].u3 = 2;
    g_FieldTextGradientBottomQuads[0].v3 = 2;

    g_FieldTextGradientTopQuads[0].tpage = GetTPage(1, 2, 0x3C0, 0x100);
    g_FieldTextGradientBottomQuads[0].tpage = GetTPage(1, 2, 0x3C0, 0x100);
    g_FieldTextGradientTopQuads[0].clut = GetClut(0, 0x1FF);
    g_FieldTextGradientBottomQuads[0].clut = GetClut(0, 0x1FF);
    SetSemiTrans(&g_FieldTextGradientTopQuads[0], 1);
    SetSemiTrans(&g_FieldTextGradientBottomQuads[0], 1);

    g_FieldTextGradientTopQuads[1] = g_FieldTextGradientTopQuads[0];
    g_FieldTextGradientBottomQuads[1] = g_FieldTextGradientBottomQuads[0];

    g_FieldTextRenderPrimBlocks = (s32)HeapAlloc(0x1000, 1);
    for (i = 0; i < 0x10; i++) {
        pRow = (FieldTextRowPrims*)(g_FieldTextRenderPrimBlocks + i * 0x100);
        pSprite = &pRow->sprites[0][0];
        SetSprt(pSprite);
        setRGB0(pSprite, 0x80, 0x80, 0x80);
        SetSemiTrans(pSprite, 0);
        pSprite->clut = GetClut(0, 0x1FF);
        setUV0(pSprite, 0, i << 4);
        setWH(pSprite, 0x80, 0x10);
        setXY0(pSprite, 0x40, i << 4);

        pRow->sprites[1][0] = pRow->sprites[0][0];
        pRow->sprites[0][1] = pRow->sprites[0][0];
        pRow->sprites[1][1] = pRow->sprites[0][0];
        pRow->sprites[0][2] = pRow->sprites[0][0];
        pRow->sprites[1][2] = pRow->sprites[0][0];
        pRow->sprites[0][3] = pRow->sprites[0][0];
        pRow->sprites[1][3] = pRow->sprites[0][0];

        for (j = 0; j < 4; j++) {
            ((FieldTextRowPrims *)(g_FieldTextRenderPrimBlocks + i * 0x100))->sprites[0][j].x0 = 0x40 + j * 0x80;
            ((FieldTextRowPrims *)(g_FieldTextRenderPrimBlocks + i * 0x100))->sprites[1][j].x0 = 0x40 + j * 0x80;
            SetDrawMode(&((FieldTextRowPrims *)(g_FieldTextRenderPrimBlocks + i * 0x100))->drModes[0][j], 0, 0, GetTPage(1, 0, 0x300 + j * 0x40, 0), 0);
            SetDrawMode(&((FieldTextRowPrims *)(g_FieldTextRenderPrimBlocks + i * 0x100))->drModes[1][j], 0, 0, GetTPage(1, 0, 0x300 + j * 0x40, 0), 0);
        }
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC3AC);

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC99C);
/* PARKED 81.27% (520 bytes, retail 500 = 125 insns; this body is 130). The
   shape is right: addPrim on the two 0x34-byte gradient polys for this render
   context (retail forms the second as the SAME symbol minus 0x68, so
   g_FieldTextGradientBottomQuads[ctx - 2] is the spelling that reproduces it),
   then 16 blocks of DR_MODE[2][4] then SPRT[2][4] (0x100 bytes each, the same
   array func_800AC3AC allocates - reuse FieldTextRowPrims, do not declare a
   second 0x100-byte struct), the four sprites of this context taking y0 from
   the other context's first sprite, and two addPrim per sprite.
   THE WHOLE RESIDUAL IS ONE SPILLED REGISTER: retail keeps 11 values in t
   registers and has NO frame at all (jr ra with no sp adjust); this cc1 puts
   the hoisted ((ctx + 1) & 1) * 0x50 offset of the y0 source in s0, which costs
   the 8-byte frame (addiu sp / sw s0 / lw s0 / addiu sp) plus one `move a2,t3`
   for the blocks + ctx*0x50 base, i.e. 5 insns over retail's 125. Everything
   else - the four y0 stores, both addPrim chains, the 0x100/0x14/0xC strides,
   the two mask constants duplicated in the prologue, the ot3 reload before
   every setaddr - matches.
   Tried this round: four y0 stores hoisted above the inner loop (retail's own
   shape) = 80.34% / 512 bytes / 128 insns: it removes one spill (s0 only) but
   cc1 then hoists t5 + ctx*0x50 as well and adds a move; `ctx ^ 1` for the
   other-context index = 74.50% (xori instead of addiu+andi); declaring `short
   y` with the other locals instead of in the loop = byte-identical to 80.34.
   Earlier rounds (all dead): an `other` local vs the inline (ctx + 1) & 1, the
   poly arguments as rows of g_FieldTextGradientTopQuads[2][2], the y0 stores as
   four statements (80.70 / 512 / 8 short with gcc merging the two sprite
   walkers), and the load must stay lh: `short y = x & 0xFF` gives lhu and
   u_char y gives lbu, so retail's lh needs a sign-dependent use of the loaded
   y0 that no shape here has found (1 instruction, harmless for the link).
*/
/*
void func_800AC99C(void) {
    FieldTextRowPrims* pBlock;
    int ctx;
    int i;
    int j;

    ctx = g_FieldCurRenderContextIndex;
    addPrim(g_FieldCurRenderContext->ot3, &g_FieldTextGradientBottomQuads[ctx]);
    addPrim(g_FieldCurRenderContext->ot3, &g_FieldTextGradientBottomQuads[ctx - 2]);

    pBlock = (FieldTextRowPrims*)g_FieldTextRenderPrimBlocks;
    for (i = 0; i < 0x1000; i += 0x100) {
        short y = (pBlock->sprites[(ctx + 1) & 1][0].y0 - 1) & 0xFF;

        for (j = 0; j < 4; j++) {
            pBlock->sprites[ctx][j].y0 = y;
            addPrim(g_FieldCurRenderContext->ot3, &pBlock->sprites[ctx][j]);
            addPrim(g_FieldCurRenderContext->ot3, &pBlock->drModes[ctx][j]);
        }
        pBlock++;
    }
}
*/


void FieldLoadFontTIMAndClearRect(void) {
    RECT rect;
    s32* pBuffer;
    s32 empty;
    int i;

    FieldLoadTIMWithClut((u_long*)g_FieldFontTimBuffer, 0x380, 0x100, 0, 0x1FF, 0, 0);
    DrawSync(0);
    HeapFree(g_FieldFontTimBuffer);

    pBuffer = HeapAlloc(0x200, 1);
    empty = -1;
    for (i = 0x7F; i >= 0; i--) {
        pBuffer[i] = empty;
    }

    rect.x = 0x3C0;
    rect.y = 0x100;
    rect.w = 0x40;
    rect.h = 4;
    LoadImage(&rect, pBuffer);
    DrawSync(0);
    HeapFree(pBuffer);
}

extern void FieldLoadTextRenderFiles();
extern void func_800AC3AC();

void FieldInitTextRender(void) {
    RECT rect; // unused, but the original frame has this 8-byte slot

    if (g_FieldTextRenderEnabled) {
        FieldLoadTextRenderFiles();
        func_800AC3AC();
        g_FieldTextRenderFrameCounter = 0;
        g_FieldTextRenderRow = 0xF;
        g_FieldTextRenderCursor = g_FieldTextRenderData;
    }
}

extern s32 g_FieldTextRenderPrimBlocks;

void FieldFreeTextRender(void) {
    if (g_FieldTextRenderEnabled) {
        HeapFree(g_FieldTextRenderData);
        HeapFree(g_FieldTextRenderPrimBlocks);
    }
}

void* FieldDrawTextRow(void* pData, s32 offset, s32 frame);

void FieldUpdateTextRender(void) {
    if (g_FieldTextRenderEnabled) {
        if ((g_FieldTextRenderFrameCounter & 0xF) == 0) {
            g_FieldTextRenderCursor = FieldDrawTextRow(g_FieldTextRenderCursor, 0x300, g_FieldTextRenderRow & 0xF);
            g_FieldTextRenderRow++;
        }
        g_FieldTextRenderFrameCounter++;
    }
}

short FieldScriptVMGetInstructionArgumentS16(int offset) {
    u_char* pData;

    pData = g_FieldScriptVMCurScriptData + (g_FieldScriptVMCurActor->scriptInstructionPointer + offset);
    return (pData[0] + (pData[1] << 8));
}

int FieldScriptVMGetInstructionArgument(int argumentIndex) {
    u_char* pData;

    pData = g_FieldScriptVMCurScriptData + (g_FieldScriptVMCurActor->scriptInstructionPointer + argumentIndex);
    return *pData | *(pData + 1) <<  8;
}

int FieldScriptVMGetArgument(int index) {
    int nArgument;

    nArgument = FieldScriptVMGetInstructionArgument(index);
    if (!(nArgument & 0x8000)) 
        return FieldScriptVMGetVariableValue(nArgument & 0xFFFF);
    return nArgument & 0x7FFF;
}
