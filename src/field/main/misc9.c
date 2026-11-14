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

extern MATRIX D_800B00E8;
extern s32 D_800C3A5C;
extern s32 D_800C3A60;

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
    D_800B00E8.t[0] = vec.vx;
    D_800B00E8.t[1] = vec.vy;
    D_800B00E8.t[2] = vec.vz;
    SetRotMatrix(&D_800B00E8);
    SetTransMatrix(&D_800B00E8);

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

    if (y1 < D_800C3A60 + 0xE0 && -D_800C3A60 < y2 && x1 < D_800C3A5C + 0x140 && -D_800C3A5C < x2) {
        return 0;
    }
    return -1;
}

extern SpriteList* D_800AFC68;

void FieldFreeSpriteList(void) {
    HeapFree(D_800AFC68);
    DrawSync(0);
}

void FieldInitSpriteList(void) {
    RECT rect;
    SPRT* pSprite;
    SPRT* pSprite2;
    int i;

    D_800AFC68 = HeapAlloc(0x840, 0x0);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 0x21; i++) {
        SetDrawMode(&D_800AFC68->drModes[i][0], 0, 0, GetTPage(0, 0, 0x3C0, 0x100) & 0xFFFF, &rect);
        SetDrawMode(&D_800AFC68->drModes[i][1], 0, 0, GetTPage(0, 0, 0x3C0, 0x140) & 0xFFFF, &rect);
        pSprite = &D_800AFC68->sprites[i][0];
        pSprite2 = &D_800AFC68->sprites[i][1];
        
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
    setRGB0(&D_800AFC68->sprites[index][0], red, green, blue);
    setRGB0(&D_800AFC68->sprites[index][1], red, green, blue);
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

    D_800AFC68->sprites[index][g_FieldCurRenderContextIndex].x0 = x;
    D_800AFC68->sprites[index][g_FieldCurRenderContextIndex].y0 = y;
    addPrim(g_FieldCurRenderContext->ot3, &D_800AFC68->sprites[index][g_FieldCurRenderContextIndex]);
    addPrim(g_FieldCurRenderContext->ot3, &D_800AFC68->drModes[index][g_FieldCurRenderContextIndex]);
}

extern SpriteList2* D_800B1DF0;
extern PolyList2* D_800C3A3C;

