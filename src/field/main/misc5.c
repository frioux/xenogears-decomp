#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "field/effects.h"
#include "field/graphics.h"
#include "field/particles.h"

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

extern void* D_800B00C4;

// 280 particle primitives in two copies (current/next) plus four u_short
// attribute arrays indexed in parallel.
typedef struct {
    /* 0x0000 */ POLY_GT4 polygons[280];
    /* 0x38E0 */ POLY_GT4 polygonsNext[280];
    /* 0x71C0 */ u_short unk71C0[280];
    /* 0x73F0 */ u_short unk73F0[280];
    /* 0x7620 */ u_short unk7620[280];
    /* 0x7850 */ u_short unk7850[280];
} FieldParticlePrimitiveSet; // Size: 0x7A80

/* Best attempt, 99.56%: only the two UV temporaries land in the opposite
   registers (v0/v1 swapped on the andi/addiu pair and their four
   stores); every other instruction matches. Tried: inline vs named UV
   locals, int vs u_char locals, declaration order, separate nX/nU/nU1.
   x0/x2 use nX, u0/u2 use nX & 0x3F, u1/u3 use that + 0x10,
   v0/v1 = i*16, v2/v3 = i*16 + 0x10 (gcc hoists i*16 itself), x1/x3 =
   nX + 0x10 (strength-reduced to a counter), 20x14 grid of 16x16 tiles,
   tpage = GetTPage(2, 1, j / 4 * 0x40 + 0x2C0, 0x100).
    void func_800A6E70(void) {
        FieldParticlePrimitiveSet* pPrims;
        POLY_GT4* pPoly;
        POLY_GT4* pPolyNext;
        int i;
        int j;
        int nIndex;
        int nU1;
        int nU;
        int nX;

        D_800B00C4 = HeapAlloc(0x7A80, 0x1);
        for (i = 0; i < 0xE; i++) {
            for (j = 0; j < 0x14; j++) {
                pPrims = D_800B00C4;
                nIndex = i * 0x14 + j;
                pPoly = pPrims->polygons + nIndex;
                pPolyNext = pPrims->polygonsNext + nIndex;
                pPrims->unk71C0[nIndex] = 0x80;
                pPrims->unk73F0[nIndex] = 0x80;
                pPrims->unk7620[nIndex] = 0x80;
                pPrims->unk7850[nIndex] = 0x80;
                SetPolyGT4(pPoly);

                nX = j * 0x10;
                pPoly->x0 = nX;
                pPoly->x2 = nX;
                nU = nX & 0x3F;
                nU1 = nU + 0x10;
                pPoly->r0 = 0x80;
                pPoly->g0 = 0x80;
                pPoly->b0 = 0x80;
                pPoly->r1 = 0x80;
                pPoly->g1 = 0x80;
                pPoly->b1 = 0x80;
                pPoly->r2 = 0x80;
                pPoly->g2 = 0x80;
                pPoly->b2 = 0x80;
                pPoly->r3 = 0x80;
                pPoly->g3 = 0x80;
                pPoly->b3 = 0x80;
                pPoly->y0 = i * 0x10;
                pPoly->x1 = nX + 0x10;
                pPoly->y1 = i * 0x10;
                pPoly->y2 = i * 0x10 + 0x10;
                pPoly->x3 = nX + 0x10;
                pPoly->y3 = i * 0x10 + 0x10;
                pPoly->u0 = nU;
                pPoly->v0 = i * 0x10;
                pPoly->u1 = nU1;
                pPoly->v1 = i * 0x10;
                pPoly->u2 = nU;
                pPoly->v2 = i * 0x10 + 0x10;
                pPoly->u3 = nU1;
                pPoly->v3 = i * 0x10 + 0x10;
                pPoly->tpage = GetTPage(0x2, 0x1, j / 4 * 0x40 + 0x2C0, 0x100);
                SetSemiTrans(pPoly, 0x1);
                *pPolyNext = *pPoly;
            }
        }
    }
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6E70);

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
extern POLY_FT4* D_800AFC60[];
extern void* D_800AFC64;

void func_800A83B4(void) {
    if (D_800AF278 != 0) {
        D_800AF278 = 0;
        DrawSync(0);
        HeapFree(D_800AFC60[0]);
        HeapFree(D_800AFC64);
    }
}

extern u_short D_800AEF14[];
extern u_short D_800AEB68[];
extern u_short D_800AEB6A[];
extern u_short D_800AEB6C[];
extern u_short D_800AEB6E[];

void func_800A8408(int nQuad, int x, int y) {
    u_long unk[6];
    POLY_FT4* pPoly;
    int nRect;
    int nY;
    int nW;
    int nX;
    int nH;
    int u0;
    int u1;
    int v0;
    int v1;

    nRect = D_800AEF14[nQuad * 4] * 4;
    nY = D_800AEB6A[nRect];
    nW = D_800AEB6C[nRect];
    nX = D_800AEB68[nRect];
    pPoly = &D_800AFC60[g_FieldCurRenderContextIndex][nQuad];
    nH = D_800AEB6E[nRect];
    nY += y;
    u0 = nX + x;
    u1 = u0 + nW;
    v1 = nY + nH - 1;
    v0 = nY - 1;
    FieldClampPolyFT4UVs(pPoly, u0, v1, u1, v1, u0, v0, u1, v0);
}

/* Best attempt, 71.78%: only instruction scheduling differs.
   The original frame has 0x18 unused bytes, hence `unk`. Two facts learned: the
   original passes the UVs untruncated, so its prototype here took ints, not
   shorts, and the arguments are (pPoly, x, y+h-1, x+w, y+h-1, x, y-1, x+w, y-1).
extern u_short D_800AEB68[]; // 8 byte entries: 0x0 -> x
extern u_short D_800AEB6A[]; //              0x2 -> y
extern u_short D_800AEB6C[]; //              0x4 -> w
extern u_short D_800AEB6E[]; //              0x6 -> h
extern u_short D_800AEF14[]; // 8 byte entries, entry 0 is the sprite rect index

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

/* Best attempt, 97.42%: the vertex math allocates registers differently (the
   original recomputes x0*16 and y0*16 instead of keeping them live), 2 insns.
   ParticlePrimitive (Size 0xC0: POLY_FT4 poly[2] at 0x50, SVECTOR vertices[4]
   at 0xA0) is in field/particles.h. The shape table is D_800AF27C, one
   ParticleShape (0x18) per index, declared as one extern u_short[] per field so
   the relocations come out as %hi/%lo of D_800AF27C + 2*k like the original.
extern u_short D_800AF27C[]; // x0
extern u_short D_800AF27E[]; // y0
extern u_short D_800AF280[]; // x1
extern u_short D_800AF282[]; // y1
extern u_short D_800AF284[]; // u0
extern u_short D_800AF286[]; // v0
extern u_short D_800AF288[]; // u1
extern u_short D_800AF28A[]; // v1
extern u_short D_800AF28C[]; // u2
extern u_short D_800AF28E[]; // v2
extern u_short D_800AF290[]; // u3
extern u_short D_800AF292[]; // v3

   My build at this percentage: only the vertex math differs - the four
   loaded shape values land in a0-a3 instead of a1-t0 and gcc CSEs
   x0*16 / y0*16 where the original recomputes them (6 sll against 4), so
   this version is 8 bytes short.
    void FieldInitializeParticlePrimitive(ParticlePrimitive* pParticle, int shapeIndex, int nTextureMode) {
        POLY_FT4* pPoly;
        POLY_FT4* pPoly2;
        int nIndex;
        int x0;
        int y0;
        int x1;
        int y1;
        int nLeft;
        int nTop;
        int nRight;
        int nBottom;
        char _pad[0x58];

        pPoly = &pParticle->poly[0];
        pPoly2 = &pParticle->poly[1];
        SetPolyFT4(pPoly);

        nIndex = shapeIndex * 0xC;
        x0 = D_800AF27C[nIndex];
        y0 = D_800AF27E[nIndex];
        x1 = D_800AF280[nIndex];
        y1 = D_800AF282[nIndex];

        pParticle->vertices[0].vz = 0;
        pParticle->vertices[1].vz = 0;
        pParticle->vertices[2].vz = 0;
        pParticle->vertices[3].vz = 0;

        setRGB0(pPoly, 0x80, 0x80, 0x80);

        nLeft = (x1 << 4) - (x0 << 4);
        nTop = (y1 << 4) - (y0 << 4);
        nRight = (x0 << 4) + (x1 << 4);
        nBottom = (y0 << 4) + (y1 << 4);

        pParticle->vertices[0].vx = nLeft;
        pParticle->vertices[0].vy = nTop;
        pParticle->vertices[1].vx = nRight;
        pParticle->vertices[1].vy = nTop;
        pParticle->vertices[2].vx = nLeft;
        pParticle->vertices[2].vy = nBottom;
        pParticle->vertices[3].vx = nRight;
        pParticle->vertices[3].vy = nBottom;

        FieldClampPolyFT4UVs(pPoly,
            D_800AF284[nIndex], D_800AF286[nIndex] + 0x40,
            D_800AF288[nIndex] - 1, D_800AF28A[nIndex] + 0x40,
            D_800AF28C[nIndex], D_800AF28E[nIndex] + 0x3F,
            D_800AF290[nIndex] - 1, D_800AF292[nIndex] + 0x3F);

        SetSemiTrans(pPoly, 0x1);
        pPoly->tpage = GetTPage(0x0, nTextureMode, 0x3C0, 0x140);
        pPoly->clut = GetClut(0x100, 0xF7);

        *pPoly2 = *pPoly;
    }
*/

