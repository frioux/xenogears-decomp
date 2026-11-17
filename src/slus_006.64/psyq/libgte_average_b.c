#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q libgte from RotAverageNclip4 through RotMatrixX. RotAverage4 (the
// function before it) is padded to 16 bytes in the shipped .OBJ, so the nop at
// 0x3B038 is an `asm` subsegment in config/slus_006.64.yaml and this file starts
// at 0x3B03C; it ends at 0x3B7E4, where RotMatrixX's own two pad words are that
// file's next `asm` subsegment and psyq/libgte_average_c begins.

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

extern long D_800523F0[]; // one entry per 1/4096 turn, (cos << 16) | sin

/* Sony's vector.s, transcribed like ScaleMatrix: gcc emits `mult` (not the
   original's `multu`), never merges the trig lookup's sign handling, and reads
   the table through its own registers, so a C body scores 15.53%. The original
   masks the angle before the sign test picks the branch and negates sin in the
   negative branch (RotMatrixY negates in the positive one), and pulls sin and
   cos out of the single table word with sll/sra and sra. */
MATRIX *RotMatrixX(long r, MATRIX *m) {
    register MATRIX *pRet __asm__("$2");

    __asm__ volatile (".set\tnoreorder\n\t"
                      "addu\t$15, $4, $0\n\t"
                      "addu\t$2, $5, $0\n\t"
                      "bgez\t$15, .LRMXpos\n\t"
                      "andi\t$25, $15, 4095\n\t"
                      "negu\t$15, $15\n\t"
                      "bgez\t$15, .LRMXneg\n\t"
                      "andi\t$15, $15, 4095\n\t"
                      ".LRMXneg:\n\t"
                      "sll\t$24, $15, 2\n\t"
                      "lui\t$25, %%hi(D_800523F0)\n\t"
                      "addu\t$25, $25, $24\n\t"
                      "lw\t$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll\t$14, $25, 16\n\t"
                      "sra\t$14, $14, 16\n\t"
                      "negu\t$9, $14\n\t"
                      "j\t.LRMXjoin\n\t"
                      "sra\t$8, $25, 16\n\t"
                      ".LRMXpos:\n\t"
                      "sll\t$24, $25, 2\n\t"
                      "lui\t$25, %%hi(D_800523F0)\n\t"
                      "addu\t$25, $25, $24\n\t"
                      "lw\t$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll\t$24, $25, 16\n\t"
                      "sra\t$9, $24, 16\n\t"
                      "sra\t$8, $25, 16\n\t"
                      ".LRMXjoin:\n\t"
                      "lh\t$10, 6($5)\n\t"
                      "lh\t$13, 12($5)\n\t"
                      "multu\t$8, $10\n\t"
                      "lh\t$11, 8($5)\n\t"
                      "lh\t$14, 14($5)\n\t"
                      "mflo\t$24\n\t"
                      "lh\t$12, 10($5)\n\t"
                      "lh\t$15, 16($5)\n\t"
                      "multu\t$9, $13\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo\t$25\n\t"
                      "subu\t$25, $24, $25\n\t"
                      "sra\t$24, $25, 12\n\t"
                      "multu\t$8, $11\n\t"
                      "sh\t$24, 6($5)\n\t"
                      "nop\n\t"
                      "mflo\t$24\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "multu\t$9, $14\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo\t$25\n\t"
                      "subu\t$25, $24, $25\n\t"
                      "sra\t$24, $25, 12\n\t"
                      "multu\t$8, $12\n\t"
                      "sh\t$24, 8($5)\n\t"
                      "nop\n\t"
                      "mflo\t$24\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "multu\t$9, $15\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo\t$25\n\t"
                      "subu\t$25, $24, $25\n\t"
                      "sra\t$24, $25, 12\n\t"
                      "multu\t$9, $10\n\t"
                      "sh\t$24, 10($5)\n\t"
                      "nop\n\t"
                      "mflo\t$24\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "multu\t$8, $13\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo\t$25\n\t"
                      "addu\t$25, $24, $25\n\t"
                      "sra\t$24, $25, 12\n\t"
                      "multu\t$9, $11\n\t"
                      "sh\t$24, 12($5)\n\t"
                      "nop\n\t"
                      "mflo\t$24\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "multu\t$8, $14\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo\t$25\n\t"
                      "addu\t$25, $24, $25\n\t"
                      "sra\t$24, $25, 12\n\t"
                      "multu\t$9, $12\n\t"
                      "sh\t$24, 14($5)\n\t"
                      "nop\n\t"
                      "mflo\t$24\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "multu\t$8, $15\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo\t$25\n\t"
                      "addu\t$25, $24, $25\n\t"
                      "sra\t$24, $25, 12\n\t"
                      "sh\t$24, 16($5)\n\t"
                      ".set\treorder"
                      : "=r"(pRet) : : "$8", "$9", "$10", "$11", "$12", "$13",
                          "$14", "$15", "$24", "$25", "memory");
    return pRet;
}
