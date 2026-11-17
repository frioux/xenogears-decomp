#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// The tail of Psy-Q's libgte: TransMatrix on. It is its own translation unit
// only because ApplyMatrixSV (the function before TransMatrix) carries a pad word
// in retail -- the shipped .OBJ pads every function's tail to a 16-byte multiple
// -- and the four bytes at 0x3A598 come from the `asm` subsegment in
// config/slus_006.64.yaml. Keep it one unit's worth of code with psyq/libgte_tail.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_tail_d", TransMatrix);
/* best attempt 0.00%. The original keeps m in a0 (stores at 0x14/0x18/0x1c off
   a0), loads v->vx/vy/vz into t0-t2 before any store, and copies a0 into v0
   after the last store. Every C form tried (three separate temps, temps in
   declaration order, m copied to a local pointer, `return (MATRIX *)m`) makes
   gcc 2.7.2 coalesce the return value into v0 at entry and reuse $3-$5 as the
   temps:
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


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_tail_d", ScaleMatrix);
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


void SetRotMatrix(MATRIX *m) {
    gte_SetRotMatrix(m);
}

void SetLightMatrix(MATRIX *m) {
    gte_SetLightMatrix(m);
}

void SetColorMatrix(MATRIX *m) {
    gte_SetColorMatrix(m);
}

void SetTransMatrix(MATRIX *m) {
    gte_SetTransMatrix(m);
}
/* 96.25%. The original loads into t0/t1/t2, gcc into
   v0/v1/a1 (it prefers the lowest call-clobbered registers). Byte-for-byte
   identical apart from those three register numbers.
void SetTransMatrix(MATRIX *m) {
    gte_ldtr(m->t[0], m->t[1], m->t[2]);
}
*/


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

void SetRGBfifo(u_char *rgb0, u_char *rgb1, u_char *rgb2) {
    gte_ldrgb3_fifo(rgb0, rgb1, rgb2);
}

void SetIR123(long ir1, long ir2, long ir3) {
    gte_ldir123(ir1, ir2, ir3);
}

void SetIR0(long p) {
    gte_lddp(p);
}

void SetSZfifo3(long sz1, long sz2, long sz3) {
    gte_ldsz3(sz1, sz2, sz3);
}

void SetSZfifo4(long sz0, long sz1, long sz2, long sz3) {
    gte_ldsz4(sz0, sz1, sz2, sz3);
}

void SetSXSYfifo(long sxy0, long sxy1, long sxy2) {
    gte_ldsxy0(sxy0);
    gte_ldsxy1(sxy1);
    gte_ldsxy2(sxy2);
}

void SetRii(long r11, long r22, long r33) {
    gte_ldr11r22r33(r11, r22, r33);
}

void SetMAC123(long mac1, long mac2, long mac3) {
    gte_ldmac123(mac1, mac2, mac3);
}

void SetData32(long data) {
    gte_ldlzc(data);
}

void SetDQA(long dqa) {
    gte_lddqa(dqa);
}

void SetDQB(long dqb) {
    gte_lddqb(dqb);
}

void ReadGeomOffset(long *pOfsX, long *pOfsY) {
    gte_ReadGeomOffset_gpr(pOfsX, pOfsY);
}

