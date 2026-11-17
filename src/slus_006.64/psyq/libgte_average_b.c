#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q libgte from RotAverageNclip4 through RotMatrixYXZ. RotAverage4 (the
// function before it) is padded to 16 bytes in the shipped .OBJ, so the nop at
// 0x3B038 is an `asm` subsegment in config/slus_006.64.yaml and this file starts
// at 0x3B03C; it ends at 0x3B3B8, where RotMatrixYXZ's own pad word is the next
// `asm` subsegment and psyq/libgte_average_b_2 begins.

/* Sony's vector.s has this one marked "Handwritten function" in the shipped
   .OBJ: the GTE macro spellings of the 3.x library (RTPT/NCLIP/RTPS/AVSZ4 with
   their load-delay nops) and the `bgtz`+`b` guard around the second half have no
   C form -- gcc folds every if-shape into one `blez` and the *_val macros in
   inline_c.h allocate differently. Transcribed as retail's instruction stream,
   so the guard and the lazy re-load of *flag off 0x28($sp) stay as written.
   The four GTE commands are spelled as inline_c.h spells them (c2 / .word): the
   report build does not include macro.inc, so the retail mnemonics do not
   assemble. The epilogue `jr` is gcc's. */
long RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                      long *sxy0, long *sxy1, long *sxy2, long *sxy3,
                      long *p, long *otz, long *flag) {
    register long rtv __asm__("$2");

    __asm__ volatile (".set\tnoreorder\n\t"
                      "lwc2	$0, 0($4)\n\t"
                      "lwc2	$1, 4($4)\n\t"
                      "lwc2	$2, 0($5)\n\t"
                      "lwc2	$3, 4($5)\n\t"
                      "lwc2	$4, 0($6)\n\t"
                      "lwc2	$5, 4($6)\n\t"
                      "nop\n\t"
                      "c2	0x280030\n\t"
                      "lw	$8, 40($29)\n\t"
                      "cfc2	$3, $31\n\t"
                      "nop\n\t"
                      "sw	$3, 0($8)\n\t"
                      ".word	0x4b400006\n\t"
                      "lw	$8, 16($29)\n\t"
                      "lw	$9, 20($29)\n\t"
                      "lw	$10, 24($29)\n\t"
                      "mfc2	$2, $24\n\t"
                      "nop\n\t"
                      "bgtz	$2, .LRAN4body\n\t"
                      "nop\n\t"
                      "b	.LRAN4end\n\t"
                      "nop\n\t"
                      ".LRAN4body:\n\t"
                      "swc2	$12, 0($8)\n\t"
                      "swc2	$13, 0($9)\n\t"
                      "swc2	$14, 0($10)\n\t"
                      "lwc2	$0, 0($7)\n\t"
                      "lwc2	$1, 4($7)\n\t"
                      "nop\n\t"
                      "c2	0x180001\n\t"
                      "lw	$8, 28($29)\n\t"
                      "lw	$9, 32($29)\n\t"
                      "lw	$10, 40($29)\n\t"
                      "swc2	$14, 0($8)\n\t"
                      "cfc2	$11, $31\n\t"
                      "swc2	$8, 0($9)\n\t"
                      "or	$11, $11, $3\n\t"
                      "sw	$11, 0($10)\n\t"
                      ".word	0x4b68002e\n\t"
                      "lw	$9, 36($29)\n\t"
                      "mfc2	$8, $7\n\t"
                      "nop\n\t"
                      "sw	$8, 0($9)\n\t"
                      ".LRAN4end:\n\t"
                      ".set\treorder"
                      : "=r"(rtv) : : "$3", "$8", "$9", "$10", "$11", "memory");
    return rtv;
}

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

extern long D_800523F0[]; // one entry per 1/4096 turn, (cos << 16) | sin

/* Sony's vector.s, transcribed like RotMatrixX: gcc emits `mult` where the
   original has `multu` and reads the trig table through its own registers.
   RotMatrixYXZ and RotMatrixZYX are the same routine with the three lookups
   (r->vx, r->vy, r->vz) and the nine products in a different order: each axis
   masks its angle before the sign test picks a branch, and the products are
   accumulated as the matrix is filled. The epilogue `jr` is gcc's. */
MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m) {
    register MATRIX *pRet __asm__("$2");

    __asm__ volatile (".set\tnoreorder\n\t"
                      "lh	$15, 0($4)\n\t"
                      "addu	$2, $5, $0\n\t"
                      "bgez	$15, .LRMYXZvxpos\n\t"
                      "andi	$25, $15, 4095\n\t"
                      "negu	$15, $15\n\t"
                      "bgez	$15, .LRMYXZvxneg\n\t"
                      "andi	$15, $15, 4095\n\t"
                      ".LRMYXZvxneg:\n\t"
                      "sll	$24, $15, 2\n\t"
                      "lui	$25, %%hi(D_800523F0)\n\t"
                      "addu	$25, $25, $24\n\t"
                      "lw	$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll	$14, $25, 16\n\t"
                      "sra	$14, $14, 16\n\t"
                      "negu	$11, $14\n\t"
                      "j	.LRMYXZvxjoin\n\t"
                      "sra	$8, $25, 16\n\t"
                      ".LRMYXZvxpos:\n\t"
                      "sll	$24, $25, 2\n\t"
                      "lui	$25, %%hi(D_800523F0)\n\t"
                      "addu	$25, $25, $24\n\t"
                      "lw	$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll	$24, $25, 16\n\t"
                      "sra	$11, $24, 16\n\t"
                      "negu	$14, $11\n\t"
                      "sra	$8, $25, 16\n\t"
                      ".LRMYXZvxjoin:\n\t"
                      "lh	$15, 2($4)\n\t"
                      "nop\n\t"
                      "bgez	$15, .LRMYXZvypos\n\t"
                      "andi	$25, $15, 4095\n\t"
                      "negu	$15, $15\n\t"
                      "bgez	$15, .LRMYXZvyneg\n\t"
                      "andi	$15, $15, 4095\n\t"
                      ".LRMYXZvyneg:\n\t"
                      "sll	$24, $15, 2\n\t"
                      "lui	$25, %%hi(D_800523F0)\n\t"
                      "addu	$25, $25, $24\n\t"
                      "lw	$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll	$24, $25, 16\n\t"
                      "sra	$24, $24, 16\n\t"
                      "negu	$12, $24\n\t"
                      "j	.LRMYXZvyjoin\n\t"
                      "sra	$9, $25, 16\n\t"
                      ".LRMYXZvypos:\n\t"
                      "sll	$24, $25, 2\n\t"
                      "lui	$25, %%hi(D_800523F0)\n\t"
                      "addu	$25, $25, $24\n\t"
                      "lw	$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll	$24, $25, 16\n\t"
                      "sra	$12, $24, 16\n\t"
                      "sra	$9, $25, 16\n\t"
                      ".LRMYXZvyjoin:\n\t"
                      "multu	$12, $8\n\t"
                      "lh	$15, 4($4)\n\t"
                      "sh	$14, 10($5)\n\t"
                      "mflo	$24\n\t"
                      "sra	$14, $24, 12\n\t"
                      "nop\n\t"
                      "multu	$9, $8\n\t"
                      "sh	$14, 4($5)\n\t"
                      "bgez	$15, .LRMYXZvzpos\n\t"
                      "andi	$25, $15, 4095\n\t"
                      "mflo	$24\n\t"
                      "sra	$14, $24, 12\n\t"
                      "sh	$14, 16($5)\n\t"
                      "negu	$15, $15\n\t"
                      "bgez	$15, .LRMYXZvzneg\n\t"
                      "andi	$15, $15, 4095\n\t"
                      ".LRMYXZvzneg:\n\t"
                      "sll	$24, $15, 2\n\t"
                      "lui	$25, %%hi(D_800523F0)\n\t"
                      "addu	$25, $25, $24\n\t"
                      "lw	$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll	$24, $25, 16\n\t"
                      "sra	$24, $24, 16\n\t"
                      "negu	$13, $24\n\t"
                      "j	.LRMYXZvzjoin\n\t"
                      "sra	$10, $25, 16\n\t"
                      ".LRMYXZvzpos:\n\t"
                      "mflo	$15\n\t"
                      "sra	$14, $15, 12\n\t"
                      "sh	$14, 16($5)\n\t"
                      "sll	$24, $25, 2\n\t"
                      "lui	$25, %%hi(D_800523F0)\n\t"
                      "addu	$25, $25, $24\n\t"
                      "lw	$25, %%lo(D_800523F0)($25)\n\t"
                      "nop\n\t"
                      "sll	$24, $25, 16\n\t"
                      "sra	$13, $24, 16\n\t"
                      "sra	$10, $25, 16\n\t"
                      ".LRMYXZvzjoin:\n\t"
                      "multu	$13, $8\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$14, $15, 12\n\t"
                      "sh	$14, 6($5)\n\t"
                      "multu	$10, $8\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$14, $15, 12\n\t"
                      "nop\n\t"
                      "multu	$12, $11\n\t"
                      "sh	$14, 8($5)\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$24, $15, 12\n\t"
                      "nop\n\t"
                      "multu	$24, $13\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$14, $15, 12\n\t"
                      "nop\n\t"
                      "multu	$9, $10\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$25, $15, 12\n\t"
                      "addu	$15, $25, $14\n\t"
                      "multu	$9, $13\n\t"
                      "sh	$15, 0($5)\n\t"
                      "nop\n\t"
                      "mflo	$14\n\t"
                      "sra	$25, $14, 12\n\t"
                      "negu	$15, $25\n\t"
                      "multu	$24, $10\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$14\n\t"
                      "sra	$25, $14, 12\n\t"
                      "addu	$14, $15, $25\n\t"
                      "multu	$9, $11\n\t"
                      "sh	$14, 2($5)\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$24, $15, 12\n\t"
                      "nop\n\t"
                      "multu	$24, $10\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$14, $15, 12\n\t"
                      "nop\n\t"
                      "multu	$12, $13\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$15\n\t"
                      "sra	$25, $15, 12\n\t"
                      "addu	$15, $25, $14\n\t"
                      "multu	$12, $10\n\t"
                      "sh	$15, 14($5)\n\t"
                      "nop\n\t"
                      "mflo	$14\n\t"
                      "sra	$25, $14, 12\n\t"
                      "negu	$15, $25\n\t"
                      "multu	$24, $13\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mflo	$14\n\t"
                      "sra	$25, $14, 12\n\t"
                      "addu	$14, $15, $25\n\t"
                      "sh	$14, 12($5)\n\t"
                      ".set\treorder"
                      : "=r"(pRet) : : "$8", "$9", "$10", "$11", "$12", "$13",
                          "$14", "$15", "$24", "$25", "memory");
    return pRet;
}
