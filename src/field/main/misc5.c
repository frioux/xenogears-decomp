#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "field/effects.h"
#include "field/graphics.h"
#include "field/particles.h"
#include "system/archive.h"
#include "main/game.h"

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

extern SVECTOR D_800B00B8;
extern u_short D_800B00BA;
extern u_short D_800B00BC;
extern int D_800C2684;

void FieldZoomFadeEffectUpdate(void) {
    MATRIX matrix;
    VECTOR scale;
    long proj;
    long flag;
    int i;
    int nOffset;

    RotMatrix(&D_800B00B8, &matrix);
    matrix.t[2] = 0;
    matrix.t[1] = 0;
    matrix.t[0] = 0;
    scale.vx = D_800C2684;
    scale.vy = D_800C2684;
    scale.vz = D_800C2684;
    ScaleMatrix(&matrix, &scale);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);

    for (i = 0; i < 5; i++) {
        nOffset = i * 2;
        if (D_800C2684 != 0x1000) {
            RotAverage4(&g_FieldZoomFadeEffect.vectors[i * 4], &g_FieldZoomFadeEffect.vectors[i * 4 + 1],
                &g_FieldZoomFadeEffect.vectors[i * 4 + 2], &g_FieldZoomFadeEffect.vectors[i * 4 + 3],
                (long*)(g_FieldZoomFadeEffect.polygons + nOffset + g_FieldCurRenderContextIndex) + 2,
                (long*)(g_FieldZoomFadeEffect.polygons + nOffset + g_FieldCurRenderContextIndex) + 4,
                (long*)(g_FieldZoomFadeEffect.polygons + nOffset + g_FieldCurRenderContextIndex) + 6,
                (long*)(g_FieldZoomFadeEffect.polygons + nOffset + g_FieldCurRenderContextIndex) + 8,
                &proj, &flag);
        }
        addPrim(&g_FieldCurRenderContext->ot3[0],
            g_FieldZoomFadeEffect.polygons + nOffset + g_FieldCurRenderContextIndex);
        addPrim(&g_FieldCurRenderContext->ot3[0],
            g_FieldZoomFadeEffect.drawModes + nOffset + g_FieldCurRenderContextIndex);
    }
}

void FieldZoomFadeEffectInitialize(int nSemiTrans, int nABR) {
    POLY_FT4* pPoly;
    POLY_FT4* pPolyNext;
    SVECTOR* pVector;
    int i;

    D_800C2684 = 0x1000;
    D_800B00B8.vx = 0;
    D_800B00BA = 0;
    D_800B00BC = 0;

    for (i = 0; i < 5; i++) {
        pPoly = &g_FieldZoomFadeEffect.polygons[i * 2];
        pPolyNext = &g_FieldZoomFadeEffect.polygons[i * 2 + 1];
        pVector = &g_FieldZoomFadeEffect.vectors[i * 4];
        SetPolyFT4(pPoly);

        pVector[0].vx = -0x50 + i * 0x20;
        pVector[0].vy = -0x38;
        pVector[0].vz = 0;
        pVector[1].vx = -0x30 + i * 0x20;
        pVector[1].vy = -0x38;
        pVector[1].vz = 0;
        pVector[2].vx = -0x50 + i * 0x20;
        pVector[2].vy = 0x38;
        pVector[2].vz = 0;
        pVector[3].vx = -0x30 + i * 0x20;
        pVector[3].vy = 0x38;
        pVector[3].vz = 0;

        pPoly->x0 = i * 0x40;
        pPoly->y0 = 0;
        pPoly->x1 = 0x40 + i * 0x40;
        pPoly->y1 = 0;
        pPoly->x2 = i * 0x40;
        pPoly->y2 = 0xDF;
        pPoly->x3 = 0x40 + i * 0x40;
        pPoly->y3 = 0xDF;

        setRECT(&g_FieldZoomFadeEffect.rects[i * 2], 0, 0, 0xFF, 0xFF);
        setRECT(&g_FieldZoomFadeEffect.rects[i * 2 + 1], 0, 0, 0xFF, 0xFF);

        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[i * 2], 0, 0,
            GetTPage(0x2, nABR, 0x2C0 + i * 0x40, 0x100), &g_FieldZoomFadeEffect.rects[i * 2]);
        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[i * 2 + 1], 0, 0,
            GetTPage(0x2, nABR, 0x2C0 + i * 0x40, 0x100), &g_FieldZoomFadeEffect.rects[i * 2 + 1]);

        pPoly->r0 = 0x80;
        pPoly->g0 = 0x80;
        pPoly->b0 = 0x80;
        SetSemiTrans(pPoly, nSemiTrans);

        pPoly->u0 = 0;
        pPoly->v0 = 0;
        pPoly->u1 = 0x40;
        pPoly->v1 = 0;
        pPoly->u2 = 0;
        pPoly->v2 = 0xDF;
        pPoly->u3 = 0x40;
        pPoly->v3 = 0xDF;
        pPoly->tpage = GetTPage(0x2, nABR, 0x2C0 + i * 0x40, 0x100);
        *pPolyNext = *pPoly;
    }
}


