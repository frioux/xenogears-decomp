#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "field/effects.h"
#include "field/graphics.h"
#include "field/particles.h"

//
void func_800A55B8(u_char* pArg0, int arg1, int arg2, int arg3) {
    *(int*)(pArg0 + 0x14) = arg1;
    *(int*)(pArg0 + 0x18) = arg2;
    *(int*)(pArg0 + 0x1C) = arg3;
}

extern void* D_800AFE80;
extern void* D_800B069C;

void func_800A55C8(void) {
    HeapFree(D_800AFE80);
    HeapFree(D_800B069C);
}

// Set PolyFT4 color for PolyFT4s of next swap chain
void func_800A5600(u_char color) {
    int i;
    POLY_FT4* pPoly;
    int nOffset;
    
    for (i = 0; i < 5; i++) {
        nOffset = i * 2;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->r0 = color;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->g0 = color;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->b0 = color;
    }
}

// Field Fade Effect
// ---------------------------------------------------
void func_800A56A8(int duration) {
    g_FieldEffects.fades[0].b0 = 0xFF00;
    g_FieldEffects.fades[0].g0 = 0xFF00;
    g_FieldEffects.fades[0].r0 = 0xFF00;
    g_FieldEffects.fades[0].duration = duration + 1;
    g_FieldEffects.fades[0].isVisible = 1;
    g_FieldEffects.fades[0].semitransparency = 1;
    g_FieldEffects.fades[0].blueDelta = -0x10000 / duration;
    g_FieldEffects.fades[0].greenDelta = -0x10000 / duration;
    g_FieldEffects.fades[0].redDelta = -0x10000 / duration;
}

void func_800A5710(int duration) {
    g_FieldEffects.fades[0].b0 = 0x0;
    g_FieldEffects.fades[0].g0 = 0x0;
    g_FieldEffects.fades[0].r0 = 0x0;
    g_FieldEffects.fades[0].duration = duration + 1;
    g_FieldEffects.fades[0].isVisible = 1;
    g_FieldEffects.fades[0].semitransparency = 1;
    g_FieldEffects.fades[0].blueDelta = 0x10000 / duration;
    g_FieldEffects.fades[0].greenDelta = 0x10000 / duration;
    g_FieldEffects.fades[0].redDelta = 0x10000 / duration;
}
// ---------------------------------------------------

void func_800A5774(int x, int y, int h) {
    RECT rect;
    int nSize;
    int i;
    u_long* pImageBuffer;
    u_long* pWorkBuffer;

    rect.x = x;
    rect.y = y;
    rect.w = 0x40;
    rect.h = h;
    pImageBuffer = HeapAlloc(h * 0x80, 0x1);
    StoreImage( &rect, pImageBuffer);
    DrawSync(0);
    
    pWorkBuffer = pImageBuffer;
    nSize = h * (0x80 / sizeof(u_long));
    for (i = 0; i < nSize; i += 8) {
        pWorkBuffer[0] |= 0x80008000;
        pWorkBuffer[1] |= 0x80008000;
        pWorkBuffer[2] |= 0x80008000;
        pWorkBuffer[3] |= 0x80008000;
        pWorkBuffer[4] |= 0x80008000;
        pWorkBuffer[5] |= 0x80008000;
        pWorkBuffer[6] |= 0x80008000;
        pWorkBuffer[7] |= 0x80008000; 
        pWorkBuffer += 8;
    }
    
    LoadImage(&rect, pImageBuffer);
    DrawSync(0);
    HeapFree(pImageBuffer);
}

// Zoom fade effect stuff
void func_800A5884(void) {
    int i;
    int nCurX;

    FieldZoomFadeEffectInitialize();
    for (i = 0; i < 2; i++) {
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
    }

    nCurX = 0x2C0;
    for (i = 0; i < 5; i++) {
        func_800A5774(nCurX, 0x100, 0xE0);
        nCurX += 0x40;
    }

    for (i = 0; i < 2; i++) {
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A5924);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A5C40);

// Or Draw?
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldZoomFadeEffectUpdate);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldZoomFadeEffectInitialize);

void FieldDisplay(void) {
    DrawSync(0);
    Vsync(2);
    ClearImage(&g_FieldCurRenderContext->drawEnvs[0].clip, 0x0, 0x0, 0x0);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    DrawOTag(g_FieldCurRenderContext->ot3 + 7);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6998);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6C40);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6E70);

