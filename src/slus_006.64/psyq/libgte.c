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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", func_8004901C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", func_800490A4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", LoadAverageByte);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", LoadAverageCol);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", CompMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyMatrixLV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyRotMatrix);
/* best attempt 86.67%. Everything matches except the return copy: the original
   ends `move v0,a2; jr $ra; nop` (the value comes from an unused THIRD
   argument, so its prototype was 3-arg), ours gets `jr $ra; move v0,a2` because
   reorg steals the copy into the delay slot. With the 2-arg prototype below the
   shape is `move v0,a1` and reorg fills the slot the same way (82.08%).
   Square0/Square12, whose copy source is a1, DO get the slot filled in retail,
   so the a2 return seems to be what stops retail's reorg; every source form
   tried (local copy, cast, statement order, $2-clobbering store macro) gives
   the same filled slot. Needs a compiler-level answer.
VECTOR *ApplyRotMatrix(SVECTOR *v0, VECTOR *v1) {
    gte_ldsv_gpr(v0);
    gte_nop();
    gte_mvmva_b(1, 0, 0, 3, 0);
    gte_stlvl_gpr(v1);
    return v1;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", PushMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", PopMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ScaleMatrixL);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", SetMulMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyRotMatrixLV);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", MulMatrix2);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ApplyMatrix);
/* best attempt 92.00%. gte_stlvnl_gpr (which lists $2 as a clobber) fixes the
   store register; one difference is left: the original ends
   `addu $v0,$a2,$zero; jr $ra; nop` and gcc's reorg steals the return-value
   copy into the jr delay slot. Same blocker as ApplyRotMatrix below, and the
   contrast there (Square0/Square12 return a1 and retail DOES fill their delay
   slot) says it is not a -f flag: retail's reorg left the a2 copy alone.
VECTOR *ApplyMatrix(MATRIX *m, SVECTOR *v0, VECTOR *v1) {
    gte_SetRotMatrix(m);
    gte_ldv0(v0);
    gte_nop();
    gte_mvmva_b(1, 0, 0, 3, 0);
    gte_stlvnl_gpr(v1);
    return v1;
}
*/

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ReadGeomScreen);

void SetBackColor(long rbk, long gbk, long bbk) {
    gte_ldbkdir(rbk << 4, gbk << 4, bbk << 4);
}

void SetFarColor(long rfc, long gfc, long bfc) {
    gte_ldfcdir(rfc << 4, gfc << 4, bfc << 4);
}

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

void DpqColor(CVECTOR *v0, long p, CVECTOR *v1) {
    gte_ldrgb(v0);
    gte_lddp(p);
    gte_nop();
    gte_dpcs_b();
    gte_strgb(v1);
}

void NormalColor(SVECTOR *v0, CVECTOR *v1) {
    gte_ldv0(v0);
    gte_nop();
    gte_ncs_b();
    gte_strgb(v1);
}


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColor3);
/* best attempt 72.67%. Only difference: gcc copies the two stack arguments
   (v4 at 0x10(sp), v5 at 0x14(sp)) into v-registers at function entry; the
   original loads them into t0/t1 after the NCT. cc1 2.7.2 (and 2.6.0) always
   materializes incoming stack arguments in the entry block, so this cannot be
   expressed in C. Needs the 6-argument form; blocked by the compiler.
void NormalColor3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4,
                  CVECTOR *v5) {
    CVECTOR *p4 = v4;
    CVECTOR *p5 = v5;

    gte_ldv3(v0, v1, v2);
    gte_nop();
    gte_nct_b();
    gte_strgb3(v3, p4, p5);
}
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColorDpq);
/* best attempt 88.89% (this form 77.78%). With gte_ldrgb before gte_lddp
   every instruction matches except one extra nop maspsx inserts between
   `lwc2 $6, 0($5)` and `mtc2 $6, $8`: it mistakes the GTE register number in
   the lwc2 for the general register read by the mtc2 (the depth argument is in
   $6 here). Same cause as ColorDpq/DpqColorLight below.
void NormalColorDpq(SVECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2) {
    gte_ldv0(v0);
    gte_lddp(p);
    gte_ldrgb(v1);
    gte_nop();
    gte_ncds_b();
    gte_strgb(v2);
}
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColorDpq3);
/* best attempt 53.50%. Same entry-block stack-argument copy as NormalColor3.
   Also note the original loads p straight into the GTE with
   `lwc2 $8, 0x10(sp)` where gte_lddp emits `mtc2 p, $8`, i.e. the original
   header's gte_lddp took a pointer.
void NormalColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, long p,
                     CVECTOR *v4, CVECTOR *v5, CVECTOR *v6) {
    gte_ldv3(v0, v1, v2);
    gte_ldrgb(v3);
    gte_lddp(p);
    gte_nop();
    gte_ncdt_b();
    gte_strgb3(v4, v5, v6);
}
*/


void NormalLightCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2) {
    gte_ldv0(v0);
    gte_ldrgb(v1);
    gte_nop();
    gte_nccs_b();
    gte_strgb(v2);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalColorCol3);
/* best attempt 64.12%. Same entry-block stack-argument copy as NormalColor3.
void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4,
                     CVECTOR *v5, CVECTOR *v6) {
    gte_ldv3(v0, v1, v2);
    gte_ldrgb(v3);
    gte_nop();
    gte_ncct_b();
    gte_strgb3(v4, v5, v6);
}
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", ColorDpq);
/* best attempt 90.00%. Everything matches except one extra nop that maspsx
   inserts between `lwc2 $6, 0($5)` and `mtc2 $6, $8` (its load-reuse check
   compares the lwc2 COP2 register number with the GPR the mtc2 reads; here the
   depth argument arrives in $6).
void ColorDpq(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2) {
    gte_ldlvl(v0);
    gte_ldrgb(v1);
    gte_lddp(p);
    gte_nop();
    gte_cdp_b();
    gte_strgb(v2);
}
*/


void ColorCol(VECTOR *v0, CVECTOR *v1, CVECTOR *v2) {
    gte_ldlvl(v0);
    gte_ldrgb(v1);
    gte_nop();
    gte_cc_b();
    gte_strgb(v2);
}

