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

// Bytes one FIELD_ZOOM_FADE_TILE_WIDTH wide scanline of 16-bit texels occupies
#define FIELD_ZOOM_FADE_TILE_STRIDE 0x80

// Where the field's off-screen screen image sits in VRAM; the zoom fade tiles
// are stored inside it
#define FIELD_SCREEN_VRAM_X 0x2C0
#define FIELD_SCREEN_VRAM_Y 0x100

// g_FieldZoomFadeScale is ramped by FIELD_ZOOM_FADE_SCALE_STEP per frame, up to
// FIELD_ZOOM_FADE_SCALE_MAX (0x1000 is 1:1)
#define FIELD_ZOOM_FADE_SCALE_MAX 0x22C0
#define FIELD_ZOOM_FADE_SCALE_STEP 0x20

// The field fades between black and FIELD_FADE_COLOR_MAX, a 16.16 fixed point
// polygon colour (so >> 16 is the 0..0xFF channel value)
#define FIELD_FADE_COLOR_MAX 0x800000

#define FIELD_SCREEN_CENTER_X (SCREEN_WIDTH / 2)
#define FIELD_SCREEN_CENTER_Y (SCREEN_HEIGHT / 2)

// Angles are a 4096-step turn counter; every sum of angles is wrapped back
// into it

// Particle primitive grid, one 16x16 tile per entry, indexed i *
// FIELD_PARTICLE_GRID_COLUMNS + j
#define FIELD_PARTICLE_GRID_ROWS 14
#define FIELD_PARTICLE_GRID_COLUMNS 20

// Bytes from the first primitive of one render context's copy of the grid to
// the first primitive of the next copy
#define FIELD_PARTICLE_POLY_COPY_SIZE (FIELD_PARTICLE_GRID_ROWS * FIELD_PARTICLE_GRID_COLUMNS * sizeof(POLY_GT4))

// How much a particle corner's fade timer drops each frame it stays inside
// g_FieldParticleFadeDistance of the screen centre
#define FIELD_PARTICLE_CORNER_FADE_STEP 6

// How much g_FieldParticleFadeDistance grows per frame of the particle-fade
// phase of the zoom fade effect
#define FIELD_PARTICLE_FADE_DISTANCE_STEP 6

// STP, the semi-transparency bit of two packed 16-bit texels
#define FIELD_TEXEL_SEMITRANS_MASK 0x80008000

// Saved copy of one 0x140 x 0x80 16-bit screen image
#define FIELD_SAVED_FRAMEBUFFER_SIZE 0x14000

// The snapshot covers the VRAM area at FIELD_SAVED_FRAMEBUFFER_VRAM_X in two
// FIELD_SAVED_FRAMEBUFFER_HEIGHT tall halves, one per saved buffer
#define FIELD_SAVED_FRAMEBUFFER_VRAM_X 0x200
#define FIELD_SAVED_FRAMEBUFFER_HEIGHT 0x80

// g_FieldStreamId value when the field is not playing a streamed sequence
#define FIELD_STREAM_ID_NONE 0xFF

// Stream frames the text segment is shown on, the frame it stops drawing text
// at, and how far one text frame advances the stream
#define FIELD_TEXT_SEGMENT_START_FRAME 0x687
#define FIELD_TEXT_SEGMENT_DRAW_END_FRAME 0x18DE
#define FIELD_TEXT_SEGMENT_END_FRAME 0x18E2
#define FIELD_STREAM_STEP_FRAMES 5

// Archive entry the field sprite sheet TIM is read from
#define FIELD_SPRITE_SHEET_ARCHIVE_INDEX 0xAA

// Archive entry holding the field's screen TIM, loaded into VRAM when the
// streamed sequence starts
#define FIELD_SCREEN_TIM_ARCHIVE_INDEX 0xA9

// Archive entry a party member's stream data sits at, above its skin id
#define FIELD_PARTY_STREAM_ARCHIVE_OFFSET 5

// Buffer StoreImage fills with the screen the streamed sequence is drawn over,
// FIELD_SCREEN_IMAGE_WIDTH x FIELD_SCREEN_IMAGE_HEIGHT 16-bit texels
#define FIELD_SCREEN_IMAGE_BUFFER_SIZE 0x18000
#define FIELD_SCREEN_IMAGE_WIDTH 0xC0
#define FIELD_SCREEN_IMAGE_HEIGHT 0x100

// Field modes the streamed sequence runs in: FIELD_MODE_PLAYBACK keeps the
// stream frames coming, FIELD_MODE_STEP is the one that is stepped by hand
#define FIELD_MODE_PLAIN 0
#define FIELD_MODE_PLAYBACK 1
#define FIELD_MODE_STEP 2

// The 24-bit to 15-bit screen conversion reads the framebuffer in
// FIELD_IMAGE_STRIP_COUNT strips of FIELD_IMAGE_STRIP_WIDTH 16-bit words each,
// into a FIELD_IMAGE_24BIT_STRIP_SIZE byte buffer, and writes each strip back as
// FIELD_IMAGE_STRIP_WORDS packed u_longs
#define FIELD_IMAGE_STRIP_COUNT 5
#define FIELD_IMAGE_STRIP_WIDTH 0x60
#define FIELD_IMAGE_24BIT_STRIP_SIZE 0xA800
#define FIELD_IMAGE_15BIT_STRIP_SIZE 0x7000
#define FIELD_IMAGE_STRIP_WORDS 0x1C00