void FieldDisplay(void) {
    DrawSync(0);
    Vsync(2);
    ClearImage(&g_FieldCurRenderContext->drawEnvs[0].clip, 0x0, 0x0, 0x0);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    DrawOTag(g_FieldCurRenderContext->ot3 + 7);
}

// Fades out the vertex colors of one corner of a particle primitive while the
// corner's projected position stays near the screen centre.
void func_800A6998(POLY_GT4* pPoly, int nCorner, int nDistance, u_short* pTimer) {
    VECTOR vecA;
    VECTOR vecB;
    int nCenterX;
    int nCenterY;

    nCenterX = 0xA0;
    nCenterY = 0x70;
    vecA.vz = 0;

    switch (nCorner) {
    case 0:
        vecA.vx = nCenterX - pPoly->x0;
        vecA.vy = nCenterY - pPoly->y0;
        Square0(&vecA, &vecB);
        if ((SquareRoot0(vecB.vx + vecB.vy) >> 1) < nDistance) {
            *pTimer -= 6;
            if ((s_short)*pTimer < 0) {
                *pTimer = 0;
            }
            pPoly->r0 = *pTimer;
            pPoly->g0 = *pTimer;
            pPoly->b0 = *pTimer;
        }
        break;
    case 1:
        vecA.vx = nCenterX - pPoly->x1;
        vecA.vy = nCenterY - pPoly->y1;
        Square0(&vecA, &vecB);
        if ((SquareRoot0(vecB.vx + vecB.vy) >> 1) < nDistance) {
            *pTimer -= 6;
            if ((s_short)*pTimer < 0) {
                *pTimer = 0;
            }
            pPoly->r1 = *pTimer;
            pPoly->g1 = *pTimer;
            pPoly->b1 = *pTimer;
        }
        break;
    case 2:
        vecA.vx = nCenterX - pPoly->x2;
        vecA.vy = nCenterY - pPoly->y2;
        Square0(&vecA, &vecB);
        if ((SquareRoot0(vecB.vx + vecB.vy) >> 1) < nDistance) {
            *pTimer -= 6;
            if ((s_short)*pTimer < 0) {
                *pTimer = 0;
            }
            pPoly->r2 = *pTimer;
            pPoly->g2 = *pTimer;
            pPoly->b2 = *pTimer;
        }
        break;
    case 3:
        vecA.vx = nCenterX - pPoly->x3;
        vecA.vy = nCenterY - pPoly->y3;
        Square0(&vecA, &vecB);
        if ((SquareRoot0(vecB.vx + vecB.vy) >> 1) < nDistance) {
            *pTimer -= 6;
            if ((s_short)*pTimer < 0) {
                *pTimer = 0;
            }
            pPoly->r3 = *pTimer;
            pPoly->g3 = *pTimer;
            pPoly->b3 = *pTimer;
        }
        break;
    }
}