long AverageSZ3(void) {
    long otz;

    gte_avsz3_b();
    gte_stotz_val(otz);
    return otz;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", AverageSZ4);

void LightColor(VECTOR *v0, VECTOR *v1) {
    gte_ldlvl(v0);
    gte_nop();
    gte_lc_b();
    gte_stlvl(v1);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", DpqColorLight);
/* best attempt 90.00%. Same maspsx nop between `lwc2 $6, 0($5)` and
   `mtc2 $6, $8` as ColorDpq.
void DpqColorLight(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2) {
    gte_ldlvl(v0);
    gte_ldrgb(v1);
    gte_lddp(p);
    gte_nop();
    gte_dpcl_b();
    gte_strgb(v2);
}
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", DpqColor3);
/* best attempt 59.33% (identical under gcc 2.6.0/2.7.2, -O1/-O2/-O3). Same
   entry-block stack-argument copy as NormalColor3: gcc loads v3/v4/v5 (at
   0x10/0x14/0x18(sp)) into registers in the entry block, the original loads
   them after the DPCT. A `"memory"` clobber between DPCT and gte_strgb3 does
   not move the loads either, so it is a compiler invariant, not scheduling.
void DpqColor3(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2, long p, CVECTOR *v3,
               CVECTOR *v4, CVECTOR *v5) {
    gte_ldrgb3(v0, v1, v2);
    gte_lddp(p);
    gte_nop();
    gte_dpct_b();
    gte_strgb3(v3, v4, v5);
}
*/

void Intpl(VECTOR *v0, long p, CVECTOR *v1) {
    gte_ldlvl(v0);
    gte_lddp(p);
    gte_nop();
    gte_intpl_b();
    gte_strgb(v1);
}

VECTOR *Square12(VECTOR *v0, VECTOR *v1) {
    gte_ldlvl(v0);
    gte_nop();
    gte_sqr12_b();
    gte_stlvnl_gpr(v1);
    return v1;
}


VECTOR *Square0(VECTOR *v0, VECTOR *v1) {
    gte_ldlvl(v0);
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl_gpr(v1);
    return v1;
}


long AverageZ3(long sz0, long sz1, long sz2) {
    long otz;

    gte_ldsz3(sz0, sz1, sz2);
    gte_nop();
    gte_avsz3_b();
    gte_stotz_val(otz);
    return otz;
}

long AverageZ4(long sz0, long sz1, long sz2, long sz3) {
    long otz;

    gte_ldsz4(sz0, sz1, sz2, sz3);
    gte_nop();
    gte_avsz4_b();
    gte_stotz_val(otz);
    return otz;
}

void OuterProduct12(VECTOR *v0, VECTOR *v1, VECTOR *v2) {
    gte_stR11R22R33();
    gte_ldopv1(v0);
    gte_ldopv2(v1);
    gte_nop();
    gte_op12_b();
    gte_stlvnl(v2);
    gte_ldR11R22R33();
}

void OuterProduct0(VECTOR *v0, VECTOR *v1, VECTOR *v2) {
    gte_stR11R22R33();
    gte_ldopv1(v0);
    gte_ldopv2(v1);
    gte_nop();
    gte_op0_b();
    gte_stlvnl(v2);
    gte_ldR11R22R33();
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", Lzc);

void RotTransSV(SVECTOR *v0, SVECTOR *v1, long *flag) {
    long f;

    gte_ldv0(v0);
    gte_nop();
    gte_mvmva_b(1, 0, 0, 0, 0);
    gte_stsv(v1);
    gte_stflg_val(f);
    *flag = f;
}

SVECTOR *SquareSS12(SVECTOR *v0, SVECTOR *v1) {
    gte_ldsv(v0);
    gte_nop();
    gte_sqr12_b();
    gte_stsv(v1);
    return v1;
}

SVECTOR *SquareSS0(SVECTOR *v0, SVECTOR *v1) {
    gte_ldsv(v0);
    gte_nop();
    gte_sqr0_b();
    gte_stsv(v1);
    return v1;
}

VECTOR *SquareSL12(SVECTOR *v0, VECTOR *v1) {
    gte_ldsv(v0);
    gte_nop();
    gte_sqr12_b();
    gte_stlvl(v1);
    return v1;
}

VECTOR *SquareSL0(SVECTOR *v0, VECTOR *v1) {
    gte_ldsv(v0);
    gte_nop();
    gte_sqr0_b();
    gte_stlvl(v1);
    return v1;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTransPers);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTransPers3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTrans);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", NormalClip);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotTransPers4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotAverage4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotAverageNclip4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", TransposeMatrix);
/* best attempt 65.00%. Word-wise transpose (below) reproduces the original's
   store sequence exactly, byte for byte in the right ORDER with the right
   values; what is left is allocation. The original ends `lh t2,0x10(a0)`, keeps
   the store base in a1 (`move v0,a1` at entry, stores through a1, last store in
   the jr delay slot) and uses t1/t2/t3 for the words, reusing t1 for p[3].
   gcc 2.7.2 stores through v0 (the return copy), picks v1/a2 for the words and
   reuses a1 for p[3], and emits `lhu` for m22. a2/a3 look reserved in retail's
   allocation, which is the same oddity as ApplyRotMatrix returning an unused
   third argument -- worth checking whether these two stubs want the 2.6.0
   preset or a 3-arg-ish prototype before more C surgery. Both pointer locals
   and direct casts give the same 65.00%.
MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1) {
    long t0, t1, t2, t3;

    t0 = ((long *)m0)[0];
    t1 = ((long *)m0)[1];
    ((long *)m1)[1] = t0;
    ((long *)m1)[0] = t1;
    ((short *)m1)[0] = t0;
    t2 = ((long *)m0)[2];
    t3 = ((long *)m0)[3];
    ((long *)m1)[3] = t2;
    ((long *)m1)[2] = t3;
    ((short *)m1)[6] = t1;
    ((short *)m1)[4] = t2;
    ((short *)m1)[8] = ((short *)m0)[8];
    ((short *)m1)[2] = t3;
    return m1;
}
/* best attempt 65.00%. Structure and store order match: word stores q[1]=p[0],
   q[0]=p[1] patched with the low half of p[0], then q[3],q[2] patched from
   p[1].lo/p[2].lo/p[3].lo and the halfword at 0x10. What is left: gcc folds the
   `addu v0,a1,zero` return copy into every store (`sw ..,0(v0)`) where the
   original keeps storing through a1, allocates the temps as v1/a2 instead of
   t1/t2 (t1 reused for p[3]), and turns the 0x10 `lh` into `lhu`.
MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1) {
    MATRIX *ret = m1;
    long *p = (long *)m0;
    long *q = (long *)m1;
    short *s = (short *)m1;
    long t0, t1, t2, t3;

    t0 = p[0];
    t1 = p[1];
    q[1] = t0;
    q[0] = t1;
    s[0] = t0;
    t2 = p[2];
    t3 = p[3];
    q[3] = t2;
    q[2] = t3;
    s[6] = t1;
    s[4] = t2;
    s[8] = ((short *)m0)[8];
    s[2] = t3;
    return ret;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixYXZ);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixZYX);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixX);
/* best attempt 15.53%. The control flow below reproduces the original exactly
   (abs on r, the table lookup duplicated in both branches, s/c pulled out of one
   lw, the six 16-bit products) but gcc 2.7.2 allocates the table/dest registers
   as v0/at/v1/a3 instead of the original's t9/t8/t1/t0 and emits `mult` where the
   original has `multu`. Casting the products to u_int gives multu but spills 16
   bytes of frame, and no preset in a try_flags sweep beats 18.85%.
extern long D_800523F0[]; // one entry per 1/4096 turn, (cos << 16) | sin

MATRIX *RotMatrixX(long r, MATRIX *m) {
    long s, c;
    long i0, i1, i2, i3, i4, i5;

    if (r < 0) {
        r = -r;
        s = -(short)D_800523F0[r & 0xfff];
        c = D_800523F0[r & 0xfff] >> 16;
    } else {
        s = (short)D_800523F0[r & 0xfff];
        c = D_800523F0[r & 0xfff] >> 16;
    }
    i0 = m->m[1][0];
    i1 = m->m[1][1];
    i2 = m->m[1][2];
    i3 = m->m[2][0];
    i4 = m->m[2][1];
    i5 = m->m[2][2];
    m->m[1][0] = (i0 * c - i3 * s) >> 12;
    m->m[1][1] = (i1 * c - i4 * s) >> 12;
    m->m[1][2] = (i2 * c - i5 * s) >> 12;
    m->m[2][0] = (i0 * s + i3 * c) >> 12;
    m->m[2][1] = (i1 * s + i4 * c) >> 12;
    m->m[2][2] = (i2 * s + i5 * c) >> 12;
    return m;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixY);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte", RotMatrixZ);

extern short D_80057030[]; // atan table, one entry per table step

long ratan2(long y, long x) {
    long a;
    long c;
    long flag1 = 0;
    long flag2 = 0;

    if (x < 0) {
        flag1 = 1;
        x = -x;
    }
    if (y < 0) {
        flag2 = 1;
        y = -y;
    }
    if (x == 0 && y == 0) {
        return 0;
    }
    if (y < x) {
        if (y & 0x7fe00000) {
            a = y / (x >> 10);
        } else {
            a = (y << 10) / x;
        }
        c = D_80057030[a];
    } else {
        if (x & 0x7fe00000) {
            a = x / (y >> 10);
        } else {
            a = (x << 10) / y;
        }
        c = 0x400 - D_80057030[a];
    }
    if (flag1) {
        c = 0x800 - c;
    }
    if (flag2) {
        c = -c;
    }
    return c;
}
