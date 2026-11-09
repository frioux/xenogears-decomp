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


void NormalColor3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4,
                  CVECTOR *v5) {
    gte_ldv3(v0, v1, v2);
    gte_nop();
    gte_nct_b();
    gte_strgb3_gpr_stack(v3, v4, v5);
}


void NormalColorDpq(SVECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2) {
    gte_ldv0(v0);
    gte_ldrgb(v1);
    gte_lddp_a2(p);
    gte_nop();
    gte_ncds_b();
    gte_strgb(v2);
}


void NormalColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, long p,
                     CVECTOR *v4, CVECTOR *v5, CVECTOR *v6) {
    gte_ldv3(v0, v1, v2);
    gte_ldrgb(v3);
    gte_lddp_stack(p);
    gte_nop();
    gte_ncdt_b();
    gte_strgb3_stack(v4, v5, v6);
}


void NormalLightCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2) {
    gte_ldv0(v0);
    gte_ldrgb(v1);
    gte_nop();
    gte_nccs_b();
    gte_strgb(v2);
}

void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4,
                     CVECTOR *v5, CVECTOR *v6) {
    gte_ldv3(v0, v1, v2);
    gte_ldrgb(v3);
    gte_nop();
    gte_ncct_b();
    gte_strgb3_stack(v4, v5, v6);
}


void ColorDpq(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2) {
    gte_ldlvl(v0);
    gte_ldrgb(v1);
    gte_lddp_a2(p);
    gte_nop();
    gte_cdp_b();
    gte_strgb(v2);
}


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

void DpqColorLight(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2) {
    gte_ldlvl(v0);
    gte_ldrgb(v1);
    gte_lddp_a2(p);
    gte_nop();
    gte_dpcl_b();
    gte_strgb(v2);
}


void DpqColor3(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2, long p, CVECTOR *v3,
               CVECTOR *v4, CVECTOR *v5) {
    gte_ldrgb3(v0, v1, v2);
    gte_lddp(p);
    gte_nop();
    gte_dpct_b();
    gte_strgb3_stack(v3, v4, v5);
}

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
/* best attempt 59.52% (180 bytes, the original's size: no frame, no extra insn).
   Everything structural is right with the *_val_stack macros in inline_c.h (each
   incoming stack argument is re-loaded at its point of use, which is why the
   original has no frame and re-loads 0x28 twice). Two things stay wrong:
   - the branch. The original is `mfc2 $2,$24; nop; bgtz $2,body; nop; b end;
     nop`, i.e. the false path is its own block that jumps to the epilogue.
     gcc 2.6.0, 2.7.2 and 2.7.2-cdk all fold an empty else / empty then / goto
     pair / do-while(0) / for(;;)+break into one `blez`, so no C if-shape tried
     produces it. Confirmed again: grep of every matched function in the repo
     (all 2046 objects under asm/<bin>/matchings) finds no bgtz followed by a
     plain b, so nothing in this tree compiles to that shape. It is the hand-written GTE
     library style, cf. RotTransPers4/RotAverage4, which carry the same
     `cfc2 $3,$31 ... or` flag idiom. Allocation also differs (retail keeps flg
     in $3 and reads opz into $2; gte_stopz_val's "=r" gives a0/v1).
   - allocation: gcc picks $3 for the opz/return and $4 (a0) for the first flag
     read; the original has $2 and $3, and it groups the three lazy `lw`s of
     sxy0-2 before the mfc2 (gcc keeps them in the branch's own block and then
     maspsx has to insert load-delay nops after them).
   Uncommenting this needs the four macros already added to inline_c.h
   (gte_stopz_val, gte_stflg_val_stack, gte_stsw_val_stack,
   gte_stotz_val_stack); RotTransPers4 and RotAverage4 need the same ones.
long RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                      long *sxy0, long *sxy1, long *sxy2, long *sxy3,
                      long *p, long *otz, long *flag) {
    long rtv, flg, flg2;

    gte_ldv3(v0, v1, v2);
    gte_nop();
    gte_rtpt_b();
    gte_stflg_val_stack(flag, flg);
    gte_nclip_b();
    gte_stopz_val(rtv);
    if (rtv > 0) {
        gte_stsxy3(sxy0, sxy1, sxy2);
        gte_ldv0(v3);
        gte_nop();
        gte_rtps_b();
        gte_stsxy(sxy3);
        gte_stflg_val(flg2);
        gte_stdp(p);
        gte_stsw_val_stack(flag, flg2 | flg);
        gte_avsz4_b();
        gte_stotz_val_stack(otz);
    }
    return rtv;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_b", TransposeMatrix);
/* best attempt 92.81%. Every instruction, its ORDER, the lh rather than
   lhu on the 0x10 halfword and the reuse of the p[0] temp for p[3] and of the
   p[1] temp for the 0x10 load all match the original; the only difference is
   which register each live value sits in, so nothing at 100% is reachable
   without retail's allocation:
     original: v0 = return copy, a1 = store base, t1/t2/t3 = the three temps
     gcc:      v0 = return copy AND store base, a1/a2/v1 = the temps
   gcc 2.7.2 copies a1 into v0 at entry (that part is right) and then folds
   every store onto v0, freeing a1 for the first temp. Keeping a1 live as the
   store base needs four more simultaneously live values in v1/a2/a3/t0, and no
   source shape tried produces them: pointer locals for the word stores, a
   separate short pointer for the halfword stores, a ret local, direct casts and
   2/3/4 temp variables all give the same 92.81%. A try_flags sweep over gcc
   2.6.0 / 2.7.2 / 2.7.2-cdk x -O1..-O3 x -G4/-G8 x mcpu 3000/r8000 also gives
   92.81% on every one of the 30 combinations, so the preset is not the issue.
   The 3-temp ordering below is the one that reaches 92.81%. The earlier 65.00%
   attempts used 4 temp variables; with 3, gcc reuses the p[0] temp for p[3] and
   the p[1] temp for the 0x10 halfword exactly like the original.
MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1) {
    MATRIX *r = m1;
    long *q = (long *)m1;
    short *qs = (short *)m1;
    long a, b, c;

    a = ((long *)m0)[0];
    b = ((long *)m0)[1];
    q[1] = a;
    q[0] = b;
    qs[0] = a;
    c = ((long *)m0)[2];
    a = ((long *)m0)[3];
    q[3] = c;
    q[2] = a;
    qs[6] = b;
    qs[4] = c;
    b = ((short *)m0)[8];
    qs[2] = a;
    qs[8] = b;
    return r;
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
