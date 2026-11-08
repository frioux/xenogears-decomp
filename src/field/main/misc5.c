#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "field/effects.h"
#include "field/graphics.h"

//
void func_800A55B8(int* pData, int a, int b, int c) {
    pData[5] = a;
    pData[6] = b;
    pData[7] = c;
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
extern u_short D_800C3A36;
extern int D_801D68B4;
extern void func_801D3538(int, int, int, int, int, int, int);

void func_800A708C(void) {
    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    if (D_800ADB74 == 2) {
        D_801D68B4 = 1;
    } else {
        D_801D68B4 = 0;
    }
    func_801D3538(0x140, 0xE0, 0x80, 0x10, 0x20, 0x800, D_800C3A36);
    D_800ADB6C = 0;
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
}

extern s32 D_800ADB78;
extern s32 D_800AFE74;
extern s32 D_800B00E4;
extern s32 D_800B06A0;

typedef void (*FieldRenderContextCallback)(u_short, int, u_short);

void func_800A7120(u_short arg0, int arg1, u_short arg2) {
    D_800B06A0 = arg0;
    D_800B00E4 = 0;
    if (arg2 == 0) {
        D_800ADB78 = 1;
    } else {
        D_800ADB78 = 0;
    }

    if (D_800ADB74 == 0 && D_800AFE74 == 0) {
        DrawSync(0);
        g_FieldCurRenderContext = &g_FieldRenderContexts[D_800ADB78];
        // Read as signed here, unsigned in func_800A708C, matching the original loads
        if (*(s16*)&D_800C3A36 == 1) {
            g_FieldRenderContexts[D_800ADB78 & 1].dispEnv.isrgb24 = 1;
        }
    }
}

extern s32 D_800ADB80;
extern s16 D_800C3A20;
extern u16 D_800C3A22;
extern u16 D_800C3A24;
extern u16 D_800C3A26;
extern u16 D_800C3A28;
extern u16 D_800C3A2A;
extern u16 D_800C3A2C;
extern u16 D_800C3A2E;
extern u16 D_800C3A3A;
extern s16 D_800C3A38;
extern void func_801D37CC(int, int, int, int, int, int, int, int, int, int, int, int, FieldRenderContextCallback);

void func_800A7218(void) {
    int nMode;

    D_800B06A0 = 0;
    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    if (D_800ADB6C == 0) {
        ArchiveSetIndex(0x18, 1);
        nMode = 1;
        if (D_800C3A38 != 0xFF || (D_800ADB80 & 0x40)) {
            nMode = 3;
        }
        func_801D37CC(D_800C3A20 + 2, D_800C3A2A, D_800C3A2C, D_800C3A2E, 1, nMode,
            D_800C3A3A, D_800C3A22, D_800C3A24, D_800C3A26, D_800C3A28, 0xE0, func_800A7120);
        ArchiveSetIndex(4, 0);
    }
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
}

extern int D_800ADB6C;
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

extern void* D_8005A418[];
extern void* D_8005A41C;

void func_800A73E8(void) {
    RECT rect;

    if (D_800ADB74 == 2) {
        HeapUnpinBlock(D_8005A418[0]);
        HeapUnpinBlock(D_8005A41C);
        HeapFree(D_8005A418[0]);
        HeapFree(D_8005A41C);
    } else {
        rect.x = 0x200;
        rect.y = 0;
        rect.w = 0x140;
        rect.h = 0x80;
        LoadImage(&rect, D_8005A418[0]);
        DrawSync(0);
        rect.x = 0x200;
        rect.y = 0x80;
        rect.w = 0x140;
        rect.h = 0x80;
        LoadImage(&rect, D_8005A41C);
        DrawSync(0);
        HeapUnpinBlock(D_8005A418[0]);
        HeapUnpinBlock(D_8005A41C);
        HeapFree(D_8005A418[0]);
        HeapFree(D_8005A41C);
    }
}

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

extern int ArchiveDecodeAlignedSize(int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX, short clutY, short clutWidth, short clutHeight);

void func_800A8314(void) {
    u_long* pTimData;

    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    ArchiveSetIndex(4, 0);
    pTimData = HeapAlloc(ArchiveDecodeAlignedSize(0xAA), 0x1);
    ArchiveReadFileToBuffer(0xAA, pTimData, 0, 0x80);
    ArchiveCdDataSync(0);
    FieldLoadTIMWithClut(pTimData, 0x380, 0x0, 0x0, 0xE8, 0x0, 0x0);
    DrawSync(0);
    HeapFree(pTimData);
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

/* Best attempt, 71.78%: only instruction scheduling differs.
   The original frame has 0x18 unused bytes, hence `unk`.
extern u_short D_800AEB68[]; // 8 byte entries: 0x0 -> x
extern u_short D_800AEB6A[]; //              0x2 -> y
extern u_short D_800AEB6C[]; //              0x4 -> w
extern u_short D_800AEB6E[]; //              0x6 -> h
extern u_short D_800AEF14[]; // 4 shorts per entry, entry 0 is the sprite index

void func_800A8408(int nQuad, int x, int y) {
    POLY_FT4* pPoly;
    int nRect;
    int nX;
    int nY;
    int nW;
    int nH;
    int u0;
    int v0;
    int u1;
    int v1;
    u_long unk[6];

    nRect = D_800AEF14[nQuad * 4] * 4;
    nY = D_800AEB6A[nRect];
    nW = D_800AEB6C[nRect];
    nX = D_800AEB68[nRect];
    pPoly = &D_800AFC60[g_FieldCurRenderContextIndex][nQuad];
    u0 = nX + x;
    u1 = u0 + nW;
    nH = D_800AEB6E[nRect];
    v0 = nY + y - 1;
    v1 = nY + y + nH - 1;
    FieldClampPolyFT4UVs(pPoly, u0, v0, u1, v1, u0, v0, u1, v0);
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A84C0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A8BA4);

// https://decomp.me/scratch/3xYjM
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldInitializeParticlePrimitive);

extern int D_800ADB34;
extern u_long* D_800AFC70;

typedef struct {
    /* 0x0 */ u_long unk0[0x2000];
} FieldStoredImage; // Size: 0x8000

void func_800A90B4(int nHeapType) {
    u_long* pImage;

    if (D_800ADB34 == 1) {
        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        pImage = HeapAlloc(0x8000, nHeapType);
        *(FieldStoredImage*)pImage = *(FieldStoredImage*)D_800AFC70;
        HeapFree(D_800AFC70);
        D_800AFC70 = pImage;
    }
}

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
