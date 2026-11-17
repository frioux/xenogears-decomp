#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "field/camera.h"
#include "field/effects.h"
#include "field/graphics.h"
#include "field/particles.h"
#include "system/archive.h"
#include "system/controller.h"
#include "main/game.h"

// Zoom fade effect: five texture tiles, FIELD_ZOOM_FADE_TILE_WIDTH pixels
// wide, two polygons each
#define FIELD_ZOOM_FADE_TILE_COUNT 5
#define FIELD_ZOOM_FADE_TILE_WIDTH 0x40

#define FIELD_SCREEN_CENTER_X (SCREEN_WIDTH / 2)
#define FIELD_SCREEN_CENTER_Y (SCREEN_HEIGHT / 2)

// Particle primitive grid, one 16x16 tile per entry, indexed i *
// FIELD_PARTICLE_GRID_COLUMNS + j
#define FIELD_PARTICLE_GRID_ROWS 14
#define FIELD_PARTICLE_GRID_COLUMNS 20

// How much a particle corner's fade timer drops each frame it stays inside
// g_FieldParticleFadeDistance of the screen centre
#define FIELD_PARTICLE_CORNER_FADE_STEP 6

// STP, the semi-transparency bit of two packed 16-bit texels
#define FIELD_TEXEL_SEMITRANS_MASK 0x80008000

// Saved copy of one 0x140 x 0x80 16-bit screen image
#define FIELD_SAVED_FRAMEBUFFER_SIZE 0x14000

// g_FieldStreamId value when the field is not playing a streamed sequence
#define FIELD_STREAM_ID_NONE 0xFF

// Stream frames the text segment is shown on, and how far one text frame
// advances the stream
#define FIELD_TEXT_SEGMENT_START_FRAME 0x687
#define FIELD_TEXT_SEGMENT_END_FRAME 0x18E2
#define FIELD_STREAM_STEP_FRAMES 5

// Archive entry the field sprite sheet TIM is read from
#define FIELD_SPRITE_SHEET_ARCHIVE_INDEX 0xAA

// Archive entry holding the field's screen TIM, loaded into VRAM when the
// streamed sequence starts
#define FIELD_SCREEN_TIM_ARCHIVE_INDEX 0xA9

// Buffer StoreImage fills with the screen the streamed sequence is drawn over
#define FIELD_SCREEN_IMAGE_BUFFER_SIZE 0x18000

// Field modes the streamed sequence runs in: FIELD_MODE_PLAYBACK keeps the
// stream frames coming, FIELD_MODE_STEP is the one that is stepped by hand
#define FIELD_MODE_PLAIN 0
#define FIELD_MODE_PLAYBACK 1
#define FIELD_MODE_STEP 2

//
void FieldSetStructWords14To1C(int* pData, int a, int b, int c) {
    pData[5] = a;
    pData[6] = b;
    pData[7] = c;
}

extern void* D_800AFE80;
extern void* D_800B069C;

void FieldFreeTwoHeapBuffers(void) {
    HeapFree(D_800AFE80);
    HeapFree(D_800B069C);
}

// Set PolyFT4 color for PolyFT4s of next swap chain
void FieldZoomFadeEffectSetPolygonColor(int color) {
    int i;
    POLY_FT4* pPoly;
    int nOffset;
    
    for (i = 0; i < FIELD_ZOOM_FADE_TILE_COUNT; i++) {
        nOffset = i * 2;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->r0 = color;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->g0 = color;
        (g_FieldZoomFadeEffect.polygons + nOffset + ((g_FieldCurRenderContextIndex + 1) & 1))->b0 = color;
    }
}

