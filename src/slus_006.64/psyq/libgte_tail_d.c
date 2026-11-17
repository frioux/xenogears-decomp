#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// TransMatrix and ScaleMatrix. It is its own translation unit only because both
// carry tail pad words in retail -- the shipped .OBJ pads every function's tail
// to a 16-byte multiple and the linker kept them -- and the twelve bytes at
// 0x3A6F0 are the `asm` subsegment in config/slus_006.64.yaml; 0x3A5C0, inside
// TransMatrix's stub, is TransMatrix's own. This file covers 0x3A59C-0x3A6F0.
// See psyq/libgte_tail for how the pads are handled and why TransMatrix stays
// asm.

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


