#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotTransPers3 alone, for the same reason: its tail pad is three words at
// 0x3AED0 (an `asm` subsegment), so the file covers 0x3AE7C-0x3AED0.

long RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1,
                   long *sxy2, long *p, long *flag) {
    register long *q1 __asm__("$8");
    register long *q2 __asm__("$9");
    register long *qp __asm__("$10");
    register long *qf __asm__("$11");
    long flg;
    long sz3;

    gte_ldv3(v0, v1, v2);
    gte_nop();
    gte_rtpt_b();
    gte_ld_arg_ptr4(sxy1, sxy2, p, flag, q1, q2, qp, qf);
    gte_stsxy3(sxy0, q1, q2);
    gte_stdp(qp);
    gte_stflg_val(flg);
    gte_stsz3_val(sz3);
    *qf = flg;
    return sz3 >> 2;
}
