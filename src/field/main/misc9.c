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


s32 func_800AA9DC(FieldActorModelData* pModel) {
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AAA74);
/* 64.08% first attempt; the structure and the four bound tests below are
   decoded and right, the frame is not. Original frame is 0x58 and keeps the
   RotTransPers sxy word in a register for both halves (`sra` high, `sll`/`sra`
   low), while this C makes gcc keep sxy on the stack and re-load the low half
   with `lhu`. Remaining gaps: 8 bytes of unused frame (0x34..0x37) and the
   second RotTransPers reading sxy back from memory. Local order here is
   VECTOR, SVECTOR, flag, sxy, p, y1, x1, y2, x2.
extern MATRIX D_800B00E8;
extern s32 D_800C3A5C;
extern s32 D_800C3A60;

// Project the two silhouette points of a model and check they are on screen
s32 func_800AAA74(FieldActorModelData* pModel) {
    VECTOR vec;
    SVECTOR point;
    long flag;
    long sxy;
    long p;
    s16 y1;
    s16 x1;
    s16 y2;
    s16 x2;

    RotTrans((SVECTOR*)&pModel->centerX, &vec, &flag);
    D_800B00E8.t[0] = vec.vx;
    D_800B00E8.t[1] = vec.vy;
    D_800B00E8.t[2] = vec.vz;
    SetRotMatrix(&D_800B00E8);
    SetTransMatrix(&D_800B00E8);

    point.vx = -pModel->size;
    point.vy = -pModel->size;
    point.vz = 0;
    RotTransPers(&point, &sxy, &p, &flag);
    y1 = ((s16*)&sxy)[1];
    x1 = ((s16*)&sxy)[0];

    point.vx = pModel->size;
    point.vy = pModel->size;
    point.vz = 0;
    RotTransPers(&point, &sxy, &p, &flag);
    y2 = ((s16*)&sxy)[1];
    x2 = ((s16*)&sxy)[0];

    if (y1 >= D_800C3A60 + 0xE0) {
        return -1;
    }
    if (-D_800C3A60 >= y2) {
        return -1;
    }
    if (x1 >= D_800C3A5C + 0x140) {
        return -1;
    }
    if (-D_800C3A5C >= x2) {
        return -1;
    }
    return 0;
}

*/

extern SpriteList* D_800AFC68;

void func_800AABD8(void) {
    HeapFree(D_800AFC68);
    DrawSync(0);
}

void func_800AAC08(void) {
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
void func_800AADC8(int index, int red, int green, int blue) {
    setRGB0(&D_800AFC68->sprites[index][0], red, green, blue);
    setRGB0(&D_800AFC68->sprites[index][1], red, green, blue);
}

void func_800AAE4C(int index, int x, int y, int type) {
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

void func_800AAF80(void) {
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
s32 func_800AB328(int itemId) {
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

void func_800AB378(s8 color) {
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

s32 func_800AB748(s32 index) {
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800ABA98);


extern SpriteList3 D_800B0188;

void func_800ABD18(void) {
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

void func_800ABEC8(void) {
    int i;

    if (D_800ADB54) {
        for (i = 0; i < 5; i++) {
            addPrim(g_FieldCurRenderContext->ot3, &D_800B0188.sprites[i][g_FieldCurRenderContextIndex]);
            addPrim(g_FieldCurRenderContext->ot3, &D_800B0188.drModes[i][g_FieldCurRenderContextIndex]);
        }
    }
}

s32 func_800ABFDC(u_char* pData, s32* pIsGlyph) {
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
void func_800AC03C(u_char* pDst, u_short* pSrc) {
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC0F0);

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveCdDataSync(int mode);

extern s32 D_8004F300;
extern void* D_800AF76C;
extern void* D_800AF774;
extern s32 D_800AF778;
extern s32 D_800AF77C;
extern s32 D_800AF780;
extern s32 D_800AF784;

// Load the two archive files this effect needs
void func_800AC308(void) {
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

void func_800ACB90(void) {
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

extern void func_800AC308();
extern void func_800AC3AC();

void func_800ACC58(void) {
    RECT rect; // unused, but the original frame has this 8-byte slot

    if (D_8004F300) {
        func_800AC308();
        func_800AC3AC();
        D_800AF77C = 0;
        D_800AF778 = 0xF;
        D_800AF774 = D_800AF76C;
    }
}

extern s32 D_800AF770;

void func_800ACCB0(void) {
    if (D_8004F300) {
        HeapFree(D_800AF76C);
        HeapFree(D_800AF770);
    }
}

void* func_800AC0F0(void* pData, s32 offset, s32 frame);

void func_800ACCF4(void) {
    if (D_8004F300) {
        if ((D_800AF77C & 0xF) == 0) {
            D_800AF774 = func_800AC0F0(D_800AF774, 0x300, D_800AF778 & 0xF);
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
