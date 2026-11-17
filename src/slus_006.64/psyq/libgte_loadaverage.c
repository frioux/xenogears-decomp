#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q libgte from LoadAverage12 to LoadAverageCol. MatrixNormal (before it)
// carries one pad word in retail -- the shipped .OBJ pads every function's tail
// to a 16-byte multiple -- so it is the `asm` subsegment at 0x39778 in
// config/slus_006.64.yaml and this file starts at 0x3977C; LoadAverageCol's own
// pad at 0x39A08 ends it and starts psyq/libgte_mulmatrix0.
//
// Everything here is transcribed as the asm Sony wrote (see psyq/libgte): the
// byte and short transforms hand gcc a register-pinned block because maspsx
// aborts on a `0x..` displacement inside inline asm, so displacements are
// decimal.

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
