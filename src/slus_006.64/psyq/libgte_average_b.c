#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q libgte from RotAverageNclip4 on. RotAverage4 (the function before it) is
// padded to 16 bytes in the shipped .OBJ, so the nop at 0x3B038 is an `asm`
// subsegment in config/slus_006.64.yaml and this file starts at 0x3B03C.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_b", RotAverageNclip4);
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

/* Sony's vector.c. Retail keeps two pseudos for m1: the return copy in v0,
   made at entry and dead until the `jr`, and the store base in a1. gcc's cse
   merges them -- whichever register the return takes over becomes the store
   base, which scores 95.62% -- so the copy is made by the asm block instead,
   and the three running temps are pinned to the original's t1-t3, where gcc
   prefers v1/a2/a3. */
MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1) {
    register MATRIX *pRet __asm__("$2");
    register long a __asm__("$9");
    register long b __asm__("$10");
    register long c __asm__("$11");
    long *q = (long *)m1;
    short *qs = (short *)m1;

    __asm__ volatile ("addu\t%0, $5, $0" : "=r"(pRet));

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
    return pRet;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_b", RotMatrixYXZ);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_b", RotMatrixZYX);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_b", RotMatrixX);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_b", RotMatrixY);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_average_b", RotMatrixZ);

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
