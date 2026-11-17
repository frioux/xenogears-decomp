#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// Psy-Q libgte from RotTrans on, as far as RotTransPers4: it is its own
// translation unit because the two functions before it, RotTransPers and
// RotTransPers3, carry tail pad words that C cannot emit. The pads, and the two
// words at 0x3AFB4 that pad RotTransPers4's tail, are `asm` subsegments in
// config/slus_006.64.yaml.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotTrans);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", NormalClip);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_xform", RotTransPers4);
/* 100.00% (120 bytes) in report mode with the body at the end of this comment,
   but it cannot be un-parked until the two tail pad words at 0x3AFB4 are carved
   into their own `asm` subsegment, which needs one more .c file for the range
   from RotTrans on - over this round's three-file budget. Two things make it
   work, both inherited from RotTransPers3 next door: load each group of three
   stack-passed pointers inside one asm block and pin them to t0-t2, because gcc
   otherwise hoists those loads to the function top; and pin the flag values,
   flg1 in $3 (it has to survive RTPS) and flg2 in $8, which is why the merge is
   spelled `flg2 |= flg1;` - writing `flg2 | flg1` puts the or destination in
   v1 instead of t0. The two three-pointer loads here are gte_ld_arg_ptr4's
   shape (inline_c.h) with this signature's slot offsets.
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
*/
