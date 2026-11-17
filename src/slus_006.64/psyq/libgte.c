#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Functions transcribed as the asm Sony wrote (the gte macros cannot express
// these): VectorNormal, VectorNormalSS, PushMatrix, PopMatrix, func_80048DD8,
// SetMulMatrix, plus the LoadAverage*/ApplyMatrix* bodies further down. Points a
// transcription must observe here: maspsx aborts on a `0x..` displacement inside
// inline asm (`sw $8, 0x0($t7)`); write it decimal. maspsx also appends a nop
// after every `jal` unless it has seen a `.set noreorder`, so blocks that call out
// open with `.set\tnoreorder` and close with `.set\treorder` to leave the
// assembler state as they found it. GAS has no `sqr`/`gpf`/`mvmva` mnemonics:
// spell those as `.word` (gte_*_b's encodings).
//
// Every stub left is Sony's hand-written vector.s: splat flags each one
// `Handwritten function` and flags every GTE op inside it `handwritten
// instruction`. What is left does not convert, for one of these reasons
// (tools/scripts/convertible.py reports the first two):
//   - nops after `.size`, which C cannot emit: the padding sits after gcc's own
//     `jr $ra`, so it can be reproduced neither by real C nor by a trailing asm
//     block (gcc appends its epilogue after every asm): MatrixNormal, InitGeom,
//     SquareRoot0. A pad can instead be carved out of the unit's range as its own
//     `asm` subsegment, the way 0x39A08 is for LoadAverageCol; psyq/libgte_tail's
//     header lists the stubs still waiting on that.
//   - dead code: InvSquareRoot's `andi $t0,$v0,1` (never read), and handwritten
//     `nop`s in the `beq`/`bltz` delay slots where reorder mode puts the next
//     instruction
//   - a private calling convention, operands and results in $t0-$t2 with $ra
//     parked in $a3: MatrixNormal, and VectorNormalS, which branches into the
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

void LoadAverageShort12(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2) {
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


void LoadAverageShort0(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2) {
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

/* The 3-channel twin of LoadAverageByte, transcribed the same way. The nop the
   shipped object pads this function's tail with is the `asm` subsegment at
   0x39A08 in config/slus_006.64.yaml, not something this C can carry. */
void LoadAverageCol(u_char *v0, u_char *v1, long p0, long p1, u_char *v2) {
    long flg;

    __asm__ volatile ( "lbu\t$8, 0( %0 );" "lbu\t$9, 1( %0 );" "lbu\t$10, 2( %0 );"
                       "mtc2\t%1, $8;" "mtc2\t$8, $9;" "mtc2\t$9, $10;"
                       "mtc2\t$10, $11"
                       : : "r"(v0), "r"(p0) : "$8", "$9", "$10" );
    gte_nop();
    gte_gpf0_b();
    __asm__ volatile ( "lbu\t$8, 0( %0 );" "lbu\t$9, 1( %0 );" "lbu\t$10, 2( %0 )"
                       : : "r"(v1) : "$8", "$9", "$10" );
    gte_stflg_val_gpr(flg);
    gte_lddp(p1);
    __asm__ volatile ( "mtc2\t$8, $9;" "mtc2\t$9, $10;" "mtc2\t$10, $11;"
                       "addiu\t$11, $0, 12"
                       : : : "$8", "$9", "$10", "$11" );
    gte_gpl0_b();
    __asm__ volatile ( "lw\t$13, %0;" "mfc2\t$8, $25;" "mfc2\t$9, $26;"
                       "mfc2\t$10, $27;" "srav\t$8, $8, $11;"
                       "srav\t$9, $9, $11;" "srav\t$10, $10, $11;"
                       "sb\t$8, 0( $13 );" "sb\t$9, 1( $13 );"
                       "sb\t$10, 2( $13 )"
                       : : "m"(v2) : "$8", "$9", "$10", "$11", "$13", "memory" );
}
