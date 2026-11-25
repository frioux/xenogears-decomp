/* Probe for func_800A8BA4 (field/main/misc5): does cc1 strength-reduce the
 * quad-table index nQuad * 4 (a biv product inside a memory address) into a
 * pointer giv, or recompute the sll like retail does?
 *
 *   tools/scripts/cc1_probe.sh field/main/misc5 tools/scripts/probe_misc5_giv.c
 *
 * Measured (round 495, this unit's real cc1 = gcc-2.7.2 -O2 -G8): BOTH bodies
 * reduce - ProbeGivFull gets three pointer givs (`addu $20,$20,8`, `$21`, `$23`)
 * where retail recomputes `sll $s2,3`, and ProbeGivFew, which uses the product
 * once, still gets one (`addu $17,$17,8` / `lhu $2,0($17)`). All nine
 * callee-saved registers are already used inside the loop in the first body, so
 * register pressure is not what stops it. ProbeGivFull's bound was also tried as
 * a constant (i < 0x6D, retail's count) with the same result. Hence a plain
 * `nQuad * 4` index cannot produce retail's body. */
#include "common.h"

extern u_short g_FieldSpriteSheetQuads[];
extern u_short g_FieldSpriteSheetRects[];
extern void ProbeCall(int* p);

void ProbeGivFull(int* pPoly, int* pOther, int nCount) {
    int i;
    int nQuad;
    int nABR;
    int nX;
    int nY;
    int nRect;
    int nW;
    int nH;

    nABR = 0;
    for (i = 0, nQuad = 0; i < 0x6D; i++) {
        ProbeCall(pPoly);
        switch ((g_FieldSpriteSheetQuads[nQuad * 4 + 1] >> 4) & 0xF) {
        case 0:
            nABR = 1;
            break;
        case 1:
            nABR = 2;
            break;
        }
        ProbeCall(pPoly);
        nX = g_FieldSpriteSheetQuads[nQuad * 4 - 2];
        nY = g_FieldSpriteSheetQuads[nQuad * 4 - 1];
        nRect = g_FieldSpriteSheetQuads[nQuad * 4] * 4;
        nW = g_FieldSpriteSheetRects[nRect];
        nH = g_FieldSpriteSheetRects[nRect + 1];
        switch (g_FieldSpriteSheetQuads[nQuad * 4 + 1] & 0xF) {
        case 0:
            ProbeCall(pOther);
            break;
        case 1:
            ProbeCall(pOther);
            break;
        }
        pPoly[0] = nX + nY + nRect + nW + nH + nABR;
        nQuad++;
    }
}

void ProbeGivFew(int* pPoly, int nCount) {
    int i;
    int nQuad;

    for (i = 0, nQuad = 0; i < nCount; i++) {
        ProbeCall(pPoly);
        pPoly[0] = g_FieldSpriteSheetQuads[nQuad * 4 + 1];
        nQuad++;
    }
}
