#include "common.h"
#include "system/memory.h"
#include "field/effects.h"
#include "field/main.h"

extern int D_800ADB24;
extern u_short D_800AEB24[30];

/* buffer0/buffer1 and buffer2/buffer3 are the two render-context pages of the
   same pair of buffers, selected with the current context index. */
#define DISTORTION_MOVE_BUFFER() \
    (((DR_MOVE**)&g_FieldEffects.distortion.buffer0)[g_FieldCurRenderContextIndex])
#define DISTORTION_PRIM_BUFFER() \
    (((POLY_FT4**)&g_FieldEffects.distortion.buffer2)[g_FieldCurRenderContextIndex])

#define DISTORTION_GRID_ROWS 20 /* prims per grid row */

typedef struct {
    /* 0x0 */ u_long tag;
    /* 0x4 */ u_char unk4[0xBC];
} DistortionPacket; // Size: 0xC0

extern DistortionPacket g_FieldOtagPacket[2];

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

/* Best attempt: 84.72%, 2020 bytes against retail's 2060 (515 insns). The control
   flow and both grid loops are 1:1 with retail now; what is left is register
   allocation (retail keeps col/idx/offsetX/offsetY/phase2 in s1/s2/s3/s6/s5 and
   pPrim in s0, the compile uses s0/s1/s2/s8/s3 and a2) plus 10 instructions that
   are NOT in the store groups - see round 496 below for where they are.

   Facts established against the assembly, worth keeping:
   - The advance block is ONE copy of the seven statements. retail's `bnez
     isFinished` leaves the not-finished arm as fallthrough, its duration test
     branches past the update, and the finished arm branches BACK into it, so the
     source cannot be the plain if/else-if/else (cc1 emits two copies and only
     cross-jumps 2-3 insns here). A label on the update plus `goto` from the
     finished arm reproduces it.
   - The six `(s16)(vN >> 16)` reads come out as `lh 0xA/0xE/0x12/0x16/0x1A/0x1E`
     only if the local is s32 and the cast truncates; first-use order in retail is
     v1, v6, unk3C, v5, v2, v3, v4 (so the source assigns them in that order).
   - phase1 = (s16)angleY + angle4 * 11 - the *11 uses v4's high half, not v6's.
   - Each group of 2-4 prim stores needs its own pointer local (a fresh
     `&PRIM_BUF()[...]`); written as repeated array subscripts cc1 reloads the
     buffer pointer before EVERY store (4 loads per group instead of 1).

   Round 496 measured the rest of the shape. Retail does NOT address the two
   row-clamp arms through pPrim: loop 1's row==16 arm writes y3/y2 through
   `li s7,0x3200` + stride 40 (i.e. `&PRIM_BUF[320 + col]`, a CONSTANT base, not
   row*20+col) and loop 2's row==13 arm writes all six y stores through
   `li s7,0x28A0` (`&PRIM_BUF[260 + col]`), the same base again for the y3/y2 pair
   after its second rsin. Writing those as `DISTORTION_GRID_ROWS * 16 + col` and
   `DISTORTION_GRID_ROWS * 13 + col` measures 83.57% / 2040 bytes / 510 insns
   against retail's 2060 / 515 (this park's `row * DISTORTION_GRID_ROWS + col`
   stays at 84.72 / 2020 / 505, so the % prefers the inline form and the byte
   count prefers the constant base). With those two arms plus the item below the
   body reaches 511 of retail's 515 insns (505 -> 510 -> 511); the last ~4 are
   retail's extra 0xFFFFFF reloads and the col*40 recompute noted below, none of
   them is in the store groups.

   - The last-column index is a VALUE in retail, not an address: a named
     `s32 lastIndex = row * DISTORTION_GRID_ROWS + DISTORTION_GRID_ROWS - 1;`
     assigned in each outer body reproduces retail's separate index IV exactly
     (init 0x12B = 299 then stride 20, spilled to 0x60, and the sll/addu/sll
     *40 done at each use). Block-scoping the declaration loses the IV again;
     with it in place the body is 511 insns but the % DROPS to 81.05 because
     the IV takes the callee-saved register retail gives offsetY.

   - The residual is ONE callee-saved membership swap, not a rotation. Retail's
     loop-1 map is s0 pPrim, s1 col, s2 idx, s3 offsetX, s4 row, s5 phase2,
     s6 offsetY, s7 the 320-index IV, s8(fp) phase1, with lastIndex, row*800,
     row*20 and the 0xFFFFFF mask all in STACK slots; the compile's set is the
     same minus offsetY (it goes to a caller-saved temp plus a stack slot,
     `sw a2,104(sp)` in the rsin delay slot) plus whatever took s6/s8. The same
     swap in loop 2. Retail also REMATERIALISES 0xFF000000 (`lui t0` inside the
     inner loop) and reloads the 0xFFFFFF mask from 0x74 per iteration.

   - Loop 2's row==0 arm is `&PRIM_BUF[col]` and retail recomputes `col*40`
     inline there (sll/addu/sll + addu) while every spelling here strength-
     reduces it to a col*40 IV (`move s,zero` per outer row, `addiu s,40` per
     col) - that IV is the register offsetY loses its home to. Writing the arm
     as `pTmp = pPrim` does remove the IV but measures 80.74% / 2020 bytes.
   - The remaining byte-offset-vs-index difference: retail's col==19 arms keep
     `row*20+19` as an INDEX (outer stride 20, `sll/addu/sll` at the use).
     SUPERSEDED in round 496 - see the `lastIndex` item above, which reproduces
     retail's index IV; the inline `row * DISTORTION_GRID_ROWS + 19` form this
     note used to blame is simply a strength-reduced (byte-offset) spelling of
     the same address. Writing the two stores as inline subscripts instead drops
     that IV but adds a reload per store (79.98% / 1988 bytes).

   ROUND 532: the preset axis is closed. try_flags.sh on this body: Default
   (2.7.2 -O2 -G8) 84.72, gcc-2.6.0 82.09, gcc-2.7.2-cdk 75.87, so the
   allocation is not a compiler-version question. Also re-measured: folding the
   loop-1 y-block's re-formed `pTmp = &DISTORTION_PRIM_BUFFER()[row * 20 + col]`
   into `pPrim[-DISTORTION_GRID_ROWS]` measures 81.12% / 1952 bytes / 488 insns
   (loses 17 insns), so the re-form is load-bearing. Read the residual as
   retail's SPILL SET, not a swap: retail keeps the 0xFFFFFF addPrim mask, the
   `lastIndex` index and `row*800` in STACK slots (0x74/0x70/0x78) and reloads
   them after each rsin, where this body gives the mask a callee-saved home
   (s8) - the 10 missing instructions are that spill traffic plus retail's own
   col*40 recompute, and the spill set follows from the IV set, which is the
   `lastIndex` item above. Parked deliberately; the next attempt should aim at
   the IV set in BOTH loops, not at a register spelling.
*/
/*
void FieldDistortionDraw(void) {
    s32 angle1;
    s32 angle2;
    s32 angle3;
    s32 angle4;
    s32 angle5;
    s32 angle6;
    s32 angleY;
    s32 phase1;
    s32 phase2;
    s32 offsetX;
    s32 offsetY;
    s32 row;
    s32 col;
    s32 i;
    s32 x;
    s32 y;
    POLY_FT4* pPrim;
    POLY_FT4* pTmp;

    if (g_FieldEffects.distortion.isActive == 0) {
        return;
    }

    if (g_FieldEffects.distortion.isFinished == 0) {
        if (g_FieldEffects.distortion.duration > 0) {
advance:
            g_FieldEffects.distortion.v1 += g_FieldEffects.distortion.delta1;
            g_FieldEffects.distortion.v2 += g_FieldEffects.distortion.delta2;
            g_FieldEffects.distortion.duration--;
            g_FieldEffects.distortion.v3 += g_FieldEffects.distortion.delta3;
            g_FieldEffects.distortion.v4 += g_FieldEffects.distortion.delta4;
            g_FieldEffects.distortion.v5 += g_FieldEffects.distortion.delta5;
            g_FieldEffects.distortion.v6 += g_FieldEffects.distortion.delta6;
        }
    } else if (g_FieldEffects.distortion.duration > 0) {
        goto advance;
    } else {
        g_FieldEffects.distortion.delta6 = 0;
        g_FieldEffects.distortion.delta5 = 0;
        g_FieldEffects.distortion.delta4 = 0;
        g_FieldEffects.distortion.delta3 = 0;
        g_FieldEffects.distortion.delta2 = 0;
        g_FieldEffects.distortion.delta1 = 0;
        g_FieldEffects.distortion.isActive = 0;
    }

    angle1 = (s16)(g_FieldEffects.distortion.v1 >> 16);
    angle6 = (s16)(g_FieldEffects.distortion.v6 >> 16);
    angleY = g_FieldEffects.distortion.unk3C + angle6;
    angle5 = (s16)(g_FieldEffects.distortion.v5 >> 16);
    angle2 = (s16)(g_FieldEffects.distortion.v2 >> 16);
    angle3 = (s16)(g_FieldEffects.distortion.v3 >> 16);
    angle4 = (s16)(g_FieldEffects.distortion.v4 >> 16);
    phase1 = (s16)angleY + angle4 * 11;
    g_FieldEffects.distortion.unk38 += angle5;
    g_FieldEffects.distortion.unk3C = angleY;

    for (row = 14; row < 17; row++) {
        offsetY = (rsin(phase1) * angle2) >> 12;
        phase1 += angle4;
        phase2 = g_FieldEffects.distortion.unk38;

        for (col = 0; col < DISTORTION_GRID_ROWS; col++) {
            offsetX = (rsin(phase2) * angle1) >> 12;
            phase2 += angle3;

            pPrim = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + col];
            if (col != 0) {
                x = (col << 4) + offsetX;
                pPrim[-1].x3 = x;
                pPrim[-1].x1 = x;
                pPrim->x2 = x;
                pPrim->x0 = x;
                if (col == DISTORTION_GRID_ROWS - 1) {
                    pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + 19];
                    pTmp->x3 = offsetX + 0x140;
                    pTmp->x1 = offsetX + 0x140;
                }
            } else {
                pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS];
                pTmp->x2 = 0;
                pTmp->x0 = 0;
            }

            y = (row << 4) + offsetY - 0x30;
            pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + col];
            pTmp[-DISTORTION_GRID_ROWS].y3 = y;
            pTmp[-DISTORTION_GRID_ROWS].y2 = y;
            pTmp->y1 = y;
            pTmp->y0 = y;
            if (row == 16) {
                pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + col];
                pTmp->y3 = 0xE0;
                pTmp->y2 = 0xE0;
            }

            addPrim(&g_FieldCurRenderContext->ot1[1], pPrim);
        }
    }

    addPrim(&g_FieldCurRenderContext->ot1[1], DISTORTION_MOVE_BUFFER());

    phase1 = g_FieldEffects.distortion.unk3C;

    for (row = 0; row < 14; row++) {
        offsetY = (rsin(phase1) * angle2) >> 12;
        phase1 += angle4;
        phase2 = g_FieldEffects.distortion.unk38;

        for (col = 0; col < DISTORTION_GRID_ROWS; col++) {
            offsetX = (rsin(phase2) * angle1) >> 12;
            phase2 += angle3;

            pPrim = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + col];
            if (row != 0) {
                if (row == 13) {
                    y = offsetY + 0xF0;
                    pPrim->y1 = y;
                    pPrim->y0 = y;
                    pPrim[-DISTORTION_GRID_ROWS].y3 = y;
                    pPrim[-DISTORTION_GRID_ROWS].y2 = y;
                    y = ((rsin(phase1) * angle2) >> 12) + 0xF0;
                    pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + col];
                    pTmp->y3 = y;
                    pTmp->y2 = y;
                } else {
                    y = (row << 4) + offsetY + 0x20;
                    pPrim[-DISTORTION_GRID_ROWS].y3 = y;
                    pPrim[-DISTORTION_GRID_ROWS].y2 = y;
                    pPrim->y1 = y;
                    pPrim->y0 = y;
                }
            } else {
                pTmp = &DISTORTION_PRIM_BUFFER()[col];
                pTmp->y1 = 0x20;
                pTmp->y0 = 0x20;
            }

            if (col != 0) {
                x = (col << 4) + offsetX;
                pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + col];
                pTmp[-1].x3 = x;
                pTmp[-1].x1 = x;
                pTmp->x2 = x;
                pTmp->x0 = x;
                if (col == DISTORTION_GRID_ROWS - 1) {
                    pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS + 19];
                    pTmp->x3 = 0x140;
                    pTmp->x1 = 0x140;
                }
            } else {
                pTmp = &DISTORTION_PRIM_BUFFER()[row * DISTORTION_GRID_ROWS];
                pTmp->x2 = 0;
                pTmp->x0 = 0;
            }

            addPrim(&g_FieldCurRenderContext->ot1[1], pPrim);
        }
    }

    for (i = 0; i < 15; i++) {
        addPrim(&g_FieldCurRenderContext->ot1[1], &DISTORTION_MOVE_BUFFER()[i + 1]);
    }

    addPrim(&g_FieldCurRenderContext->ot1[1], &g_FieldOtagPacket[g_FieldCurRenderContextIndex]);
}
*/
