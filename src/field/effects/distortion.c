#include "common.h"
#include "system/memory.h"
#include "field/effects.h"

extern int D_800ADB24;
extern u_short D_800AEB24[30];

void FieldDistortionSetTarget(int t1, int t2, int t3, int t4, int t5, int t6, int duration);

void FieldDistortionFree(void) {
    g_FieldEffects.distortion.isActive = 0;
    if (D_800ADB24) {
        HeapFree(g_FieldEffects.distortion.buffer0);
        HeapFree(g_FieldEffects.distortion.buffer1);
        HeapFree(g_FieldEffects.distortion.buffer2);
        HeapFree(g_FieldEffects.distortion.buffer3);
        D_800ADB24 = 0;
    }
}

void FieldDistortionInitialize(int isResume) {
    int i;
    int j;
    int offset;
    RECT rect;

    g_FieldEffects.distortion.isActive = 1;

    if (D_800ADB24 == 0) {
        g_FieldEffects.distortion.buffer0 = HeapAlloc(0x180, 0);
        g_FieldEffects.distortion.buffer1 = HeapAlloc(0x180, 0);
        g_FieldEffects.distortion.buffer2 = HeapAlloc(0x3840, 0);
        g_FieldEffects.distortion.buffer3 = HeapAlloc(0x3840, 0);
        D_800ADB24 = 1;

        offset = 0;
        for (i = 0; i < 17; i++) {
            for (j = 0; j < 20; j++) {
                POLY_FT4* pPoly = &g_FieldEffects.distortion.buffer2[j + offset];
                POLY_FT4* pPoly2 = &g_FieldEffects.distortion.buffer3[j + offset];

                SetPolyFT4(pPoly);
                SetSemiTrans(pPoly, 0);
                pPoly->r0 = 0x80;
                pPoly->g0 = 0x80;
                pPoly->b0 = 0x80;
                pPoly->x0 = j * 16;
                pPoly->y0 = i * 16;
                pPoly->x1 = (j + 1) * 16;
                pPoly->y1 = i * 16;
                pPoly->x2 = j * 16;
                pPoly->y2 = i * 16 + 16;
                pPoly->x3 = (j + 1) * 16;
                pPoly->y3 = i * 16 + 16;

                if (i >= 14) {
                    pPoly->u0 = (j * 16) & 0x3F;
                    pPoly->v0 = (j >> 2) * 16 + (i - 14) * 80;
                    pPoly->u1 = ((j * 16) & 0x3F) + 16;
                    pPoly->v1 = (j >> 2) * 16 + (i - 14) * 80;
                    pPoly->u2 = (j * 16) & 0x3F;
                    pPoly->v2 = (j >> 2) * 16 + ((i - 14) * 80 + 16);
                    pPoly->u3 = ((j * 16) & 0x3F) + 16;
                    pPoly->v3 = (j >> 2) * 16 + ((i - 14) * 80 + 16);
                    pPoly->tpage = GetTPage(2, 0, 0x3C0, 0);
                    *pPoly2 = *pPoly;
                } else {
                    pPoly->u0 = (j * 16) & 0x3F;
                    pPoly->v0 = i * 16;
                    pPoly->u1 = ((j * 16) & 0x3F) + 16;
                    pPoly->v1 = i * 16;
                    pPoly->u2 = (j * 16) & 0x3F;
                    pPoly->v2 = i * 16 + 16;
                    pPoly->u3 = ((j * 16) & 0x3F) + 16;
                    pPoly->v3 = i * 16 + 16;
                    pPoly->tpage = GetTPage(2, 0, (j * 16) & 0xFFC0, 0);
                    *pPoly2 = *pPoly;
                    pPoly2->tpage = GetTPage(2, 0, (j * 16) & 0xFFC0, 0x100);
                }
            }
            offset += 20;
        }

        rect.x = 0;
        rect.y = 0x20;
        rect.w = 0x140;
        rect.h = 0xC0;
        SetDrawMove(g_FieldEffects.distortion.buffer0, &rect, 0, 0);
        rect.y = 0x120;
        SetDrawMove(g_FieldEffects.distortion.buffer1, &rect, 0, 0x100);
        rect.w = 0x40;
        rect.h = 0x10;
        for (i = 0; i < 15; i++) {
            rect.x = D_800AEB24[i * 2];
            rect.y = D_800AEB24[i * 2 + 1];
            SetDrawMove(&g_FieldEffects.distortion.buffer0[i + 1], &rect, 0x3C0, i * 16);
            rect.y = D_800AEB24[i * 2 + 1] + 0x100;
            SetDrawMove(&g_FieldEffects.distortion.buffer1[i + 1], &rect, 0x3C0, i * 16);
        }
    }

    if (isResume == 0) {
        FieldDistortionSetTarget(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3),
                                 FieldScriptVMGetArgument(5), FieldScriptVMGetArgument(7),
                                 FieldScriptVMGetArgument(9), FieldScriptVMGetArgument(11),
                                 FieldScriptVMGetArgument(13));
    }

    g_FieldEffects.distortion.isFinished = 0;
}

void FieldDistortionSetTarget(int t1, int t2, int t3, int t4, int t5, int t6, int duration) {
    if (duration == 0)
        duration = 1;
    
    g_FieldEffects.distortion.duration = duration;
    g_FieldEffects.distortion.delta1 = (t1 * 0x10000 - g_FieldEffects.distortion.v1) / duration;
    g_FieldEffects.distortion.delta2 = (t2 * 0x10000 - g_FieldEffects.distortion.v2) / duration;
    g_FieldEffects.distortion.delta3 = (t3 * 0x10000 - g_FieldEffects.distortion.v3) / duration;
    g_FieldEffects.distortion.delta4 = (t4 * 0x10000 - g_FieldEffects.distortion.v4) / duration;
    g_FieldEffects.distortion.delta5 = (t5 * 0x10000 - g_FieldEffects.distortion.v5) / duration;
    g_FieldEffects.distortion.delta6 = (t6 * 0x10000 - g_FieldEffects.distortion.v6) / duration;
}

INCLUDE_ASM("asm/field/nonmatchings/effects/distortion", FieldDistortionDraw);
