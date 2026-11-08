#include "common.h"
#include "system/graphics.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

LineScroll* GfxLineScrollInitialize(LineScroll* pLineScroll, 
    u16 x, u16 y, u16 modulus, s16 width, s16 numLines, u16 destX, u16 destY, s8* pData
) {
    u_short* pWork;
    int i;

    HeapChangeCurrentUser(HEAP_USER_MASA, 0);
    pLineScroll->x = x;
    pLineScroll->y = y;
    pLineScroll->modulus = modulus;
    pLineScroll->width = width;
    pLineScroll->numLines = numLines;
    pLineScroll->destX = destX;
    pLineScroll->destY = destY;
    pLineScroll->pData = pData;
    pLineScroll->height = width / numLines;
    
    pWork = HeapAlloc(numLines * sizeof(u_short), 0);
    pLineScroll->unk14 = pWork;
    if (pWork == 0) {
        pLineScroll = NULL;
    } else {
        for (i = 0; i < numLines; i++) {
            pLineScroll->unk14[i] = 0;
        }
    }
    return pLineScroll;
}

void GfxLineScrollUpdate(LineScroll* pLineScroll) {
    RECT srcRect;
    int i;
    u16 destY;
    u16 offset;
    u16 remainder;

    if (!pLineScroll->unk14) {
        return;
    }

    srcRect.y = pLineScroll->y;
    srcRect.h = pLineScroll->height;
    destY = pLineScroll->destY;
    
    for (i = 0; i < pLineScroll->numLines; i++) {
        pLineScroll->unk14[i] += pLineScroll->pData[i];

        // Divide pLineScroll->unk14[i] by 0x10 and modulo it
        remainder = (u32) (((pLineScroll->unk14[i] << 0x10) >> 0x14) & 0xFFFF) % pLineScroll->modulus;
        
        offset = pLineScroll->modulus - remainder;
        
        srcRect.x = pLineScroll->x;
        srcRect.w = remainder;
        MoveImage(&srcRect, pLineScroll->destX + offset, destY);
        
        srcRect.x = remainder + pLineScroll->x;
        srcRect.w = offset;
        MoveImage(&srcRect, pLineScroll->destX, destY);
        
        destY += pLineScroll->height;
        srcRect.y += pLineScroll->height;
    }
}

void GfxLineScrollFree(LineScroll* pLineScroll) {
    if (pLineScroll->unk14) {
        HeapFree(pLineScroll->unk14);
        pLineScroll->unk14 = NULL;
    }
}

// NOTE: Probably part of libarchive, not line_scroll

// Debug feedback for a failing CD read: flashes the screen on every retry, and
// on the last one paints it white and dumps the current archive entry forever.
extern void* FontLoadFont(int startX, int startY, int width, int height, int maxLetters, unsigned int flags,
    int texpageX, int texpageY, int clutX, int clutY, void* pCompressedFontFile);
extern void FontDrawLetters(void* ot);
extern char* ArchiveGetFilePath(int entryIndex);
extern u32 g_CurArchiveOffset;
extern u32 D_80059F0C;
extern char D_800188EC[]; // "\n%d"
extern char D_800188F0[]; // "\n%s"

// The frame is packed tightly (rect 0x30, drawEnv 0x38, dispEnv 0x94, ot 0xA8),
// which a compiler that rounds every BLK local up to 8 only produces when the
// four of them are members of one aggregate.
void func_8002804C(int entryIndex, int r, int g, int b) {
    struct {
        RECT rect;
        DRAWENV drawEnv;
        DISPENV dispEnv;
        u_long ot[16];
    } gfx;

    gfx.rect.x = entryIndex * 10 + 10;
    gfx.rect.y = 0;
    gfx.rect.w = 8;
    gfx.rect.h = 0x1C0;

    if (entryIndex + 1 >= 4) {
        r = 0xFF;
        g = 0xFF;
        b = 0xFF;
    }

    ClearImage(&gfx.rect, r, g, b);
    DrawSync(0);

    if (entryIndex + 1 < 4)
        return;

    ResetGraph(0);
    InitGeom();

    SetDefDrawEnv(&gfx.drawEnv, 0, 0, 0x140, 0x100);
    SetDefDispEnv(&gfx.dispEnv, 0, 0, 0x140, 0xF0);
    gfx.drawEnv.dtd = 0;
    gfx.drawEnv.isbg = 0;
    gfx.drawEnv.dfe = 1;
    gfx.dispEnv.isinter = 0;
    PutDrawEnv(&gfx.drawEnv);
    PutDispEnv(&gfx.dispEnv);

    FontLoadFont(0x10, 0x10, 0x280, 0xF0, 0x400, 0, 0x280, 0, 0x280, 0x100, NULL);
    SetDispMask(1);

    gfx.rect.x = 0;
    gfx.rect.y = 0;
    gfx.rect.w = 0x280;
    gfx.rect.h = 0x30;
    ClearImage(&gfx.rect, 0, 0, 0);

    for (;;) {
        ClearOTagR(gfx.ot, 8);
        FontPrintf(D_800188EC, g_CurArchiveOffset + D_80059F0C - 1);
        FontPrintf(D_800188F0, ArchiveGetFilePath(D_80059F0C));
        FontDrawLetters(gfx.ot);
        DrawOTag(&gfx.ot[7]);
        DrawSync(0);
    }
}
