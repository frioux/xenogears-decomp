#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// ApplyMatrix and ApplyMatrixSV. It is its own translation unit only because
// MulMatrix2 (the function before ApplyMatrix) carries a pad word in retail --
// the shipped .OBJ pads every function's tail to a 16-byte multiple -- and the
// four bytes at 0x3A4E8 come from the `asm` subsegment in
// config/slus_006.64.yaml; 0x3A598 is ApplyMatrixSV's pad and starts
// psyq/libgte_tail_d. Keep the five files one unit's worth of code.

VECTOR *ApplyMatrix(MATRIX *m, SVECTOR *v0, VECTOR *v1) {
    VECTOR *pRet;

    gte_SetRotMatrix(m);
    gte_ldv0(v0);
    gte_nop();
    gte_mvmva_b(1, 0, 0, 3, 0);
    __asm__ volatile ("swc2\t$25, 0( %1 );" "swc2\t$26, 4( %1 );" "swc2\t$27, 8( %1 );"
                      "addu\t%0, %1, $zero"
                      : "=r"(pRet)
                      : "r"(v1)
                      : "memory");
    return pRet;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_tail_b", ApplyMatrixSV);