extern u_short D_800AF27C[]; // x0
extern u_short D_800AF27E[]; // y0
extern u_short D_800AF280[]; // x1
extern u_short D_800AF282[]; // y1
extern u_short D_800AF284[]; // u0
extern u_short D_800AF286[]; // v0
extern u_short D_800AF288[]; // u1
extern u_short D_800AF28A[]; // v1
extern u_short D_800AF28C[]; // u2
extern u_short D_800AF28E[]; // v2
extern u_short D_800AF290[]; // u3
extern u_short D_800AF292[]; // v3

void FieldInitializeParticlePrimitive(ParticlePrimitive* pParticle, int shapeIndex, int nTextureMode) {
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;
    int nIndex;
    int x0;
    int y0;
    int x1;
    int y1;
    char _pad[0x58];

    pPoly = &pParticle->poly[0];
    pPoly2 = &pParticle->poly[1];
    SetPolyFT4(pPoly);

    nIndex = shapeIndex * 0xC;
    x0 = D_800AF27C[nIndex];
    y0 = D_800AF27E[nIndex];
    x1 = D_800AF280[nIndex];
    y1 = D_800AF282[nIndex];

    pParticle->vertices[0].vz = 0;
    pParticle->vertices[1].vz = 0;
    pParticle->vertices[2].vz = 0;
    pParticle->vertices[3].vz = 0;

    setRGB0(pPoly, 0x80, 0x80, 0x80);

    pParticle->vertices[0].vx = (x1 << 4) - (x0 << 4);
    pParticle->vertices[0].vy = (y1 << 4) - (y0 << 4);
    pParticle->vertices[1].vx = (x0 << 4) + (x1 << 4);
    pParticle->vertices[1].vy = (y0 << 4) + (y1 << 4);
    pParticle->vertices[2].vx = (x1 << 4) - (x0 << 4);
    pParticle->vertices[2].vy = (y0 << 4) + (y1 << 4);
    pParticle->vertices[3].vx = (x0 << 4) + (x1 << 4);
    pParticle->vertices[3].vy = (y0 << 4) + (y1 << 4);

    FieldClampPolyFT4UVs(pPoly,
        D_800AF284[nIndex], D_800AF286[nIndex] + 0x40,
        D_800AF288[nIndex] - 1, D_800AF28A[nIndex] + 0x40,
        D_800AF28C[nIndex], D_800AF28E[nIndex] + 0x3F,
        D_800AF290[nIndex] - 1, D_800AF292[nIndex] + 0x3F);

    SetSemiTrans(pPoly, 0x1);
    pPoly->tpage = GetTPage(0x0, nTextureMode, 0x3C0, 0x140);
    pPoly->clut = GetClut(0x100, 0xF7);

    *pPoly2 = *pPoly;
}


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