extern void* D_800B00C4;

// 280 particle primitives in two copies (current/next) plus four u_short
// attribute arrays indexed in parallel.
typedef struct {
    /* 0x0000 */ POLY_GT4 polygons[560];
    /* 0x71C0 */ u_short unk71C0[280];
    /* 0x73F0 */ u_short unk73F0[280];
    /* 0x7620 */ u_short unk7620[280];
    /* 0x7850 */ u_short unk7850[280];
} FieldParticlePrimitiveSet; // Size: 0x7A80

extern int D_800C3A40;
extern u_char D_800B1E24[];

void func_800A6C40(void) {
    POLY_GT4* pPoly;
    int i;
    int j;

    for (i = 0; i < 0xE; i++) {
        for (j = 0; j < 0x14; j++) {
            pPoly = (POLY_GT4*)((u_char*)D_800B00C4 + g_FieldCurRenderContextIndex * 0x38E0) + (i * 0x14 + j);
            func_800A6998(pPoly, 0, D_800C3A40, ((FieldParticlePrimitiveSet*)D_800B00C4)->unk71C0 + (i * 0x14 + j));
            func_800A6998(pPoly, 1, D_800C3A40, ((FieldParticlePrimitiveSet*)D_800B00C4)->unk73F0 + (i * 0x14 + j));
            func_800A6998(pPoly, 2, D_800C3A40, ((FieldParticlePrimitiveSet*)D_800B00C4)->unk7620 + (i * 0x14 + j));
            func_800A6998(pPoly, 3, D_800C3A40, ((FieldParticlePrimitiveSet*)D_800B00C4)->unk7850 + (i * 0x14 + j));
            addPrim(&g_FieldCurRenderContext->ot3[0],
                (POLY_GT4*)((u_char*)D_800B00C4 + g_FieldCurRenderContextIndex * 0x38E0) + (i * 0x14 + j));
        }
    }
    addPrim(&g_FieldCurRenderContext->ot3[0],
        &D_800B1E24[g_FieldCurRenderContextIndex * 0xC0]);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6E70);

/* Best attempt, 99.56% / 500 bytes (the original is 500 too): everything but
   the two UV temporaries matches, and those only land in swapped registers -
   the original keeps (j*16) with its `& 0x3F` in v0 and the `+ 0x10` in v1,
   we get v0 and v1 the other way round (the whole instruction sequence, the
   store order and every offset are identical). Tried and no help: inline vs
   named UV locals, int vs u_char locals, three declaration orders, no UV
   locals at all, swapping the u0/u2 and u1/u3 stores, a separate nU1 local.
   Settled facts: x0/x2 = nX, u0/u2 = nX & 0x3F, u1/u3 = that + 0x10,
   v0/v1 = i*16, v2/v3 = i*16 + 0x10 (gcc strength-reduces both to the s3/s4
   counters), x1/x3 = nX + 0x10, 0xE x 0x14 grid of 16x16 tiles,
   tpage = GetTPage(2, 1, j / 4 * 0x40 + 0x2C0, 0x100), and
   *pPolyNext = *pPoly copies 0x34 bytes. polygons is 560 POLY_GT4 (two
   copies of 280), so polygonsNext is at +0x38E0 and the four u_short
   attribute arrays start at 0x71C0.
    void func_800A6E70(void) {
        FieldParticlePrimitiveSet* pPrims;
        POLY_GT4* pPoly;
        POLY_GT4* pPolyNext;
        int i;
        int j;
        int nIndex;
        int nU;
        int nX;

        D_800B00C4 = HeapAlloc(0x7A80, 0x1);
        for (i = 0; i < 0xE; i++) {
            for (j = 0; j < 0x14; j++) {
                pPrims = D_800B00C4;
                nIndex = i * 0x14 + j;
                pPoly = pPrims->polygons + nIndex;
                pPolyNext = pPrims->polygons + 280 + nIndex;
                pPrims->unk71C0[nIndex] = 0x80;
                pPrims->unk73F0[nIndex] = 0x80;
                pPrims->unk7620[nIndex] = 0x80;
                pPrims->unk7850[nIndex] = 0x80;
                SetPolyGT4(pPoly);

                nX = j * 0x10;
                pPoly->x0 = nX;
                pPoly->x2 = nX;
                nU = nX & 0x3F;
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
                pPoly->u1 = nU + 0x10;
                pPoly->v1 = i * 0x10;
                pPoly->u2 = nU;
                pPoly->v2 = i * 0x10 + 0x10;
                pPoly->u3 = nU + 0x10;
                pPoly->v3 = i * 0x10 + 0x10;
                pPoly->tpage = GetTPage(0x2, 0x1, j / 4 * 0x40 + 0x2C0, 0x100);
                SetSemiTrans(pPoly, 0x1);
                *pPolyNext = *pPoly;
            }
        }
    }*/

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

