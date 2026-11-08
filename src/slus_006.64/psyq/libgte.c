#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", InitGeom);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SquareRoot0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", InvSquareRoot);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", VectorNormalS);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", VectorNormal);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", VectorNormalSS);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", func_80048DD8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", func_80048E94);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", LoadAverage12);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", LoadAverage0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", func_8004901C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", func_800490A4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", LoadAverageByte);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", LoadAverageCol);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", CompMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyMatrixLV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyRotMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", PushMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", PopMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ScaleMatrixL);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetMulMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyRotMatrixLV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix2);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyMatrixSV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", TransMatrix);
/* best attempt 0.00%: the original copies v->vx/vy/vz into three separate
   registers (t0-t2) before the stores and sets v0 = m afterwards; gcc 2.7.2
   reuses one temp and hoists `move v0,a0`:
MATRIX *TransMatrix(MATRIX *m, VECTOR *v) {
    long t0, t1, t2;

    t0 = v->vx;
    t1 = v->vy;
    t2 = v->vz;
    m->t[0] = t0;
    m->t[1] = t1;
    m->t[2] = t2;
    return m;
}
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ScaleMatrix);
/* best attempt 2.37% (with `>> 12`; `/ ONE` rounds and is worse). The original
   reads two adjacent shorts with one `lw` and merges the pair of stores into one
   `sw` (andi 0xffff / sll 16 / or), and multiplies with `multu`; the naive
   element-wise version gives lh/sh and `mult`:
MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v) {
    m->m[0][0] = (m->m[0][0] * v->vx) >> 12;
    m->m[0][1] = (m->m[0][1] * v->vy) >> 12;
    m->m[0][2] = (m->m[0][2] * v->vz) >> 12;
    m->m[1][0] = (m->m[1][0] * v->vx) >> 12;
    m->m[1][1] = (m->m[1][1] * v->vy) >> 12;
    m->m[1][2] = (m->m[1][2] * v->vz) >> 12;
    m->m[2][0] = (m->m[2][0] * v->vx) >> 12;
    m->m[2][1] = (m->m[2][1] * v->vy) >> 12;
    m->m[2][2] = (m->m[2][2] * v->vz) >> 12;
    return m;
}
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetRotMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetLightMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetColorMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetTransMatrix);

void SetVertex0(SVECTOR *v0) {
    gte_ldv0(v0);
}

void SetVertex1(SVECTOR *v0) {
    gte_ldv1(v0);
}

void SetVertex2(SVECTOR *v0) {
    gte_ldv2(v0);
}

void SetVertexTri(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2) {
    gte_ldv3(v0, v1, v2);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetRGBfifo);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetIR123);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetIR0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetSZfifo3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetSZfifo4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetSXSYfifo);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetRii);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetMAC123);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetData32);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetDQA);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetDQB);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ReadGeomOffset);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ReadGeomScreen);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetBackColor);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetFarColor);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetGeomOffset);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetGeomScreen);

// The libgte object in this binary was built with the one-nop INLINE_C.H
// revision; include/psyq/inline_c.h is 4.3's two-nop one, so the _b form plus
// an explicit gte_nop() is what the original emitted.
void LocalLight(SVECTOR *v0, VECTOR *v1) {
    gte_ldv0(v0);
    gte_nop();
    gte_ll_b();
    gte_stlvl(v1);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", DpqColor);

void NormalColor(SVECTOR *v0, CVECTOR *v1) {
    gte_ldv0(v0);
    gte_nop();
    gte_ncs_b();
    gte_strgb(v1);
}


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColor3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColorDpq);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColorDpq3);

void NormalLightCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2) {
    gte_ldv0(v0);
    gte_ldrgb(v1);
    gte_nop();
    gte_nccs_b();
    gte_strgb(v2);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColorCol3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ColorDpq);

void ColorCol(VECTOR *v0, CVECTOR *v1, CVECTOR *v2) {
    gte_ldlvl(v0);
    gte_ldrgb(v1);
    gte_nop();
    gte_cc_b();
    gte_strgb(v2);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", AverageSZ3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", AverageSZ4);

void LightColor(VECTOR *v0, VECTOR *v1) {
    gte_ldlvl(v0);
    gte_nop();
    gte_lc_b();
    gte_stlvl(v1);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", DpqColorLight);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", DpqColor3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", Intpl);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", Square12);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", Square0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", AverageZ3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", AverageZ4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", OuterProduct12);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", OuterProduct0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", Lzc);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTransSV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SquareSS12);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SquareSS0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SquareSL12);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SquareSL0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTransPers);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTransPers3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTrans);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalClip);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTransPers4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotAverage4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotAverageNclip4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", TransposeMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixYXZ);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixZYX);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixX);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixY);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixZ);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ratan2);
