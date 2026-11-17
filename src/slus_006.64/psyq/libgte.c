#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Functions transcribed as the asm Sony wrote (the gte macros cannot express
// these): VectorNormal, VectorNormalSS, func_80048DD8, plus the LoadAverage*
// bodies in psyq/libgte_loadaverage. Points a
// transcription must observe here: maspsx aborts on a `0x..` displacement inside
// inline asm (`sw $8, 0x0($t7)`); write it decimal. maspsx also appends a nop
// after every `jal` unless it has seen a `.set noreorder`, so blocks that call out
// open with `.set\tnoreorder` and close with `.set\treorder` to leave the
// assembler state as they found it. GAS has no `sqr`/`gpf`/`mvmva` mnemonics:
// spell those as `.word` (gte_*_b's encodings).
//
// This file covers 0x393C4-0x39778 only: MatrixNormal's tail pad word at 0x39778
// is an `asm` subsegment in config/slus_006.64.yaml and psyq/libgte_loadaverage
// continues from 0x3977C.
//
// Every stub left is Sony's hand-written vector.s: splat flags each one
// `Handwritten function` and flags every GTE op inside it `handwritten
// instruction`. What is left does not convert, for one of these reasons
// (tools/scripts/convertible.py reports the first two):
//   - nops after `.size`, which C cannot emit: the padding sits after gcc's own
//     `jr $ra`, so it can be reproduced neither by real C nor by a trailing asm
//     block (gcc appends its epilogue after every asm): InitGeom,
//     SquareRoot0. A pad can instead be carved out of the unit's range as its own
//     `asm` subsegment, the way 0x39A08 is for LoadAverageCol; psyq/libgte_tail's
//     header lists the stubs still waiting on that.
//   - dead code: InvSquareRoot's `andi $t0,$v0,1` (never read), and handwritten
//     `nop`s in the `beq`/`bltz` delay slots where reorder mode puts the next
//     instruction
//   - a private calling convention, operands and results in $t0-$t2 with $ra
//     parked in $a3: MatrixNormal (its pad is carved, so only this stands in the
//     way), and VectorNormalS, which branches into the
//     middle of VectorNormalSS (the label lives in the C) and so cannot carry an
//     epilogue of its own.
//
// GTE registers packed by hand around three MVMVAs (lhu + lw + lui 0xFFFF0000
// then and/or) are not a barrier: CompMatrix in psyq/libgte_tail is the same
// shape as SetMulMatrix and transcribes. When the original's return value move sits in the `jr` delay
// slot, pin the asm output to the argument register (`__asm__("$6")`) so gcc
// emits the copy itself; pinning it to $2 lets gcc place it at entry instead.

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

long VectorNormal(VECTOR *v0, VECTOR *v1) {
    register long t0 __asm__("$8");
    register long t1 __asm__("$9");
    register long t2 __asm__("$10");
    register long ret __asm__("$2");

    __asm__ volatile ("lw\t$8, 0( $4 )\n\t"
                      "lw\t$9, 4( $4 )\n\t"
                      "lw\t$10, 8( $4 )\n\t"
                      "addu\t$7, $31, $zero\n\t"
                      "jal\tfunc_80048DD8\n\t"
                      "addu\t$31, $7, $zero"
                      : "=r"(t0), "=r"(t1), "=r"(t2) : : "$7", "memory");
    v1->vx = t0;
    v1->vy = t1;
    v1->vz = t2;
    return ret;
}

long VectorNormalSS(SVECTOR *v0, SVECTOR *v1) {
    register long t0 __asm__("$8");
    register long t1 __asm__("$9");
    register long t2 __asm__("$10");
    register long ret __asm__("$2");

    __asm__ volatile ("lh\t$8, 0( $4 )\n\t"
                      "lh\t$9, 2( $4 )\n\t"
                      "lh\t$10, 4( $4 )\n\t"
                      "nop\n\t"
                      "addu\t$7, $31, $zero\n\t"
                      /* VectorNormalS branches here instead of repeating the body. */
                      ".L80048DBC:\n\t"
                      "jal\tfunc_80048DD8\n\t"
                      "addu\t$31, $7, $zero"
                      : "=r"(t0), "=r"(t1), "=r"(t2) : : "$7", "memory");
    v1->vx = t0;
    v1->vy = t1;
    v1->vz = t2;
    return ret;
}

/* The private convention VectorNormal/VectorNormalS/VectorNormalSS call: the
   vector arrives in $8-$10 and comes back normalised in $8-$10. */