extern void* D_80065B00[];
extern int g_GamePartyMemberSkins[3];
extern void* g_PartyStreamDataPointers[];
extern int ArchiveDecodeAlignedSize(int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void LZSSDecompress(void*, void*);
extern void func_80029AFC(StreamDataQueueEntry*, int, int);

void func_800A74F8(void) {
    int i;
    int entryIndex;
    RECT rect;
    StreamDataQueueEntry queue[MAX_PARTY_MEMBERS + 1];
    void* pBuffer;

    if (D_800ADB74 == 2) {
        D_8005A418[0] = HeapAlloc(0x14000, 0);
        D_8005A41C = HeapAlloc(0x14000, 0);
        HeapPinBlock(D_8005A418[0]);
        HeapPinBlock(D_8005A41C);
        ArchiveSetIndex(4, 0);

        entryIndex = 0;
        for (i = 1; i < 3; i++) {
            if (g_GamePartyMemberSkins[i] == CHARACTER_ID_NONE) {
                continue;
            }
            queue[entryIndex].archiveIndex = g_GamePartyMemberSkins[i] + 5;
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_GamePartyMemberSkins[i] + 5), 1);
            g_PartyStreamDataPointers[i] = pBuffer;
            queue[entryIndex].pData = pBuffer;
            entryIndex++;
        }

        queue[entryIndex].pData = 0;
        queue[entryIndex].archiveIndex = 0;
        func_80029AFC(queue, 0, 0);
        ArchiveCdDataSync(0);

        for (i = 1; i < 3; i++) {
            if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
                LZSSDecompress(D_80065B00[i - 1], D_8005A418[i - 1]);
                HeapFree(D_80065B00[i - 1]);
            }
        }
    } else {
        D_8005A418[0] = HeapAlloc(0x14000, 0);
        D_8005A41C = HeapAlloc(0x14000, 0);
        HeapPinBlock(D_8005A418[0]);
        HeapPinBlock(D_8005A41C);

        rect.x = 0x200;
        rect.y = 0;
        rect.w = 0x140;
        rect.h = 0x80;
        StoreImage(&rect, D_8005A418[0]);
        DrawSync(0);

        rect.x = 0x200;
        rect.y = 0x80;
        rect.w = 0x140;
        rect.h = 0x80;
        StoreImage(&rect, D_8005A41C);
        DrawSync(0);
    }
}

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

extern s32 D_8004F300;
extern s32 D_801E89E0;
extern void GameCheckAndHandleSoftReset(void);
extern void FieldClearAndSwapOTagInternal(void);
extern void func_800AC99C(void);
extern void func_800ACB90(void);
extern void func_800ACCB0(void);
extern void func_800ACCF4(void);