extern void* D_800B00C4;

void func_800A7064(void) {
    HeapFree(D_800B00C4);
}

extern int D_800ADB6C;
extern int D_800ADB74;
extern int D_800ADB34;
extern u_long* D_800AFC70;
extern int D_801D68B4;
extern u_short D_800C3A36;
extern void func_801D3538(int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);

void func_800A708C(void) {
    HeapChangeCurrentUser(4, NULL);
    if (D_800ADB74 == 2) {
        D_801D68B4 = 1;
    } else {
        D_801D68B4 = 0;
    }
    func_801D3538(0x140, 0xE0, 0x80, 0x10, 0x20, 0x800, D_800C3A36);
    D_800ADB6C = 0;
    HeapChangeCurrentUser(8, NULL);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7120);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7218);

extern void func_801D3F7C(void);
extern void func_80085678(void);

void func_800A732C(int nFrames) {
    int i;

    GameCheckAndHandleSoftReset();
    if (D_800ADB6C == 0) {
        for (i = 0; i < nFrames; i++) {
            func_801D3F7C();
            func_80085678();
        }
    }
}

extern void func_80077DAC(void);
extern void func_8007554C(void);
extern int ArchiveDataSync(void);

void func_800A7394(void) {
    do {
        func_80077DAC();
        func_8007554C();
    } while (ArchiveDataSync() != 0 || g_FieldCurRenderContextIndex != 0);
    CdDataSync(0);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A73E8);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A74F8);

extern int g_FieldPixelIndex;
extern u_long g_FieldCurPixel;
extern u_long* g_Field24BitImageData;
extern u_long* g_Field15BitImageData;

u_int FieldImageConvert24BitColorTo15Bit(void) {
    u_long nPixel;
    u_int nLSB;

    // Are we done reading RGB channels?
    if (!(g_FieldPixelIndex & 3)) {
        nPixel = *g_Field24BitImageData;
        g_Field24BitImageData += 1;
        g_FieldCurPixel = nPixel;
    }
    
    g_FieldPixelIndex++;
    
    nLSB = g_FieldCurPixel & 0xFF;
    g_FieldCurPixel >>= 8;
    
    if (nLSB) {
        nLSB = nLSB >> 3;
        if (nLSB == 0)
            nLSB = 1;
    }
    
    return nLSB;
}

void FieldImageConvert24BitTo15Bit(void) {
    RECT rect;
    int i;
    int j;
    u_long* pImage24Bit;
    u_long* pImage15Bit;
    u_long n15BitPixels;

    pImage24Bit = HeapAlloc(0xA800, 0x0);
    pImage15Bit = HeapAlloc(0x7000, 0x0);
    
    for (i = 0; i < 5; i++) {
        rect.x = i * 0x60;
        rect.y = 0;
        rect.w = 0x60;
        rect.h = 0xE0;
        StoreImage(&rect, pImage24Bit);
        DrawSync(0);
        g_Field24BitImageData = pImage24Bit;
        g_Field15BitImageData = pImage15Bit;
        g_FieldPixelIndex = 0;
        
        for (j = 0; j < 0x1C00; j++) {
            // 15 Bit: RGBRGB
            n15BitPixels = FieldImageConvert24BitColorTo15Bit();
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x5;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0xA;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x10;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x15;
            n15BitPixels |= FieldImageConvert24BitColorTo15Bit() << 0x1A;
            *g_Field15BitImageData = n15BitPixels;
            g_Field15BitImageData += 1;
        }
        
        rect.x = i << 6;
        rect.y = 0x100;
        rect.w = 0x40;
        rect.h = 0xE0;
        LoadImage(&rect, pImage15Bit);
        DrawSync(0);
    }
    HeapFree(pImage24Bit);
    HeapFree(pImage15Bit);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7948);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7C58);

extern void ArchiveCdDataSync(int mode);
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX, short clutY, short clutWidth, short clutHeight);

void func_800A8314(void) {
    void* pBuffer;

    HeapChangeCurrentUser(8, NULL);
    ArchiveSetIndex(4, 0);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0xAA), 0x1);
    ArchiveReadFileToBuffer(0xAA, pBuffer, 0, 0x80);
    ArchiveCdDataSync(0);
    FieldLoadTIMWithClut(pBuffer, 0x380, 0x0, 0x0, 0xE8, 0x0, 0x0);
    DrawSync(0);
    HeapFree(pBuffer);
}

