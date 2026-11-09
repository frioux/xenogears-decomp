#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", ReadGeomScreen);

void SetBackColor(long rbk, long gbk, long bbk) {
    gte_ldbkdir(rbk << 4, gbk << 4, bbk << 4);
}

void SetFarColor(long rfc, long gfc, long bfc) {
    gte_ldfcdir(rfc << 4, gfc << 4, bfc << 4);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", SetGeomOffset);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", SetGeomScreen);

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


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", NormalColor3);
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


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", NormalColorDpq);
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


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", NormalColorDpq3);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", NormalColorCol3);
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


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", ColorDpq);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", AverageSZ4);

void LightColor(VECTOR *v0, VECTOR *v1) {
    gte_ldlvl(v0);
    gte_nop();
    gte_lc_b();
    gte_stlvl(v1);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", DpqColorLight);
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


INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", DpqColor3);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", Lzc);

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotTransPers);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotTransPers3);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotTrans);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", NormalClip);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotTransPers4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotAverage4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotAverageNclip4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", TransposeMatrix);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotMatrixYXZ);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotMatrixZYX);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotMatrixX);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotMatrixY);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", RotMatrixZ);

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
