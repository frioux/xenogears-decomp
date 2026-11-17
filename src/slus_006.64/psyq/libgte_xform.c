#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// The rest of Psy-Q libgte from RotTrans on (0x3AEDC to the end of the unit).
// It is its own translation unit because the two functions before it,
// RotTransPers and RotTransPers3, carry tail pad words that C cannot emit; the
// pads are `asm` subsegments in config/slus_006.64.yaml. RotTransPers4 still has
// its own two pad words at 0x3AFB4 inside this range, so it stays INCLUDE_ASM.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotTrans);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", NormalClip);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotTransPers4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotAverage4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotAverageNclip4);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", TransposeMatrix);
// best attempt 93.75% (64 bytes, the original's size and its exact instruction
// multiset). NOTE the original is a real Psy-Q library object: its 16 words are
// byte-for-byte the TransposeMatrix of ~/xw/psyq-sigs/sig40.txt and sig43.txt
// (LIBGTE/FGO_00.OBJ), so the source was C and the residual is allocation only:
//   original: v0 = entry copy of a1 (the return value, dead until jr ra), a0 =
//             load base, a1 = store base (still live at the end), t1/t2/t3 temps
//   'return m0': v0 = entry copy of a0 and it takes over the load base  93.75%
//   'return m1': v0 = entry copy of a1 but cse folds every store onto it, which
//                frees a1 for a temp (a1/a2/v1 become the temps)      92.81%
// i.e. retail has TWO pseudos holding m1: the return copy in v0 and the store
// base in a1. cse always merges them (the set's destination wins), so the store
// base follows the return register; retail's compiler did not merge, which is a
// cse/allocator difference, not a source shape.
// Retried: a do-while(0) around the whole store cluster (same 92.81), gcc 2.6.0 /
// 2.7.2 / 2.7.2-cdk at -O1/-O2/-O3 - all 92.81 or 93.75. The earlier worker's
// 4-minute permute found nothing from the 93.75 body.
// Next lever, untested here: sis40/sig43 = Psy-Q 4.0/4.3, which is gcc 2.8.0 /
// 2.8.1 territory (AGENTS.md: Psy-Q 4.3 -> 2.8.0). Score this body with
// tools/scripts/try_flags.sh --gcc gcc-2.8.1-psx once that toolchain is present;
// a 2.8 cse reordering is the only remaining explanation for the split pseudo.
// Same registers/widths: the lh on 0x10(a0) is a short load, the sh stores
// truncate a long temp, which is what the body below already writes.
/*
MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1) {
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
    return m0;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotMatrixYXZ);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotMatrixZYX);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotMatrixX);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotMatrixY);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotMatrixZ);

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