extern int D_800AF278;
extern void* D_800AFC60;
extern void* D_800AFC64;

void func_800A83B4(void) {
    if (D_800AF278 != 0) {
        D_800AF278 = 0;
        DrawSync(0);
        HeapFree(D_800AFC60);
        HeapFree(D_800AFC64);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A8408);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A84C0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A8BA4);

extern ParticleShape D_800AF27C[];
extern void FieldClampPolyFT4UVs(POLY_FT4* poly, short u0, short v0, short u1, short v1, short u2, short v2, short u3, short v3);

void FieldInitializeParticlePrimitive(ParticlePrimitive* pPrim, int shape, int abr) {
    POLY_FT4* pPoly;
    ParticleShape* pShape;
    int nWidth;
    int nHeight;
    int nXSum;
    int nYSum;

    pPoly = &pPrim->poly[0];
    SetPolyFT4(pPoly);
    pShape = &D_800AF27C[shape];

    pPrim->vertices[0].vz = 0;
    pPrim->vertices[1].vz = 0;
    pPrim->vertices[2].vz = 0;
    pPrim->vertices[3].vz = 0;

    pPoly->r0 = 0x80;
    pPoly->g0 = 0x80;
    pPoly->b0 = 0x80;

    nWidth = (pShape->x1 - pShape->x0) << 4;
    nHeight = (pShape->y1 - pShape->y0) << 4;
    nXSum = (pShape->x0 + pShape->x1) << 4;
    nYSum = (pShape->y0 + pShape->y1) << 4;

    pPrim->vertices[0].vx = nWidth;
    pPrim->vertices[0].vy = nHeight;
    pPrim->vertices[1].vx = nXSum;
    pPrim->vertices[1].vy = nHeight;
    pPrim->vertices[2].vx = nWidth;
    pPrim->vertices[2].vy = nYSum;
    pPrim->vertices[3].vx = nXSum;
    pPrim->vertices[3].vy = nYSum;

    FieldClampPolyFT4UVs(pPoly,
        pShape->u0,     pShape->v0 + 0x40,
        pShape->u1 - 1, pShape->v1 + 0x40,
        pShape->u2,     pShape->v2 + 0x3F,
        pShape->u3 - 1, pShape->v3 + 0x3F);

    SetSemiTrans(pPoly, 1);
    pPoly->tpage = GetTPage(0, abr, 0x3C0, 0x140);
    pPoly->clut = GetClut(0x100, 0xF7);
    pPrim->poly[1] = pPrim->poly[0];
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A90B4);

/* Best attempt: 98.69%, instructions identical but the two loop pointers land in
   the other registers (original: destination $t0 / end $a3, gcc: $a3 / $t0).
typedef struct {
    u_long w0;
    u_long w1;
    u_long w2;
    u_long w3;
} ImageCopyQuad; // Size: 0x10

void func_800A90B4(u_int allocFlags) {
    void* pNew;
    ImageCopyQuad* pSrc;
    ImageCopyQuad* pDst;
    ImageCopyQuad* pEnd;

    if (D_800ADB34 == 1) {
        HeapChangeCurrentUser(8, NULL);
        pNew = HeapAlloc(0x8000, allocFlags);
        pDst = pNew;
        pSrc = (ImageCopyQuad*)D_800AFC70;
        pEnd = (ImageCopyQuad*)D_800AFC70 + 0x800;
        do {
            *pDst = *pSrc;
            pDst++;
            pSrc++;
        } while (pSrc != pEnd);
        HeapFree(D_800AFC70);
        D_800AFC70 = pNew;
    }
}
*/

extern RECT g_FieldStoredImageDest[];

void func_800A915C(void) {
    if (D_800ADB34 != 1) {
        D_800ADB34 = 1;
        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        D_800AFC70 = HeapAlloc(0x8000, 0x1);
        setRECT(&g_FieldStoredImageDest[0], 0x3C0, 0x100, 0x40, 0x100);
        StoreImage(&g_FieldStoredImageDest[0], D_800AFC70);
        DrawSync(0);
    }
}

void func_800A91F0(void) {
    if (D_800ADB34 != 0) {
        D_800ADB34 = 0;
        setRECT(&g_FieldStoredImageDest[0], 0x3C0, 0x100, 0x40, 0x100);
        LoadImage(&g_FieldStoredImageDest[0], D_800AFC70);
        DrawSync(0);
        HeapFree(D_800AFC70);
    }
}