void FieldInitScreenOverlayPrims(void) {
    RECT rect;
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;
    SPRT* pSprite;
    SPRT* pSprite2;
    int i;

    D_800C3A3C = HeapAlloc(0x2F8, 0x0);
    D_800B1DF0 = HeapAlloc(0x400, 0x0);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 4; i++) {
        SetDrawMode(&D_800B1DF0->drModes[i][0], 0, 0, GetTPage(0, 0, 0x3C0, 0x140), &rect);
        SetDrawMode(&D_800B1DF0->drModes[i][1], 0, 0, GetTPage(0, 0, 0x3C0, 0x140), &rect);
        pSprite = &D_800B1DF0->sprites[i][0];
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
        pPoly = &D_800C3A3C->polys[i][0];
        pPoly2 = pPoly + 1;
        SetPolyFT4(pPoly);
        setXY4(pPoly, 
           i * 0x80, 0x0, 
           i * 0x80 + 0x80, 0x0, 
           i * 0x80, 0xDF, 
           i * 0x80 + 0x80, 0xDF
        );
        setRECT(&D_800C3A3C->rects[i][0], 0x0, 0x0, 0xFF, 0xFF);
        setRECT(&D_800C3A3C->rects[i][1], 0x0, 0x0, 0xFF, 0xFF);
        SetDrawMode(&D_800C3A3C->drModes[i][0], 0, 0, GetTPage(1, 0, 0x300 + (i * 0x40), 0x100), &D_800C3A3C->rects[i][0]);
        SetDrawMode(&D_800C3A3C->drModes[i][1], 0, 0, GetTPage(1, 0, 0x300 + (i * 0x40), 0x100), &D_800C3A3C->rects[i][1]);
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

extern SpriteList2* D_800B1DF0;
extern PolyList2* D_800C3A3C;

extern s32 D_800AFE78;
extern s32 D_800AFE7C;
extern s32 g_PlayerActorIndex;
extern s32 D_800C3914;
extern s32 D_800C3A18;

void FieldDrawScreenOverlayPrims(s8 color) {
    ActorData* pActor;
    int i;
    int y;
    int x;
    
    pActor = g_FieldActors[g_PlayerActorIndex].pActorData;
    x = (CONV_TO_GTE(pActor->position.vx) * D_800C3914) >> 0x10;
    y = -(CONV_TO_GTE(pActor->position.vz) * D_800C3A18) >> 0x10;
    
    for (i = 0; i < 1; i++) {
        if (i == 0) {
            y -= 0xC;
            x -= 4;
        }
        
        D_800B1DF0->sprites[i][g_FieldCurRenderContextIndex].x0 = x + D_800AFE78;
        D_800B1DF0->sprites[i][g_FieldCurRenderContextIndex].y0 = y + D_800AFE7C;
        
        setRGB0(&D_800B1DF0->sprites[i][g_FieldCurRenderContextIndex & 1], color, color, color);
        addPrim(g_FieldCurRenderContext->ot3, &D_800B1DF0->sprites[i][g_FieldCurRenderContextIndex]);
        addPrim(g_FieldCurRenderContext->ot3, &D_800B1DF0->drModes[i][g_FieldCurRenderContextIndex]);
    }

    for (i = 0; i < 3; i++) {
        setRGB0(&D_800C3A3C->polys[i][g_FieldCurRenderContextIndex & 1], color, color, color);
        addPrim(g_FieldCurRenderContext->ot3, &D_800C3A3C->polys[i][g_FieldCurRenderContextIndex]);
        addPrim(g_FieldCurRenderContext->ot3, &D_800C3A3C->drModes[i][g_FieldCurRenderContextIndex]);
    }
}

s32 FieldCheckSpriteSlotFlag(s32 index) {
    switch (index) {
        case 0:
            if (g_pGameState->unk1930[0x73] & 0x8) {
                return 0;
            }
            break;
        case 1:
            if (g_pGameState->unk1930[0x73] & 0x10) {
                return 0;
            }
            break;
        case 2:
            if (g_pGameState->unk1930[0x73] & 0x20) {
                return 0;
            }
            break;
        case 3:
            if (g_pGameState->unk1930[0x73] & 0x40) {
                return 0;
            }
            break;
        case 4:
            if (!(g_pGameState->unk1930[0x73] & 0x80)) {
                return 0;
            }
            break;
    }
    return -1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AB808);
/* 68.25%: shape is right (archive read, OpenTIM/ReadTIM, per-slot copy loop,
   rect fill, LoadImage) but the original's frame is 0x80 and it keeps pFile in
   its 0x28 stack slot across every use, while gcc keeps it in s8 for me; the
   original also spills a temp at 0x50. The extra frame bytes suggest two more
   live locals in the original source than I have recovered.
extern RECT D_800AF5C0[5];

// Upload the five inventory-slot TIM pages for the slots that are enabled
void func_800AB808(void) {
    TIM_IMAGE tim;
    void* pFile;
    void* pImage;
    s32 i;
    s32 j;
    u_char* pDst;

    pFile = HeapAlloc(ArchiveDecodeAlignedSize(0x802), 0);
    ArchiveReadFileToBuffer(0x802, pFile, 0, 0x80);
    ArchiveCdDataSync(0);
    pImage = HeapAlloc(0xF20, 0);
    OpenTIM(pFile);
    if (ReadTIM(&tim) != 0) {
        for (i = 0; i < 5; i++) {
            if (FieldCheckSpriteSlotFlag(i) != -1) {
                continue;
            }
            if (tim.paddr == 0) {
                continue;
            }

            pDst = pImage;
            for (j = 0; j < D_800AF5C0[i].h; j++) {
                memcpy(pDst, (u_char*)tim.paddr + (j + D_800AF5C0[i].y) * 0x140 + D_800AF5C0[i].x / 4 * 4,
                    D_800AF5C0[i].w);
                pDst += D_800AF5C0[i].w / 4 * 4;
            }

            tim.prect->x = D_800AF5C0[i].x / 2 + 0x300;
            tim.prect->y = D_800AF5C0[i].y + 0x100;
            tim.prect->w = D_800AF5C0[i].w / 2;
            tim.prect->h = D_800AF5C0[i].h;
            LoadImage(tim.prect, pImage);
            DrawSync(0);
        }
    }

    HeapFree(pFile);
    HeapFree(pImage);
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800ABA98);
/* 82.61% (624 bytes, original is 668). Shape is right (table walk, item check,
   StoreImage/archive read/FieldLoadTIMWithClut, the three render loops, the
   LoadImage restore) but gcc keeps the table base D_800AF47C in a call-clobbered
   register and re-forms `lui/addiu` after every call, where the original keeps
   it in s2 for the whole function and does five separate `addu v0,s2,s1` for
   the entry fields. Also undefined-1 sentinel was 0xFFFF (fixed) and the frame
   is 0x38 vs the original 0x40 (no s3). Tried: index loop / pointer loop /
   both a base and a walker pointer, and -1 vs 0xFFFF for the sentinel.
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

extern FieldMapSpriteEntry D_800AF47C[];

extern s32 g_GameSceneMapNum;
extern u16 D_800C3900;
extern void func_800AB808();

void func_800ABA98(void) {
    RECT rect;
    FieldMapSpriteEntry* pTable;
    void* pImage;
    void* pFile;
    s32 mapNum;
    s32 archiveIndex;
    s32 index;

    mapNum = g_GameSceneMapNum & 0x3FFF;
    pTable = D_800AF47C;
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
        D_800C3914 = pTable[index].unk4;
        D_800C3A18 = pTable[index].unk8;
        D_800AFE78 = pTable[index].unk14;
        D_800AFE7C = pTable[index].unk18;

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

        while (!(D_800C3900 & 0x100)) {
            FieldClearAndSwapOTag();
            FieldDrawScreenOverlayPrims(0x80);
            FieldDisplay();
            FieldPollControllers();
        }

        for (index = 0x10; index > 0; index--) {
            FieldClearAndSwapOTag();
            FieldDrawScreenOverlayPrims(index << 3);
            FieldDisplay();
        }

        DrawSync(0);
        HeapFree(D_800C3A3C);
        HeapFree(D_800B1DF0);
        LoadImage(&rect, pImage);
        DrawSync(0);
        HeapFree(pImage);
    }
}
*/

extern SpriteList3 D_800B0188;

void FieldInitScreenStripSprites(void) {
    int i;
    RECT rect;
    
    rect.x = 0;
    rect.y = 0;
    rect.w = 0xFF;
    rect.h = 0xFF;
    
    for (i = 0; i < 5; i++) {
        SetDrawMode(&D_800B0188.drModes[i][0], 0, 0, GetTPage(1, 0, 0x280 + (i * 0x40), 0), &rect);
        SetDrawMode(&D_800B0188.drModes[i][1], 0, 0, GetTPage(1, 0, 0x280 + (i * 0x40), 0), &rect);
        SetSprt(&D_800B0188.sprites[i][0]);
        setRGB0(&D_800B0188.sprites[i][0], 0x80, 0x80, 0x80);
        setXY0(&D_800B0188.sprites[i][0], i * 0x80, 0x0);
        setUV0(&D_800B0188.sprites[i][0], 0x0, 0x0);
        setWH(&D_800B0188.sprites[i][0], 0x80, 0xE0);
        SetSemiTrans(&D_800B0188.sprites[i][0], 0);
        (&D_800B0188.sprites[i][0])->clut = GetClut(0, 0xE8);
        D_800B0188.sprites[i][1] = D_800B0188.sprites[i][0];
    }
}

extern s16 D_800ADB54;

void FieldDrawScreenStripSprites(void) {
    int i;

    if (D_800ADB54) {
        for (i = 0; i < 5; i++) {
            addPrim(g_FieldCurRenderContext->ot3, &D_800B0188.sprites[i][g_FieldCurRenderContextIndex]);
            addPrim(g_FieldCurRenderContext->ot3, &D_800B0188.drModes[i][g_FieldCurRenderContextIndex]);
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

extern s32 D_800AF780;

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
    if (D_800AF780 <= 0) {
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

    D_800AF780 -= index;
    return (u_char*)pData + index;
}

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveCdDataSync(int mode);

extern s32 D_8004F300;
extern void* D_800AF76C;
extern void* D_800AF774;
extern s32 D_800AF778;
extern s32 D_800AF77C;
extern s32 D_800AF784;

// Load the two archive files this effect needs
void FieldLoadTextRenderFiles(void) {
    ArchiveSetIndex(4, 0);
    D_800AF780 = ArchiveDecodeSize(0xAB);
    D_800AF76C = HeapAlloc(ArchiveDecodeAlignedSize(0xAB), 1);
    ArchiveReadFileToBuffer(0xAB, D_800AF76C, 0, 0x80);
    ArchiveCdDataSync(0);
    D_800AF784 = HeapAlloc(ArchiveDecodeAlignedSize(0xAC), 1);
    ArchiveReadFileToBuffer(0xAC, D_800AF784, 0, 0x80);
    ArchiveCdDataSync(0);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC3AC);

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC99C);
/* 81.27% (520 bytes, same size as the original). The shape is right: addPrim
   on the two 0x34-byte polys for this render context, then 16 blocks of
   DR_MODE[2][4] followed by SPRT[2][4] (0x100 each), the four sprites of the
   context taking y0 from the other context's first sprite, and two addPrim
   per sprite. Only register allocation differs: gcc spills the block walker
   and the 0x1000 byte counter to s0/s1 (8-byte frame) where the original
   keeps everything in t registers, and gcc emits lhu where the original has
   lh for the y0 clone (the `& 0xFF` lets combine drop the sign extension; a
   sign-dependent use of the loaded y0 is still missing). Tried: an `other`
   local vs the inline (ctx + 1) & 1, the two poly arguments as rows of
   D_800AF788[2][2] vs D_800AF7F0[ctx] and D_800AF7F0[ctx - 2] (the latter
   reproduces the original's single base symbol plus a -0x68 fold exactly),
   the y0 stores hoisted above the inner loop as four statements (80.70%,
   512 bytes, 8 short: gcc then merges the two 0x14-stride sprite walkers into
   one and drops the per-iteration addu pair). The
   in-loop y0 store is the only shape that reproduces the original's TWO
   sprite walkers (t1 accumulator for the addPrim lvalue, t2 accumulator for
   getaddr), so keep it; the gap is gcc hoisting only one of the two mask
   constant pairs and spilling the counter and the 0x50*other offset.
   Later round: the original DOES hoist the four y0 stores above the inner loop
   (four sh at 0x6A/0x7E/0x92/0xA6 straight after the lh), so four explicit
   stores are the right shape, but that variant scores only 80.34% (512 bytes,
   8 short): gcc then hoists t5+ctx*0x50 once per block where the original
   re-forms it with addu inside the inner loop, and the extra live base adds an
   s0 spill and an 8-byte frame the original does not have. The load must stay
   lh: `short y = x & 0xFF` compiles to lhu and `u_char y` to lbu, so a
   sign-dependent use of the loaded y0 is still missing from every shape tried.
typedef struct {
    DR_MODE drModes[2][4]; // 0x0
    SPRT sprites[2][4];    // 0x60
} FieldEffectPrimBlock; // Size: 0x100

extern POLY_GT4 D_800AF7F0[2];
extern s32 D_800AF770;

void func_800AC99C(void) {
    FieldEffectPrimBlock* pBlock;
    int ctx;
    int i;
    int j;

    ctx = g_FieldCurRenderContextIndex;
    addPrim(g_FieldCurRenderContext->ot3, &D_800AF7F0[ctx]);
    addPrim(g_FieldCurRenderContext->ot3, &D_800AF7F0[ctx - 2]);

    pBlock = (FieldEffectPrimBlock*)D_800AF770;
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

    FieldLoadTIMWithClut((u_long*)D_800AF784, 0x380, 0x100, 0, 0x1FF, 0, 0);
    DrawSync(0);
    HeapFree(D_800AF784);

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

    if (D_8004F300) {
        FieldLoadTextRenderFiles();
        func_800AC3AC();
        D_800AF77C = 0;
        D_800AF778 = 0xF;
        D_800AF774 = D_800AF76C;
    }
}

extern s32 D_800AF770;

void FieldFreeTextRender(void) {
    if (D_8004F300) {
        HeapFree(D_800AF76C);
        HeapFree(D_800AF770);
    }
}

void* FieldDrawTextRow(void* pData, s32 offset, s32 frame);

void FieldUpdateTextRender(void) {
    if (D_8004F300) {
        if ((D_800AF77C & 0xF) == 0) {
            D_800AF774 = FieldDrawTextRow(D_800AF774, 0x300, D_800AF778 & 0xF);
            D_800AF778++;
        }
        D_800AF77C++;
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