// Field Fade Effect
// ---------------------------------------------------
void FieldStartFadeFromWhite(int duration) {
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

void FieldStartFadeToWhite(int duration) {
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

void FieldSetVRAMTextureSemiTrans(int x, int y, int h) {
    RECT rect;
    int nSize;
    int i;
    u_long* pImageBuffer;
    u_long* pWorkBuffer;

    rect.x = x;
    rect.y = y;
    rect.w = FIELD_ZOOM_FADE_TILE_WIDTH;
    rect.h = h;
    pImageBuffer = HeapAlloc(h * 0x80, 0x1);
    StoreImage( &rect, pImageBuffer);
    DrawSync(0);
    
    pWorkBuffer = pImageBuffer;
    nSize = h * (0x80 / sizeof(u_long));
    for (i = 0; i < nSize; i += 8) {
        pWorkBuffer[0] |= FIELD_TEXEL_SEMITRANS_MASK;
        pWorkBuffer[1] |= FIELD_TEXEL_SEMITRANS_MASK;
        pWorkBuffer[2] |= FIELD_TEXEL_SEMITRANS_MASK;
        pWorkBuffer[3] |= FIELD_TEXEL_SEMITRANS_MASK;
        pWorkBuffer[4] |= FIELD_TEXEL_SEMITRANS_MASK;
        pWorkBuffer[5] |= FIELD_TEXEL_SEMITRANS_MASK;
        pWorkBuffer[6] |= FIELD_TEXEL_SEMITRANS_MASK;
        pWorkBuffer[7] |= FIELD_TEXEL_SEMITRANS_MASK; 
        pWorkBuffer += 8;
    }
    
    LoadImage(&rect, pImageBuffer);
    DrawSync(0);
    HeapFree(pImageBuffer);
}

// Zoom fade effect stuff
void FieldZoomFadeEffectLoadTextures(void) {
    int i;
    int nCurX;

    FieldZoomFadeEffectInitialize();
    for (i = 0; i < 2; i++) {
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
    }

    nCurX = 0x2C0;
    for (i = 0; i < FIELD_ZOOM_FADE_TILE_COUNT; i++) {
        FieldSetVRAMTextureSemiTrans(nCurX, 0x100, SCREEN_HEIGHT);
        nCurX += FIELD_ZOOM_FADE_TILE_WIDTH;
    }

    for (i = 0; i < 2; i++) {
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
    }
}

extern s32 g_FieldScriptFadeMode;
extern s32 g_FieldScriptFadeDuration;
extern s16 D_800ADC08;
extern int g_FieldParticleFadeDistance;
extern void FontFree(void);
extern void FieldResetPortraitSlots(void);
extern void FieldCopyScreenToFadeTexture(void);
extern void FieldClearAndSwapOTag(void);
extern void FieldRenderSync(void);
extern void FieldFadeToBlack(int duration);
extern void FieldFadeToWhite(int duration);
extern void FieldUpdateFrame(void);
extern void func_8007554C(void);
extern void FieldTickSkipCounter(void);
extern void FieldLoadFont(void);
extern void func_800A6E70(void);
extern void func_800A7064(void);
extern void FieldStoreScreenImage(void);
extern void FieldRestoreScreenImage(void);
extern void FieldZoomFadeEffectInitialize(int nSemiTrans, int nABR);

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A5924);

/* Best attempt, 49.06%. The whole body decompiles cleanly - every call, both
   16.16 fixed-point fade loops (the color starts at 0x800000 and moves by
   0x800000 / g_FieldScriptFadeDuration per frame), the FieldSetVRAMTextureSemiTrans tile loop and the
   "wait until g_FieldScriptFadeMode leaves 3" loop - but gcc lays the four switch case
   bodies out in a different order, which costs ~1500 of the 1600 bytes of
   branches. Body order in the original is [case 4][case 1,2][case 3]
   (0x59D8 / 0x5A7C / 0x5B20); with the source order below gcc emits
   [case 3][case 4][case 1,2], and writing the cases in that order in the
   source moves case 3 to the front instead, so it is not source order. The
   dispatch tree itself (== 3, then < 4, then == 4, then <= 0) is identical.
   Worth trying: a case label order or a switch spelling that reproduces the
   original layout, or a jump-table-looking enum range.
   Side fact recovered from the original call sites: FieldZoomFadeEffectSetPolygonColor takes an
   int (its u_char parameter used to force an `andi 0xff` on the argument),
   and both FieldZoomFadeEffectLoadTextures call sites pass (1, 1).
    void func_800A5924(void) {
        int i;
        int nColor;

        if (g_FieldScriptFadeMode == 0) {
            return;
        }
        FontFree();
        FieldResetPortraitSlots();
        FieldStoreScreenImage();
        if (g_FieldScriptFadeMode == 1 || g_FieldScriptFadeMode == 4) {
            FieldCopyScreenToFadeTexture();
            DrawSync(0);
            FieldClearAndSwapOTag();
            FieldRenderSync();
        }
        FieldRenderSync();

        for (;;) {
            switch (g_FieldScriptFadeMode) {
            case 3:
                FieldZoomFadeEffectInitialize(1, 1);
                nColor = 0x2C0;
                for (i = 0; i < 5; i++) {
                    FieldSetVRAMTextureSemiTrans(nColor, 0x100, 0xE0);
                    nColor += 0x40;
                }
                FieldFadeToWhite(g_FieldScriptFadeDuration);
                FieldZoomFadeEffectSetPolygonColor(0);
                nColor = 0;
                for (i = 0; i < g_FieldScriptFadeDuration; i++) {
                    FieldUpdateFrame();
                    FieldZoomFadeEffectUpdate();
                    func_8007554C();
                    FieldTickSkipCounter();
                    FieldZoomFadeEffectSetPolygonColor(nColor >> 16);
                    nColor += 0x800000 / g_FieldScriptFadeDuration;
                }
                break;
            case 4:
                ((void (*)(int, int))FieldZoomFadeEffectLoadTextures)(1, 1);
                func_800A6E70();
                D_800ADC08 = 1;
                FieldFadeToBlack(g_FieldScriptFadeDuration);
                g_FieldParticleFadeDistance = 0;
                for (i = 0; i < g_FieldScriptFadeDuration; i++) {
                    FieldUpdateFrame();
                    FieldAddParticlePrims();
                    func_8007554C();
                    FieldTickSkipCounter();
                    g_FieldParticleFadeDistance += 6;
                }
                DrawSync(0);
                func_800A7064();
                goto end;
            case 1:
            case 2:
                ((void (*)(int, int))FieldZoomFadeEffectLoadTextures)(1, 1);
                D_800ADC08 = 1;
                FieldFadeToBlack(g_FieldScriptFadeDuration);
                nColor = 0x800000;
                for (i = 0; i < g_FieldScriptFadeDuration; i++) {
                    FieldUpdateFrame();
                    FieldZoomFadeEffectUpdate();
                    func_8007554C();
                    FieldTickSkipCounter();
                    FieldZoomFadeEffectSetPolygonColor(nColor >> 16);
                    nColor -= 0x800000 / g_FieldScriptFadeDuration;
                    if (nColor < 0) {
                        nColor = 0;
                    }
                }
                break;
            default:
                goto end;
            }

            while (g_FieldScriptFadeMode == 3) {
                FieldUpdateFrame();
                FieldZoomFadeEffectUpdate();
                func_8007554C();
                FieldTickSkipCounter();
            }
        }

    end:
        g_FieldScriptFadeMode = 0;
        FieldRestoreScreenImage();
        FieldLoadFont();
    }
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A5C40);

extern SVECTOR g_FieldZoomFadeRotation;
extern u_short D_800B00BA;
extern u_short D_800B00BC;
extern int g_FieldZoomFadeScale;

void FieldZoomFadeEffectUpdate(void) {
    MATRIX matrix;
    VECTOR scale;
    long proj;
    long flag;
    int i;
    int nOffset;

    RotMatrix(&g_FieldZoomFadeRotation, &matrix);
    matrix.t[2] = 0;
    matrix.t[1] = 0;
    matrix.t[0] = 0;
    scale.vx = g_FieldZoomFadeScale;
    scale.vy = g_FieldZoomFadeScale;
    scale.vz = g_FieldZoomFadeScale;
    ScaleMatrix(&matrix, &scale);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);

    for (i = 0; i < FIELD_ZOOM_FADE_TILE_COUNT; i++) {
        nOffset = i * 2;
        if (g_FieldZoomFadeScale != 0x1000) {
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

    g_FieldZoomFadeScale = 0x1000;
    g_FieldZoomFadeRotation.vx = 0;
    D_800B00BA = 0;
    D_800B00BC = 0;

    for (i = 0; i < FIELD_ZOOM_FADE_TILE_COUNT; i++) {
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

        pPoly->x0 = i * FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->y0 = 0;
        pPoly->x1 = FIELD_ZOOM_FADE_TILE_WIDTH + i * FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->y1 = 0;
        pPoly->x2 = i * FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->y2 = 0xDF;
        pPoly->x3 = FIELD_ZOOM_FADE_TILE_WIDTH + i * FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->y3 = 0xDF;

        setRECT(&g_FieldZoomFadeEffect.rects[i * 2], 0, 0, 0xFF, 0xFF);
        setRECT(&g_FieldZoomFadeEffect.rects[i * 2 + 1], 0, 0, 0xFF, 0xFF);

        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[i * 2], 0, 0,
            GetTPage(0x2, nABR, 0x2C0 + i * FIELD_ZOOM_FADE_TILE_WIDTH, 0x100), &g_FieldZoomFadeEffect.rects[i * 2]);
        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[i * 2 + 1], 0, 0,
            GetTPage(0x2, nABR, 0x2C0 + i * FIELD_ZOOM_FADE_TILE_WIDTH, 0x100), &g_FieldZoomFadeEffect.rects[i * 2 + 1]);

        pPoly->r0 = 0x80;
        pPoly->g0 = 0x80;
        pPoly->b0 = 0x80;
        SetSemiTrans(pPoly, nSemiTrans);

        pPoly->u0 = 0;
        pPoly->v0 = 0;
        pPoly->u1 = FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->v1 = 0;
        pPoly->u2 = 0;
        pPoly->v2 = 0xDF;
        pPoly->u3 = FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->v3 = 0xDF;
        pPoly->tpage = GetTPage(0x2, nABR, 0x2C0 + i * FIELD_ZOOM_FADE_TILE_WIDTH, 0x100);
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
void FieldFadeParticleCornerColor(POLY_GT4* pPoly, int nCorner, int nDistance, u_short* pTimer) {
    VECTOR vecA;
    VECTOR vecB;
    int nCenterX;
    int nCenterY;

    nCenterX = FIELD_SCREEN_CENTER_X;
    nCenterY = FIELD_SCREEN_CENTER_Y;
    vecA.vz = 0;

    switch (nCorner) {
    case 0:
        vecA.vx = nCenterX - pPoly->x0;
        vecA.vy = nCenterY - pPoly->y0;
        Square0(&vecA, &vecB);
        if ((SquareRoot0(vecB.vx + vecB.vy) >> 1) < nDistance) {
            *pTimer -= FIELD_PARTICLE_CORNER_FADE_STEP;
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
            *pTimer -= FIELD_PARTICLE_CORNER_FADE_STEP;
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
            *pTimer -= FIELD_PARTICLE_CORNER_FADE_STEP;
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
            *pTimer -= FIELD_PARTICLE_CORNER_FADE_STEP;
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

extern void* g_FieldParticlePrimitives;

// 280 particle primitives in two copies (current/next) plus four u_short
// attribute arrays indexed in parallel.
typedef struct {
    /* 0x0000 */ POLY_GT4 polygons[560];
    /* 0x71C0 */ u_short unk71C0[280];
    /* 0x73F0 */ u_short unk73F0[280];
    /* 0x7620 */ u_short unk7620[280];
    /* 0x7850 */ u_short unk7850[280];
} FieldParticlePrimitiveSet; // Size: 0x7A80