void func_80048DD8(void) {
    __asm__ volatile (".set\tnoreorder\n\t"
                      "mtc2\t$8, $9\n\t"
                      "mtc2\t$9, $10\n\t"
                      "mtc2\t$10, $11\n\t"
                      "nop\n\t"
                      ".word\t0x4aa00428\n\t"
                      "mfc2\t$11, $25\n\t"
                      "mfc2\t$12, $26\n\t"
                      "mfc2\t$13, $27\n\t"
                      "add\t$11, $11, $12\n\t"
                      "add\t$2, $11, $13\n\t"
                      "mtc2\t$2, $30\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mfc2\t$3, $31\n\t"
                      "addiu\t$at, $zero, -2\n\t"
                      "and\t$3, $3, $at\n\t"
                      "addiu\t$14, $zero, 31\n\t"
                      "sub\t$14, $14, $3\n\t"
                      "addi\t$11, $3, -24\n\t"
                      "bltz\t$11, .L80048E34\n\t"
                      "sra\t$14, $14, 1\n\t"
                      "b\t.L80048E40\n\t"
                      "sllv\t$12, $2, $11\n\t"
                      ".L80048E34:\n\t"
                      "addiu\t$11, $zero, 24\n\t"
                      "sub\t$11, $11, $3\n\t"
                      "srav\t$12, $2, $11\n\t"
                      ".L80048E40:\n\t"
                      "addi\t$12, $12, -64\n\t"
                      "sll\t$12, $12, 1\n\t"
                      "lui\t$13, %%hi(D_80056B94)\n\t"
                      "addu\t$13, $13, $12\n\t"
                      "lh\t$13, %%lo(D_80056B94)($13)\n\t"
                      "nop\n\t"
                      "mtc2\t$13, $8\n\t"
                      "mtc2\t$8, $9\n\t"
                      "mtc2\t$9, $10\n\t"
                      "mtc2\t$10, $11\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      ".word\t0x4b90003d\n\t"
                      "mfc2\t$8, $25\n\t"
                      "mfc2\t$9, $26\n\t"
                      "mfc2\t$10, $27\n\t"
                      "srav\t$8, $8, $14\n\t"
                      "srav\t$9, $9, $14\n\t"
                      "srav\t$10, $10, $14\n\t"
                      ".set\treorder"
                      : : : "$1", "$2", "$3", "$8", "$9", "$10", "$11", "$12",
                          "$13", "$14", "memory");
}

/* Sony's MatrixNormal, transcribed from MSC02.OBJ. It normalises the two rows
   it keeps in R11R22R33 with OP (outer product) and runs the three result
   triples through func_80048DD8, whose operands and results ride in $8-$10 with
   $ra parked in $a3 (see psyq/libgte). The block is in noreorder mode so the
   three `jal` delay slots come out as retail has them (nop, nop, the last
   `addu`), and it ends before the third row's stores so gcc's own epilogue can
   put the last one in the `jr` delay slot. The two pointers stay in $a0/$a1, so
   the block spells them out (as VectorNormal does) and the C stores the third
   row through `n`. */
void MatrixNormal(MATRIX *m, MATRIX *n) {
    register long t0 __asm__("$8");
    register long t1 __asm__("$9");
    register long t2 __asm__("$10");

    __asm__ volatile (".set\tnoreorder\n\t"
                      "lh\t$8, 0( $4 )\n\t"
                      "lh\t$9, 2( $4 )\n\t"
                      "lh\t$10, 4( $4 )\n\t"
                      "lh\t$11, 6( $4 )\n\t"
                      "lh\t$12, 8( $4 )\n\t"
                      "lh\t$13, 10( $4 )\n\t"
                      "cfc2\t$2, $0\n\t"
                      "cfc2\t$3, $2\n\t"
                      "cfc2\t$6, $4\n\t"
                      "ctc2\t$8, $0\n\t"
                      "ctc2\t$9, $2\n\t"
                      "ctc2\t$10, $4\n\t"
                      "mtc2\t$13, $11\n\t"
                      "mtc2\t$11, $9\n\t"
                      "mtc2\t$12, $10\n\t"
                      "nop\n\t"
                      ".word\t0x4b78000c\n\t"
                      "mfc2\t$15, $25\n\t"
                      "mfc2\t$24, $26\n\t"
                      "mfc2\t$25, $27\n\t"
                      "ctc2\t$11, $0\n\t"
                      "ctc2\t$12, $2\n\t"
                      "ctc2\t$13, $4\n\t"
                      "nop\n\t"
                      ".word\t0x4b78000c\n\t"
                      "mtc2\t$11, $0\n\t"
                      "mtc2\t$12, $1\n\t"
                      "mtc2\t$13, $2\n\t"
                      "mfc2\t$8, $25\n\t"
                      "mfc2\t$9, $26\n\t"
                      "mfc2\t$10, $27\n\t"
                      "ctc2\t$2, $0\n\t"
                      "ctc2\t$3, $2\n\t"
                      "ctc2\t$6, $4\n\t"
                      "addu\t$7, $31, $zero\n\t"
                      "jal\tfunc_80048DD8\n\t"
                      "nop\n\t"
                      "sh\t$8, 0( $5 )\n\t"
                      "sh\t$9, 2( $5 )\n\t"
                      "sh\t$10, 4( $5 )\n\t"
                      "mfc2\t$8, $0\n\t"
                      "mfc2\t$9, $1\n\t"
                      "mfc2\t$10, $2\n\t"
                      "jal\tfunc_80048DD8\n\t"
                      "nop\n\t"
                      "sh\t$8, 6( $5 )\n\t"
                      "sh\t$9, 8( $5 )\n\t"
                      "sh\t$10, 10( $5 )\n\t"
                      "addu\t$8, $15, $zero\n\t"
                      "addu\t$9, $24, $zero\n\t"
                      "jal\tfunc_80048DD8\n\t"
                      "addu\t$10, $25, $zero\n\t"
                      "addu\t$31, $7, $zero\n\t"
                      ".set\treorder"
                      : "=r"(t0), "=r"(t1), "=r"(t2)
                      :
                      : "$2", "$3", "$6", "$7", "$11", "$12", "$13", "$15",
                        "$24", "$25", "memory");
    n->m[2][0] = t0;
    n->m[2][1] = t1;
    n->m[2][2] = t2;
}