// Palette rows the sprite prims cycle their source V through
#define FIELD_SPRITE_PALETTE_ROW_COUNT 3

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
    pImageBuffer = HeapAlloc(h * FIELD_ZOOM_FADE_TILE_STRIDE, 0x1);
    StoreImage( &rect, pImageBuffer);
    DrawSync(0);
    
    pWorkBuffer = pImageBuffer;
    nSize = h * (FIELD_ZOOM_FADE_TILE_STRIDE / sizeof(u_long));
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

    nCurX = FIELD_SCREEN_VRAM_X;
    for (i = 0; i < FIELD_ZOOM_FADE_TILE_COUNT; i++) {
        FieldSetVRAMTextureSemiTrans(nCurX, FIELD_SCREEN_VRAM_Y, SCREEN_HEIGHT);
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
extern s16 g_FieldFadeIsWhite;
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

extern void FieldAddParticlePrims(void);
extern void FieldZoomFadeEffectUpdate(void);

void FieldRunScriptFadeEffect(void) {
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

dispatch:
    switch (g_FieldScriptFadeMode) {
    case 4:
        ((void (*)(int, int))FieldZoomFadeEffectLoadTextures)(1, 1);
        func_800A6E70();
        g_FieldFadeIsWhite = 1;
        FieldFadeToBlack(g_FieldScriptFadeDuration);
        g_FieldParticleFadeDistance = 0;
        for (i = 0; i < g_FieldScriptFadeDuration; i++) {
            FieldUpdateFrame();
            FieldAddParticlePrims();
            func_8007554C();
            FieldTickSkipCounter();
            g_FieldParticleFadeDistance += FIELD_PARTICLE_FADE_DISTANCE_STEP;
        }
        DrawSync(0);
        func_800A7064();
        goto end;
    case 1:
    case 2:
        ((void (*)(int, int))FieldZoomFadeEffectLoadTextures)(1, 1);
        g_FieldFadeIsWhite = 1;
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
        goto end;
    case 3:
        FieldZoomFadeEffectInitialize(1, 1);
        for (i = 0, nColor = FIELD_SCREEN_VRAM_X; i < FIELD_ZOOM_FADE_TILE_COUNT; i++) {
            FieldSetVRAMTextureSemiTrans(nColor, FIELD_SCREEN_VRAM_Y, SCREEN_HEIGHT);
            nColor += FIELD_ZOOM_FADE_TILE_WIDTH;
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
    default:
        goto end;
    }

    while (1) {
        if (g_FieldScriptFadeMode != 3) {
            break;
        }
        FieldUpdateFrame();
        FieldZoomFadeEffectUpdate();
        func_8007554C();
        FieldTickSkipCounter();
    }
    goto dispatch;

end:
    g_FieldScriptFadeMode = 0;
    FieldRestoreScreenImage();
    FieldLoadFont();
}

extern s32 g_FieldFadeMode;
extern s32 g_FieldFadeDuration;
extern s32 g_FieldActive;
extern s32 g_FieldScriptDataStreamActive;
extern int* g_FieldScriptDataStream;
extern int g_GameStagedFileSize;
extern void* g_GameStagedFileBuffer;
extern s32 g_GameWdsBankLoadState;
extern s32 g_GameRequestedWdsBankIndex;
extern int g_FieldZoomFadeScale;
extern void FieldFree(void);
extern void FieldParticlesFreeAll(void);
extern void FieldResetActorSoundSlots(void);
extern void FieldTextBoxFreeAll(void);
extern void FieldMoveStoredImageToHeap(int arg0);
extern void FieldOrVramStripPixels(void);
extern void FieldStartWdsBankLoad(int arg0, int arg1);
extern void FieldFadeUpdateAndDraw(u_long* pOt, int index);
extern void FieldDisplay(void);
extern int ArchiveDataSync(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);
extern void FieldLoad(void);
extern void FieldStartScriptDataStream(void);
extern void FieldFreeScriptDataStream(void);

// Tears the field down, runs the exit fade the pending g_FieldFadeMode selects
// and restores the screen. FieldLoad and the stream calls reset the fade state,
// so the mode and duration are saved around them and written back afterwards.
void FieldUnloadSceneAndFade(void) {
    RECT rect;
    u_long* pStagedBuffer;
    s32 nFadeMode;
    s32 nFadeDuration;
    int i;
    int nColor;

    FontFree();
    FieldParticlesFreeAll();
    FieldResetActorSoundSlots();
    FieldTextBoxFreeAll();
    if (g_FieldFadeMode != 6) {
        FieldStoreScreenImage();
        if (g_FieldFadeMode != 4) {
            FieldCopyScreenToFadeTexture();
        }
    }
    DrawSync(0);
    FieldClearAndSwapOTag();
    FieldRenderSync();
    FieldFree();

    pStagedBuffer = HeapAlloc(g_GameStagedFileSize, 0);
    memcpy(pStagedBuffer, g_GameStagedFileBuffer, g_GameStagedFileSize);
    HeapUnpinBlock(g_GameStagedFileBuffer);
    HeapFree(g_GameStagedFileBuffer);

    if (g_FieldFadeMode != 6) {
        FieldMoveStoredImageToHeap(1);
    }

    g_GameStagedFileBuffer = HeapAlloc(g_GameStagedFileSize, 1);
    memcpy(g_GameStagedFileBuffer, pStagedBuffer, g_GameStagedFileSize);
    HeapPinBlock(g_GameStagedFileBuffer);
    HeapFree(pStagedBuffer);

    switch (g_FieldFadeMode) {
    case 6:
        FieldFadeToWhite(g_FieldFadeDuration);
        for (i = 0; i < g_FieldFadeDuration; i++) {
            FieldClearAndSwapOTag();
            FieldFadeUpdateAndDraw(g_FieldCurRenderContext->ot3, g_FieldCurRenderContextIndex);
            FieldDisplay();
        }
        FieldClearAndSwapOTag();
        FieldDisplay();
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        nFadeMode = g_FieldFadeMode;
        nFadeDuration = g_FieldFadeDuration;
        FieldLoad();
        FieldStartScriptDataStream();
        FieldFreeScriptDataStream();
        g_FieldFadeMode = nFadeMode;
        g_FieldFadeDuration = nFadeDuration;
        if (g_GameWdsBankLoadState == -1) {
            FieldStartWdsBankLoad(g_GameRequestedWdsBankIndex, 0);
        }
        FieldFadeToBlack(g_FieldFadeDuration);
        break;
    case 0:
        FieldZoomFadeEffectInitialize(0, 0);
        FieldFadeToWhite(g_FieldFadeDuration);
        for (i = 0; i < g_FieldFadeDuration; i++) {
            FieldClearAndSwapOTag();
            FieldFadeUpdateAndDraw(g_FieldCurRenderContext->ot3, g_FieldCurRenderContextIndex);
            FieldZoomFadeEffectUpdate();
            FieldDisplay();
        }
        // The same tail as case 6; cc1 cross-jumps the two into one block
        FieldClearAndSwapOTag();
        FieldDisplay();
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        nFadeMode = g_FieldFadeMode;
        nFadeDuration = g_FieldFadeDuration;
        FieldLoad();
        FieldStartScriptDataStream();
        FieldFreeScriptDataStream();
        g_FieldFadeMode = nFadeMode;
        g_FieldFadeDuration = nFadeDuration;
        if (g_GameWdsBankLoadState == -1) {
            FieldStartWdsBankLoad(g_GameRequestedWdsBankIndex, 0);
        }
        FieldFadeToBlack(g_FieldFadeDuration);
        break;
    case 1:
        FieldZoomFadeEffectInitialize(0, 0);
        FieldStartFadeToWhite(g_FieldFadeDuration);
        for (i = 0; i < g_FieldFadeDuration; i++) {
            FieldClearAndSwapOTag();
            FieldFadeUpdateAndDraw(g_FieldCurRenderContext->ot3, g_FieldCurRenderContextIndex);
            FieldZoomFadeEffectUpdate();
            FieldDisplay();
        }
        FieldRenderSync();
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        nFadeMode = g_FieldFadeMode;
        nFadeDuration = g_FieldFadeDuration;
        FieldLoad();
        FieldStartScriptDataStream();
        FieldFreeScriptDataStream();
        g_FieldFadeMode = nFadeMode;
        g_FieldFadeDuration = nFadeDuration;
        if (g_GameWdsBankLoadState == -1) {
            FieldStartWdsBankLoad(g_GameRequestedWdsBankIndex, 0);
        }
        FieldStartFadeFromWhite(g_FieldFadeDuration);
        break;
    case 2:
    case 4:
        ((void (*)(int, int))FieldZoomFadeEffectLoadTextures)(1, 1);
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        nFadeMode = g_FieldFadeMode;
        nFadeDuration = g_FieldFadeDuration;
        FieldLoad();
        FieldStartScriptDataStream();
        if (g_FieldScriptDataStreamActive == 1) {
            while (ArchiveDataSync() != 0) {
                FieldClearAndSwapOTag();
                FieldZoomFadeEffectUpdate();
                FieldDisplay();
                if (g_FieldZoomFadeScale < FIELD_ZOOM_FADE_SCALE_MAX) {
                    g_FieldZoomFadeScale += FIELD_ZOOM_FADE_SCALE_STEP;
                }
            }
            HeapFree(g_FieldScriptDataStream);
            g_FieldScriptDataStreamActive = 0;
            FieldOrVramStripPixels();
        }
        g_FieldActive = 1;
        g_FieldFadeMode = nFadeMode;
        g_FieldFadeDuration = nFadeDuration;
        if (g_GameWdsBankLoadState == -1) {
            FieldStartWdsBankLoad(g_GameRequestedWdsBankIndex, 0);
        }
        FieldFadeToBlack(g_FieldFadeDuration);
        nColor = FIELD_FADE_COLOR_MAX;
        for (i = 0; i < g_FieldFadeDuration; i++) {
            FieldUpdateFrame();
            FieldZoomFadeEffectUpdate();
            func_8007554C();
            FieldTickSkipCounter();
            FieldZoomFadeEffectSetPolygonColor(nColor >> 16);
            nColor -= FIELD_FADE_COLOR_MAX / g_FieldFadeDuration;
            if (nColor < 0) {
                nColor = 0;
            }
            if (g_FieldZoomFadeScale < FIELD_ZOOM_FADE_SCALE_MAX) {
                g_FieldZoomFadeScale += FIELD_ZOOM_FADE_SCALE_STEP;
            }
        }
        break;
    case 3:
        FieldZoomFadeEffectInitialize(0, 0);
        FieldStartScriptDataStream();
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        nFadeMode = g_FieldFadeMode;
        nFadeDuration = g_FieldFadeDuration;
        g_FieldActive = 1;
        FieldLoad();
        FieldFreeScriptDataStream();
        g_FieldFadeMode = nFadeMode;
        g_FieldFadeDuration = nFadeDuration;
        if (g_GameWdsBankLoadState == -1) {
            FieldStartWdsBankLoad(g_GameRequestedWdsBankIndex, 0);
        }
        for (i = 0; i < 4; i++) {
            FieldUpdateFrame();
            FieldZoomFadeEffectUpdate();
            func_8007554C();
            FieldTickSkipCounter();
        }
        break;
    case 5:
        FieldZoomFadeEffectInitialize(0, 0);
        FieldStartScriptDataStream();
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
        GamePartySyncSkinData();
        GamePartySyncStreamedData();
        nFadeMode = g_FieldFadeMode;
        nFadeDuration = g_FieldFadeDuration;
        g_FieldActive = 1;
        FieldLoad();
        FieldFreeScriptDataStream();
        rect.x = FIELD_SCREEN_VRAM_X;
        rect.y = FIELD_SCREEN_VRAM_Y;
        rect.w = 0x140;
        rect.h = 0xFF;
        g_FieldFadeMode = nFadeMode;
        g_FieldFadeDuration = nFadeDuration;
        MoveImage(&rect, 0x140, 0xFF);
        if (g_GameWdsBankLoadState == -1) {
            FieldStartWdsBankLoad(g_GameRequestedWdsBankIndex, 0);
        }
        for (i = 0; i < 4; i++) {
            FieldUpdateFrame();
            FieldZoomFadeEffectUpdate();
            func_8007554C();
            FieldTickSkipCounter();
        }
        break;
    default:
        break;
    }

    if (g_FieldFadeMode != 6) {
        FieldRestoreScreenImage();
    }
    g_FieldFadeMode = 2;
    g_FieldFadeDuration = 0x20;
    g_FieldActive = 0;
    FieldLoadFont();
    HeapConsolidate();
}

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
        pPoly->y2 = SCREEN_HEIGHT - 1;
        pPoly->x3 = FIELD_ZOOM_FADE_TILE_WIDTH + i * FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->y3 = SCREEN_HEIGHT - 1;

        setRECT(&g_FieldZoomFadeEffect.rects[i * 2], 0, 0, 0xFF, 0xFF);
        setRECT(&g_FieldZoomFadeEffect.rects[i * 2 + 1], 0, 0, 0xFF, 0xFF);

        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[i * 2], 0, 0,
            GetTPage(0x2, nABR, FIELD_SCREEN_VRAM_X + i * FIELD_ZOOM_FADE_TILE_WIDTH, FIELD_SCREEN_VRAM_Y), &g_FieldZoomFadeEffect.rects[i * 2]);
        SetDrawMode(&g_FieldZoomFadeEffect.drawModes[i * 2 + 1], 0, 0,
            GetTPage(0x2, nABR, FIELD_SCREEN_VRAM_X + i * FIELD_ZOOM_FADE_TILE_WIDTH, FIELD_SCREEN_VRAM_Y), &g_FieldZoomFadeEffect.rects[i * 2 + 1]);

        pPoly->r0 = 0x80;
        pPoly->g0 = 0x80;
        pPoly->b0 = 0x80;
        SetSemiTrans(pPoly, nSemiTrans);

        pPoly->u0 = 0;
        pPoly->v0 = 0;
        pPoly->u1 = FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->v1 = 0;
        pPoly->u2 = 0;
        pPoly->v2 = SCREEN_HEIGHT - 1;
        pPoly->u3 = FIELD_ZOOM_FADE_TILE_WIDTH;
        pPoly->v3 = SCREEN_HEIGHT - 1;
        pPoly->tpage = GetTPage(0x2, nABR, FIELD_SCREEN_VRAM_X + i * FIELD_ZOOM_FADE_TILE_WIDTH, FIELD_SCREEN_VRAM_Y);
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
            pPoly = (POLY_GT4*)((u_char*)g_FieldParticlePrimitives + g_FieldCurRenderContextIndex * FIELD_PARTICLE_POLY_COPY_SIZE) + (i * FIELD_PARTICLE_GRID_COLUMNS + j);
            FieldFadeParticleCornerColor(pPoly, 0, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk71C0 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            FieldFadeParticleCornerColor(pPoly, 1, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk73F0 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            FieldFadeParticleCornerColor(pPoly, 2, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk7620 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            FieldFadeParticleCornerColor(pPoly, 3, g_FieldParticleFadeDistance, ((FieldParticlePrimitiveSet*)g_FieldParticlePrimitives)->unk7850 + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
            addPrim(&g_FieldCurRenderContext->ot3[0],
                (POLY_GT4*)((u_char*)g_FieldParticlePrimitives + g_FieldCurRenderContextIndex * FIELD_PARTICLE_POLY_COPY_SIZE) + (i * FIELD_PARTICLE_GRID_COLUMNS + j));
        }
    }
    addPrim(&g_FieldCurRenderContext->ot3[0],
        &D_800B1E24[g_FieldCurRenderContextIndex * 0xC0]);
}


// PARKED 99.56% at retail's exact 500 bytes / 125 insns: the body is
// instruction-identical except for ONE caller-saved pair. Retail keeps j*16
// (truncated in place by the andi) in $v0 and the mask + 0x10 in $v1; ours mirrors
// them (mask in $v1, mask + 0x10 in $v0), 18 differing lines and nothing else.
//
// Round 452: an empty `do { } while (0);` between `nU = nX & 0x3F;` and
// `nU1 = nU + 0x10;` FLIPS the pair into retail's arrangement - 98.40%, 500 bytes,
// 125 insns, two differing lines - so the mirror is not an unreachable local-alloc
// quotient wall: it is what happens when the mask and the + 0x10 value share one
// basic block. Split off after the andi, the mask alone takes $v0 in the first
// block and the + 0x10 value falls to $v1 in the second, exactly as retail has it.
// The price is the other differing line: `move a2,s2`, the GetTPage division's
// dividend copy that sched2 hoists to the block head (right after SetPolyGT4's
// delay slot) in both retail and the unbarriered body, is then hoisted only into
// the second block, landing after the addiu. Measured placements: barrier before
// the andi keeps the pair mirrored and puts the copy after the andi (97.96%),
// barrier at the loop-body head alone changes nothing, and a second barrier at that
// head on top of the good one changes nothing either - the copy follows the LAST
// barrier. So the two differing lines trade against each other and no legitimate
// source statement puts a block boundary between the mask's definition and the
// + 0x10's; both are pinned by retail's own 125-instruction stream.
//
// local-alloc's own numbers for this body (-dl/-dg): the mask is 12 refs across 7
// insns, the mask + 0x10 is 9 across 6, and j*16 is coalesced into the mask; the
// sort key is deaths/refs, so 1/9 beats 1/12 and the + 0x10 value is processed
// first. The mask would have to fall to 8 refs (2 insns) or the + 0x10 value rise
// to 11 (4 insns), and retail's stream pins both counts at 3 insns each.
//
// Presets: 2.7.2-cdk 99.52, 2.6.0 99.56, -O1 75.83, -O3 does not compile, and a
// try_flags --sweep over 2.7.2 / 2.6.0 / 2.7.2-cdk x -O1/-O2/-O3 x -G4/-G8 x
// -mcpu 3000/r8000 finds nothing better, so this is not a compiler-version pair.
// Seven permuter runs on this body, base=55 every time (1314, 1971, 2018, 222,
// 1550, 1423, and round 452's third run below), i.e. no local edit reaches it.
//
// Spelling dead ends measured (99.56 and byte-identical unless noted): the mask
// before or after the x stores, mask and + 0x10 adjacent at retail's own position,
// a named temp at its first use, a 0x10 + nU operand swap, nU1 before nU,
// declaration order, nX reused for the mask or for the + 0x10 value, both inline
// forms, a second 0x10 local, an x1/x3 stepping counter in the for increment clause
// (97.60, moves the s4/s5 roles), nX written back as nX &= 0x3F (95.54 / 488 -
// folds three insns), a block-scope declaration with initialiser, u_char mask
// (94.91), one or both of nU/nU1 u_short (94.9), nIndex reused (94.2), grouping the
// u0/u2 stores above u1/u3 (96.53 - hands the mask $v0 but re-orders the sb
// stream), an u_int block-scope copy of nU1 (copy-propagated away, identical).
//
// Settled facts: 14 x 20 grid of 16x16 POLY_GT4 tiles, i outer / j inner,
// nIndex = i*20 + j, polygons is 560 POLY_GT4 (two 280 copies, the second at
// +0x38E0), the four u_short attribute arrays at 0x71C0/0x73F0/0x7620/0x7850 set
// to 0x80, y0/y1 = i*16 (s5), y2/y3 = i*16 + 0x10 (s3 counter), x1/x3 = j*16 + 0x10
// (s4 counter), u0/u2 = (j*16) & 0x3F, u1/u3 = that + 0x10, tpage =
// GetTPage(2, 1, j / 4 * 0x40 + 0x2C0, 0x100), SetSemiTrans, then
// *pPolyNext = *pPoly (0x34 bytes: a 3x lw/sw unrolled loop plus one word).
// best 99.56% (500 bytes = retail's) - see the note above the stub
/*
void func_800A6E70(void) {
    FieldParticlePrimitiveSet* pPrims;
    POLY_GT4* pPoly;
    POLY_GT4* pPolyNext;
    int i;
    int j;
    int nIndex;
    int nU;
    int nU1;
    int nX;

    g_FieldParticlePrimitives = HeapAlloc(sizeof(FieldParticlePrimitiveSet), 0x1);
    for (i = 0; i < FIELD_PARTICLE_GRID_ROWS; i++) {
        for (j = 0; j < FIELD_PARTICLE_GRID_COLUMNS; j++) {
            pPrims = g_FieldParticlePrimitives;
            nIndex = i * FIELD_PARTICLE_GRID_COLUMNS + j;
            pPoly = pPrims->polygons + nIndex;
            pPolyNext = pPrims->polygons + FIELD_PARTICLE_GRID_ROWS * FIELD_PARTICLE_GRID_COLUMNS + nIndex;
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
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A6E70);




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
    if (g_FieldMode == FIELD_MODE_STEP) {
        D_801D68B4 = 1;
    } else {
        D_801D68B4 = 0;
    }
    func_801D3538(SCREEN_WIDTH, SCREEN_HEIGHT, 0x80, 0x10, 0x20, 0x800, g_FieldStreamRgb24);
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

    if (g_FieldMode == FIELD_MODE_PLAIN && g_FieldStreamTextSegmentActive == 0) {
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
extern u16 g_FieldStreamImageX1;
extern u16 g_FieldStreamImageY1;
extern u16 g_FieldStreamImageX2;
extern u16 g_FieldStreamImageY2;
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
        ArchiveSetIndex(ARCHIVE_DIR_MOVIES, 1);
        nMode = 1;
        if (g_FieldStreamId != FIELD_STREAM_ID_NONE || (g_FieldStreamFlags & 0x40)) {
            nMode = 3;
        }
        func_801D37CC(g_FieldStreamFileIndex + 2, D_800C3A2A, g_FieldStreamFrameBase, g_FieldStreamTotalFrames, 1, nMode,
            g_FieldStreamLoopPlayback, g_FieldStreamImageX1, g_FieldStreamImageY1, g_FieldStreamImageX2, g_FieldStreamImageY2, SCREEN_HEIGHT, FieldSelectStreamRenderContext);
        ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
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

    if (g_FieldMode == FIELD_MODE_STEP) {
        HeapUnpinBlock(g_FieldSavedFramebuffers[0]);
        HeapUnpinBlock(D_8005A41C);
        HeapFree(g_FieldSavedFramebuffers[0]);
        HeapFree(D_8005A41C);
    } else {
        rect.x = FIELD_SAVED_FRAMEBUFFER_VRAM_X;
        rect.y = 0;
        rect.w = SCREEN_WIDTH;
        rect.h = FIELD_SAVED_FRAMEBUFFER_HEIGHT;
        LoadImage(&rect, g_FieldSavedFramebuffers[0]);
        DrawSync(0);
        rect.x = FIELD_SAVED_FRAMEBUFFER_VRAM_X;
        rect.y = FIELD_SAVED_FRAMEBUFFER_HEIGHT;
        rect.w = SCREEN_WIDTH;
        rect.h = FIELD_SAVED_FRAMEBUFFER_HEIGHT;
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
extern void ArchiveStartStreamQueue(StreamDataQueueEntry*, int, int);

void FieldSaveFramebuffers(void) {
    int i;
    int entryIndex;
    RECT rect;
    StreamDataQueueEntry queue[MAX_PARTY_MEMBERS + 1];
    void* pBuffer;

    if (g_FieldMode == FIELD_MODE_STEP) {
        g_FieldSavedFramebuffers[0] = HeapAlloc(FIELD_SAVED_FRAMEBUFFER_SIZE, 0);
        D_8005A41C = HeapAlloc(FIELD_SAVED_FRAMEBUFFER_SIZE, 0);
        HeapPinBlock(g_FieldSavedFramebuffers[0]);
        HeapPinBlock(D_8005A41C);
        ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);

        entryIndex = 0;
        for (i = 1; i < MAX_PARTY_MEMBERS; i++) {
            if (g_GamePartyMemberSkins[i] == CHARACTER_ID_NONE) {
                continue;
            }
            queue[entryIndex].archiveIndex = g_GamePartyMemberSkins[i] + FIELD_PARTY_STREAM_ARCHIVE_OFFSET;
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_GamePartyMemberSkins[i] + FIELD_PARTY_STREAM_ARCHIVE_OFFSET), 1);
            g_PartyStreamDataPointers[i] = pBuffer;
            queue[entryIndex].pData = pBuffer;
            entryIndex++;
        }

        queue[entryIndex].pData = 0;
        queue[entryIndex].archiveIndex = 0;
        ArchiveStartStreamQueue(queue, 0, 0);
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

        rect.x = FIELD_SAVED_FRAMEBUFFER_VRAM_X;
        rect.y = 0;
        rect.w = SCREEN_WIDTH;
        rect.h = FIELD_SAVED_FRAMEBUFFER_HEIGHT;
        StoreImage(&rect, g_FieldSavedFramebuffers[0]);
        DrawSync(0);

        rect.x = FIELD_SAVED_FRAMEBUFFER_VRAM_X;
        rect.y = FIELD_SAVED_FRAMEBUFFER_HEIGHT;
        rect.w = SCREEN_WIDTH;
        rect.h = FIELD_SAVED_FRAMEBUFFER_HEIGHT;
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

    pImage24Bit = HeapAlloc(FIELD_IMAGE_24BIT_STRIP_SIZE, 0x0);
    pImage15Bit = HeapAlloc(FIELD_IMAGE_15BIT_STRIP_SIZE, 0x0);
    
    for (i = 0; i < FIELD_IMAGE_STRIP_COUNT; i++) {
        rect.x = i * FIELD_IMAGE_STRIP_WIDTH;
        rect.y = 0;
        rect.w = FIELD_IMAGE_STRIP_WIDTH;
        rect.h = SCREEN_HEIGHT;
        StoreImage(&rect, pImage24Bit);
        DrawSync(0);
        g_Field24BitImageData = pImage24Bit;
        g_Field15BitImageData = pImage15Bit;
        g_FieldPixelIndex = 0;
        
        for (j = 0; j < FIELD_IMAGE_STRIP_WORDS; j++) {
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
        if (g_FieldStreamFramePosition < FIELD_TEXT_SEGMENT_DRAW_END_FRAME) {
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
    g_FieldRenderContexts[1].dispEnv.isinter = 0;
    g_FieldRenderContexts[0].dispEnv.isinter = 0;
    FieldFreeTextRender();
}

extern int ArchiveDecodeAlignedSize(int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ArchiveCdSeekOrPause(s32 sector);
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX, short clutY, short clutWidth, short clutHeight);

extern s32 g_FieldScriptCounter;
extern s32 g_FieldStreamReadyFlag;
extern s32 g_FieldSuppressStateChange;
extern s32 g_FieldScreenEffectSuppressed;
extern s32 g_FieldScriptFadeDuration;
extern s32 g_FieldScriptFadeMode;
extern s32 g_FieldActorModelSlotCount;
extern u_short g_FieldButtonsPressedOnce;
extern void* g_FieldArchiveQueueBuffer;
extern s32 g_FieldStreamDataSize;
extern s32 g_FieldScriptDataStreamActive;
extern s32 g_FieldScriptSuppressScreenEffect;
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

void FieldRunStreamedSequence(void) {
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
    ArchiveSetIndex(ARCHIVE_DIR_MOVIES, 0);
    ArchiveCdSeekOrPause(g_FieldStreamFileIndex);
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    FieldWaitForArchiveAndFrameSync();

    if (g_FieldActorModelSlotCount != 0) {
        LibGearShutdown();
        FieldRenderSyncAndFlush();
        FieldRenderSync();
        HeapFree(g_FieldArchiveQueueBuffer);
    }
    FieldParticlesFreeAll();

    if (g_FieldMode != FIELD_MODE_STEP) {
        rect.x = SCREEN_WIDTH;
        rect.y = 0;
        rect.w = FIELD_SCREEN_IMAGE_WIDTH;
        rect.h = FIELD_SCREEN_IMAGE_HEIGHT;
        LoadImage(&rect, pImage);
        DrawSync(0);
        HeapFree(pImage);
        pImage = HeapAlloc(FIELD_SCREEN_IMAGE_BUFFER_SIZE, 0);
        StoreImage(&rect, pImage);
    }

    nSuppressStateChange = g_FieldSuppressStateChange;
    nStreamSize = g_FieldStreamDataSize;
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
                    SoundSetCdVolumeWithFade(0, 10);
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

    if (g_FieldScriptSuppressScreenEffect != 0) {
        g_FieldScreenEffectSuppressed = 1;
    } else {
        g_FieldScreenEffectSuppressed = 0;
    }
    func_80077AB4();

    if (g_FieldMode != FIELD_MODE_STEP) {
        ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
        HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
        g_FieldScriptDataStreamActive = 0;
        FieldStartScriptDataStream();
        FieldFreeScriptDataStream();
        g_FieldScriptFadeDuration = 32;
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
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
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
extern s32 g_FieldSpriteAnimFrame;
extern s32 g_FieldSpritePaletteRow;

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

void FieldUpdateSpriteSheetPrims(void) {
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
        FIELD_ANGLE_MASK;
    nInterpAngle = g_CamInterpolation.curAngleY & FIELD_ANGLE_MASK;

    for (i = 0; i < 16; i++) {
        FieldSetSpriteUVs(i, (nInterpAngle >> 4) & 0xF, 0);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 29; i++) {
        FieldSetSpriteUVs(i, 0, (nCameraAngle >> 4) & 0xF);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 34; i++) {
        FieldSetSpriteUVs(i, (nInterpAngle & 0xF) << 3, 0);
        nInterpAngle >>= 2;
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 39; i++) {
        FieldSetSpriteUVs(i, (nCameraAngle & 0xF) << 3, 0);
        nCameraAngle >>= 2;
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 40; i++) {
        if ((g_FieldSpriteAnimFrame & 0xF) == 0) {
            g_FieldSpritePaletteRow++;
        }
        if (g_FieldSpritePaletteRow >= FIELD_SPRITE_PALETTE_ROW_COUNT) {
            g_FieldSpritePaletteRow = 0;
        }
        FieldSetSpriteUVs(i, 0, g_FieldSpritePaletteRow << 3);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 42; i++) {
        FieldSetSpriteUVs(i, (g_FieldSpriteAnimFrame >> 2) & 0xF, 0);
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    for (; i < 43; i++) {
        if (!(g_FieldSpriteAnimFrame & 0x10)) {
            addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
                &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
        }
    }
    for (; i < nPolyCount; i++) {
        addPrim(&g_FieldCurRenderContext->ot3[FIELD_SPRITE_POLY_OT_INDEX],
            &g_FieldSpritePolys[g_FieldCurRenderContextIndex][i]);
    }
    g_FieldSpriteAnimFrame++;
}

extern u_short D_800AEF16[];

// PARKED 85.21% - 784 bytes / 196 insns against retail's 772 / 194 (0x800A8EA8 - 0x800A8BA4).
// Round 440: the pRect pointer local WAS the untried lead and it is a win - 800 -> 784 bytes.
// With 'u_short *pRect = D_800AEB6C;' and nW = pRect[nRect], nH = pRect[nRect + 1],
// nSrcX = pRect[nRect - 2], nSrcY = D_800AEB6A[nRect], gcc keeps ONE rect base and folds the -4
// as a load displacement, which is retail's s5 = 0x800AEB6C with 'lhu a3,-4(t0)'; the old
// D_800AEB6C[nRect - 2] spelling made gcc fold -4 into a SECOND symbol (0x800AEB68) and pay its
// own lui/addiu. Keep this spelling.
// Residual 1 (biggest): loop.c reduces the nQuad * 4 mult giv into three 'addiu $ptr,$ptr,8'
// pointer counters, which eat the callee-saved registers the pRect base and the second quad base
// need (so pRect is re-formed inside the loop instead of hoisted). Retail keeps the raw index and
// recomputes 'sll $a1,$s2,3' on each side of the calls, adding each base at use time
// (addu v1,a1,s7 / addu v0,s7,a1 / addu a0,s7,a1 / addu a1,a1,s8). Every spelling in the old note
// reduced; lever g-qimode-index-blocks-the-mult-giv measured that an unreduced mult giv with no
// andi on the index and no sltiu test is not source-reachable, and retail's counter is a plain
// s32 (addiu s2,s2,1, slti s3,0x6D), so treat it as a declared wall.
// Residual 2: gcc sinks 3 of the 5 stack-argument stores of FieldClampPolyFT4UVs into each case
// body (retail's case tails are just the arg computations plus 'j' with the last addu in the slot,
// all five stores living in the cross-jumped block). The CALL argument list itself is already
// retail's: pPoly, nSrcX, nSrcY, nSrcX + nW, nSrcY, nSrcX, nSrcY + nH, nSrcX + nW, nSrcY + nH,
// per-case +/-1 variants verified instruction by instruction against the .s. Declaration order of
// the locals changes nothing (measured).
// Presets ruled out this round: gcc-2.6.0 85.46, gcc-2.7.2-cdk 83.45 (2.6.0 also equals Default's
// 96.58 on FieldInitializeParticlePrimitive next door, so this unit is not a 2.6.0 unit either).
// Permuter round 440: base=2995 best=2575 over 1302 iterations, and the only find is WRONG CODE -
// it sets 'nQuad = nW' inside case 2 and uses nQuad for case 3's corner sums, destroying the quad
// counter that the next iteration indexes with. Rejected, do not port it.
// Settled facts (from the earlier rounds): 0x6D POLY_FT4 prims (HeapAlloc(FIELD_SPRITE_POLY_COUNT *
// sizeof(POLY_FT4)) = 0x1108 twice); the quad table is a u_short array whose 8-byte records are
// x, y, rectIndex at D_800AEF16-6, -4, -2 and flags at D_800AEF16 (+0), flags >> 4 and 0xF = ABR,
// flags and 0xF = UV orientation (case 0 -> 1, case 1 -> 2, no default, nABR stays live across the
// loop in a callee-saved register); the rect table is a u_short array at 0x800AEB68 (x, y, w, h)
// indexed by rectIndex * 4 in u_short units; '*pPolyNext = *pPoly' is the last statement of the body
// and compiles to the 2x unrolled 0x28-byte copy loop both sides already have.
// Round 454: re-measured from a live un-park, same 85.21% / 784 / 196, and this body is still the
// best known. The quad table below is the SAME table the matched FieldSetSpriteUVs next door reads
// as g_FieldSpriteSheetQuads (config/symbol_addrs.field.txt: 0x800AEF14); D_800AEF16 is its +2, so
// the records are g_FieldSpriteSheetQuads[nQuad * 4 - 2 .. + 1] = x, y, rectIndex, flags and this
// body's base could be spelled with that name instead. Not measured: the rect-table round 440 note
// shows a negative index folds into a SECOND symbol and pays its own lui/addiu, and retail's s7 =
// fp - 6 says the original kept the offset, so re-measure before changing the spelling.
// Residual 1 is the whole diff: loop.c turns nQuad * 8 into THREE pointer givs ($17
// flags base, $18 base offset, $20 base-6, each 'addiu +8' in the latch) where retail recomputes
// 'sll v0,s2,3' at each use. Retail's nine callee-saved registers are ALL already live (s0 pPoly,
// s1 i * 0x28, s2 nQuad, s3 i, s4 nABR, s5 rect base, s6 pPolyNext, s7 quad base-6, fp quad base),
// so the original had no register to give a giv; the compile is pushed past nine and the whole
// allocation then moves. Residual 2 is an artefact of residual 1, not a second wall: retail's five
// FieldClampPolyFT4UVs stack stores live in ONE tail shared by four call sites (each case leaves its
// args in a1/a2/a3/v0 before the j, which is what jump.c merges), and the compile keeps three of
// them in case 0's own body. Do not attack residual 2 before residual 1.
/*
void func_800A8BA4(void) {
    POLY_FT4* pPoly;
    POLY_FT4* pPolyNext;
    u_short* pRect;
    int i;
    int nQuad;
    int nRect;
    int nX;
    int nY;
    int nW;
    int nH;
    int nSrcX;
    int nSrcY;
    int nABR;

    FieldLoadSpriteSheetTIM();
    nABR = 0;
    g_FieldSpritePaletteRow = 0;
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    g_FieldSpritePolys[0] = HeapAlloc(FIELD_SPRITE_POLY_COUNT * sizeof(POLY_FT4), 0);
    g_FieldSpritePolysCopy = HeapAlloc(FIELD_SPRITE_POLY_COUNT * sizeof(POLY_FT4), 0);

    nQuad = 0;
    pRect = D_800AEB6C;
    for (i = 0; i < FIELD_SPRITE_POLY_COUNT; i++) {
        pPoly = &g_FieldSpritePolys[0][i];
        pPolyNext = (POLY_FT4*)g_FieldSpritePolysCopy + i;
        SetPolyFT4(pPoly);

        pPoly->r0 = 0x80;
        pPoly->g0 = 0x80;
        pPoly->b0 = 0x80;
        pPoly->clut = GetClut(0, 0xE8);

        switch ((D_800AEF16[nQuad * 4] >> 4) & 0xF) {
        case 0:
            nABR = 1;
            break;
        case 1:
            nABR = 2;
            break;
        }
        pPoly->tpage = GetTPage(0, nABR, 0x380, 0);
        SetSemiTrans(pPoly, 1);

        nX = D_800AEF16[nQuad * 4 - 3];
        nY = D_800AEF16[nQuad * 4 - 2];
        nRect = D_800AEF16[nQuad * 4 - 1] * 4;
        nW = pRect[nRect];
        nH = pRect[nRect + 1];
        nSrcX = pRect[nRect - 2];
        nSrcY = D_800AEB6A[nRect];

        pPoly->x0 = nX;
        pPoly->y0 = nY;
        pPoly->x1 = nX + nW;
        pPoly->y1 = nY;
        pPoly->x2 = nX;
        pPoly->y2 = nY + nH;
        pPoly->x3 = nX + nW;
        pPoly->y3 = nY + nH;

        switch (D_800AEF16[nQuad * 4] & 0xF) {
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
        nQuad++;
    }
    g_FieldSpritePrimsAllocated = 1;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A8BA4);




// One rectangle (x0,y0,x1,y1) plus one texture rectangle (u0..v3) per shape.
extern ParticleShape D_800AF27C[];


// PARKED 96.58% - 512 bytes / 128 insns against retail's 520 / 130 (0x800A8EAC - 0x800A90B4).
// Round 453: re-measured live, unchanged, and the four-vs-six sll residual is now CLOSED as a
// cc1 cse gap rather than a source shape. Retail's eight vertex stores need four values (vx0 =
// x1s-x0s, vy0 = y1s-y0s, vx1 = x0s+x1s, vy1 = y0s+y1s, where xs = x<<4) and retail computes SIX
// sll for them: `sll a3,a3,4` (x1s) and `sll a1,a1,4` (y1s) are shared between the difference and
// the sum, but `sll a0,a1,4`/`sll v0,a2,4` (x0s, y0s) are recomputed later as `sll a1,a1,4`/
// `sll a2,a2,4` - in each `subu a0,a3,a0` the result takes the register of the SECOND operand, so
// that cse entry dies and the sum re-forms it. Every source spelling gives 4 sll instead, because
// our allocator puts the subu result in a third register and keeps x0s/y0s alive for the sum.
// Ten spellings measured with cc1_probe.sh (one run each, this unit's real cc1): the four int
// locals with the eight stores below, `u_short` locals, the four vx values then the four vy
// values, the shifted extremes in their own locals (sx0 = x0 << 4, ...), a `SVECTOR*` vertex
// pointer local, a `ParticleShape* pShape` local with inline field reads, no locals at all with the
// table read inline at all 16 sites, `x0 * 16 + x1 * 16` for the sums, and `(x0 << 4) - -(x1 << 4)`
// for the sums - all 4 sll, except the pointer-local form (sll after the subu) and the inline-read
// forms (16 lhu, the stores invalidate the load entries). A second tell that retail had one more
// live pseudo at the loads: its four lhu start at a1 with a0 left free, every spelling here starts
// at a0. -O1 73.08, -O3 also 4 sll, gcc-2.7.2-cdk 71.36, gcc-2.6.0 96.58 (identical to Default),
// so no preset recovers it either. Precedent: the companion fact of lever
// c-a-minus-chain-reassociates-the-subtract - an "extra instructions retail has" gap of one
// recompute per axis is a cse difference to park. Below 99% there is no permuter run.
// Frame: retail's 0x98 is 0x24 of o32 stack args plus ~0x5C of local storage nobody has explained;
// the `_pad[0x58]` below reproduces it, so the prologue and every stack offset match. No dlabel
// data in the stub (convertible.py: linkable), and nothing in this body depends on the pad.
/*
void FieldInitializeParticlePrimitive(ParticlePrimitive* pParticle, int nShape, int nTextureMode) {
    POLY_FT4* pPoly;
    POLY_FT4* pPolyNext;
    int x0;
    int y0;
    int x1;
    int y1;
    char _pad[0x58]; // unused, reproduces the original 0x98 frame

    pPoly = &pParticle->poly[0];
    pPolyNext = &pParticle->poly[1];
    SetPolyFT4(pPoly);

    x0 = D_800AF27C[nShape].x0;
    y0 = D_800AF27C[nShape].y0;
    x1 = D_800AF27C[nShape].x1;
    y1 = D_800AF27C[nShape].y1;

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
        D_800AF27C[nShape].u0, D_800AF27C[nShape].v0 + 0x40,
        D_800AF27C[nShape].u1 - 1, D_800AF27C[nShape].v1 + 0x40,
        D_800AF27C[nShape].u2, D_800AF27C[nShape].v2 + 0x3F,
        D_800AF27C[nShape].u3 - 1, D_800AF27C[nShape].v3 + 0x3F);

    SetSemiTrans(pPoly, 0x1);
    pPoly->tpage = GetTPage(0x0, nTextureMode, 0x3C0, 0x140);
    pPoly->clut = GetClut(0x100, 0xF7);

    *pPolyNext = *pPoly;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc5", FieldInitializeParticlePrimitive);


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