extern u_char D_800B1E24[];

void FieldAddParticlePrims(void) {
    POLY_GT4* pPoly;
    int i;
    int j;

    for (i = 0; i < FIELD_PARTICLE_GRID_ROWS; i++) {
        for (j = 0; j < FIELD_PARTICLE_GRID_COLUMNS; j++) {
            pPoly = (POLY_GT4*)((u_char*)g_FieldParticlePrimitives + g_FieldCurRenderContextIndex * 0x38E0) + (i * FIELD_PARTICLE_GRID_COLUMNS + j);
            FieldFadeParticleCornerColor(pPoly, 0, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk71C0 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            FieldFadeParticleCornerColor(pPoly, 1, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk73F0 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            FieldFadeParticleCornerColor(pPoly, 2, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk7620 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            FieldFadeParticleCornerColor(pPoly, 3, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk7850 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            addPrim(&g_FieldCurRenderContext->ot3[0],
                (POLY_GT4*)((u_char*)g_FieldParticlePrimitives + g_FieldCurRenderContextIndex * 0x38E0) + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
        }
    }
    addPrim(&g_FieldCurRenderContext->ot3[0],
        &D_800B1E24[g_FieldCurRenderContextIndex * 0xC0]);
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6E70);

/* Re-attempt at 99.56% / 500 bytes (the original size). Identical to the
   original instruction for instruction except the two UV temporaries swap
   registers: retail has j*16 and (j*16 & 0x3F) in v0 with +0x10 in v1, ours
   the other way round. See the note below for the shapes already measured.
    void func_800A6E70(void) {
        FieldParticlePrimitiveSet* pPrims;
        POLY_GT4* pPoly;
        POLY_GT4* pPolyNext;
        int i;
        int j;
        int nIndex;
        int nU;
        int nX;

        g_FieldParticlePrimitives = HeapAlloc(0x7A80, 0x1);
        for (i = 0; i < 0xE; i++) {
            for (j = 0; j < 0x14; j++) {
                pPrims = g_FieldParticlePrimitives;
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
    }
*/

/* PARKED-NOTE (previous attempt was 99.56% / 500 bytes, the original's size). Everything
   matches - instruction sequence, store order, offsets, every other register -
   except which of the two UV temporaries lands in v0:
     original: j*16 in v0, u0/u2 = (v0 & 0x3F) reusing v0 after j*16 dies,
               u1/u3 = that + 0x10 in v1
     ours:     j*16 and the mask in v1 (same reuse), the +0x10 in v0
   So gcc's allocno priority ranks the +0x10 temp above the j*16/mask group and
   we need the opposite. Measured, none of these work:
     - temp as a named local, int / u_short / u_char: the temp is allocated
       LAST (v1 then a0), nX takes v0 and nU does not reuse it, and the
       andi/addiu are emitted before the x0/x2 sh -> 94.92%.
     - cast the temp at the store, (u_char)(nU + 0x10): same 94.92%.
     - no UV locals at all (u0 = nX & 0x3F inline): same 94.92%.
     - mask or x local as a narrow type: adds a truncation andi -> 94.91%.
   Settled: 0xE x 0x14 grid of 16x16 tiles, i outer / j inner,
   nIndex = i*0x14 + j, polygons is 560 POLY_GT4 (two copies of 280, next copy
   at +0x38E0), the four u_short attribute arrays at 0x71C0/0x73F0/0x7620/
   0x7850 set to 0x80, y0/y1 = i*16 in s5, y2/y3 = i*16 + 0x10 in s3,
   x1/x3 = j*16 + 0x10 strength-reduced to an s4 counter,
   tpage = GetTPage(2, 1, j / 4 * 0x40 + 0x2C0, 0x100), SetSemiTrans, then
   *pPolyNext = *pPoly (0x34 bytes: a 4x lw/sw unrolled loop plus one word).
*/
void func_800A7064(void) {
    HeapFree(g_FieldParticlePrimitives);
}

extern int g_FieldStreamPlayerInitialized;
extern int g_FieldMode;
extern u_short g_FieldStreamRgb24;
extern int D_801D68B4;
extern void func_801D3538(int, int, int, int, int, int, int);

void FieldInitializeStreamPlayer(void) {
    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    if (g_FieldMode == 2) {
        D_801D68B4 = 1;
    } else {
        D_801D68B4 = 0;
    }
    func_801D3538(0x140, 0xE0, 0x80, 0x10, 0x20, 0x800, g_FieldStreamRgb24);
    g_FieldStreamPlayerInitialized = 0;
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
}

extern s32 g_FieldFrameBufferIndex;
extern s32 g_FieldStreamTextSegmentActive;
extern s32 g_FieldStreamFramePending;
extern s32 g_FieldStreamFramePosition;

typedef void (*FieldRenderContextCallback)(u_short, int, u_short);

void FieldSelectStreamRenderContext(u_short arg0, int arg1, u_short arg2) {
    g_FieldStreamFramePosition = arg0;
    g_FieldStreamFramePending = 0;
    if (arg2 == 0) {
        g_FieldFrameBufferIndex = 1;
    } else {
        g_FieldFrameBufferIndex = 0;
    }

    if (g_FieldMode == 0 && g_FieldStreamTextSegmentActive == 0) {
        DrawSync(0);
        g_FieldCurRenderContext = &g_FieldRenderContexts[g_FieldFrameBufferIndex];
        // Read as signed here, unsigned in FieldInitializeStreamPlayer, matching the original loads
        if (*(s16*)&g_FieldStreamRgb24 == 1) {
            g_FieldRenderContexts[g_FieldFrameBufferIndex & 1].dispEnv.isrgb24 = 1;
        }
    }
}

extern s32 g_FieldStreamFlags;
extern s16 g_FieldStreamFileIndex;
extern u16 D_800C3A22;
extern u16 D_800C3A24;
extern u16 D_800C3A26;
extern u16 D_800C3A28;
extern u16 D_800C3A2A;
extern u16 g_FieldStreamFrameBase;
extern u16 g_FieldStreamTotalFrames;
extern u16 g_FieldStreamLoopPlayback;
extern s16 g_FieldStreamId;
extern void func_801D37CC(int, int, int, int, int, int, int, int, int, int, int, int, FieldRenderContextCallback);

void FieldPlayStreamedSequence(void) {
    int nMode;

    g_FieldStreamFramePosition = 0;
    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    if (g_FieldStreamPlayerInitialized == 0) {
        ArchiveSetIndex(0x18, 1);
        nMode = 1;
        if (g_FieldStreamId != FIELD_STREAM_ID_NONE || (g_FieldStreamFlags & 0x40)) {
            nMode = 3;
        }
        func_801D37CC(g_FieldStreamFileIndex + 2, D_800C3A2A, g_FieldStreamFrameBase, g_FieldStreamTotalFrames, 1, nMode,
            g_FieldStreamLoopPlayback, D_800C3A22, D_800C3A24, D_800C3A26, D_800C3A28, 0xE0, FieldSelectStreamRenderContext);
        ArchiveSetIndex(4, 0);
    }
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
}

extern int g_FieldStreamPlayerInitialized;
extern void func_801D3F7C(void);
extern void FieldAdvanceSedsSequence(void);

void FieldStepStreamPlayback(int nFrames) {
    int i;

    GameCheckAndHandleSoftReset();
    if (g_FieldStreamPlayerInitialized == 0) {
        for (i = 0; i < nFrames; i++) {
            func_801D3F7C();
            FieldAdvanceSedsSequence();
        }
    }
}

extern void FieldUpdateFrame(void);
extern void func_8007554C(void);
extern int ArchiveDataSync(void);

void FieldWaitForArchiveAndFrameSync(void) {
    do {
        FieldUpdateFrame();
        func_8007554C();
    } while (ArchiveDataSync() != 0 || g_FieldCurRenderContextIndex != 0);
    CdDataSync(0);
}

extern void* g_FieldSavedFramebuffers[];
extern void* D_8005A41C;

void FieldRestoreFramebuffers(void) {
    RECT rect;

    if (g_FieldMode == 2) {
        HeapUnpinBlock(g_FieldSavedFramebuffers[0]);
        HeapUnpinBlock(D_8005A41C);
        HeapFree(g_FieldSavedFramebuffers[0]);
        HeapFree(D_8005A41C);
    } else {
        rect.x = 0x200;
        rect.y = 0;
        rect.w = SCREEN_WIDTH;
        rect.h = 0x80;
        LoadImage(&rect, g_FieldSavedFramebuffers[0]);
        DrawSync(0);
        rect.x = 0x200;
        rect.y = 0x80;
        rect.w = SCREEN_WIDTH;
        rect.h = 0x80;
        LoadImage(&rect, D_8005A41C);
        DrawSync(0);
        HeapUnpinBlock(g_FieldSavedFramebuffers[0]);
        HeapUnpinBlock(D_8005A41C);
        HeapFree(g_FieldSavedFramebuffers[0]);
        HeapFree(D_8005A41C);
    }
}

extern void* g_FieldCompressedPartyStreamData[];
extern int g_GamePartyMemberSkins[3];
extern void* g_PartyStreamDataPointers[];
extern int ArchiveDecodeAlignedSize(int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void LZSSDecompress(void*, void*);
extern void func_80029AFC(StreamDataQueueEntry*, int, int);

void FieldSaveFramebuffers(void) {
    int i;
    int entryIndex;
    RECT rect;
    StreamDataQueueEntry queue[MAX_PARTY_MEMBERS + 1];
    void* pBuffer;

    if (g_FieldMode == 2) {
        g_FieldSavedFramebuffers[0] = HeapAlloc(FIELD_SAVED_FRAMEBUFFER_SIZE, 0);
        D_8005A41C = HeapAlloc(FIELD_SAVED_FRAMEBUFFER_SIZE, 0);
        HeapPinBlock(g_FieldSavedFramebuffers[0]);
        HeapPinBlock(D_8005A41C);
        ArchiveSetIndex(4, 0);

        entryIndex = 0;
        for (i = 1; i < MAX_PARTY_MEMBERS; i++) {
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

        for (i = 1; i < MAX_PARTY_MEMBERS; i++) {
            if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
                LZSSDecompress(g_FieldCompressedPartyStreamData[i - 1], g_FieldSavedFramebuffers[i - 1]);
                HeapFree(g_FieldCompressedPartyStreamData[i - 1]);
            }
        }
    } else {
        g_FieldSavedFramebuffers[0] = HeapAlloc(FIELD_SAVED_FRAMEBUFFER_SIZE, 0);
        D_8005A41C = HeapAlloc(FIELD_SAVED_FRAMEBUFFER_SIZE, 0);
        HeapPinBlock(g_FieldSavedFramebuffers[0]);
        HeapPinBlock(D_8005A41C);

        rect.x = 0x200;
        rect.y = 0;
        rect.w = SCREEN_WIDTH;
        rect.h = 0x80;
        StoreImage(&rect, g_FieldSavedFramebuffers[0]);
        DrawSync(0);

        rect.x = 0x200;
        rect.y = 0x80;
        rect.w = SCREEN_WIDTH;
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

// nIndex is passed by every caller and ignored by the body, in the original too
void FieldImageConvert24BitTo15Bit(int nIndex) {
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
        rect.h = SCREEN_HEIGHT;
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
        rect.h = SCREEN_HEIGHT;
        LoadImage(&rect, pImage15Bit);
        DrawSync(0);
    }
    HeapFree(pImage24Bit);
    HeapFree(pImage15Bit);
}

extern s32 g_FieldTextRenderEnabled;
extern s32 D_801E89E0;
extern void GameCheckAndHandleSoftReset(void);
extern void FieldClearAndSwapOTagInternal(void);
extern void func_800AC99C(void);
extern void FieldLoadFontTIMAndClearRect(void);
extern void FieldFreeTextRender(void);
extern void FieldUpdateTextRender(void);

s32 func_800A7948(void) {
    RECT rect;

    if (!g_FieldTextRenderEnabled) {
        return;
    }
    if (g_FieldStreamFramePosition < FIELD_TEXT_SEGMENT_START_FRAME) {
        return;
    }
    if (g_FieldStreamFramePosition >= FIELD_TEXT_SEGMENT_END_FRAME) {
        return;
    }

    g_FieldStreamTextSegmentActive = 1;
    D_801E89E0 = 0;
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x500;
    rect.h = 0x200;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    Vsync(0);
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 0x280, SCREEN_HEIGHT);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, 0x280, SCREEN_HEIGHT);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, SCREEN_HEIGHT);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, SCREEN_HEIGHT);
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 0;
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 0;
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    rect.x = 0x300;
    rect.y = 0;
    rect.w = 0x200;
    rect.h = 0x100;
    ClearImage(&rect, 0, 0, 0);
    FieldLoadFontTIMAndClearRect();
    Vsync(0);
    DrawSync(0);

    while (g_FieldStreamFramePosition < FIELD_TEXT_SEGMENT_END_FRAME) {
        GameCheckAndHandleSoftReset();
        FieldClearAndSwapOTagInternal();
        if (g_FieldStreamFramePosition < 0x18DE) {
            func_800AC99C();
        }
        DrawSync(0);
        Vsync(2);
        ClearImage(&g_FieldCurRenderContext->drawEnvs[0].clip, 0, 0, 0);
        PutDispEnv(&g_FieldCurRenderContext->dispEnv);
        PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
        DrawOTag(g_FieldCurRenderContext->ot3 + 7);
        FieldUpdateTextRender();
        FieldStepStreamPlayback(FIELD_STREAM_STEP_FRAMES);
    }

    g_FieldStreamTextSegmentActive = 0;
    D_801E89E0 = 1;
    DrawSync(0);
    Vsync(0);
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 0x100, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 1;
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 1;
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 0;
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 0;
    FieldFreeTextRender();
}

extern int ArchiveDecodeAlignedSize(int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void func_8002A2D0(s32 sector);
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX, short clutY, short clutWidth, short clutHeight);

extern s32 g_FieldScriptCounter;
extern s32 g_FieldStreamReadyFlag;
extern s32 g_FieldSuppressStateChange;
extern s32 g_FieldScreenEffectSuppressed;
extern s32 g_FieldScriptFadeDuration;
extern s32 g_FieldScriptFadeMode;
extern s32 g_FieldActorModelSlotCount;
extern u_short g_FieldButtonsPressedOnce;
extern void* D_800ADB20;
extern s32 D_800ADB30;
extern s32 D_800ADB60;
extern s32 D_800AFE84;
extern void LibGearShutdown(void);
extern void FieldRenderSyncAndFlush(void);
extern void FieldParticlesFreeAll(void);
extern void FieldInitTextRender(void);
extern void FieldRenderFrame(void);
extern void FieldPollControllers(void);
extern void FieldStartScriptDataStream(void);
extern void FieldFreeScriptDataStream(void);
extern void FieldStopSedsPlayback(void);
extern void SoundSetCdVolumeWithFade(s32 targetVolume, s32 fadeFrames);
extern void func_801D43B0(void);
extern void func_80077AB4(void);
extern void func_80077884(void);
extern void func_80085788(void);

void func_800A7C58(void) {
    RECT rect;
    RenderContext* pRenderContext;
    void* pImage;
    void* pStreamData;
    s16* pStreamInfo;
    s32 nSize;
    s32 nStreamSize;
    s32 nSuppressStateChange;
    int i;

    g_FieldScriptCounter = 0;
    g_FieldStreamFramePending = 0;
    g_FieldFrameBufferIndex = 0;
    pImage = HeapAlloc(ArchiveDecodeAlignedSize(FIELD_SCREEN_TIM_ARCHIVE_INDEX), 0);
    ArchiveReadFileToBuffer(FIELD_SCREEN_TIM_ARCHIVE_INDEX, pImage, 0, 0x80);
    g_FieldStreamFramePosition = 0;
    g_FieldStreamTextSegmentActive = 0;
    FieldWaitForArchiveAndFrameSync();
    ArchiveSetIndex(0x18, 0);
    func_8002A2D0(g_FieldStreamFileIndex);
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    FieldWaitForArchiveAndFrameSync();

    if (g_FieldActorModelSlotCount != 0) {
        LibGearShutdown();
        FieldRenderSyncAndFlush();
        FieldRenderSync();
        HeapFree(D_800ADB20);
    }
    FieldParticlesFreeAll();

    if (g_FieldMode != FIELD_MODE_STEP) {
        rect.x = SCREEN_WIDTH;
        rect.y = 0;
        rect.w = 0xC0;
        rect.h = 0x100;
        LoadImage(&rect, pImage);
        DrawSync(0);
        HeapFree(pImage);
        pImage = HeapAlloc(FIELD_SCREEN_IMAGE_BUFFER_SIZE, 0);
        StoreImage(&rect, pImage);
    }

    nSuppressStateChange = g_FieldSuppressStateChange;
    nStreamSize = D_800ADB30;
    if (nSuppressStateChange == 0) {
        nSize = (nStreamSize & 0xFFFFFF) + 0xFFE2CFF8;
        pStreamData = HeapAlloc(nSize, 1);
        memcpy(pStreamData, pImage, ArchiveDecodeAlignedSize(FIELD_SCREEN_TIM_ARCHIVE_INDEX));
    } else {
        pStreamData = HeapAlloc(8, 1);
    }

    HeapFree(pImage);
    func_80085788();
    FieldInitTextRender();
    g_FieldStreamFramePending = 1;
    FieldRestoreFramebuffers();
    FieldRenderSyncAndFlush();
    HeapConsolidate();
    FieldInitializeStreamPlayer();
    FieldPlayStreamedSequence();
    g_FieldStreamReadyFlag = 1;

    do {
        Vsync(0);
        FieldStepStreamPlayback(3);
    } while (g_FieldStreamFramePending != 0);

    pStreamInfo = (s16*)&g_FieldStreamLoopPlayback;
    while (1) {
        switch (g_FieldMode) {
            case FIELD_MODE_PLAYBACK:
                FieldClearAndSwapOTagInternal();
                FieldRenderFrame();
                FieldStepStreamPlayback(6);
                break;
            case FIELD_MODE_PLAIN:
                DrawSync(0);
                Vsync(0);
                PutDispEnv(&g_FieldCurRenderContext->dispEnv);
                PutDrawEnv(g_FieldCurRenderContext);
                FieldStepStreamPlayback(3);
                DrawSync(0);
                Vsync(0);
                PutDispEnv(&g_FieldCurRenderContext->dispEnv);
                PutDrawEnv(g_FieldCurRenderContext);
                FieldStepStreamPlayback(3);
                func_800A7948();
                break;
            case FIELD_MODE_STEP:
                FieldUpdateFrame();
                func_8007554C();
                FieldStepStreamPlayback(9);
                break;
        }

        if (g_FieldSystemMode == 0) {
            if (g_FieldMode == FIELD_MODE_STEP) {
                if (g_FieldButtonsPressedOnce & CTRL_BTN_SQUARE) {
                    break;
                }
                if (g_FieldScriptCounter != 0) {
                    break;
                }
            } else {
                FieldPollControllers();
                if (g_FieldButtonsPressedOnce & CTRL_BTN_CIRCLE) {
                    break;
                }
            }
        } else {
            if (g_FieldStreamFlags & 0x80) {
                FieldPollControllers();
                if (g_FieldButtonsPressedOnce & CTRL_BTN_CIRCLE) {
                    SoundSetCdVolumeWithFade(0, 0xA);
                    for (i = 0; i < 5; i++) {
                        Vsync(0);
                    }
                    break;
                }
            }
        }

        if (g_FieldMode == FIELD_MODE_STEP && g_FieldScriptCounter != 0) {
            break;
        }
        // The stream state words: g_FieldStreamTotalFrames sits six words below
        // g_FieldStreamLoopPlayback, and the original reaches it off the same base
        if (pStreamInfo[0] == 0 && g_FieldStreamFramePosition >= ((u16*)pStreamInfo)[-6]) {
            break;
        }
    }

    Vsync(0);
    DrawSync(0);
    func_801D43B0();
    FieldRenderSyncAndFlush();
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(g_FieldCurRenderContext);
    Vsync(0);
    DrawSync(0);
    HeapFree(pStreamData);
    HeapConsolidate();
    FieldSaveFramebuffers();
    func_80077884();

    rect.x = 0;
    rect.y = g_FieldFrameBufferIndex << 8;
    rect.w = 0x1E0;
    rect.h = SCREEN_HEIGHT;
    MoveImage(&rect, 0, g_FieldFrameBufferIndex << 8);
    DrawSync(0);
    Vsync(0);

    pRenderContext = &g_FieldRenderContexts[1];
    g_FieldCurRenderContext = pRenderContext;
    PutDispEnv(&pRenderContext->dispEnv);
    PutDrawEnv(g_FieldCurRenderContext);
    if (g_FieldMode != FIELD_MODE_STEP) {
        FieldImageConvert24BitTo15Bit(0);
    }
    Vsync(0);
    g_FieldRenderContexts[0].dispEnv.isrgb24 = 0;
    g_FieldCurRenderContext = &g_FieldRenderContexts[0];
    PutDispEnv(&g_FieldRenderContexts[0].dispEnv);
    PutDrawEnv(g_FieldCurRenderContext);
    rect.x = 0;
    rect.y = 0x100;
    rect.w = SCREEN_WIDTH;
    rect.h = SCREEN_HEIGHT;
    MoveImage(&rect, 0, 0);
    DrawSync(0);
    Vsync(0);
    g_FieldRenderContexts[1].dispEnv.isrgb24 = 0;
    g_FieldCurRenderContext = pRenderContext;
    PutDispEnv(&pRenderContext->dispEnv);
    PutDrawEnv(g_FieldCurRenderContext);

    if (D_800AFE84 != 0) {
        g_FieldScreenEffectSuppressed = 1;
    } else {
        g_FieldScreenEffectSuppressed = 0;
    }
    func_80077AB4();

    if (g_FieldMode != FIELD_MODE_STEP) {
        ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        D_800ADB60 = 0;
        FieldStartScriptDataStream();
        FieldFreeScriptDataStream();
        g_FieldScriptFadeDuration = 0x20;
        g_FieldScriptFadeMode = 1;
    }
    FieldStopSedsPlayback();
    g_FieldStreamId = FIELD_STREAM_ID_NONE;
    g_FieldMode = FIELD_MODE_PLAIN;
    g_FieldStreamPlayerInitialized = -1;
}

void FieldLoadSpriteSheetTIM(void) {
    u_long* pTimData;

    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    ArchiveSetIndex(4, 0);
    pTimData = HeapAlloc(ArchiveDecodeAlignedSize(FIELD_SPRITE_SHEET_ARCHIVE_INDEX), 0x1);
    ArchiveReadFileToBuffer(FIELD_SPRITE_SHEET_ARCHIVE_INDEX, pTimData, 0, 0x80);
    ArchiveCdDataSync(0);
    FieldLoadTIMWithClut(pTimData, 0x380, 0x0, 0x0, 0xE8, 0x0, 0x0);
    DrawSync(0);
    HeapFree(pTimData);
}

extern int g_FieldSpritePrimsAllocated;
extern POLY_FT4* g_FieldSpritePolys[];
extern void* g_FieldSpritePolysCopy;

void FieldFreeSpritePrimitives(void) {
    if (g_FieldSpritePrimsAllocated != 0) {
        g_FieldSpritePrimsAllocated = 0;
        DrawSync(0);
        HeapFree(g_FieldSpritePolys[0]);
        HeapFree(g_FieldSpritePolysCopy);
    }
}

extern u_short g_FieldSpriteSheetQuads[];
extern u_short g_FieldSpriteSheetRects[];
extern u_short D_800AEB6A[];
extern u_short D_800AEB6C[];
extern u_short D_800AEB6E[];

void FieldSetSpriteUVs(int nQuad, int x, int y) {
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

    nRect = g_FieldSpriteSheetQuads[nQuad * 4] * 4;
    nY = D_800AEB6A[nRect];
    nW = D_800AEB6C[nRect];
    nX = g_FieldSpriteSheetRects[nRect];
    pPoly = &g_FieldSpritePolys[g_FieldCurRenderContextIndex][nQuad];
    nH = D_800AEB6E[nRect];
    nY += y;
    u0 = nX + x;
    u1 = u0 + nW;
    v1 = nY + nH - 1;
    v0 = nY - 1;
    FieldClampPolyFT4UVs(pPoly, u0, v1, u1, v1, u0, v0, u1, v0);
}


extern POLY_FT4* g_FieldSpritePolys[];
extern s32 g_FieldSpritePrimsAllocated;
extern long FieldGetVec2Magnitude(long, long);
extern long ratan2(long y, long x);
extern s32 D_800AEB60;
extern s32 D_800AEB64;

// Camera eye and at, 16.16 fixed point (the sprite colour code below reads
// them as three longs each)
typedef struct {
    /* 0x0 */ s32 vx;
    /* 0x4 */ s32 vy;
    /* 0x8 */ s32 vz;
} FieldCameraPosition;

extern FieldCameraPosition g_CameraEye;
extern FieldCameraPosition g_CameraAt;

// Primitive count one render context's sprite poly list holds
#define FIELD_SPRITE_POLY_COUNT 0x6D

// Colour of the sprite prims, and which ordering table slot they are linked into
#define FIELD_SPRITE_POLY_OT_INDEX 4

void func_800A84C0(void) {
    s32 nCameraAngle;
    s32 nInterpAngle;
    s32 nPolyCount;
    int i;

    if (g_FieldSpritePrimsAllocated == 0) {
        return;
    }
    nPolyCount = FIELD_SPRITE_POLY_COUNT;

    nCameraAngle = ratan2(
                       FieldGetVec2Magnitude((g_CameraAt.vx - g_CameraEye.vx) >> 16,
                           (g_CameraAt.vz - g_CameraEye.vz) >> 16),
                       (g_CameraAt.vy - g_CameraEye.vy) >> 16) &
        0xFFF;
    nInterpAngle = g_CamInterpolation.curAngleY & 0xFFF;

    for (i = 0; i < 0x10; i++) {
        FieldSetSpriteUVs(i, (nInterpAngle >> 4) & 0xF, 0);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 0x1D; i++) {
        FieldSetSpriteUVs(i, 0, (nCameraAngle >> 4) & 0xF);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 0x22; i++) {
        FieldSetSpriteUVs(i, (nInterpAngle & 0xF) << 3, 0);
        nInterpAngle >>= 2;
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 0x27; i++) {
        FieldSetSpriteUVs(i, (nCameraAngle & 0xF) << 3, 0);
        nCameraAngle >>= 2;
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 0x28; i++) {
        if ((D_800AEB60 & 0xF) == 0) {
            D_800AEB64++;
        }
        if (D_800AEB64 >= 3) {
            D_800AEB64 = 0;
        }
        FieldSetSpriteUVs(i, 0, D_800AEB64 << 3);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 0x2A; i++) {
        FieldSetSpriteUVs(i, (D_800AEB60 >> 2) & 0xF, 0);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 0x2B; i++) {
        if (!(D_800AEB60 & 0x10)) {
            addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
                &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
        }
    }
    for (; i < nPolyCount; i++) {
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    D_800AEB60++;
}


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
   g_FieldSpriteSheetQuads as the record base, -O3/gcc-2.6.0.
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

       FieldLoadSpriteSheetTIM();
       D_800AEB64 = 0;
       HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
       g_FieldSpritePolys[0] = HeapAlloc(0x1108, 0);
       g_FieldSpritePolysCopy = HeapAlloc(0x1108, 0);

       nABR = 0;
       for (i = 0; i < 0x6D; i++) {
           pPoly = &g_FieldSpritePolys[0][i];
           pPolyNext = (POLY_FT4*)g_FieldSpritePolysCopy + i;
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
       g_FieldSpritePrimsAllocated = 1;
   } */

INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldInitializeParticlePrimitive);

/* Best attempt, 97.42% / 512 bytes: the original is 520, so it is 8 bytes (2
   insns) short. Everything matches except the vertex math: the original
   re-materializes x0*16 and y0*16 (6 sll for four products; x1*16 stays in a3
   and y1*16 in t0, which are the values it does NOT recompute), while gcc CSEs
   all four products (4 sll) however they are spelled. Its four shape loads also
   land in a1(x0), a2(y0), a3(x1), t0(y1); ours get a0..a3, i.e. a0 is free in
   ours where the original used it as the x-minus scratch. The missing 2 sll are
   a cse1 invalidation artifact, not a spelling problem: retail's RTL gives the
   MINUS the second product's own pseudo as target (`subu a0,a3,a0`), which
   kills the CSE entry for x0*16 so the sum recomputes it; our expander gives
   the subtraction a fresh target (`subu t0,a2,a0`), so both product entries
   survive and the sum reuses them. Result locals in any order do not change it.
   Same 97.42 with -O3,
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

extern int g_FieldScreenImageStored;
extern u_long* g_FieldStoredImageBuffer;

typedef struct {
    /* 0x0 */ u_long unk0[0x2000];
} FieldStoredImage; // Size: 0x8000

void FieldMoveStoredImageToHeap(int nHeapType) {
    u_long* pImage;

    if (g_FieldScreenImageStored == 1) {
        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        pImage = HeapAlloc(sizeof(FieldStoredImage), nHeapType);
        *(FieldStoredImage*)pImage = *(FieldStoredImage*)g_FieldStoredImageBuffer;
        HeapFree(g_FieldStoredImageBuffer);
        g_FieldStoredImageBuffer = pImage;
    }
}

extern RECT g_FieldStoredImageDest[];

void FieldStoreScreenImage(void) {
    if (g_FieldScreenImageStored != 1) {
        g_FieldScreenImageStored = 1;
        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        g_FieldStoredImageBuffer = HeapAlloc(sizeof(FieldStoredImage), 0x1);
        setRECT(&g_FieldStoredImageDest[0], 0x3C0, 0x100, 0x40, 0x100);
        StoreImage(&g_FieldStoredImageDest[0], g_FieldStoredImageBuffer);
        DrawSync(0);
    }
}

void FieldRestoreScreenImage(void) {
    if (g_FieldScreenImageStored != 0) {
        g_FieldScreenImageStored = 0;
        setRECT(&g_FieldStoredImageDest[0], 0x3C0, 0x100, 0x40, 0x100);
        LoadImage(&g_FieldStoredImageDest[0], g_FieldStoredImageBuffer);
        DrawSync(0);
        HeapFree(g_FieldStoredImageBuffer);
    }
}