s32 func_800A7948(void) {
    RECT rect;

    if (!D_8004F300) {
        return;
    }
    if (D_800B06A0 < 0x687) {
        return;
    }
    if (D_800B06A0 >= 0x18E2) {
        return;
    }

    D_800AFE74 = 1;
    D_801E89E0 = 0;
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x500;
    rect.h = 0x200;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    Vsync(0);
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 0x280, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, 0x280, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, 0xE0);
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 0;
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 0;
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    rect.x = 0x300;
    rect.y = 0;
    rect.w = 0x200;
    rect.h = 0x100;
    ClearImage(&rect, 0, 0, 0);
    func_800ACB90();
    Vsync(0);
    DrawSync(0);

    while (D_800B06A0 < 0x18E2) {
        GameCheckAndHandleSoftReset();
        FieldClearAndSwapOTagInternal();
        if (D_800B06A0 < 0x18DE) {
            func_800AC99C();
        }
        DrawSync(0);
        Vsync(2);
        ClearImage(&g_FieldCurRenderContext->drawEnvs[0].clip, 0, 0, 0);
        PutDispEnv(&g_FieldCurRenderContext->dispEnv);
        PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
        DrawOTag(g_FieldCurRenderContext->ot3 + 7);
        func_800ACCF4();
        func_800A732C(5);
    }

    D_800AFE74 = 0;
    D_801E89E0 = 1;
    DrawSync(0);
    Vsync(0);
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, 0x140, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x140, 0xE0);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x140, 0xE0);
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 1;
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 1;
    g_FieldRenderContexts[1].dispEnv.isinter = 0;
    g_FieldRenderContexts[0].dispEnv.isinter = 0;
    func_800ACCB0();
}

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


INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A84C0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A8BA4);

/* Best attempt, 83.87% / 800 bytes (original is 772). Everything up to the
   per-tile UV switch matches; the delta is pure loop strength reduction and
   register shuffling. What is settled: the five palette/rect loads
   (nW/nH/nSrcX/nSrcY and the shape table) each go through ONE base symbol with
   constant offsets, so write nW/nH/nSrcX as D_800AEB6C[nRect], [nRect + 1],
   [nRect - 2] and nSrcY as D_800AEB6A[nRect] (that is the s5 = &D_800AEB6C /
   t3 = &D_800AEB6A split the original has), and reach the tile record as
   D_800AEF16[i*4-3..i*4]. The remaining difference: gcc strength-reduces the
   two tile bases to loop-carried `addu $p,$p,8` pointers (plus two `move`s in
   the prologue) where the original keeps one invariant base and re-forms
   `sll/addu` per access; the original also keeps three counters (i for the
   loop bound, i for the *8 index, i*0x28 for the polygons) exactly as this
   source does. Tried: pointer locals, splitting nAttr into a local, use of
   D_800AEF14 as the record base, -O3/gcc-2.6.0.
   extern u_short D_800AEF16[];
   extern int D_800AEB64;

   void func_800A8BA4(void) {
       POLY_FT4* pPoly;
       POLY_FT4* pPolyNext;
       int i;
       int nRect;
       int nX;
       int nY;
       int nW;
       int nH;
       int nSrcX;
       int nSrcY;
       int nABR;

       func_800A8314();
       D_800AEB64 = 0;
       HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
       D_800AFC60[0] = HeapAlloc(0x1108, 0);
       D_800AFC64 = HeapAlloc(0x1108, 0);

       nABR = 0;
       for (i = 0; i < 0x6D; i++) {
           pPoly = &D_800AFC60[0][i];
           pPolyNext = (POLY_FT4*)D_800AFC64 + i;
           SetPolyFT4(pPoly);

           pPoly->r0 = 0x80;
           pPoly->g0 = 0x80;
           pPoly->b0 = 0x80;
           pPoly->clut = GetClut(0, 0xE8);

           switch ((D_800AEF16[i * 4] >> 4) & 0xF) {
           case 0:
               nABR = 1;
               break;
           case 1:
               nABR = 2;
               break;
           }
           pPoly->tpage = GetTPage(0, nABR, 0x380, 0);
           SetSemiTrans(pPoly, 1);

           nX = D_800AEF16[i * 4 - 3];
           nY = D_800AEF16[i * 4 - 2];
           nRect = D_800AEF16[i * 4 - 1] * 4;
           nW = D_800AEB6C[nRect];
           nH = D_800AEB6C[nRect + 1];
           nSrcX = D_800AEB6C[nRect - 2];
           nSrcY = D_800AEB6A[nRect];

           pPoly->x0 = nX;
           pPoly->y0 = nY;
           pPoly->x1 = nX + nW;
           pPoly->y1 = nY;
           pPoly->x2 = nX;
           pPoly->y2 = nY + nH;
           pPoly->x3 = nX + nW;
           pPoly->y3 = nY + nH;

           switch (D_800AEF16[i * 4] & 0xF) {
           case 0:
               FieldClampPolyFT4UVs(pPoly, nSrcX, nSrcY, nSrcX + nW, nSrcY,
                   nSrcX, nSrcY + nH, nSrcX + nW, nSrcY + nH);
               break;
           case 1:
               FieldClampPolyFT4UVs(pPoly, nSrcX + nW - 1, nSrcY, nSrcX - 1, nSrcY,
                   nSrcX + nW - 1, nSrcY + nH, nSrcX - 1, nSrcY + nH);
               break;
           case 2:
               FieldClampPolyFT4UVs(pPoly, nSrcX, nSrcY + nH - 1, nSrcX + nW, nSrcY - 1,
                   nSrcX, nSrcY + nH - 1, nSrcX + nW, nSrcY - 1);
               break;
           case 3:
               FieldClampPolyFT4UVs(pPoly, nSrcX + nW - 1, nSrcY + nH - 1, nSrcX - 1, nSrcY - 1,
                   nSrcX + nW - 1, nSrcY + nH - 1, nSrcX - 1, nSrcY - 1);
               break;
           }
           *pPolyNext = *pPoly;
       }
       D_800AF278 = 1;
   } */

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldInitializeParticlePrimitive);

/* Best attempt, 97.42% / 512 bytes: the original is 520, so it is 8 bytes (2
   insns) short. Everything matches except the vertex math: the original
   re-materializes x0*16 and y0*16 (6 sll for four products; x1*16 stays in a3
   and y1*16 in t0, which are the values it does NOT recompute), while gcc CSEs
   all four products (4 sll) however they are spelled. Its four shape loads also
   land in a1(x0), a2(y0), a3(x1), t0(y1); ours get a0..a3, i.e. a0 is free in
   ours where the original used it as the x-minus scratch. Same 97.42 with -O3,
   gcc-2.6.0-psx, product locals, s16 casts, <<4/mult mixes and every
   declaration order. Vertex pairing below is the one in the disassembly
   (0xAA = y1*16 - y0*16, 0xB2/0xBA = y0*16 + y1*16); pairing vertices[1].vy to
   the sum scores 97.54 but contradicts it - operand-register noise, same 512
   bytes. ParticlePrimitive (0xC0: POLY_FT4 poly[2] at 0x50, SVECTOR vertices[4]
   at 0xA0) is in field/particles.h; `char _pad[0x58]` reproduces the 0x98 frame.
   D_800AF27C is the shape table, one ParticleShape (0x18) per index, declared as
   one extern u_short[] per field so each load relocates as %hi/%lo of
   D_800AF27C + 2*k like the original; nIndex is in u_short units (shape*0xC).
    // The shape table D_800AF27C holds one ParticleShape (0x18) per index; one extern
    // u_short[] per field so each load relocates as %hi/%lo of D_800AF27C + 2*k, and
    // nIndex is the index in u_short units (shape * 0xC).
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
        char _pad[0x58]; // unused, reproduces the 0x98 frame

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
        pParticle->vertices[1].vy = (y1 << 4) - (y0 << 4);
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
    }*/

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
