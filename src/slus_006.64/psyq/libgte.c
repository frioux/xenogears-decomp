#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", InitGeom);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SquareRoot0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", InvSquareRoot);
/* best attempt 56.69%. The C below is structurally the right function (LZCS/LZCR,
   two early-outs, table lookup, the two stores), but retail's instruction list
   carries a dead `andi t0,v0,0x1` and keeps every constant in a t-register where
   gcc 2.7.2 uses v0 (li v0,-2 / li v0,31), and it has its own branch layout, so
   this is Sony's handwritten vector.s rather than compiler output:
long InvSquareRoot(long a, long *b, long *c) {
    long flg, even, sh, t;

    gte_ldlzc(a);
    gte_nop();
    gte_nop();
    gte_stflg_val_gpr(flg);
    if (flg == 0x20 || flg == 0) {
        return -1;
    }
    even = flg & ~1;
    sh = (0x1F - even) >> 1;
    if (even >= 0x18) {
        t = a << (even - 0x18);
    } else {
        t = a >> (0x18 - even);
    }
    *c = sh;
    *b = D_80056B94[t - 0x40];
    return 1;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", VectorNormalS);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", VectorNormal);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", VectorNormalSS);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", func_80048DD8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MatrixNormal);

void LoadAverage12(VECTOR *v0, VECTOR *v1, long p0, long p1, VECTOR *v2) {
    long flg;

    gte_lddp(p0);
    gte_ldlvl(v0);
    gte_nop();
    gte_gpf12_b();
    gte_stflg_val_gpr(flg);
    gte_lddp(p1);
    gte_ldlvl(v1);
    gte_nop();
    gte_gpl12_b();
    gte_stlvl_gpr_stack(v2);
}

void LoadAverage0(VECTOR *v0, VECTOR *v1, long p0, long p1, VECTOR *v2) {
    long flg;

    gte_lddp(p0);
    gte_ldlvl(v0);
    gte_nop();
    gte_gpf0_b();
    gte_stflg_val_gpr(flg);
    gte_lddp(p1);
    gte_ldlvl(v1);
    gte_nop();
    gte_gpl0_b();
    gte_stlvl_gpr_stack(v2);
}

void func_8004901C(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2) {
    long flg;

    gte_ldv3_gpr(v0);
    gte_lddp_v3_gpr(p0);
    gte_nop();
    gte_gpf12_b();
    gte_ldv3_gpr(v1);
    gte_stflg_val_gpr(flg);
    gte_lddp_v3_gpr(p1);
    gte_nop();
    gte_gpl12_b();
    gte_stsv_gpr_stack(v2);
}


void func_800490A4(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2) {
    long flg;

    gte_ldv3_gpr(v0);
    gte_lddp_v3_gpr(p0);
    gte_nop();
    gte_gpf0_b();
    gte_ldv3_gpr(v1);
    gte_stflg_val_gpr(flg);
    gte_lddp_v3_gpr(p1);
    gte_nop();
    gte_gpl0_b();
    gte_stsv_gpr_stack(v2);
}


/* The 3.x revision loaded and stored the byte pair through $8/$9, split into
   three blocks so a flag read can sit between the load and the mtc2 chain. */
#define gte_ldbv_gpr_lo( r0 ) __asm__ volatile (		\
	"lbu\t$8, 0( %0 );"					\
	"lbu\t$9, 1( %0 )"					\
	:							\
	: "r"( r0 )						\
	: "$8", "$9" )

#define gte_ldbv_gpr_hi( ) __asm__ volatile (			\
	"mtc2\t$8, $9;"						\
	"mtc2\t$9, $10;"					\
	"addiu\t$11, $0, 12"					\
	:							\
	:							\
	: "$8", "$9", "$11" )

#define gte_stbv_gpr_stack( r0 ) __asm__ volatile (		\
	"lw\t$13, %0;"						\
	"mfc2\t$8, $25;"					\
	"mfc2\t$9, $26;"					\
	"srav\t$8, $8, $11;"					\
	"srav\t$9, $9, $11;"					\
	"sb\t$8, 0( $13 );"					\
	"sb\t$9, 1( $13 )"					\
	:							\
	: "m"( r0 )						\
	: "$8", "$9", "$11", "$13", "memory" )

void LoadAverageByte(u_char *v0, u_char *v1, long p0, long p1, u_char *v2) {
    long flg;

    __asm__ volatile ( "lbu\t$8, 0( %0 );" "lbu\t$9, 1( %0 );" "mtc2\t%1, $8;"
                       "mtc2\t$8, $9;" "mtc2\t$9, $10"
                       : : "r"(v0), "r"(p0) : "$8", "$9" );
    gte_nop();
    gte_gpf0_b();
    gte_ldbv_gpr_lo(v1);
    gte_stflg_val_gpr(flg);
    gte_lddp(p1);
    gte_ldbv_gpr_hi();
    gte_gpl0_b();
    gte_stbv_gpr_stack(v2);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", LoadAverageCol);

// MulMatrix0/MulMatrix/MulMatrix2/ScaleMatrixL/ApplyRotMatrixLV/LoadAverageCol/
// SquareRoot0 own instructions after `.size` (trailing nops no C definition
// emits), so they are not linkable from C: see tools/scripts/convertible.py.
// ApplyMatrixLV/CompMatrix/SetMulMatrix are linkable but their .s holds
// unreachable instructions (e.g. the `sra $t3,$t0,15` at 0x800494D0 in
// ApplyMatrixLV's 16-bit packing), which a C compiler cannot produce.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", CompMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyMatrixLV);

VECTOR *ApplyRotMatrix(SVECTOR *v0, VECTOR *v1) {
    VECTOR *pRet;

    gte_ldsv_gpr(v0);
    gte_nop();
    gte_mvmva_b(1, 0, 0, 3, 0);
    __asm__ volatile ("swc2\t$9, 0( %1 );" "swc2\t$10, 4( %1 );" "swc2\t$11, 8( %1 );"
                      "addu\t%0, $6, $zero"
                      : "=r"(pRet)
                      : "r"(v1)
                      : "memory");
    return pRet;
}
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", PushMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", PopMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ScaleMatrixL);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetMulMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyRotMatrixLV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix2);

VECTOR *ApplyMatrix(MATRIX *m, SVECTOR *v0, VECTOR *v1) {
    VECTOR *pRet;

    gte_SetRotMatrix(m);
    gte_ldv0(v0);
    gte_nop();
    gte_mvmva_b(1, 0, 0, 3, 0);
    __asm__ volatile ("swc2\t$25, 0( %1 );" "swc2\t$26, 4( %1 );" "swc2\t$27, 8( %1 );"
                      "addu\t%0, %1, $zero"
                      : "=r"(pRet)
                      : "r"(v1)
                      : "memory");
    return pRet;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyMatrixSV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", TransMatrix);
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

