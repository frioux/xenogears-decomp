#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// RotTransPers4, between NormalClip's three tail pad words at 0x3AF30 and its own
// two at 0x3AFB4 (both `asm` subsegments in config/slus_006.64.yaml), so this
// file covers 0x3AF3C-0x3AFB4. See psyq/libgte_nclip.

long RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                   long *sxy0, long *sxy1, long *sxy2, long *sxy3, long *p,
                   long *flag) {
    register long *q0 __asm__("$8");
    register long *q1 __asm__("$9");
    register long *q2 __asm__("$10");
    register long flg1 __asm__("$3");
    register long flg2 __asm__("$8");
    long sz3;

    gte_ldv3(v0, v1, v2);
    gte_nop();
    gte_rtpt_b();
    __asm__ volatile ("lw\t%0, 16($sp);" "lw\t%1, 20($sp);" "lw\t%2, 24($sp)"
                      : "=r"(q0), "=r"(q1), "=r"(q2));
    gte_stsxy3(q0, q1, q2);
    gte_stflg_val(flg1);
    gte_ldv0(v3);
    gte_nop();
    gte_rtps_b();
    __asm__ volatile ("lw\t%0, 28($sp);" "lw\t%1, 32($sp);" "lw\t%2, 36($sp)"
                      : "=r"(q0), "=r"(q1), "=r"(q2));
    gte_stsxy(q0);
    gte_stdp(q1);
    gte_stflg_val(flg2);
    gte_stsz3_val(sz3);
    flg2 |= flg1;
    *q2 = flg2;
    return sz3 >> 2;
}
