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


/* Sony's vector.s. Each word of the matrix is a pair of shorts scaled by the
   vector's components and packed back with one store (andi/sll/or), with `multu`
   rather than `mult`, and the return copy is already made at 0x3A5F4 (hence the
   $2 pin, which is written mid-body) -- the shape ScaleMatrixL has, and objdiff
   scores 2.37% as nine short assignments. The last store covers m[2][2] and the
   struct's tail padding with one word, in the `jr` delay slot, so that one is
   left to C, where reorg fills the slot. */
MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v) {
    register MATRIX *pRet __asm__("$2");
    register long m22 __asm__("$9");

    __asm__ volatile (".set\tnoreorder\n\t"
                      "lw\t$8, 0($4)\n\t"
                      "lw\t$11, 0($5)\n\t"
                      "andi\t$9, $8, 65535\n\t"
                      "sll\t$9, $9, 16\n\t"
                      "sra\t$9, $9, 16\n\t"
                      "multu\t$9, $11\n\t"
                      "lw\t$12, 4($5)\n\t"
                      "sra\t$10, $8, 16\n\t"
                      "lw\t$13, 8($5)\n\t"
                      "lw\t$8, 4($4)\n\t"
                      "addu\t$2, $4, $0\n\t"
                      "mflo\t$9\n\t"
                      "sra\t$9, $9, 12\n\t"
                      "andi\t$9, $9, 65535\n\t"
                      "multu\t$10, $12\n\t"
                      "mflo\t$10\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "sll\t$10, $10, 16\n\t"
                      "or\t$9, $9, $10\n\t"
                      "sw\t$9, 0($4)\n\t"
                      "andi\t$9, $8, 65535\n\t"
                      "sll\t$9, $9, 16\n\t"
                      "sra\t$9, $9, 16\n\t"
                      "multu\t$9, $13\n\t"
                      "sra\t$10, $8, 16\n\t"
                      "lw\t$8, 8($4)\n\t"
                      "mflo\t$9\n\t"
                      "sra\t$9, $9, 12\n\t"
                      "andi\t$9, $9, 65535\n\t"
                      "multu\t$10, $11\n\t"
                      "mflo\t$10\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "sll\t$10, $10, 16\n\t"
                      "or\t$9, $9, $10\n\t"
                      "sw\t$9, 4($4)\n\t"
                      "andi\t$9, $8, 65535\n\t"
                      "sll\t$9, $9, 16\n\t"
                      "sra\t$9, $9, 16\n\t"
                      "multu\t$9, $12\n\t"
                      "sra\t$10, $8, 16\n\t"
                      "lw\t$8, 12($4)\n\t"
                      "mflo\t$9\n\t"
                      "sra\t$9, $9, 12\n\t"
                      "andi\t$9, $9, 65535\n\t"
                      "multu\t$10, $13\n\t"
                      "mflo\t$10\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "sll\t$10, $10, 16\n\t"
                      "or\t$9, $9, $10\n\t"
                      "sw\t$9, 8($4)\n\t"
                      "andi\t$9, $8, 65535\n\t"
                      "sll\t$9, $9, 16\n\t"
                      "sra\t$9, $9, 16\n\t"
                      "multu\t$9, $11\n\t"
                      "sra\t$10, $8, 16\n\t"
                      "lw\t$8, 16($4)\n\t"
                      "mflo\t$9\n\t"
                      "sra\t$9, $9, 12\n\t"
                      "andi\t$9, $9, 65535\n\t"
                      "multu\t$10, $12\n\t"
                      "mflo\t$10\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "sll\t$10, $10, 16\n\t"
                      "or\t$9, $9, $10\n\t"
                      "sw\t$9, 12($4)\n\t"
                      "andi\t$9, $8, 65535\n\t"
                      "sll\t$9, $9, 16\n\t"
                      "sra\t$9, $9, 16\n\t"
                      "multu\t$9, $13\n\t"
                      "mflo\t$9\n\t"
                      "sra\t$9, $9, 12\n\t"
                      ".set\treorder"
                      : "=r"(pRet), "=r"(m22) : : "$8", "$10", "$11", "$12",
                          "$13", "memory");
    *(long *)&m->m[2][2] = m22;
    return pRet;
}
